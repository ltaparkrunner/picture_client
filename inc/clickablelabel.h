#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H
#include <QLabel>

class ClickableLabel : public QLabel {
    Q_OBJECT
public:
    using QLabel::QLabel;
    ClickableLabel();
    ClickableLabel(QWidget *parent);
    void resizeEvent(QResizeEvent *event) override;
signals:
    void clicked();
protected:
    void mousePressEvent2(QMouseEvent* event) /* override */ {
        emit clicked();
    }
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // CLICKABLELABEL_H
