#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "dialogabout.h"
#include "dialogobsrtmpsettingsinfo.h"

#include "global.h"
#include "qobjectdefs.h"
#include "wintoastlib.h"

#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QWebSocket>

QSettings getSettings("Settings.ini", QSettings::IniFormat);

QTimer *currentTimeTimer;
QTimer *bilibiliLiveStatusUpdateTimer;
QTimer *OBSSetupUpdateTimer;

QTime time_passed_QTime(0,0,0); //Live time

QWebSocket *OBSWebSocket = new QWebSocket();
QString latestOBSWebSocketMessageStr;
QJsonDocument latestOBSWebSocketMessageJSONDoc;

QWebSocket *managerWebSocket = new QWebSocket();

bool isOBSWebSocketConnected = false;
bool isOnAir = false;
bool isManagerWebSocketConnected = false;
bool allowManagerHelp = true;

QString liveRTMPAddress = "";
QString liveRTMPCode = "";
int musicInfoSceneItemId = 0;
int lyricsSceneItemId = 0;
int restNotificationSceneItemId = 0;
int videoCaptureSceneItemId = 0;
bool restNotificationEnabled = false;
bool isMicrophoneMuted = false;
bool willTurnOff = false;

bool setOBSRTMPSettings()
{
    if(isOBSWebSocketConnected)
    {
        QString requestUuid = QUuid::createUuid().toString();
        QJsonObject jsonEditOBSRTMPSettings;
        jsonEditOBSRTMPSettings.insert("op", 6);
        QJsonObject jsonEditOBSRTMPSettingsData;
        jsonEditOBSRTMPSettingsData.insert("requestType", "SetStreamServiceSettings");
        jsonEditOBSRTMPSettingsData.insert("requestId", requestUuid);
        QJsonObject jsonEditOBSRTMPSettingsRequestData;
        jsonEditOBSRTMPSettingsRequestData.insert("streamServiceType", "rtmp_custom");
        QJsonObject jsonEditOBSRTMPSettingsStreamServiceSettings;
        jsonEditOBSRTMPSettingsStreamServiceSettings.insert("server", liveRTMPAddress);
        jsonEditOBSRTMPSettingsStreamServiceSettings.insert("key", liveRTMPCode);
        jsonEditOBSRTMPSettingsRequestData.insert("streamServiceSettings", jsonEditOBSRTMPSettingsStreamServiceSettings);
        jsonEditOBSRTMPSettingsData.insert("requestData", jsonEditOBSRTMPSettingsRequestData);
        jsonEditOBSRTMPSettings.insert("d", jsonEditOBSRTMPSettingsData);
        QJsonDocument jsonDocEditOBSRTMPSettings(jsonEditOBSRTMPSettings);
        OBSWebSocket->sendTextMessage(jsonDocEditOBSRTMPSettings.toJson());
        return true;
    }
    else
        return false;
}

void MainWindow::buildUI()
{
    //statusbar
    //ManagerConnectInfo
    label_ManagerConnectInfo = new QLabel(this);
    label_ManagerConnectInfo->setText("房管远程未连接");
    ui->statusbar->addPermanentWidget(label_ManagerConnectInfo);
    //OBSConnectInfo
    label_OBSConnectInfo = new QLabel(this);
    label_OBSConnectInfo->setText("OBS未连接");
    ui->statusbar->addPermanentWidget(label_OBSConnectInfo);
    //liveTimeInfo
    label_LiveStatus = new QLabel(this);
    label_LiveStatus->setText("直播未开始");
    ui->statusbar->addPermanentWidget(label_LiveStatus);

    //LiveNow FastSetting Content
    ui->pushButtonLiveNowFastSetting_1->setText(getSettings.value("Live/FastSetting_1_Label").toString());
    ui->pushButtonLiveNowFastSetting_2->setText(getSettings.value("Live/FastSetting_2_Label").toString());
    ui->pushButtonLiveNowFastSetting_3->setText(getSettings.value("Live/FastSetting_3_Label").toString());
    ui->pushButtonLiveNowFastSetting_4->setText(getSettings.value("Live/FastSetting_4_Label").toString());
    ui->pushButtonLiveNowFastSetting_5->setText(getSettings.value("Live/FastSetting_5_Label").toString());
    ui->pushButtonLiveNowFastSetting_6->setText(getSettings.value("Live/FastSetting_6_Label").toString());
}


void MainWindow::closeEvent(QCloseEvent *event)
{
#ifndef DEBUG
    if(getSettings.value("Execute/Danmuji_OnClose").toBool())
        on_action_ShutDanmuji_triggered();
    if(getSettings.value("Execute/BLiveChat_OnClose").toBool())
        on_action_ShutBlivechat_triggered();
    if(getSettings.value("Execute/OBS_OnClose").toBool())
        on_action_ShutOBS_triggered();
    qApp->quit();
#endif
}

void MainWindow::showTime()
{
    //Display UTC+8 Time
    QString string;
    QDateTime Timedata = QDateTime::currentDateTime();
    string = Timedata.toString("yyyy-MM-dd hh:mm:ss");
    ui->labelCurrentTime->setText(string);
    //Display Live Time
    if(isOnAir)
    {
        time_passed_QTime = time_passed_QTime.addSecs(1);
        QString time_passed_string = time_passed_QTime.toString("hh:mm:ss");
        ui->labelLiveTime->setText(time_passed_string);
    }
}

void MainWindow::updateBilibiliLiveStatus()
{
    //GET room info
    QNetworkRequest updateRequest;
    QNetworkReply *updateReply = nullptr;
    QString strUpdateReply;
    QNetworkAccessManager updateManager;
    QString updateRequestURL = "https://api.live.bilibili.com/room/v1/Room/get_info?room_id=" + getSettings.value("BiliBili/Bilibili_Live_Room").toString();
    updateRequest.setUrl(QUrl(updateRequestURL));
    updateReply = updateManager.get(updateRequest);
    //Out of time process
    QEventLoop eventloop;
    connect(updateReply, SIGNAL(finished()), &eventloop, SLOT(quit()));
    QTimer::singleShot(2500, &eventloop, &QEventLoop::quit);
    eventloop.exec();
    if(updateReply->isFinished())
    {
        if(updateReply->error() == QNetworkReply::NoError)
        {
            strUpdateReply = updateReply->readAll();
            //Begin Data Process
            //Refer to: https://github.com/SocialSisterYi/bilibili-API-collect/blob/master/docs/live/info.md
            QJsonDocument jsonUpdateReply = QJsonDocument::fromJson(strUpdateReply.toUtf8());
            QJsonObject jsonObjUpdateReply = jsonUpdateReply.object();
            QJsonValue jsonValUpdateReply_isSuccess = jsonObjUpdateReply.value("code");
            if(jsonValUpdateReply_isSuccess.toInt())
                ui->statusbar->showMessage("获取B站直播间信息返回数据失败。");
            else
            {
                //Read "data" object
                QJsonValue jsonValUpdateReply_data = jsonObjUpdateReply.value("data");
                QJsonObject jsonObjUpdateReply_data = jsonValUpdateReply_data.toObject();
                //Update Live Status
                if(jsonObjUpdateReply_data.value("live_status").toInt() == 1 && !isOnAir)
                    liveStatusChange(true);
                if(!(jsonObjUpdateReply_data.value("live_status").toInt() == 1) && isOnAir)
                    liveStatusChange(false);
                //Update Display Info
                if(!ui->lineEditLiveTitle->hasFocus())
                    ui->lineEditLiveTitle->setText(jsonObjUpdateReply_data.value("title").toString());
                if(isOnAir)
                {
                    ui->labelOnline->setText(QString::number(jsonObjUpdateReply_data.value("online").toInt()));
                    ui->labelArea->setText(jsonObjUpdateReply_data.value("area_name").toString());
                    QString strStartLiveTime = jsonObjUpdateReply_data.value("live_time").toString();
                    QDateTime startLiveTime = QDateTime::fromString(strStartLiveTime, "yyyy-MM-dd hh:mm:ss");
                    QDateTime currentTime = QDateTime::currentDateTime();
                    //Calculate time difference
                    uint start_time_timeStamp = startLiveTime.toSecsSinceEpoch();
                    uint current_time_timeStamp = currentTime.toSecsSinceEpoch();
                    uint time_passed_secs = current_time_timeStamp - start_time_timeStamp;
                    time_passed_QTime.setHMS(0,0,0);
                    time_passed_QTime = time_passed_QTime.addSecs(time_passed_secs);
                }
            }
        }
        else
            ui->statusbar->showMessage("未能连接成功B站服务器并取得更新信息。");
    }
    else    //Timeout
    {
        disconnect(updateReply, &QNetworkReply::finished, &eventloop, &QEventLoop::quit);
        updateReply->abort();
        ui->statusbar->showMessage("与B站服务器连接超时。");
    }
}

