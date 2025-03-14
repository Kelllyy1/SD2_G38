from PySide6.QtWidgets import QApplication, QWidget, QVBoxLayout, QHBoxLayout, QLabel, QSizePolicy
from PySide6.QtCore import Qt

class StatsWindow(QWidget):
    def __init__(self):
        super().__init__()

        self.stats_dict = {
            "Overcharge": 5,
            "Overdischarge": 3,
            "Overcurrent": 8,
            "Overheating": 1,
            "Thermal Runaway": 0
        }

        self.main_layout = QVBoxLayout()
        self.main_layout.setSpacing(0)
        self.main_layout.setContentsMargins(0, 0, 0, 0)



        for stat, value in self.stats_dict.items():

            stats = QWidget()
            stats.setObjectName("stat")

            stats_layout = QHBoxLayout()
            stats.setLayout(stats_layout)



            stat_label = QLabel(stat)
            stat_label.setAlignment(Qt.AlignLeft)
            stat_label.setStyleSheet("padding: 0px; margin: 0px;") # Remove padding and margins

            value_label = QLabel(str(value))
            value_label.setAlignment(Qt.AlignRight)
            value_label.setStyleSheet("padding: 0px; margin: 0px;") # Remove padding and margins

            stats_layout.addWidget(stat_label, stretch=3)
            stats_layout.addWidget(value_label, stretch=1)

            self.main_layout.addWidget(stats)


        self.setLayout(self.main_layout)
        self.main_layout.setSpacing(0)
        self.main_layout.setContentsMargins(0, 0, 0, 0)
        self.setWindowTitle("Statistics")
        self.show()

if __name__ == "__main__":
    app = QApplication([])
    window = StatsWindow()
    app.exec()