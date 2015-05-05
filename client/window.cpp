#include <iostream>
#include "window.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow), address("127.0.0.1:5555")
{
	ui->setupUi(this);
	connect(ui->horizontalSlider, &QSlider::sliderMoved, this, &MainWindow::send);

	client.setCallback([this](VRayMessage & message, ZmqWrapper * client) {
		std::cout << "Server's ack\n";
	});
	this->connectServer();
	client.start();
}

MainWindow::~MainWindow()
{
	client.stop();
	delete ui;
}

void MainWindow::connectServer() {
	QString address("tcp://" + this->address);
	client.connect(address.toStdString().c_str());
}

void MainWindow::send()
{
	int value = this->ui->horizontalSlider->value();
	std::cout << "\nMoved to: " << value << std::endl;

	unsigned char tr_data[] = {0xf2, 0xca, 0x8e, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf2, 0xca, 0x8e, 0x3f, 0x00, 0x00, 0x00, 0x00,
							   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf2, 0xca, 0x8e, 0x3f, 0xcd, 0xcc, 0x4c, 0x3e, 0x59, 0x4a, 0x08, 0x3f, 0xc0, 0x68, 0x02, 0x3e};

	VRayBaseTypes::AttrTransform transform;
	memcpy(&transform, tr_data, sizeof(tr_data));
	transform.offs.x = ((float)value - 50) / 50;


	const char * pl_name = "OBSphere";
	const char * tr_name = "transform";

	VRayMessage msg = VRayMessage::createMessage(pl_name, tr_name, transform);
	this->client.send(msg);
}

void MainWindow::on_pushButton_clicked()
{
	this->connectServer();
}
