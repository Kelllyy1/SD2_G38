import math
import json
from Methods import Methods  # Correct import for the Methods class
from Methods import Methods
from PySide6.QtCore import QObject, Signal

import matplotlib

matplotlib.use('Qt5Agg')  # Explicitly use Qt5Agg if needed.
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas  # Use Qt5Agg
from matplotlib.figure import Figure

class Core(QObject):
    response_received = Signal(str)  # Define a signal to send ESP32 data
    data_ready = Signal(list)
    config_updated = Signal()
    def __init__(self, parent=None):
        # Initialize variables
        super().__init__(parent)  # Ensure QObject is initialized
        self.filename = None
        self.methods = Methods
        #self.prob = self.methods.FAULT_PROBABILITY
        self.plant_size = None
        self.location = None
        self.blade_length = None
        self.energy_coefficient = None
        self.min_speed = None
        self.max_wind_speed = None
        self.block = None
        self.rack = None
        self.module = None
        self.cell = None
        self.num_turbines = None
        self.com_ports = []  # Store COM ports
        self.config = {}
        self.statistics = {}
        self.specific_fault_counts = {}

    def receive_data(self, validated_data):
        # Store the data in the class variables
        self.plant_size = validated_data.get("plant_size")

        if validated_data.get("location") == "Onshore":
            self.location = float(1.293)
        elif validated_data.get("location") == "Offshore":
            self.location = float(1.225)
        self.blade_length = validated_data.get("blade_length")
        self.energy_coefficient = validated_data.get("energy_coefficient")
        # convert from KMPH to MPS
        # ms = (kmh * 1000) / 3600
        self.min_speed = (validated_data.get("min_speed") * 1000) / 3600
        self.max_wind_speed = (validated_data.get("max_wind_speed") * 1000) / 3600
        self.block = validated_data.get("block")
        self.rack = validated_data.get("rack")
        self.module = validated_data.get("module")
        self.cell = validated_data.get("cell")
        self.num_turbines = validated_data.get("num_turbines")
        self.filename = validated_data.get("filename")

        self.config = {
            "plant_size": self.plant_size,
            "location": self.location,
            "blade_length": self.blade_length,
            "energy_coefficient": self.energy_coefficient,
            "min_wind_speed": self.min_speed,
            "max_wind_speed": self.max_wind_speed,
            "blocks": self.block,
            "racks_per_block": self.rack,
            "modules_per_rack": self.module,
            "cells_per_module": self.cell,
            "num_turbines": self.num_turbines,
            "filename": self.filename
        }


        self.methods = Methods(self, self.config)

        power = (1 / 2) * self.plant_size * self.location * (self.min_speed ** 3) * (
                    math.pi * (self.blade_length)) * self.energy_coefficient
        print(power)
        # Print the stored data to confirm it's correct
        print("From the core: ", self.config)
        self.print_stored_data()
        print("Wooohooo! Check here! ", self.config["modules_per_rack"])
        self.config_updated.emit()
        # self.run_simulation()  # Call run_simulation here!

    def receive_com_ports(self, com_ports):
        """Receive COM port data from the interface."""
        self.com_ports = com_ports
        print("Received COM ports in Core:", self.com_ports)

        # You can now use self.com_ports for further processing

    def print_stored_data(self):
        print("Stored Data:")
        print(f"Plant size: {self.plant_size}")
        print(f"Location: {self.location}")
        print(f"Blade length: {self.blade_length}")
        print(f"Energy coefficient: {self.energy_coefficient}")
        print(f"Min speed: {self.min_speed}")
        print(f"Max wind speed: {self.max_wind_speed}")
        print(f"Block: {self.block}")
        print(f"Rack: {self.rack}")
        print(f"Module: {self.module}")
        print(f"Cell: {self.cell}")
        print(f"Number of turbines: {self.num_turbines}")

    def run_simulation(self):
        """Runs the full simulation process."""
        self.methods = Methods(self, self.config)  # Initialize Methods when Core is initialized

        self.methods.generate_cells()
        self.methods.simulate_weather()
        self.methods.simulate_energy_distribution()
        self.methods.store_data()

        #self.methods.send_data_to_esp32()
        print("Simulation complete. Data stored successfully.")
        self.methods.send_data_to_esp32()
        #self.data_ready.emit()

    def receive_data_from_methods(self, data):
        """This method receives data from Methods and emits a signal to update the UI."""
        print("reached!")
        self.response_received.emit(data)  # Send data to the interface

    def runSimulationWithoutESP(self):
        """Runs the simulation without ESP32 communication."""
        self.methods.generate_cells()
        self.methods.simulate_weather()
        self.methods.simulate_energy_distribution()
        self.methods.store_data()  # Store results
        print("Simulation complete. Skipping ESP32 communication.")


    def process_file(self, file_path):
        # Parse the JSON file
        with open(file_path, 'r') as f:
            data = json.load(f)
        # Check the type of data
        #print(type(data))  # This should print <class 'list'> if the file is structured correctly
        #print(data)  # Print out the first few items to verify it's a list of dictionaries
        # Calculate statistics
        self.calculate_statistics(data)

    def calculate_statistics(self, data):
        if not isinstance(data, dict) or 'blocks' not in data:
            print("Invalid data format. Expected dictionary with 'blocks' key.")
            return

        all_cells = []
        for block_list_level1 in data['blocks']:
            for block_list_level2 in block_list_level1:
                for block in block_list_level2:
                    if isinstance(block, dict) and 'cells' in block:
                        all_cells.extend(block['cells'])

        total_cells = len(all_cells)

        if total_cells == 0:
            self.statistics = {
                "total_cells": 0,
                "compromised_cells": 0,
                "avg_voltage": 0,
                "avg_current": 0,
                "avg_temp": 0
            }
            self.specific_fault_counts = {  # Added this line
                "Overcharge": 0,
                "Overdischarge": 0,
                "Overcurrent": 0,
                "Overheating": 0,
                "Thermal_runaway": 0
            }
            print(self.statistics)
            print(self.specific_fault_counts)  # Added this line
            return

        compromised_cells = sum(1 for cell in all_cells if cell.get("status") == "Compromised")
        avg_voltage = sum(cell.get("voltage", 0) for cell in all_cells) / total_cells
        avg_current = sum(cell.get("current", 0) for cell in all_cells) / total_cells
        avg_temp = sum(cell.get("temperature", 0) for cell in all_cells) / total_cells

        faults = [cell.get("faults") for cell in all_cells if cell.get("faults")]
        # Initialize fault counts
        fault_types = ["Overcharge", "Overdischarge", "Overcurrent", "Overheating", "Thermal_runaway"]
        self.fault_types = fault_types

        # Calculate specific fault counts
        specific_fault_counts = {
            "Overcharge": 0,
            "Overdischarge": 0,
            "Overcurrent": 0,
            "Overheating": 0,
            "Thermal_runaway": 0
        }
        for cell in all_cells:
            fault = cell.get("faults")
            if fault in specific_fault_counts:
                specific_fault_counts[fault] += 1

        self.statistics = {
            "Total Cells Generated": total_cells,
            "Number of Compromised Cells": compromised_cells,
            "Average Voltage": f"{avg_voltage:.2f}",
            "Average Current": f"{avg_current:.2f}",
            "Average Temperature": f"{avg_temp:.2f}"
        }
        print("Specific Fault Counts Dictionary:", specific_fault_counts)
        print("Fault Types List:", fault_types)
        fault_counts_list = [specific_fault_counts[fault] for fault in fault_types]
        self.fault_counts_list = fault_counts_list
        print(fault_counts_list)
        self.data_ready.emit(fault_counts_list)  # Emit list of fault counts
        print(self.statistics)
        print(self.specific_fault_counts)  # Added this line

class MplCanvas(FigureCanvas):
    def __init__(self, core):
        self.core = core  # Store reference to Core
        self.fig = Figure()
        self.ax = self.fig.add_subplot(111)
        print("From canvas ")
        FigureCanvas.__init__(self, self.fig)
        self.update_plot()  # Initialize with empty/placeholder graph

    def update_plot(self):
        """Clear the plot and redraw with new data."""
        self.ax.clear()  # Clear previous plot
        self.fig.patch.set_facecolor("#fffdfa")
        self.ax.patch.set_facecolor('#fffdfa')


        if hasattr(self.core, "specific_fault_counts") and self.core.specific_fault_counts:
            fault_types = list(self.core.specific_fault_counts.keys())
            fault_counts = list(self.core.specific_fault_counts.values())
            print("From canvas ", fault_counts)

            self.ax.xaxis.set_tick_params(labelcolor='none')

            self.ax.bar(fault_types, fault_counts, color='skyblue')
            self.ax.set_title('Fault Distribution in Simulation', fontsize=16)
            self.ax.set_xlabel('Fault Types', fontsize=12)
            self.ax.set_ylabel('Count of Faults', fontsize=12)

        self.draw()  # Redraw the figure
