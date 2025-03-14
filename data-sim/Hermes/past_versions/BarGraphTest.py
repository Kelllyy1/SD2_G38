from PySide6.QtCharts import QChartView, QChart, QBarSeries, QBarSet
from PySide6.QtGui import QPainter, QColor, Qt
from PySide6.QtWidgets import QApplication, QWidget, QVBoxLayout

class BarChart(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Bar Chart with Different Colors")

        # Data for the bar chart
        categories = ["Category 1", "Category 2", "Category 3", "Category 4", "Category 5"]
        values = [10, 25, 15, 30, 20]
        colors = [QColor("red"), QColor("blue"), QColor("green"), QColor("orange"), QColor("purple")]

        # Create bar sets and series
        series = QBarSeries()
        for i, category in enumerate(categories):
            bar_set = QBarSet(category)
            bar_set.append(values[i])
            bar_set.setBrush(colors[i]) # Set color for each bar
            series.append(bar_set)

        # Create chart and add series
        chart = QChart()
        chart.addSeries(series)
        chart.createDefaultAxes()

        chart.axes(Qt.Orientation.Horizontal)[0].setVisible(False)
        chart.setTitle("Bar Chart with Different Colors")

        # Create chart view and set render hint
        chart_view = QChartView(chart)
        chart_view.setRenderHint(QPainter.RenderHint.Antialiasing)

        # Set layout
        layout = QVBoxLayout()
        layout.addWidget(chart_view)
        self.setLayout(layout)

if __name__ == "__main__":
    app = QApplication([])
    window = BarChart()
    window.show()
    app.exec()