#include "imgclient.h"
#include "pict_data/message.qpb.h"
#include <QDebug>
#include <QFileInfo>
#include <QDateTime>
#include <QtProtobuf/QProtobufSerializer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>

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
    qDebug() << "fileData.size()" << fileData.size();
    QFileInfo fileInfo(filePath);

    // ВАЖНО: Если сервер ждет Socket.io пакет, нужно добавить префикс.
    // Если сервер ждет чистый Protobuf через сырой WebSocket:
    // [Здесь должна быть сериализация через сгенерированный Protobuf класс]
    // Для теста отправим просто байты:

    pict_data::BaseMessage base;
    pict_data::Picture message;
    message.setFilename(fileInfo.fileName());
    message.setEmailLogin("forever_young");
    message.setData(fileData);
    message.setContentType("image_1");
    message.setTimestamp(QDateTime::currentMSecsSinceEpoch());

    base.setPict(message);
    QProtobufSerializer serializer;
    QByteArray data = base.serialize(&serializer);
    qint64 sz = m_webSocket.sendBinaryMessage(data);

    qDebug() << "Image sent, size:" << sz;
}

// void ImageClient::onBinaryMessageReceived(const QByteArray &message) {
//     qDebug() << "Answer from server received";
// }

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

void ImageClient::requestInitialImages(){
    pict_data::BaseMessage base;
    pict_data::ImageListRequest message;
    message.setCount(6);

    base.setListRequest(message);
    QProtobufSerializer serializer;
    QByteArray data = base.serialize(&serializer);
    qint64 sz = m_webSocket.sendBinaryMessage(data);

    qDebug() << "Image sent, size:" << sz;
}

// В слоте binaryMessageReceived
void ImageClient::onBinaryMessageReceived(const QByteArray &data) {
    qDebug() << "ImageClient::onBinaryMessageReceived(const QByteArray &data)";
    pict_data::BaseMessage base;
    QProtobufSerializer serializer;
    qDebug() << "ImageClient::onBinaryMessageReceived(const QByteArray &data) 2";
    if (!base.deserialize(&serializer, data)) return;
    qDebug() << "ImageClient::onBinaryMessageReceived(const QByteArray &data) 3";
    if (base.contentField() == pict_data::BaseMessage::ContentFields::ListResponse) {
        qDebug() << "ImageClient::onBinaryMessageReceived(const QByteArray &data) 4";
        const auto &response = base.listResponse();
        for (const auto &info : response.images()) {
            qDebug() << "ImageClient::onBinaryMessageReceived(const QByteArray &data) 5";
            downloadImage2(info.url());
        }
    }
    qDebug() << "ImageClient::onBinaryMessageReceived(const QByteArray &data) 6";
}

void ImageClient::downloadImage(const QString &urlStr) {
    qDebug() << "ImageClient::downloadImage(const QString &url) 1" << urlStr;
    QUrl url(urlStr);
    // QString originalHost = url.host();
    // if (url.port() != -1) {
    //     originalHost += ":" + QString::number(url.port());
    // }
    // url.setHost("127.0.0.1");
    QUrl url2("http://localhost:9001/api/v1/download-shared-object/aHR0cDovLzEyNy4wLjAuMTo5MDAwL2ltYWdlcy9pbWcyMS5qcGc_WC1BbXotQWxnb3JpdGhtPUFXUzQtSE1BQy1TSEEyNTYmWC1BbXotQ3JlZGVudGlhbD1QMTM2SUNYNTJaQVhITUZWNjZRVSUyRjIwMjYwNDE2JTJGdXMtZWFzdC0xJTJGczMlMkZhd3M0X3JlcXVlc3QmWC1BbXotRGF0ZT0yMDI2MDQxNlQxNDM2MjBaJlgtQW16LUV4cGlyZXM9NDMxODUmWC1BbXotU2VjdXJpdHktVG9rZW49ZXlKaGJHY2lPaUpJVXpVeE1pSXNJblI1Y0NJNklrcFhWQ0o5LmV5SmhZMk5sYzNOTFpYa2lPaUpRTVRNMlNVTllOVEphUVZoSVRVWldOalpSVlNJc0ltVjRjQ0k2TVRjM05qTTVNREE0TUN3aWNHRnlaVzUwSWpvaVlXUnRhVzRpZlEuZFB6czVGcFhiR19fdUVCUzhNLXFkdG1xSWhDSEZZeVRJdkwyOW5TMHI1MFZIT2hjdjRzOFRyS1BLdk5aMXNRNDRBMFNFMkdqTVZzU01BR2JSOGNlbncmWC1BbXotU2lnbmVkSGVhZGVycz1ob3N0JnZlcnNpb25JZD1udWxsJlgtQW16LVNpZ25hdHVyZT1iY2U2Njc1Yjc4NTc1MTM2NGU5ZmVmNzRlYzU5NGZlOTc3ZWU2NTJjODQxZjBjZTFmMTRhMTA4Y2ZjODAzYTZi");
    QNetworkRequest request(url2);
//    request.setRawHeader("Host", originalHost.toUtf8());
    qDebug() << "Запрос отправлен на:" << url.toString();
//    qDebug() << "С заголовком Host:" << originalHost;

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    manager->get(request);

    qDebug() << "ImageClient::downloadImage(const QString &url) 2" << url;
    connect(manager, &QNetworkAccessManager::finished, [this, manager](QNetworkReply *reply) {
        qDebug() << "ImageClient::downloadImage(const QString &url): ";
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "ImageClient::downloadImage(const QString &url) 4";
            QPixmap pix;
            qint64 len = pix.loadFromData(reply->readAll());
            qDebug() << "ImageClient::downloadImage(const QString &url) 5 : " << len;
            emit imageReady(pix); // Отправляем в UI
            qDebug() << "ImageClient::downloadImage(const QString &url) 6";
        }
        else{
            qDebug() << "ImageClient::downloadImage error" << reply->error();
        }
        qDebug() << "ImageClient::downloadImage(const QString &url) 7";
        reply->deleteLater();
        manager->deleteLater();
    });
    qDebug() << "ImageClient::downloadImage(const QString &url) 8";
    manager->get(QNetworkRequest(QUrl(url)));
    qDebug() << "ImageClient::downloadImage(const QString &url) 9";
}

