#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QProcess>
#include <QtWidgets/QApplication>
#include <QLabel>
#include <QPixmap>
#include <QGraphicsOpacityEffect>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ---------- 背景图半透明 ----------
    bgLabel = new QLabel(this);  // 注意是 QLabel
    QPixmap bgPixmap("C:\\Users\\tangz\\Pictures\\42.jpg");
    bgLabel->setPixmap(bgPixmap);
    bgLabel->setScaledContents(true);
    bgLabel->setGeometry(0, 0, this->width(), this->height());

    QGraphicsOpacityEffect *opacity = new QGraphicsOpacityEffect(bgLabel);
    opacity->setOpacity(0.5); // 半透明
    bgLabel->setGraphicsEffect(opacity);
    bgLabel->lower(); // 放到最底层
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 窗口大小变化时自动调整背景
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if(bgLabel)
        bgLabel->setGeometry(0, 0, width(), height());
}

// 浏览按钮
void MainWindow::on_browseButton_clicked()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        "选择视频文件",
        "",
        "Video Files (*.mp4 *.avi *.mkv)"
        );

    if (!file.isEmpty()) {
        ui->videoPathEdit->setText(file);
    }
}

// 开始分析按钮
void MainWindow::on_startButton_clicked()
{
    QString videoPath = ui->videoPathEdit->text();

    if (videoPath.isEmpty()) {
        ui->logTextEdit->append("请先选择视频");
        return;
    }

    QString exePath = QApplication::applicationDirPath() + "/skill_detector.exe";
    QString outputCsv = "out.csv";

    ui->logTextEdit->append("开始分析...");

    QProcess *process = new QProcess(this);

    QStringList args;
    args << "batch"
         << videoPath
         << outputCsv;

    process->start(exePath, args);

    // 输出日志
    connect(process, &QProcess::readyReadStandardOutput, [=]() {
        QString out = process->readAllStandardOutput();
        ui->logTextEdit->append(out);
    });

    // 错误日志
    connect(process, &QProcess::readyReadStandardError, [=]() {
        QString err = process->readAllStandardError();
        ui->logTextEdit->append("[错误] " + err);
    });

    // 完成
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=]() {
                ui->logTextEdit->append("分析完成，CSV已生成");
            });
}
