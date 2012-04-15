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

    void reset();
    void setControlsEnabled(bool enable);

    int count() const;
    void setCount(int count);

    int stepping() const;
    void setStepping(int stepping);

    void setFramesWritten(int n, int total);

signals:
    void writeFrames(int count, int stepping);

private slots:
    void on_buttonSave_clicked();
    void on_buttonStop_clicked();

private:
    Ui::RecordingDock *ui;
};

#endif // RECORDINGDOCK_H
