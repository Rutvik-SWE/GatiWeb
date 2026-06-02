#include "HttpClient.h"
#include <QNetworkRequest>
#include <QUrl>

HttpClient::HttpClient(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &HttpClient::onReplyFinished);
}

void HttpClient::fetchUrl(const QString &urlStr) {
    QUrl url(urlStr);
    if (url.scheme().isEmpty()) {
        url.setScheme("http");
    }

    QNetworkRequest request(url);
    manager->get(request);
}

void HttpClient::onReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QString html = QString::fromUtf8(responseData);
        emit dataReady(html);
    } else {
        QString errorDetail = reply->errorString();
        emit dataReady("Network Error: " + errorDetail);
    }

    reply->deleteLater();
}