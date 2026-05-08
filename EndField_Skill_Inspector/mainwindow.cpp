#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QProcess>
#include <QtWidgets/QApplication>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


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