void MainWindow::getOBSSetup()
{
    if(isOBSWebSocketConnected)
    {
        QString requestUuid;
        QEventLoop eventloop;
        connect(this, SIGNAL(do_OBSWebSocketMessageReceivedSignal()), &eventloop, SLOT(quit()));
        QTimer::singleShot(1000, &eventloop, &QEventLoop::quit);
        //Get "现在在播" Text
        requestUuid = QUuid::createUuid().toString();
        QJsonObject jsonGetLiveNowTextRequest;
        jsonGetLiveNowTextRequest.insert("op", 6);
        QJsonObject jsonGetLiveNowTextRequestData;
        jsonGetLiveNowTextRequestData.insert("requestType", "GetInputSettings");
        jsonGetLiveNowTextRequestData.insert("requestId", requestUuid);
        QJsonObject jsonGetLiveNowTextRequestContentData;
        jsonGetLiveNowTextRequestContentData.insert("inputName", "现在在播");
        jsonGetLiveNowTextRequestData.insert("requestData", jsonGetLiveNowTextRequestContentData);
        jsonGetLiveNowTextRequest.insert("d", jsonGetLiveNowTextRequestData);
        QJsonDocument jsonDocGetLiveNowTextRequest(jsonGetLiveNowTextRequest);
        OBSWebSocket->sendTextMessage(jsonDocGetLiveNowTextRequest.toJson());
        eventloop.exec();
        QJsonObject jsonGetLiveNowTextReply = latestOBSWebSocketMessageJSONDoc.object();
        QJsonObject jsonGetLiveNowTextReplyData = jsonGetLiveNowTextReply.value("d").toObject();
        QJsonObject jsonGetLiveNowTextReplyStatus = jsonGetLiveNowTextReplyData.value("requestStatus").toObject();
        if((jsonGetLiveNowTextReplyData.value("requestId").toString() == requestUuid) && (jsonGetLiveNowTextReplyStatus.value("result").toBool()))
        {
            QJsonObject jsonGetLiveNowTextReplyResponseData = jsonGetLiveNowTextReplyData.value("responseData").toObject();
            QJsonObject jsonGetLiveNowTextReplyInputSettings = jsonGetLiveNowTextReplyResponseData.value("inputSettings").toObject();
            if(!ui->lineEditLiveNowlabel->hasFocus())
                ui->lineEditLiveNowlabel->setText(jsonGetLiveNowTextReplyInputSettings.value("text").toString());
        }
        //Get Scene Info
        requestUuid = QUuid::createUuid().toString();
        QJsonObject jsonGetSceneInfoRequest;
        jsonGetSceneInfoRequest.insert("op", 6);
        QJsonObject jsonGetSceneInfoRequestData;
        jsonGetSceneInfoRequestData.insert("requestType", "GetSceneItemList");
        jsonGetSceneInfoRequestData.insert("requestId", requestUuid);
        QJsonObject jsonGetSceneInfoRequestContentData;
        jsonGetSceneInfoRequestContentData.insert("sceneName", "场景");
        jsonGetSceneInfoRequestData.insert("requestData", jsonGetSceneInfoRequestContentData);
        jsonGetSceneInfoRequest.insert("d", jsonGetSceneInfoRequestData);
        QJsonDocument jsonDocGetSceneInfoRequest(jsonGetSceneInfoRequest);
        OBSWebSocket->sendTextMessage(jsonDocGetSceneInfoRequest.toJson());
        eventloop.exec();
        QJsonObject jsonGetSceneInfoReply = latestOBSWebSocketMessageJSONDoc.object();
        QJsonObject jsonGetSceneInfoReplyData = jsonGetSceneInfoReply.value("d").toObject();
        QJsonObject jsonGetSceneInfoReplyStatus = jsonGetSceneInfoReplyData.value("requestStatus").toObject();
        if((jsonGetSceneInfoReplyData.value("requestId").toString() == requestUuid) && (jsonGetSceneInfoReplyStatus.value("result").toBool()))
        {
            QJsonObject jsonGetSceneInfoResponseData = jsonGetSceneInfoReplyData.value("responseData").toObject();
            QJsonArray jsonArrGetSceneInfoSceneItems = jsonGetSceneInfoResponseData.value("sceneItems").toArray();
            for(int i=0; i<jsonArrGetSceneInfoSceneItems.count(); i++)
            {
                QJsonObject jsonGetSceneInfoSceneItemsSearchNow = jsonArrGetSceneInfoSceneItems.at(i).toObject();
                if(jsonGetSceneInfoSceneItemsSearchNow.value("sourceName").toString() == "点歌")
                {
                    ui->checkBoxMusic->setChecked(jsonGetSceneInfoSceneItemsSearchNow.value("sceneItemEnabled").toBool());
                    musicInfoSceneItemId = jsonGetSceneInfoSceneItemsSearchNow.value("sceneItemId").toInt();
                }
                else if(jsonGetSceneInfoSceneItemsSearchNow.value("sourceName").toString() == "歌词")
                {
                    ui->checkBoxLyrics->setChecked(jsonGetSceneInfoSceneItemsSearchNow.value("sceneItemEnabled").toBool());
                    lyricsSceneItemId = jsonGetSceneInfoSceneItemsSearchNow.value("sceneItemId").toInt();
                }
                else if(jsonGetSceneInfoSceneItemsSearchNow.value("sourceName").toString() == "休息")
                {
                    if(jsonGetSceneInfoSceneItemsSearchNow.value("sceneItemEnabled").toBool())
                    {
                        restNotificationEnabled = true;
                        ui->pushButtonRestNotification->setText("隐藏休息公告");
                    }
                    else
                    {
                        restNotificationEnabled = false;
                        ui->pushButtonRestNotification->setText("显示休息公告");
                    }
                    restNotificationSceneItemId = jsonGetSceneInfoSceneItemsSearchNow.value("sceneItemId").toInt();
                }
                else if(jsonGetSceneInfoSceneItemsSearchNow.value("sourceName").toString() == "视频采集设备")
                    videoCaptureSceneItemId = jsonGetSceneInfoSceneItemsSearchNow.value("sceneItemId").toInt();
            }
        }
        //Get Microphone Info (Mute/Unmute)
        requestUuid = QUuid::createUuid().toString();
        QJsonObject jsonGetMicrophoneMuteRequest;
        jsonGetMicrophoneMuteRequest.insert("op", 6);
        QJsonObject jsonGetMicrophoneMuteRequestData;
        jsonGetMicrophoneMuteRequestData.insert("requestType", "GetInputMute");
        jsonGetMicrophoneMuteRequestData.insert("requestId", requestUuid);
        QJsonObject jsonGetMicrophoneMuteRequestContentData;
        jsonGetMicrophoneMuteRequestContentData.insert("inputName", "麦克风/Aux");
        jsonGetMicrophoneMuteRequestData.insert("requestData", jsonGetMicrophoneMuteRequestContentData);
        jsonGetMicrophoneMuteRequest.insert("d", jsonGetMicrophoneMuteRequestData);
        QJsonDocument jsonDocGetMicrophoneMuteRequest(jsonGetMicrophoneMuteRequest);
        OBSWebSocket->sendTextMessage(jsonDocGetMicrophoneMuteRequest.toJson());
        eventloop.exec();
        QJsonObject jsonGetMicrophoneMuteReply = latestOBSWebSocketMessageJSONDoc.object();
        QJsonObject jsonGetMicrophoneMuteReplyData = jsonGetMicrophoneMuteReply.value("d").toObject();
        QJsonObject jsonGetMicrophoneMuteReplyStatus = jsonGetMicrophoneMuteReplyData.value("requestStatus").toObject();
        if((jsonGetMicrophoneMuteReplyData.value("requestId").toString() == requestUuid) && (jsonGetMicrophoneMuteReplyStatus.value("result").toBool()))
        {
            QJsonObject jsonGetMicrophoneMuteResponseData = jsonGetMicrophoneMuteReplyData.value("responseData").toObject();
            if(jsonGetMicrophoneMuteResponseData.value("inputMuted").toBool())
            {
                isMicrophoneMuted = true;
                ui->pushButtonMute->setText("解除静音");
            }
            else
            {
                isMicrophoneMuted = false;
                ui->pushButtonMute->setText("静音");
            }
        }
        //Get RestNotice Word
        requestUuid = QUuid::createUuid().toString();
        QJsonObject jsonGetRestNoticeWordRequest;
        jsonGetRestNoticeWordRequest.insert("op", 6);
        QJsonObject jsonGetRestNoticeWordRequestData;
        jsonGetRestNoticeWordRequestData.insert("requestType", "GetInputSettings");
        jsonGetRestNoticeWordRequestData.insert("requestId", requestUuid);
        QJsonObject jsonGetRestNoticeWordRequestContentData;
        jsonGetRestNoticeWordRequestContentData.insert("inputName", "休息");
        jsonGetRestNoticeWordRequestData.insert("requestData", jsonGetRestNoticeWordRequestContentData);
        jsonGetRestNoticeWordRequest.insert("d", jsonGetRestNoticeWordRequestData);
        QJsonDocument jsonDocGetRestNoticeWordRequest(jsonGetRestNoticeWordRequest);
        OBSWebSocket->sendTextMessage(jsonDocGetRestNoticeWordRequest.toJson());
        eventloop.exec();
        QJsonObject jsonGetRestNoticeWordReply = latestOBSWebSocketMessageJSONDoc.object();
        QJsonObject jsonGetRestNoticeWordReplyData = jsonGetRestNoticeWordReply.value("d").toObject();
        QJsonObject jsonGetRestNoticeWordReplyStatus = jsonGetRestNoticeWordReplyData.value("requestStatus").toObject();
        if((jsonGetRestNoticeWordReplyData.value("requestId").toString() == requestUuid) && (jsonGetRestNoticeWordReplyStatus.value("result").toBool()))
        {
            QJsonObject jsonGetRestNoticeResponseData = jsonGetRestNoticeWordReplyData.value("responseData").toObject();
            QJsonObject jsonGetRestNoticeResponseDataInputSettings = jsonGetRestNoticeResponseData.value("inputSettings").toObject();
            if(!ui->plainTextEditRestNotification->hasFocus())
                ui->plainTextEditRestNotification->setPlainText(jsonGetRestNoticeResponseDataInputSettings.value("text").toString());
        }
        //Enable UI OBS Settings
        if(!ui->groupBoxLiveInterface->isEnabled())
            ui->groupBoxLiveInterface->setEnabled(true);
    }
}

