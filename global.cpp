#include "global.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTimer>

//Basic Function

/*
 * WaitSec(s)
 * Wait for s seconds.
 */
void waitSec(int s)
{
    QEventLoop loop;
    QTimer::singleShot(s*1000, &loop, SLOT(quit()));
    loop.exec();
}
