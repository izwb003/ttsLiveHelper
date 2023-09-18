#ifndef GLOBAL_H
#define GLOBAL_H

#include "wintoastlib.h"
#include <QDebug>

//Extern functions
//Wintoast custom handler
class CustomHandler : public WinToastLib::IWinToastHandler {
public:
    void toastActivated() const {
    }

    void toastActivated(int actionIndex) const {
    }

    void toastFailed() const {
    }

    void toastDismissed(WinToastDismissalReason state) const {
        switch (state) {
        case UserCanceled:
            break;
        case ApplicationHidden:
            break;
        case TimedOut:
            break;
        default:
            break;
        }
    }
};

//Basic Function

/*
 * WaitSec(s)
 * Wait for s seconds.
 */
void waitSec(int s);

#endif // GLOBAL_H
