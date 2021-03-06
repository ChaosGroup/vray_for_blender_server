#ifndef RENDERER_CONTROLLER_H
#define RENDERER_CONTROLLER_H

#include "zmq_wrapper.hpp"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

#include <vraysdk.hpp>
#include <queue>
#include <memory>
#include <unordered_set>

#include "utils/logger.h"

/// Wrapper over VRay::VRayRenderer to process incomming messages
class RendererController {
	enum RunState {
		IDLE,
		STARTING,
		RUNNING,
		STOPPING,
	};
public:

	/// Create a wrapper
	/// @sendFn - function called when data needs to be sent back to client (image or message)
	/// @showVFB - enable/disable vfb
	RendererController(zmq::context_t & zmqContext, uint64_t clId, ClientType type, bool showVFB);
	~RendererController();

	RendererController(const RendererController &) = delete;
	RendererController & operator=(const RendererController &) = delete;

	/// Handle an incomming message - calls appropriate @pluginMessage or @rendererMessage and handles errors
	/// This function takes ownership of the provided message and might move it to extend it's lifetime
	/// @message - the message
	void handle(VRayMessage && message);

	/// Stop serving messages
	void stop();

	/// Start serving messages
	/// @return true - thread started
	bool start();

	/// Check if currently this controller is serving messages
	bool isRunning() const;
private:
	/// Cleany stop amd free the renderer
	void stopRenderer(bool lockMtx = true);

	/// Callback for VRayRenderer::setOnProgress, sends message and progress in [0,1]
	void onProgress(VRay::VRayRenderer & renderer, const char* msg, int elementNumber, int elementsCount, void * arg);

	/// Callback for VRayRenderer::setOnRTImageUpdated, sends image to client
	void imageUpdate(VRay::VRayRenderer & renderer, VRay::VRayImage * img, void * arg);

	/// Callback for VRayRenderer::setOnImageReady, sends image to client
	void imageDone(VRay::VRayRenderer & renderer, void * arg);

	/// Callback for VRayRenderer::setOnVFBClosed, free renderer object
	void closeVFB(VRay::VRayRenderer & renderer, void * arg);

	/// Callback for VRayRenderer::setOnBucketReady, sends bucket to client
	void bucketReady(VRay::VRayRenderer & renderer, int x, int y, const char * host, VRay::VRayImage * img, void * arg);

	/// Callback for VRayRenderer::setOnDumpMessage, sends message to client
	void vrayMessageDumpHandler(VRay::VRayRenderer &, const char * msg, int level, void * arg);

	/// Actual sender for images
	/// @img - the image received from vray
	/// @fullImageType - the image enconding format (JPG, RGBA_REAL, etc)
	/// @sourceType - RT image update or image done
	void sendImages(VRay::VRayImage * img, VRayBaseTypes::AttrImage::ImageType fullImageType, VRayBaseTypes::ImageSourceType sourceType);

	/// Update plugin in current renderer from message data
	void pluginMessage(VRayMessage && message);

	/// Update renderer state from message data
	void rendererMessage(VRayMessage && message);

	/// Starts serving messages
	void run();

	/// Change current state in thread safe way and also signal the cond var for the state
	/// @current - the supposed current state - if this->runState != current then nothing happens
	/// @newState - the state after the transition
	void transitionState(RunState current, RunState newState);

	/// Convert interface value list to vray value list
	/// @val - the value to be converted
	/// @message - the current message being inserted - needted so it can be delayed if needed
	/// @return - bool true if we can insert the returned value, false on error
	///         - VRay::Value that is the converted result
	std::pair<bool, VRay::Value> toVrayValue(const VRayBaseTypes::AttrValue & val, VRayMessage & message);

	/// Check if the currently allocated renderer is suitable for persistence
	bool canPersistCurrentRenderer() const;
private:

	RunState runState;  ///< Internal state to make correct transitions
	ClientType clType; ///< Either exporter or heartbeat
	std::mutex stateMtx; ///< Lock protecting runState
	std::condition_variable stateCond; ///< Condition variable for run state change signals
	std::thread runnerThread; ///< Thread processing messages from client

	uint64_t clientId; ///< Our id
	zmq::context_t & zmqContext; ///< The zmq context to pass to socket
	std::mutex messageMtx; ///< Lock protecting the queue for sending
	std::queue<zmq::message_t> outstandingMessages; ///< Queue for messages to be sent

	/// Hash map that stores plugins that reference other plugins that are not yet exported
	/// When creating a new plugin, this map is checked to see if some other plugin is waiting for the new one
	/// When commit action comes this is flushed and the storred error messages (in Logger object) are also displayed
	std::unordered_map<std::string, std::vector<std::pair<VRayMessage, Logger>>> delayedMessages;

	VRay::RendererOptions options; ///< Options for VRayRenderer
	VRay::VRayRenderer * renderer; ///< Pointer to VRayRenderer
	std::unordered_set<VRay::RenderElement::Type, std::hash<int>> elementsToSend; ///< Renderer elements to send to client when sending images
	std::mutex elemsToSendMtx;
	VRayMessage::RendererType type; ///< RT or Animation
	float currentFrame; ///< Currently rendered frame
	int jpegQuality; ///< Desiered jpeg quality for images sent to client
	VRayBaseTypes::AttrImage::ImageType viewportType; ///< Desiered image type for imageUpdate callback

	std::mutex rendererMtx; ///< Protects all callbacks in order to ensure they are executing with valid renderer
	bool vfbClosed; ///< True if user closed VFB and we dont want to save current renderer as persistent
};


#endif // RENDERER_CONTROLLER_H
