from PySide6.QtGui import QPixmap, QFontDatabase
from PySide6.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel, QSizePolicy, QHBoxLayout, QLineEdit, \
    QRadioButton, QPushButton
from PySide6.QtCore import QFile, QTextStream, Qt


# Function to load QSS file
def load_stylesheet(file_path):
    file = QFile(file_path)
    if not file.open(QFile.ReadOnly):
        raise Exception(f"Cannot open file: {file_path}")
    stream = QTextStream(file)
    stylesheet = stream.readAll()
    file.close()
    return stylesheet


app = QApplication([])
Icons = QFontDatabase.addApplicationFont("/resources/images/Font_005.ttf")
Icons = QFontDatabase.addApplicationFont("/resources/images/LEMONMILK-Medium.otf")

# Load QSS
stylesheet = load_stylesheet("../resources/style_test_001.qss")
app.setStyleSheet(stylesheet)

# Create the main window
window = QWidget()
window.setObjectName("window")
window.setWindowTitle("Centered Layout Example")

#form details


# left
left = QWidget()
left.setObjectName("left")

left_layout = QHBoxLayout()
left_layout.setAlignment(Qt.AlignCenter)  # Center canvas inside easel
left.setLayout(left_layout)

#sidebar
sidebar = QWidget()
sidebar.setObjectName("sidebar")

sidebar_layout = QVBoxLayout()
sidebar_layout.setAlignment(Qt.AlignCenter)
sidebar.setLayout(sidebar_layout)

left_layout.addWidget(sidebar, stretch=1)

sidebar.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

#innards
sidebarInnards = QWidget()
sidebarInnards.setObjectName("sidebarInnards")

sidebarInnards_layout = QVBoxLayout()
sidebarInnards_layout.setAlignment(Qt.AlignCenter)
sidebarInnards.setLayout(sidebarInnards_layout)

sidebarLogo = QLabel("H")
sidebarLogo.setObjectName("sidebarLogo")
#logoImage = QPixmap("C:/Users/Erica/Pictures/Senior Design/Hermes/Hermes/resources/images/Logo_002.png")
#sidebarLogo.setPixmap(logoImage)
#sidebarLogo.setFixedSize(150, 144)
sidebarLogo.setScaledContents(True)
sidebarLogo.setAlignment(Qt.AlignVCenter | Qt.AlignHCenter)
sidebarLogo.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

#sidebarTitle = QLabel("Hermes")
#sidebarTitle.setObjectName("sidebarTitle")
#sidebarTitle.setAlignment(Qt.AlignCenter)  # Align text in center

sidebarDescription = QLabel("Generate fully customizable plant and weather configurations to simulate the behavior of a BESS-BMS.")
sidebarDescription.setObjectName("sidebarDescription")
sidebarDescription.setAlignment(Qt.AlignLeft)  # Align text in center

sidebarButton1 = QPushButton("Save")
sidebarButton1.setObjectName("sidebarButton1")

sidebarButton2 = QPushButton("Button2")
sidebarButton2.setObjectName("sidebarButton2")

sidebarButton3 = QPushButton("Button3")
sidebarButton3.setObjectName("sidebarButton3")

sidebarCollapse = QPushButton("❰")
sidebarCollapse.setObjectName("collapse")


sidebarSpace = QLabel("")
sidebarSpace.setObjectName("sidebarSpace")
sidebarSpace.setAlignment(Qt.AlignCenter)

sidebar_layout.addWidget(sidebarInnards, stretch=1)
#sidebar.setFixedHeight(600)

sidebarInnards_layout.addWidget(sidebarCollapse, stretch=1)
sidebarInnards_layout.addWidget(sidebarLogo, stretch=3)
#sidebarInnards_layout.addWidget(sidebarTitle, stretch=1)
sidebarInnards_layout.addWidget(sidebarDescription, stretch=3)
sidebarInnards_layout.addWidget(sidebarButton1, stretch=1, alignment=Qt.AlignCenter)
sidebarInnards_layout.addWidget(sidebarButton2, stretch=1, alignment=Qt.AlignCenter)
sidebarInnards_layout.addWidget(sidebarButton3, stretch=1, alignment=Qt.AlignCenter)
sidebarInnards_layout.addWidget(sidebarSpace, stretch=3)

sidebar_layout.addWidget(sidebarInnards, stretch=1)

