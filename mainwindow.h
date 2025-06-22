#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updateFareDisplay(double fareAmount);
    void cleanCheckpoints(int routeId);

public slots:
    void updateUpcomingLocation(const QString &currentCheckpoint);


private slots:
    void on_btnEnterBus_clicked();

    void on_btnExitBus_clicked();

    void on_btnRechargeCard_clicked();

    void on_btnAdminPanel_clicked();

    void on_btnExit_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
