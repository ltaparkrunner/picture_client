#include "WebSocketClient.h"
#include <QFile>
#include <QDebug>

WebSocketClient::WebSocketClient(const QUrl &url, QObject *parent)
    : QObject(parent), m_url(url) {

    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &WebSocketClient::connectToServer);

    connect(&m_pingTimer, &QTimer::timeout, [&]() {
        if (m_webSocket.state() == QAbstractSocket::ConnectedState) m_webSocket.ping();
    });

    // Соединяем сигналы
    connect(&m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    connect(&m_webSocket, &QWebSocket::binaryMessageReceived, this, &WebSocketClient::onBinaryMessageReceived); // Бинарка
    connect(&m_webSocket, &QWebSocket::errorOccurred, this, &WebSocketClient::onError);
    connect(&m_webSocket, &QWebSocket::sslErrors, this, [=](const QList<QSslError> &errors) {
        qDebug() << "SSL Errors occurred, ignoring...";
        for (const QSslError &error : errors) {
            qDebug() << "SSL Error:" << error.errorString();
        }
        m_webSocket.ignoreSslErrors(); // Разрешаем соединение
    });
    // connect(&m_webSocket, &QWebSocket::sslErrors, this, [=](const QList<QSslError> &errors) {
    //     m_webSocket.ignoreSslErrors();
    // });

    qDebug() << "Поддерживает ли Qt SSL?" << QSslSocket::supportsSsl();
    qDebug() << "Версия OpenSSL при сборке:" << QSslSocket::sslLibraryBuildVersionString();
    qDebug() << "Версия OpenSSL в системе:" << QSslSocket::sslLibraryVersionString();

    connectToServer();
}

void WebSocketClient::connectToServer() {
    qDebug() << "Try to connect";
    QNetworkRequest request(m_url);
    // request.setRawHeader("Authorization", "Bearer my_token_123");
    /////////////////////////////////////////
    // QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    // config.setProtocol(QSsl::TlsV1_2OrLater); // Совместимость с большинством серверов
    // config.setPeerVerifyMode(QSslSocket::VerifyNone); // Отключаем проверку для теста

    // m_webSocket.setSslConfiguration(config);

    // QSslConfiguration config = m_webSocket.sslConfiguration();
    // // Отключаем проверку того, что сертификат выписан именно на "localhost"
    // config.setPeerVerifyMode(QSslSocket::VerifyNone);
    // // Это критично для SChannel при работе с самоподписанными сертификатами
    // config.setPeerVerifyDepth(0);

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    QList<QSslCertificate> certs = QSslCertificate::fromPath("C:/wrk/nodejs/picture_server/cert.pem");
    config.addCaCertificates(certs);

    m_webSocket.setSslConfiguration(config);
    m_webSocket.ignoreSslErrors(); // Игнорируем ошибки цепочки доверия
    ////////////////////////////////////
    qDebug() << "Before an attempt to open wss connection";
    m_webSocket.open(request);
    qDebug() << "After an attempt to open wss connection";
}

void WebSocketClient::onConnected() {
    qDebug() << "Подключено. Heartbeat запущен.";
    m_reconnectTimer.stop();
    m_pingTimer.start(PING_INTERVAL);
}

void WebSocketClient::onDisconnected() {
    qDebug() << "Связь потеряна. Ожидание переподключения...";
    m_pingTimer.stop();
    m_reconnectTimer.start(RECONNECT_INTERVAL);
}

// Обработка текста
void WebSocketClient::onTextMessageReceived(const QString &message) {
    qDebug() << "Текст от сервера:" << message;
}

// Обработка бинарных данных (файлы, пакеты и т.д.)
void WebSocketClient::onBinaryMessageReceived(const QByteArray &data) {
    qDebug() << "Получено бинарных данных, байт:" << data.size();
    // Здесь можно сохранить в файл или распарсить структуру
}

void WebSocketClient::sendText(const QString &message) {
    if (m_webSocket.state() == QAbstractSocket::ConnectedState) {
        m_webSocket.sendTextMessage(message);
    }
}

void WebSocketClient::sendBinary(const QByteArray &data) {
    if (m_webSocket.state() == QAbstractSocket::ConnectedState) {
        m_webSocket.sendBinaryMessage(data); // Отправка бинарного пакета
    }
}

void WebSocketClient::onError(QAbstractSocket::SocketError error) {
    qDebug() << "Ошибка:" << m_webSocket.errorString();
    if (m_webSocket.state() != QAbstractSocket::ConnectedState && !m_reconnectTimer.isActive()) {
        m_reconnectTimer.start(RECONNECT_INTERVAL);
    }
}

void WebSocketClient::setupSslConfiguration() {
    QSslConfiguration config = m_webSocket.sslConfiguration();

    // Загружаем ваш файл сертификата (.crt или .pem)
    QFile certFile(":/server_cert.crt");
    if (certFile.open(QIODevice::ReadOnly)) {
        QSslCertificate cert(&certFile, QSsl::Pem);

        // Добавляем его в список доверенных
        QList<QSslCertificate> caCerts = config.caCertificates();
        caCerts.append(cert);
        config.setCaCertificates(caCerts);

        m_webSocket.setSslConfiguration(config);
    }
}
