#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>

QSettings *getSettings = new QSettings("Settings.ini", QSettings::IniFormat);

void MainWindow::fillSettings()
{
    //Execute
    ui->lineEditBLiveChatPath->setText(getSettings->value("Execute/BLiveChat_Path").toString());
    ui->lineEditDanmujiURL->setText(getSettings->value("Execute/Danmuji_URL").toString());
    ui->lineEditOBSURL->setText(getSettings->value("Execute/OBS_URL").toString());

    ui->checkBoxStartUpDanmuji->setChecked(getSettings->value("Execute/Danmuji_OnStart").toBool());
    ui->checkBoxStartUpBLiveChat->setChecked(getSettings->value("Execute/BLiveChat_OnStart").toBool());
    ui->checkBoxStartUpOBS->setChecked(getSettings->value("Execute/OBS_OnStart").toBool());

    ui->checkBoxOnCloseDanmuji->setChecked(getSettings->value("Execute/Danmuji_OnClose").toBool());
    ui->checkBoxOnCloseBLiveChat->setChecked(getSettings->value("Execute/BLiveChat_OnClose").toBool());
    ui->checkBoxOnCloseOBS->setChecked(getSettings->value("Execute/OBS_OnClose").toBool());

    //Operate
    ui->lineEditOBSWebSocketLink->setText(getSettings->value("Operate/OBSWebSocket_URL").toString());
    ui->lineEditManagerWebSocketLink->setText(getSettings->value("Operate/ManagerWebSocket_URL").toString());
    ui->lineEditBLiveChatManage->setText(getSettings->value("Operate/BLiveChat_URL").toString());

    //BiliBili
    ui->lineEditRoomId->setText(getSettings->value("BiliBili/Bilibili_Live_Room").toString());
    ui->lineEditTargetArea->setText(getSettings->value("BiliBili/area_v2").toString());
    ui->lineEditCSRF->setText(getSettings->value("BiliBili/Cookie_csrf").toString());
    ui->plainTextEditSESSDATA->setPlainText(getSettings->value("BiliBili/Cookie_SESSDATA").toString());

    //Live
    ui->lineEditButton1Label->setText(getSettings->value("Live/FastSetting_1_Label").toString());
    ui->lineEditButton1Text->setText(getSettings->value("Live/FastSetting_1_Text").toString());

    ui->lineEditButton2Label->setText(getSettings->value("Live/FastSetting_2_Label").toString());
    ui->lineEditButton2Text->setText(getSettings->value("Live/FastSetting_2_Text").toString());

    ui->lineEditButton3Label->setText(getSettings->value("Live/FastSetting_3_Label").toString());
    ui->lineEditButton3Text->setText(getSettings->value("Live/FastSetting_3_Text").toString());

    ui->lineEditButton4Label->setText(getSettings->value("Live/FastSetting_4_Label").toString());
    ui->lineEditButton4Text->setText(getSettings->value("Live/FastSetting_4_Text").toString());

    ui->lineEditButton5Label->setText(getSettings->value("Live/FastSetting_5_Label").toString());
    ui->lineEditButton5Text->setText(getSettings->value("Live/FastSetting_5_Text").toString());

    ui->lineEditButton6Label->setText(getSettings->value("Live/FastSetting_6_Label").toString());
    ui->lineEditButton6Text->setText(getSettings->value("Live/FastSetting_6_Text").toString());
}

