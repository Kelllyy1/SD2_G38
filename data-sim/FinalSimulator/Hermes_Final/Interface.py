import os
import random
import sys
import math
from pathlib import Path

import serial.tools.list_ports
from PySide6.QtGraphs import QAbstractSeries

from PySide6.QtGui import QPixmap, QFontDatabase, QAction, QIcon, QPainter, QBrush, QColor, QFont
from PySide6.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel, QSizePolicy, QHBoxLayout, QLineEdit, \
    QRadioButton, QPushButton, QMenu, QMenuBar, QMessageBox, QMainWindow, QStackedWidget, QScrollArea, QPlainTextEdit, \
    QGraphicsOpacityEffect, QDialog, QDialogButtonBox, QFileDialog, QDoubleSpinBox, QSpinBox
from PySide6.QtCore import QFile, QTextStream, Qt, Signal
from PySide6.QtGui import QIntValidator
from PySide6.QtCharts import QChart, QChartView, QBarSeries, QBarSet, QBarCategoryAxis, QValueAxis, QCategoryAxis, \
    QLegend

import matplotlib

matplotlib.use('Qt5Agg')  # Explicitly use Qt5Agg if needed.
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas  # Use Qt5Agg
from matplotlib.figure import Figure

from Core import Core, MplCanvas
from Methods import Methods

project_path = Path(__file__).resolve().parent
print("Project path:", project_path)

def show_error_popup(message):
    # Create a message box to show the error
    msg = QMessageBox()
    msg.setIcon(QMessageBox.Critical)
    msg.setText("Error")
    msg.setInformativeText(message)
    msg.setWindowTitle("Error")
    msg.exec()