sidebarInnards_layout.setSpacing(0)

sidebarInnards.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

sidebarSpace.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

# right
right = QWidget()
right.setObjectName("right")

right_layout = QHBoxLayout()
right_layout.setAlignment(Qt.AlignCenter)  # Center canvas inside easel
right.setLayout(right_layout)

# topElement
topRight_layout = QVBoxLayout()
topRight = QWidget()
topRight.setObjectName("topRight")
topRight.setLayout(topRight_layout)
topRight_layout.setSpacing(0)

# bottomElementt
bottomRight_layout = QVBoxLayout()
bottomRight = QWidget()
bottomRight.setObjectName("bottomRight")
bottomRight.setLayout(bottomRight_layout)
bottomRight_layout.setSpacing(0)


# interact
interact_layout = QHBoxLayout()
interact = QWidget()
interact.setObjectName("interact")
interact.setLayout(interact_layout)

interactInput = QLineEdit()
interactInput.setPlaceholderText("Filename")
interactInput.setObjectName("interactInput")
interactInput.setAlignment(Qt.AlignLeft)  # Align text in center
interact_layout.addWidget(interactInput, stretch=9)

interactButton = QPushButton("Submit")
interactButton.setObjectName("interactButton")
interact_layout.addWidget(interactButton, stretch=2)

# tab
tab_layout = QVBoxLayout()
tab = QWidget()
tab.setObjectName("tab")
tab.setLayout(tab_layout)

# box - holds plant, turbine and weather
box_layout = QVBoxLayout()
box = QWidget()
box.setObjectName("box")
box.setLayout(box_layout)

# tabBottom
tabBottom_layout = QVBoxLayout()
tabBottom = QWidget()
tabBottom.setObjectName("tabBottom")
tabBottom.setLayout(tabBottom_layout)

# box - holds plant, turbine and weather
boxBottom_layout = QVBoxLayout()
boxBottom = QWidget()
boxBottom.setObjectName("boxBottom")
boxBottom.setLayout(boxBottom_layout)

#top right
#plant
plant_layout = QVBoxLayout()
plant = QWidget()
plant.setObjectName("plant")
plant.setLayout(plant_layout)

plantTitle = QLabel("Plant Size")
plantTitle.setObjectName("plantTitle")
plantTitle.setAlignment(Qt.AlignCenter)  # Align text in center

plantIcon = QLabel("A")
plantIcon.setObjectName("plantIcon")
plantIcon.setAlignment(Qt.AlignCenter)  # Align text in center

plantParameters = QLabel("Parameters")
plantParameters.setObjectName("plantParameters")
plantParameters.setAlignment(Qt.AlignCenter)  # Align text in center

plantInputs_layout = QHBoxLayout()
plantInputs = QWidget()
plantInputs.setObjectName("plantInputs")
plantInputs.setLayout(plantInputs_layout)

plantInputsLeft_layout = QVBoxLayout()
plantInputsLeft = QWidget()
plantInputsLeft.setObjectName("plantInputsLeft")
plantInputsLeft.setLayout(plantInputsLeft_layout)

plantInputsLeftNumberTitle = QLabel("Number of Turbines")
plantInputsLeftNumberTitle.setObjectName("plantInputsLeftNumberTitle")
plantInputsLeftNumberTitle.setAlignment(Qt.AlignLeft)

plantInputsLeftNumberInput = QLineEdit()
plantInputsLeftNumberInput.setObjectName("plantInputsLeftNumberInput")
plantInputsLeftNumberInput.setAlignment(Qt.AlignLeft)

plantInputsRight_layout = QVBoxLayout()
plantInputsRight = QWidget()
plantInputsRight.setObjectName("plantInputsRight")
plantInputsRight.setLayout(plantInputsRight_layout)

plantInputsLeftLocationTitle = QRadioButton("On-shore")
plantInputsLeftLocationTitle.setObjectName("plantInputsLeftLocationTitle")

plantInputsLeftLocationInput = QRadioButton("Off-shore")
plantInputsLeftLocationInput.setObjectName("plantInputsLeftLocationInput")

plantInputsLeftNumberInput = QLineEdit()
plantInputsLeftNumberInput.setPlaceholderText("Max = 1000")
plantInputsLeftNumberInput.setObjectName("plantInputsLeftNumberInput")
plantInputsLeftNumberInput.setAlignment(Qt.AlignLeft)

