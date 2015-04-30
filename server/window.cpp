#include "window.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <vraysdk.hpp>

const int msg_size = (1 << 20) * 200;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	dummyData = new char[msg_size];
	ui->setupUi(this);
	connect(ui->horizontalSlider, &QSlider::sliderMoved, this, &MainWindow::send);

	server.bind("tcp://*:5555");
	server.setCallback([this](ZmqWrapperMessage & message, ZmqWrapper * server) {
		int value = *(reinterpret_cast<int*>(message.data()));
		this->ui->horizontalSlider->setValue(value);
		std::cout << "Set to: " << value << std::endl;
	});
	server.start();
}

MainWindow::~MainWindow()
{
	server.stop();
	delete[] dummyData;
	delete ui;
}

void MainWindow::send()
{
	int value = this->ui->horizontalSlider->value();
	std::cout << "Moved to: " << value << std::endl;
	memcpy(dummyData, &value, sizeof(value));
	this->server.send(dummyData, msg_size);
}
