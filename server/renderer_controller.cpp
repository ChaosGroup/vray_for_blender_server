#include <unordered_map>
#include "renderer_controller.h"
#include "utils/logger.h"

using namespace VRayBaseTypes;


RendererController::RendererController(send_fn_t fn, bool showVFB):
	renderer(nullptr),
	showVFB(showVFB),
	sendFn(fn),
	currentFrame(0),
	type(VRayMessage::RendererType::None) {
}

RendererController::~RendererController() {
	if (renderer) {
		renderer->stop();
	}
}

void RendererController::handle(VRayMessage & message) {
	try {
		switch (message.getType()) {
		case VRayMessage::Type::ChangePlugin:
			if (!renderer) {
				Logger::getInstance().log(Logger::Warning, "Can't change plugin - no renderer loaded!");
				return;
			}
			this->pluginMessage(message);
			break;
		case VRayMessage::Type::ChangeRenderer: {
			auto actionType = message.getRendererAction();
			if (actionType == VRayMessage::RendererAction::SetRendererType) {
				if (renderer) {
					Logger::log(Logger::Error, "Can't set renderer type after init!");
				} else {
					type = message.getRendererType();
					Logger::log(Logger::Debug, "Setting type to", static_cast<int>(type));
				}
				return;
			}

			if (!renderer && actionType != VRayMessage::RendererAction::Init) {
				Logger::getInstance().log(Logger::Warning, "Can't change renderer - no renderer loaded!");
				return;
			}
			this->rendererMessage(message);
			break;
		}
		default:
			Logger::getInstance().log(Logger::Error, "Unknown message type");
			return;
		}
	} catch (std::exception & e) {
		Logger::getInstance().log(Logger::Error, e.what());
		assert(false && "std::exception in RendererController::handle.");
	} catch (VRay::VRayException & e) {
		Logger::getInstance().log(Logger::Error, e.what());
		assert(false && "VRay::VRayException in RendererController::handle.");
	}
}

