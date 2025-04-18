#include "main_window.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), networkThread(new QThread(this)), worker(new NetworkWorker()), requestSuccessful(false) {
    worker->moveToThread(networkThread);
    connect(networkThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &MainWindow::destroyed, networkThread, &QThread::quit);
    connect(this, &MainWindow::sendNetworkRequest, worker, &NetworkWorker::processRequest);
    connect(this, &MainWindow::cancelNetworkRequest, worker, &NetworkWorker::cancelRequest);
    connect(worker, &NetworkWorker::dataReceived, this, &MainWindow::onDataReceived);
    connect(worker, &NetworkWorker::finished, this, &MainWindow::onRequestFinished);
    connect(worker, &NetworkWorker::errorOccurred, this, &MainWindow::onErrorOccurred);
    networkThread->start();

    setupUi();
    connect(addFieldButton, &QPushButton::clicked, this, &MainWindow::addField);
    connect(removeFieldButton, &QPushButton::clicked, this, &MainWindow::removeSelectedField);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendRequest);
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::cancelRequest);
}

MainWindow::~MainWindow() {
    networkThread->quit();
    networkThread->wait();
}

void MainWindow::setupUi() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *tableNameLayout = new QHBoxLayout();
    tableNameLayout->addWidget(new QLabel("Table Name:", this));
    tableNameEdit = new QLineEdit("users", this);
    tableNameEdit->setObjectName("tableNameEdit");
    tableNameLayout->addWidget(tableNameEdit);
    mainLayout->addLayout(tableNameLayout);

    QHBoxLayout *rowsLayout = new QHBoxLayout();
    rowsLayout->addWidget(new QLabel("Rows:", this));
    rowsSpinBox = new QSpinBox(this);
    rowsSpinBox->setObjectName("rowsSpinBox");
    rowsSpinBox->setRange(1, 10000);
    rowsSpinBox->setValue(10);
    rowsLayout->addWidget(rowsSpinBox);
    mainLayout->addLayout(rowsLayout);

    QHBoxLayout *outputFileLayout = new QHBoxLayout();
    outputFileLayout->addWidget(new QLabel("Output File:", this));
    outputFileEdit = new QLineEdit("output.csv", this);
    outputFileEdit->setObjectName("outputFileEdit");
    outputFileLayout->addWidget(outputFileEdit);
    mainLayout->addLayout(outputFileLayout);

    fieldsTable = new QTableWidget(0, 3, this);
    fieldsTable->setObjectName("fieldsTable");
    fieldsTable->setHorizontalHeaderLabels({"Name", "Type", "Parameters"});
    fieldsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    fieldsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(fieldsTable);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addFieldButton = new QPushButton("Add Field", this);
    addFieldButton->setObjectName("addFieldButton");
    removeFieldButton = new QPushButton("Remove Selected Field", this);
    removeFieldButton->setObjectName("removeFieldButton");
    sendButton = new QPushButton("Send Request", this);
    sendButton->setObjectName("sendButton");
    cancelButton = new QPushButton("Cancel Request", this);
    cancelButton->setObjectName("cancelButton");
    cancelButton->setVisible(false);
    buttonLayout->addWidget(addFieldButton);
    buttonLayout->addWidget(removeFieldButton);
    buttonLayout->addWidget(sendButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    resize(600, 400);
}

void MainWindow::addField() {
    int row = fieldsTable->rowCount();
    fieldsTable->insertRow(row);

    auto *nameEdit = new QLineEdit(fieldsTable);
    fieldsTable->setCellWidget(row, 0, nameEdit);

    auto *typeCombo = new QComboBox(fieldsTable);
    typeCombo->addItems({"int", "double", "string", "name"});
    fieldsTable->setCellWidget(row, 1, typeCombo);

    QWidget *paramWidget = createParamsWidget("int", row);
    fieldsTable->setCellWidget(row, 2, paramWidget);

    connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            typeCombo, [this, row]() { updateFieldParams(row); });
}

void MainWindow::removeSelectedField() {
    auto selectedRows = fieldsTable->selectionModel()->selectedRows();
    if (!selectedRows.isEmpty()) {
        int row = selectedRows.first().row();
        for (int col = 0; col < fieldsTable->columnCount(); ++col) {
            QWidget *w = fieldsTable->cellWidget(row, col);
            if (w) {
                fieldsTable->removeCellWidget(row, col);
                w->deleteLater();
            }
        }
        fieldsTable->removeRow(row);
    }
}

QWidget* MainWindow::createParamsWidget(const QString& type, int row) {
    auto *container = new QWidget(fieldsTable);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    if (type == "int" || type == "double") {
        auto *minSpin = new QSpinBox(container);
        minSpin->setRange(-1000000, 1000000);
        minSpin->setValue(1);
        minSpin->setObjectName("min");

        auto *maxSpin = new QSpinBox(container);
        maxSpin->setRange(-1000000, 1000000);
        maxSpin->setValue(100);
        maxSpin->setObjectName("max");

        layout->addWidget(new QLabel("Min:", container));
        layout->addWidget(minSpin);
        layout->addWidget(new QLabel("Max:", container));
        layout->addWidget(maxSpin);
    } else if (type == "string") {
        auto *lengthSpin = new QSpinBox(container);
        lengthSpin->setRange(1, 1000);
        lengthSpin->setValue(10);
        lengthSpin->setObjectName("length");

        layout->addWidget(new QLabel("Length:", container));
        layout->addWidget(lengthSpin);
    } else if (type == "name") {
        layout->addWidget(new QLabel("No parameters", container));
    }

    layout->addStretch();
    return container;
}