#for the turbine
turbine_layout = QVBoxLayout()
turbine = QWidget()
turbine.setObjectName("turbine")
turbine.setLayout(turbine_layout)

turbineTitle = QLabel("Turbine Configuration")
turbineTitle.setObjectName("turbineTitle")
turbineTitle.setAlignment(Qt.AlignCenter)  # Align text in center

turbineIcon = QLabel("B")
turbineIcon.setObjectName("turbineIcon")
turbineIcon.setAlignment(Qt.AlignCenter)  # Align text in center

turbineParameters = QLabel("Parameters")
turbineParameters.setObjectName("turbineParameters")
turbineParameters.setAlignment(Qt.AlignCenter)  # Align text in center

turbineInputs_layout = QHBoxLayout()
turbineInputs = QWidget()
turbineInputs.setObjectName("turbineInputs")
turbineInputs.setLayout(turbineInputs_layout)

turbineInputsLeft_layout = QVBoxLayout()
turbineInputsLeft = QWidget()
turbineInputsLeft.setObjectName("turbineInputsLeft")
turbineInputsLeft.setLayout(turbineInputsLeft_layout)

turbineInputsLeftNumberTitle = QLabel("Blade Length")
turbineInputsLeftNumberTitle.setObjectName("turbineInputsLeftNumberTitle")
turbineInputsLeftNumberTitle.setAlignment(Qt.AlignLeft)

turbineInputsLeftNumberInput = QLineEdit()
turbineInputsLeftNumberInput.setObjectName("turbineInputsLeftNumberInput")
turbineInputsLeftNumberInput.setAlignment(Qt.AlignLeft)

turbineInputsRight_layout = QVBoxLayout()
turbineInputsRight = QWidget()
turbineInputsRight.setObjectName("turbineInputsRight")
turbineInputsRight.setLayout(turbineInputsRight_layout)

turbineInputsLeftLocationTitle = QLabel("Energy Coefficient")
turbineInputsLeftLocationTitle.setObjectName("turbineInputsLeftLocationTitle")
turbineInputsLeftLocationTitle.setAlignment(Qt.AlignLeft)

turbineInputsLeftLocationInput = QLineEdit()
turbineInputsLeftLocationInput.setPlaceholderText("Max = 0.59")
turbineInputsLeftLocationInput.setObjectName("turbineInputsLeftLocationInput")
turbineInputsLeftLocationInput.setAlignment(Qt.AlignLeft)

turbineInputsLeftNumberInput = QLineEdit()
turbineInputsLeftNumberInput.setPlaceholderText("Max = 120")
turbineInputsLeftNumberInput.setObjectName("turbineInputsLeftNumberInput")
turbineInputsLeftNumberInput.setAlignment(Qt.AlignLeft)

#for the weather
weather_layout = QVBoxLayout()
weather = QWidget()
weather.setObjectName("weather")
weather.setLayout(weather_layout)

weatherTitle = QLabel("Weather")
weatherTitle.setObjectName("weatherTitle")
weatherTitle.setAlignment(Qt.AlignCenter)  # Align text in center

weatherIcon = QLabel("C")
weatherIcon.setObjectName("weatherIcon")
weatherIcon.setAlignment(Qt.AlignCenter)  # Align text in center

weatherParameters = QLabel("Parameters")
weatherParameters.setObjectName("weatherParameters")
weatherParameters.setAlignment(Qt.AlignCenter)  # Align text in center

weatherInputs_layout = QHBoxLayout()
weatherInputs = QWidget()
weatherInputs.setObjectName("weatherInputs")
weatherInputs.setLayout(weatherInputs_layout)

weatherInputsLeft_layout = QVBoxLayout()
weatherInputsLeft = QWidget()
weatherInputsLeft.setObjectName("weatherInputsLeft")
weatherInputsLeft.setLayout(weatherInputsLeft_layout)

weatherInputsLeftNumberTitle = QLabel("Min. Wind Speed")
weatherInputsLeftNumberTitle.setObjectName("weatherInputsLeftNumberTitle")
weatherInputsLeftNumberTitle.setAlignment(Qt.AlignLeft)

weatherInputsLeftNumberInput = QLineEdit()
weatherInputsLeftNumberInput.setObjectName("weatherInputsLeftNumberInput")
weatherInputsLeftNumberInput.setAlignment(Qt.AlignLeft)

