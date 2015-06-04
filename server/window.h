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

	void setListeningPort(int port);
	bool start();

private:
	ZmqServer server;
	Ui::MainWindow *ui;
	std::unique_ptr<VRay::VRayInit> vray;
	std::unique_ptr<VRay::VRayRenderer> renderer;
	int serverPort;
};


#endif // MAINWINDOW_H
