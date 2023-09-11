#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QMainWindow>
#include <QWebSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    QLabel *label_ManagerConnectInfo;
    QLabel *label_OBSConnectInfo;
    QLabel *label_LiveStatus;
    void buildUI();
    void showTime();
    void updateBilibiliLiveStatus();
    void liveStatusChange(bool);
    void OBSWebSocketConnect();
    void OBSWebSocketConnectStatusUpdate(bool);

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void do_MainWindow_Loaded();

private slots:
    void do_OBSWebSocketConnected();
    void do_OBSWebSocketDisconnected();
    void do_OBSWebSocketMessageReceived(QString);
    void do_MainWindow_AfterShow();

    void on_pushButtonOpenBilibiliLiveSettings_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
