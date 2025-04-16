#include <QtTest/QtTest>
#include <QLabel>
#include "../src/main_window.h"

class MockNetworkReply : public QNetworkReply {
public:
    MockNetworkReply(QObject *parent = nullptr) : QNetworkReply(parent) {}
    void setHttpStatusCode(int code) {
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, code);
    }
    void setRawData(const QByteArray &data) {
        rawData = data;
        open(ReadOnly);
    }
    qint64 readData(char *data, qint64 maxSize) override {
        qint64 len = qMin(maxSize, qint64(rawData.size()));
        memcpy(data, rawData.constData(), len);
        rawData.remove(0, len);
        return len;
    }
    qint64 bytesAvailable() const override {
        return rawData.size() + QIODevice::bytesAvailable();
    }
    void emitFinished() {
        emit finished();
    }
    void abort() override {
        setError(OperationCanceledError, "Operation canceled");
        emit finished();
    }

private:
    QByteArray rawData;
};

class MockNetworkAccessManager : public QNetworkAccessManager {
public:
    MockNetworkAccessManager(QObject *parent = nullptr) : QNetworkAccessManager(parent) {}
    ~MockNetworkAccessManager() {
        qDeleteAll(replies);
        replies.clear();
    }
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = nullptr) override {
        lastRequest = req;
        lastData = outgoingData ? outgoingData->readAll() : QByteArray();
        auto reply = new MockNetworkReply(this);
        replies.append(reply);
        return reply;
    }
    void clearReplies() {
        qDeleteAll(replies);
        replies.clear();
    }
    QNetworkRequest lastRequest;
    QByteArray lastData;
    QList<MockNetworkReply*> replies;
};

class TestMainWindow : public MainWindow {
public:
    TestMainWindow(QWidget *parent = nullptr) : MainWindow(parent) {}
    QString getSaveFileName(const QString&, const QString& dir, const QString&) override {
        return dir;
    }
    void showWarning(const QString& title, const QString& text) override {
        lastMessage = {title, text, "warning"};
    }
    void showCritical(const QString& title, const QString& text) override {
        lastMessage = {title, text, "critical"};
    }
    void showInformation(const QString& title, const QString& text) override {
        lastMessage = {title, text, "information"};
    }
    struct Message {
        QString title;
        QString text;
        QString type;
    };
    Message lastMessage;
};

class TestMainWindowTests : public QObject {
    Q_OBJECT

private slots:
    void cleanup() {
        QCoreApplication::processEvents();
    }

    void testUiInitialization() {
        TestMainWindow w;
        QTest::qWait(100);

        QLineEdit *tableNameEdit = w.findChild<QLineEdit*>("tableNameEdit");
        QVERIFY2(tableNameEdit, "Table name QLineEdit not found");
        QCOMPARE(tableNameEdit->text(), QString("users"));

        QSpinBox *rowsSpinBox = w.findChild<QSpinBox*>("rowsSpinBox");
        QVERIFY2(rowsSpinBox, "Rows QSpinBox not found");
        QCOMPARE(rowsSpinBox->value(), 10);

        QLineEdit *outputFileEdit = w.findChild<QLineEdit*>("outputFileEdit");
        QVERIFY2(outputFileEdit, "Output file QLineEdit not found");
        QCOMPARE(outputFileEdit->text(), QString("output.csv"));

        QTableWidget *fieldsTable = w.findChild<QTableWidget*>("fieldsTable");
        QVERIFY2(fieldsTable, "Fields QTableWidget not found");
        QCOMPARE(fieldsTable->rowCount(), 0);
        QCOMPARE(fieldsTable->columnCount(), 3);
        QCOMPARE(fieldsTable->horizontalHeaderItem(0)->text(), QString("Name"));
        QCOMPARE(fieldsTable->horizontalHeaderItem(1)->text(), QString("Type"));
        QCOMPARE(fieldsTable->horizontalHeaderItem(2)->text(), QString("Parameters"));

        QPushButton *addFieldButton = w.findChild<QPushButton*>("addFieldButton");
        QPushButton *removeFieldButton = w.findChild<QPushButton*>("removeFieldButton");
        QPushButton *sendButton = w.findChild<QPushButton*>("sendButton");
        QVERIFY2(addFieldButton, "Add Field button not found");
        QVERIFY2(removeFieldButton, "Remove Selected Field button not found");
        QVERIFY2(sendButton, "Send Request button not found");
    }