class HermesHome(QWidget):
    def __init__(self, core, parent=None):
        super().__init__(parent)
        self.core = core
        self.init_ui()
        self.load_resources()
        self.setup_menu()
        self.create_layouts()
        self.setup_widgets()
        self.setup_layout()
        self.apply_stylesheet()
        self.setup_actions()
        #self.popup = dialog

    # ---------------------------------- load stylesheet ----------------------------------#
    def load_stylesheet(file_path):
        file = QFile(file_path)
        if not file.open(QFile.ReadOnly):
            raise Exception(f"Cannot open file: {file_path}")
        stream = QTextStream(file)
        stylesheet = stream.readAll()
        file.close()
        return stylesheet

    # ---------------------------------- initialize window ----------------------------------#
    def init_ui(self):
        """Initializes the user interface."""
        self.setObjectName("window")
        self.setWindowTitle(" Hermes")
        self.resize(1200, 600)

    # ---------------------------------- load resources  ----------------------------------#
    def load_resources(self):
        """Loads resources like icons and fonts."""

        print("Project path:", project_path)
        app = QApplication.instance()
        app.setWindowIcon(QIcon(str(project_path) + "resources/images/Logo.ico"))
        font_id = QFontDatabase.addApplicationFont(
            str(project_path) + "/resources/images/HermesIcons.ttf")

        if font_id == -1:
            print("Font failed to load.")
        else:
            print(f"Font loaded successfully with font ID: {font_id}")
        QFontDatabase.addApplicationFont(
            str(project_path) + "/resources/style_hermes.qss")

    def apply_stylesheet(self):
        """Applies the stylesheet to the application."""
        app = QApplication.instance()
        stylesheet = self.load_stylesheet("resources\\style_hermes.qss")
        app.setStyleSheet(stylesheet)

    # ---------------------------------- load navbar  ----------------------------------#
    def setup_menu(self):
        """Sets up the menu bar."""
        self.menu_bar_home = QMenuBar()
        self.menu_bar_home.setObjectName("menu1")
        file_menu_home = QMenu("File", self.menu_bar_home)
        settings_menu_home = QMenu("Settings", self.menu_bar_home)
        help_menu_home = QMenu("Help", self.menu_bar_home)
        self.menu_bar_home.addMenu(file_menu_home)

        new_action_home = QAction("New", self)
        open_action_home = QAction("Open", self)
        save_action_home = QAction("Save", self)
        exit_action_home = QAction("Exit", self)
        exit_action_home.triggered.connect(QApplication.instance().quit)

        preferences_home = QAction("Preferences", self)
        about_action_home = QAction("About", self)
        help_action_home = QAction("Help", self)

        file_menu_home.addAction(open_action_home)
        file_menu_home.addSeparator()
        file_menu_home.addAction(exit_action_home)

        self.menu_bar_home.addMenu(settings_menu_home)
        settings_menu_home.addAction(preferences_home)
        preferences_home.triggered.connect(self.show_settings)

        self.menu_bar_home.addMenu(help_menu_home)
        help_menu_home.addAction(help_action_home)
        help_menu_home.addAction(about_action_home)


    # ---------------------------------- create layouts  ----------------------------------#
    def create_layouts(self):
        """Creates all the layouts used in the application."""
        self.window_home_layout = QVBoxLayout()
        self.main_content_layout = QHBoxLayout()
        self.left_layout = QHBoxLayout()
        self.sidebar_layout = QVBoxLayout()
        self.sidebarInnards_layout = QVBoxLayout()
        self.right_layout = QVBoxLayout()
        self.topRight_layout = QVBoxLayout()
        self.bottomRight_layout = QVBoxLayout()
        self.interact_layout = QHBoxLayout()
        self.tab_layout = QVBoxLayout()
        self.box_layout = QHBoxLayout()
        self.tabBottom_layout = QVBoxLayout()
        self.boxBottom_layout = QHBoxLayout()
        self.plant_layout = QVBoxLayout()
        self.plantInputs_layout = QHBoxLayout()
        self.plantInputsLeft_layout = QVBoxLayout()
        self.plantInputsRight_layout = QVBoxLayout()
        self.turbine_layout = QVBoxLayout()
        self.turbineInputs_layout = QHBoxLayout()
        self.turbineInputsLeft_layout = QVBoxLayout()
        self.turbineInputsRight_layout = QVBoxLayout()
        self.weather_layout = QVBoxLayout()
        self.weatherInputs_layout = QHBoxLayout()
        self.weatherInputsLeft_layout = QVBoxLayout()
        self.weatherInputsRight_layout = QVBoxLayout()
        self.block_layout = QVBoxLayout()
        self.rack_layout = QVBoxLayout()
        self.module_layout = QVBoxLayout()
        self.cell_layout = QVBoxLayout()

    # ---------------------------------- set-up widgets  ----------------------------------#

    def setup_widgets(self):
        """Sets up all the widgets used in the application."""
        self.left = QWidget()
        self.left.setObjectName("left")
        self.sidebar = QWidget()
        self.sidebar.setObjectName("sidebar")
        self.sidebarTitle = QLabel("Hermes")
        self.sidebarTitle.setObjectName("sidebarTitle")
        self.sidebarTitle.setAlignment(Qt.AlignCenter)
        self.sidebarInnards = QWidget()
        self.sidebarInnards.setObjectName("sidebarInnards")
        self.sidebarLogo = QLabel("H")
        self.sidebarLogo.setObjectName("sidebarLogo")
        self.sidebarDescription = QLabel(
            "Generate fully customizable plant and weather configurations to simulate the behavior of a BESS-BMS.")
        self.sidebarDescription.setObjectName("sidebarDescription")
        self.sidebarButton1 = QPushButton("Clear")
        self.sidebarButton1.setObjectName("sidebarButton1")
        self.sidebarButton2 = QPushButton("Random")
        self.sidebarButton2.setObjectName("sidebarButton2")
        self.sidebarButton3 = QPushButton("Button3")
        self.sidebarButton3.setObjectName("sidebarButton3")
        self.sidebarButton1.setVisible(True)
        self.sidebarButton2.setVisible(True)
        self.sidebarButton3.setVisible(False)
        self.sidebarCollapse = QPushButton("❰")
        self.sidebarCollapse.setObjectName("collapse")
        self.sidebarSpace = QLabel("")
        self.sidebarSpace.setObjectName("sidebarSpace")
        self.right = QWidget()
        self.right.setObjectName("right")
        self.topRight = QWidget()
        self.topRight.setObjectName("topRight")
        self.bottomRight = QWidget()
        self.bottomRight.setObjectName("bottomRight")
        self.interact = QWidget()
        self.interact.setObjectName("interact")
        self.interactInput = QLineEdit()
        # placeholder
        self.interactInput.setText("Untitled-01")
        self.interactInput.setPlaceholderText("Filename")
        self.interactInput.setObjectName("interactInput")
        self.interactButton = QPushButton("Next")
        self.interactButton.setObjectName("interactButton")
        self.tab = QWidget()
        self.tab.setObjectName("tab")
        self.box = QWidget()
        self.box.setObjectName("box")
        self.tabBottom = QWidget()
        self.tabBottom.setObjectName("tabBottom")
        self.boxBottom = QWidget()
        self.boxBottom.setObjectName("boxBottom")
        self.plant = QWidget()
        self.plant.setObjectName("plant")
        self.plantTitle = QLabel("Plant Size")
        self.plantTitle.setObjectName("plantTitle")
        self.plantIcon = QLabel("A")
        self.plantIcon.setObjectName("plantIcon")
        self.plantParameters = QLabel("Parameters")
        self.plantParameters.setObjectName("plantParameters")
        self.plantInputs = QWidget()
        self.plantInputs.setObjectName("plantInputs")
        self.plantInputsLeft = QWidget()
        self.plantInputsLeft.setObjectName("plantInputsLeft")
        self.plantInputsLeftNumberTitle = QLabel("Number of Turbines")
        self.plantInputsLeftNumberTitle.setObjectName("plantInputsLeftNumberTitle")
        self.plantInputsLeftNumberInput = QLineEdit()
        # placeholder
        self.plantInputsLeftNumberInput.setText("3")
        self.plantInputsLeftNumberInput.setPlaceholderText("Max = 1000")
        self.plantInputsLeftNumberInput.setObjectName("plantInputsLeftNumberInput")
        self.plantInputsRight = QWidget()
        self.plantInputsRight.setObjectName("plantInputsRight")
        self.plantInputsLeftLocationTitle = QRadioButton("On-shore")
        self.plantInputsLeftLocationTitle.setChecked(True)
        self.plantInputsLeftLocationTitle.setObjectName("plantInputsLeftLocationTitle")
        self.plantInputsLeftLocationInput = QRadioButton("Off-shore")
        self.plantInputsLeftLocationInput.setObjectName("plantInputsLeftLocationInput")
        self.turbine = QWidget()
        self.turbine.setObjectName("turbine")
        self.turbineTitle = QLabel("Turbine Configuration")
        self.turbineTitle.setObjectName("turbineTitle")
        self.turbineIcon = QLabel("B")
        self.turbineIcon.setObjectName("turbineIcon")
        self.turbineParameters = QLabel("Parameters")
        self.turbineParameters.setObjectName("turbineParameters")
        self.turbineInputs = QWidget()
        self.turbineInputs.setObjectName("turbineInputs")
        self.turbineInputsLeft = QWidget()
        self.turbineInputsLeft.setObjectName("turbineInputsLeft")
        self.turbineInputsLeftNumberTitle = QLabel("Blade Length (m)")
        self.turbineInputsLeftNumberTitle.setObjectName("turbineInputsLeftNumberTitle")
        self.turbineInputsLeftNumberInput = QLineEdit()
        # placeholder
        self.turbineInputsLeftNumberInput.setText("80")
        self.turbineInputsLeftNumberInput.setPlaceholderText("Max = 180")
        self.turbineInputsLeftNumberInput.setObjectName("turbineInputsLeftNumberInput")
        self.turbineInputsRight = QWidget()
        self.turbineInputsRight.setObjectName("turbineInputsRight")
        self.turbineInputsLeftLocationTitle = QLabel("Energy Coefficient")
        self.turbineInputsLeftLocationTitle.setObjectName("turbineInputsLeftLocationTitle")
        self.turbineInputsLeftLocationInput = QLineEdit()
        # placeholder
        self.turbineInputsLeftLocationInput.setText(".3")
        self.turbineInputsLeftLocationInput.setPlaceholderText("Max = 0.59")
        self.turbineInputsLeftLocationInput.setObjectName("turbineInputsLeftLocationInput")
        self.weather = QWidget()
        self.weather.setObjectName("weather")
        self.weatherTitle = QLabel("Weather")
        self.weatherTitle.setObjectName("weatherTitle")
        self.weatherIcon = QLabel("C")
        self.weatherIcon.setObjectName("weatherIcon")
        self.weatherParameters = QLabel("Parameters")
        self.weatherParameters.setObjectName("weatherParameters")
        self.weatherInputs = QWidget()
        self.weatherInputs.setObjectName("weatherInputs")
        self.weatherInputsLeft = QWidget()
        self.weatherInputsLeft.setObjectName("weatherInputsLeft")
        self.weatherInputsLeftNumberTitle = QLabel("Min. Wind Speed (km)")
        self.weatherInputsLeftNumberTitle.setObjectName("weatherInputsLeftNumberTitle")
        self.weatherInputsLeftNumberInput = QLineEdit()
        # placeholder
        self.weatherInputsLeftNumberInput.setText("3")
        self.weatherInputsLeftNumberInput.setPlaceholderText("Min = 0")
        self.weatherInputsLeftNumberInput.setObjectName("weatherInputsLeftNumberInput")
        self.weatherInputsRight = QWidget()
        self.weatherInputsRight.setObjectName("weatherInputsRight")
        self.weatherInputsLeftLocationTitle = QLabel("Max. Wind Speed (km)")
        self.weatherInputsLeftLocationTitle.setObjectName("weatherInputsLeftLocationTitle")
        self.weatherInputsLeftLocationInput = QLineEdit()
        # placeholder
        self.weatherInputsLeftLocationInput.setText("4")
        self.weatherInputsLeftLocationInput.setPlaceholderText("Max = 70")
        self.weatherInputsLeftLocationInput.setObjectName("weatherInputsLeftLocationInput")
        self.block = QWidget()
        self.block.setObjectName("block")
        self.blockIcon = QLabel("D")
        self.blockIcon.setObjectName("blockIcon")
        self.blockTitle = QLabel("Block")
        self.blockTitle.setObjectName("blockTitle")
        self.blockInput = QLineEdit()
        # placeholder
        self.blockInput.setText("1")
        self.blockInput.setPlaceholderText("Max = 10")
        self.blockInput.setObjectName("blockInput")
        self.rack = QWidget()
        self.rack.setObjectName("rack")
        self.rackIcon = QLabel("E")
        self.rackIcon.setObjectName("rackIcon")
        self.rackTitle = QLabel("Rack")
        self.rackTitle.setObjectName("rackTitle")
        self.rackInput = QLineEdit()
        # placeholder
        self.rackInput.setText("1")
        self.rackInput.setPlaceholderText("Max = 10")
        self.rackInput.setObjectName("rackInput")
        self.module = QWidget()
        self.module.setObjectName("module")
        self.moduleIcon = QLabel("F")
        self.moduleIcon.setObjectName("moduleIcon")
        self.moduleTitle = QLabel("Module")
        self.moduleTitle.setObjectName("moduleTitle")
        self.moduleInput = QLineEdit()
        # placeholder
        self.moduleInput.setText("16")
        self.moduleInput.setPlaceholderText("Must be n²")
        self.moduleInput.setObjectName("moduleInput")
        self.cell = QWidget()
        self.cell.setObjectName("cell")
        self.cellIcon = QLabel("G")
        self.cellIcon.setObjectName("cellIcon")
        self.cellTitle = QLabel("Cell")
        self.cellTitle.setObjectName("cellTitle")
        self.cellInput = QLineEdit()
        # placeholder
        self.cellInput.setText("10")
        self.cellInput.setPlaceholderText("Max = 10")
        self.cellInput.setObjectName("cellInput")
        self.titleTop = QLabel("Plant Configuration")
        self.titleTop.setObjectName("titleTop")
        self.titleBottom = QLabel("BESS Configuration")
        self.titleBottom.setObjectName("titleBottom")
        self.window_home_layout = QVBoxLayout()

        # ---- centering icons ---#
        self.sidebarLogo.setAlignment(Qt.AlignCenter)

        self.plantIcon.setAlignment(Qt.AlignCenter)
        self.turbineIcon.setAlignment(Qt.AlignCenter)
        self.weatherIcon.setAlignment(Qt.AlignCenter)

        self.blockIcon.setAlignment(Qt.AlignCenter)
        self.rackIcon.setAlignment(Qt.AlignCenter)
        self.moduleIcon.setAlignment(Qt.AlignCenter)
        self.cellIcon.setAlignment(Qt.AlignCenter)

        # ---- centering text ----#
        self.plantTitle.setAlignment(Qt.AlignCenter)
        self.turbineTitle.setAlignment(Qt.AlignCenter)
        self.weatherTitle.setAlignment(Qt.AlignCenter)

    # ---------------------------------- set-up layouts  ----------------------------------#
    def setup_layout(self):
        """Sets up the layouts for all widgets."""
        self.setLayout(self.window_home_layout)
        #self.window_home_layout.addWidget(self.menu_bar_home)
        self.main_content_layout.addWidget(self.left, stretch=1)
        self.main_content_layout.addWidget(self.right, stretch=4)
        self.main_content_layout.setAlignment(Qt.AlignCenter)
        self.window_home_layout.addLayout(self.main_content_layout)
        self.left.setLayout(self.left_layout)
        self.left_layout.addWidget(self.sidebar, stretch=1)
        self.sidebar.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.sidebar.setLayout(self.sidebar_layout)
        self.sidebar_layout.setAlignment(Qt.AlignCenter)
        self.sidebar_layout.addWidget(self.sidebarInnards, stretch=1)
        self.sidebarInnards.setLayout(self.sidebarInnards_layout)
        self.sidebarInnards_layout.setAlignment(Qt.AlignCenter)
        self.sidebarInnards_layout.addWidget(self.sidebarCollapse, stretch=1)
        self.sidebarInnards_layout.addWidget(self.sidebarLogo, stretch=3)
        self.sidebarInnards_layout.addWidget(self.sidebarTitle, stretch=1)
        self.sidebarInnards_layout.addWidget(self.sidebarDescription, stretch=3)
        self.sidebarInnards_layout.addWidget(self.sidebarButton1, stretch=1, alignment=Qt.AlignCenter)
        self.sidebarInnards_layout.addWidget(self.sidebarButton2, stretch=1, alignment=Qt.AlignCenter)
        self.sidebarInnards_layout.addWidget(self.sidebarButton3, stretch=1, alignment=Qt.AlignCenter)
        self.sidebarInnards_layout.addWidget(self.sidebarSpace, stretch=3)
        self.sidebarInnards_layout.setSpacing(0)
        self.sidebarInnards.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.sidebarSpace.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.right.setLayout(self.right_layout)
        self.right_layout.addWidget(self.topRight, stretch=6)
        self.right_layout.addWidget(self.bottomRight, stretch=6)
        self.right_layout.addWidget(self.interact, stretch=1)
        self.topRight.setLayout(self.topRight_layout)
        self.topRight_layout.addWidget(self.tab, stretch=1)
        self.topRight_layout.addWidget(self.box, stretch=8)
        self.topRight.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        self.bottomRight.setLayout(self.bottomRight_layout)
        self.bottomRight_layout.addWidget(self.tabBottom, stretch=1)
        self.bottomRight_layout.addWidget(self.boxBottom, stretch=8)
        self.bottomRight.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        self.interact.setLayout(self.interact_layout)
        self.interact_layout.addWidget(self.interactInput, stretch=10)
        self.interact_layout.addWidget(self.interactButton, stretch=1)
        self.tab.setLayout(self.tab_layout)
        self.tab_layout.addWidget(self.titleTop, stretch=1)
        self.box.setLayout(self.box_layout)
        self.box_layout.addWidget(self.box, stretch=8)
        self.box_layout.addWidget(self.plant, stretch=1)
        self.box_layout.addWidget(self.turbine, stretch=1)
        self.box_layout.addWidget(self.weather, stretch=1)
        self.tabBottom.setLayout(self.tabBottom_layout)
        self.tabBottom_layout.addWidget(self.titleBottom, stretch=1)
        self.boxBottom.setLayout(self.boxBottom_layout)
        self.boxBottom_layout.addWidget(self.boxBottom, stretch=8)
        self.boxBottom_layout.addWidget(self.block, stretch=1)
        self.boxBottom_layout.addWidget(self.rack, stretch=1)
        self.boxBottom_layout.addWidget(self.module, stretch=1)
        self.boxBottom_layout.addWidget(self.cell, stretch=1)
        self.boxBottom_layout.setSpacing(0)
        self.plant.setLayout(self.plant_layout)
        self.plant_layout.addWidget(self.plantTitle, stretch=1)
        self.plant_layout.addWidget(self.plantIcon, stretch=4)
        self.plant_layout.addWidget(self.plantInputs, stretch=2)
        self.plantInputs.setLayout(self.plantInputs_layout)
        self.plantInputs_layout.addWidget(self.plantInputsLeft, stretch=1)
        self.plantInputsLeft.setLayout(self.plantInputsLeft_layout)
        self.plantInputsLeft_layout.addWidget(self.plantInputsLeftNumberTitle, stretch=1)
        self.plantInputsLeft_layout.addWidget(self.plantInputsLeftNumberInput, stretch=1)
        self.plantInputs_layout.addWidget(self.plantInputsRight, stretch=1)
        self.plantInputsRight.setLayout(self.plantInputsRight_layout)
        self.plantInputsRight_layout.addWidget(self.plantInputsLeftLocationTitle, stretch=1)
        self.plantInputsRight_layout.addWidget(self.plantInputsLeftLocationInput, stretch=1)
        self.turbine.setLayout(self.turbine_layout)
        self.turbine_layout.addWidget(self.turbineTitle, stretch=1)
        self.turbine_layout.addWidget(self.turbineIcon, stretch=4)
        self.turbine_layout.addWidget(self.turbineInputs, stretch=2)
        self.turbineInputs.setLayout(self.turbineInputs_layout)
        self.turbineInputs_layout.addWidget(self.turbineInputsLeft, stretch=1)
        self.turbineInputsLeft.setLayout(self.turbineInputsLeft_layout)
        self.turbineInputsLeft_layout.addWidget(self.turbineInputsLeftNumberTitle, stretch=1)
        self.turbineInputsLeft_layout.addWidget(self.turbineInputsLeftNumberInput, stretch=1)
        self.turbineInputs_layout.addWidget(self.turbineInputsRight, stretch=1)
        self.turbineInputsRight.setLayout(self.turbineInputsRight_layout)
        self.turbineInputsRight_layout.addWidget(self.turbineInputsLeftLocationTitle, stretch=1)
        self.turbineInputsRight_layout.addWidget(self.turbineInputsLeftLocationInput, stretch=1)
        self.weather.setLayout(self.weather_layout)
        self.weather_layout.addWidget(self.weatherTitle, stretch=1)
        self.weather_layout.addWidget(self.weatherIcon, stretch=4)
        self.weather_layout.addWidget(self.weatherInputs, stretch=2)
        self.weatherInputs.setLayout(self.weatherInputs_layout)
        self.weatherInputs_layout.addWidget(self.weatherInputsLeft, stretch=1)
        self.weatherInputsLeft.setLayout(self.weatherInputsLeft_layout)
        self.weatherInputsLeft_layout.addWidget(self.weatherInputsLeftNumberTitle, stretch=1)
        self.weatherInputsLeft_layout.addWidget(self.weatherInputsLeftNumberInput, stretch=1)
        self.weatherInputs_layout.addWidget(self.weatherInputsRight, stretch=1)
        self.weatherInputsRight.setLayout(self.weatherInputsRight_layout)
        self.weatherInputsRight_layout.addWidget(self.weatherInputsLeftLocationTitle, stretch=1)
        self.weatherInputsRight_layout.addWidget(self.weatherInputsLeftLocationInput, stretch=1)
        self.block.setLayout(self.block_layout)
        self.block_layout.addWidget(self.blockIcon, stretch=4)
        self.block_layout.addWidget(self.blockTitle, stretch=1)
        self.block_layout.addWidget(self.blockInput, stretch=1)
        self.rack.setLayout(self.rack_layout)
        self.rack_layout.addWidget(self.rackIcon, stretch=4)
        self.rack_layout.addWidget(self.rackTitle, stretch=1)
        self.rack_layout.addWidget(self.rackInput, stretch=1)
        self.module.setLayout(self.module_layout)
        self.module_layout.addWidget(self.moduleIcon, stretch=4)
        self.module_layout.addWidget(self.moduleTitle, stretch=1)
        self.module_layout.addWidget(self.moduleInput, stretch=1)
        self.cell.setLayout(self.cell_layout)
        self.cell_layout.addWidget(self.cellIcon, stretch=4)
        self.cell_layout.addWidget(self.cellTitle, stretch=1)
        self.cell_layout.addWidget(self.cellInput, stretch=1)
        self.cell_layout.setAlignment(Qt.AlignCenter)

        self.topRight_layout.setSpacing(0)
        self.bottomRight_layout.setSpacing(0)

        central_widget = QWidget(self)

    def setup_actions(self):
        """Set up actions for buttons."""
        # Connect the button click event to the function that prints input values
        self.interactButton.clicked.connect(self.on_submit_button_clicked)
        self.sidebarButton1.clicked.connect(self.on_clear_button_clicked)
        self.sidebarButton2.clicked.connect(self.on_random_button_clicked)
        self.sidebarButton3.clicked.connect(self.show_settings)

    def on_submit_button_clicked(self):
        if self.validate_inputs():
            print("Inputs are valid, proceed with processing.")
            # Proceed with whatever you want to do after validation
            self.parent().setCurrentIndex(1)  # Switch to the second window (config)
        else:
            print("Some inputs are invalid.")

    def open_custom_dialog(self):
        """Opens the custom file dialog."""
        dialog = self.popup
        dialog.setFixedSize(400, 200)
        if dialog.exec():  # If user clicks OK
            file_name = dialog.input_field.text()
            print(f"User chose to save file: {file_name}")

    def on_clear_button_clicked(self):
        self.plantInputsLeftNumberInput.setText("")
        self.turbineInputsLeftNumberInput.setText("")
        self.turbineInputsLeftLocationInput.setText("")
        self.weatherInputsLeftLocationInput.setText("")
        self.weatherInputsLeftNumberInput.setText("")
        self.blockInput.setText("")
        self.rackInput.setText("")
        self.moduleInput.setText("")
        self.cellInput.setText("")
        self.interactInput.setText("")

    def on_random_button_clicked(self):
        self.plantInputsLeftNumberInput.setText(str(random.randint(1, 5)))
        self.turbineInputsLeftNumberInput.setText(str(random.randint(50, 180)))
        self.turbineInputsLeftLocationInput.setText(str(round(random.uniform(0.01, 0.59), 2)))
        speeds = random.randint(1, 70)
        self.weatherInputsLeftLocationInput.setText(str(speeds + 5))
        self.weatherInputsLeftNumberInput.setText(str(speeds))
        self.blockInput.setText(str(random.randint(1, 5)))
        self.rackInput.setText(str(random.randint(1, 5)))
        self.moduleInput.setText(str(random.randint(1, 5)**2))
        self.cellInput.setText(str(random.randint(1, 5)))
        chooseCheck = random.randint(0, 1)
        if chooseCheck == 0:
            self.plantInputsLeftLocationInput.setChecked(True)
            print("We got false!")
        if chooseCheck == 1:
            print("we got true!")
            self.plantInputsLeftLocationTitle.setChecked(True)
        self.interactInput.setText("")

    def show_error(self, message):
        dialog = UniversalDialog()
        error_page = ErrorPage(message, dialog)
        dialog.set_content(error_page)
        dialog.exec()

    def show_settings(self):
        dialog = UniversalDialog()
        defaults = {
            "path": "",
            "prob": 0.25,
            "delay": 1000
        }
        settings_page = SettingsPage(dialog, defaults, self.save_settings)
        dialog.set_content(settings_page)
        dialog.exec()

    def save_settings(self, settings):
        print("Settings saved:", settings)
        self.prob = settings['prob']
        self.delay = settings['delay']
        self.path = settings['path']
        print(self.prob)
        #print(f"Current FAULT_PROBABILITY before running simulation: {Methods.FAULT_PROBABILITY}")
        #print(f"Current FAULT_PROBABILITY: {Methods.get_fault_probability()}")

        Methods.FAULT_PROBABILITY = 0.75
        print(Methods.FAULT_PROBABILITY)
        #Methods.FAULT_PROBABILITY = self.prob
        #print(f"Updated FAULT_PROBABILITY to: {Methods.FAULT_PROBABILITY}")

    def print_input_values(self):
        """Print the values of the input fields to the console."""
        plant_number = self.plantInputsLeftNumberInput.text()
        location = self.plantInputsLeftLocationInput.text()
        turbine_blade_length = self.turbineInputsLeftNumberInput.text()
        energy_coefficient = self.turbineInputsLeftLocationTitle.text()
        weather_min_wind = self.weatherInputsLeftNumberInput.text()
        weather_max_wind = self.weatherInputsLeftNumberTitle.text()
        block_input = self.blockInput.text()
        rack_input = self.rackInput.text()
        module_input = self.moduleInput.text()
        cell_input = self.cellInput.text()

        print("Plant Number of Turbines:", plant_number)
        print("Location:", location)
        print("Turbine Blade Length:", turbine_blade_length)
        print("Weather Min Wind Speed:", weather_min_wind)
        print("Block Input:", block_input)
        print("Rack Input:", rack_input)
        print("Module Input:", module_input)
        print("Cell Input:", cell_input)

    import math

    import math

    def validate_inputs(self):
        validated_data = {}  # Dictionary to store validated inputs

        try:
            # Retrieve input values
            plant_size = float(self.plantInputsLeftNumberInput.text())
            blade_length = float(self.turbineInputsLeftNumberInput.text())
            energy_coefficient = float(self.turbineInputsLeftLocationInput.text())
            min_speed = float(self.weatherInputsLeftNumberInput.text())
            max_wind_speed = float(self.weatherInputsLeftLocationInput.text())
            filename = self.interactInput.text()

            # Retrieve and validate integer inputs
            try:
                block = int(self.blockInput.text().strip())
                rack = int(self.rackInput.text().strip())
                module = int(self.moduleInput.text().strip())
                cell = int(self.cellInput.text().strip())
                num_turbines = int(self.plantInputsLeftNumberInput.text().strip())  # New field
                if block <= 0 or rack <= 0 or module <= 0 or cell <= 0 or num_turbines <= 0:
                    raise ValueError("Block, Rack, Module, Cell, and Number of Turbines must be positive integers.")
            except ValueError:
                raise ValueError("Block, Rack, Module, Cell, and Number of Turbines must be valid integers.")

            # Check which radio button is selected
            if self.plantInputsLeftLocationTitle.isChecked():
                location = "Onshore"
            elif self.plantInputsLeftLocationInput.isChecked():
                location = "Offshore"
            else:
                raise ValueError("On or Offshore MUST be selected.")

            # Validation checks
            if plant_size > 1000:
                raise ValueError("Plant size cannot be over 1000.")

            if blade_length > 120:
                raise ValueError("Blade length cannot be over 120.")

            if energy_coefficient > 0.59:
                raise ValueError("Energy coefficient cannot be over 0.59.")

            if min_speed < 0:
                raise ValueError("Min speed cannot be below 0.")

            if max_wind_speed > 70:
                raise ValueError("Max wind speed cannot be over 70.")

            if min_speed > max_wind_speed:
                raise ValueError("Min speed cannot be greater than max speed.")

            if max_wind_speed < min_speed:
                raise ValueError("Max speed cannot be less than min speed.")

            # Check if module is a perfect square
            if math.isqrt(module) ** 2 != module:
                raise ValueError("Module must be a perfect square.")

            filepath = str(project_path) + filename + ".json"
            print(filepath)
            # C:\Users\Erica\Pictures\Senior Design\Hermes\Hermes_Reloaded\Untitled-01.json
            # if os.path.exists(filepath):
            # raise FileExistsError(f"Error: The file '{filename}' already exists. Choose a different name.")

            numPorts = int(math.sqrt(module))
            print("Number of ports Home: ", numPorts)

            # Store validated values in the dictionary
            validated_data['plant_size'] = plant_size
            validated_data['location'] = location
            validated_data['blade_length'] = blade_length
            validated_data['energy_coefficient'] = energy_coefficient
            validated_data['min_speed'] = min_speed
            validated_data['max_wind_speed'] = max_wind_speed
            validated_data['block'] = block
            validated_data['rack'] = rack
            validated_data['module'] = module
            validated_data['cell'] = cell
            validated_data['num_turbines'] = num_turbines  # Added field
            validated_data['filename'] = filename  # Added field

            print(validated_data)  # Debugging print statement
            self.core.receive_data(validated_data)  # Send to core

            return validated_data  # Return the dictionary if validation passes

        except ValueError as e:
            if "could not convert string to float: ''" in str(e):
                show_error_popup("Please review the form. Some fields are empty.")
            else:
                self.show_error(str(e))
            return None  # Return None if validation fails

        except FileExistsError as e:
            if "could not convert string to float: ''" in str(e):
                show_error_popup("Please review the form. Some fields are empty.")
            else:
                show_error_popup(str(e))
            return None  # Return None if validation fails

    def show_error_message(self):
        """Function to display error messages."""
        msg = QMessageBox()
        msg.setObjectName("error")
        msg.setIcon(QMessageBox.Critical)
        msg.setText(self)
        msg.setWindowTitle("Input Error")
        msg.setFixedSize(600, 600)
        msg.exec()

    def on_button_click(self):
        if self.validate_inputs():
            print("Inputs are valid. Proceeding...")
        else:
            print("Input validation failed.")

    def apply_stylesheet(self):
        """Applies the application stylesheet."""
        app = QApplication.instance()
        stylesheet = load_stylesheet(self, "resources/style_hermes.qss")
        app.setStyleSheet(stylesheet)


