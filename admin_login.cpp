#include "admin_login.h"
#include "ui_admin_login.h"
#include <QMessageBox>

bool admin_login::isLoggedIn = false;

admin_login::admin_login(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::admin_login)
{
    ui->setupUi(this);
    connectToDatabase();

    //Construtor for the LOGIN Button
    connect(ui->pushButton_login, &QPushButton::clicked, this, &admin_login::on_pushButton_login_clicked);
}

admin_login::~admin_login()
{
    db.close(); // clean close
    delete ui;
}

void admin_login::connectToDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("smart_bus_fare_system.db");

    if (!db.open()) {
        QMessageBox::critical(this, "Database Error", "Failed to connect to database!");
    }
}

void admin_login::on_pushButton_login_clicked()
{
    qDebug() << "[DEBUG] Login button clicked!";
    QString username = ui->lineEdit_username->text().trimmed();
    QString password = ui->lineEdit_password->text().trimmed();

    if (!db.isOpen()) {
        QMessageBox::warning(this, "Error", "Database is not open.");
        return;
    }

    // Debug check j garda ni kaam gardaina mero bhejaa fry vayo yar
    qDebug() << "[DEBUG] Username entered:" << username;
    qDebug() << "[DEBUG] Password entered:" << password;

    //Despite of the valid credentials it is not letting me login.
    QSqlQuery query;
    query.prepare("SELECT * FROM Admin WHERE TRIM(username) = :username AND TRIM(password) = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", password);

    if (!query.exec()) {
        QMessageBox::critical(this, "SQL Error", query.lastError().text());
        return;
    }

    if (query.next()) {
        // Success
        QMessageBox::information(this, "Login Success", "Welcome " + username);
        admin_login::isLoggedIn = true;
        accept(); // closes the login dialog
    } else {
        QMessageBox::warning(this, "Login Failed", "Invalid username or password.");
    }
}
