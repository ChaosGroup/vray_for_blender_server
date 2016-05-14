#define VRAY_RUNTIME_LOAD_SECONDARY
#include <algorithm>
#include <unordered_map>
#include "renderer_controller.h"
#include "utils/logger.h"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

using namespace VRayBaseTypes;


RendererController::RendererController(send_fn_t fn, bool showVFB):
	renderer(nullptr),
	showVFB(showVFB),
	sendFn(fn),
	currentFrame(0),
	type(VRayMessage::RendererType::None),
	jpegQuality(60) {
}

RendererController::~RendererController() {
	stopRenderer();
}

void RendererController::handle(const VRayMessage & message) {
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
		//assert(false && "std::exception in RendererController::handle.");
	} catch (VRay::VRayException & e) {
		Logger::getInstance().log(Logger::Error, e.what());
		//assert(false && "VRay::VRayException in RendererController::handle.");
	}
}

void RendererController::pluginMessage(const VRayMessage & message) {
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

			if (attrPlugin.plugin == "NULL") {
				success = plugin.setValue(message.getProperty(), VRay::Plugin());
				Logger::log(Logger::Info, "Removing", message.getProperty(), "for plugin", message.getPlugin(), "Success:", success);
			} else {
				if (!renderer->getPlugin(attrPlugin.plugin)) {
					Logger::log(Logger::Error, "Failed setting:", message.getProperty(), "=", pluginData, "for plugin", message.getPlugin());
				} else {
					success = plugin.setValueAsString(message.getProperty(), pluginData);

					Logger::log(Logger::Info,
						"Setting", message.getProperty(), "for plugin", message.getPlugin(), "value:",
						pluginData, "\nSuccess:", success);
				}
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
			VRay::VUtils::ValueRefList pluginList(plist.getCount());

			for (int c = 0; c < plist.getCount(); ++c) {
				const auto & messagePlugin = (*plist)[c];
				const auto & vrayPlugin = renderer->getPlugin(messagePlugin.plugin);
				VRay::VUtils::ObjectID pluginId = { VRay::NO_ID };

				if (!vrayPlugin) {
					Logger::log(Logger::Warning, "Missing plugin", messagePlugin.plugin, "referenced in plugin list for", message.getPlugin());
				} else {
					pluginId = {vrayPlugin.getId()};
				}

				pluginList[c].setObjectID(pluginId);
			}
			plugin.setValue(message.getProperty(), pluginList);

			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "size:",
				plist.getCount(), "\nSuccess:", success);
			break;
		}
		case VRayBaseTypes::ValueType::ValueTypeListString:
		{
			const VRayBaseTypes::AttrListString & slist = *message.getValue<VRayBaseTypes::AttrListString>();
			VRay::VUtils::CharStringRefList stringList(slist.getCount());

			for (int c = 0; c < slist.getCount(); ++c) {
				stringList[c].set((*slist)[c].c_str());
			}
			success = plugin.setValue(message.getProperty(), stringList);

			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "size:",
				slist.getCount(), "\nSuccess:", success);
			break;
		}
		case VRayBaseTypes::ValueType::ValueTypeMapChannels:
		{
			const VRayBaseTypes::AttrMapChannels & channelMap = *message.getValue<VRayBaseTypes::AttrMapChannels>();
			VRay::VUtils::ValueRefList map_channels(static_cast<int>(channelMap.data.size()));

			int i = 0;
			for (const auto &mcIt : channelMap.data) {
				const VRayBaseTypes::AttrMapChannels::AttrMapChannel &map_channel_data = mcIt.second;

				// Construct VRay::VUtils::IntRefList from std::vector (VRayBaseTypes::AttrListInt)'s data ptr
				auto & intVec = *map_channel_data.faces.getData();
				VRay::VUtils::IntRefList faces(static_cast<int>(intVec.size()));
				memcpy(faces.get(), intVec.data(), intVec.size() * sizeof(int));

				// Construct VRay::VUtils::IntRefList from std::vector (VRayBaseTypes::AttrListInt)'s data ptr
				auto & vecVec = *map_channel_data.vertices.getData();
				// Additionally VRay::Vector and VRayBaseTypes::AttrVector should have the same layout
				VRay::VUtils::VectorRefList vertices(static_cast<int>(vecVec.size()));
				memcpy(vertices.get(), vecVec.data(), vecVec.size() * sizeof(VRayBaseTypes::AttrVector));

				VRay::VUtils::ValueRefList map_channel(3);
				map_channel[0].setDouble(i);
				map_channel[1].setListVector(vertices);
				map_channel[2].setListInt(faces);

				map_channels[i++].setList(map_channel);
			}

			success = plugin.setValue(message.getProperty(), map_channels);

			Logger::log(Logger::Info,
				"Setting", message.getProperty(), "for plugin", message.getPlugin(), "size:",
				channelMap.data.size(), "\nSuccess:", success);
			break;
		}
		case VRayBaseTypes::ValueType::ValueTypeInstancer:
		{
			const VRayBaseTypes::AttrInstancer & inst = *message.getValue<VRayBaseTypes::AttrInstancer>();
			VRay::VUtils::ValueRefList instancer(inst.data.getCount() + 1);
			instancer[0] = VRay::VUtils::Value(inst.frameNumber);

			std::unordered_map<std::string, VRay::Plugin> instanceReferences;

			for (int i = 0; i < inst.data.getCount(); ++i) {
				const VRayBaseTypes::AttrInstancer::Item &item = (*inst.data)[i];

				// XXX: Also pretty crazy...
				VRay::VUtils::ValueRefList instance(4);

				const VRay::Transform * tm, *vel;
				tm = reinterpret_cast<const VRay::Transform*>(&item.tm);
				vel = reinterpret_cast<const VRay::Transform*>(&item.vel);

				instance[0].setDouble(item.index);
				instance[1].setTransform(*tm);
				instance[2].setTransform(*vel);

				auto pluginIter = instanceReferences.find(item.node.plugin);
				if (pluginIter == instanceReferences.end()) {
					pluginIter = instanceReferences.insert(pluginIter, make_pair(item.node.plugin, renderer->getPlugin(item.node.plugin)));
				}

				auto & plugin = pluginIter->second;
				if (!plugin) {
					Logger::log(Logger::Warning, "Instancer (", message.getPlugin() ,") referencing not existing plugin [", item.node.plugin, "]");
					break;
				}

				VRay::VUtils::ObjectID pluginId = { plugin.getId() };

				instance[3].setObjectID(pluginId);
				instancer[i + 1].setList(instance);
			}

			if (instancer.size() == inst.data.getCount() + 1) {
				success = plugin.setValue(message.getProperty(), instancer);
				Logger::log(Logger::Info,
					"Setting", message.getProperty(), "for plugin", message.getPlugin(), "size:",
					inst.data.getCount(), "\nSuccess:", success);
			}

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
		if (!renderer->getOrCreatePlugin(message.getPlugin(), message.getPluginType())) {
			Logger::log(Logger::Warning, "Failed to create plugin:", message.getPlugin());
		} else {
			Logger::log(Logger::Info, "Created plugin", message.getPlugin(), "with type:", message.getPluginType());
		}
	} else if (message.getPluginAction() == VRayMessage::PluginAction::Remove) {
		VRay::Plugin plugin = renderer->getPlugin(message.getPlugin().c_str());
		if (plugin) {
			if (!renderer->removePlugin(plugin)) {
				auto err = renderer->getLastError();
				Logger::log(Logger::Error, err.toString());
			}
			Logger::log(Logger::Info, "Removed plugin", message.getPlugin());
		} else {
			Logger::log(Logger::Warning, "Failed to find plugin to remove:", message.getPlugin());
		}
	}
}

