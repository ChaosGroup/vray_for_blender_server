#define VRAY_RUNTIME_LOAD_SECONDARY
#include <algorithm>
#include <unordered_map>
#include "renderer_controller.h"
#include "utils/logger.h"

using namespace VRayBaseTypes;
using namespace std;

RendererController::RendererController(zmq::context_t & zmqContext, uint64_t clientId, ClientType type, bool showVFB)
	: runState(IDLE)
	, clType(type)
	, clientId(clientId)
	, zmqContext(zmqContext)
	, renderer(nullptr)
	, showVFB(showVFB)
	, currentFrame(-1000)
	, type(VRayMessage::RendererType::None)
	, jpegQuality(60)
	, viewportType(VRayBaseTypes::AttrImage::ImageType::JPG) {
}

RendererController::~RendererController() {
	stop();
	stopRenderer();
}

void RendererController::stopRenderer() {
	Logger::log(Logger::Debug, "Freeing renderer object");
	if (renderer) {
		std::unique_lock<std::mutex> rendLock(rendererMtx);
		delete renderer;
		renderer = nullptr;
	}
}

void RendererController::handle(VRayMessage && message) {
	bool success = false;
	try {
		switch (message.getType()) {
		case VRayMessage::Type::ChangePlugin:
			if (!renderer) {
				Logger::getInstance().log(Logger::Warning, "Can't change plugin - no renderer loaded!");
				return;
			}
			this->pluginMessage(std::move(message));
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
			this->rendererMessage(std::move(message));
			break;
		}
		default:
			Logger::getInstance().log(Logger::Error, "Unknown message type");
			return;
		}
	} catch (VRay::VRayException & e) {
		Logger::getInstance().log(Logger::Error, e.what());
		transitionState(runState, IDLE);
	} catch (std::exception & e) {
		Logger::getInstance().log(Logger::Error, e.what());
	}
}

std::pair<bool, VRay::Value> RendererController::toVrayValue(const VRayBaseTypes::AttrValue & val, VRayMessage & message) {
	using namespace VRayBaseTypes;
	switch (val.type) {
	case ValueType::ValueTypeListValue: {
		const auto & list = val.as<AttrListValue>();
		VRay::ValueList vList(list.getCount());
		for (int c = 0; c < list.getCount(); c++) {
			auto converted = toVrayValue((*list)[c], message);
			if (!converted.first) {
				return{false, VRay::Value()};
			}
			vList[c] = converted.second;
		}
		return {true, VRay::Value(vList)};
	}
	case ValueType::ValueTypeListPlugin: {
		const auto & list = val.as<AttrListPlugin>();
		VRay::ValueList vList(list.getCount());
		for (int c = 0; c < list.getCount(); ++c) {
			const auto & plugin = (*list)[c];
			auto pluginRef = plugin.plugin;
			if (!plugin.output.empty()) {
				pluginRef += "::" + plugin.output;
			}
			auto plg = renderer->getPlugin(pluginRef);
			if (!plg) {
				auto buffLog = Logger::getInstance().makeBuffered();
				buffLog.log(Logger::Error, "Failed setting:", message.getProperty(), "=", pluginRef, "for plugin", message.getPlugin());
				delayedMessages[plugin.plugin].emplace_back(std::move(message), std::move(buffLog));
				return {false, VRay::Value()};
			}
			vList[c] = VRay::Value(plg);
		}
		return {true, VRay::Value(vList)};
	}
	default: return {false, VRay::Value()};
	}
}


