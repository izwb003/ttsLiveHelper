#include "startwindow.h"
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    StartWindow wStart;
    wStart.show();
    MainWindow mainWindow;
    mainWindow.show();
    return a.exec();
}
