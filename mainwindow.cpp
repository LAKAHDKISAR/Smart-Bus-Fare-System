#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "adminpanel.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    // DB Driver Name : SQL Lite
    // DATABASE OF THE ROUTE (RATNAPARK - THIMI).

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("smart_bus_fare_system.db");

    //Generated the tabular format of the database in DB Browser

    if (!db.open()) {
        qDebug() << "Error: connection with database failed - " << db.lastError().text();
    } else {
        qDebug() << "Database connected successfully.";


        QSqlQuery query;
        QString createTable = "CREATE TABLE IF NOT EXISTS routes ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                              "route_name TEXT,"
                              "start_point TEXT,"
                              "end_point TEXT,"
                              "total_distance REAL"
                              ")";
        if (!query.exec(createTable)) {
            qDebug() << "Failed to create routes table: " << query.lastError().text();
        }

        QString createCheckpointsTable = "CREATE TABLE IF NOT EXISTS checkpoints ("
                                         "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                         "route_id INTEGER,"
                                         "checkpoint_name TEXT,"
                                         "distance_from_start REAL,"
                                         "FOREIGN KEY(route_id) REFERENCES routes(id)"
                                         ")";
        if (!query.exec(createCheckpointsTable)) {
            qDebug() << "Failed to create checkpoints table: " << query.lastError().text();
        }

        QString createTravelLogTable = "CREATE TABLE IF NOT EXISTS travel_log ("
                                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                       "card_id TEXT,"
                                       "entry_checkpoint TEXT,"
                                       "entry_time TEXT,"
                                       "exit_checkpoint TEXT,"
                                       "exit_time TEXT,"
                                       "fare_charged REAL)";
        if (!query.exec(createTravelLogTable)) {
            qDebug() << "Failed to create travel_log table:" << query.lastError().text();
        }

        QString createCardsTable = "CREATE TABLE IF NOT EXISTS cards ("
                                   "card_id TEXT PRIMARY KEY,"
                                   "balance REAL DEFAULT 0"
                                   ")";
        if (!query.exec(createCardsTable)) {
            qDebug() << "Failed to create cards table: " << query.lastError().text();
        }

        //------------------------

        //

        query.exec("SELECT COUNT(*) FROM routes WHERE route_name = 'Route Ratna Park - Thimi'");
        if (query.next()) {
            int count = query.value(0).toInt();

            // Always get the routeId
            int routeId = 0;
            if (count == 0) {
                // Insert sample route if not already present
                if (!query.exec("INSERT INTO routes (route_name, start_point, end_point, total_distance) VALUES ('Route Ratna Park - Thimi', 'Ratna Park', 'Thimi', 10.0)")) {
                    qDebug() << "Failed to insert sample route: " << query.lastError().text();
                }
            }

            if (query.exec("SELECT id FROM routes WHERE route_name = 'Route Ratna Park - Thimi'")) {
                if (query.next()) {
                    routeId = query.value(0).toInt();
                }
            }


            cleanCheckpoints(routeId);

            // Insert checkpoints afresh
            QStringList points = {"Ratna Park", "MaitiGhar", "New Baneshwor", "Tinkune", "Koteshwor", "Thimi"};
            QList<double> distances = {0.0, 2.0, 4.0, 6.5, 8.0, 10.0};

            for (int i = 0; i < points.size(); ++i) {
                QSqlQuery insertCheckpointQuery;
                insertCheckpointQuery.prepare("INSERT INTO checkpoints (route_id, checkpoint_name, distance_from_start) VALUES (?, ?, ?)");
                insertCheckpointQuery.addBindValue(routeId);
                insertCheckpointQuery.addBindValue(points[i]);
                insertCheckpointQuery.addBindValue(distances[i]);
                if (!insertCheckpointQuery.exec()) {
                    qDebug() << "Failed to insert checkpoint: " << insertCheckpointQuery.lastError().text();
                }
            }

        }
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}


// ENTER BUS

