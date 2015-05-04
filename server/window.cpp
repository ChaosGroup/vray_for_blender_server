#include "window.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <iomanip>
#include <vraysdk.hpp>


MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow), vray(new VRay::VRayInit(true))
{
	VRay::RendererOptions options;
	options.keepRTRunning = true;
	renderer = new VRay::VRayRenderer(options);
	renderer->load("D:/dev/cornellbox.vrscene");

	renderer->setOnDumpMessage([] (VRay::VRayRenderer &, const char * msg, int level, void *) {
		if (level <= VRay::MessageError) {
			std::cout << "Error: " << msg;
		}
	});

	renderer->setRenderMode(VRay::RendererOptions::RENDER_MODE_RT_CPU);
	renderer->showFrameBuffer(true);
	renderer->start();

	ui->setupUi(this);

	server.bind("tcp://*:5555");
	server.setCallback([this](VRayMessage & message, ZmqWrapper * server) {
		std::cout << "Message plugin:" << message.getPlugin() << " prop:" << message.getProperty() << std::endl;

		VRay::Plugin ob = renderer->getPlugin(message.getPlugin());

		if (ob) {
			VRayBaseTypes::AttrTransform tr;
			message.getValue(tr);
			VRay::Transform * v_transform = reinterpret_cast<VRay::Transform*>(&tr);
			ob.setValue(message.getProperty(), *v_transform);
			std::cout << "Set to: " << v_transform->offset.x << std::endl;
		}

	});
	server.start();
}

MainWindow::~MainWindow()
{
	server.stop();
	delete renderer;
	delete vray;
	delete ui;
}