void RendererController::pluginMessage(VRayMessage & message) {
	if (message.getPluginAction() == VRayMessage::PluginAction::Update) {
		VRay::Plugin plugin = renderer->getPlugin(message.getPlugin());
		if (!plugin) {
			Logger::getInstance().log(Logger::Warning, "Failed to load plugin: ", message.getPlugin());
			return;
		}
		bool success = true;

		switch (message.getValueType()) {
		case VRayBaseTypes::ValueType::ValueTypeTransform:
			success = plugin.setValue(message.getProperty(), *message.getValue<const VRay::Transform>());
			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "value:",
				*message.getValue<const VRay::Transform>(), "\nSuccess:", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeInt:
			success = plugin.setValue(message.getProperty(), *message.getValue<int>());
			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "value:",
				*message.getValue<int>(), "\nSuccess:", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeFloat:
			success = plugin.setValue(message.getProperty(), *message.getValue<float>());
			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "value:",
				*message.getValue<float>(), "\nSuccess:", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypePlugin:
		{
			const VRayBaseTypes::AttrPlugin & attrPlugin = *message.getValue<VRayBaseTypes::AttrPlugin>();
			std::string pluginData = attrPlugin.plugin;
			if (!attrPlugin.output.empty()) {
				pluginData.append("::");
				pluginData.append(attrPlugin.output);
			}

			if (!renderer->getPlugin(attrPlugin.plugin)) {
				Logger::log(Logger::Error, "Failed setting:", message.getProperty(), "=", pluginData, "for plugin", message.getPlugin());
			} else {
				success = plugin.setValueAsString(message.getProperty(), pluginData);

				Logger::log(Logger::Info,
					"Setting", message.getProperty(), "for plugin", message.getPlugin(), "value:",
					pluginData, "\nSuccess:", success);
			}

			break;
		}
		case VRayBaseTypes::ValueType::ValueTypeVector:
			success = plugin.setValue(message.getProperty(), *message.getValue<VRay::Vector>());

			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "value:",
				*message.getValue<VRay::Vector>(), "\nSuccess:", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeString:
			if (message.getValueSetter() == VRayMessage::ValueSetter::AsString) {
				success = plugin.setValueAsString(message.getProperty(), *message.getValue<std::string>());
			} else {
				success = plugin.setValue(message.getProperty(), *message.getValue<std::string>());
			}
			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "value:",
				*message.getValue<std::string>(), "\nSuccess:", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeColor:
			success = plugin.setValue(message.getProperty(), *message.getValue<VRay::Color>());
			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "value:",
				*message.getValue<VRay::Color>(), "\nSuccess:", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeAColor:
			success = plugin.setValue(message.getProperty(), *message.getValue<VRay::AColor>());
			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "value:",
				*message.getValue<VRay::AColor>(), "\nSuccess:", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeListInt:
			success = plugin.setValue(message.getProperty(),
				**message.getValue<VRayBaseTypes::AttrListInt>(),
				message.getValue<VRayBaseTypes::AttrListInt>()->getBytesCount());

			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "size:",
				message.getValue<VRayBaseTypes::AttrListInt>()->getCount(), "\nSuccess:", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeListFloat:
			success = plugin.setValue(message.getProperty(),
				**message.getValue<VRayBaseTypes::AttrListFloat>(),
				message.getValue<VRayBaseTypes::AttrListFloat>()->getBytesCount());

			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "size:",
				message.getValue<VRayBaseTypes::AttrListFloat>()->getCount(), "\nSuccess:", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeListVector:
			success = plugin.setValue(message.getProperty(),
				**message.getValue<VRayBaseTypes::AttrListVector>(),
				message.getValue<VRayBaseTypes::AttrListVector>()->getBytesCount());

			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "size:",
				message.getValue<VRayBaseTypes::AttrListVector>()->getCount(), "\nSuccess:", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeListPlugin:
		{
			const VRayBaseTypes::AttrListPlugin & plist = *message.getValue<VRayBaseTypes::AttrListPlugin>();
			VRay::ValueList pluginList(plist.getCount());
			std::transform(plist.getData()->begin(), plist.getData()->end(), pluginList.begin(), [](const VRayBaseTypes::AttrPlugin & plugin) {
				return VRay::Value(plugin.plugin);
			});
			success = plugin.setValue(message.getProperty(), VRay::Value(pluginList));
			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "size:",
				plist.getCount(), "\nSuccess:", success);
			break;
		}
		case VRayBaseTypes::ValueType::ValueTypeListString:
		{
			const VRayBaseTypes::AttrListString & slist = *message.getValue<VRayBaseTypes::AttrListString>();
			VRay::ValueList stringList(slist.getCount());
			std::transform(slist.getData()->begin(), slist.getData()->end(), stringList.begin(), [](const std::string & str) {
				return VRay::Value(str);
			});
			success = plugin.setValue(message.getProperty(), VRay::Value(stringList));

			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "size:",
				slist.getCount(), "\nSuccess:", success);
			break;
		}
		case VRayBaseTypes::ValueType::ValueTypeMapChannels:
		{
			VRay::ValueList map_channels;
			const VRayBaseTypes::AttrMapChannels & channelMap = *message.getValue<VRayBaseTypes::AttrMapChannels>();

			int i = 0;
			for (const auto &mcIt : channelMap.data) {
				const VRayBaseTypes::AttrMapChannels::AttrMapChannel &map_channel_data = mcIt.second;

				VRay::ValueList map_channel;
				map_channel.push_back(VRay::Value(i++));

				// VRay::IntList should be binary the same as VRayBaseTypes::AttrListInt
				const VRay::IntList & faces = reinterpret_cast<const VRay::IntList &>(*map_channel_data.faces.getData());

				// VRay::VectorList should be binary the same as VRayBaseTypes::AttrListVector
				const VRay::VectorList & vertices = reinterpret_cast<const VRay::VectorList &>(*map_channel_data.vertices.getData());

				map_channel.push_back(VRay::Value(vertices));
				map_channel.push_back(VRay::Value(faces));

				map_channels.push_back(VRay::Value(map_channel));
			}

			success = plugin.setValue(message.getProperty(), VRay::Value(map_channels));

			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "size:",
				channelMap.data.size(), "\nSuccess:", success);
			break;
		}
		case VRayBaseTypes::ValueType::ValueTypeInstancer:
		{
			const VRayBaseTypes::AttrInstancer & inst = *message.getValue<VRayBaseTypes::AttrInstancer>();
			VRay::ValueList instancer(0);
			instancer.reserve(inst.data.getCount() + 1);
			instancer.push_back(VRay::Value(inst.frameNumber));

			std::unordered_map<std::string, VRay::Plugin> instanceReferences;

			for (int i = 0; i < inst.data.getCount(); ++i) {
				const VRayBaseTypes::AttrInstancer::Item &item = (*inst.data)[i];

				// XXX: Also pretty crazy...
				VRay::ValueList instance;

				const VRay::Transform * tm, *vel;
				tm = reinterpret_cast<const VRay::Transform*>(&item.tm);
				vel = reinterpret_cast<const VRay::Transform*>(&item.vel);

				instance.push_back(VRay::Value(item.index));
				instance.push_back(VRay::Value(*tm));
				instance.push_back(VRay::Value(*vel));

				if (instanceReferences.find(item.node.plugin) == instanceReferences.end()) {
					instanceReferences[item.node.plugin] = renderer->getPlugin(item.node.plugin);
				}

				auto & plugin = instanceReferences[item.node.plugin];

				if (!plugin) {
					Logger::log(Logger::Warning, "Instancer referencing not existing plugin", item.node.plugin);
					break;
				}
				instance.push_back(VRay::Value(plugin));

				instancer.push_back(VRay::Value(instance));
			}

			if (instancer.size() == inst.data.getCount() + 1) {
				success = plugin.setValue(message.getProperty(), VRay::Value(instancer));
			} else {
				success = false;
			}

			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "size:",
				inst.data.getCount(), "\nSuccess:", success);
			break;
		}
		default:
			Logger::getInstance().log(Logger::Warning, "Missing case for", message.getValueType());
			success = false;
		}

		if (!success) {
			Logger::log(Logger::Warning, "Failed to set property:", message.getProperty(), "for:", message.getPlugin());
		}
	} else if (message.getPluginAction() == VRayMessage::PluginAction::Create) {
		if (!renderer->newPlugin(message.getPlugin(), message.getPluginType())) {
			Logger::log(Logger::Warning, "Failed to create plugin:", message.getPlugin());
		} else {
			Logger::log(Logger::Info, "Created plugin", message.getPlugin(), "with type:", message.getPluginType());
		}
	} else if (message.getPluginAction() == VRayMessage::PluginAction::Remove) {
		VRay::Plugin plugin = renderer->getPlugin(message.getPlugin().c_str());
		if (plugin) {
			renderer->removePlugin(plugin);
			Logger::log(Logger::Info, "Removed plugin", message.getPlugin());
		} else {
			Logger::log(Logger::Warning, "Failed to remove plugin:", message.getPlugin());
		}
	}
}

