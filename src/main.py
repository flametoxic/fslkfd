import sys
from PyQt5.QtWidgets import QApplication, QWidget, QPushButton, QLineEdit, QFileDialog, QLabel
from PyQt5.QtCore import QCoreApplication
import subprocess

class Window(QWidget):
  def __init__(self):
    super().__init__()

    self.initUI()

  def initUI(self):
    self.setWindowTitle('Server start')
    self.setGeometry(300, 300, 300, 200)

    # Текстовое поле "Введите порт"
    self.port_label = QLabel("Введите порт:", self)
    self.port_label.move(10, 10)

    # Поле ввода цифр
    self.number_input = QLineEdit(self)
    self.number_input.move(10, 30)
    self.number_input.resize(270, 30)

    # Кнопка выбора папки
    self.folder_select_button = QPushButton('Выбрать папку', self)
    self.folder_select_button.move(10, 70)
    self.folder_select_button.clicked.connect(self.select_folder)

    # Текстовое поле для пути к папке
    self.folder_input = QLineEdit(self)
    self.folder_input.move(10, 110)
    self.folder_input.resize(270, 30)

    # Кнопка запуска сервера
    self.start_server_button = QPushButton('Запустить сервер', self)
    self.start_server_button.move(160, 150) # Справа внизу
    self.start_server_button.clicked.connect(self.start_server)

    # Кнопка выхода
    self.exit_button = QPushButton('Выход', self)
    self.exit_button.move(10, 150) # Слева внизу
    self.exit_button.clicked.connect(QCoreApplication.instance().quit)

    self.show()

  def select_folder(self):
    # Открываем диалог выбора папки
    self.selected_folder = QFileDialog.getExistingDirectory(self, "Выберите папку")
    self.folder_input.setText(self.selected_folder) # Устанавливаем путь в поле ввода

  def start_server(self):
    # Получаем цифры из поля ввода
    port = self.number_input.text()
    folder = self.folder_input.text() # Получаем путь из поля ввода

    # Запускаем сервер с помощью subprocess.Popen
    if folder: # Проверка, пустое ли поле
      command = ['../build/server', port, folder]
      subprocess.Popen(command)
      print(f"Сервер запущен с параметрами: {command}")
    else:
      print("Введите путь к папке!")

if __name__ == '__main__':
  app = QApplication(sys.argv)
  window = Window()
  sys.exit(app.exec_())