class HermesConfig(QWidget):
    reset_requested = Signal()  # Add this at class level
    def __init__(self, core, home):
        super().__init__()
        self.home= home
        self.core = core
        self.init_ui()
        self.load_resources()
        self.setup_menu()
        self.create_layouts()
        self.setup_widgets()
        self.setup_layout()
        self.apply_stylesheet()
        self.setup_actions()
        self.core.config_updated.connect(self.setup_ports)  # Listen for changes
        # self.echo(self.core)

    # ---------------------------------- load stylesheet ----------------------------------#
    def load_stylesheet(file_path):
        file = QFile(file_path)
        if not file.open(QFile.ReadOnly):
            raise Exception(f"Cannot open file: {file_path}")
        stream = QTextStream(file)
        stylesheet = stream.readAll()
        file.close()
        return stylesheet

    # ---------------------------------- initialize window ----------------------------------#
    def init_ui(self):
        """Initializes the user interface."""
        self.setObjectName("window")
        self.setWindowTitle(" Hermes")
        self.resize(1200, 600)

    # ---------------------------------- load resources  ----------------------------------#
    def load_resources(self):
        """Loads resources like icons and fonts."""
        app = QApplication.instance()
        app.setWindowIcon(QIcon(str(project_path) + "resources/images/Logo.ico"))
        QFontDatabase.addApplicationFont(
            str(project_path) + "/resources/images/Font_005.ttf")
        QFontDatabase.addApplicationFont(
            str(project_path) + "resources/images/LEMONMILK-Medium.otf")

    def apply_stylesheet(self):
        """Applies the stylesheet to the application."""
        app = QApplication.instance()
        stylesheet = self.load_stylesheet("resources\\style_hermes.qss")
        app.setStyleSheet(stylesheet)

    # ---------------------------------- load navbar  ----------------------------------#
    def setup_menu(self):
        """Sets up the menu bar."""
        self.menu_bar = QMenuBar()
        self.menu_bar.setObjectName("menu2")
        file_menu = QMenu("File", self.menu_bar)
        self.menu_bar.addMenu(file_menu)

        new_action = QAction("New", self)
        open_action = QAction("Open", self)
        save_action = QAction("Save", self)
        exit_action = QAction("Exit", self)
        exit_action.triggered.connect(QApplication.instance().quit)

        file_menu.addAction(new_action)
        file_menu.addAction(open_action)
        file_menu.addAction(save_action)
        file_menu.addSeparator()
        file_menu.addAction(exit_action)

        self.menu_bar.addMenu(QMenu("Edit", self.menu_bar))
        self.menu_bar.addMenu(QMenu("View", self.menu_bar))
        self.menu_bar.addMenu(QMenu("Help", self.menu_bar))

    # ---------------------------------- create layouts  ----------------------------------#
    def create_layouts(self):
        """Creates all the layouts used in the application."""
        self.window_home_layout = QVBoxLayout()
        self.main_content_layout = QHBoxLayout()
        self.left_layout = QHBoxLayout()
        self.sidebar_layout = QVBoxLayout()
        self.sidebarInnards_layout = QVBoxLayout()
        self.right_layout = QHBoxLayout()
        self.topRight_layout = QVBoxLayout()
        self.bottomRight_layout = QVBoxLayout()
        self.tab_layout = QVBoxLayout()
        self.box_layout = QVBoxLayout()
        self.tabBottom_layout = QVBoxLayout()
        self.boxBottom_layout = QVBoxLayout()
        self.leftButtons_layout = QHBoxLayout()
        self.rightButtons_layout = QHBoxLayout()
        self.statusBar_layout = QHBoxLayout()
        self.box_layout = QVBoxLayout()
        self.statisticsWindow_layout = QVBoxLayout()
        self.barGraphWindow_layout = QVBoxLayout()
        self.container_layout = QVBoxLayout()

    def update_display(self):
        print("Updated modules_per_rack:", self.core.config.get("modules_per_rack", "Not Set"))

    def setup_ports(self):
        # Clean up existing scroll area if it's already there
        if hasattr(self, "scroll_area") and self.scroll_area:
            self.box_layout.removeWidget(self.scroll_area)
            self.scroll_area.deleteLater()
            self.scroll_area = None

        self.numPort = self.core.config.get("modules_per_rack", "Not Set")
        content_widget = QWidget()
        content_widget.setObjectName("scrollDetails")
        chassis_layout = QVBoxLayout(content_widget)

        chassis = QWidget()
        chassis.setObjectName("chassis")
        chassis.setLayout(chassis_layout)
        chassis.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        nodePort = QWidget()
        nodePort.setObjectName("nodePort")
        nodePort_layout = QHBoxLayout()
        nodePort.setLayout(nodePort_layout)

        self.scroll_area = QScrollArea()
        self.scroll_area.setWidgetResizable(True)

        self.comNumber_fields = []
        chassis_layout.setAlignment(Qt.AlignTop)
        nodePort_layouts = []

        self.numberPorts = len(self.core.com_ports)
        print("Number of ports detected: ", self.numberPorts)
        print(self.core.config)
        print("Look here! Number of modules is: ", self.numPort)

        for i in range(int(math.sqrt(self.numPort))):

            nodePort_layout = QHBoxLayout()
            nodePort_layouts.append(nodePort_layout)

            nodeName = QLabel(f"Node {i + 1}")
            comLabel = QLabel("COM:")
            comNumber = QLineEdit()

            if i == 0:
                comNumber.setText("7")
            elif i == 1:
                comNumber.setText("15")
            elif i == 2:
                comNumber.setText("17")
            elif i == 3:
                comNumber.setText("19")

            int_validator = QIntValidator(1, 99)
            comNumber.setValidator(int_validator)

            nodePort_layout.addWidget(nodeName, stretch=6)
            nodePort_layout.addWidget(comLabel, stretch=1)
            nodePort_layout.addWidget(comNumber, stretch=1)

            self.comNumber_fields.append(comNumber)

            chassis_layout.addLayout(nodePort_layout)

        content_widget.setLayout(chassis_layout)
        self.scroll_area.setWidget(content_widget)
        self.box_layout.addWidget(self.scroll_area, stretch=11)

    # ---------------------------------- set-up widgets  ----------------------------------#

    def setup_widgets(self):
        """Sets up all the widgets used in the application."""
        self.left = QWidget()
        self.left.setObjectName("left")
        self.sidebar = QWidget()
        self.sidebar.setObjectName("sidebar")
        self.sidebarInnards = QWidget()
        self.sidebarTitle = QLabel("Hermes")
        self.sidebarTitle.setObjectName("sidebarTitle")
        self.sidebarTitle.setAlignment(Qt.AlignCenter)
        self.sidebarInnards.setObjectName("sidebarInnards")
        self.sidebarLogo = QLabel("H")
        self.sidebarLogo.setObjectName("sidebarLogo")
        self.sidebarDescription = QLabel(
            "Generate fully customizable plant and weather configurations to simulate the behavior of a BESS-BMS.")
        self.sidebarDescription.setObjectName("sidebarDescription")
        self.sidebarButton1 = QPushButton("Back")
        self.sidebarButton1.setObjectName("sidebarButton1")
        self.sidebarButton2 = QPushButton("New")
        self.sidebarButton2.setObjectName("sidebarButton2")
        self.sidebarButton3 = QPushButton("Button3")
        self.sidebarButton3.setObjectName("sidebarButton3")

        self.sidebarButton1.setVisible(True)
        self.sidebarButton2.setVisible(True)
        self.sidebarButton3.setVisible(False)

        self.sidebarCollapse = QPushButton("❰")
        self.sidebarCollapse.setObjectName("collapse")
        self.sidebarSpace = QLabel("")
        self.sidebarSpace.setObjectName("sidebarSpace")
        self.right = QWidget()
        self.right.setObjectName("right")
        self.topRight = QWidget()
        self.topRight.setObjectName("topRight")
        self.bottomRight = QWidget()
        self.bottomRight.setObjectName("bottomRight")
        self.tab = QWidget()
        self.tab.setObjectName("tab")
        self.box = QWidget()
        self.box.setObjectName("box")
        self.tabBottom = QWidget()
        self.tabBottom.setObjectName("tabBottom")
        self.boxBottom = QWidget()
        self.boxBottom.setObjectName("boxBottom")

        self.titleTop = QLabel("Port Configuration")
        self.titleTop.setObjectName("titleTop")
        self.titleBottom = QLabel("Progress")
        self.titleBottom.setObjectName("titleBottom")
        self.window_home_layout = QVBoxLayout()

        # ---- centering icons ---#
        self.sidebarLogo.setAlignment(Qt.AlignCenter)

        # ---- config buttons ---- #
        self.leftButtons = QWidget()
        self.leftButtons.setObjectName("leftButtons")

        self.horSpace = QWidget()
        self.horSpace.setObjectName("horSpace")

        self.skipConfig = QPushButton("Skip")
        self.skipConfig.setObjectName("skipConfig")
        self.skipConfig.clicked.connect(self.skipSimulation)

        self.submitConfig = QPushButton("Send")
        self.submitConfig.setObjectName("submitConfig")

        # ---- progress buttons ---- #

        self.rightButtons = QWidget()
        self.rightButtons.setObjectName("rightButtons")

        self.horSpaceProgress = QWidget()
        self.horSpaceProgress.setObjectName("horSpaceProgress")

        self.resultsProgress = QPushButton("Results")
        self.resultsProgress.setObjectName("resultsProgress")
        self.resultsProgress.clicked.connect(self.skipSimulation)

        # ---- ESP32 echo window ----#
        self.response = QPlainTextEdit()
        self.response.setObjectName("response")
        self.response.setReadOnly(True)
        self.response.setVisible(False)

        # ---- status bar ----#
        self.statusTitle = QLabel("Status:")
        self.statusTitle.setObjectName("statusTitle")
        self.statusTitle.setAlignment(Qt.AlignCenter)

        self.statusSpace = QLabel()
        self.statusSpace.setObjectName("statusSpace")

        # ready, working, complete
        self.statusUpdate = QLabel("Ready")
        self.statusUpdate.setObjectName("statusUpdate")
        self.statusUpdate.setAlignment(Qt.AlignCenter)

        self.statusBar = QWidget()
        self.statusBar.setObjectName("statusBar")

        self.barGraphWindow = QWidget()
        self.barGraphWindow.setObjectName("barGraphWindow")

        self.statisticsWindow = QWidget()
        self.statisticsWindow.setObjectName("statisticsWindow")

        self.tab = QWidget()
        self.tab.setObjectName("tab")
        self.box = QWidget()
        self.box.setObjectName("box")

        self.container = QWidget()
        self.container.setObjectName("container")
        self.titleTop = QLabel("Port Configuration")
        self.titleTop.setObjectName("titleTop")
        # bar diagram widget
        self.barDiagram = QLabel("barDiagram")
        self.barDiagram.setObjectName("barDiagram")
        # bar diagram legend widget
        self.barDiagramLegend = QLabel("barDiagramLegend")
        self.barDiagramLegend.setObjectName("barDiagramLegend")
        self.titleBottom = QLabel("Trends")
        self.titleBottom.setObjectName("titleBottom")

        self.scroll_area_statistics = QScrollArea()
        self.scroll_area_statistics.setWidgetResizable(True)  # Allows resizing content dynamically

    # ---------------------------------- set-up layouts  ----------------------------------#
    def setup_layout(self):
        """Sets up the layouts for all widgets."""
        self.setLayout(self.window_home_layout)
        #self.window_home_layout.addWidget(self.menu_bar)
        self.main_content_layout.addWidget(self.left, stretch=1)
        self.main_content_layout.addWidget(self.right, stretch=4)
        self.main_content_layout.setAlignment(Qt.AlignCenter)
        self.window_home_layout.addLayout(self.main_content_layout)
        self.left.setLayout(self.left_layout)
        self.left_layout.addWidget(self.sidebar, stretch=1)
        self.sidebar.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.sidebar.setLayout(self.sidebar_layout)
        self.sidebar_layout.setAlignment(Qt.AlignCenter)
        self.sidebar_layout.addWidget(self.sidebarInnards, stretch=1)
        self.sidebarInnards.setLayout(self.sidebarInnards_layout)
        self.sidebarInnards_layout.setAlignment(Qt.AlignCenter)
        self.sidebarInnards_layout.addWidget(self.sidebarCollapse, stretch=1)
        self.sidebarInnards_layout.addWidget(self.sidebarLogo, stretch=3)
        self.sidebarInnards_layout.addWidget(self.sidebarTitle, stretch=1)
        self.sidebarInnards_layout.addWidget(self.sidebarDescription, stretch=3)
        self.sidebarInnards_layout.addWidget(self.sidebarButton1, stretch=1, alignment=Qt.AlignCenter)
        self.sidebarInnards_layout.addWidget(self.sidebarButton2, stretch=1, alignment=Qt.AlignCenter)
        self.sidebarInnards_layout.addWidget(self.sidebarButton3, stretch=1, alignment=Qt.AlignCenter)
        self.sidebarInnards_layout.addWidget(self.sidebarSpace, stretch=4)
        self.sidebarInnards_layout.setSpacing(0)
        self.sidebarInnards.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.sidebarSpace.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.right.setLayout(self.right_layout)
        self.right_layout.addWidget(self.topRight, stretch=6)
        self.right_layout.addWidget(self.bottomRight, stretch=6)
        self.topRight.setLayout(self.topRight_layout)
        self.topRight_layout.addWidget(self.tab, stretch=1)
        self.topRight_layout.addWidget(self.box, stretch=16)

        self.leftButtons_layout.addWidget(self.horSpace, stretch=6)
        self.leftButtons_layout.addWidget(self.skipConfig, stretch=2)
        self.leftButtons_layout.addWidget(self.submitConfig, stretch=2)

        self.topRight_layout.addWidget(self.leftButtons, stretch=1)

        self.topRight.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        self.bottomRight.setLayout(self.bottomRight_layout)
        self.bottomRight_layout.addWidget(self.tabBottom, stretch=1)
        self.bottomRight_layout.addWidget(self.boxBottom, stretch=16)

        self.rightButtons_layout.addWidget(self.horSpaceProgress, stretch=8)
        self.rightButtons_layout.addWidget(self.resultsProgress, stretch=2)

        # self.bottomRight_layout.addWidget(self.rightButtons, stretch=1)

        self.bottomRight.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        self.tab.setLayout(self.tab_layout)
        self.tab_layout.addWidget(self.titleTop, stretch=1)
        self.box.setLayout(self.box_layout)

        self.tabBottom.setLayout(self.tabBottom_layout)
        # self.tabBottom_layout.addWidget(self.scroll_area)
        self.tabBottom_layout.addWidget(self.titleBottom, stretch=1)

        self.statusBar.setLayout(self.statusBar_layout)
        self.statusBar_layout.addWidget(self.statusTitle, stretch=1)
        self.statusBar_layout.addWidget(self.statusSpace, stretch=3)
        self.statusBar_layout.addWidget(self.statusUpdate, stretch=1)

        self.boxBottom.setLayout(self.boxBottom_layout)
        self.box_layout.addWidget(self.response, stretch=5)
        self.box_layout.addWidget(self.statusBar, stretch=1)

        self.boxBottom_layout.setSpacing(0)

        self.topRight_layout.setSpacing(0)
        self.bottomRight_layout.setSpacing(0)

        self.leftButtons.setLayout(self.leftButtons_layout)
        self.rightButtons.setLayout(self.rightButtons_layout)

        self.barGraphWindow.setLayout(self.barGraphWindow_layout)
        self.boxBottom_layout.addWidget(self.barGraphWindow, stretch=1)

        self.boxBottom.setLayout(self.box_layout)
        # self.boxBottom_layout.addLayout(self.statisticsWindow_layout)

        self.scroll_area_statistics.setWidget(self.statisticsWindow)
        self.boxBottom_layout.addLayout(self.statisticsWindow_layout)

    def get_available_com_ports(self):
        """Retrieve a list of available COM ports."""
        ports = serial.tools.list_ports.comports()
        available_ports = [port.device.replace("COM", "") for port in ports]  # Extract COM numbers
        return set(available_ports)

    def validate_com_ports(self):
        """Validate that input COM ports are integers and available."""
        com_numbers_text = []
        available_ports = self.get_available_com_ports()

        for i, com in enumerate(self.comNumber_fields):
            text = com.text().strip()

            if text == "":
                self.show_error_popup(f"COM Number {i + 1} cannot be empty!")
                return

            try:
                value = int(text)  # Ensure it's an integer

                if str(value) not in available_ports:
                    self.show_error_popup(f"COM{value} is not available on this system.")
                    return

                com_numbers_text.append(value)
            except ValueError:
                self.show_error_popup(f"Invalid input in COM Number {i + 1}. Must be an integer!")
                return

        # Print the valid and available COM ports
        for i, value in enumerate(com_numbers_text):
            print(f"COM Number {i + 1}: COM{value} (Valid & Available)")

        self.core.receive_com_ports(com_numbers_text)

        print("COM Ports sent to core:", com_numbers_text)  # Debugging print
        print("From the interface:", self.core.config)  # Debug print to ensure config is correct
        self.core.run_simulation()

        self.core.data_ready.connect(self.update_chart)
        #self.core.run_simulation()  # Call the Core method
        file_path = self.core.config["filename"] + ".json"
        self.core.process_file(file_path)

        self.skipConfig.setStyleSheet("color: #decdbc; background-color: #f0f0f0; border: 1px solid #decdbc;")
        self.statusBar.setStyleSheet("color: #decdbc; background-color: #f0f0f0; border: 1px solid #decdbc;")
        self.submitConfig.setStyleSheet("color: #decdbc; background-color: #f0f0f0; border: 1px solid #decdbc;")
        # self.topRight.setStyleSheet("color: gray; background-color: lightgray; border: 1px solid gray;")

        self.skipConfig.setEnabled(False)
        self.statusBar.setEnabled(False)
        self.submitConfig.setEnabled(False)
        self.topRight.setEnabled(False)

        # self.echo(self.core.response_received)

        return com_numbers_text

    def show_error_popup(self, message):
        """Display an error popup."""
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Critical)
        msg.setWindowTitle("Input Error")
        msg.setText(message)
        msg.exec()

    def setup_actions(self):
        """Set up actions for buttons."""
        # Connect the button click event to the function that prints input values
        self.sidebarButton1.clicked.connect(self.on_back_button_clicked)
        self.sidebarButton2.clicked.connect(self.on_new_button_clicked)
        self.submitConfig.clicked.connect(self.validate_com_ports)

    def on_back_button_clicked(self):
        print("Going back!")
        # Proceed with whatever you want to do after validation
        if hasattr(self, "scroll_area") and self.scroll_area:
            self.box_layout.removeWidget(self.scroll_area)
            self.scroll_area.deleteLater()
            self.scroll_area = None
        if hasattr(self, "barChart") and self.chart_view:
            self.barGraphWindow_layout.removeWidget(self.chart_view)
        self.parent().setCurrentIndex(0)  # Switch to the second window (config)

    def on_new_button_clicked(self):
        print("Going back!")
        self.reset_requested.emit()
        # Proceed with whatever you want to do after validation
        if hasattr(self, "scroll_area") and self.scroll_area:
            self.box_layout.removeWidget(self.scroll_area)
            self.scroll_area.deleteLater()
            self.scroll_area = None
        if hasattr(self, "chart_view") and self.chart_view:
            self.barGraphWindow_layout.removeWidget(self.chart_view)
            self.chart_view.deleteLater()
            self.chart_view = None

            #self.statisticsWindow_layout.addLayout(stat_layout)
            #self.scroll_area_statistics.setWidget(self.statisticsWindow)
        self.parent().setCurrentIndex(0)  # Switch to the second window (config)



    def on_submit_button_clicked(self):
        # Ensure we have a valid parent with QStackedWidget

        self.echo(self.core.response_received)

        self.core.data_ready.connect(self.update_chart)
        self.core.runSimulationWithoutESP()  # Call the Core method
        file_path = self.core.config["filename"] + ".json"
        self.core.process_file(file_path)
        self.statusUpdate = QLabel("Sent")
        self.skipConfig.setStyleSheet("color: #decdbc; background-color: #f0f0f0; border: 1px solid #decdbc;")
        self.statusBar.setStyleSheet("color: #decdbc; background-color: #f0f0f0; border: 1px solid #decdbc;")
        self.submitConfig.setStyleSheet("color: #decdbc; background-color: #f0f0f0; border: 1px solid #decdbc;")
        # self.topRight.setStyleSheet("color: gray; background-color: lightgray; border: 1px solid gray;")

        self.skipConfig.setEnabled(False)
        self.statusBar.setEnabled(False)
        self.submitConfig.setEnabled(False)
        self.topRight.setEnabled(False)

    def print_input_values(self):
        """Print the values of the input fields to the console."""
        plant_number = self.plantInputsLeftNumberInput.text()
        location = self.plantInputsLeftLocationInput.text()
        turbine_blade_length = self.turbineInputsLeftNumberInput.text()
        energy_coefficient = self.turbineInputsLeftLocationTitle.text()
        weather_min_wind = self.weatherInputsLeftNumberInput.text()
        weather_max_wind = self.weatherInputsLeftNumberTitle.text()
        block_input = self.blockInput.text()
        rack_input = self.rackInput.text()
        module_input = self.moduleInput.text()
        cell_input = self.cellInput.text()

        print("Plant Number of Turbines:", plant_number)
        print("Location:", location)
        print("Turbine Blade Length:", turbine_blade_length)
        print("Weather Min Wind Speed:", weather_min_wind)
        print("Block Input:", block_input)
        print("Rack Input:", rack_input)
        print("Module Input:", module_input)
        print("Cell Input:", cell_input)

    def show_error_message(self):
        """Function to display error messages."""
        msg = QMessageBox()
        msg.setObjectName("error")
        msg.setIcon(QMessageBox.Critical)
        msg.setText(self)
        msg.setWindowTitle("Input Error")
        msg.setFixedSize(600, 600)
        msg.exec()

    def on_button_click(self):
        if self.validate_inputs():
            print("Inputs are valid. Proceeding...")
        else:
            print("Input validation failed.")

    def apply_stylesheet(self):
        """Applies the application stylesheet."""
        app = QApplication.instance()
        stylesheet = load_stylesheet(self, "resources/style_hermes.qss")
        app.setStyleSheet(stylesheet)

    def echo(self, data):
        """Appends received data to the response monitor"""
        self.statusUpdate.setStyleSheet("""
          background: qlineargradient(spread:pad,
          x1:0, y1:0, x2:1, y2:0,
          stop:0 #ffaa00,
          stop:1 #ffe53f);
          color: white;
          border: 1px solid #ffaa00;
          padding: 3px;
          border-radius: 5px;
                                        """)
        self.statusUpdate.setText("Working")
        print(f"Received: {data}")  # Debug print
        self.response.appendPlainText(data)
        if not data.strip():
            self.statusUpdate.setStyleSheet("""
                                                background: qlineargradient(spread:pad,
              x1:0, y1:0, x2:1, y2:0,
              stop:0 #00ff66,
              stop:1 #8fff15);
              color: white;
              border: 1px solid #00ff66;
              padding: 3px;
            border-radius: 5px;
                                                """)
            self.statusUpdate.setText("Complete")

    def skipSimulation(self):
        self.core.data_ready.connect(self.update_chart)
        self.core.runSimulationWithoutESP()  # Call the Core method
        file_path = self.core.config["filename"] + ".json"
        self.core.process_file(file_path)

        effect = QGraphicsOpacityEffect()
        effect.setOpacity(0.5)  # Adjust transparency (0.0 to 1.0)

        self.skipConfig.setStyleSheet("color: #decdbc; background-color: #f0f0f0; border: 1px solid #decdbc;")
        self.statusBar.setStyleSheet("color: #decdbc; background-color: #f0f0f0; border: 0px solid #decdbc;")
        self.submitConfig.setStyleSheet("color: #decdbc; background-color: #f0f0f0; border: 1px solid #decdbc;")
        # self.topRight.setStyleSheet("color: gray; background-color: lightgray; border: 1px solid gray;")

        self.skipConfig.setEnabled(False)
        self.statusBar.setEnabled(False)
        self.submitConfig.setEnabled(False)
        self.topRight.setEnabled(False)

        # self.parent().setCurrentIndex(2)  # Switch to the second window (config)

    def setup_chart(self):
        """Sets up the QtCharts bar chart for visualization."""
        self.categories = ["Overcharge", "Overdischarged", "Overcurrent", "Overheating", "Thermal Runaway"]

        # Initialize an empty bar series
        self.bar_series = QBarSeries()

        # Create the chart
        self.chart = QChart()
        self.chart.addSeries(self.bar_series)
        self.chart.setTitle("BESS Fault Analysis")
        self.chart.setAnimationOptions(QChart.SeriesAnimations)

        # X-Axis (categories)
        self.axisX = QBarCategoryAxis()
        self.axisX.append(self.categories)
        self.chart.addAxis(self.axisX, Qt.AlignBottom)
        self.bar_series.attachAxis(self.axisX)

        # Y-Axis (values)
        self.axisY = QValueAxis()
        # self.axisY.setRange(0, 100)  # Default range (can be updated dynamically)
        self.axisY.setTickInterval(1)
        self.chart.addAxis(self.axisY, Qt.AlignLeft)
        self.bar_series.attachAxis(self.axisY)

        # Create a chart view
        self.chart_view = QChartView(self.chart)
        self.chart_view.setObjectName("barChart")
        self.chart.setBackgroundBrush(QBrush(QColor(0, 255, 255, 0)))  # Set the background color

        # Get the chart's legend
        legend = self.chart.legend()

        # Set properties for the legend (adjustments to show and style it)
        legend.setVisible(True)  # Show the legend

        legend.setAlignment(Qt.AlignRight | Qt.AlignBottom)  # Adjust alignment as needed
        legend.setWrap(True)  # Enable wrapping

        legend.setFont(QFont("Arial", 10))  # Set font

        # Customize legend background color
        legend.setBrush(QBrush(QColor(255, 255, 255, 100)))  # Optional: Set background color for the legend

        # Set render hint for better appearance
        self.chart_view.setRenderHint(QPainter.Antialiasing)

        # Add the chart view to your layout
        self.barGraphWindow_layout.addWidget(self.chart_view, stretch=3)
        self.barGraphWindow_layout.addWidget(self.barDiagramLegend, stretch=1)

    def update_chart(self):
        # Define a color palette for each fault type
        self.fault_colors = [
            QColor(255, 99, 132),  # Overcharge - Red
            QColor(54, 162, 235),  # Overdischarge - Blue
            QColor(255, 206, 86),  # Overcurrent - Yellow
            QColor(75, 192, 192),  # Overheating - Teal
            QColor(153, 102, 255)  # Thermal Runaway - Purple
        ]

        # Ensure data is up-to-date
        self.category_labels = self.core.fault_types
        self.new_data = self.core.fault_counts_list

        if not self.new_data:
            print("Error: No data to update!")
            return

        # Create bar sets and series
        series = QBarSeries()
        for i, category in enumerate(self.category_labels):
            self.bar_set = QBarSet(category)
            self.bar_set.append(self.new_data[i])
            self.bar_set.setBrush(self.fault_colors[i])  # Set color for each bar
            legend_label = f"{category}: {self.new_data[i]}"
            self.bar_set.setLabel(legend_label)  # Set the label for legend
            series.append(self.bar_set)

        # Create chart and add series
        self.chart = QChart()
        self.chart.addSeries(series)
        # self.chart.createDefaultAxes()
        # self.chart.axes(Qt.Orientation.Horizontal)[0].setVisible(False)
        self.chart.setTitle("Fault Distribution")

        # After creating the chart
        self.axisY = QValueAxis()
        max_value = max([0] + self.new_data)

        self.axisY.setRange(0, max(10, max_value + 5))  # Ensure padding for visibility
        self.axisY.setTickInterval(1)  # Force ticks to appear every 1 unit
        self.axisY.setLabelFormat("%d")  # Ensure integer labels

        self.chart.addAxis(self.axisY, Qt.AlignLeft)
        series.attachAxis(self.axisY)  # Attach the axis to the series

        # self.axisY = QValueAxis()
        # max_value = max([0] + self.new_data)
        # self.axisY.setRange(0, max(10, max_value + 5))  # Ensure padding for better visibility
        #
        # self.bar_series.attachAxis(self.axisY)

        legend = self.chart.legend()

        legend.setAlignment(Qt.AlignBottom)  # Align the legend at the bottom
        # legend.setAlignment(Qt.AlignRight)  # Moves legend to the right, forcing it to wrap
        # legend.setColumnCount(2)
        # Create chart view and set render hint
        self.chart_view = QChartView(self.chart)
        self.chart_view.setObjectName("barChart")
        self.chart_view.setRenderHint(QPainter.RenderHint.Antialiasing)

        self.barGraphWindow_layout.addWidget(self.chart_view, stretch=6)

        self.chart.setBackgroundBrush(QBrush(Qt.transparent))  # Make chart background transparent
        self.chart_view.setBackgroundBrush(QBrush(Qt.transparent))

        # Force chart refresh
        self.chart_view.update()

        # ---- the statistics ----#
        self.stats_dict = self.core.statistics
        self.faults = self.core.specific_fault_counts

        self.units = [" cells", " cells", " V", " A", " °C"]

        """
                    "Total Cells Generated": total_cells,
            "Number of Compromised Cells": compromised_cells,
            "Average Voltage": f"{avg_voltage:.2f}",
            "Average Current": f"{avg_current:.2f}",
            "Average Temperature": f"{avg_temp:.2f}"""
        i = 0
        # Iterate over the dictionary to create rows for each statistic
        for stat, value in self.stats_dict.items():
            # Create the horizontal layout (QHBoxLayout) for each statistic

            stat_layout = QHBoxLayout()

            # Create the QLabel for the statistic name
            stat_label = QLabel(stat)
            stat_label.setAlignment(Qt.AlignLeft)

            print(stat)

            # Create the QLabel for the statistic value
            value_label = QLabel(str(value) + self.units[i])
            value_label.setAlignment(Qt.AlignRight)
            i += 1
            # Add the labels to the horizontal layout
            stat_layout.addWidget(stat_label, stretch=3)
            stat_layout.addWidget(value_label, stretch=1)

            # Add the horizontal layout to the main vertical layout
            self.statisticsWindow_layout.addLayout(stat_layout)

            # Iterate over the dictionary to create rows for each statistic
