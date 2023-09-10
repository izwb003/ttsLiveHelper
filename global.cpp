#include "global.h"

#include <QCoreApplication>
#include <QElapsedTimer>

//Basic Function

/*
 * WaitSec(s)
 * Wait for s seconds.
 */
void waitSec(int s)
{
    QElapsedTimer t;
    t.start();
    while(t.elapsed()<1000*s)
        QCoreApplication::processEvents();
}
