from PySide6.QtGui import QPainter
from PySide6.QtWidgets import QApplication, QMainWindow
from PySide6.QtCharts import QChart, QChartView, QLineSeries
from PySide6.QtCore import Qt

app = QApplication([])

series = QLineSeries()
series.append(0, 6)
series.append(2, 4)
series.append(3, 8)
series.append(7, 4)
series.append(10, 5)

chart = QChart()
chart.legend().setVisible(False)
chart.addSeries(series)
chart.createDefaultAxes()
chart.setTitle("Simple line chart")

chart_view = QChartView(chart)
chart_view.setRenderHint(QPainter.RenderHint.Antialiasing) # Corrected line

window = QMainWindow()
window.setCentralWidget(chart_view)
window.resize(640, 480)
window.show()

app.exec()