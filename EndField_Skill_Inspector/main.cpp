#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyleSheet(R"(
QWidget {
    background-color: #14161c;
    color: #e6e6e6;
    font-size: 13px;
}

QLineEdit {
    background-color: #1e222b;
    border: 1px solid #2c3442;
    padding: 6px;
    border-radius: 6px;
    color: #ffffff;
}

QLineEdit:focus {
    border: 1px solid #4a90e2;
}

QPushButton {
    background-color: #2a2f3a;
    border: 1px solid #3a4252;
    padding: 7px 12px;
    border-radius: 6px;
}

QPushButton:hover {
    background-color: #3a4252;
}

QPushButton:pressed {
    background-color: #4a5670;
}

QTextEdit {
    background-color: #10131a;
    border: 1px solid #2a3140;
    border-radius: 6px;
}

QGroupBox {
    border: 1px solid #2a3140;
    margin-top: 10px;
    border-radius: 6px;
    padding: 8px;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 5px;
}

QScrollBar:vertical {
    background: #1a1f29;
    width: 10px;
}

QScrollBar::handle:vertical {
    background: #3a4252;
    border-radius: 4px;
}

QScrollBar::handle:vertical:hover {
    background: #4a5670;
}
)");
    MainWindow w;
    w.show();
    return a.exec();
}

