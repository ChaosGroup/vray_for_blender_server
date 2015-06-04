#include "window.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <iomanip>
#include <vraysdk.hpp>


MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow), vray(new VRay::VRayInit(true)), renderer(nullptr), showVFB(false), serverPort(-1)
{
	ui->setupUi(this);
    controller.setWindow(this);
}

void MainWindow::setListeningPort(int port) {
	this->serverPort = port;
}

void MainWindow::setShowVFB(bool show) {
	this->showVFB = show;
}

bool MainWindow::start() {
	try {
		server.bind((QString("tcp://*:") + QString::number(this->serverPort)).toStdString().c_str());
	} catch (zmq::error_t & e) {
		std::cerr << e.what();
		return false;
	}
	return true;
}

MainWindow::~MainWindow()
{
	delete ui;
}
