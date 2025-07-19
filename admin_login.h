#ifndef ADMIN_LOGIN_H
#define ADMIN_LOGIN_H

#include <QDialog>
#include <QMessageBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QCryptographicHash>
#include <QSettings>

class AdminLogin : public QDialog
{
    Q_OBJECT

public:
    explicit AdminLogin(QWidget *parent = nullptr);
    ~AdminLogin();

private slots:
    void attemptLogin();
    void cancelLogin();

private:
    // UI Elements
    QLabel *titleLabel;
    QLabel *usernameLabel;
    QLabel *passwordLabel;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QPushButton *cancelButton;

    // Database connection
    QSqlDatabase db;

    bool initializeDatabase();
    bool authenticate(const QString &username, const QString &password);
    QString hashPassword(const QString &password, const QString &salt);
    QString generateSalt();
    void logLoginAttempt(const QString &username, bool success);
    bool isAccountLocked(const QString &username);
};

#endif // ADMIN_LOGIN_H
