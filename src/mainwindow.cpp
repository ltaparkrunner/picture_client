#include "mainwindow.h"
#include "../forms/ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , iwsc(new ImageClient(QUrl("wss://localhost:8080"), this))
{
    ui->setupUi(this);
    this->setStyleSheet(
        "ClickableLabel {"
        "  border: 2px solid #dcdcdc;" // Серая рамка по умолчанию
        "  border-radius: 5px;"        // Скругленные углы
        "  padding: 2px;"
        "}"
        "ClickableLabel:hover {"      // Подсветка при наведении
        "  border: 2px solid #3498db;" // Синяя рамка
        "  background-color: #f0f8ff;" // Легкий фон
        "}"
        );
    connect(ui->pushButton_5, &QPushButton::clicked, this, &MainWindow::on_selectFolderButton_clicked);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::connect_to_server);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::disconnect_to_server);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_selectFolderButton_clicked() {
    // static метод getExistingDirectory возвращает строку с путем к папке
    QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите папку с изображениями"),
                                                    "/home",
                                                    QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        qDebug() << "Выбранный путь:" << dir;
        // Здесь можно вызвать функцию загрузки картинок в QGridLayout, которую мы обсуждали
    }
    ui->lineEdit_5->setText(dir);
    loadSixImages(dir);
}

void MainWindow::loadSixImages(const QString &dir) {
    // 1. Выбираем любой файл в нужной папке
    //  QString filePath = QFileDialog::getOpenFileName(this, "Выберите файл в папке", "", "Images (*.png *.jpg)");

    //  if (filePath.isEmpty()) return;

    // 2. Получаем путь к папке, где лежит этот файл
    //  QFileInfo fileInfo(filePath);
    //  QDir directory = fileInfo.dir();
    QDir directory = QDir(dir);

    // 3. Получаем список всех подходящих файлов в этой папке
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg";
    QStringList allFiles = directory.entryList(filters, QDir::Files);

    // 4. Берем первые 6 (или меньше, если в папке мало файлов)
    int limit = qMin(6, (int)allFiles.size());

    if(limit > 0){
        loadToLabel((ui->label_3), dir+"/"+allFiles[0]);
    }
    if(limit > 1){
        loadToLabel((ui->label_4), dir+"/"+allFiles[1]);
    }
    if(limit > 2){
        loadToLabel((ui->label_5), dir+"/"+allFiles[2]);
    }
    if(limit > 3){
        loadToLabel((ui->label_6), dir+"/"+allFiles[3]);
    }
    // for (int i = 0; i < limit; ++i) {
    //     QString fullPath = directory.absoluteFilePath(allFiles[i]);

    //     // Здесь создаем ваш ClickableWidget и добавляем в QGridLayout
    //     // MyClickableWidget *w = new MyClickableWidget(fullPath);
    //     // layout->addWidget(w, i / 3, i % 3);

    // }
}

void MainWindow::loadToLabel(QLabel *lbl, const QString &path){

    // 1. Создаем объект QPixmap, загружая файл
    QPixmap pix(path);

    // 2. Устанавливаем картинку в QLabel
//    lbl->setPixmap(pix);
    // int w = ui->label->width();
    // int h = ui->label->height();
    // lbl->setPixmap(pix.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QPixmap scaledPix = pix.scaled(lbl->size(),
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
    // 3. (Опционально) Разрешаем картинке масштабироваться под размер лейбла
    //lbl->setScaledContents(true);
    lbl->setPixmap(scaledPix);
}

void MainWindow::disconnect_to_server() {

}

void MainWindow::connect_to_server() {

}
