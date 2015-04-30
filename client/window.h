#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "../zmq-wrapper/zmq_wrapper.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void send();

private:
	char * dummyData;
	ZmqClient client;
    Ui::MainWindow *ui;
};


#endif // MAINWINDOW_H