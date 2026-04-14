#ifndef IMGCCLIENT_H
#define IMGCCLIENT_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QFile>

class ImageClient : public QObject {
    Q_OBJECT
public:
    explicit ImageClient(QObject *parent = nullptr);
    void connectToServer(const QUrl &url);
    void sendImage(const QString &filePath);

private slots:
    void onConnected();
//    void onMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message);

private:
    QWebSocket m_webSocket;
};

#endif