    void testAddField() {
        TestMainWindow w;
        QPushButton *addFieldButton = w.findChild<QPushButton*>("addFieldButton");
        QTableWidget *fieldsTable = w.findChild<QTableWidget*>("fieldsTable");

        QTest::mouseClick(addFieldButton, Qt::LeftButton);
        QTest::qWait(100);
        QCOMPARE(fieldsTable->rowCount(), 1);

        QVERIFY2(qobject_cast<QLineEdit*>(fieldsTable->cellWidget(0, 0)), "Name QLineEdit not found in row 0");
        QComboBox *typeCombo = qobject_cast<QComboBox*>(fieldsTable->cellWidget(0, 1));
        QVERIFY2(typeCombo, "Type QComboBox not found in row 0");
        QCOMPARE(typeCombo->count(), 4);
        QCOMPARE(typeCombo->itemText(0), QString("int"));
        QVERIFY2(fieldsTable->cellWidget(0, 2)->findChild<QSpinBox*>("min"), "Min QSpinBox not found for int");
        QVERIFY2(fieldsTable->cellWidget(0, 2)->findChild<QSpinBox*>("max"), "Max QSpinBox not found for int");
    }

    void testRemoveField() {
        TestMainWindow w;
        QPushButton *addFieldButton = w.findChild<QPushButton*>("addFieldButton");
        QPushButton *removeFieldButton = w.findChild<QPushButton*>("removeFieldButton");
        QTableWidget *fieldsTable = w.findChild<QTableWidget*>("fieldsTable");

        QTest::mouseClick(addFieldButton, Qt::LeftButton);
        QTest::qWait(100);
        QCOMPARE(fieldsTable->rowCount(), 1);

        fieldsTable->selectRow(0);
        QTest::mouseClick(removeFieldButton, Qt::LeftButton);
        QTest::qWait(100);
        QCOMPARE(fieldsTable->rowCount(), 0);
    }

    void testFieldTypeChange() {
        TestMainWindow w;
        QPushButton *addFieldButton = w.findChild<QPushButton*>("addFieldButton");
        QTableWidget *fieldsTable = w.findChild<QTableWidget*>("fieldsTable");

        QTest::mouseClick(addFieldButton, Qt::LeftButton);
        QTest::qWait(100);

        QComboBox *typeCombo = qobject_cast<QComboBox*>(fieldsTable->cellWidget(0, 1));
        QVERIFY2(typeCombo, "Type QComboBox not found");
        QCOMPARE(typeCombo->currentText(), QString("int"));

        QSignalSpy comboSpy(typeCombo, &QComboBox::currentIndexChanged);
        typeCombo->setCurrentText("int");
        QTest::qWait(100);
        QVERIFY2(comboSpy.count() == 0, "Unexpected signal for same value");
        QVERIFY2(fieldsTable->cellWidget(0, 2)->findChild<QSpinBox*>("min"), "Min QSpinBox not found for int");
        QVERIFY2(fieldsTable->cellWidget(0, 2)->findChild<QSpinBox*>("max"), "Max QSpinBox not found for int");

        typeCombo->setCurrentText("double");
        QTest::qWait(100);
        QVERIFY2(comboSpy.count() == 1, "Expected signal for double");
        QVERIFY2(fieldsTable->cellWidget(0, 2)->findChild<QSpinBox*>("min"), "Min QSpinBox not found for double");
        QVERIFY2(fieldsTable->cellWidget(0, 2)->findChild<QSpinBox*>("max"), "Max QSpinBox not found for double");

        typeCombo->setCurrentText("string");
        QTest::qWait(100);
        QVERIFY2(comboSpy.count() == 2, "Expected signal for string");
        QVERIFY2(fieldsTable->cellWidget(0, 2)->findChild<QSpinBox*>("length"), "Length QSpinBox not found for string");
        QVERIFY2(!fieldsTable->cellWidget(0, 2)->findChild<QSpinBox*>("min"), "Min QSpinBox should not exist for string");

        typeCombo->setCurrentText("name");
        QTest::qWait(100);
        QVERIFY2(comboSpy.count() == 3, "Expected signal for name");
        QVERIFY2(fieldsTable->cellWidget(0, 2)->findChild<QLabel*>()->text() == "No parameters", "No parameters label not found for name");
        QVERIFY2(!fieldsTable->cellWidget(0, 2)->findChild<QSpinBox*>(), "No QSpinBox should exist for name");
    }

