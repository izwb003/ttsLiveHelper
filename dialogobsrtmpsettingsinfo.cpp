#include "dialogobsrtmpsettingsinfo.h"
#include "ui_dialogobsrtmpsettingsinfo.h"

#include <QClipboard>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QTimer>
#include <QUuid>
#include <QWebSocket>

extern bool isOBSWebSocketConnected;

extern QString liveRTMPAddress;
extern QString liveRTMPCode;

extern QWebSocket *OBSWebSocket;
extern QJsonDocument latestOBSWebSocketMessageJSONDoc;

extern bool setOBSRTMPSettings();

QClipboard *clipboard = QGuiApplication::clipboard();

DialogOBSRTMPSettingsInfo::DialogOBSRTMPSettingsInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogOBSRTMPSettingsInfo)
{
    ui->setupUi(this);

    ui->lineEditRTMPAddress->setText(liveRTMPAddress);
    ui->lineEditRTMPPassword->setText(liveRTMPCode);
}

DialogOBSRTMPSettingsInfo::~DialogOBSRTMPSettingsInfo()
{
    delete ui;
}

void DialogOBSRTMPSettingsInfo::on_pushButtonRTMPAddressCopy_clicked()
{
    clipboard->setText(liveRTMPAddress);
}


void DialogOBSRTMPSettingsInfo::on_pushButtonRTMPPasswordCopy_clicked()
{
    clipboard->setText(liveRTMPCode);
}


void DialogOBSRTMPSettingsInfo::on_pushButtonOBSAuto_clicked()
{
    if(!setOBSRTMPSettings())
        QMessageBox::critical(this, "错误", "OBS未连接，无法设置。", QMessageBox::Close);
    this->close();
}

