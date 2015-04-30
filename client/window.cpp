#include <iostream>
#include "window.h"
#include "ui_mainwindow.h"

const int msg_size = (1 << 20) * 200;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	dummyData = new char[msg_size];
    ui->setupUi(this);
    connect(ui->horizontalSlider, &QSlider::sliderMoved, this, &MainWindow::send);

	client.connect("tcp://localhost:5555");
	client.setCallback([this](ZmqWrapperMessage & message, ZmqWrapper * client) {
		int value = *(reinterpret_cast<int*>(message.data()));
		this->ui->horizontalSlider->setValue(value);
		std::cout << "Set to: " << value << std::endl;
	});
	client.start();
}

MainWindow::~MainWindow()
{
	client.stop();
	delete[] dummyData;
    delete ui;
}

void MainWindow::send()
{
	int value = this->ui->horizontalSlider->value();
	std::cout << "Moved to: " << value << std::endl;
	memcpy(dummyData, &value, sizeof(value));
	this->client.send(dummyData, msg_size);
}
