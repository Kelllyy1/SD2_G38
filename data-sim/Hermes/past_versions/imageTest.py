import sys
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QToolBar, QLabel, QStatusBar, QWidget, QVBoxLayout
)
from PySide6.QtGui import QIcon, QAction

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Navbar Example")

        # Central Widget (Optional)
        central_widget = QWidget()
        layout = QVBoxLayout(central_widget)
        label = QLabel("Main Content Area")
        layout.addWidget(label)
        self.setCentralWidget(central_widget)

        # Create a toolbar (navbar)
        toolbar = QToolBar("My Toolbar")
        self.addToolBar(toolbar)

        # Create actions for the toolbar
        action_home = QAction(QIcon.fromTheme("go-home"), "Home", self) #Using system icons, or provide your own path
        action_profile = QAction(QIcon.fromTheme("face-profile"), "Profile", self)
        action_settings = QAction(QIcon.fromTheme("preferences-system"), "Settings", self)
        action_exit = QAction(QIcon.fromTheme("application-exit"), "Exit", self)

        # Add actions to the toolbar
        toolbar.addAction(action_home)
        toolbar.addAction(action_profile)
        toolbar.addAction(action_settings)
        toolbar.addSeparator() # Add a visual separator
        toolbar.addAction(action_exit)

        # Connect actions to functions (optional)
        action_home.triggered.connect(self.on_home_clicked)
        action_profile.triggered.connect(self.on_profile_clicked)
        action_settings.triggered.connect(self.on_settings_clicked)
        action_exit.triggered.connect(self.close) # Close the window

        # Status bar
        self.statusBar().showMessage("Ready")

    def on_home_clicked(self):
        self.statusBar().showMessage("Home clicked")
        # Add your home action logic here

    def on_profile_clicked(self):
        self.statusBar().showMessage("Profile clicked")
        # Add your profile action logic here

    def on_settings_clicked(self):
        self.statusBar().showMessage("Settings clicked")
        # Add your settings action logic here


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())