weatherInputsRight_layout = QVBoxLayout()
weatherInputsRight = QWidget()
weatherInputsRight.setObjectName("weatherInputsRight")
weatherInputsRight.setLayout(weatherInputsRight_layout)

weatherInputsLeftLocationTitle = QLabel("Max. Wind Speed")
weatherInputsLeftLocationTitle.setObjectName("weatherInputsLeftLocationTitle")
weatherInputsLeftLocationTitle.setAlignment(Qt.AlignLeft)

weatherInputsLeftLocationInput = QLineEdit()
weatherInputsLeftLocationInput.setPlaceholderText("Max = 40")
weatherInputsLeftLocationInput.setObjectName("weatherInputsLeftLocationInput")
weatherInputsLeftLocationInput.setAlignment(Qt.AlignLeft)

weatherInputsLeftNumberInput = QLineEdit()
weatherInputsLeftNumberInput.setPlaceholderText("Min = 0")
weatherInputsLeftNumberInput.setObjectName("weatherInputsLeftNumberInput")
weatherInputsLeftNumberInput.setAlignment(Qt.AlignLeft)

#bttom right / BESS configuration
#block
block_layout = QVBoxLayout()
block = QWidget()
block.setObjectName("block")
block.setLayout(block_layout)

blockIcon = QLabel("D")
blockIcon.setObjectName("blockIcon")
blockIcon.setAlignment(Qt.AlignCenter)  # Align text in center
block_layout.addWidget(blockIcon, stretch=4)

blockTitle = QLabel("Block")
blockTitle.setObjectName("blockTitle")
blockTitle.setAlignment(Qt.AlignLeft)  # Align text in center
block_layout.addWidget(blockTitle, stretch=1)

blockInput = QLineEdit()
blockInput.setObjectName("blockInput")
blockInput.setPlaceholderText("Max = 10")
blockInput.setAlignment(Qt.AlignLeft)  # Align text in center
block_layout.addWidget(blockInput, stretch=1)
#rack
rack_layout = QVBoxLayout()
rack = QWidget()
rack.setObjectName("rack")
rack.setLayout(rack_layout)

rackIcon = QLabel("E")
rackIcon.setObjectName("rackIcon")
rackIcon.setAlignment(Qt.AlignCenter)  # Align text in center
rack_layout.addWidget(rackIcon, stretch=4)

rackTitle = QLabel("Rack")
rackTitle.setObjectName("rackTitle")
rackTitle.setAlignment(Qt.AlignLeft)  # Align text in center
rack_layout.addWidget(rackTitle, stretch=1)

rackInput = QLineEdit()
rackInput.setObjectName("rackInput")
rackInput.setPlaceholderText("Max = 10")
rackInput.setAlignment(Qt.AlignLeft)  # Align text in center
rack_layout.addWidget(rackInput, stretch=1)

#module
module_layout = QVBoxLayout()
module = QWidget()
module.setObjectName("module")
module.setLayout(module_layout)

moduleIcon = QLabel("F")
moduleIcon.setObjectName("moduleIcon")
moduleIcon.setAlignment(Qt.AlignCenter)  # Align text in center
module_layout.addWidget(moduleIcon, stretch=4)

moduleTitle = QLabel("Module")
moduleTitle.setObjectName("moduleTitle")
moduleTitle.setAlignment(Qt.AlignLeft)  # Align text in center
module_layout.addWidget(moduleTitle, stretch=1)

moduleInput = QLineEdit()
moduleInput.setObjectName("moduleInput")
moduleInput.setPlaceholderText("Max = 10")
moduleInput.setAlignment(Qt.AlignLeft)  # Align text in center
module_layout.addWidget(moduleInput, stretch=1)

#cell
cell_layout = QVBoxLayout()
cell = QWidget()
cell.setObjectName("cell")
cell.setLayout(cell_layout)

cellIcon = QLabel("G")
cellIcon.setObjectName("cellIcon")
cellIcon.setAlignment(Qt.AlignCenter)  # Align text in center
cell_layout.addWidget(cellIcon, stretch=4)

cellTitle = QLabel("Cell")
cellTitle.setObjectName("cellTitle")
cellTitle.setAlignment(Qt.AlignLeft)  # Align text in center
cell_layout.addWidget(cellTitle, stretch=1)

