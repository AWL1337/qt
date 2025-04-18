#pragma once

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QTableWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    QJsonObject createJsonBody() const;
    void setNetworkManager(QNetworkAccessManager *manager) {
        networkManager = manager;
    }

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

    private slots:
        void addField();
        void removeSelectedField();
        void sendRequest();
        void cancelRequest();
        void onReplyFinished(QNetworkReply *reply);
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
    QNetworkAccessManager *networkManager;
    QNetworkReply *currentReply;

    void setupUi();
    QWidget* createParamsWidget(const QString& type, int row);
};