void MainWindow::on_btnEnterBus_clicked()
{
    QString cardId = QInputDialog::getText(this, "Enter Bus", "Enter your Card ID:");
    if (cardId.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Card ID cannot be empty.");
        return;
    }

    QSqlQuery query;  // declare once

    query.prepare("SELECT balance FROM cards WHERE card_id = ?");
    query.addBindValue(cardId);

    if (!query.exec()) {
        QMessageBox::critical(this, "Error", "Database error: " + query.lastError().text());
        return;
    }

    if (!query.next()) {

        // SOMETIMES IF USER IS NEW THEN CONFLICT BHAYERA IT DOESNT WORK SO IF NEW USER THEN STARTS WITH ZERO BALANCE KIND OF REGISTER ( PURANO CODE IN MIXED BHAYO JASTO LAGCHA BUT LEAVE IT FOR NOW --- IF IT WORKS IT WORKS)

        QSqlQuery insertQuery;  // NEW VARIABLE HERE TO AVOID CONFLICT
        insertQuery.prepare("INSERT INTO cards (card_id, balance) VALUES (?, ?)");
        insertQuery.addBindValue(cardId);
        insertQuery.addBindValue(0.0);
        if (!insertQuery.exec()) {
            QMessageBox::critical(this, "Error", "Failed to create new card: " + insertQuery.lastError().text());
            return;
        }
    }

    // UPCOMING CHECKPOINT LINE
    QString entryCheckpoint = ui->labelUpcomingCheckpoint->text().replace("Next stop: ", "");
    QString entryTime = QDateTime::currentDateTime().toString(Qt::ISODate); // DATE TIME CLASS

    // Reusing original query
    query.prepare("INSERT INTO travel_log (card_id, entry_checkpoint, entry_time) "
                  "VALUES (:cardId, :entryCheckpoint, :entryTime)");
    query.bindValue(":cardId", cardId);
    query.bindValue(":entryCheckpoint", entryCheckpoint);
    query.bindValue(":entryTime", entryTime);

    if (query.exec()) {
        QMessageBox::information(this, "Success", "Entry logged successfully.");
    } else {
        QMessageBox::critical(this, "Error", "Failed to log entry: " + query.lastError().text());
    }
}



// EXIT BUS ----------------------------------------------------------------------

void MainWindow::on_btnExitBus_clicked()
{
    QString cardId = QInputDialog::getText(this, "Exit Bus", "Enter your Card ID:");
    if(cardId.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Card ID cannot be empty.");
        return;
    }

    QString currentCheckpoint = ui->labelUpcomingCheckpoint->text().replace("Next stop: ", "").trimmed();

    if (currentCheckpoint.isEmpty() || currentCheckpoint == "End of route") {
        QMessageBox::warning(this, "Warning", "Current checkpoint not set or end of route.");
        return;
    }

    QSqlQuery query;

    // Step 1: Get the last entry where exit is NULL
    query.prepare("SELECT id, entry_checkpoint FROM travel_log WHERE card_id = ? AND exit_checkpoint IS NULL ORDER BY id DESC LIMIT 1");
    query.addBindValue(cardId);

    if (!query.exec() || !query.next()) {
        QMessageBox::warning(this, "Warning", "No matching entry found or already exited.");
        return;
    }

    int travelId = query.value(0).toInt();
    QString entryCheckpoint = query.value(1).toString();

    // Step 2: Get distance of entry & exit checkpoints
    QSqlQuery distanceQuery;
    double entryDist = 0.0, exitDist = 0.0;

    distanceQuery.prepare("SELECT distance_from_start FROM checkpoints WHERE checkpoint_name = ?");
    distanceQuery.addBindValue(entryCheckpoint);
    if (distanceQuery.exec() && distanceQuery.next()) {
        entryDist = distanceQuery.value(0).toDouble();
    }

    distanceQuery.prepare("SELECT distance_from_start FROM checkpoints WHERE checkpoint_name = ?");
    distanceQuery.addBindValue(currentCheckpoint);
    if (distanceQuery.exec() && distanceQuery.next()) {
        exitDist = distanceQuery.value(0).toDouble();
    }

    // Calculate fare

    double distance = qAbs(exitDist - entryDist);
    double farePerKm = 10.0;
    double fare = distance * farePerKm;

    // balance

    QSqlQuery balanceQuery;
    balanceQuery.prepare("SELECT balance FROM cards WHERE card_id = ?");
    balanceQuery.addBindValue(cardId);

    if (!balanceQuery.exec() || !balanceQuery.next()) {
        QMessageBox::warning(this, "Warning", "Card not found.");
        return;
    }

    double balance = balanceQuery.value(0).toDouble();

    if (balance < fare) {
        QMessageBox::warning(this, "Warning", "Insufficient balance. Please recharge your card.");
        return;
    }

    double newBalance = balance - fare;

    // Update balance

    QSqlQuery updateBalanceQuery;
    updateBalanceQuery.prepare("UPDATE cards SET balance = ? WHERE card_id = ?");
    updateBalanceQuery.addBindValue(newBalance);
    updateBalanceQuery.addBindValue(cardId);

    if (!updateBalanceQuery.exec()) {
        QMessageBox::critical(this, "Error", "Failed to update balance: " + updateBalanceQuery.lastError().text());
        return;
    }

    // Update record

    QSqlQuery updateQuery;
    updateQuery.prepare("UPDATE travel_log SET exit_checkpoint = ?, exit_time = ?, fare_charged = ? WHERE id = ?");
    updateQuery.addBindValue(currentCheckpoint);
    updateQuery.addBindValue(QDateTime::currentDateTime().toString());
    updateQuery.addBindValue(fare);
    updateQuery.addBindValue(travelId);

    if (updateQuery.exec()) {
        QMessageBox::information(this, "Success", QString("Trip completed. Fare: ₨ %1").arg(fare, 0, 'f', 2));
        updateFareDisplay(fare);
    } else {
        QMessageBox::critical(this, "Error", "Failed to update travel log: " + updateQuery.lastError().text());
    }
}

