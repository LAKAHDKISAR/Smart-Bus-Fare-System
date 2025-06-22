#ifndef ADMINPANEL_H
#define ADMINPANEL_H

#include <QDialog>

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

signals:
    void currentCheckpointChanged(const QString &checkpointName);

private slots:
    void on_btnSetLocation_clicked();

private:
    Ui::AdminPanel *ui;
    void loadCheckpoints();
};

#endif // ADMINPANEL_H