void RendererController::rendererMessage(VRayMessage & message) {
	bool completed = true;
	switch (message.getRendererAction()) {
	case VRayMessage::RendererAction::SetCurrentTime:
		currentFrame = message.getValue<AttrSimpleType<float>>()->m_Value;
		renderer->setCurrentTime(message.getValue<AttrSimpleType<float>>()->m_Value);
		Logger::log(Logger::Debug, "Renderer::setCurrentTime", message.getValue<AttrSimpleType<float>>()->m_Value);
		break;
	case VRayMessage::RendererAction::ClearFrameValues:
		completed = renderer->clearAllPropertyValuesUpToTime(message.getValue<AttrSimpleType<float>>()->m_Value);
		Logger::log(Logger::Info, "Renderer::clearAllPropertyValuesUpToTime", message.getValue<AttrSimpleType<float>>()->m_Value);
		break;
	case VRayMessage::RendererAction::Pause:
		completed = renderer->pause();
		Logger::log(Logger::Info, "Renderer::pause");
		break;
	case VRayMessage::RendererAction::Resume:
		completed = renderer->resume();
		Logger::log(Logger::Info, "Renderer::resume");
		break;
	case VRayMessage::RendererAction::Start:
		renderer->start();
		Logger::log(Logger::Info, "Renderer::start");

		if (type == VRayMessage::RendererType::Animation) {
			renderer->waitForImageReady();
			imageDone(*renderer, nullptr);
		}

		break;
	case VRayMessage::RendererAction::Stop:
		Logger::log(Logger::Info, "Renderer::stop");
		renderer->stop();
		break;
	case VRayMessage::RendererAction::Free:
		Logger::log(Logger::Info, "Renderer::free");
		renderer->stop();

		renderer->showFrameBuffer(false);
		renderer.release();
		break;
	case VRayMessage::RendererAction::Init:
	{
		Logger::log(Logger::Info, "Renderer::init");

		VRay::RendererOptions options;
		options.keepRTRunning = type == VRayMessage::RendererType::RT;
		options.noDR = true;
		renderer.reset(new VRay::VRayRenderer(options));
		if (!renderer) {
			completed = false;
		} else {
			auto mode = type == VRayMessage::RendererType::RT ? VRay::RendererOptions::RENDER_MODE_RT_CPU : VRay::RendererOptions::RENDER_MODE_PRODUCTION;
			completed = renderer->setRenderMode(mode);
			renderer->showFrameBuffer(showVFB);


			renderer->setOnRTImageUpdated<RendererController, &RendererController::imageUpdate>(*this);
			renderer->setOnImageReady<RendererController, &RendererController::imageDone>(*this);

			renderer->setOnDumpMessage<RendererController, &RendererController::vrayMessageDumpHandler>(*this);
		}

		break;
	}
	case VRayMessage::RendererAction::Resize:
		int width, height;
		message.getRendererSize(width, height);
		completed = renderer->setWidth(width);
		completed = completed && renderer->setHeight(height);

		Logger::log(Logger::Info, "Renderer::resize", width, "x", height);
		break;
	case VRayMessage::RendererAction::AddHosts:
		completed = 0 == renderer->addHosts(message.getValue<AttrSimpleType<std::string>>()->m_Value);
		Logger::log(Logger::Info, "Renderer::addHosts", message.getValue<AttrSimpleType<std::string>>()->m_Value);
		break;
	case VRayMessage::RendererAction::RemoveHosts:
		completed = 0 == renderer->removeHosts(message.getValue<AttrSimpleType<std::string>>()->m_Value);
		Logger::log(Logger::Info, "Renderer::removeHosts", message.getValue<AttrSimpleType<std::string>>()->m_Value);
		break;
	case VRayMessage::RendererAction::LoadScene:
		completed = 0 == renderer->load(message.getValue<AttrSimpleType<std::string>>()->m_Value);
		Logger::log(Logger::Info, "Renderer::load", message.getValue<AttrSimpleType<std::string>>()->m_Value);
		break;
	case VRayMessage::RendererAction::AppendScene:
		completed = 0 == renderer->append(message.getValue<AttrSimpleType<std::string>>()->m_Value);
		Logger::log(Logger::Info, "Renderer::append", message.getValue<AttrSimpleType<std::string>>()->m_Value);
		break;
	case VRayMessage::RendererAction::ExportScene:
	{
		VRay::VRayExportSettings exportParams;
		exportParams.useHexFormat = false;
		exportParams.compressed = false;

		completed = 0 == renderer->exportScene(message.getValue<AttrSimpleType<std::string>>()->m_Value, &exportParams);

		Logger::log(Logger::Info, "Renderer::exportScene", message.getValue<AttrSimpleType<std::string>>()->m_Value);
	}
		break;
	default:
		Logger::log(Logger::Warning, "Invalid renderer action: ", static_cast<int>(message.getRendererAction()));
	}

	if (!completed) {
		Logger::log(Logger::Warning,
			"Failed renderer action:", static_cast<int>(message.getRendererAction()),
			"\nerror:", renderer->getLastError());
	}
}


