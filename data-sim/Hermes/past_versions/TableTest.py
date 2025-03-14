import sys
from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QPainter, QPen, QColor
from PySide6.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel


class CircularProgressBorder(QWidget):
    def __init__(self):
        super().__init__()
        self.progress_value = 0  # Initial progress value (0-100)
        self.setMinimumSize(150, 150)  # Set widget size

        # Timer to simulate progress updates
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_progress)
        self.timer.start(100)  # Update every 100ms

    def update_progress(self):
        """Simulates progress increment."""
        if self.progress_value < 100:
            self.progress_value += 1
            self.update()  # Repaint the widget
        else:
            self.timer.stop()  # Stop when complete

    def paintEvent(self, event):
        """Handles the drawing of the circular progress border."""
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)  # Smooth rendering

        # Define the size of the progress bar
        rect = self.rect().adjusted(10, 10, -10, -10)  # Padding inside the widget

        # Draw the background circle
        pen_bg = QPen(QColor(200, 200, 200), 10, Qt.SolidLine)  # Light gray background
        painter.setPen(pen_bg)
        painter.drawArc(rect, 0, 360 * 16)  # Full circle

        # Draw the progress arc
        pen_progress = QPen(QColor(76, 175, 80), 10, Qt.SolidLine)  # Green border
        painter.setPen(pen_progress)
        angle = int(360 * 16 * (self.progress_value / 100))  # Convert percentage to angle
        painter.drawArc(rect, 90 * 16, -angle)  # Start at top and go counterclockwise


class MainWindow(QWidget):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Circular Status Bar Example")
        self.setGeometry(200, 200, 200, 200)

        layout = QVBoxLayout()
        self.progress_widget = CircularProgressBorder()
        layout.addWidget(self.progress_widget)

        # Label for displaying progress percentage
        self.label = QLabel("0%", self)
        self.label.setAlignment(Qt.AlignCenter)
        layout.addWidget(self.label)

        self.setLayout(layout)

        # Timer to update the percentage label
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_label)
        self.timer.start(100)

    def update_label(self):
        """Updates the percentage label based on progress."""
        self.label.setText(f"{self.progress_widget.progress_value}%")


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())
