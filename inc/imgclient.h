#ifndef IMGCCLIENT_H
#define IMGCCLIENT_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QTimer>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QFile>

class ImageClient : public QObject {
    Q_OBJECT
public:
    explicit ImageClient(const QUrl &url, QObject *parent = nullptr);
    void connectToServer();
    void sendImage(const QString &filePath);
    void getImage(const QString &filePath);
    void setupSslConfiguration();
    void sendText(const QString &message);
    void sendBinary(const QByteArray &data);

private slots:
    void onConnected();
//    void onMessageReceived(const QString &message);
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message);
    void onError(QAbstractSocket::SocketError error);

private:
    QWebSocket m_webSocket;
    QUrl m_url;
    QTimer m_reconnectTimer;
    QTimer m_pingTimer;

    const int RECONNECT_INTERVAL = 5000;
    const int PING_INTERVAL = 30000;
};

#endif
