#include "clickablelabel.h"

ClickableLabel::ClickableLabel(): QLabel() {}

ClickableLabel::ClickableLabel(QWidget *parent)
    : QLabel(parent) // Передаем parent в базовый класс QLabel
{
    // Здесь можно задать настройки по умолчанию для всех таких лейблов
    this->setCursor(Qt::PointingHandCursor); // Курсор в виде "руки" при наведении
    this->setAlignment(Qt::AlignCenter);      // Текст/картинка по центру
}

// Реализация события клика
void ClickableLabel::mousePressEvent(QMouseEvent *event) {
    this->setStyleSheet("border: 3px solid #2ecc71;"); // Зеленая рамка при клике

    emit clicked(); // Генерируем сигнал
    QLabel::mousePressEvent(event); // Вызываем базовую обработку (хороший тон)
}

void ClickableLabel::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);

    // if (!originalPixmap.isNull()) {
    //     ui->label->setPixmap(originalPixmap.scaled(ui->label->size(),
    //                                                Qt::KeepAspectRatio,
    //                                                Qt::SmoothTransformation));
    // }
}
