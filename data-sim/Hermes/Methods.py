import random
import json
import math
import time
import serial
from PySide6.QtCore import QThread, Signal

# Constants
AIR_DENSITY = 1.225
MAX_VOLTAGE = 2.7
FAULT_PROBABILITY = 0.5
C_RATE = random.uniform(1, 5)  # Allow discharge from 1C to 5C


class Cell:
    """Represents a single cell in the hierarchy with attributes for simulation."""

    def __init__(self, block, rack, module, cell_id):
        self.id = f"B{(block.id):03}-{(rack.id):03}-{(module.id):03}-C{(cell_id):03}"
        self.voltage = 0.0
        self.current = 0.0
        self.temperature = 25.0
        self.status = "Normal"
        self.faults = {
            "overcharge": False,
            "over_discharge": False,
            "overheating": False,
            "over_current": False,
            "thermal_runaway": False
        }

    def update_status(self):
        """Randomly assigns a fault with a small probability."""
        if random.random() < FAULT_PROBABILITY:
            fault_type = random.choice(list(self.faults.keys()))
            self.faults[fault_type] = True
            self.status = "Compromised"

    def update_attributes(self, power, temperature_variance=0):
        """Update cell's attributes based on power input."""

        def update_voltage(self, power_per_cell):
            self.voltage = min(MAX_VOLTAGE, power_per_cell / self.current)

    def get_fault(self):
        """Return the fault type as a string if any, otherwise 'Normal'."""
        for fault, is_active in self.faults.items():
            if is_active:
                return fault.capitalize()  # Capitalize the fault name (e.g., 'Overcharge')
        return "Normal"  # If no faults are active, return 'Normal'


class Module:
    """Represents a module, containing multiple cells."""

    def __init__(self, block, rack, module_id, num_cells, deviceID):
        self.id = module_id
        self.rack_id = rack.id
        self.deviceID = deviceID
        self.isProcessed = False  # Flag for processed state
        self.cells = [Cell(block, rack, self, i + 1) for i in range(num_cells)]

    def get_data(self):
        """Get structured JSON data for this module."""
        return {
            "rack_id": self.rack_id,
            "module_id": self.id,
            "deviceID": self.deviceID,
            "cells": [
                {
                    **cell.__dict__,
                    "faults": cell.get_fault()  # Replace the faults with a string (e.g., 'Overcharge', 'Normal')
                }
                for cell in self.cells
            ]
        }


class Rack:
    """Represents a rack, containing multiple modules."""

    def __init__(self, block, rack_id, num_modules, num_cells_per_module):
        self.id = rack_id
        self.deviceID = 0
        self.modules = [Module(block, self, f"M{(i + 1):03}", num_cells_per_module, deviceID=(i + 1)) for i in
                        range(num_modules)]

    def get_module_data(self):
        """Get data for all modules in this rack."""
        return [module.get_data() for module in self.modules]


class Block:
    """Represents a block, containing multiple racks."""

    def __init__(self, block_id, num_racks, num_modules_per_rack, num_cells_per_module):
        self.id = block_id
        self.racks = [Rack(self, f"R{(i + 1):03}", num_modules_per_rack, num_cells_per_module) for i in
                      range(num_racks)]

    def get_rack_data(self):
        """Get data for all racks in this block."""
        return [rack.get_module_data() for rack in self.racks]


