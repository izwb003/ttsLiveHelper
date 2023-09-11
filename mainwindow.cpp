#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "global.h"

#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QTimer>
#include <QWebSocket>

QSettings getSettings("Settings.ini", QSettings::IniFormat);

QTimer *currentTimeTimer;
QTimer *bilibiliLiveStatusUpdateTimer;

QWebSocket *OBSWebSocket = new QWebSocket();
QString latestOBSWebSocketMessageStr;
QJsonDocument latestOBSWebSocketMessageJSONDoc;
bool isOBSWebSocketConnected = false;

bool isOnAir = false;

void MainWindow::buildUI()
{
    /*
     * Some notes:
     * By declaring these three Labels in this way, when using the MSVC2019 compiler,
     * an error will be reported when exiting the program: Run Time Check Failure # 2 – Stack around the variable 'mainWindow' was corrupted.
     * I was unable to identify the cause of the error, so I am temporarily using the MinGW compiler to complete the work.
     */

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
}

void MainWindow::showTime()
{
    QString string;
    QDateTime Timedata = QDateTime::currentDateTime();   //  获取当前时间
    string = Timedata.toString("yyyy-MM-dd hh:mm:ss");   //  设置显示格式
    ui->labelCurrentTime->setText(string);  // 设置标签内容
}

void MainWindow::updateBilibiliLiveStatus()
{
    //GET room info
    QNetworkRequest updateRequest;
    QNetworkReply *updateReply = nullptr;
    QString strUpdateReply;
    QNetworkAccessManager updateManager;
    QString updateRequestURL = "https://api.live.bilibili.com/room/v1/Room/get_info?room_id=" + getSettings.value("Operate/Bilibili_Live_Room").toString();
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
                if(isOnAir)
                {
                    if(!ui->lineEditLiveTitle->hasFocus())
                        ui->lineEditLiveTitle->setText(jsonObjUpdateReply_data.value("title").toString());
                    ui->labelOnline->setText(QString::number(jsonObjUpdateReply_data.value("online").toInt()));
                    ui->labelArea->setText(jsonObjUpdateReply_data.value("area_name").toString());
                    QString strStartLiveTime = jsonObjUpdateReply_data.value("live_time").toString();
                    QDateTime startLiveTime = QDateTime::fromString(strStartLiveTime, "yyyy-MM-dd hh:mm:ss");
                    QDateTime currentTime = QDateTime::currentDateTime();
                    //Calculate time difference
                    uint start_time_timeStamp = startLiveTime.toSecsSinceEpoch();
                    uint current_time_timeStamp = currentTime.toSecsSinceEpoch();
                    uint time_passed_secs = current_time_timeStamp - start_time_timeStamp;
                    QTime time_passed_QTime(0,0,0);
                    time_passed_QTime = time_passed_QTime.addSecs(time_passed_secs);
                    QString time_passed_string = time_passed_QTime.toString("hh:mm:ss");
                    ui->labelLiveTime->setText(time_passed_string);
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
    }
    else
    {
        isOnAir = false;
        ui->labelLiveStatus->setText("未直播");
        paletteSet.setColor(QPalette::WindowText, Qt::black);
        ui->labelLiveStatus->setPalette(paletteSet);
        label_LiveStatus->setText("直播未开始");
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

    //Generate UI components
    buildUI();

    //Set Current Time Timer
    currentTimeTimer = new QTimer;
    connect(currentTimeTimer, &QTimer::timeout, this, &MainWindow::showTime);
    currentTimeTimer->start(1000);
    
    //Set BilibiliLive Status Updater Timer
    bilibiliLiveStatusUpdateTimer = new QTimer;
    connect(bilibiliLiveStatusUpdateTimer, &QTimer::timeout, this, &MainWindow::updateBilibiliLiveStatus);
    bilibiliLiveStatusUpdateTimer->start(5000);


    //Loaded Complete
    emit do_MainWindow_Loaded();
}

MainWindow::~MainWindow()
{
    delete ui;
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
    }
    if(!connected)
    {
        isOBSWebSocketConnected = false;
        label_OBSConnectInfo->setText("OBS未连接");
        ui->statusbar->showMessage("OBS连接已断开。");
    }
}

void MainWindow::do_OBSWebSocketConnected()
{
    waitSec(1);
    OBSWebSocket->sendTextMessage("{ \"op\": 1, \"d\": { \"rpcVersion\": 1} }");
    waitSec(1);
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
}

void MainWindow::do_MainWindow_AfterShow()
{
    //Connect to OBS
    ui->statusbar->showMessage("正在连接OBS...");
    OBSWebSocketConnect();
    waitSec(2);

    //Complete
    ui->statusbar->showMessage("就绪。");
}

void MainWindow::on_pushButtonOpenBilibiliLiveSettings_clicked()
{
    QString bilibiliLiveSettingsURL = "https://link.bilibili.com/p/center/index/my-room/start-live#/my-room/start-live";
    if(!QDesktopServices::openUrl(QUrl(bilibiliLiveSettingsURL)))
        QMessageBox::critical(this, "错误", "打开Bilibili直播设置页未成功。", QMessageBox::Close);
}
