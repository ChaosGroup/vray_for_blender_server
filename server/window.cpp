#include "window.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <iomanip>
#include <vraysdk.hpp>


void imageUpdate(VRay::VRayRenderer & renderer, VRay::VRayImage * img, void * arg) {
	ZmqWrapper * server = reinterpret_cast<ZmqWrapper*>(arg);

	//size_t size;
	//std::unique_ptr<VRay::Jpeg> jpeg(img->getJpeg(size, 50));
	//int width, height;
	//img->getSize(width, height);

	//VRayMessage msg = VRayMessage::createMessage(VRayBaseTypes::AttrImage(jpeg.get(), size, VRayBaseTypes::AttrImage::ImageType::JPG, width, height));

	size_t size;
	VRay::AColor * data = img->getPixelData(size);
	size *= sizeof(VRay::AColor);

	int width, height;
	img->getSize(width, height);

	VRayMessage msg = VRayMessage::createMessage(VRayBaseTypes::AttrImage(data, size, VRayBaseTypes::AttrImage::ImageType::RGBA_REAL, width, height));

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
				bool success = true;

				std::cout << "Property Name: " << message.getProperty() << "  ";

				switch (message.getValueType()) {
				case VRayBaseTypes::ValueType::ValueTypeTransform:
					std::cout << "Setting property Transform for:" << message.getPlugin() << "\n";
					success = plugin.setValue(message.getProperty(), *message.getValue<const VRay::Transform>());
					break;
				case VRayBaseTypes::ValueType::ValueTypeInt:
					std::cout << "Setting property Int for:" << message.getPlugin() << " value:" << *message.getValue<int>() << "\n";
					success = plugin.setValue(message.getProperty(), *message.getValue<int>());
					break;
				case VRayBaseTypes::ValueType::ValueTypeFloat:
					std::cout << "Setting property Float for:" << message.getPlugin() << " value:" << *message.getValue<float>() << "\n";
					success = plugin.setValue(message.getProperty(), *message.getValue<float>());
					break;
				case VRayBaseTypes::ValueType::ValueTypeString:
					if (message.getValueSetter() == VRayMessage::ValueSetter::AsString) {
						std::cout << "Setting property String (as String) for:" << message.getPlugin() << " value:" << *message.getValue<std::string>() << "\n";
						success = plugin.setValueAsString(message.getProperty(), *message.getValue<std::string>());
					} else {
						std::cout << "Setting property String for:" << message.getPlugin() << " value:" << *message.getValue<std::string>() << "\n";
						success = plugin.setValue(message.getProperty(), *message.getValue<std::string>());
					}
					break;
				case VRayBaseTypes::ValueType::ValueTypeColor:
					std::cout << "Setting property Color for:" << message.getPlugin() << "\n";
					success = plugin.setValue(message.getProperty(), *message.getValue<VRay::Color>());
					break;
				case VRayBaseTypes::ValueType::ValueTypeAColor:
					std::cout << "Setting property AColor for:" << message.getPlugin() << "\n";
					success = plugin.setValue(message.getProperty(), *message.getValue<VRay::AColor>());
					break;
				case VRayBaseTypes::ValueType::ValueTypeListInt:
					std::cout << "Setting property ListInt for:" << message.getPlugin() << "\n";
					success = plugin.setValue(message.getProperty(),
						**message.getValue<VRayBaseTypes::AttrListInt>(),
						message.getValue<VRayBaseTypes::AttrListInt>()->getBytesCount());
					break;
				case VRayBaseTypes::ValueType::ValueTypeListVector:
					std::cout << "Setting property ListVector for:" << message.getPlugin() << "\n";
					success = plugin.setValue(message.getProperty(),
						**message.getValue<VRayBaseTypes::AttrListVector>(),
						message.getValue<VRayBaseTypes::AttrListVector>()->getBytesCount());
					break;
				case VRayBaseTypes::ValueType::ValueTypeListPlugin:
				{
					std::cout << "Setting property ListPlugin for:" << message.getPlugin() << "\n";
					const VRayBaseTypes::AttrListPlugin & plist = *message.getValue<VRayBaseTypes::AttrListPlugin>();
					VRay::ValueList pluginList(plist.getCount());
					std::transform(plist.getData()->begin(), plist.getData()->end(), pluginList.begin(), [](const VRayBaseTypes::AttrPlugin & plugin) {
						return VRay::Value(plugin);
					});
					success = plugin.setValue(message.getProperty(), VRay::Value(pluginList));
					break;
				}
				case VRayBaseTypes::ValueType::ValueTypeListString:
				{
					std::cout << "Setting property ListPlugin for:" << message.getPlugin() << "\n";
					const VRayBaseTypes::AttrListString & slist = *message.getValue<VRayBaseTypes::AttrListString>();
					VRay::ValueList stringList(slist.getCount());
					std::transform(slist.getData()->begin(), slist.getData()->end(), stringList.begin(), [](const std::string & str) {
						return VRay::Value(str);
					});
					success = plugin.setValue(message.getProperty(), VRay::Value(stringList));
					break;
				}
				case VRayBaseTypes::ValueType::ValueTypeMapChannels:
				{
					std::cout << "Setting property ListPlugin for:" << message.getPlugin() << "\n";
					VRay::ValueList map_channels;
					const VRayBaseTypes::AttrMapChannels & channelMap = *message.getValue<VRayBaseTypes::AttrMapChannels>();

					int i = 0;
					for (const auto &mcIt : channelMap.data) {
						const VRayBaseTypes::AttrMapChannels::AttrMapChannel &map_channel_data = mcIt.second;

						VRay::ValueList map_channel;
						map_channel.push_back(VRay::Value(i++));

						// XXX: Craaaazy...
						VRay::IntList faces;
						faces.resize(map_channel_data.faces.getCount());
						memcpy(&faces[0], *map_channel_data.faces, map_channel_data.faces.getBytesCount());

						VRay::VectorList vertices;
						vertices.resize(map_channel_data.vertices.getCount());
						memcpy(&vertices[0], *map_channel_data.vertices, map_channel_data.vertices.getBytesCount());

						map_channel.push_back(VRay::Value(vertices));
						map_channel.push_back(VRay::Value(faces));

						map_channels.push_back(VRay::Value(map_channel));
					}

					success = plugin.setValue(message.getProperty(), VRay::Value(map_channels));
					break;
				}
				default:
					std::cerr << "Missing case for " << message.getValueType() << std::endl;
					success = false;
				}

				if (!success) {
					std::cerr << "Failed to set property: " << message.getProperty() << " for: " << message.getPlugin() << std::endl;
				}
			} else if (message.getPluginAction() == VRayMessage::PluginAction::Create) {
				if (!renderer->newPlugin(message.getPlugin(), message.getPluginType()) ){
					std::cerr << "Failed to create plugin: " << message.getPlugin() << std::endl;
				} else {
					std::cout << "Plugin created " << message.getPlugin() << " " << message.getPluginType() << "\n";
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
				std::cout << "Renderer paused\n";
				completed = renderer->pause();
				break;
			case VRayMessage::RendererAction::Resume:
				std::cout << "Renderer resumed\n";
				completed = renderer->resume();
				break;
			case VRayMessage::RendererAction::Start:
				std::cout << "Renderer started\n";
				renderer->start();
				break;
			case VRayMessage::RendererAction::Stop:
				std::cout << "Renderer stopped\n";
				renderer->stop();
				break;
			case VRayMessage::RendererAction::Free:
				std::cout << "Renderer freed\n";
				renderer->stop();
				renderer.release();
				break;
			case VRayMessage::RendererAction::Init:
				std::cout << "Renderer initted\n";
				renderer.reset(new VRay::VRayRenderer(options));
				break;
			case VRayMessage::RendererAction::Resize:
				int width, height;
				message.getRendererSize(width, height);

				std::cout << "Renderer resized to " << width << " " << height << "\n";
				completed = renderer->setWidth(width);
				completed = completed && renderer->setHeight(height);
				break;
			case VRayMessage::RendererAction::AddHosts:
				std::cout << "Renderer hosts added: " << message.getRendererArgument() << "\n";
				completed = 0 == renderer->addHosts(message.getRendererArgument());
				break;
			case VRayMessage::RendererAction::RemoveHosts:
				std::cout << "Renderer hosts removed: " << message.getRendererArgument() << "\n";
				completed = 0 == renderer->removeHosts(message.getRendererArgument());
				break;
			case VRayMessage::RendererAction::LoadScene:
				std::cout << "Renderer scene loaded: " << message.getRendererArgument() << "\n";
				completed = 0 == renderer->load(message.getRendererArgument());
				break;
			case VRayMessage::RendererAction::AppendScene:
				std::cout << "Renderer scene appended: " << message.getRendererArgument() << "\n";
				completed = 0 == renderer->append(message.getRendererArgument());
				break;
			case VRayMessage::RendererAction::ExportScene:
				std::cout << "Renderer scene exported: " << message.getRendererArgument() << "\n";
				{
					VRay::VRayExportSettings exportParams;
					exportParams.useHexFormat = false;
					exportParams.compressed = false;

					completed = 0 == renderer->exportScene(message.getRendererArgument(), &exportParams);
				}
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