class CustomFileDialog(QDialog):
    """Custom pop-up for entering a file name before saving."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Save File")
        self.setFixedSize(300, 150)

        layout = QVBoxLayout()

        # Label & Input Field
        self.label = QLabel("Enter file name:")
        self.input_field = QLineEdit(self)

        layout.addWidget(self.label)
        layout.addWidget(self.input_field)

        # Dialog Buttons
        self.button_box = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        self.button_box.accepted.connect(self.check_file_exists)
        self.button_box.rejected.connect(self.reject)  # Close on cancel

        layout.addWidget(self.button_box)
        self.setLayout(layout)

    def check_file_exists(self):
        """Checks if the file exists before accepting."""
        file_name = self.input_field.text().strip()

        if not file_name:
            self.label.setText("⚠️ File name cannot be empty!")
            return

        file_path = os.path.join(os.getcwd(), file_name)  # Save in current directory

        if os.path.exists(file_path):
            self.label.setText("⚠️ File already exists! Choose another name.")
        else:
            self.accept()  # Close dialog and confirm selection

class UniversalDialog(QDialog):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Universal Dialog")
        self.container = QVBoxLayout()
        self.setLayout(self.container)
        self.setFixedSize(400, 200)

    def set_content(self, widget: QWidget):
        # Clear previous content
        while self.container.count():
            child = self.container.takeAt(0)
            if child.widget():
                child.widget().deleteLater()
        self.container.addWidget(widget)

# 🟥 Error Page
class ErrorPage(QWidget):
    def __init__(self, message, parent_dialog):
        super().__init__()
        layout = QVBoxLayout(self)
        layout.addWidget(QLabel(f"Error: {message}"))
        close_btn = QPushButton("OK")
        close_btn.clicked.connect(parent_dialog.close)
        layout.addWidget(close_btn)

# ⚙️ Settings Page
class SettingsPage(QWidget):
    def __init__(self, parent_dialog, default_values, on_save_callback):
        super().__init__()
        layout = QVBoxLayout(self)

        # Project Path
        self.path_input = QLineEdit(default_values.get("path", ""))
        browse_btn = QPushButton("Browse")
        browse_btn.clicked.connect(self.browse_path)
        path_layout = QHBoxLayout()
        path_layout.addWidget(self.path_input)
        path_layout.addWidget(browse_btn)
        layout.addWidget(QLabel("📂 Project Path"))
        layout.addLayout(path_layout)

        # Probability of Failure
        self.prob_input = QDoubleSpinBox()
        self.prob_input.setRange(0.0, 1.0)
        self.prob_input.setSingleStep(0.01)
        self.prob_input.setValue(default_values.get("prob", 0.0))
        layout.addWidget(QLabel("🎲 Probability of Failure"))
        layout.addWidget(self.prob_input)

        # Delay Time
        self.delay_input = QSpinBox()
        self.delay_input.setRange(0, 10000)
        self.delay_input.setSuffix(" ms")
        self.delay_input.setValue(default_values.get("delay", 0))
        layout.addWidget(QLabel("⏱️ Delay Time"))
        layout.addWidget(self.delay_input)

        # Save / Cancel
        btn_layout = QHBoxLayout()
        save_btn = QPushButton("Save")
        cancel_btn = QPushButton("Cancel")
        btn_layout.addWidget(save_btn)
        btn_layout.addWidget(cancel_btn)
        layout.addLayout(btn_layout)

        # Callbacks
        save_btn.clicked.connect(lambda: self._save(parent_dialog, on_save_callback))
        cancel_btn.clicked.connect(parent_dialog.close)

    def _save(self, dialog, callback):
        config = {
            "path": self.path_input.text(),
            "prob": self.prob_input.value(),
            "delay": self.delay_input.value()
        }
        callback(config)
        dialog.close()

    def browse_path(self):
        path = QFileDialog.getExistingDirectory(self, "Select Project Directory")
        if path:
            self.path_input.setText(path)

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.core = Core()  # Initialize Core here
        #self.dialog = UniversalDialog()
        self.home = HermesHome(self.core)
        self.setWindowTitle('Hermes')

        self.stack = QStackedWidget(self)
        self.setCentralWidget(self.stack)

        self.window_home = HermesHome(self.core)
        self.window_config = HermesConfig(self.core, HermesHome)
        # self.window_results = HermesResults(self.core)

        # ---- adding the windows to the stack for the page order ----#
        # self.stack.addWidget(self.window_results)
        self.stack.addWidget(self.window_home)
        self.stack.addWidget(self.window_config)
        # self.stack.addWidget(self.window_results)

        self.core.response_received.connect(self.window_config.echo, Qt.QueuedConnection)
        self.window_config.reset_requested.connect(self.full_reset)

        self.resize(1200, 600)

    def show_page1(self):
        self.stack.setCurrentWidget(self.window_home)

    def show_page2(self):
        self.stack.setCurrentWidget(self.window_config)

    def show_page3(self):
        self.stack.setCurrentWidget(self.window_results)

    def full_reset(self):
        print("Full UI reset triggered.")

        # Remove all pages from the stack
        while self.stack.count() > 0:
            widget = self.stack.widget(0)
            self.stack.removeWidget(widget)
            widget.deleteLater()

        # Recreate the pages fresh
        self.window_home = HermesHome(self.core)
        self.window_config = HermesConfig(self.core, HermesHome)

        # Reconnect the reset signal
        self.window_config.reset_requested.connect(self.full_reset)

        # Re-add to stack
        self.stack.addWidget(self.window_home)
        self.stack.addWidget(self.window_config)

        self.stack.setCurrentWidget(self.window_home)  # Go back to start


def load_stylesheet(self, file_path):
    """Loads a stylesheet from a file."""
    print("The file path is " + str(file_path))
    file = QFile(file_path)
    print("The file is " + str(file))
    if not file.open(QFile.ReadOnly):
        raise Exception(f"Cannot open file: {file_path}")
    stream = QTextStream(file)
    stylesheet = stream.readAll()
    file.close()
    return stylesheet


def launch_app():
    app = QApplication([])
    window_home = MainWindow()
    window_home.show()
    app.exec()


if __name__ == "__main__":
    launch_app()