void MainWindow::updateFieldParams(int row) {
    auto *typeCombo = qobject_cast<QComboBox*>(fieldsTable->cellWidget(row, 1));
    if (!typeCombo) return;

    QString type = typeCombo->currentText();
    QWidget *oldWidget = fieldsTable->cellWidget(row, 2);
    if (oldWidget) {
        fieldsTable->removeCellWidget(row, 2);
        oldWidget->deleteLater();
    }
    QWidget *newWidget = createParamsWidget(type, row);
    fieldsTable->setCellWidget(row, 2, newWidget);
}

QJsonObject MainWindow::createJsonBody() const {
    QJsonObject json;
    json["table_name"] = tableNameEdit->text();
    json["rows"] = rowsSpinBox->value();
    json["output_file"] = outputFileEdit->text();

    QJsonArray fields;
    for (int row = 0; row < fieldsTable->rowCount(); ++row) {
        auto *nameEdit = qobject_cast<QLineEdit*>(fieldsTable->cellWidget(row, 0));
        auto *typeCombo = qobject_cast<QComboBox*>(fieldsTable->cellWidget(row, 1));
        QWidget *paramsWidget = fieldsTable->cellWidget(row, 2);

        if (!nameEdit || !typeCombo || !paramsWidget) continue;

        QJsonObject field;
        field["name"] = nameEdit->text();
        field["type"] = typeCombo->currentText();

        if (typeCombo->currentText() != "name") {
            QJsonObject params;
            if (typeCombo->currentText() == "int" || typeCombo->currentText() == "double") {
                auto *minSpin = paramsWidget->findChild<QSpinBox*>("min");
                auto *maxSpin = paramsWidget->findChild<QSpinBox*>("max");
                if (minSpin && maxSpin) {
                    params["min"] = QString::number(minSpin->value());
                    params["max"] = QString::number(maxSpin->value());
                }
            } else if (typeCombo->currentText() == "string") {
                auto *lengthSpin = paramsWidget->findChild<QSpinBox*>("length");
                if (lengthSpin) {
                    params["length"] = QString::number(lengthSpin->value());
                }
            }
            field["params"] = params;
        }
        fields.append(field);
    }
    json["fields"] = fields;

    return json;
}

void MainWindow::sendRequest() {
    if (tableNameEdit->text().isEmpty()) {
        showWarning("Input Error", "Table name cannot be empty.");
        return;
    }
    if (fieldsTable->rowCount() == 0) {
        showWarning("Input Error", "At least one field is required.");
        return;
    }
    for (int row = 0; row < fieldsTable->rowCount(); ++row) {
        auto *nameEdit = qobject_cast<QLineEdit*>(fieldsTable->cellWidget(row, 0));
        if (!nameEdit || nameEdit->text().isEmpty()) {
            showWarning("Input Error", QString("Field name in row %1 cannot be empty.").arg(row + 1));
            return;
        }
    }

    QJsonObject json = createJsonBody();
    QJsonDocument doc(json);
    QByteArray jsonData = doc.toJson();

    QNetworkRequest request(QUrl("http://localhost:8080/generate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    responseData.clear();
    requestSuccessful = false; // Сбрасываем флаг успеха
    emit sendNetworkRequest(request, jsonData);

    sendButton->setText("Processing...");
    sendButton->setEnabled(false);
    cancelButton->setVisible(true);
}

void MainWindow::cancelRequest() {
    emit cancelNetworkRequest();
    sendButton->setText("Send Request");
    sendButton->setEnabled(true);
    cancelButton->setVisible(false);
    requestSuccessful = false; // Запрос неуспешен
    responseData.clear();
    showInformation("Cancelled", "Request has been cancelled.");
}

void MainWindow::onDataReceived(const QByteArray &data) {
    responseData.append(data);
    requestSuccessful = true; // Помечаем запрос как успешный при получении данных
}

void MainWindow::onRequestFinished() {
    sendButton->setText("Send Request");
    sendButton->setEnabled(true);
    cancelButton->setVisible(false);

    if (!requestSuccessful || responseData.isEmpty()) {
        // Не показываем QFileDialog при ошибке или отмене
        responseData.clear();
        return;
    }

    QString fileName = getSaveFileName("Save CSV File", outputFileEdit->text(), "CSV Files (*.csv)");
    if (fileName.isEmpty()) {
        responseData.clear();
        return;
    }

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(responseData);
        file.close();
        showInformation("Success", "CSV file saved successfully!");
    } else {
        showCritical("File Error", "Failed to save CSV file: " + file.errorString());
    }

    responseData.clear();
}

void MainWindow::onErrorOccurred(const QString &error) {
    sendButton->setText("Send Request");
    sendButton->setEnabled(true);
    cancelButton->setVisible(false);
    requestSuccessful = false; // Запрос неуспешен
    showCritical("Network Error", error);
    responseData.clear();
}