void MainWindow::applySettings()
{
    //Execute
    getSettings->beginGroup("Execute");
    getSettings->setValue("BLiveChat_Path", ui->lineEditBLiveChatPath->text());
    getSettings->setValue("Danmuji_URL", ui->lineEditDanmujiURL->text());
    getSettings->setValue("OBS_URL", ui->lineEditOBSURL->text());

    getSettings->setValue("Danmuji_OnStart", ui->checkBoxStartUpDanmuji->isChecked());
    getSettings->setValue("BLiveChat_OnStart", ui->checkBoxStartUpBLiveChat->isChecked());
    getSettings->setValue("OBS_OnStart", ui->checkBoxStartUpOBS->isChecked());

    getSettings->setValue("Danmuji_OnClose", ui->checkBoxOnCloseDanmuji->isChecked());
    getSettings->setValue("BLiveChat_OnClose", ui->checkBoxOnCloseBLiveChat->isChecked());
    getSettings->setValue("OBS_OnClose", ui->checkBoxOnCloseOBS->isChecked());
    getSettings->endGroup();

    //Operate
    getSettings->beginGroup("Operate");
    getSettings->setValue("OBSWebSocket_URL", ui->lineEditOBSWebSocketLink->text());
    getSettings->setValue("ManagerWebSocket_URL", ui->lineEditManagerWebSocketLink->text());
    getSettings->setValue("BLiveChat_URL", ui->lineEditBLiveChatManage->text());
    getSettings->endGroup();

    //BiliBili
    getSettings->beginGroup("BiliBili");
    getSettings->setValue("Bilibili_Live_Room", ui->lineEditRoomId->text());
    getSettings->setValue("area_v2", ui->lineEditTargetArea->text());
    getSettings->setValue("Cookie_csrf", ui->lineEditCSRF->text());
    getSettings->setValue("Cookie_SESSDATA", ui->plainTextEditSESSDATA->toPlainText());
    getSettings->endGroup();

    //Live
    getSettings->beginGroup("Live");
    getSettings->setValue("FastSetting_1_Label", ui->lineEditButton1Label->text());
    getSettings->setValue("FastSetting_1_Text", ui->lineEditButton1Text->text());

    getSettings->setValue("FastSetting_2_Label", ui->lineEditButton2Label->text());
    getSettings->setValue("FastSetting_2_Text", ui->lineEditButton2Text->text());

    getSettings->setValue("FastSetting_3_Label", ui->lineEditButton3Label->text());
    getSettings->setValue("FastSetting_3_Text", ui->lineEditButton3Text->text());

    getSettings->setValue("FastSetting_4_Label", ui->lineEditButton4Label->text());
    getSettings->setValue("FastSetting_4_Text", ui->lineEditButton4Text->text());

    getSettings->setValue("FastSetting_5_Label", ui->lineEditButton5Label->text());
    getSettings->setValue("FastSetting_5_Text", ui->lineEditButton5Text->text());

    getSettings->setValue("FastSetting_6_Label", ui->lineEditButton6Label->text());
    getSettings->setValue("FastSetting_6_Text", ui->lineEditButton6Text->text());
    getSettings->endGroup();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //Check if Settings Exist
    QFileInfo settingsFileInfo("Settings.ini");
    if(!settingsFileInfo.isFile())
    {
        QMessageBox::critical(this, "错误", "没有找到配置文件。");
        QFile file("Settings.ini");
        file.open(QIODevice::ReadWrite);
    }
    fillSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButtonBrowseBLiveChat_clicked()
{
    ui->lineEditBLiveChatPath->setText(QFileDialog::getExistingDirectory(this, "选择BLiveChat所在目录位置..."));
}


void MainWindow::on_pushButtonBrowseDanmuji_clicked()
{
    ui->lineEditDanmujiURL->setText(QFileDialog::getOpenFileUrl(this, "选择弹幕姬启动位置...").toString());
}


void MainWindow::on_pushButtonBrowseOBS_clicked()
{
    ui->lineEditOBSURL->setText(QFileDialog::getOpenFileUrl(this, "选择OBS启动位置...").toString());
}


void MainWindow::on_pushButtonYes_clicked()
{
    applySettings();
    QMessageBox::information(this, "提示", "修改已完成。下次启动生效。");
    qApp->quit();
}


void MainWindow::on_pushButtonNo_clicked()
{
    qApp->quit();
}


void MainWindow::on_pushButtonApply_clicked()
{
    applySettings();
    QMessageBox::information(this, "提示", "修改已完成。下次启动生效。");
}