void RendererController::pluginMessage(VRayMessage && message) {
	bool success = true;
	if (message.getPluginAction() == VRayMessage::PluginAction::Update) {
		VRay::Plugin plugin = renderer->getPlugin(message.getPlugin());
		if (!plugin) {
			Logger::getInstance().log(Logger::Warning, "Failed to load plugin: ", message.getPlugin());
			return;
		}

		switch (message.getValueType()) {
		case VRayBaseTypes::ValueType::ValueTypeMatrix:
			success = plugin.setValueAtTime(message.getProperty(), *message.getValue<const VRay::Matrix>(), currentFrame);
			Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
				"\").setValueAtTime(\"", message.getProperty(), "\",", *message.getValue<const VRay::Matrix>(), ", ", currentFrame, "); // success == ", success);

			break;
		case VRayBaseTypes::ValueType::ValueTypeTransform:
			success = plugin.setValueAtTime(message.getProperty(), *message.getValue<const VRay::Transform>(), currentFrame);
			Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
				"\").setValueAtTime(\"", message.getProperty(), "\",", *message.getValue<const VRay::Transform>(), ", ", currentFrame, "); // success == ", success);

			break;
		case VRayBaseTypes::ValueType::ValueTypeInt:
			success = plugin.setValueAtTime(message.getProperty(), *message.getValue<int>(), currentFrame);
			Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
				"\").setValueAtTime(\"", message.getProperty(), "\",", *message.getValue<int>(), ", ", currentFrame, "); // success == ", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeFloat:
			success = plugin.setValueAtTime(message.getProperty(), *message.getValue<float>(), currentFrame);
			Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
				"\").setValueAtTime(\"", message.getProperty(), "\",", *message.getValue<float>(), ", ", currentFrame, "); // success == ", success);
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
				success = plugin.setValueAtTime(message.getProperty(), VRay::Plugin(), currentFrame);
				Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
					"\").setValueAtTime(\"", message.getProperty(), "\", \"NULL\", ", currentFrame, "); // success == ", success);
			} else {
				if (!renderer->getPlugin(attrPlugin.plugin)) {
					Logger::log(Logger::Debug, "Plugin [", message.getPlugin(), "] references (",  attrPlugin.plugin, ") which is not yet exported - delaying.");

					// save message and it's error if it is not processed before commit
					auto buffLog = Logger::getInstance().makeBuffered();
					buffLog.log(Logger::Error, "Failed setting:", message.getProperty(), "=", pluginData, "for plugin", message.getPlugin());

					delayedMessages[attrPlugin.plugin].emplace_back(std::move(message), std::move(buffLog));
				} else {
					success = plugin.setValueAsStringAtTime(message.getProperty(), pluginData, currentFrame);

					Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
						"\").setValueAsStringAtTime(\"", message.getProperty(), "\",\"", pluginData, "\", ", currentFrame, "); // success == ", success);
				}
			}

			break;
		}
		case VRayBaseTypes::ValueType::ValueTypeVector:
			success = plugin.setValueAtTime(message.getProperty(), *message.getValue<VRay::Vector>(), currentFrame);

			Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
				"\").setValueAtTime(\"", message.getProperty(), "\",", *message.getValue<VRay::Vector>(), ", ", currentFrame, "); // success == ", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeString:
			if (message.getValueSetter() == VRayMessage::ValueSetter::AsString) {
				success = plugin.setValueAsStringAtTime(message.getProperty(), *message.getValue<std::string>(), currentFrame);
				Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
					"\").setValueAsStringAtTime(\"", message.getProperty(), ",", *message.getValue<std::string>(), "\", ", currentFrame, "); // success == ", success);
			} else {
				success = plugin.setValueAtTime(message.getProperty(), *message.getValue<std::string>(), currentFrame);
				Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
					"\").setValueAtTime(\"", message.getProperty(), "\",\"", *message.getValue<std::string>(), "\", ", currentFrame, "); // success == ", success);
			}
			break;
		case VRayBaseTypes::ValueType::ValueTypeColor:
			success = plugin.setValueAtTime(message.getProperty(), *message.getValue<VRay::Color>(), currentFrame);
			Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
					"\").setValueAtTime(\"", message.getProperty(), "\",", *message.getValue<VRay::Color>(), ", ", currentFrame, "); // success == ", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeAColor:
			success = plugin.setValueAtTime(message.getProperty(), *message.getValue<VRay::AColor>(), currentFrame);
			Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
					"\").setValueAtTime(\"", message.getProperty(), "\",", *message.getValue<VRay::AColor>(), ", ", currentFrame, "); // success == ", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeListInt:
			success = plugin.setValueAtTime(message.getProperty(),
				**message.getValue<VRayBaseTypes::AttrListInt>(),
				message.getValue<VRayBaseTypes::AttrListInt>()->getBytesCount(), currentFrame);

			Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
				"\").setValueAtTime(\"", message.getProperty(), "\",", *message.getValue<VRayBaseTypes::AttrListInt>()->getData(), ", ", currentFrame, "); // success == ", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeListFloat:
			success = plugin.setValueAtTime(message.getProperty(),
				**message.getValue<VRayBaseTypes::AttrListFloat>(),
				message.getValue<VRayBaseTypes::AttrListFloat>()->getBytesCount(), currentFrame);

			Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
				"\").setValueAtTime(\"", message.getProperty(), "\",", *message.getValue<VRayBaseTypes::AttrListFloat>()->getData(), ", ", currentFrame, "); // success == ", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeListVector:
			success = plugin.setValueAtTime(message.getProperty(),
				**message.getValue<VRayBaseTypes::AttrListVector>(),
				message.getValue<VRayBaseTypes::AttrListVector>()->getBytesCount(), currentFrame);

			Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
				"\").setValueAtTime(\"", message.getProperty(), "\",", *message.getValue<VRayBaseTypes::AttrListVector>()->getData(), ", ", currentFrame, "); // success == ", success);
			break;
		case VRayBaseTypes::ValueType::ValueTypeListPlugin:
		{
			const VRayBaseTypes::AttrListPlugin & plist = *message.getValue<VRayBaseTypes::AttrListPlugin>();
#if 1 // enable this for ValueList
			VRay::ValueList pluginList(plist.getCount());
			bool delayed = false;
			for (int c = 0; c < plist.getCount(); ++c) {
				const auto & messagePlugin = (*plist)[c];
				const auto vrayPlugin = renderer->getPlugin(messagePlugin.plugin);
				if (!vrayPlugin) {
					Logger::log(Logger::Debug, "Plugin [", message.getPlugin(), "] references (", messagePlugin.plugin, ") which is not yet exported - delaying.");

					auto buffLog = Logger::getInstance().makeBuffered();
					buffLog.log(Logger::Error, "Failed setting:", message.getProperty(), "=", messagePlugin.plugin, "for plugin", message.getPlugin());

					delayed = true;
					delayedMessages[messagePlugin.plugin].emplace_back(std::move(message), std::move(buffLog));
					break;
				}
				pluginList[c] = VRay::Value(vrayPlugin);
			}
			if (!delayed) {
				success = plugin.setValueAtTime(message.getProperty(), pluginList, currentFrame);

				Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
					"\").setValueAtTime(\"", message.getProperty(), "\",", *message.getValue<VRayBaseTypes::AttrListPlugin>()->getData(), ", ", currentFrame, "); // success == ", success);
			}
