import sys
from PySide6.QtCore import Qt
from PySide6.QtWidgets import QApplication, QWidget, QVBoxLayout, QPushButton, QStackedWidget, QLabel


class MainWindow(QWidget):
    def __init__(self):
        super().__init__()

        # Set up the stacked widget
        self.stacked_widget = QStackedWidget()

        # Create pages
        self.home_page = QLabel("This is the Home Page")
        self.results_page = QLabel("This is the Results Page")

        # Add pages to stacked widget
        self.stacked_widget.addWidget(self.home_page)
        self.stacked_widget.addWidget(self.results_page)

        # Set up layout
        layout = QVBoxLayout()

        # Buttons to switch between pages
        self.home_button = QPushButton("Go to Home")
        self.results_button = QPushButton("Go to Results")

        self.home_button.clicked.connect(self.show_home_page)
        self.results_button.clicked.connect(self.show_results_page)

        layout.addWidget(self.home_button)
        layout.addWidget(self.results_button)
        layout.addWidget(self.stacked_widget)

        self.setLayout(layout)

    def show_home_page(self):
        self.stacked_widget.setCurrentWidget(self.home_page)

    def show_results_page(self):
        self.stacked_widget.setCurrentWidget(self.results_page)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.setWindowTitle("Scene Manager Example")
    window.show()
    sys.exit(app.exec())
