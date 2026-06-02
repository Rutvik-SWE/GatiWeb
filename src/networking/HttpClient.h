#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

class HttpClient : public QObject {
    Q_OBJECT

public:
    explicit HttpClient(QObject *parent = nullptr);
    void fetchUrl(const QString &url);

signals:
    void dataReady(const QString &htmlData);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
};

#endif // HTTPCLIENT_H