#else
			VRay::VUtils::ValueRefList pluginList(plist.getCount());

			Logger::log(Logger::APIDump, "{VUtils::ValueRefList l(", plist.getCount(), ");");

			for (int c = 0; c < plist.getCount(); ++c) {
				const auto & messagePlugin = (*plist)[c];
				const auto & vrayPlugin = renderer->getPlugin(messagePlugin.plugin);
				VRay::VUtils::ObjectID pluginId = { VRay::NO_ID };

				if (!vrayPlugin) {
					Logger::log(Logger::Warning, "Missing plugin", messagePlugin.plugin, "referenced in plugin list for", message.getPlugin());
				} else {
					pluginId = {vrayPlugin.getId()};
				}
				Logger::log(Logger::APIDump, "l[", c, "].setObjectID(VUtils::ObjectID{renderer.getPlugin(\"", messagePlugin.plugin, "\").getId()});");

				pluginList[c].setObjectID(pluginId);
			}
			success = plugin.setValueAtTime(message.getProperty(), pluginList, currentFrame);
			Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
				"\").setValueAtTime(\"", message.getProperty(), "\",l, ", currentFrame, ");} // success == ", success);
#endif
			break;

		}
		case VRayBaseTypes::ValueType::ValueTypeListString:
		{
			const VRayBaseTypes::AttrListString & slist = *message.getValue<VRayBaseTypes::AttrListString>();
			VRay::VUtils::CharStringRefList stringList(slist.getCount());

			for (int c = 0; c < slist.getCount(); ++c) {
				stringList[c].set((*slist)[c].c_str());
			}
			success = plugin.setValueAtTime(message.getProperty(), stringList, currentFrame);

			Logger::log(Logger::APIDump, "renderer.getPlugin(\"", message.getPlugin(),
				"\").setValueAtTime(\"", message.getProperty(), "\",", *message.getValue<VRayBaseTypes::AttrListString>()->getData(), ", ", currentFrame, "); // success == ", success);
			break;
		}
		case VRayBaseTypes::ValueType::ValueTypeListValue:{
			auto res = toVrayValue(message.getAttrValue(), message);
			success = res.first;
			if (success) {
				success = plugin.setValueAtTime(message.getProperty(), res.second, currentFrame);
			}
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

			success = plugin.setValueAtTime(message.getProperty(), map_channels, currentFrame);

			// TODO: maybe implement - tricky as it usually contains alot of data
			Logger::log(Logger::APIDump, "// AttrMapChannels unimplemented log success == ", success);
			break;
		}
		case VRayBaseTypes::ValueType::ValueTypeInstancer:
		{
			bool delayed = false;
			const VRayBaseTypes::AttrInstancer & inst = *message.getValue<VRayBaseTypes::AttrInstancer>();
			VRay::VUtils::ValueRefList instancer(inst.data.getCount() + 1);
			instancer[0] = VRay::VUtils::Value(inst.frameNumber);

			auto logBuff = Logger::getInstance().makeBuffered();

			logBuff.log(Logger::APIDump, "{\n\tVUtils::ValueRefList i(", inst.data.getCount() + 1, ");\n\ti[0]=VUtils::Value(", inst.frameNumber, ");");

			for (int i = 0; i < inst.data.getCount(); ++i) {
				const VRayBaseTypes::AttrInstancer::Item &item = (*inst.data)[i];

				VRay::VUtils::ValueRefList instance(4);

				const VRay::Transform * tm, *vel;
				tm = reinterpret_cast<const VRay::Transform*>(&item.tm);
				vel = reinterpret_cast<const VRay::Transform*>(&item.vel);

				instance[0].setDouble(item.index);
				instance[1].setTransform(*tm);
				instance[2].setTransform(*vel);

				auto plugin = renderer->getPlugin(item.node.plugin);
				if (!plugin) {
					Logger::log(Logger::Debug, "Plugin [", message.getPlugin(), "] references (", item.node.plugin, ") which is not yet exported - delaying.");

					auto buffLog = Logger::getInstance().makeBuffered();
					buffLog.log(Logger::Warning, "Instancer (", message.getPlugin() ,") referencing not existing plugin [", item.node.plugin, "]");

					delayedMessages[item.node.plugin].emplace_back(std::move(message), std::move(buffLog));
					delayed = true;
					break;
				}

				VRay::VUtils::ObjectID pluginId = { plugin.getId() };
				logBuff.log(Logger::APIDump, "\t{\n\t\tVUtils::ValueRefList in(4);");
				logBuff.log(Logger::APIDump, "\t\tin[0].setDouble(", item.index, ");");
				logBuff.log(Logger::APIDump, "\t\tin[1].setTransform(", *tm, ");");
				logBuff.log(Logger::APIDump, "\t\tin[2].setTransform(", *vel, ");");
				logBuff.log(Logger::APIDump, "\t\tin[3].setObjectID(VUtils::ObjectID{", plugin.getId(), "});/*", item.node.plugin, "*/");
				logBuff.log(Logger::APIDump, "\t\ti[", i + 1, "].setList(in);\n\t}");

				instance[3].setObjectID(pluginId);
				instancer[i + 1].setList(instance);
			}

			if (!delayed) {
				if (instancer.size() == inst.data.getCount() + 1) {
					success = plugin.setValueAtTime(message.getProperty(), instancer, currentFrame);
				}

				logBuff.log(Logger::APIDump, "\trenderer.getPlugin(\"", message.getPlugin(), "\").setValueAtTime(\"", message.getProperty(), "\",i, ", currentFrame, ");\n} // success == ", success);

				Logger::getInstance().log(logBuff); // dump buff
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
		const bool created = renderer->getOrCreatePlugin(message.getPlugin(), message.getPluginType());
		if (!created) {
			Logger::log(Logger::Warning, "Failed to create plugin:", message.getPlugin());
		} else {
			auto toInsert = delayedMessages.find(message.getPlugin());
			if (toInsert != delayedMessages.end()) {
				for (auto & msg : toInsert->second) {
					Logger::log(Logger::Debug, "Inserting delayed plugin [", msg.first.getPlugin(), "] referencing (", toInsert->first, ").");
					handle(std::move(msg.first));
				}
				delayedMessages.erase(toInsert);
			}
		}
		Logger::log(Logger::APIDump, "renderer.getOrCreatePlugin(\"", message.getPlugin(), "\",\"", message.getPluginType(), "\"); // success == ", created);
	} else if (message.getPluginAction() == VRayMessage::PluginAction::Remove) {
		bool removed = false;
		VRay::Plugin plugin = renderer->getPlugin(message.getPlugin().c_str());
		if (plugin) {
			if (!renderer->removePlugin(plugin)) {
				auto err = renderer->getLastError();
				Logger::log(Logger::Error, err.toString());
			} else {
				removed = true;
				Logger::log(Logger::Debug, "Removed plugin", message.getPlugin());
			}
		} else {
			Logger::log(Logger::Warning, "Failed to find plugin to remove:", message.getPlugin());
		}

		Logger::log(Logger::APIDump, "renderer.removePlugin(renderer.getPlugin(\"", message.getPlugin(), "\")); // success == ", removed);
	} else if (message.getPluginAction() == VRayMessage::PluginAction::Replace) {
		bool replaced = false;
		VRay::Plugin oldPlugin = renderer->getPlugin(message.getPlugin());
		VRay::Plugin newPlugin = renderer->getPlugin(message.getPluginNew());
		if (oldPlugin && newPlugin) {
			if (!renderer->replacePlugin(oldPlugin, newPlugin)) {
				auto err = renderer->getLastError();
				Logger::log(Logger::Error, "Failed to replace plugin:", message.getPlugin(), "With", message.getPluginNew(), " Error:", err.toString());
			} else {
				replaced = true;
				Logger::log(Logger::Debug, "Replaced plugin", message.getPlugin());
			}
		} else {
			Logger::log(Logger::Warning, "Failed to find plugin/s to replace:", message.getPlugin(), message.getPluginNew());
		}

		Logger::log(Logger::APIDump, "renderer.replacePlugin(renderer.getPlugin(\"", message.getPlugin(), "\"), renderer.getPlugin(\"", message.getPluginNew() ,"\")); // success == ", replaced);
	}
}

