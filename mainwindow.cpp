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



    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("smart_bus_fare_system.db");

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

        query.exec("SELECT COUNT(*) FROM routes WHERE route_name = 'Route Ratna Park - Thimi'");
        if (query.next()) {
            int count = query.value(0).toInt();


            int routeId = 0;
            if (count == 0) {

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

    QSqlQuery query;

    query.prepare("SELECT balance FROM cards WHERE card_id = ?");
    query.addBindValue(cardId);

    if (!query.exec()) {
        QMessageBox::critical(this, "Error", "Database error: " + query.lastError().text());
        return;
    }

    if (!query.next()) {


        QSqlQuery insertQuery;  // NEW VARIABLE HERE TO AVOID CONFLICT
        insertQuery.prepare("INSERT INTO cards (card_id, balance) VALUES (?, ?)");
        insertQuery.addBindValue(cardId);
        insertQuery.addBindValue(0.0);
        if (!insertQuery.exec()) {
            QMessageBox::critical(this, "Error", "Failed to create new card: " + insertQuery.lastError().text());
            return;
        }
    }

    // checking to prevent a card from entering again without exiting.
    QSqlQuery activeTripCheck;
    activeTripCheck.prepare("SELECT COUNT(*) FROM travel_log WHERE card_id = ? AND exit_checkpoint IS NULL");
    activeTripCheck.addBindValue(cardId);

    if (!activeTripCheck.exec()) {
        QMessageBox::critical(this, "Error", "Failed to check active trip: " + activeTripCheck.lastError().text());
        return;
    }
    if (activeTripCheck.next() && activeTripCheck.value(0).toInt() > 0) {
        QMessageBox::warning(this, "Warning", "This card has already entered the bus. Please exit before entering again.");
        return;
    }

    // Proceed to log entry if no active trip exists . if driver has not set the location then no log -in.
    QString rawLabelText = ui->labelUpcomingCheckpoint->text();
    qDebug() << "Raw labelUpcomingCheckpoint text:" << rawLabelText;

    QString entryCheckpoint = rawLabelText;
    if (rawLabelText.startsWith("Next stop: ")) {
        entryCheckpoint = rawLabelText.mid(QString("Next stop: ").length()).trimmed();
    }

    qDebug() << "Processed entryCheckpoint text:" << entryCheckpoint;

    if (entryCheckpoint.isEmpty() || entryCheckpoint.compare("End of route", Qt::CaseInsensitive) == 0) {
        QMessageBox::warning(this, "Warning", "No valid upcoming checkpoint is set. Please wait for the next stop.");
        return;
    }

    QString entryTime = QDateTime::currentDateTime().toString(Qt::ISODate);

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

    // balance and remaining balance
    if (updateQuery.exec()) {
        QMessageBox::information(this, "Success",QString("Trip completed.\nFare: ₨ %1\nRemaining Balance: ₨ %2").arg(fare, 0, 'f', 2).arg(newBalance, 0, 'f', 2));
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

    QSqlQuery query;
    query.prepare("SELECT balance FROM cards WHERE card_id = ?");
    query.addBindValue(cardId);

    double currentBalance = 0.0;
    bool cardExists = false;

    if (query.exec() && query.next()) {
        currentBalance = query.value(0).toDouble();
        cardExists = true;

        // Show current balance to user before recharge
        QMessageBox::information(this, "Current Balance", QString("Current balance for Card ID %1 is ₨ %2").arg(cardId).arg(currentBalance, 0, 'f', 2));
    } else {
        // Card does not exist
        QMessageBox::information(this, "Card Not Found", "Card not found. New card will be created on recharge.");
    }

    bool ok;
    double amount = QInputDialog::getDouble(this, "Recharge Card", "Enter amount to recharge:", 0, 0, 10000, 2, &ok);
    if(!ok) {
        QMessageBox::warning(this, "Warning", "Invalid recharge amount.");
        return;
    }

    if(cardExists) {
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
        // Insert new card
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT INTO cards (card_id, balance) VALUES (?, ?)");
        insertQuery.addBindValue(cardId);
        insertQuery.addBindValue(amount);

        if (!insertQuery.exec()) {
            QMessageBox::critical(this, "Error", "Failed to add new card: " + insertQuery.lastError().text());
            return;
        }
    }
    //popup
    QMessageBox::information(this, "Success", QString("Recharged ₨ %1 for Card ID: %2").arg(amount).arg(cardId));
}



// Admin Window

void MainWindow::on_btnAdminPanel_clicked()
{
    AdminPanel admin(this);

    connect(&admin, &AdminPanel::currentCheckpointChanged,this, &MainWindow::updateUpcomingLocation);

    admin.exec();

    // After closing dialog, updating ui manually
    QString checkpoint = admin.getCurrentCheckpoint();
    Direction direction = admin.getCurrentDirection();

    updateUpcomingLocation(checkpoint, direction);
}




// EXIT-----------------------------


void MainWindow::on_btnExit_clicked()
{
    this->close();
}


// Fare Display

void MainWindow::updateFareDisplay(double fareAmount) {
    ui->labelFareDisplay->setText("Fare: ₨ " + QString::number(fareAmount, 'f', 2));
}


// Upcoming Location

void MainWindow::updateUpcomingLocation(const QString &currentCheckpoint, Direction newDirection)
{
    currentDirection = newDirection;
    QSqlQuery query;
    query.prepare("SELECT checkpoint_name FROM checkpoints ORDER BY distance_from_start ASC");

    QStringList checkpoints;
    if (!query.exec()) {
        qDebug() << "Failed to get checkpoints:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        checkpoints.append(query.value(0).toString());
    }

    int currentIndex = checkpoints.indexOf(currentCheckpoint);
    if (currentIndex == -1) {
        qDebug() << "Current checkpoint not found in checkpoints list!";
        return;
    }

    QString nextCheckpoint;

    if (currentDirection == Forward) {
        if (currentIndex == checkpoints.size() - 1) {
            // Reached last checkpoint, reverse direction
            currentDirection = Backward;
            qDebug() << "Reached end, switching direction to Backward";
            nextCheckpoint = (currentIndex - 1 >= 0) ? checkpoints[currentIndex - 1] : "";
        } else {
            nextCheckpoint = checkpoints[currentIndex + 1];
        }
    } else { // Backward
        if (currentIndex == 0) {
            // Reached first checkpoint, reverse direction
            currentDirection = Forward;
            qDebug() << "Reached start, switching direction to Forward";
            nextCheckpoint = (currentIndex + 1 < checkpoints.size()) ? checkpoints[currentIndex + 1] : "";
        } else {
            nextCheckpoint = checkpoints[currentIndex - 1];
        }
    }

    if (nextCheckpoint.isEmpty()) {
        nextCheckpoint = "Last Stop";
    }

    ui->labelUpcomingCheckpoint->setText("Next stop: " + nextCheckpoint);

    // Update direction label
    QString directionText = (currentDirection == Forward) ? "Direction: Ratna Park → Thimi" : "Direction: Thimi → Ratna Park";

    ui->labelDirection->setText(directionText);

    qDebug() << "Current checkpoint:" << currentCheckpoint;
    qDebug() << "Next checkpoint:" << nextCheckpoint;
    qDebug() << "Direction:" << (currentDirection == Forward ? "Forward" : "Backward");
}





// to clean checkpoints
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
