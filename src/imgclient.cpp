#include "imgclient.h"
#include "pict_data/message.qpb.h"
#include <QDebug>
#include <QFileInfo>
#include <QDateTime>
#include <QtProtobuf/QProtobufSerializer>

ImageClient::ImageClient(const QUrl &url, QObject *parent)
    : QObject(parent)
    , m_url(url)
{
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &ImageClient::connectToServer);
    connect(&m_pingTimer, &QTimer::timeout, [&]() {
        if (m_webSocket.state() == QAbstractSocket::ConnectedState) m_webSocket.ping();
    });
    connect(&m_webSocket, &QWebSocket::connected, this, &ImageClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &ImageClient::onDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &ImageClient::onTextMessageReceived);
    connect(&m_webSocket, &QWebSocket::binaryMessageReceived, this, &ImageClient::onBinaryMessageReceived);
    connect(&m_webSocket, &QWebSocket::errorOccurred, this, &ImageClient::onError);
    connect(&m_webSocket, &QWebSocket::sslErrors, this, [=](const QList<QSslError> &errors) {
        qDebug() << "SSL Errors occurred, ignoring...";
        for (const QSslError &error : errors) {
            qDebug() << "SSL Error:" << error.errorString();
        }
        m_webSocket.ignoreSslErrors(); // Разрешаем соединение
    });
    connectToServer();
}

void ImageClient::connectToServer() {
    qDebug() << "Try to connect";
    QNetworkRequest request(m_url);

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    QList<QSslCertificate> certs = QSslCertificate::fromPath("C:/wrk/nodejs/picture_server/cert.pem");
    config.addCaCertificates(certs);

    m_webSocket.setSslConfiguration(config);
    m_webSocket.ignoreSslErrors(); // Игнорируем ошибки цепочки доверия
    m_webSocket.open(request);
}

void ImageClient::onConnected() {
    qDebug() << "Подключено. Heartbeat запущен.";
    m_reconnectTimer.stop();
    m_pingTimer.start(PING_INTERVAL);
}

void ImageClient::onDisconnected() {
    qDebug() << "Связь потеряна. Ожидание переподключения...";
    m_pingTimer.stop();
    m_reconnectTimer.start(RECONNECT_INTERVAL);
}

// Обработка текста
void ImageClient::onTextMessageReceived(const QString &message) {
    qDebug() << "Текст от сервера:" << message;
}

void ImageClient::sendImage(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open file";
        return;
    }

    QByteArray fileData = file.readAll();
    QFileInfo fileInfo(filePath);

    // ВАЖНО: Если сервер ждет Socket.io пакет, нужно добавить префикс.
    // Если сервер ждет чистый Protobuf через сырой WebSocket:
    // [Здесь должна быть сериализация через сгенерированный Protobuf класс]
    // Для теста отправим просто байты:

    pict_data::ImageUpload message;
    message.setFilename(fileInfo.fileName());
    message.setEmailLogin("forever_young");
    message.setData(fileData.constData());
    message.setContentType("image");
    message.setTimestamp(QDateTime::currentMSecsSinceEpoch());
    QProtobufSerializer serializer;
    QByteArray data = message.serialize(&serializer);
    qint64 sz = m_webSocket.sendBinaryMessage(data);

    qDebug() << "Image sent, size:" << sz;
}

void ImageClient::onBinaryMessageReceived(const QByteArray &message) {
    qDebug() << "Answer from server received";
}

void ImageClient::sendText(const QString &message) {
    if (m_webSocket.state() == QAbstractSocket::ConnectedState) {
        m_webSocket.sendTextMessage(message);
    }
}

void ImageClient::sendBinary(const QByteArray &data) {
    if (m_webSocket.state() == QAbstractSocket::ConnectedState) {
        m_webSocket.sendBinaryMessage(data); // Отправка бинарного пакета
    }
}

void ImageClient::onError(QAbstractSocket::SocketError error) {
    qDebug() << "Ошибка:" << m_webSocket.errorString();
    if (m_webSocket.state() != QAbstractSocket::ConnectedState && !m_reconnectTimer.isActive()) {
        m_reconnectTimer.start(RECONNECT_INTERVAL);
    }
}

void ImageClient::setupSslConfiguration() {
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

/*

to use inside method sendImage


#include "client.h"
#include <QDebug>

ImageClient::ImageClient(QObject *parent) : QObject(parent) {
    connect(&m_webSocket, &QWebSocket::connected, this, &ImageClient::onConnected);
    connect(&m_webSocket, &QWebSocket::binaryMessageReceived, this, &ImageClient::onBinaryMessageReceived);
}

void ImageClient::connectToServer(const QUrl &url) {
    m_webSocket.open(url);
}

void ImageClient::onConnected() {
    qDebug() << "Connected to server!";
}

void ImageClient::sendImage(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open file";
        return;
    }

    QByteArray fileData = file.readAll();

    // ВАЖНО: Если сервер ждет Socket.io пакет, нужно добавить префикс.
    // Если сервер ждет чистый Protobuf через сырой WebSocket:
    // [Здесь должна быть сериализация через сгенерированный Protobuf класс]
    // Для теста отправим просто байты:
    m_webSocket.sendBinaryMessage(fileData);

    qDebug() << "Image sent, size:" << fileData.size();
}

void ImageClient::onBinaryMessageReceived(const QByteArray &message) {
    qDebug() << "Answer from server received";
}


3. Нюанс для Qt (C++)
Теперь в вашем Qt-коде метод sendImage будет работать напрямую:
Используйте m_webSocket.sendBinaryMessage(serializedData).
URL для подключения в Qt будет: ws://localhost:3000.

*/
