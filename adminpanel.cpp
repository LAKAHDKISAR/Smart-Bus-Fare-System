#include "adminpanel.h"
#include "ui_adminpanel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

AdminPanel::AdminPanel(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AdminPanel)
{
    ui->setupUi(this);
    loadCheckpoints();
}

AdminPanel::~AdminPanel()
{
    delete ui;
}





void AdminPanel::loadCheckpoints()
{

    ui->combocheckpoint->clear();

    static int printCount = 0;
    printCount++;
    qDebug() << "Print call count:" << printCount;
    // Then print checkpoints


    QSqlQuery query;
    query.prepare("SELECT checkpoint_name FROM checkpoints ORDER BY distance_from_start ASC");

    QStringList labels = {"A", "B", "C", "D", "E", "F"};
    int index = 0;

    if (query.exec()) {
        while (query.next()) {
            QString checkpoint = query.value(0).toString();
            QString label = (index < labels.size()) ? labels[index] + ". " + checkpoint : checkpoint;
            ui->combocheckpoint->addItem(label, checkpoint);
            index++;
        }
    } else {
        qDebug() << "Failed to load checkpoints:" << query.lastError().text();
    }

    if (ui->combocheckpoint->count() == 0) {
        qDebug() << "ComboBox is empty after loading checkpoints!";
    }


    if (ui->combocheckpoint->count() == 0) {
        ui->btnSetLocation->setEnabled(false);
        ui->labelShowLocation->setText("No checkpoints loaded");
    } else {
        ui->btnSetLocation->setEnabled(true);
    }


}



void AdminPanel::on_btnSetLocation_clicked()
{
    int currentIndex = ui->combocheckpoint->currentIndex();
    QString currentCheckpoint = ui->combocheckpoint->currentData().toString();

    if (currentCheckpoint.isEmpty()) {
        ui->labelShowLocation->setText("No checkpoint selected");
        ui->labelDirection->setText("Direction: -");
        return;
    }

    ui->labelShowLocation->setText("Currently: " + currentCheckpoint);

    QString nextCheckpoint;
    int nextIndex = currentIndex + 1;
    if (nextIndex < ui->combocheckpoint->count()) {
        nextCheckpoint = ui->combocheckpoint->itemData(nextIndex).toString();
    } else {
        nextCheckpoint = "Last Stop";
    }
    ui->labelupcominglocation->setText("Upcoming: " + nextCheckpoint);

    // Updating the direction
    QString directionText;
    directionText = (currentDirection == Forward) ? "Direction: Ratna Park → Thimi": "Direction: Thimi → Ratna Park";

    ui->labelDirection->setText(directionText);

    emit currentCheckpointChanged(currentCheckpoint, currentDirection); //emit currentcheckpoint and also direction

    qDebug() << "Current checkpoint set to:" << currentCheckpoint;
}



QString AdminPanel::getCurrentCheckpoint() const {
    return ui->combocheckpoint->currentData().toString();
}

Direction AdminPanel::getCurrentDirection() const {
    return currentDirection;
}