void RendererController::rendererMessage(VRayMessage && message) {
	std::unique_lock<std::mutex> rendLock(rendererMtx);
	if (!renderer && message.getRendererAction() != VRayMessage::RendererAction::Init) {
		return;
	}
	bool completed = true;
	switch (message.getRendererAction()) {
	case VRayMessage::RendererAction::SetCurrentFrame:
		Logger::log(Logger::APIDump, "renderer.setCurrentFrame(", message.getValue<AttrSimpleType<float>>()->value, ");");
		currentFrame = message.getValue<AttrSimpleType<float>>()->value;
		renderer->setCurrentFrame(message.getValue<AttrSimpleType<float>>()->value);
		break;
	case VRayMessage::RendererAction::SetCurrentTime:
		Logger::log(Logger::APIDump, "renderer.setCurrentTime(", message.getValue<AttrSimpleType<float>>()->value, ");");
		currentFrame = message.getValue<AttrSimpleType<float>>()->value;
		renderer->setCurrentTime(message.getValue<AttrSimpleType<float>>()->value);
		break;
	case VRayMessage::RendererAction::ClearFrameValues:
		Logger::log(Logger::APIDump, "renderer.clearAllPropertyValuesUpToTime(", message.getValue<AttrSimpleType<float>>()->value, ");");
		completed = renderer->clearAllPropertyValuesUpToTime(message.getValue<AttrSimpleType<float>>()->value);
		break;
	case VRayMessage::RendererAction::Pause:
		Logger::log(Logger::APIDump, "renderer.pause();");
		completed = renderer->pause();
		break;
	case VRayMessage::RendererAction::Resume:
		Logger::log(Logger::APIDump, "renderer.resume()");
		completed = renderer->resume();
		break;
	case VRayMessage::RendererAction::Start:
		Logger::log(Logger::APIDump, "renderer.startSync();");
		renderer->startSync();
		break;
	case VRayMessage::RendererAction::Stop:{
		Logger::log(Logger::APIDump, "renderer.stop();");
		renderer->stop();
		break;
	}
	case VRayMessage::RendererAction::Free:
		renderer->stop();
		{
			std::lock_guard<std::mutex> lk(elemsToSendMtx);
			elementsToSend.clear();
		}
		delete renderer;
		renderer = nullptr;

		break;
	case VRayMessage::RendererAction::Init:
	{
		VRay::RendererOptions options;
		options.keepRTRunning = type == VRayMessage::RendererType::RT;
		options.noDR = true;
		options.showFrameBuffer = showVFB;
		Logger::log(Logger::APIDump, "RendererOptions o;o.keepRTRunning=", options.keepRTRunning, ";o.noDR=true;o.showFrameBuffer=", showVFB, ";VRayRenderer renderer(o);");
		renderer = new VRay::VRayRenderer(options);

		const bool useAnimatedValues = false;
		renderer->useAnimatedValues(useAnimatedValues);
		Logger::log(Logger::APIDump, "renderer.useAnimatedValues(",useAnimatedValues,"); // success == ", completed);

		renderer->setOnProgress<RendererController, &RendererController::onProgress>(*this);
		renderer->setOnRTImageUpdated<RendererController, &RendererController::imageUpdate>(*this);
		renderer->setOnImageReady<RendererController, &RendererController::imageDone>(*this);
		renderer->setOnBucketReady<RendererController, &RendererController::bucketReady>(*this);

		renderer->setOnDumpMessage<RendererController, &RendererController::vrayMessageDumpHandler>(*this);

		break;
	}
	case VRayMessage::RendererAction::SetRenderMode:
		completed = renderer->setRenderMode(static_cast<VRay::RendererOptions::RenderMode>(message.getValue<AttrSimpleType<int>>()->value));
		Logger::log(Logger::APIDump, "renderer.setRenderMode(RendererOptions::RenderMode(", message.getValue<AttrSimpleType<int>>()->value, ")); // success == ", completed);

		break;
	case VRayMessage::RendererAction::Resize:
		int width, height;
		message.getRendererSize(width, height);
		if (width > 1 && height > 1) {
			completed = renderer->setImageSize(width, height);
		} else {
			completed = false;
		}

		Logger::log(Logger::APIDump, "renderer.setImageSize(", width, ",", height, "); // success == ", completed);
		break;
	case VRayMessage::RendererAction::AddHosts:
		completed = 0 == renderer->addHosts(message.getValue<AttrSimpleType<std::string>>()->value);
		Logger::log(Logger::APIDump, "renderer.addHosts(\"", message.getValue<AttrSimpleType<std::string>>()->value, "\"); // success == ", completed);
		break;
	case VRayMessage::RendererAction::RemoveHosts:
		completed = 0 == renderer->removeHosts(message.getValue<AttrSimpleType<std::string>>()->value);
		Logger::log(Logger::APIDump, "renderer.removeHosts(\"", message.getValue<AttrSimpleType<std::string>>()->value, "\"); // success == ", completed);
		break;
	case VRayMessage::RendererAction::LoadScene:
		completed = 0 == renderer->load(message.getValue<AttrSimpleType<std::string>>()->value);
		Logger::log(Logger::APIDump, "renderer.load(\"", message.getValue<AttrSimpleType<std::string>>()->value, "\"); // success == ", completed);
		break;
	case VRayMessage::RendererAction::AppendScene:
		completed = 0 == renderer->append(message.getValue<AttrSimpleType<std::string>>()->value);
		Logger::log(Logger::APIDump, "renderer.append(\"", message.getValue<AttrSimpleType<std::string>>()->value, "\"); // success == ", completed);
		break;
	case VRayMessage::RendererAction::ExportScene:
	{
		VRay::VRayExportSettings exportParams;
		exportParams.useHexFormat = false;
		exportParams.compressed = false;

		completed = 0 == renderer->exportScene(message.getValue<AttrSimpleType<std::string>>()->value, exportParams);

		Logger::log(Logger::APIDump, "renderer.exportScene(\"", message.getValue<AttrSimpleType<std::string>>()->value, "\"); // success == ", completed);
		break;
	}
	case VRayMessage::RendererAction::GetImage:
	{
		std::lock_guard<std::mutex> lk(elemsToSendMtx);
		elementsToSend.insert(static_cast<VRay::RenderElement::Type>(message.getValue<AttrSimpleType<int>>()->value));
	}
		break;
	case VRayMessage::RendererAction::SetQuality:
		jpegQuality = message.getValue<AttrSimpleType<int>>()->value;
		jpegQuality = std::max(0, std::min(100, jpegQuality));
		break;
	case VRayMessage::RendererAction::SetCurrentCamera: {
		auto cameraPlugin = renderer->getPlugin(message.getValue<AttrSimpleType<std::string>>()->value);
		if (!cameraPlugin) {
			// lets try to delay, maybe out of order export
			Logger::log(Logger::Debug, "Plugin [", message.getPlugin(), "] references (", message.getValue<AttrSimpleType<std::string>>()->value, ") which is not yet exported - delaying.");

			auto buffLog = Logger::getInstance().makeBuffered();
			buffLog.log(Logger::Warning, "Failed to find", message.getValue<AttrSimpleType<std::string>>()->value, "to set as current camera.");

			delayedMessages[message.getValue<AttrSimpleType<std::string>>()->value].emplace_back(std::move(message), std::move(buffLog));
		} else {
			completed = renderer->setCamera(cameraPlugin);
		}
		Logger::log(Logger::APIDump, "renderer.setCamera(renderer.getPlugin(\"", message.getValue<AttrSimpleType<std::string>>()->value, "\")); // success == ", completed);
		break;
	}
	case VRayMessage::RendererAction::SetCommitAction: {
		bool isFlush = false;
		switch (static_cast<CommitAction>(message.getValue<AttrSimpleType<int>>()->value)) {
		case CommitAction::CommitNow:
			renderer->commit(false);
			Logger::log(Logger::APIDump, "renderer.commit(false);");
			isFlush = true;
			break;
		case CommitAction::CommitNowForce:
			renderer->commit(true);
			Logger::log(Logger::APIDump, "renderer.commit(true);");
			isFlush = true;
			break;
		case CommitAction::CommitAutoOn:
			renderer->setAutoCommit(true);
			Logger::log(Logger::APIDump, "renderer.setAutoCommit(true);");
			break;
		case CommitAction::CommitAutoOff:
			renderer->setAutoCommit(false);
			Logger::log(Logger::APIDump, "renderer.setAutoCommit(false);");
			break;
		default:
			Logger::log(Logger::Warning, "Invalid CommitAction: ", message.getValue<AttrSimpleType<int>>()->value);
		}

		if (isFlush) {
			for (const auto & ref : delayedMessages) {
				for (const auto & plg : ref.second) {
					Logger::getInstance().log(plg.second);
				}
			}
			delayedMessages.clear();
		}

	}
		break;
	case VRayMessage::RendererAction::SetVfbShow:
		if (message.getValue<AttrSimpleType<int>>()->value && showVFB) {
			renderer->vfb.show(true, false);
			Logger::log(Logger::APIDump, "renderer.vfb.show(true, false);");
		} else {
			renderer->vfb.show(false, false);
			Logger::log(Logger::APIDump, "renderer.vfb.show(false, false);");
		}
		break;
	case VRayMessage::RendererAction::SetViewportImageFormat:
		viewportType = static_cast<VRayBaseTypes::AttrImage::ImageType>(message.getValue<AttrSimpleType<int>>()->value);
		Logger::log(Logger::Debug, "Viewport image type set to", viewportType);
		break;
	default:
		Logger::log(Logger::Warning, "Invalid renderer action: ", static_cast<int>(message.getRendererAction()));
	}

	if (!completed) {
		const auto * err = renderer ? renderer->getLastError().toString() : "[empty] renderer";
		Logger::log(Logger::Warning,
			"Failed renderer action:", static_cast<int>(message.getRendererAction()),
			"\nerror:", err);
	}
}

