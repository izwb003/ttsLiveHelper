#include "startwindow.h"
#include "ui_startwindow.h"

#include "global.h"
#include "wintoastlib.h"

#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QMessageBox>
#include <QPainterPath>
#include <QProcess>
#include <QSettings>

#define QUOTE(string) _QUOTE(string)
#define _QUOTE(string) #string

StartWindow::StartWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StartWindow)
{
    ui->setupUi(this);

    //Make it more beautiful
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    QPainterPath painterPathWidget;
    painterPathWidget.addRoundedRect(this->rect(), 10, 10);
    QRegion windowMask = QRegion(painterPathWidget.toFillPolygon().toPolygon());
    this->setMask(windowMask);
    QPainterPath painterPathPic;
    painterPathPic.addRoundedRect(ui->labelStartWindowPic->rect(), 10, 10);
    QRegion picMask = QRegion(painterPathPic.toFillPolygon().toPolygon());
    ui->labelStartWindowPic->setMask(picMask);

    ui->labelVersionInfo->setText("版本 " + QString(QUOTE(MACRO_VERSION)) + " 构建于 " + QString(QUOTE(__DATE__)));

    //Signal Connect
    connect(this, SIGNAL(do_StartWindow_Loaded()), this, SLOT(do_StartWindow_AfterShow()), Qt::QueuedConnection);
    //Loaded complete
    emit do_StartWindow_Loaded();
}

StartWindow::~StartWindow()
{
    delete ui;
}

void StartWindow::on_pushButtonStopProcess_clicked()
{
    qApp->exit(0);
}

void StartWindow::do_StartWindow_AfterShow()
{
    //Begin Process

    //Read ini setup
    ui->labelStartWindowProcessInfo->setText("读取配置文件");
    if(!QFile::exists("Settings.ini"))
    {
        QMessageBox::critical(this, "错误", "没有找到Settings.ini配置文件。", QMessageBox::Close);
        qApp->exit(0);
    }
    QSettings getSettings("Settings.ini", QSettings::IniFormat);

#ifndef DEBUG
    /*
     * I don't want it to startup so many programs when I am debugging it. So a macro may help.
     * See CMakeLists:
     * if(CMAKE_BUILD_TYPE STREQUAL Debug)
     * add_definitions(-DDEBUG)
     * endif()
     */

    //Execute Programs
    waitSec(1);
    //BLiveChat
    if(getSettings.value("Execute/BLiveChat_OnStart").toBool())
    {
        ui->labelStartWindowProcessInfo->setText("启动BLiveChat...");
        if(getSettings.contains("Execute/BLiveChat_Path"))
        {
            QString BLiveChatPath = getSettings.value("Execute/BLiveChat_Path").toString();
            QString BLiveChatExe = BLiveChatPath + "/blivechat.exe";
            //qDebug()<<BLiveChatPath<<"    "<<BLiveChatExe;
            QProcess BLiveChatProcess;
            BLiveChatProcess.setProgram(BLiveChatExe);
            BLiveChatProcess.setWorkingDirectory(BLiveChatPath);
            BLiveChatProcess.startDetached();
        }
        else
            QMessageBox::critical(this, "错误", "没有找到BLiveChat位置。将不会自动启动BLiveChat。", QMessageBox::Close);
        waitSec(1);
    }
    //OBS
    if(getSettings.value("Execute/OBS_OnStart").toBool())
    {
        ui->labelStartWindowProcessInfo->setText("启动OBS...");
        if(getSettings.contains("Execute/OBS_URL"))
        {
            QString OBSUrl = getSettings.value("Execute/OBS_URL").toString();
            if(!QDesktopServices::openUrl(QUrl(OBSUrl)))
                QMessageBox::critical(this, "错误", "启动OBS未成功，将跳过。", QMessageBox::Close);
        }
        else
            QMessageBox::critical(this, "错误", "没有找到OBS位置。将不会自动启动OBS。", QMessageBox::Close);
        waitSec(1);
    }
    //Danmuji
    if(getSettings.value("Execute/Danmuji_OnStart").toBool())
    {
        ui->labelStartWindowProcessInfo->setText("启动弹幕姬...");
        if(getSettings.contains("Execute/Danmuji_URL"))
        {
            QString DanmujiUrl = getSettings.value("Execute/Danmuji_URL").toString();
            if(!QDesktopServices::openUrl(QUrl(DanmujiUrl)))
                QMessageBox::critical(this, "错误", "启动弹幕姬未成功，将跳过。", QMessageBox::Close);
        }
        else
            QMessageBox::critical(this, "错误", "没有找到弹幕姬位置。将不会自动启动弹幕姬。", QMessageBox::Close);
        waitSec(1);
    }
    //LiveStart Settings (No need as automatic process were set)
    /*
    ui->labelStartWindowProcessInfo->setText("打开开播设置网站...");
    QString LiveStartUrl = "https://link.bilibili.com/p/center/index#/my-room/start-live";
    if(!QDesktopServices::openUrl(QUrl(LiveStartUrl)))
        QMessageBox::critical(this, "错误", "打开直播间设置页面失败，将跳过。", QMessageBox::Close);
    waitSec(1);
    */
#endif

    //WinToast init
    ui->labelStartWindowProcessInfo->setText("初始化通知功能...");
    WinToastLib::WinToast::instance()->setAppName(L"tt's Live Helper");
    const auto aumi = WinToastLib::WinToast::configureAUMI(L"izwb003", L"LiveHelper", L"ttsLiveHelper", L"20230917");
    WinToastLib::WinToast::instance()->setAppUserModelId(aumi);
    if (!WinToastLib::WinToast::instance()->initialize()) {
        std::wcout << L"Error, could not initialize the lib!" << std::endl;
    }
    else
        std::cout<<"OK"<<std::endl;

    //Message Hint
    ui->labelStartWindowProcessInfo->setText("配置完成。感谢您今日准时开播。");
    waitSec(1);
    //Done, close.
    this->close();
}
