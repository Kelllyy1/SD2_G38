from PySide6.QtWidgets import QApplication, QWidget, QVBoxLayout, QFormLayout, QPushButton, QLineEdit, QRadioButton, QLabel
from Core import Core

class FormWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("User Form")
        self.setStyleSheet(open("../resources/style.qss").read())  # Apply QSS styling

        # Create layout
        self.layout = QFormLayout()

        # Create form inputs
        self.name_input = QLineEdit(self)
        self.name_input.setPlaceholderText("Name")

        self.age_input = QLineEdit(self)
        self.age_input.setPlaceholderText("Age")

        self.location_input = QLineEdit(self)
        self.location_input.setPlaceholderText("Location")

        # Radio buttons for 'Call'
        self.call_yes = QRadioButton("Yes", self)
        self.call_no = QRadioButton("No", self)

        # Add the form fields to layout
        self.layout.addRow("Name:", self.name_input)
        self.layout.addRow("Age:", self.age_input)
        self.layout.addRow("Location:", self.location_input)
        self.layout.addRow("Call:", self.call_yes)
        self.layout.addRow("", self.call_no)

        # Submit button
        self.submit_button = QPushButton("Submit", self)
        self.submit_button.clicked.connect(self.show_results)
        self.layout.addWidget(self.submit_button)

        self.setLayout(self.layout)

    def show_results(self):
        # Capture data and process it using the Core class
        core = Core()
        results = core.process_input(self.name_input.text(), self.age_input.text(), self.location_input.text(),
                                     self.call_yes.isChecked())

        # Display results in a new window
        self.results_window = QWidget()
        self.results_window.setWindowTitle("Results")

        # Ensure results window stays open independently
        results_layout = QVBoxLayout()
        results_label = QLabel(f"Name: {results['name']}\nAge: {results['age']}\nLocation: {results['location']}\nCall: {results['call']}")
        results_layout.addWidget(results_label)
        self.results_window.setLayout(results_layout)

        # Show results window
        self.results_window.show()

if __name__ == "__main__":
    app = QApplication([])
    window = FormWindow()
    window.show()
    app.exec()