void RendererController::imageUpdate(VRay::VRayRenderer & renderer, VRay::VRayImage * img, void * arg) {
	(void)arg;

	size_t size;
	std::unique_ptr<VRay::Jpeg> jpeg(img->getJpeg(size, 50));
	int width, height;
	img->getSize(width, height);

	VRayMessage msg = VRayMessage::createMessage(VRayBaseTypes::AttrImage(jpeg.get(), size, VRayBaseTypes::AttrImage::ImageType::JPG, width, height));
	size_t imageSize = msg.getMessage().size();

	sendFn(std::move(msg));

	Logger::log(Logger::Info, "Renderer::OnRTImageUpdated size:", imageSize);
}


void RendererController::imageDone(VRay::VRayRenderer & renderer, void * arg) {
	(void)arg;

	VRay::VRayImage * img = renderer.getImage();

	size_t size;
	VRay::AColor * data = img->getPixelData(size);
	size *= sizeof(VRay::AColor);

	int width, height;
	img->getSize(width, height);

	VRayMessage msg = VRayMessage::createMessage(VRayBaseTypes::AttrImage(data, size, VRayBaseTypes::AttrImage::ImageType::RGBA_REAL, width, height));
	sendFn(std::move(msg));

	if (type == VRayMessage::RendererType::Animation) {
		VRayMessage::RendererStatus status = renderer.isAborted() ? VRayMessage::RendererStatus::Abort : VRayMessage::RendererStatus::Continue;
		sendFn(VRayMessage::createMessage(status, this->currentFrame));
		Logger::log(Logger::Debug, "Animation frame completed ", currentFrame);
	} else {
		Logger::log(Logger::Info, "Renderer::OnImageReady");
	}
}


void RendererController::vrayMessageDumpHandler(VRay::VRayRenderer &, const char * msg, int level, void * arg) {
	RendererController * rc = reinterpret_cast<RendererController*>(arg);

	Logger::Level logLvl = level <= VRay::MessageError ? Logger::Warning :
	                       level <= VRay::MessageWarning ? Logger::Debug :
	                       Logger::Info;
	Logger::log(logLvl, "VRAY:", msg);

	sendFn(VRayMessage::createMessage(VRayBaseTypes::AttrSimpleType<std::string>(msg)));
}