class Methods:
    """Methods for managing simulations."""

    def __init__(self, core, config):
        self.core = core
        self.config = config
        self.blocks = []
        self.wind_speed = 0.0
        self.weather_data = []

    def generate_cells(self):
        """Generate the blocks, racks, modules, and cells."""
        for b in range(1, self.config["blocks"] + 1):
            block = Block(b, self.config["racks_per_block"], self.config["modules_per_rack"],
                          self.config["cells_per_module"])
            self.blocks.append(block)  # This appends Block, not Module

    def simulate_weather(self):
        """Simulate weather by generating wind speed."""
        self.wind_speed = random.uniform(self.config["min_wind_speed"], self.config["max_wind_speed"])
        self.weather_data.append(self.wind_speed)

    def calculate_power_output(self):
        """Calculate power output based on wind speed and energy coefficient."""
        swept_area = math.pi * (self.config["blade_length"] / 2) ** 2
        return 0.5 * AIR_DENSITY * swept_area * (self.wind_speed ** 3) * self.config["energy_coefficient"] * \
            self.config["plant_size"]

    def simulate_energy_distribution(self):
        """Simulate energy distribution across cells."""
        total_power = self.calculate_power_output()  # Calculate total power
        total_cells = sum(len(module.cells) for block in self.blocks for rack in block.racks for module in rack.modules)

        if total_cells == 0:
            print("No cells available for power distribution.")
            return

        power_per_cell = total_power / total_cells  # Split power among all cells

        for block in self.blocks:
            for rack in block.racks:
                for module in rack.modules:
                    for cell in module.cells:
                        # Randomized small current within a tight range
                        cell.current = random.uniform(2, 5)
                        # Calculate voltage based on power and current
                        cell.voltage = min(MAX_VOLTAGE, power_per_cell / cell.current)
                        # Apply some temperature variation
                        cell.temperature += random.uniform(-2, 2)
                        # Update fault status
                        cell.update_status()

    def store_data(self):
        """Store simulation data in JSON files."""
        data = {"blocks": [block.get_rack_data() for block in self.blocks]}
        self.filename = self.config["filename"]
        with open(self.filename + ".json", "w") as json_file:
            json.dump(data, json_file, indent=4)

    def send_data_to_esp32(self):
        """Start the serial worker threads for live updates, distributing across available COM ports."""
        com_ports = self.core.com_ports
        if not com_ports:
            print("No COM ports available.")
            return

        num_ports = len(com_ports)
        print(f"Started {num_ports} serial workers distributing across {num_ports} COM ports.")

        self.serial_workers = []  # Store worker references
        com_ports.sort()  # Ensure COM ports are in order (COM1, COM2, etc.)

        worker_counter = 0  # To ensure every module gets processed across COM ports

        for block in self.blocks:  # Iterate through each block
            print(f"Processing Block {block.id}")

            for rack in block.racks:  # Iterate through racks in the current block
                print(f"Processing Rack {rack.id} in Block {block.id}")

                for i, module in enumerate(rack.modules):  # Iterate through modules in the current rack
                    com_index = worker_counter % num_ports  # Use modulo to distribute modules evenly
                    com_port = com_ports[com_index]  # Assign the COM port based on the index

                    print(f"Assigning Module {module.id} from Block {block.id} to COM{com_port}")

                    # Pass the block, rack, and module to the worker
                    worker = SerialWorker(f"COM{com_port}", block, rack, module)
                    worker.response_received.connect(self.core.receive_data_from_methods)
                    worker.start()
                    self.serial_workers.append(worker)

                    worker_counter += 1  # Increment the counter to ensure every module is assigned

                    # Debugging output: show each module processed
                    print(
                        f"Worker {worker_counter} started for Module {module.id} in Block {block.id}, Rack {rack.id} on COM{com_port}")

    def run_simulation(self):
        """Run the full simulation process."""
        self.generate_cells()
        self.simulate_weather()
        self.simulate_energy_distribution()
        self.store_data()
        self.send_data_to_esp32()
        print("Simulation complete.")


