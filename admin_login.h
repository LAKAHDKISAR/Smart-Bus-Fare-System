#ifndef ADMIN_LOGIN_H
#define ADMIN_LOGIN_H

#include <QDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

namespace Ui {
class admin_login;
}

class admin_login : public QDialog
{
    Q_OBJECT

public:
    explicit admin_login(QWidget *parent = nullptr);
    ~admin_login();

    static bool isLoggedIn; // global session-like flag

private slots:
    void on_pushButton_login_clicked();

private:
    Ui::admin_login *ui;
    QSqlDatabase db;
    void connectToDatabase();
};

#endif // ADMIN_LOGIN_H
