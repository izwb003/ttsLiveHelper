#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Signal Connect
    connect(this, SIGNAL(do_MainWindow_Loaded()), this, SLOT(do_MainWindow_AfterShow()), Qt::QueuedConnection);
    //Loaded Complete
    emit do_MainWindow_Loaded();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::do_MainWindow_AfterShow()
{

}