void MainWindow::liveStatusChange(bool status)
{
    QPalette paletteSet = ui->labelLiveStatus->palette();
    if(status)
    {
        isOnAir = true;
        ui->labelLiveStatus->setText("直播中");
        paletteSet.setColor(QPalette::WindowText, Qt::red);
        ui->labelLiveStatus->setPalette(paletteSet);
        label_LiveStatus->setText("直播已开始");
        ui->action_BilibiliStopLive->setEnabled(true);
        ui->action_OnekeyStopLive->setEnabled(true);
        ui->action_BilibiliStartLive->setEnabled(false);
        ui->action_OnekeyStartLive->setEnabled(false);
    }
    else
    {
        isOnAir = false;
        ui->labelLiveStatus->setText("未直播");
        paletteSet.setColor(QPalette::WindowText, Qt::black);
        ui->labelLiveStatus->setPalette(paletteSet);
        label_LiveStatus->setText("直播未开始");
        ui->action_BilibiliStopLive->setEnabled(false);
        ui->action_OnekeyStopLive->setEnabled(false);
        ui->action_BilibiliStartLive->setEnabled(true);
        ui->action_OnekeyStartLive->setEnabled(true);
    }
}

void MainWindow::liveTitleUpdateRequest(QString title)
{
    QNetworkRequest updateTitleRequest;
    QNetworkReply *updateTitleReply = nullptr;
    QJsonDocument jsonUpdateTitleReply;
    QNetworkAccessManager updateTitleManager;
    updateTitleRequest.setUrl(QUrl("https://api.live.bilibili.com/room/v1/Room/update"));
    //Request Header Set
    updateTitleRequest.setRawHeader("Content-Type", "application/x-www-form-urlencoded;charset=utf-8");
    updateTitleRequest.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/117.0");
    QString updateTitleCookies = "";
    updateTitleCookies += "SESSDATA=" + getSettings.value("BiliBili/Cookie_SESSDATA").toString();
    updateTitleCookies += ";bili_jct=" + getSettings.value("BiliBili/Cookie_csrf").toString();
    updateTitleRequest.setRawHeader("Cookie", updateTitleCookies.toUtf8());
    //Request Content Set
    QString updateTitleData = "";
    updateTitleData += "room_id=" + getSettings.value("BiliBili/Bilibili_Live_Room").toString();
    updateTitleData += "&title=" + title;
    updateTitleData += "&csrf=" + getSettings.value("BiliBili/Cookie_csrf").toString();
    //POST Request
    updateTitleReply = updateTitleManager.post(updateTitleRequest, updateTitleData.toUtf8());
    //Out of time process
    QEventLoop eventloop;
    connect(updateTitleReply, SIGNAL(finished()), &eventloop, SLOT(quit()));
    QTimer::singleShot(10000, &eventloop, &QEventLoop::quit);
    eventloop.exec();
    if(updateTitleReply->isFinished())
    {
        if(updateTitleReply->error() == QNetworkReply::NoError)
        {
            jsonUpdateTitleReply = QJsonDocument::fromJson(updateTitleReply->readAll());
            QJsonObject jsonObjUpdateTitleReply = jsonUpdateTitleReply.object();
            if(jsonObjUpdateTitleReply.value("code").toInt() == 0)
                ui->statusbar->showMessage("标题修改成功。");
            else if(jsonObjUpdateTitleReply.value("code").toInt() == 1)
                ui->statusbar->showMessage("标题修改失败。未知错误。");
            else if(jsonObjUpdateTitleReply.value("code").toInt() == 65530)
                ui->statusbar->showMessage("标题修改失败。认证错误。");
        }
        else
            ui->statusbar->showMessage("读取标题修改结果发生错误。");
    }
}

void MainWindow::managerWebSocketConnect()
{
    if(getSettings.contains("Operate/ManagerWebSocket_URL"))
    {
        QString ManagerWebSocketURL = getSettings.value("Operate/ManagerWebSocket_URL").toString();
        managerWebSocket->open(QUrl(ManagerWebSocketURL));
    }
    else
        QMessageBox::critical(this, "错误", "没有房管端 WebSocket连接URL配置。", QMessageBox::Close);
}

