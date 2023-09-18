#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QLabel>
#include <QListWidgetItem>
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
    void closeEvent(QCloseEvent *event);
    void getOBSSetup();
    void liveStatusChange(bool);
    void liveTitleUpdateRequest(QString);
    void managerWebSocketConnect();
    void managerWebSocketConnectStatusUpdate(bool);
    void setLiveBegin();
    void showTime();
    void updateBilibiliLiveStatus();
    void OBSWebSocketConnect();
    void OBSWebSocketConnectStatusUpdate(bool);

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void do_MainWindow_Loaded();
    void do_OBSWebSocketMessageReceivedSignal();

private slots:
    void do_ManagerWebSocketConnected();
    void do_ManagerWebSocketDisconnected();
    void do_ManagerWebSocketMessageReceived(QString);
    void do_OBSWebSocketConnected();
    void do_OBSWebSocketDisconnected();
    void do_OBSWebSocketMessageReceived(QString);
    void do_shutDown();
    void do_MainWindow_AfterShow();

    void on_pushButtonOpenBilibiliLiveSettings_clicked();

    void on_pushButtonLiveTitleUpdate_clicked();

    void on_pushButton_clicked();

    void on_action_BilibiliStartLive_triggered();

    void on_pushButtonTest_clicked();

    void on_action_BilibiliStopLive_triggered();

    void on_action_OBSStartLive_triggered();

    void on_action_OBSStopLive_triggered();

    void on_action_OnekeyStartLive_triggered();

    void on_action_OnekeyStopLive_triggered();

    void on_action_About_triggered();

    void on_action_OpenDanmuji_triggered();

    void on_action_OpenBlivechat_triggered();

    void on_action_OpenOBS_triggered();

    void on_action_ShutDanmuji_triggered();

    void on_action_ShutBlivechat_triggered();

    void on_action_ShutOBS_triggered();

    void on_action_SetBlivechat_triggered();

    void on_pushButtonLiveNowEditConfirm_clicked();

    void on_checkBoxMusic_clicked(bool checked);

    void on_checkBoxLyrics_clicked(bool checked);

    void on_pushButtonRestNotification_clicked();

    void on_pushButtonRestNotificationModified_clicked();

    void on_pushButtonMute_clicked();

    void on_pushButtonShut_clicked();

    void on_listWidgetLiveNowHistory_itemClicked(QListWidgetItem *item);

    void on_pushButtonLiveNowFastSetting_1_clicked();

    void on_pushButtonLiveNowFastSetting_2_clicked();

    void on_pushButtonLiveNowFastSetting_3_clicked();

    void on_pushButtonLiveNowFastSetting_4_clicked();

    void on_pushButtonLiveNowFastSetting_5_clicked();

    void on_pushButtonLiveNowFastSetting_6_clicked();

    void on_action_allowManagerHelp_triggered(bool checked);

    void on_action_Settings_triggered();

private:
    Ui::MainWindow *ui;
};

bool setOBSRTMPSettings();
#endif // MAINWINDOW_H
