#ifndef STARTWINDOW_H
#define STARTWINDOW_H

#include <QWidget>

namespace Ui {
class StartWindow;
}

class StartWindow : public QWidget
{
    Q_OBJECT

public:
    explicit StartWindow(QWidget *parent = nullptr);
    ~StartWindow();

signals:
    void do_StartWindow_Loaded();

private slots:
    void on_pushButtonStopProcess_clicked();
    void do_StartWindow_AfterShow();

private:
    Ui::StartWindow *ui;
};

#endif // STARTWINDOW_H
