#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "websocketclient.h"

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

    void connect_to_server();
    void disconnect_to_server();

private:
    Ui::MainWindow *ui;
    WebSocketClient *wsc;
};
#endif // MAINWINDOW_H
