#include "window.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <iomanip>
#include <vraysdk.hpp>


void imageUpdate(VRay::VRayRenderer & renderer, VRay::VRayImage * img, void * arg) {
	ZmqWrapper * server = reinterpret_cast<ZmqWrapper*>(arg);

	size_t size;
	std::unique_ptr<VRay::Jpeg> jpeg(img->getJpeg(size, 50));
	int width, height;
	img->getSize(width, height);

	VRayMessage msg = VRayMessage::createMessage(VRayBaseTypes::AttrImage(jpeg.get(), size, VRayBaseTypes::AttrImage::ImageType::JPG, width, height));
	server->send(msg);
}

void imageDone(VRay::VRayRenderer & renderer, void * arg) {
	ZmqWrapper * server = reinterpret_cast<ZmqWrapper*>(arg);

	size_t size;
	VRay::AColor * data = renderer.getImage()->getPixelData(size);
	size *= sizeof(VRay::AColor);

	int width, height;
	renderer.getImage()->getSize(width, height);

	VRayMessage msg = VRayMessage::createMessage(VRayBaseTypes::AttrImage(data, size, VRayBaseTypes::AttrImage::ImageType::RGBA_REAL, width, height));

	server->send(msg);
}



MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow), vray(new VRay::VRayInit(true)), renderer(nullptr)
{
	VRay::RendererOptions options;
	options.keepRTRunning = true;
	renderer.reset(new VRay::VRayRenderer(options));

	renderer->setOnDumpMessage([] (VRay::VRayRenderer &, const char * msg, int level, void *) {
		if (level <= VRay::MessageError) {
			std::cout << "Error: " << msg;
		}
	});

	renderer->setRenderMode(VRay::RendererOptions::RENDER_MODE_RT_CPU);
	renderer->showFrameBuffer(true);

	renderer->setOnRTImageUpdated(imageUpdate, &server);
	renderer->setOnImageReady(imageDone, &server);

	ui->setupUi(this);

	server.setCallback([this, &options] (VRayMessage & message, ZmqWrapper * server) {

		if (message.getType() == VRayMessage::Type::ChangePlugin) {
			if (message.getPluginAction() == VRayMessage::PluginAction::Update) {
				VRay::Plugin plugin = renderer->getPlugin(message.getPlugin());
				if (!plugin) {
					std::cerr << "Failed to load plugin: " << message.getPlugin() << std::endl;
					return;
				}

				switch (message.getValueType()) {
				case VRayBaseTypes::ValueType::ValueTypeTransform:
					plugin.setValue(message.getProperty(), *reinterpret_cast<const VRay::Transform*>(message.getValue<const VRayBaseTypes::AttrTransform*>()));
					break;
				}
			} else if (message.getPluginAction() == VRayMessage::PluginAction::Create) {
				if (!renderer->newPlugin(message.getPlugin()) ){
					std::cerr << "Failed to create plugin: " << message.getPlugin() << std::endl;
				}
			} else if (message.getPluginAction() == VRayMessage::PluginAction::Remove) {
				VRay::Plugin plugin = renderer->getPlugin(message.getPlugin());
				if (plugin) {
					renderer->removePlugin(plugin);
				} else {
					std::cerr << "Failed to remove plugin: " << message.getPlugin() << std::endl;
				}
			}
		} else if (message.getType() == VRayMessage::Type::ChangeRenderer) {
			bool completed = true;
			switch (message.getRendererAction()) {
			case VRayMessage::RendererAction::Pause:
				completed = renderer->pause();
				break;
			case VRayMessage::RendererAction::Resume:
				completed = renderer->resume();
				break;
			case VRayMessage::RendererAction::Start:
				renderer->start();
				break;
			case VRayMessage::RendererAction::Stop:
				renderer->stop();
				break;
			case VRayMessage::RendererAction::Free:
				renderer->stop();
				renderer.release();
				break;
			case VRayMessage::RendererAction::Init:
				renderer.reset(new VRay::VRayRenderer(options));
				break;
			case VRayMessage::RendererAction::Resize:
				int width, height;
				message.getRendererSize(width, height);
				completed = renderer->setWidth(width);
				completed = completed && renderer->setHeight(height);
				break;
			case VRayMessage::RendererAction::AddHosts:
				completed = 0 == renderer->addHosts(message.getRendererArgument());
				break;
			case VRayMessage::RendererAction::RemoveHosts:
				completed = 0 == renderer->removeHosts(message.getRendererArgument());
				break;
			case VRayMessage::RendererAction::LoadScene:
				completed = 0 == renderer->load(message.getRendererArgument());
				break;
			case VRayMessage::RendererAction::AppendScene:
				completed = 0 == renderer->append(message.getRendererArgument());
				break;
			case VRayMessage::RendererAction::ExportScene:
				completed = 0 == renderer->exportScene(message.getRendererArgument(), nullptr);
				break;
			default:
				std::cerr << "Invalid renderer action: " << static_cast<int>(message.getRendererAction()) << std::endl;
			}

			if (!completed) {
				std::cerr << "Failed renderer action: " << static_cast<int>(message.getRendererAction())
						  << "\nerror:" << renderer->getLastError()
						  << "\narguments: [" << message.getRendererArgument() << "]" << std::endl;
			}
		}
	});

	server.bind("tcp://*:5555");
}

MainWindow::~MainWindow()
{
	delete ui;
}
