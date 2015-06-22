#include "renderer_controller.h"

using namespace VRayBaseTypes;

namespace {
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

        VRay::VRayImage * img = renderer.getImage();

        size_t size;
        VRay::AColor * data = img->getPixelData(size);
        size *= sizeof(VRay::AColor);

        int width, height;
        img->getSize(width, height);

        VRayMessage msg = VRayMessage::createMessage(VRayBaseTypes::AttrImage(data, size, VRayBaseTypes::AttrImage::ImageType::RGBA_REAL, width, height));

        server->send(msg);
    }
}

void vrayMessageDumpHandler(VRay::VRayRenderer &, const char * msg, int level, void * arg) {
	RendererController * rc = reinterpret_cast<RendererController*>(arg);
	if (level <= VRay::MessageError) {
		std::cout << "Error: " << msg;
	}
	if (1) {
		rc->server.send(VRayMessage::createMessage(VRayBaseTypes::AttrSimpleType<std::string>(msg)));
	}
}

RendererController::RendererController(const std::string & address, uint64_t rendererId):
	renderer(nullptr), showVFB(showVFB) {

	this->server.setIdentity(rendererId);
	this->server.setCallback(std::bind(&RendererController::dispatcher, this, std::placeholders::_1, std::placeholders::_2));
	try {
		this->server.connect(address.c_str());
	} catch (zmq::error_t & e) {
		std::cerr << e.what();
	}
}

RendererController::~RendererController() {
	if (renderer) {
		renderer->stop();
	}
}

void RendererController::dispatcher(VRayMessage & message, ZmqWrapper * server) {
	try {
		switch (message.getType()) {
		case VRayMessage::Type::ChangePlugin:
			if (!renderer) {
				std::cerr << "No renderer loaded!\n";
				return;
			}
			this->pluginMessage(message);
			break;
		case VRayMessage::Type::ChangeRenderer:
			if (!renderer && message.getRendererAction() != VRayMessage::RendererAction::Init) {
				std::cerr << "No renderer loaded!\n";
				return;
			}
			this->rendererMessage(message, server);
			break;
		}
	} catch (zmq::error_t & e) {
		auto x = e.what();
		assert(false);
	} catch (std::exception & e) {
		auto x = e.what();
		assert(false);
	} catch (VRay::VRayException & e) {
		auto x = e.what();
		assert(false);
	}
}

