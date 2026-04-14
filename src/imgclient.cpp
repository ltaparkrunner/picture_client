#include "imgclient.h"
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