void MainWindow::managerWebSocketConnectStatusUpdate(bool connected)
{
    if(connected)
    {
        isManagerWebSocketConnected = true;
        label_ManagerConnectInfo->setText("房管远程已连接");
        ui->statusbar->showMessage("房管远程已连接。");
    }
    if(!connected)
    {
        isManagerWebSocketConnected = false;
        label_ManagerConnectInfo->setText("房管远程未连接");
        if(!allowManagerHelp)
            label_ManagerConnectInfo->setText("房管远程已阻止");
        ui->statusbar->showMessage("房管远程已断开。");
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Signal Connect
    connect(this, SIGNAL(do_MainWindow_Loaded()), this, SLOT(do_MainWindow_AfterShow()), Qt::QueuedConnection);
    connect(OBSWebSocket, SIGNAL(connected()), this, SLOT(do_OBSWebSocketConnected()));
    connect(OBSWebSocket, SIGNAL(disconnected()), this, SLOT(do_OBSWebSocketDisconnected()));
    connect(OBSWebSocket, SIGNAL(textMessageReceived(QString)), this, SLOT(do_OBSWebSocketMessageReceived(QString)));
    connect(managerWebSocket, SIGNAL(connected()), this, SLOT(do_ManagerWebSocketConnected()));
    connect(managerWebSocket, SIGNAL(disconnected()), this, SLOT(do_ManagerWebSocketDisconnected()));
    connect(managerWebSocket, SIGNAL(textMessageReceived(QString)), this, SLOT(do_ManagerWebSocketMessageReceived(QString)));

    //Generate UI components
    buildUI();

    //Set Current Time Timer
    currentTimeTimer = new QTimer;
    connect(currentTimeTimer, &QTimer::timeout, this, &MainWindow::showTime);
    currentTimeTimer->start(1000);
    
    //Set BilibiliLive Status Updater Timer
    bilibiliLiveStatusUpdateTimer = new QTimer;
    connect(bilibiliLiveStatusUpdateTimer, &QTimer::timeout, this, &MainWindow::updateBilibiliLiveStatus);
    bilibiliLiveStatusUpdateTimer->start(60000);

    //Set OBS Setup Updater Timer
    OBSSetupUpdateTimer = new QTimer;
    connect(OBSSetupUpdateTimer, &QTimer::timeout, this, &MainWindow::getOBSSetup);
    OBSSetupUpdateTimer->start(5000);

    //Loaded Complete
    emit do_MainWindow_Loaded();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::do_ManagerWebSocketConnected()
{
    managerWebSocketConnectStatusUpdate(true);
}

void MainWindow::do_ManagerWebSocketDisconnected()
{
    managerWebSocketConnectStatusUpdate(false);
    if(allowManagerHelp)
        managerWebSocketConnect();
}

void MainWindow::do_ManagerWebSocketMessageReceived(QString msg)
{
    QJsonDocument jsonDocManagerMessage = QJsonDocument::fromJson(msg.toUtf8());
    QJsonObject jsonManagerMessage = jsonDocManagerMessage.object();
    if(jsonManagerMessage.value("message").toInt() == 2)
    {
        QString operation = jsonManagerMessage.value("operate").toString();
        if(operation == "StopLive")
        {
            WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::Text02);
            templ.setTextField(L"房管已中断您的直播。", WinToastLib::WinToastTemplate::FirstLine);
            WinToastLib::WinToast::instance()->showToast(templ, new CustomHandler());
            on_action_OnekeyStopLive_triggered();
        }
        else if(operation == "EditLiveTitle")
        {
            ui->lineEditLiveTitle->setText(jsonManagerMessage.value("data").toString());
            on_pushButtonLiveTitleUpdate_clicked();
            QString receivedMessage = jsonManagerMessage.value("data").toString();
            WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::Text02);
            templ.setTextField(L"房管已修改您的直播间标题", WinToastLib::WinToastTemplate::FirstLine);
            templ.setTextField(receivedMessage.toStdWString().c_str(), WinToastLib::WinToastTemplate::SecondLine);
            WinToastLib::WinToast::instance()->showToast(templ, new CustomHandler());
        }
        else if(operation == "OnDanmuji")
        {
            on_action_OpenDanmuji_triggered();
        }
        else if(operation == "OffDanmuji")
        {
            on_action_ShutDanmuji_triggered();
        }
        else if(operation == "OnOBS")
        {
            on_action_OpenOBS_triggered();
        }
        else if(operation == "OffOBS")
        {
            on_action_ShutOBS_triggered();
        }
        else if(operation == "Onblivechat")
        {
            on_action_OpenBlivechat_triggered();
        }
        else if(operation == "Offblivechat")
        {
            on_action_ShutBlivechat_triggered();
        }
        else if(operation == "EditLiveNow")
        {
            ui->lineEditLiveNowlabel->setText(jsonManagerMessage.value("data").toString());
            on_pushButtonLiveNowEditConfirm_clicked();
            QString receivedMessage = jsonManagerMessage.value("data").toString();
            WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::Text02);
            templ.setTextField(L"房管已修改您的“现在在播”标签", WinToastLib::WinToastTemplate::FirstLine);
            templ.setTextField(receivedMessage.toStdWString().c_str(), WinToastLib::WinToastTemplate::SecondLine);
            WinToastLib::WinToast::instance()->showToast(templ, new CustomHandler());
        }
        else if(operation == "TriggerMusic")
        {
            if(!ui->checkBoxMusic->isChecked())
                on_checkBoxMusic_clicked(true);
            else
                on_checkBoxMusic_clicked(false);
        }
        else if(operation == "TriggerLyrics")
        {
            if(!ui->checkBoxLyrics->isChecked())
                on_checkBoxLyrics_clicked(true);
            else
                on_checkBoxLyrics_clicked(false);
        }
        else if(operation == "TriggerRestInfo")
        {
            on_pushButtonRestNotification_clicked();
            WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::Text02);
            templ.setTextField(L"房管已设置显示/关闭休息公告", WinToastLib::WinToastTemplate::FirstLine);
            WinToastLib::WinToast::instance()->showToast(templ, new CustomHandler());
        }
        else if(operation == "EditRestInfo")
        {
            ui->plainTextEditRestNotification->setPlainText(jsonManagerMessage.value("data").toString());
            on_pushButtonRestNotificationModified_clicked();
            QString receivedMessage = jsonManagerMessage.value("data").toString();
            WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::Text02);
            templ.setTextField(L"房管已修改您的休息公告", WinToastLib::WinToastTemplate::FirstLine);
            templ.setTextField(receivedMessage.toStdWString().c_str(), WinToastLib::WinToastTemplate::SecondLine);
            WinToastLib::WinToast::instance()->showToast(templ, new CustomHandler());
        }
        else if(operation == "Mute")
        {
            QString requestUuid = QUuid::createUuid().toString();
            QJsonObject jsonToggleMicrophoneMuteRequest;
            jsonToggleMicrophoneMuteRequest.insert("op", 6);
            QJsonObject jsonToggleMicrophoneMuteRequestData;
            jsonToggleMicrophoneMuteRequestData.insert("requestType", "SetInputMute");
            jsonToggleMicrophoneMuteRequestData.insert("requestId", requestUuid);
            QJsonObject jsonToggleMicrophoneMuteContentData;
            jsonToggleMicrophoneMuteContentData.insert("inputName", "麦克风/Aux");
            jsonToggleMicrophoneMuteContentData.insert("inputMuted", true);
            jsonToggleMicrophoneMuteRequestData.insert("requestData", jsonToggleMicrophoneMuteContentData);
            jsonToggleMicrophoneMuteRequest.insert("d", jsonToggleMicrophoneMuteRequestData);
            QJsonDocument jsonDocToggleMicrophoneMuteRequest(jsonToggleMicrophoneMuteRequest);
            OBSWebSocket->sendTextMessage(jsonDocToggleMicrophoneMuteRequest.toJson());
            QEventLoop eventloop;
            connect(this, SIGNAL(do_OBSWebSocketMessageReceivedSignal()), &eventloop, SLOT(quit()));
            QTimer::singleShot(1000, &eventloop, &QEventLoop::quit);
            eventloop.exec();
            getOBSSetup();
            WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::Text02);
            templ.setTextField(L"房管已设置您静音", WinToastLib::WinToastTemplate::FirstLine);
            WinToastLib::WinToast::instance()->showToast(templ, new CustomHandler());
        }
        else if(operation == "Urgent")
        {
            on_pushButtonShut_clicked();
            WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::Text02);
            templ.setTextField(L"房管已设置紧急静默", WinToastLib::WinToastTemplate::FirstLine);
            WinToastLib::WinToast::instance()->showToast(templ, new CustomHandler());
        }
        else if(operation == "SendMessage")
        {
            QString receivedMessage = jsonManagerMessage.value("data").toString();
            WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::Text02);
            templ.setTextField(L"来自房管的消息", WinToastLib::WinToastTemplate::FirstLine);
            templ.setTextField(receivedMessage.toStdWString().c_str(), WinToastLib::WinToastTemplate::SecondLine);
            WinToastLib::WinToast::instance()->showToast(templ, new CustomHandler());
        }
        else if(operation == "ExitProcess")
        {
            qApp->quit();
        }
        else if(operation == "AskPowerOff")
        {
            //Try to shutdown
            willTurnOff = true;
            QEventLoop eventloop;
            //Construct Notification
            class responseHandler : public WinToastLib::IWinToastHandler {
            public:
                void toastActivated() const {
                }

                void toastActivated(int actionIndex) const {
                    willTurnOff = false;
                }

                void toastFailed() const {
                }

                void toastDismissed(WinToastDismissalReason state) const {
                }
            };

            WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::Text04);
            templ.setTextField(L"远程房管发送了一个关机请求", WinToastLib::WinToastTemplate::FirstLine);
            templ.setTextField(L"如果您不做任何操作，系统将于5分钟后为您下播并关机。", WinToastLib::WinToastTemplate::SecondLine);
            templ.setTextField(L"若要阻止关机，请点击下方的按钮。", WinToastLib::WinToastTemplate::ThirdLine);
            templ.addAction(L"停止关机");
            WinToastLib::WinToast::instance()->showToast(templ, new responseHandler());
            QTimer::singleShot(300000, Qt::VeryCoarseTimer, this, &MainWindow::do_shutDown);
        }
    }
}

