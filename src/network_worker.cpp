#include "network_worker.h"

NetworkWorker::NetworkWorker(QObject *parent) : QObject(parent) {
    m_qnam.reset(new QNetworkAccessManager(this));
}

NetworkWorker::~NetworkWorker() {
    if (m_reply) {
        m_reply->abort();
    }
}

void NetworkWorker::processRequest(const QNetworkRequest &request, const QByteArray &data) {
    m_reply.reset();
    m_reply.reset(m_qnam->post(request, data));
    connect(m_reply.get(), &QNetworkReply::readyRead, this, &NetworkWorker::onReadyRead);
    connect(m_reply.get(), &QNetworkReply::finished, this, &NetworkWorker::onFinished);
}

void NetworkWorker::cancelRequest() {
    if (m_reply) {
        m_reply->abort();
    }
}

void NetworkWorker::onReadyRead() {
    emit dataReceived(m_reply->readAll());
}

void NetworkWorker::onFinished() {
    if (m_reply->error() == QNetworkReply::OperationCanceledError) {
        emit finished();
        return;
    }
    if (m_reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(m_reply->errorString());
    }
    emit finished();
}