    void testJsonBodyGeneration() {
        TestMainWindow w;
        QPushButton *addFieldButton = w.findChild<QPushButton*>("addFieldButton");
        QTableWidget *fieldsTable = w.findChild<QTableWidget*>("fieldsTable");
        QLineEdit *tableNameEdit = w.findChild<QLineEdit*>("tableNameEdit");
        QSpinBox *rowsSpinBox = w.findChild<QSpinBox*>("rowsSpinBox");
        QLineEdit *outputFileEdit = w.findChild<QLineEdit*>("outputFileEdit");

        tableNameEdit->setText("users");
        rowsSpinBox->setValue(10);
        outputFileEdit->setText("users.csv");

        QTest::mouseClick(addFieldButton, Qt::LeftButton);
        QTest::qWait(100);
        QTest::mouseClick(addFieldButton, Qt::LeftButton);
        QTest::qWait(100);

        QLineEdit *nameEdit1 = qobject_cast<QLineEdit*>(fieldsTable->cellWidget(0, 0));
        QComboBox *typeCombo1 = qobject_cast<QComboBox*>(fieldsTable->cellWidget(0, 1));
        QSpinBox *minSpin1 = fieldsTable->cellWidget(0, 2)->findChild<QSpinBox*>("min");
        QSpinBox *maxSpin1 = fieldsTable->cellWidget(0, 2)->findChild<QSpinBox*>("max");
        nameEdit1->setText("id");
        typeCombo1->setCurrentText("int");
        minSpin1->setValue(1);
        maxSpin1->setValue(1000);

        QLineEdit *nameEdit2 = qobject_cast<QLineEdit*>(fieldsTable->cellWidget(1, 0));
        QComboBox *typeCombo2 = qobject_cast<QComboBox*>(fieldsTable->cellWidget(1, 1));
        QSignalSpy comboSpy(typeCombo2, &QComboBox::currentIndexChanged);
        typeCombo2->setCurrentText("string");
        QTest::qWait(100);
        QVERIFY2(comboSpy.count() == 1, "Expected signal for string");
        QSpinBox *lengthSpin2 = fieldsTable->cellWidget(1, 2)->findChild<QSpinBox*>("length");
        nameEdit2->setText("name");
        lengthSpin2->setValue(10);

        QJsonObject json = w.createJsonBody();
        QJsonDocument doc(json);
        QString jsonStr = QString(doc.toJson(QJsonDocument::Compact));

        QString expectedJson = R"({"fields":[{"name":"id","params":{"max":"1000","min":"1"},"type":"int"},{"name":"name","params":{"length":"10"},"type":"string"}],"output_file":"users.csv","rows":10,"table_name":"users"})";
        QCOMPARE(jsonStr, expectedJson);
    }

    void testInputValidation() {
        TestMainWindow w;
        MockNetworkAccessManager *mockManager = new MockNetworkAccessManager(&w);
        w.setNetworkManager(mockManager);

        QPushButton *sendButton = w.findChild<QPushButton*>("sendButton");
        QLineEdit *tableNameEdit = w.findChild<QLineEdit*>("tableNameEdit");
        QPushButton *addFieldButton = w.findChild<QPushButton*>("addFieldButton");

        tableNameEdit->setText("");
        QTest::mouseClick(sendButton, Qt::LeftButton);
        QTest::qWait(100);
        QCOMPARE(mockManager->replies.size(), 0);
        QCOMPARE(w.lastMessage.title, QString("Input Error"));
        QCOMPARE(w.lastMessage.text, QString("Table name cannot be empty."));
        QCOMPARE(w.lastMessage.type, QString("warning"));

        tableNameEdit->setText("users");
        w.lastMessage = {};
        QTest::mouseClick(sendButton, Qt::LeftButton);
        QTest::qWait(100);
        QCOMPARE(mockManager->replies.size(), 0);
        QCOMPARE(w.lastMessage.title, QString("Input Error"));
        QCOMPARE(w.lastMessage.text, QString("At least one field is required."));
        QCOMPARE(w.lastMessage.type, QString("warning"));

        QTest::mouseClick(addFieldButton, Qt::LeftButton);
        QTest::qWait(100);
        w.lastMessage = {};
        QTest::mouseClick(sendButton, Qt::LeftButton);
        QTest::qWait(100);
        QCOMPARE(mockManager->replies.size(), 0);
        QCOMPARE(w.lastMessage.title, QString("Input Error"));
        QCOMPARE(w.lastMessage.text, QString("Field name in row 1 cannot be empty."));
        QCOMPARE(w.lastMessage.type, QString("warning"));

        mockManager->clearReplies();
        delete mockManager;
    }

private:
    QApplication *app = nullptr;
};

QTEST_MAIN(TestMainWindowTests)
#include "main_window_test.moc"