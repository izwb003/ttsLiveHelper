#ifndef DIALOGOBSRTMPSETTINGSINFO_H
#define DIALOGOBSRTMPSETTINGSINFO_H

#include <QDialog>

namespace Ui {
class DialogOBSRTMPSettingsInfo;
}

class DialogOBSRTMPSettingsInfo : public QDialog
{
    Q_OBJECT

public:
    explicit DialogOBSRTMPSettingsInfo(QWidget *parent = nullptr);
    ~DialogOBSRTMPSettingsInfo();

private slots:
    void on_pushButtonRTMPAddressCopy_clicked();

    void on_pushButtonRTMPPasswordCopy_clicked();

    void on_pushButtonOBSAuto_clicked();

private:
    Ui::DialogOBSRTMPSettingsInfo *ui;
};

#endif // DIALOGOBSRTMPSETTINGSINFO_H
