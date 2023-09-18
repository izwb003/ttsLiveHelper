#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    void fillSettings();
    void applySettings();

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButtonBrowseBLiveChat_clicked();

    void on_pushButtonBrowseDanmuji_clicked();

    void on_pushButtonBrowseOBS_clicked();

    void on_pushButtonYes_clicked();

    void on_pushButtonNo_clicked();

    void on_pushButtonApply_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
