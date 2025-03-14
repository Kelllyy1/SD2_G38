import sys
import math
from PySide6.QtWidgets import QApplication, QWidget, QVBoxLayout, QLineEdit, QPushButton, QScrollArea, QMessageBox

class DynamicForm(QWidget):
    def __init__(self, total_modules=16):  # Set total modules dynamically
        super().__init__()

        self.setWindowTitle("Dynamic Form Example")
        self.setGeometry(200, 200, 400, 400)

        # Calculate max fields based on square root
        self.max_fields = int(math.sqrt(total_modules))
        self.fields = []  # List to store QLineEdits

        # Main layout
        self.layout = QVBoxLayout(self)

        # Scroll area (for overflow)
        self.scroll_area = QScrollArea()
        self.scroll_area.setWidgetResizable(True)
        self.scroll_content = QWidget()
        self.form_layout = QVBoxLayout(self.scroll_content)

        self.scroll_area.setWidget(self.scroll_content)
        self.layout.addWidget(self.scroll_area)

        # Button to add new fields
        self.add_button = QPushButton(f"+ Add Field (Max: {self.max_fields})")
        self.add_button.clicked.connect(self.add_field)
        self.layout.addWidget(self.add_button)

    def add_field(self):
        """Add a new QLineEdit field, but limit the number."""
        if len(self.fields) < self.max_fields:
            new_field = QLineEdit()
            new_field.setPlaceholderText(f"Response {len(self.fields) + 1}")
            self.form_layout.addWidget(new_field)
            self.fields.append(new_field)
        else:
            QMessageBox.warning(self, "Limit Reached", f"You can only add {self.max_fields} fields.")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    total_modules = 16  # Change this value as needed
    window = DynamicForm(total_modules)
    window.show()
    sys.exit(app.exec())
