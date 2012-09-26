#include "recordingdock.h"
#include "ui_recordingdock.h"

RecordingDock::RecordingDock(QWidget *parent)
    : QDockWidget(parent),
      ui(new Ui::RecordingDock)
{
    ui->setupUi(this);
}

RecordingDock::~RecordingDock()
{
    delete ui;
}

void RecordingDock::reset()
{
    setCount(1);
    setStepping(1);
    setFramesWritten(0, 0);
}

void RecordingDock::setControlsEnabled(bool enable)
{
    ui->spinCount->setEnabled(enable);
    ui->spinStepping->setEnabled(enable);
    ui->buttonSave->setEnabled(enable);
    ui->buttonStop->setEnabled(enable);
}

int RecordingDock::count() const
{
    return ui->spinCount->value();
}

void RecordingDock::setCount(int count)
{
    ui->spinCount->setValue(count);
}

int RecordingDock::stepping() const
{
    return ui->spinStepping->value();
}

void RecordingDock::setStepping(int stepping)
{
    ui->spinCount->setValue(stepping);
}

void RecordingDock::setFramesWritten(int n, int total, const QByteArray &fileId)
{
    ui->labelFileId->setText(fileId);
    ui->labelFilesWritten->setText(
            tr("Wrote %1 of %2 file(s)").arg(n).arg(total));
}

void RecordingDock::on_buttonSave_clicked()
{
    setFramesWritten(0, count());
    emit writeFrames(count(), stepping());
}

void RecordingDock::on_buttonStop_clicked()
{
    emit writeFrames(0, 1);
}
