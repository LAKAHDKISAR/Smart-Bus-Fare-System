#include "admin_login.h"
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    admin_login login;
    if (!admin_login::isLoggedIn) {
        if (login.exec() != QDialog::Accepted) {
            return 0; // Exit if login fails or user cancels
        }
    }

    MainWindow w;
    w.show();
    return a.exec();
}
