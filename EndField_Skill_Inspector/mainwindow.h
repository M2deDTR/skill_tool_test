#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QResizeEvent>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;
private slots:
    void on_browseButton_clicked();

    void on_startButton_clicked();

private:
    QMediaPlayer *player;
    QLabel *bgLabel;

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H