cellInput = QLineEdit()
cellInput.setObjectName("cellInput")
cellInput.setPlaceholderText("Max = 10")
cellInput.setAlignment(Qt.AlignLeft)  # Align text in center
cell_layout.addWidget(cellInput, stretch=1)

#adding
#box_layout.addWidget(plant, stretch=1)
#box_layout.addWidget(turbine, stretch=1)
#box_layout.addWidget(weather, stretch=1)

#boxBottom_layout.addWidget(block, stretch=1)
#boxBottom_layout.addWidget(rack, stretch=1)
#boxBottom_layout.addWidget(module, stretch=1)
#boxBottom_layout.addWidget(cell, stretch=1)

boxBottom_layout.setSpacing(0)

titleTop = QLabel("Port Configuration")
titleTop.setObjectName("titleTop")
titleTop.setAlignment(Qt.AlignLeft)  # Align text in center
tab_layout.addWidget(titleTop, stretch=1)

titleBottom = QLabel("Status")
titleBottom.setObjectName("titleBottom")
titleBottom.setAlignment(Qt.AlignLeft)  # Align text in center
tabBottom_layout.addWidget(titleBottom, stretch=1)

#add top right to frame
#adding plant object to window
plant_layout.addWidget(plantTitle, stretch=1)
plant_layout.addWidget(plantIcon, stretch=4)
plant_layout.addWidget(plantInputs, stretch=2)

plantInputs_layout.addWidget(plantInputsLeft, stretch=1)
plantInputsLeft_layout.addWidget(plantInputsLeftNumberTitle, stretch=1)
plantInputsLeft_layout.addWidget(plantInputsLeftNumberInput, stretch=1)

plantInputs_layout.addWidget(plantInputsRight, stretch=1)
plantInputsRight_layout.addWidget(plantInputsLeftLocationTitle, stretch=1)
plantInputsRight_layout.addWidget(plantInputsLeftLocationInput, stretch=1)

#adding turbine objects to window
turbine_layout.addWidget(turbineTitle, stretch=1)
turbine_layout.addWidget(turbineIcon, stretch=4)
turbine_layout.addWidget(turbineInputs, stretch=2)

turbineInputs_layout.addWidget(turbineInputsLeft, stretch=1)
turbineInputsLeft_layout.addWidget(turbineInputsLeftNumberTitle, stretch=1)
turbineInputsLeft_layout.addWidget(turbineInputsLeftNumberInput, stretch=1)

turbineInputs_layout.addWidget(turbineInputsRight, stretch=1)
turbineInputsRight_layout.addWidget(turbineInputsLeftLocationTitle, stretch=1)
turbineInputsRight_layout.addWidget(turbineInputsLeftLocationInput, stretch=1)

#weather
weather_layout.addWidget(weatherTitle, stretch=1)
weather_layout.addWidget(weatherIcon, stretch=4)
weather_layout.addWidget(weatherInputs, stretch=2)

weatherInputs_layout.addWidget(weatherInputsLeft, stretch=1)
weatherInputsLeft_layout.addWidget(weatherInputsLeftNumberTitle, stretch=1)
weatherInputsLeft_layout.addWidget(weatherInputsLeftNumberInput, stretch=1)

weatherInputs_layout.addWidget(weatherInputsRight, stretch=1)
weatherInputsRight_layout.addWidget(weatherInputsLeftLocationTitle, stretch=1)
weatherInputsRight_layout.addWidget(weatherInputsLeftLocationInput, stretch=1)

right_layout.addWidget(topRight, stretch=1)
right_layout.addWidget(bottomRight, stretch=1)

topRight_layout.addWidget(tab, stretch=1)
topRight_layout.addWidget(box, stretch=17)

bottomRight_layout.addWidget(tabBottom, stretch=1)
bottomRight_layout.addWidget(boxBottom, stretch=17)

topRight.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)

#navigation bar

# Main window layout
window_layout = QHBoxLayout()
window_layout.addWidget(left, stretch=1)
window_layout.addWidget(right, stretch=4)
window_layout.setAlignment(Qt.AlignCenter)  # Center container inside the window
window.setLayout(window_layout)

# Set initial size and show window
window.resize(1200, 600)  # Start with a reasonable size
window.show()

app.exec()
