#pragma once

#include "network_worker.h"

#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>

class QNetworkRequest;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QJsonObject createJsonBody() const;

protected:
    virtual QString getSaveFileName(const QString& caption, const QString& dir, const QString& filter) {
        return QFileDialog::getSaveFileName(this, caption, dir, filter);
    }
    virtual void showWarning(const QString& title, const QString& text) {
        QMessageBox::warning(this, title, text);
    }
    virtual void showCritical(const QString& title, const QString& text) {
        QMessageBox::critical(this, title, text);
    }
    virtual void showInformation(const QString& title, const QString& text) {
        QMessageBox::information(this, title, text);
    }

    signals:
        void sendNetworkRequest(const QNetworkRequest &request, const QByteArray &data);
    void cancelNetworkRequest();

    private slots:
        void addField();
    void removeSelectedField();
    void sendRequest();
    void cancelRequest();
    void onDataReceived(const QByteArray &data);
    void onRequestFinished();
    void onErrorOccurred(const QString &error);
    void updateFieldParams(int row);

private:
    QLineEdit *tableNameEdit;
    QSpinBox *rowsSpinBox;
    QLineEdit *outputFileEdit;
    QTableWidget *fieldsTable;
    QPushButton *addFieldButton;
    QPushButton *removeFieldButton;
    QPushButton *sendButton;
    QPushButton *cancelButton;
    QThread *networkThread;
    NetworkWorker *worker;
    QByteArray responseData;
    bool requestSuccessful; // Флаг для отслеживания успешности запроса

    void setupUi();
    QWidget* createParamsWidget(const QString& type, int row);
};