void RendererController::pluginMessage(VRayMessage & message) {
    if (message.getPluginAction() == VRayMessage::PluginAction::Update) {
        VRay::Plugin plugin = renderer->getPlugin(message.getPlugin());
        if (!plugin) {
            std::cerr << "Failed to load plugin: " << message.getPlugin() << std::endl;
            return;
        }
        bool success = true;

        switch (message.getValueType()) {
        case VRayBaseTypes::ValueType::ValueTypeTransform:
            success = plugin.setValue(message.getProperty(), *message.getValue<const VRay::Transform>());
            break;
        case VRayBaseTypes::ValueType::ValueTypeInt:
            success = plugin.setValue(message.getProperty(), *message.getValue<int>());
            break;
        case VRayBaseTypes::ValueType::ValueTypeFloat:
            success = plugin.setValue(message.getProperty(), *message.getValue<float>());
            break;
        case VRayBaseTypes::ValueType::ValueTypePlugin:
        {
            const VRayBaseTypes::AttrPlugin & attrPlugin = *message.getValue<VRayBaseTypes::AttrPlugin>();
            std::string pluginData = attrPlugin.plugin;
            if (!attrPlugin.output.empty()) {
                pluginData.append("::");
                pluginData.append(attrPlugin.output);
            }
            success = plugin.setValueAsString(message.getProperty(), pluginData);
            break;
        }
        case VRayBaseTypes::ValueType::ValueTypeString:
            if (message.getValueSetter() == VRayMessage::ValueSetter::AsString) {
                success = plugin.setValueAsString(message.getProperty(), *message.getValue<std::string>());
            } else {
                success = plugin.setValue(message.getProperty(), *message.getValue<std::string>());
            }
            break;
        case VRayBaseTypes::ValueType::ValueTypeColor:
            success = plugin.setValue(message.getProperty(), *message.getValue<VRay::Color>());
            break;
        case VRayBaseTypes::ValueType::ValueTypeAColor:
            success = plugin.setValue(message.getProperty(), *message.getValue<VRay::AColor>());
            break;
        case VRayBaseTypes::ValueType::ValueTypeListInt:
            success = plugin.setValue(message.getProperty(),
                **message.getValue<VRayBaseTypes::AttrListInt>(),
                message.getValue<VRayBaseTypes::AttrListInt>()->getBytesCount());
            break;
        case VRayBaseTypes::ValueType::ValueTypeListFloat:
            success = plugin.setValue(message.getProperty(),
                **message.getValue<VRayBaseTypes::AttrListFloat>(),
                message.getValue<VRayBaseTypes::AttrListFloat>()->getBytesCount());
            break;
        case VRayBaseTypes::ValueType::ValueTypeListVector:
            success = plugin.setValue(message.getProperty(),
                **message.getValue<VRayBaseTypes::AttrListVector>(),
                message.getValue<VRayBaseTypes::AttrListVector>()->getBytesCount());
            break;
        case VRayBaseTypes::ValueType::ValueTypeListPlugin:
        {
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
        case VRayBaseTypes::ValueType::ValueTypeInstancer:
        {
            const VRayBaseTypes::AttrInstancer & inst = *message.getValue<VRayBaseTypes::AttrInstancer>();
            VRay::ValueList instancer;

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
                instance.push_back(VRay::Value(item.node));

                instancer.push_back(VRay::Value(instance));
            }

            success = plugin.setValue(message.getProperty(), VRay::Value(instancer));
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
        if (!renderer->newPlugin(message.getPlugin(), message.getPluginType())) {
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
}

void RendererController::rendererMessage(VRayMessage & message, ZmqWrapper * server) {
    bool completed = true;
    switch (message.getRendererAction()) {
    case VRayMessage::RendererAction::SetCurrentTime:
		//completed = renderer->setCurrentTime(message.getValue<AttrSimpleType<float>>()->m_Value);
        break;
    case VRayMessage::RendererAction::ClearFrameValues:
		//completed = renderer->clearAllPropertyValuesUpToTime(message.getValue<AttrSimpleType<float>>()->m_Value);
        break;
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

        renderer->showFrameBuffer(false);
        renderer.release();
        break;
    case VRayMessage::RendererAction::Init:
    {
        VRay::RendererOptions options;
        options.keepRTRunning = true;
        renderer.reset(new VRay::VRayRenderer(options));
        if (!renderer) {
            completed = false;
        } else {
            completed = renderer->setRenderMode(VRay::RendererOptions::RENDER_MODE_RT_CPU);
            renderer->showFrameBuffer(true);

            renderer->setOnRTImageUpdated(imageUpdate, server);
            renderer->setOnImageReady(imageDone, server);

            renderer->setOnDumpMessage(vrayMessageDumpHandler, this);
        }

        break;
    }
    case VRayMessage::RendererAction::Resize:
        int width, height;
        message.getRendererSize(width, height);
        completed = renderer->setWidth(width);
        completed = completed && renderer->setHeight(height);
        break;
    case VRayMessage::RendererAction::AddHosts:
		completed = 0 == renderer->addHosts(message.getValue<AttrSimpleType<std::string>>()->m_Value);
        break;
    case VRayMessage::RendererAction::RemoveHosts:
		completed = 0 == renderer->removeHosts(message.getValue<AttrSimpleType<std::string>>()->m_Value);
        break;
    case VRayMessage::RendererAction::LoadScene:
		completed = 0 == renderer->load(message.getValue<AttrSimpleType<std::string>>()->m_Value);
        break;
    case VRayMessage::RendererAction::AppendScene:
		completed = 0 == renderer->append(message.getValue<AttrSimpleType<std::string>>()->m_Value);
        break;
    case VRayMessage::RendererAction::ExportScene:
    {
        VRay::VRayExportSettings exportParams;
        exportParams.useHexFormat = false;
        exportParams.compressed = false;

		completed = 0 == renderer->exportScene(message.getValue<AttrSimpleType<std::string>>()->m_Value, &exportParams);
    }
        break;
    default:
        std::cerr << "Invalid renderer action: " << static_cast<int>(message.getRendererAction()) << std::endl;
    }

    if (!completed) {
        std::cerr << "Failed renderer action: " << static_cast<int>(message.getRendererAction())
            << "\nerror:" << renderer->getLastError() << std::endl;
    }
}