void RendererController::rendererMessage(const VRayMessage & message) {
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
		renderer->startSync();
		Logger::log(Logger::Info, "Renderer::start");

		if (type == VRayMessage::RendererType::Animation) {
			renderer->waitForImageReady();
			imageDone(*renderer, nullptr);
		}

		break;
	case VRayMessage::RendererAction::Stop:{
		std::lock_guard<std::mutex> l(rendererMtx);
		Logger::log(Logger::Info, "Renderer::stop");
		renderer->stop();
		break;
	}
	case VRayMessage::RendererAction::Free:
		Logger::log(Logger::Info, "Renderer::free");
		stopRenderer();
		elementsToSend.clear();

		break;
	case VRayMessage::RendererAction::Init:
	{
		Logger::log(Logger::Info, "Renderer::init");

		VRay::RendererOptions options;
		options.keepRTRunning = type == VRayMessage::RendererType::RT;
		options.noDR = true;
		options.showFrameBuffer = showVFB;
		renderer.reset(new VRay::VRayRenderer(options));
		if (!renderer) {
			completed = false;
		} else {
			auto mode = type == VRayMessage::RendererType::RT ? VRay::RendererOptions::RENDER_MODE_RT_CPU : VRay::RendererOptions::RENDER_MODE_PRODUCTION;
			completed = renderer->setRenderMode(mode);
			if (showVFB) {
				renderer->vfb.show(true, true);
			} else {
				renderer->vfb.show(false, false);
			}

			renderer->setOnRTImageUpdated<RendererController, &RendererController::imageUpdate>(*this);
			renderer->setOnImageReady<RendererController, &RendererController::imageDone>(*this);
			renderer->setOnBucketReady<RendererController, &RendererController::bucketReady>(*this);

			renderer->setOnDumpMessage<RendererController, &RendererController::vrayMessageDumpHandler>(*this);
		}

		break;
	}
	case VRayMessage::RendererAction::SetRenderMode:
		completed = renderer->setRenderMode(static_cast<VRay::RendererOptions::RenderMode>(message.getValue<AttrSimpleType<int>>()->m_Value));
		Logger::log(Logger::Info, "Renderer::setRenderMode", message.getValue<AttrSimpleType<int>>()->m_Value);

		break;
	case VRayMessage::RendererAction::Resize:
		int width, height;
		message.getRendererSize(width, height);
		completed = renderer->setImageSize(width, height);

		Logger::log(Logger::Info, "Renderer::resize", width, "x", height);
		break;
	case VRayMessage::RendererAction::Commit:
		completed = renderer->commit();
		Logger::log(Logger::Info, "Renderer::commit");
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
		break;
	}
	case VRayMessage::RendererAction::GetImage:
		elementsToSend.insert(static_cast<VRay::RenderElement::Type>(message.getValue<AttrSimpleType<int>>()->m_Value));
		break;
	case VRayMessage::RendererAction::SetQuality:
		jpegQuality = message.getValue<AttrSimpleType<int>>()->m_Value;
		jpegQuality = std::max(0, std::min(100, jpegQuality));
		break;
	case VRayMessage::RendererAction::SetCurrentCamera: {
		auto cameraPlugin = renderer->getPlugin(message.getValue<AttrSimpleType<std::string>>()->m_Value);
		if (!cameraPlugin) {
			Logger::log(Logger::Warning, "Failed to find", message.getValue<AttrSimpleType<std::string>>()->m_Value, "to set as current camera.");
			completed = false;
		} else {
			completed = renderer->setCamera(cameraPlugin);
		}
		break;
	}
	case VRayMessage::RendererAction::SetCommitAction:
		switch (static_cast<CommitAction>(message.getValue<AttrSimpleType<int>>()->m_Value)) {
		case CommitAction::CommitNow:
			renderer->commit(false);
			Logger::log(Logger::Info, "Renderer::commit(false)");
			break;
		case CommitAction::CommitNowForce:
			renderer->commit(true);
			Logger::log(Logger::Info, "Renderer::commit(true)");
			break;
		case CommitAction::CommitAutoOn:
			renderer->setAutoCommit(true);
			Logger::log(Logger::Info, "Renderer::setAutoCommit(true)");
			break;
		case CommitAction::CommitAutoOff:
			renderer->setAutoCommit(false);
			Logger::log(Logger::Info, "Renderer::setAutoCommit(false)");
			break;
		default:
			Logger::log(Logger::Warning, "Invalid CommitAction: ", message.getValue<AttrSimpleType<int>>()->m_Value);
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

void RendererController::sendImages(VRay::VRayImage * img, VRayBaseTypes::AttrImage::ImageType fullImageType, VRayBaseTypes::ImageSourceType sourceType) {
	AttrImageSet set(sourceType);

	auto allElements = renderer->getRenderElements();

	for (const auto &type : elementsToSend) {
		switch (type) {
		case VRay::RenderElement::Type::NONE:
		{
			size_t size;
			int width, height;
			AttrImage attrImage;
			if (fullImageType == VRayBaseTypes::AttrImage::ImageType::RGBA_REAL) {
				VRay::AColor * data = img->getPixelData(size);
				size *= sizeof(VRay::AColor);
				img->getSize(width, height);
				attrImage = AttrImage(data, size, fullImageType, width, height);
			} else if (fullImageType == VRayBaseTypes::AttrImage::ImageType::JPG) {
				std::unique_ptr<VRay::Jpeg> jpeg(img->getJpeg(size, jpegQuality));
				img->getSize(width, height);
				attrImage = AttrImage(jpeg.get(), size, VRayBaseTypes::AttrImage::ImageType::JPG, width, height);
			}
			set.images.emplace(static_cast<VRayBaseTypes::RenderChannelType>(type), std::move(attrImage));
			break;
		}
		case VRay::RenderElement::Type::ZDEPTH:
		case VRay::RenderElement::Type::REALCOLOR:
		case VRay::RenderElement::Type::NORMALS:
		case VRay::RenderElement::Type::RENDERID:
			try {
				auto element = allElements.getByType(type);
				if (element) {
					VRay::RenderElement::PixelFormat pixelFormat = element.getDefaultPixelFormat();

					VRayBaseTypes::AttrImage::ImageType imgType = VRayBaseTypes::AttrImage::ImageType::NONE;
					switch (pixelFormat) {
					case VRay::RenderElement::PF_BW_FLOAT:
						imgType = VRayBaseTypes::AttrImage::ImageType::BW_REAL;
						break;
					case VRay::RenderElement::PF_RGB_FLOAT:
						imgType = VRayBaseTypes::AttrImage::ImageType::RGB_REAL;
						break;
					case VRay::RenderElement::PF_RGBA_FLOAT:
						imgType = VRayBaseTypes::AttrImage::ImageType::RGBA_REAL;
						break;
					}


					if (imgType == VRayBaseTypes::AttrImage::ImageType::NONE) {
						Logger::log(Logger::Error, "Unsupported pixel format!", pixelFormat);
					} else {
						Logger::log(Logger::Error, "Render channel:", type, "Pixel format:", pixelFormat);
						VRay::VRayImage *img = element.getImage();

						int width, height;
						img->getSize(width, height);
						size_t size;
						VRay::AColor *data = img->getPixelData(size);
						size *= sizeof(VRay::AColor);

						set.images.emplace(static_cast<VRayBaseTypes::RenderChannelType>(type), VRayBaseTypes::AttrImage(data, size, imgType, width, height));
					}
				}
			} catch (VRay::InvalidRenderElementErr &e) {
				Logger::log(Logger::Warning, e.what(), static_cast<int>(type));
			} catch (VRay::VRayException &e) {
				Logger::log(Logger::Error, e.what(), static_cast<int>(type));
			}
		break;

		default:
			Logger::log(Logger::Warning, "Element requested, but not found", static_cast<int>(type));
		}
	}

	sendFn(VRayMessage::createMessage(std::move(set)));
}


void RendererController::stopRenderer() {
	Logger::log(Logger::Info, "Freeing renderer object");

	std::lock_guard<std::mutex> l(rendererMtx);
	if (renderer) {

		renderer->setOnRTImageUpdated(nullptr);
		renderer->setOnImageReady(nullptr);
		renderer->setOnBucketReady(nullptr);
		renderer->setOnDumpMessage(nullptr);
		renderer->vfb.show(false, false);

		renderer->stop();
		//renderer->reset();
		renderer.release();
	}
}


void RendererController::imageUpdate(VRay::VRayRenderer &, VRay::VRayImage * img, void * arg) {
	(void)arg;
	std::lock_guard<std::mutex> l(rendererMtx);
	if (renderer && !renderer->isAborted()) {
		sendImages(img, VRayBaseTypes::AttrImage::ImageType::JPG, VRayBaseTypes::ImageSourceType::RtImageUpdate);
	}
}


void RendererController::imageDone(VRay::VRayRenderer &, void * arg) {
	(void)arg;
	std::lock_guard<std::mutex> l(rendererMtx);

	if (renderer) {
		if (!renderer->isAborted()) {
			VRay::VRayImage * img = renderer->getImage();
			sendImages(img, VRayBaseTypes::AttrImage::ImageType::RGBA_REAL, VRayBaseTypes::ImageSourceType::ImageReady);
		}

		VRayMessage::RendererStatus status = renderer->isAborted() ? VRayMessage::RendererStatus::Abort : VRayMessage::RendererStatus::Continue;
		sendFn(VRayMessage::createMessage(status, this->currentFrame));

		if (type == VRayMessage::RendererType::Animation) {
			Logger::log(Logger::Debug, "Animation frame completed ", currentFrame);
		} else {
			Logger::log(Logger::Info, "Renderer::OnImageReady");
		}
	}
}

void RendererController::bucketReady(VRay::VRayRenderer &, int x, int y, const char * host, VRay::VRayImage * img, void * arg) {
	(void)arg;
	std::lock_guard<std::mutex> l(rendererMtx);

	if (renderer && !renderer->isAborted()) {
		AttrImageSet set(VRayBaseTypes::ImageSourceType::BucketImageReady);

		int width, height;
		size_t size;
		img->getSize(width, height);
		const VRay::AColor * data = img->getPixelData(size);
		size *= sizeof(VRay::AColor);
		set.images.emplace(VRayBaseTypes::RenderChannelType::RenderChannelTypeNone, VRayBaseTypes::AttrImage(data, size, VRayBaseTypes::AttrImage::ImageType::RGBA_REAL, width, height, x, y));

		sendFn(VRayMessage::createMessage(std::move(set)));

		Logger::log(Logger::Debug, "Sending bucket bucket", x, "->", width + x, ":", y, "->", height + y);
	}
}


void RendererController::vrayMessageDumpHandler(VRay::VRayRenderer &, const char * msg, int level, void * arg) {
	RendererController * rc = reinterpret_cast<RendererController*>(arg);

	if (level <= VRay::MessageError) {
		Logger::log(Logger::Error, "VRAY:", msg);
	} else if (level <= VRay::MessageWarning) {
		Logger::log(Logger::Warning, "VRAY:", msg);
	} else {
		Logger::log(Logger::Info, "VRAY:", msg);
	}

	sendFn(VRayMessage::createMessage(VRayBaseTypes::AttrSimpleType<std::string>(msg)));
}