//-----------------

// Recharge Card -------------------------------

void MainWindow::on_btnRechargeCard_clicked()
{
    QString cardId = QInputDialog::getText(this, "Recharge Card", "Enter your Card ID:");
    if(cardId.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Card ID cannot be empty.");
        return;
    }

    bool ok;
    double amount = QInputDialog::getDouble(this, "Recharge Card", "Enter amount to recharge:", 0, 0, 10000, 2, &ok);
    if(!ok) {
        QMessageBox::warning(this, "Warning", "Invalid recharge amount.");
        return;
    }

    QSqlQuery query;

    // Checking if card exists, aaile lai it exits if we input it in Enter bus

    query.prepare("SELECT balance FROM cards WHERE card_id = ?");
    query.addBindValue(cardId);

    double currentBalance = 0.0;

    if (query.exec() && query.next()) {
        currentBalance = query.value(0).toDouble();

        // Update balance

        double newBalance = currentBalance + amount;
        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE cards SET balance = ? WHERE card_id = ?");
        updateQuery.addBindValue(newBalance);
        updateQuery.addBindValue(cardId);

        if (!updateQuery.exec()) {
            QMessageBox::critical(this, "Error", "Failed to update balance: " + updateQuery.lastError().text());
            return;
        }
    } else {

        // Yei ho it inserts new card with balence first ma ani I think yesai le mathi problem ekchoti garirako thyo cause mathi bata ni I tried to do the same, But feri yo aaileko chalirako cha ---

        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT INTO cards (card_id, balance) VALUES (?, ?)");
        insertQuery.addBindValue(cardId);
        insertQuery.addBindValue(amount);

        if (!insertQuery.exec()) {
            QMessageBox::critical(this, "Error", "Failed to add new card: " + insertQuery.lastError().text());
            return;
        }
    }

    // popup

    QMessageBox::information(this, "Success", QString("Recharged ₨ %1 for Card ID: %2").arg(amount).arg(cardId));
}


// Admin Window

void MainWindow::on_btnAdminPanel_clicked()
{
    AdminPanel admin(this);
    connect(&admin, &AdminPanel::currentCheckpointChanged, this, &MainWindow::updateUpcomingLocation);
    admin.exec();


    QString checkpoint = admin.getCurrentCheckpoint();
    updateUpcomingLocation(checkpoint);
}



// EXIT-----------------------------


void MainWindow::on_btnExit_clicked()
{
    this->close();
}


// Fare Display --------(yo kati ramro sanga bhayo yei mathi hudaina nonsence)--------

void MainWindow::updateFareDisplay(double fareAmount) {
    ui->labelFareDisplay->setText("Fare: ₨ " + QString::number(fareAmount, 'f', 2));
}


// Upcoming Location

void MainWindow::updateUpcomingLocation(const QString &currentCheckpoint)
{
    // Query checkpoints ordered by distance. // don't know simply tanda bhayena signal diyera admin page bata
    QSqlQuery query;
    query.prepare("SELECT checkpoint_name FROM checkpoints ORDER BY distance_from_start ASC");

    if (!query.exec()) {
        qDebug() << "Failed to get checkpoints:" << query.lastError().text();
        return;
    }

    QString nextCheckpoint;
    bool foundCurrent = false;

    while (query.next()) {
        QString checkpoint = query.value(0).toString();
        if (foundCurrent) {
            nextCheckpoint = checkpoint;
            break;
        }
        if (checkpoint == currentCheckpoint) {
            foundCurrent = true;
        }
    }

    if (nextCheckpoint.isEmpty()) {
        nextCheckpoint = "End of route";
    }

    // Ya cai update gareko Mathi dk

    ui->labelUpcomingCheckpoint->setText("Next stop: " + nextCheckpoint);

}

// One of the block of code that cleans the checkpoint cause of tyo hawa error in admin pannel select location option ---- I think I have used this same type of code (block of code that does same thing) multiple time mathi, don't know if this one still works but solved bhayo error , SO NOT GONNA TOUCH THIS ALSO

void MainWindow::cleanCheckpoints(int routeId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM checkpoints WHERE route_id = ?");
    query.addBindValue(routeId);

    if (!query.exec()) {
        qDebug() << "Failed to delete old checkpoints:" << query.lastError().text();
    } else {
        qDebug() << "Old checkpoints cleared for route_id:" << routeId;
    }
}