void ImageClient::downloadImage2(const QString &url) {
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
//    manager->get(QNetworkRequest(QUrl("http://localhost:9001/api/v1/download-shared-object/aHR0cDovLzEyNy4wLjAuMTo5MDAwL2ltYWdlcy9pbWcyMS5qcGc_WC1BbXotQWxnb3JpdGhtPUFXUzQtSE1BQy1TSEEyNTYmWC1BbXotQ3JlZGVudGlhbD1QMTM2SUNYNTJaQVhITUZWNjZRVSUyRjIwMjYwNDE2JTJGdXMtZWFzdC0xJTJGczMlMkZhd3M0X3JlcXVlc3QmWC1BbXotRGF0ZT0yMDI2MDQxNlQxNDM2MjBaJlgtQW16LUV4cGlyZXM9NDMxODUmWC1BbXotU2VjdXJpdHktVG9rZW49ZXlKaGJHY2lPaUpJVXpVeE1pSXNJblI1Y0NJNklrcFhWQ0o5LmV5SmhZMk5sYzNOTFpYa2lPaUpRTVRNMlNVTllOVEphUVZoSVRVWldOalpSVlNJc0ltVjRjQ0k2TVRjM05qTTVNREE0TUN3aWNHRnlaVzUwSWpvaVlXUnRhVzRpZlEuZFB6czVGcFhiR19fdUVCUzhNLXFkdG1xSWhDSEZZeVRJdkwyOW5TMHI1MFZIT2hjdjRzOFRyS1BLdk5aMXNRNDRBMFNFMkdqTVZzU01BR2JSOGNlbncmWC1BbXotU2lnbmVkSGVhZGVycz1ob3N0JnZlcnNpb25JZD1udWxsJlgtQW16LVNpZ25hdHVyZT1iY2U2Njc1Yjc4NTc1MTM2NGU5ZmVmNzRlYzU5NGZlOTc3ZWU2NTJjODQxZjBjZTFmMTRhMTA4Y2ZjODAzYTZi")));
    connect(manager, &QNetworkAccessManager::finished, [this, manager](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "ImageClient::downloadImage2 error NoError" ;
            QPixmap pix;
            pix.loadFromData(reply->readAll());
            emit imageReady(pix); // Отправляем в UI
        }
        else{
            qDebug() << "ImageClient::downloadImage2 error" << reply->error();
        }
        reply->deleteLater();
        manager->deleteLater();
    });
    manager->get(QNetworkRequest(QUrl(url)));
//    manager->get(QNetworkRequest(QUrl("http://localhost:9000/images/img21.jpg")));
    qDebug() << "ImageClient::downloadImage2 the URL is: " << url;
//    manager->get(QNetworkRequest(QUrl("http://localhost:9001/api/v1/download-shared-object/aHR0cDovLzEyNy4wLjAuMTo5MDAwL2ltYWdlcy9pbWcyMS5qcGc_WC1BbXotQWxnb3JpdGhtPUFXUzQtSE1BQy1TSEEyNTYmWC1BbXotQ3JlZGVudGlhbD1QMTM2SUNYNTJaQVhITUZWNjZRVSUyRjIwMjYwNDE2JTJGdXMtZWFzdC0xJTJGczMlMkZhd3M0X3JlcXVlc3QmWC1BbXotRGF0ZT0yMDI2MDQxNlQxNDM2MjBaJlgtQW16LUV4cGlyZXM9NDMxODUmWC1BbXotU2VjdXJpdHktVG9rZW49ZXlKaGJHY2lPaUpJVXpVeE1pSXNJblI1Y0NJNklrcFhWQ0o5LmV5SmhZMk5sYzNOTFpYa2lPaUpRTVRNMlNVTllOVEphUVZoSVRVWldOalpSVlNJc0ltVjRjQ0k2TVRjM05qTTVNREE0TUN3aWNHRnlaVzUwSWpvaVlXUnRhVzRpZlEuZFB6czVGcFhiR19fdUVCUzhNLXFkdG1xSWhDSEZZeVRJdkwyOW5TMHI1MFZIT2hjdjRzOFRyS1BLdk5aMXNRNDRBMFNFMkdqTVZzU01BR2JSOGNlbncmWC1BbXotU2lnbmVkSGVhZGVycz1ob3N0JnZlcnNpb25JZD1udWxsJlgtQW16LVNpZ25hdHVyZT1iY2U2Njc1Yjc4NTc1MTM2NGU5ZmVmNzRlYzU5NGZlOTc3ZWU2NTJjODQxZjBjZTFmMTRhMTA4Y2ZjODAzYTZi")));
}