void MainWindow::setLiveBegin()
{
    QNetworkRequest startLiveRequest;
    QNetworkReply *startLiveReply = nullptr;
    QJsonDocument jsonStartLiveReply;
    QNetworkAccessManager startLiveManager;
    startLiveRequest.setUrl(QUrl("https://api.live.bilibili.com/room/v1/Room/startLive"));
    //Request Header Set
    startLiveRequest.setRawHeader("Content-Type", "application/x-www-form-urlencoded;charset=utf-8");
    startLiveRequest.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/117.0");
    QString startLiveCookies = "";
    startLiveCookies += "SESSDATA=" + getSettings.value("BiliBili/Cookie_SESSDATA").toString();
    startLiveCookies += ";bili_jct=" + getSettings.value("BiliBili/Cookie_csrf").toString();
    startLiveRequest.setRawHeader("Cookie", startLiveCookies.toUtf8());
    QString startLiveData = "";
    startLiveData += "room_id=" + getSettings.value("BiliBili/Bilibili_Live_Room").toString();
    startLiveData += "&area_v2=" + getSettings.value("BiliBili/area_v2").toString();
    startLiveData += "&platform=pc";
    startLiveData += "&csrf=" + getSettings.value("BiliBili/Cookie_csrf").toString();
    //POST Request
    startLiveReply = startLiveManager.post(startLiveRequest, startLiveData.toUtf8());
    //Out of time process
    QEventLoop eventloop;
    connect(startLiveReply, SIGNAL(finished()), &eventloop, SLOT(quit()));
    QTimer::singleShot(10000, &eventloop, &QEventLoop::quit);
    eventloop.exec();
    if(startLiveReply->isFinished())
    {
        if(startLiveReply->error() == QNetworkReply::NoError)
        {
            jsonStartLiveReply = QJsonDocument::fromJson(startLiveReply->readAll());
            QJsonObject jsonObjStartLiveReply = jsonStartLiveReply.object();
            if(jsonObjStartLiveReply.value("code").toInt() == 0)
            {
                QJsonObject jsonObjStartLiveReply_data = jsonObjStartLiveReply.value("data").toObject();
                QJsonObject jsonObjStartLiveReply_RTMP = jsonObjStartLiveReply_data.value("rtmp").toObject();
                liveRTMPAddress = jsonObjStartLiveReply_RTMP.value("addr").toString();
                liveRTMPCode = jsonObjStartLiveReply_RTMP.value("code").toString();
            }
            else if(jsonObjStartLiveReply.value("code").toInt() == 65530)
                QMessageBox::critical(this, "错误", "设置开播失败。请更新登录信息。", QMessageBox::Close);
            else if(jsonObjStartLiveReply.value("code").toInt() == 60009)
                QMessageBox::critical(this, "错误", "设置开播失败。请更新分区信息。", QMessageBox::Close);
            else if(jsonObjStartLiveReply.value("code").toInt() == -101)
                QMessageBox::critical(this, "错误", "设置开播失败。登录信息失效或无效。", QMessageBox::Close);
            else
                QMessageBox::critical(this, "错误", "设置开播失败。", QMessageBox::Close);
        }
        else
            QMessageBox::critical(this, "错误", "设置开播失败。连接错误。", QMessageBox::Close);
    }
    updateBilibiliLiveStatus();
}

void MainWindow::OBSWebSocketConnect()
{
    if(getSettings.contains("Operate/OBSWebSocket_URL"))
    {
        QString OBSWebSocketURL = getSettings.value("Operate/OBSWebSocket_URL").toString();
        OBSWebSocket->open(QUrl(OBSWebSocketURL));
    }
    else
        QMessageBox::critical(this, "错误", "没有OBS WebSocket连接URL配置。", QMessageBox::Close);
}

void MainWindow::OBSWebSocketConnectStatusUpdate(bool connected)
{
    if(connected)
    {
        isOBSWebSocketConnected = true;
        label_OBSConnectInfo->setText("OBS已连接");
        ui->statusbar->showMessage("OBS已连接。");
        //ui->groupBoxLiveInterface->setEnabled(true);  //this work should be done by the first getOBSSetup().
    }
    if(!connected)
    {
        isOBSWebSocketConnected = false;
        label_OBSConnectInfo->setText("OBS未连接");
        ui->statusbar->showMessage("OBS连接已断开。");
        ui->groupBoxLiveInterface->setEnabled(false);
    }
}

void MainWindow::do_OBSWebSocketConnected()
{
    OBSWebSocket->sendTextMessage("{ \"op\": 1, \"d\": { \"rpcVersion\": 1} }");
    OBSWebSocketConnectStatusUpdate(true);
}

void MainWindow::do_OBSWebSocketDisconnected()
{
    OBSWebSocketConnectStatusUpdate(false);
    OBSWebSocketConnect();
}

void MainWindow::do_OBSWebSocketMessageReceived(QString msg)
{
    latestOBSWebSocketMessageStr = msg;
    latestOBSWebSocketMessageJSONDoc = QJsonDocument::fromJson(latestOBSWebSocketMessageStr.toUtf8());
    //qDebug()<<latestOBSWebSocketMessageJSONDoc;
    emit do_OBSWebSocketMessageReceivedSignal();
}

void MainWindow::do_shutDown()
{
    if(willTurnOff)
    {
        on_action_OnekeyStopLive_triggered();
        QStringList shutCommand;
        shutCommand.append("/s");
        QProcess::execute("shutdown", shutCommand);
    }
}

void MainWindow::do_MainWindow_AfterShow()
{
    //Connect to OBS
    ui->statusbar->showMessage("正在连接OBS...");
    OBSWebSocketConnect();

    //Connect to Manager
    ui->statusbar->showMessage("正在连接房管服务器...");
    managerWebSocketConnect();
    updateBilibiliLiveStatus();

    //Complete
    //ui->statusbar->showMessage("就绪。");
}

void MainWindow::on_pushButtonOpenBilibiliLiveSettings_clicked()
{
    QString bilibiliLiveSettingsURL = "https://link.bilibili.com/p/center/index/my-room/start-live#/my-room/start-live";
    if(!QDesktopServices::openUrl(QUrl(bilibiliLiveSettingsURL)))
        QMessageBox::critical(this, "错误", "打开Bilibili直播设置页未成功。", QMessageBox::Close);
}


void MainWindow::on_pushButtonLiveTitleUpdate_clicked()
{
    liveTitleUpdateRequest(ui->lineEditLiveTitle->text());
}


void MainWindow::on_pushButton_clicked()
{
    QString liveRoomUrl = "https://live.bilibili.com/" + getSettings.value("BiliBili/Bilibili_Live_Room").toString();
    if(!QDesktopServices::openUrl(QUrl(liveRoomUrl)))
        QMessageBox::critical(this, "错误", "打开直播间未成功。", QMessageBox::Close);
}


void MainWindow::on_action_BilibiliStartLive_triggered()
{
    setLiveBegin();
    DialogOBSRTMPSettingsInfo *OBSRTMPSettingsInfo = new DialogOBSRTMPSettingsInfo();
    OBSRTMPSettingsInfo->open();
}


