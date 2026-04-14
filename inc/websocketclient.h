#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QTimer>
#include <QNetworkRequest>
#include <QSslConfiguration>

class WebSocketClient : public QObject {
    Q_OBJECT
public:
    explicit WebSocketClient(const QUrl &url, QObject *parent = nullptr);

    // Методы для отправки данных извне
    void sendText(const QString &message);
    void sendBinary(const QByteArray &data);

    void setupSslConfiguration();

private slots:
    void connectToServer();
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &data); // Новый слот
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
