from PySide6.QtWidgets import QLineEdit, QRadioButton, QWidget, QVBoxLayout, QPushButton
import sys

from PySide6.QtWidgets import QApplication


class MyApp(QWidget):
    def __init__(self):
        super().__init__()
        self.init_ui()

    def init_ui(self):
        # Create QLineEdits and QRadioButtons in app_launch()
        self.lineEdit_1 = QLineEdit(self)
        self.lineEdit_2 = QLineEdit(self)
        self.lineEdit_3 = QLineEdit(self)

        self.radioButton_1 = QRadioButton("Option 1", self)
        self.radioButton_2 = QRadioButton("Option 2", self)

        # Button to trigger data collection
        self.collect_button = QPushButton("Collect Data", self)
        self.collect_button.clicked.connect(self.app_launch)

        # Layout for widgets
        layout = QVBoxLayout()
        layout.addWidget(self.lineEdit_1)
        layout.addWidget(self.lineEdit_2)
        layout.addWidget(self.lineEdit_3)
        layout.addWidget(self.radioButton_1)
        layout.addWidget(self.radioButton_2)
        layout.addWidget(self.collect_button)
        self.setLayout(layout)

    def app_launch(self):
        # Pass QLineEdits and QRadioButtons to collect_form_data
        self.collect_form_data(self.lineEdit_1, self.lineEdit_2, self.lineEdit_3,
                               self.radioButton_1, self.radioButton_2)

    def collect_form_data(self, lineEdit_1, lineEdit_2, lineEdit_3, radioButton_1, radioButton_2):
        # Collect data from QLineEdits
        input1 = lineEdit_1.text()
        input2 = lineEdit_2.text()
        input3 = lineEdit_3.text()

        # Check which QRadioButton is selected
        selected_radio_button = None
        if radioButton_1.isChecked():
            selected_radio_button = "RadioButton 1"
        elif radioButton_2.isChecked():
            selected_radio_button = "RadioButton 2"
        else:
            selected_radio_button = "None"  # If no radio button is selected

        # Print collected data to the console
        print(f"Input 1: {input1}")
        print(f"Input 2: {input2}")
        print(f"Input 3: {input3}")
        print(f"Selected Radio Button: {selected_radio_button}")


# Run the application
app = QApplication(sys.argv)
window = MyApp()
window.show()
sys.exit(app.exec_())
