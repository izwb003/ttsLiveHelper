#include "dialogabout.h"
#include "ui_dialogabout.h"

#define QUOTE(string) _QUOTE(string)
#define _QUOTE(string) #string

DialogAbout::DialogAbout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogAbout)
{
    ui->setupUi(this);
    ui->labelVersionInfo->setText("tt's Live Helper 版本 " + QString(QUOTE(MACRO_VERSION)) + " 编译于 " + QString(QUOTE(__DATE__)));
    ui->labelQtVersionInfo->setText("基于Qt版本 " + QString(QUOTE(MACRO_QT_VERSION)) + " 构建。");
}

DialogAbout::~DialogAbout()
{
    delete ui;
}
