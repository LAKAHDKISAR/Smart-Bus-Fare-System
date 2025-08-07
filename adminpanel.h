#ifndef ADMINPANEL_H
#define ADMINPANEL_H

#include <QDialog>
#include "direction.h"

namespace Ui {
class AdminPanel;
}

class AdminPanel : public QDialog
{
    Q_OBJECT

public:
    explicit AdminPanel(QWidget *parent = nullptr);
    ~AdminPanel();
    QString getCurrentCheckpoint() const;
    Direction getCurrentDirection() const;  // add getter for direction

signals:
    void currentCheckpointChanged(const QString &checkpointName, Direction direction);

private slots:
    void on_btnSetLocation_clicked();

private:
    Ui::AdminPanel *ui;
    void loadCheckpoints();

    Direction currentDirection = Forward;
};

#endif // ADMINPANEL_H