void MainWindow::on_pushButtonTest_clicked()
{
    WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::Text02);
    templ.setTextField(L"title", WinToastLib::WinToastTemplate::FirstLine);
    templ.setTextField(L"subtitle", WinToastLib::WinToastTemplate::SecondLine);
    templ.setAudioOption(WinToastLib::WinToastTemplate::Silent);
    WinToastLib::WinToast::instance()->showToast(templ, new CustomHandler());
}


void MainWindow::on_action_BilibiliStopLive_triggered()
{
    QNetworkRequest stopLiveRequest;
    QNetworkReply *stopLiveReply = nullptr;
    QJsonDocument jsonStopLiveReply;
    QNetworkAccessManager stopLiveManager;
    stopLiveRequest.setUrl(QUrl("https://api.live.bilibili.com/room/v1/Room/stopLive"));
    //Request Header Set
    stopLiveRequest.setRawHeader("Content-Type", "application/x-www-form-urlencoded;charset=utf-8");
    stopLiveRequest.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/117.0");
    QString stopLiveCookies = "";
    stopLiveCookies += "SESSDATA=" + getSettings.value("BiliBili/Cookie_SESSDATA").toString();
    stopLiveCookies += ";bili_jct=" + getSettings.value("BiliBili/Cookie_csrf").toString();
    stopLiveRequest.setRawHeader("Cookie", stopLiveCookies.toUtf8());
    QString stopLiveData = "";
    stopLiveData += "room_id=" + getSettings.value("BiliBili/Bilibili_Live_Room").toString();
    stopLiveData += "&csrf=" + getSettings.value("BiliBili/Cookie_csrf").toString();
    //POST Request
    stopLiveReply = stopLiveManager.post(stopLiveRequest, stopLiveData.toUtf8());
    //Out of time process
    QEventLoop eventloop;
    connect(stopLiveReply, SIGNAL(finished()), &eventloop, SLOT(quit()));
    QTimer::singleShot(10000, &eventloop, &QEventLoop::quit);
    eventloop.exec();
    if(stopLiveReply->isFinished())
    {
        if(stopLiveReply->error() == QNetworkReply::NoError)
        {
            jsonStopLiveReply = QJsonDocument::fromJson(stopLiveReply->readAll());
            QJsonObject jsonObjStopLiveReply = jsonStopLiveReply.object();
            if(jsonObjStopLiveReply.value("code").toInt() == 0)
                ui->statusbar->showMessage("停止直播指令已发送。");
            else if(jsonObjStopLiveReply.value("code").toInt() == 65530)
                QMessageBox::critical(this, "错误", "设置关播失败。请更新登录信息。", QMessageBox::Close);
        }
        else
            QMessageBox::critical(this, "错误", "设置关播失败。连接错误。", QMessageBox::Close);
    }
    updateBilibiliLiveStatus();
}


void MainWindow::on_action_OBSStartLive_triggered()
{
    if(isOBSWebSocketConnected)
    {
        QString requestUuid = QUuid::createUuid().toString();
        QJsonObject jsonOBSStartLiveRequest;
        jsonOBSStartLiveRequest.insert("op", 6);
        QJsonObject jsonOBSStartLiveRequestData;
        jsonOBSStartLiveRequestData.insert("requestType", "StartStream");
        jsonOBSStartLiveRequestData.insert("requestId", requestUuid);
        jsonOBSStartLiveRequest.insert("d", jsonOBSStartLiveRequestData);
        QJsonDocument jsonDocOBSStartLiveRequest(jsonOBSStartLiveRequest);
        OBSWebSocket->sendTextMessage(jsonDocOBSStartLiveRequest.toJson());
    }
    else
        QMessageBox::critical(this, "错误", "OBS未连接，无法设置。", QMessageBox::Close);
}


void MainWindow::on_action_OBSStopLive_triggered()
{
    if(isOBSWebSocketConnected)
    {
        QString requestUuid = QUuid::createUuid().toString();
        QJsonObject jsonOBSStopLiveRequest;
        jsonOBSStopLiveRequest.insert("op", 6);
        QJsonObject jsonOBSStopLiveRequestData;
        jsonOBSStopLiveRequestData.insert("requestType", "StopStream");
        jsonOBSStopLiveRequestData.insert("requestId", requestUuid);
        jsonOBSStopLiveRequest.insert("d", jsonOBSStopLiveRequestData);
        QJsonDocument jsonDocOBSStopLiveRequest(jsonOBSStopLiveRequest);
        OBSWebSocket->sendTextMessage(jsonDocOBSStopLiveRequest.toJson());
    }
    else
        QMessageBox::critical(this, "错误", "OBS未连接，无法设置。", QMessageBox::Close);
}


void MainWindow::on_action_OnekeyStartLive_triggered()
{
    setLiveBegin();
    setOBSRTMPSettings();
    waitSec(5);
    on_action_OBSStartLive_triggered();
}


void MainWindow::on_action_OnekeyStopLive_triggered()
{
    on_action_OBSStopLive_triggered();
    on_action_BilibiliStopLive_triggered();
}


void MainWindow::on_action_About_triggered()
{
    DialogAbout *aboutWindow = new DialogAbout();
    aboutWindow->show();
}


void MainWindow::on_action_OpenDanmuji_triggered()
{
    if(getSettings.contains("Execute/Danmuji_URL"))
    {
        QString DanmujiUrl = getSettings.value("Execute/Danmuji_URL").toString();
        if(!QDesktopServices::openUrl(QUrl(DanmujiUrl)))
            QMessageBox::critical(this, "错误", "启动弹幕姬未成功，将跳过。", QMessageBox::Close);
    }
    else
        QMessageBox::critical(this, "错误", "没有找到弹幕姬位置。将不会自动启动弹幕姬。", QMessageBox::Close);
}


void MainWindow::on_action_OpenBlivechat_triggered()
{
    if(getSettings.contains("Execute/BLiveChat_Path"))
    {
        QString BLiveChatPath = getSettings.value("Execute/BLiveChat_Path").toString();
        QString BLiveChatExe = BLiveChatPath + "/blivechat.exe";
        QProcess BLiveChatProcess;
        BLiveChatProcess.setProgram(BLiveChatExe);
        BLiveChatProcess.setWorkingDirectory(BLiveChatPath);
        BLiveChatProcess.startDetached();
    }
    else
        QMessageBox::critical(this, "错误", "没有找到BLiveChat位置。将不会自动启动BLiveChat。", QMessageBox::Close);
}


void MainWindow::on_action_OpenOBS_triggered()
{
    if(getSettings.contains("Execute/OBS_URL"))
    {
        QString OBSUrl = getSettings.value("Execute/OBS_URL").toString();
        if(!QDesktopServices::openUrl(QUrl(OBSUrl)))
            QMessageBox::critical(this, "错误", "启动OBS未成功，将跳过。", QMessageBox::Close);
    }
    else
        QMessageBox::critical(this, "错误", "没有找到OBS位置。将不会自动启动OBS。", QMessageBox::Close);
}


void MainWindow::on_action_ShutDanmuji_triggered()
{
    QStringList shutCommand;
    shutCommand.append("/im");
    shutCommand.append("Bililive_dm.exe");
    QProcess::execute("taskkill", shutCommand);
}


void MainWindow::on_action_ShutBlivechat_triggered()
{
    QStringList shutCommand;
    shutCommand.append("/im");
    shutCommand.append("blivechat.exe");
    shutCommand.append("/f");
    QProcess::execute("taskkill", shutCommand);
}


void MainWindow::on_action_ShutOBS_triggered()
{
    QStringList shutCommand;
    shutCommand.append("/im");
    shutCommand.append("obs64.exe");
    QProcess::execute("taskkill", shutCommand);
}


void MainWindow::on_action_SetBlivechat_triggered()
{
    QString bLiveChatUrl = getSettings.value("Operate/BLiveChat_URL").toString();
    QDesktopServices::openUrl(QUrl(bLiveChatUrl));
}


