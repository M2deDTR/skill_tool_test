#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyleSheet(R"(
/* 输入框 */
QLineEdit {
    background-color: rgba(0,0,0,0);  /* 背景透明 */
    border: 1px solid black;          /* 黑色边框固定 */
    padding: 6px;
    border-radius: 6px;
    color: black;                     /* 文字黑色 */
}
QLineEdit:focus {
    border: 1px solid black;          /* 焦点仍黑色 */
}
QLineEdit:hover {
    background-color: rgba(255,0,0,0.2); /* 半透明红色高亮 */
}

/* 文本输出框 */
QTextEdit {
    background-color: rgba(0,0,0,0);  /* 背景透明 */
    border: 1px solid black;
    border-radius: 6px;
    color: black;                     /* 文字黑色 */
}
/* QTextEdit 悬停不变红，保持透明 */

/* 按钮 */
QPushButton {
    background-color: rgba(0,0,0,0);
    border: 1px solid black;
    padding: 7px 12px;
    border-radius: 6px;
    color: black;
}
QPushButton:hover {
    background-color: rgba(255,0,0,0.2); /* 半透明红色高亮 */
}
QPushButton:pressed {
    background-color: rgba(255,0,0,0.3);
}

/* 分组框 */
QGroupBox {
    border: 1px solid black;
    margin-top: 10px;
    border-radius: 6px;
    padding: 8px;
    background-color: rgba(0,0,0,0);
}
QGroupBox:hover {
    background-color: rgba(255,0,0,0.2);
}

/* 滚动条 */
QScrollBar:vertical {
    background: rgba(0,0,0,0);
    width: 10px;
}
QScrollBar::handle:vertical {
    background: rgba(0,0,0,0.3);
    border-radius: 4px;
}
QScrollBar::handle:vertical:hover {
    background: rgba(255,0,0,0.3);
}
)");

    MainWindow w;
    w.show();
    return a.exec();
}
