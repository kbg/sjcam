#ifndef RECORDINGDOCK_H
#define RECORDINGDOCK_H

#include <QtGui/QDockWidget>

namespace Ui {
    class RecordingDock;
}

class RecordingDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit RecordingDock(QWidget *parent = 0);
    ~RecordingDock();

private:
    Ui::RecordingDock *ui;
};

#endif // RECORDINGDOCK_H