void MainWindow::on_pushButtonLiveNowEditConfirm_clicked()
{
    QString requestUuid = QUuid::createUuid().toString();
    QJsonObject jsonEditLiveNowRequest;
    jsonEditLiveNowRequest.insert("op", 6);
    QJsonObject jsonEditLiveNowRequestData;
    jsonEditLiveNowRequestData.insert("requestType", "SetInputSettings");
    jsonEditLiveNowRequestData.insert("requestId", requestUuid);
    QJsonObject jsonEditLiveNowContentData;
    jsonEditLiveNowContentData.insert("inputName", "现在在播");
    QJsonObject jsonEditLiveNowInputSettings;
    jsonEditLiveNowInputSettings.insert("text", ui->lineEditLiveNowlabel->text());
    jsonEditLiveNowContentData.insert("inputSettings", jsonEditLiveNowInputSettings);
    jsonEditLiveNowRequestData.insert("requestData", jsonEditLiveNowContentData);
    jsonEditLiveNowRequest.insert("d", jsonEditLiveNowRequestData);
    QJsonDocument jsonDocEditLiveNowRequest(jsonEditLiveNowRequest);
    OBSWebSocket->sendTextMessage(jsonDocEditLiveNowRequest.toJson());
    QEventLoop eventloop;
    connect(this, SIGNAL(do_OBSWebSocketMessageReceivedSignal()), &eventloop, SLOT(quit()));
    QTimer::singleShot(1000, &eventloop, &QEventLoop::quit);
    eventloop.exec();
    getOBSSetup();
    //Write Into History
    ui->listWidgetLiveNowHistory->insertItem(0, ui->lineEditLiveNowlabel->text());
}

void MainWindow::on_checkBoxMusic_clicked(bool checked)
{
    QString requestUuid = QUuid::createUuid().toString();
    QJsonObject jsonToggleMusicInfoShowRequest;
    jsonToggleMusicInfoShowRequest.insert("op", 6);
    QJsonObject jsonToggleMusicInfoShowRequestData;
    jsonToggleMusicInfoShowRequestData.insert("requestType", "SetSceneItemEnabled");
    jsonToggleMusicInfoShowRequestData.insert("requestId", requestUuid);
    QJsonObject jsonToggleMusicInfoShowContentData;
    jsonToggleMusicInfoShowContentData.insert("sceneName", "场景");
    jsonToggleMusicInfoShowContentData.insert("sceneItemId", musicInfoSceneItemId);
    jsonToggleMusicInfoShowContentData.insert("sceneItemEnabled", checked);
    jsonToggleMusicInfoShowRequestData.insert("requestData", jsonToggleMusicInfoShowContentData);
    jsonToggleMusicInfoShowRequest.insert("d", jsonToggleMusicInfoShowRequestData);
    QJsonDocument jsonDocToggleMusicInfoShowRequest(jsonToggleMusicInfoShowRequest);
    OBSWebSocket->sendTextMessage(jsonDocToggleMusicInfoShowRequest.toJson());
    QEventLoop eventloop;
    connect(this, SIGNAL(do_OBSWebSocketMessageReceivedSignal()), &eventloop, SLOT(quit()));
    QTimer::singleShot(1000, &eventloop, &QEventLoop::quit);
    eventloop.exec();
    getOBSSetup();
}


void MainWindow::on_checkBoxLyrics_clicked(bool checked)
{
    QString requestUuid = QUuid::createUuid().toString();
    QJsonObject jsonToggleLyricsShowRequest;
    jsonToggleLyricsShowRequest.insert("op", 6);
    QJsonObject jsonToggleLyricsShowRequestData;
    jsonToggleLyricsShowRequestData.insert("requestType", "SetSceneItemEnabled");
    jsonToggleLyricsShowRequestData.insert("requestId", requestUuid);
    QJsonObject jsonToggleLyricsShowContentData;
    jsonToggleLyricsShowContentData.insert("sceneName", "场景");
    jsonToggleLyricsShowContentData.insert("sceneItemId", lyricsSceneItemId);
    jsonToggleLyricsShowContentData.insert("sceneItemEnabled", checked);
    jsonToggleLyricsShowRequestData.insert("requestData", jsonToggleLyricsShowContentData);
    jsonToggleLyricsShowRequest.insert("d", jsonToggleLyricsShowRequestData);
    QJsonDocument jsonDocToggleLyricsShowRequest(jsonToggleLyricsShowRequest);
    OBSWebSocket->sendTextMessage(jsonDocToggleLyricsShowRequest.toJson());
    QEventLoop eventloop;
    connect(this, SIGNAL(do_OBSWebSocketMessageReceivedSignal()), &eventloop, SLOT(quit()));
    QTimer::singleShot(1000, &eventloop, &QEventLoop::quit);
    eventloop.exec();
    getOBSSetup();
}


void MainWindow::on_pushButtonRestNotification_clicked()
{
    QString requestUuid = QUuid::createUuid().toString();
    QJsonObject jsonToggleRestNotificationShowRequest;
    jsonToggleRestNotificationShowRequest.insert("op", 6);
    QJsonObject jsonToggleRestNotificationShowRequestData;
    jsonToggleRestNotificationShowRequestData.insert("requestType", "SetSceneItemEnabled");
    jsonToggleRestNotificationShowRequestData.insert("requestId", requestUuid);
    QJsonObject jsonToggleRestNotificationShowContentData;
    jsonToggleRestNotificationShowContentData.insert("sceneName", "场景");
    jsonToggleRestNotificationShowContentData.insert("sceneItemId", restNotificationSceneItemId);
    jsonToggleRestNotificationShowContentData.insert("sceneItemEnabled", !restNotificationEnabled);
    jsonToggleRestNotificationShowRequestData.insert("requestData", jsonToggleRestNotificationShowContentData);
    jsonToggleRestNotificationShowRequest.insert("d", jsonToggleRestNotificationShowRequestData);
    QJsonDocument jsonDocToggleRestNotificationShowRequest(jsonToggleRestNotificationShowRequest);
    OBSWebSocket->sendTextMessage(jsonDocToggleRestNotificationShowRequest.toJson());
    QEventLoop eventloop;
    connect(this, SIGNAL(do_OBSWebSocketMessageReceivedSignal()), &eventloop, SLOT(quit()));
    QTimer::singleShot(1000, &eventloop, &QEventLoop::quit);
    eventloop.exec();
    getOBSSetup();
}


void MainWindow::on_pushButtonRestNotificationModified_clicked()
{
    QString requestUuid = QUuid::createUuid().toString();
    QJsonObject jsonModifyRestNotificationRequest;
    jsonModifyRestNotificationRequest.insert("op", 6);
    QJsonObject jsonModifyRestNotificationRequestData;
    jsonModifyRestNotificationRequestData.insert("requestType", "SetInputSettings");
    jsonModifyRestNotificationRequestData.insert("requestId", requestUuid);
    QJsonObject jsonModifyRestNotificationContentData;
    jsonModifyRestNotificationContentData.insert("inputName", "休息");
    QJsonObject jsonModifyRestNotificationInputSettings;
    jsonModifyRestNotificationInputSettings.insert("text", ui->plainTextEditRestNotification->toPlainText());
    jsonModifyRestNotificationContentData.insert("inputSettings", jsonModifyRestNotificationInputSettings);
    jsonModifyRestNotificationRequestData.insert("requestData", jsonModifyRestNotificationContentData);
    jsonModifyRestNotificationRequest.insert("d", jsonModifyRestNotificationRequestData);
    QJsonDocument jsonDocModifyRestNotificationRequest(jsonModifyRestNotificationRequest);
    OBSWebSocket->sendTextMessage(jsonDocModifyRestNotificationRequest.toJson());
    QEventLoop eventloop;
    connect(this, SIGNAL(do_OBSWebSocketMessageReceivedSignal()), &eventloop, SLOT(quit()));
    QTimer::singleShot(1000, &eventloop, &QEventLoop::quit);
    eventloop.exec();
    getOBSSetup();
}