class SerialWorker(QThread):
    response_received = Signal(str)  # Signal to send data to UI

    def __init__(self, com_port, block, rack, module):
        super().__init__()
        self.com_port = com_port
        self.block = block
        self.rack = rack
        self.module = module
        self.running = True  # Control flag for stopping safely

    def run(self):
        """Send data to ESP32 in a thread and update UI in real-time."""
        try:
            # Retry logic for port access
            retry_count = 3
            while retry_count > 0:
                try:
                    with serial.Serial(self.com_port, 9600, timeout=1) as ser:
                        self.response_received.emit(f"Connected to {self.com_port}")

                        if self.module:
                            start_time = time.perf_counter()
                            # If module is provided, send its data
                            module_data = self.module.get_data()
                            # json_data = json.dumps(module_data)  # Convert to JSON string
                            # json_data = '{"rack_id": "R001", "module_id": "M001", "deviceID": 1, "cells": [{"id": "B001-R001-M001-C001", "voltage": 2.7, "current": 3.662480574465921, "temperature": 24.01304740637377, "status": "Normal", "faults": "Normal"}, {"id": "B001-R001-M001-C002", "voltage": 2.7, "current": 4.228569303975679, "temperature": 24.073489120782547, "status": "Compromised", "faults": "Overcharge"}, {"id": "B001-R001-M001-C003", "voltage": 2.7, "current": 4.999730437881932, "temperature": 26.99408467524904, "status": "Normal", "faults": "Normal"}, {"id": "B001-R001-M001-C004", "voltage": 2.7, "current": 4.695379128960707, "temperature": 26.67950588797423, "status": "Normal", "faults": "Normal"}, {"id": "B001-R001-M001-C005", "voltage": 2.7, "current": 4.35868823443618, "temperature": 23.57284683084601, "status": "Normal", "faults": "Normal"}, {"id": "B001-R001-M001-C006", "voltage": 2.7, "current": 2.0340567725961747, "temperature": 24.724736469578453, "status": "Compromised", "faults": "Overcharge"}, {"id": "B001-R001-M001-C007", "voltage": 2.7, "current": 4.385575606436539, "temperature": 24.194147529291346, "status": "Compromised", "faults": "Over_current"}, {"id": "B001-R001-M001-C008", "voltage": 2.7, "current": 3.313222750338175, "temperature": 23.123393414820473, "status": "Normal", "faults": "Normal"}, {"id": "B001-R001-M001-C009", "voltage": 2.7, "current": 3.242357332476904, "temperature": 26.90300698426893, "status": "Compromised", "faults": "Thermal_runaway"}, {"id": "B001-R001-M001-C010", "voltage": 2.7, "current": 4.144146963824967, "temperature": 25.939625967715514, "status": "Normal", "faults": "Normal"}]}'
                            json_data = '{"test": "value"}'
                            # json.loads(json_data)  # Validate JSON
                            # print(json_data)

                            encodedJson = (json_data + '\n').encode('utf-8')

                            print(f"Sending Bytes: {encodedJson}")  # Print the byte array.

                            ser.write(encodedJson)  # Use \r\n instead of \n
                            print("From the JSON ", json_data.encode('utf-8'))
                            print(f"Sent data for Module {self.module.id} to ESP32")
                            self.response_received.emit(f"Sent data for Module {self.module.id} to ESP32")
                            self.response_received.emit(json_data)
                            end_time = time.perf_counter()
                            execution_time = end_time - start_time
                            print("Execution time for module: ", execution_time)
                            time.sleep(1)
                        # Wait for response and handle it
                        if ser.in_waiting > 0:
                            response = ser.readline().decode(errors="ignore").strip()
                            if response:  # Check to avoid emitting empty responses
                                self.response_received.emit(f"ESP32: {response}")  # Forward valid response
                        break  # Exit the retry loop on success
                except serial.SerialException as e:
                    retry_count -= 1
                    if retry_count == 0:
                        self.response_received.emit(f"Serial Error: {e}")
                    else:
                        self.response_received.emit(f"Retrying {self.com_port} due to error: {e}")
                        time.sleep(1)  # Delay before retrying
        except Exception as e:
            self.response_received.emit(f"Unexpected Error: {e}")

    def stop(self):
        """Stop the worker thread."""
        self.running = False
        self.quit()
        self.wait()
