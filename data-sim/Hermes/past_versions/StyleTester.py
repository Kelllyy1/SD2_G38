from PySide6.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel, QSizePolicy
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

# Load QSS
stylesheet = load_stylesheet("../resources/style.qss")
app.setStyleSheet(stylesheet)

# Create container widget
container = QWidget()
container.setObjectName("container")

# Create outer box (Easel)
easel = QWidget()
easel.setObjectName("easel")

easelRight = QWidget()
easelRight.setObjectName("easelRight")

#left widget
# Create inner box (Canvas)
canvas = QLabel("Inner Canvas Box")
canvas.setObjectName("canvas")
canvas.setAlignment(Qt.AlignCenter)  # Align text in center

title = QLabel("Title")
title.setObjectName("title")
title.setAlignment(Qt.AlignCenter)  # Align text in center

# Make sure widgets resize properly but do not expand uncontrollably
easel.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)
canvas.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)
container.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)

# #left widget
easel_layout = QVBoxLayout()
easel_layout.addWidget(title, stretch=.05)
easel_layout.addWidget(canvas, stretch=1)

easelRight_layout = QVBoxLayout()
easelRight_layout.addWidget(title, stretch=.05)
easelRight_layout.addWidget(canvas, stretch=1)


easel_layout.setAlignment(Qt.AlignCenter)  # Center canvas inside easel
easel.setLayout(easel_layout)

easelRight_layout.setAlignment(Qt.AlignRight)  # Center canvas inside easel
easel.setLayout(easelRight_layout)

# Layout for container (contains easel)
container_layout = QVBoxLayout()
container_layout.addWidget(easel)

container_layout.setAlignment(Qt.AlignCenter)  # Center easel inside container
container.setLayout(container_layout)

# Create the main window
window = QWidget()
window.setWindowTitle("Centered Layout Example")

# Main window layout
window_layout = QVBoxLayout()
window_layout.addWidget(container)
window_layout.setAlignment(Qt.AlignCenter)  # Center container inside the window
window.setLayout(window_layout)

# Set initial size and show window
window.resize(1000, 600)  # Start with a reasonable size
window.show()

app.exec()