void MainWindow::on_pushButtonMute_clicked()
{
    QString requestUuid = QUuid::createUuid().toString();
    QJsonObject jsonToggleMicrophoneMuteRequest;
    jsonToggleMicrophoneMuteRequest.insert("op", 6);
    QJsonObject jsonToggleMicrophoneMuteRequestData;
    jsonToggleMicrophoneMuteRequestData.insert("requestType", "SetInputMute");
    jsonToggleMicrophoneMuteRequestData.insert("requestId", requestUuid);
    QJsonObject jsonToggleMicrophoneMuteContentData;
    jsonToggleMicrophoneMuteContentData.insert("inputName", "麦克风/Aux");
    jsonToggleMicrophoneMuteContentData.insert("inputMuted", !isMicrophoneMuted);
    jsonToggleMicrophoneMuteRequestData.insert("requestData", jsonToggleMicrophoneMuteContentData);
    jsonToggleMicrophoneMuteRequest.insert("d", jsonToggleMicrophoneMuteRequestData);
    QJsonDocument jsonDocToggleMicrophoneMuteRequest(jsonToggleMicrophoneMuteRequest);
    OBSWebSocket->sendTextMessage(jsonDocToggleMicrophoneMuteRequest.toJson());
    QEventLoop eventloop;
    connect(this, SIGNAL(do_OBSWebSocketMessageReceivedSignal()), &eventloop, SLOT(quit()));
    QTimer::singleShot(1000, &eventloop, &QEventLoop::quit);
    eventloop.exec();
    getOBSSetup();
}


void MainWindow::on_pushButtonShut_clicked()
{
    QString requestUuid = QUuid::createUuid().toString();
    QJsonObject jsonSetMicrophoneMuteRequest;
    jsonSetMicrophoneMuteRequest.insert("op", 6);
    QJsonObject jsonSetMicrophoneMuteRequestData;
    jsonSetMicrophoneMuteRequestData.insert("requestType", "SetInputMute");
    jsonSetMicrophoneMuteRequestData.insert("requestId", requestUuid);
    QJsonObject jsonSetMicrophoneMuteContentData;
    jsonSetMicrophoneMuteContentData.insert("inputName", "麦克风/Aux");
    jsonSetMicrophoneMuteContentData.insert("inputMuted", true);
    jsonSetMicrophoneMuteRequestData.insert("requestData", jsonSetMicrophoneMuteContentData);
    jsonSetMicrophoneMuteRequest.insert("d", jsonSetMicrophoneMuteRequestData);
    QJsonDocument jsonDocSetMicrophoneMuteRequest(jsonSetMicrophoneMuteRequest);
    OBSWebSocket->sendTextMessage(jsonDocSetMicrophoneMuteRequest.toJson());
    requestUuid = QUuid::createUuid().toString();
    QJsonObject jsonCaptureAudioMuteRequest;
    jsonCaptureAudioMuteRequest.insert("op", 6);
    QJsonObject jsonCaptureAudioMuteRequestData;
    jsonCaptureAudioMuteRequestData.insert("requestType", "SetInputMute");
    jsonCaptureAudioMuteRequestData.insert("requestId", requestUuid);
    QJsonObject jsonCaptureAudioMuteContentData;
    jsonCaptureAudioMuteContentData.insert("inputName", "音频输入采集");
    jsonCaptureAudioMuteContentData.insert("inputMuted", true);
    jsonCaptureAudioMuteRequestData.insert("requestData", jsonCaptureAudioMuteContentData);
    jsonCaptureAudioMuteRequest.insert("d", jsonCaptureAudioMuteRequestData);
    QJsonDocument jsonDocCaptureAudioMuteRequest(jsonCaptureAudioMuteRequest);
    OBSWebSocket->sendTextMessage(jsonDocCaptureAudioMuteRequest.toJson());
    requestUuid = QUuid::createUuid().toString();
    QJsonObject jsonSetDesktopAudioMuteRequest;
    jsonSetDesktopAudioMuteRequest.insert("op", 6);
    QJsonObject jsonSetDesktopAudioMuteRequestData;
    jsonSetDesktopAudioMuteRequestData.insert("requestType", "SetInputMute");
    jsonSetDesktopAudioMuteRequestData.insert("requestId", requestUuid);
    QJsonObject jsonSetDesktopAudioMuteContentData;
    jsonSetDesktopAudioMuteContentData.insert("inputName", "桌面音频");
    jsonSetDesktopAudioMuteContentData.insert("inputMuted", true);
    jsonSetDesktopAudioMuteRequestData.insert("requestData", jsonSetDesktopAudioMuteContentData);
    jsonSetDesktopAudioMuteRequest.insert("d", jsonSetDesktopAudioMuteRequestData);
    QJsonDocument jsonDocSetDesktopAudioMuteRequest(jsonSetDesktopAudioMuteRequest);
    OBSWebSocket->sendTextMessage(jsonDocSetDesktopAudioMuteRequest.toJson());
    requestUuid = QUuid::createUuid().toString();
    QJsonObject jsonShutVideoCaptureRequest;
    jsonShutVideoCaptureRequest.insert("op", 6);
    QJsonObject jsonShutVideoCaptureRequestData;
    jsonShutVideoCaptureRequestData.insert("requestType", "SetSceneItemEnabled");
    jsonShutVideoCaptureRequestData.insert("requestId", requestUuid);
    QJsonObject jsonShutVideoCaptureContentData;
    jsonShutVideoCaptureContentData.insert("sceneName", "场景");
    jsonShutVideoCaptureContentData.insert("sceneItemId", videoCaptureSceneItemId);
    jsonShutVideoCaptureContentData.insert("sceneItemEnabled", false);
    jsonShutVideoCaptureRequestData.insert("requestData", jsonShutVideoCaptureContentData);
    jsonShutVideoCaptureRequest.insert("d", jsonShutVideoCaptureRequestData);
    QJsonDocument jsonDocShutVideoCaptureRequest(jsonShutVideoCaptureRequest);
    OBSWebSocket->sendTextMessage(jsonDocShutVideoCaptureRequest.toJson());
}


void MainWindow::on_listWidgetLiveNowHistory_itemClicked(QListWidgetItem *item)
{
    ui->lineEditLiveNowlabel->setText(item->text());
    on_pushButtonLiveNowEditConfirm_clicked();
}


void MainWindow::on_pushButtonLiveNowFastSetting_1_clicked()
{
    ui->lineEditLiveNowlabel->setText(getSettings.value("Live/FastSetting_1_Text").toString());
    on_pushButtonLiveNowEditConfirm_clicked();
}


void MainWindow::on_pushButtonLiveNowFastSetting_2_clicked()
{
    ui->lineEditLiveNowlabel->setText(getSettings.value("Live/FastSetting_2_Text").toString());
    on_pushButtonLiveNowEditConfirm_clicked();
}


void MainWindow::on_pushButtonLiveNowFastSetting_3_clicked()
{
    ui->lineEditLiveNowlabel->setText(getSettings.value("Live/FastSetting_3_Text").toString());
    on_pushButtonLiveNowEditConfirm_clicked();
}


void MainWindow::on_pushButtonLiveNowFastSetting_4_clicked()
{
    ui->lineEditLiveNowlabel->setText(getSettings.value("Live/FastSetting_4_Text").toString());
    on_pushButtonLiveNowEditConfirm_clicked();
}


void MainWindow::on_pushButtonLiveNowFastSetting_5_clicked()
{
    ui->lineEditLiveNowlabel->setText(getSettings.value("Live/FastSetting_5_Text").toString());
    on_pushButtonLiveNowEditConfirm_clicked();
}


void MainWindow::on_pushButtonLiveNowFastSetting_6_clicked()
{
    ui->lineEditLiveNowlabel->setText(getSettings.value("Live/FastSetting_6_Text").toString());
    on_pushButtonLiveNowEditConfirm_clicked();
}


void MainWindow::on_action_allowManagerHelp_triggered(bool checked)
{
    if(checked)
    {
        ui->statusbar->showMessage("房管已被允许介入远程管理。");
        label_ManagerConnectInfo->setText("房管远程未连接");
        allowManagerHelp = true;
        managerWebSocketConnect();
    }
    else
    {
        ui->statusbar->showMessage("房管已被禁止介入远程管理。");
        label_ManagerConnectInfo->setText("房管远程已阻止");
        allowManagerHelp = false;
        managerWebSocket->close();
    }
}


void MainWindow::on_action_Settings_triggered()
{
    QString SettingsPath = QDir::currentPath();
    QString SettingsExe = SettingsPath + "/SettingsPage.exe";
    QProcess SettingsProcess;
    SettingsProcess.setProgram(SettingsExe);
    SettingsProcess.setWorkingDirectory(SettingsPath);
    SettingsProcess.startDetached();
}

