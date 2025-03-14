from PySide6.QtGui import QPixmap, QFontDatabase, QAction
from PySide6.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel, QSizePolicy, QHBoxLayout, QLineEdit, \
    QRadioButton, QPushButton, QMenuBar, QMenu
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

# Create the navigation bar (menu bar)
menu_bar = QMenuBar()

# File menu
file_menu = QMenu("File", menu_bar)
menu_bar.addMenu(file_menu)

# Add actions to File menu
new_action = QAction("New", window)
open_action = QAction("Open", window)
save_action = QAction("Save", window)
exit_action = QAction("Exit", window)
exit_action.triggered.connect(app.quit)  # Connect exit action to close the app

file_menu.addAction(new_action)
file_menu.addAction(open_action)
file_menu.addAction(save_action)
file_menu.addSeparator()
file_menu.addAction(exit_action)

# Edit menu
edit_menu = QMenu("Edit", menu_bar)
menu_bar.addMenu(edit_menu)

# View menu
view_menu = QMenu("View", menu_bar)
menu_bar.addMenu(view_menu)

# Help menu
help_menu = QMenu("Help", menu_bar)
menu_bar.addMenu(help_menu)

# Left panel
left = QWidget()
left.setObjectName("left")

left_layout = QHBoxLayout()
left_layout.setAlignment(Qt.AlignCenter)  # Center canvas inside easel
left.setLayout(left_layout)

# Sidebar
sidebar = QWidget()
sidebar.setObjectName("sidebar")

sidebar_layout = QVBoxLayout()
sidebar_layout.setAlignment(Qt.AlignCenter)
sidebar.setLayout(sidebar_layout)

left_layout.addWidget(sidebar, stretch=1)
sidebar.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

# Right panel
right = QWidget()
right.setObjectName("right")

right_layout = QVBoxLayout()
right_layout.setAlignment(Qt.AlignCenter)  # Center canvas inside easel
right.setLayout(right_layout)

# Main layout
window_layout = QVBoxLayout()
window_layout.addWidget(menu_bar)  # Add menu bar at the top
main_content_layout = QHBoxLayout()
main_content_layout.addWidget(left, stretch=1)
main_content_layout.addWidget(right, stretch=4)
main_content_layout.setAlignment(Qt.AlignCenter)

window_layout.addLayout(main_content_layout)  # Add main content layout
window.setLayout(window_layout)

# Set initial size and show window
window.resize(1200, 600)  # Start with a reasonable size
window.show()

app.exec()
