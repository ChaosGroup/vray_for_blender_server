#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <vraysdk.hpp>
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
	ZmqServer server;
	Ui::MainWindow *ui;
	VRay::VRayInit *vray;
	VRay::VRayRenderer *renderer;
};


#endif // MAINWINDOW_H
