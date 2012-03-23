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
