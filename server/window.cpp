#include "window.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <vraysdk.hpp>

const int msg_size = (1 << 20) * 200;


static void CbDumpMessage(VRay::VRayRenderer&, const char *msg, int level, void*)
{
	if (level <= VRay::MessageError) {
		printf("Error: %s", msg);
		//PRINT_PREFIX("V-Ray", "Error: %s", msg);
	}
#if 0
	else if (level > VRay::MessageError && level <= VRay::MessageWarning) {
		PRINT_PREFIX("V-Ray", "Warning: %s", msg);
	}
	else if (level > VRay::MessageWarning && level <= VRay::MessageInfo) {
		PRINT_PREFIX("V-Ray", "%s", msg);
	}
#endif
}

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow), vray(new VRay::VRayInit(true))
{
	VRay::RendererOptions options;
	options.keepRTRunning = true;
	renderer = new VRay::VRayRenderer(options);
	renderer->load("D:/dev/cornellbox.vrscene");
	renderer->setOnDumpMessage(CbDumpMessage);
	renderer->setRenderMode(VRay::RendererOptions::RENDER_MODE_RT_CPU);
	renderer->showFrameBuffer(true);
	renderer->start();


	dummyData = new char[msg_size];
	ui->setupUi(this);
	connect(ui->horizontalSlider, &QSlider::sliderMoved, this, &MainWindow::send);

	server.bind("tcp://*:5555");
	server.setCallback([this](ZmqWrapperMessage & message, ZmqWrapper * server) {
		VRay::Plugin ob = renderer->getPlugin("OBSphere");
		int val = *(reinterpret_cast<int*>(message.data()));

		if (ob) {
			float x = ((float)val - 50) / 50;
			VRay::Transform mat = ob.getValue("transform").getTransform();
			std :: cout << mat.offset.x << " to " << x << std::endl;
			mat.offset.x = x;
			ob.setValue("transform", mat);
		}

		this->ui->horizontalSlider->setValue(val);
		std::cout << "Set to: " << val << std::endl;
	});
	server.start();
}

MainWindow::~MainWindow()
{
	server.stop();
	delete[] dummyData;
	delete renderer;
	delete vray;
	delete ui;
}

void MainWindow::send()
{
	int value = this->ui->horizontalSlider->value();
	std::cout << "Moved to: " << value << std::endl;
	memcpy(dummyData, &value, sizeof(value));
	this->server.send(dummyData, msg_size);
}
