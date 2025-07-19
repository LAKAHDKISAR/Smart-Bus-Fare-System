#include "admin_login.h"
#include <QDateTime>

AdminLogin::AdminLogin(QWidget *parent)
    : QDialog(parent)
{
    // Initialize database connection
    if (!initializeDatabase()) {
        QMessageBox::critical(nullptr, "Database Error", "Cannot connect to admin database");
        reject();
        return;
    }

    // UI Setup (same as before)
    setWindowTitle("Admin Login");
    setFixedSize(320, 220);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    titleLabel = new QLabel("Administrator Login");
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    usernameLabel = new QLabel("Username:");
    passwordLabel = new QLabel("Password:");
    usernameEdit = new QLineEdit();
    passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);

    loginButton = new QPushButton("Login");
    cancelButton = new QPushButton("Cancel");

    // Layout setup (same as before)
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    formLayout->addRow(usernameLabel, usernameEdit);
    formLayout->addRow(passwordLabel, passwordEdit);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(loginButton);

    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(buttonLayout);
    mainLayout->setContentsMargins(25, 20, 25, 20);

    connect(loginButton, &QPushButton::clicked, this, &AdminLogin::attemptLogin);
    connect(cancelButton, &QPushButton::clicked, this, &AdminLogin::cancelLogin);
    connect(usernameEdit, &QLineEdit::returnPressed, this, &AdminLogin::attemptLogin);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &AdminLogin::attemptLogin);
}

AdminLogin::~AdminLogin()
{
    if (db.isOpen()) {
        db.close();
    }
}

bool AdminLogin::initializeDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE", "admin_connection");
    db.setDatabaseName("smart_bus_fare_system.db");

    if (!db.open()) {
        return false;
    }

    // Create admin table if it doesn't exist
    QSqlQuery query(db);
    return query.exec(
        "CREATE TABLE IF NOT EXISTS admins ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "password_hash TEXT NOT NULL,"
        "salt TEXT NOT NULL,"
        "last_login TEXT,"
        "failed_attempts INTEGER DEFAULT 0,"
        "account_locked INTEGER DEFAULT 0,"
        "lock_until TEXT"
        ")");
}

bool AdminLogin::authenticate(const QString &username, const QString &password)
{
    if (isAccountLocked(username)) {
        QMessageBox::warning(this, "Account Locked",
                             "This account is temporarily locked due to too many failed attempts.");
        return false;
    }

    QSqlQuery query(db);
    query.prepare("SELECT password_hash, salt FROM admins WHERE username = ?");
    query.addBindValue(username);

    if (!query.exec() || !query.next()) {
        logLoginAttempt(username, false);
        return false;
    }

    QString storedHash = query.value(0).toString();
    QString salt = query.value(1).toString();
    QString computedHash = hashPassword(password, salt);

    if (computedHash == storedHash) {
        // Reset failed attempts on successful login
        query.prepare("UPDATE admins SET failed_attempts = 0, last_login = datetime('now') WHERE username = ?");
        query.addBindValue(username);
        query.exec();

        logLoginAttempt(username, true);
        return true;
    } else {
        // Increment failed attempts
        query.prepare("UPDATE admins SET failed_attempts = failed_attempts + 1 WHERE username = ?");
        query.addBindValue(username);
        query.exec();

        // Lock account after 5 failed attempts
        query.prepare("UPDATE admins SET account_locked = 1, lock_until = datetime('now', '+30 minutes') "
                      "WHERE username = ? AND failed_attempts >= 5");
        query.addBindValue(username);
        query.exec();

        logLoginAttempt(username, false);
        return false;
    }
}

QString AdminLogin::hashPassword(const QString &password, const QString &salt)
{
    // Use PBKDF2 with SHA-256 for password hashing
    QByteArray hash;
    QByteArray passwordData = (password + salt).toUtf8();


    hash = QCryptographicHash::hash(passwordData, QCryptographicHash::Sha256);
    for (int i = 0; i < 99999; ++i) {
        hash = QCryptographicHash::hash(hash + passwordData, QCryptographicHash::Sha256);
    }

    return hash.toHex();
}

QString AdminLogin::generateSalt()
{
    // Generate a random 32-byte salt
    QByteArray salt;
    salt.resize(32);

    for (int i = 0; i < salt.size(); ++i) {
        salt[i] = rand() % 256;
    }

    return salt.toHex();
}

bool AdminLogin::isAccountLocked(const QString &username)
{
    QSqlQuery query(db);
    query.prepare("SELECT account_locked, lock_until FROM admins WHERE username = ?");
    query.addBindValue(username);

    if (query.exec() && query.next()) {
        bool locked = query.value(0).toBool();
        QString lockUntil = query.value(1).toString();

        if (locked && !lockUntil.isEmpty()) {
            QDateTime lockTime = QDateTime::fromString(lockUntil, Qt::ISODate);
            if (QDateTime::currentDateTime() < lockTime) {
                return true;
            } else {
                // Auto-unlock if lock time has passed
                query.prepare("UPDATE admins SET account_locked = 0, failed_attempts = 0 WHERE username = ?");
                query.addBindValue(username);
                query.exec();
                return false;
            }
        }
    }
    return false;
}

void AdminLogin::logLoginAttempt(const QString &username, bool success)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO login_logs (username, attempt_time, success) "
                  "VALUES (?, datetime('now'), ?A)");
    query.addBindValue(username);
    query.addBindValue(success ? 1 : 0);
    query.exec();
}

void AdminLogin::attemptLogin()
{
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Login Failed", "Both username and password are required.");
        return;
    }

    if (authenticate(username, password)) {
        QMessageBox::information(this, "Success", "Admin login successful!");
        accept();
    } else {
        QMessageBox::critical(this, "Login Failed", "Invalid admin credentials.");
        passwordEdit->clear();
        usernameEdit->setFocus();
    }
}



void AdminLogin::cancelLogin()
{
    reject();
}