void RendererController::sendImages(VRay::VRayImage * img, VRayBaseTypes::AttrImage::ImageType fullImageType, VRayBaseTypes::ImageSourceType sourceType) {
	AttrImageSet set(sourceType);

	auto allElements = renderer->getRenderElements();
	std::unordered_set<VRay::RenderElement::Type, std::hash<int>> elToSend;
	{
		std::lock_guard<std::mutex> lk(elemsToSendMtx);
		elToSend = elementsToSend;
	}

	for (const auto &type : elToSend) {
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
					default:
						imgType = VRayBaseTypes::AttrImage::ImageType::NONE;
					}


					if (imgType == VRayBaseTypes::AttrImage::ImageType::NONE) {
						Logger::log(Logger::Error, "Unsupported pixel format!", pixelFormat);
					} else {
						Logger::log(Logger::Debug, "Render channel:", type, "Pixel format:", pixelFormat);
						VRay::VRayImage *img = element.getImage();

						int width, height;
						img->getSize(width, height);
						size_t size;
						VRay::AColor *data = img->getPixelData(size);
						size *= sizeof(VRay::AColor);

						set.images.emplace(static_cast<VRayBaseTypes::RenderChannelType>(type), VRayBaseTypes::AttrImage(data, size, imgType, width, height));
						delete img;
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

	{
		lock_guard<mutex> lock(messageMtx);
		outstandingMessages.push(VRayMessage::msgImageSet(std::move(set)));
	}
}

void RendererController::onProgress(VRay::VRayRenderer & renderer, const char* msg, int elementNumber, int elementsCount, void *) {
	float progress = static_cast<float>(elementNumber) / elementsCount;

	lock_guard<mutex> lock(messageMtx);
	if (msg && *msg) {
		outstandingMessages.push(VRayMessage::msgRendererState(VRayMessage::RendererState::ProgressMessage, std::string(msg)));
	}
	outstandingMessages.push(VRayMessage::msgRendererState(VRayMessage::RendererState::Progress, progress));
}


void RendererController::imageUpdate(VRay::VRayRenderer &, VRay::VRayImage * img, void * arg) {
	if (renderer && !renderer->isAborted()) {
		sendImages(img, viewportType, VRayBaseTypes::ImageSourceType::RtImageUpdate);
	}
}


void RendererController::imageDone(VRay::VRayRenderer &, void * arg) {
	(void)arg;

	if (renderer) {
		if (!renderer->isAborted()) {
			VRay::VRayImage * img = renderer->getImage();
			sendImages(img, VRayBaseTypes::AttrImage::ImageType::RGBA_REAL, VRayBaseTypes::ImageSourceType::ImageReady);
			delete img;
		}

		VRayMessage::RendererState state = renderer->isAborted() ? VRayMessage::RendererState::Abort : VRayMessage::RendererState::Continue;
		{
			lock_guard<mutex> lock(messageMtx);
			outstandingMessages.push(VRayMessage::msgRendererState(state, this->currentFrame));
		}

		if (type == VRayMessage::RendererType::Animation) {
			Logger::log(Logger::Debug, "Animation frame completed ", currentFrame);
		} else {
			Logger::log(Logger::Debug, "Renderer::OnImageReady");
		}
	}
}

void RendererController::bucketReady(VRay::VRayRenderer &, int x, int y, const char *, VRay::VRayImage * img, void * arg) {
	(void)arg;

	if (renderer && !renderer->isAborted()) {
		AttrImageSet set(VRayBaseTypes::ImageSourceType::BucketImageReady);

		int width, height;
		size_t size;
		img->getSize(width, height);
		const VRay::AColor * data = img->getPixelData(size);
		size *= sizeof(VRay::AColor);
		set.images.emplace(VRayBaseTypes::RenderChannelType::RenderChannelTypeNone, VRayBaseTypes::AttrImage(data, size, VRayBaseTypes::AttrImage::ImageType::RGBA_REAL, width, height, x, y));

		{
			lock_guard<mutex> lock(messageMtx);
			outstandingMessages.push(VRayMessage::msgImageSet(std::move(set)));
		}
	}
}


void RendererController::vrayMessageDumpHandler(VRay::VRayRenderer &, const char * msg, int level, void *) {
	lock_guard<mutex> lock(messageMtx);
	outstandingMessages.push(VRayMessage::msgVRayLog(level, msg));
}

void RendererController::stop() {
	if (runState != RUNNING) {
		return;
	}
	transitionState(RUNNING, IDLE);

	assert(runnerThread.joinable() && "Missmatch between runState and actual thread state.");
	if (runnerThread.joinable()) {
		runnerThread.join();
	}

	{
		unique_lock<mutex> l(stateMtx);
		stateCond.wait(l, [this]() { return this->runState == IDLE; });
	}
}

bool RendererController::start() {
	if (runState != IDLE) {
		assert(runState == IDLE && "Invalid state transition");
		return runState == RUNNING;
	}
	transitionState(IDLE, STARTING);

	runnerThread = thread(&RendererController::run, this);

	{
		unique_lock<mutex> l(stateMtx);
		stateCond.wait(l, [this]() { return this->runState == RUNNING || this->runState == IDLE; });
	}
	return runState == RUNNING;
}

bool RendererController::isRunning() const {
	return runState == RUNNING;
}

void RendererController::transitionState(RunState current, RunState newState) {
	{
		lock_guard<mutex> l(stateMtx);
		assert(current == runState && "Unexpected state transition!");
		runState = newState;
	}
	stateCond.notify_all();
}

void RendererController::run() {
	zmq::socket_t zmqRendererSocket(zmqContext, ZMQ_DEALER);
	zmq::message_t emtpyFrame(0);

	try {
		int wait = EXPORTER_TIMEOUT * 2;
		zmqRendererSocket.setsockopt(ZMQ_SNDTIMEO, &wait, sizeof(wait));
		wait = EXPORTER_TIMEOUT * 2;
		zmqRendererSocket.setsockopt(ZMQ_RCVTIMEO, &wait, sizeof(wait));

		zmqRendererSocket.setsockopt(ZMQ_IDENTITY, clientId);
		zmqRendererSocket.connect("inproc://backend");
		// send handshake
		if (clType == ClientType::Exporter) {
			zmqRendererSocket.send(ControlFrame::make(ClientType::Exporter, ControlMessage::RENDERER_CREATE_MSG), ZMQ_SNDMORE);
		} else {
			zmqRendererSocket.send(ControlFrame::make(ClientType::Heartbeat, ControlMessage::HEARTBEAT_CREATE_MSG), ZMQ_SNDMORE);
		}
		zmqRendererSocket.send(emtpyFrame);
	} catch (zmq::error_t & ex) {
		Logger::log(Logger::Error, "Error while creating worker:", ex.what());
		transitionState(STARTING, IDLE);
		return;
	}
	zmq::pollitem_t backEndPoll = {zmqRendererSocket, 0, ZMQ_POLLIN | ZMQ_POLLOUT, 0};

	bool sendHB = false;

	transitionState(STARTING, RUNNING);
	while (runState == RUNNING) {
		bool diWork = false;
		int pollRes = 0;
		try {
			pollRes = zmq::poll(&backEndPoll, 1, 10);
		} catch (zmq::error_t & ex) {
			if (ex.num() != ETERM) {
				Logger::log(Logger::Error, "Error while polling for messages:", ex.what());
			}
			break;
		}

		if (backEndPoll.revents & ZMQ_POLLIN) {
			diWork = true;
			zmq::message_t ctrlMsg, payloadMsg;
			bool recv = false;
			try {
				recv = zmqRendererSocket.recv(&ctrlMsg);
				assert(recv && "Failed recv for ControlFrame while poll returned ZMQ_POLLIN event.");
				recv = zmqRendererSocket.recv(&payloadMsg);
				assert(recv && "Failed recv for payload while poll returned ZMQ_POLLIN event.");
			} catch (zmq::error_t & ex) {
				if (ex.num() != ETERM) {
					Logger::log(Logger::Error, "Error while renderer is receiving message:", ex.what());
				}
				transitionState(RUNNING, IDLE);
				return;
			}

			ControlFrame frame(ctrlMsg);

			assert(!!frame && "Client sent malformed control frame");

			if (frame.control == ControlMessage::DATA_MSG) {
				handle(VRayMessage::fromZmqMessage(payloadMsg));
			} else if (frame.control == ControlMessage::PING_MSG) {
				sendHB = true;
			}
		}

		if (backEndPoll.revents & ZMQ_POLLOUT) {
			if (sendHB) {
				bool sent = false;
				diWork = true;
				try {
					sent = zmqRendererSocket.send(ControlFrame::make(clType, ControlMessage::PONG_MSG), ZMQ_SNDMORE);
					assert(sent && "Failed sending ControlFrame for PONG.");
					sent = zmqRendererSocket.send(emtpyFrame);
					assert(sent && "Failed sending empty frame for PONG.");
				} catch (zmq::error_t & ex) {
					if (ex.num() != ETERM) {
						Logger::log(Logger::Error, "Error while renderer is sending message:", ex.what());
					}
					transitionState(RUNNING, IDLE);
					return;
				}
				sendHB = !sent;
			}

			if (!outstandingMessages.empty()) {
				lock_guard<mutex> lock(messageMtx);
				for (int c = 0; c < MAX_CONSEQ_MESSAGES && !outstandingMessages.empty() && runState == RUNNING; ++c) {
					diWork = true;
					bool sent = false;
					try {
						sent = zmqRendererSocket.send(ControlFrame::make(clType), ZMQ_SNDMORE);
						if (sent) {
							zmqRendererSocket.send(outstandingMessages.front().getMessage());
						}
					} catch (zmq::error_t & ex) {
						if (ex.num() != ETERM) {
							Logger::log(Logger::Error, "Error while renderer is sending message:", ex.what());
						}
						transitionState(RUNNING, IDLE);
						return;
					}
					if (!sent) {
						break;
					}
					outstandingMessages.pop();
				}
			}
			if (!diWork) {
				this_thread::sleep_for(chrono::milliseconds(1));
			}
		}
	}

	if (clType == ClientType::Exporter) {
		try {
			Logger::log(Logger::Debug, "Renderer stopping, sending abort message to client.");
			// lets try fast cleanup
			int wait = EXPORTER_TIMEOUT / 2;
			zmqRendererSocket.setsockopt(ZMQ_SNDTIMEO, &wait, sizeof(wait));
			zmqRendererSocket.send(ControlFrame::make(), ZMQ_SNDMORE);
			zmqRendererSocket.send(VRayMessage::msgRendererState(VRayMessage::RendererState::Abort, currentFrame).getMessage());
		} catch (zmq::error_t & ex) {
			if (ex.num() != ETERM) {
				Logger::log(Logger::Error, "Error while renderer is sending cleanup message:", ex.what());
			}
		}
	}
}

