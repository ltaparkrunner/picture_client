#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "imgclient.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void on_selectFolderButton_clicked();
    void loadSixImages(const QString &dir);
    void loadToLabel(QLabel *lbl, const QString &path);

    void send_image();
    void send_smth();

private:
    Ui::MainWindow *ui;
    ImageClient *iwsc;
    int counter;

    void displayDBImage(QPixmap &pm);
};
#endif // MAINWINDOW_H
