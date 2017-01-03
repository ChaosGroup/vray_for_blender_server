#ifndef RENDERER_CONTROLLER_H
#define RENDERER_CONTROLLER_H

#include <memory>
#include <unordered_set>
#include "zmq_wrapper.hpp"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

#include <vraysdk.hpp>

/// Wrapper over VRay::VRayRenderer to process incomming messages
class RendererController {
public:
	typedef std::function<void(VRayMessage && msg)> send_fn_t;

	/// Create a wrapper
	/// @sendFn - function called when data needs to be sent back to client (image or message)
	/// @showVFB - enable/disable vfb
	RendererController(send_fn_t sendFn, bool showVFB);
	~RendererController();

	RendererController(const RendererController &) = delete;
	RendererController & operator=(const RendererController &) = delete;

	/// Handle an incomming message - calls appropriate @pluginMessage or @rendererMessage and handles errors
	/// @message - the message
	void handle(const VRayMessage & message);
private:
	/// Cleany stop amd free the renderer
	void stopRenderer();

	/// Callback for VRayRenderer::setOnRTImageUpdated, sends image to client
	void imageUpdate(VRay::VRayRenderer & renderer, VRay::VRayImage * img, void * arg);

	/// Callback for VRayRenderer::setOnImageReady, sends image to client
	void imageDone(VRay::VRayRenderer & renderer, void * arg);

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
	void pluginMessage(const VRayMessage & message);

	/// Update renderer state from message data
	void rendererMessage(const VRayMessage & message);
private:

	send_fn_t sendFn; ///< Callback to be used for sending data back to client
	std::unique_ptr<VRay::VRayRenderer> renderer; ///< Pointer to VRayRenderer
	std::unordered_set<VRay::RenderElement::Type, std::hash<int>> elementsToSend; ///< Renderer elements to send to client when sending images
	bool showVFB; ///< Enable/disable vfb
	VRayMessage::RendererType type; ///< RT or Animation
	float currentFrame; ///< Currently rendered frame
	int jpegQuality; ///< Desiered jpeg quality for images sent to client

	std::mutex rendererMtx; ///< Protects all callbacks in order to ensure they are executing with valid renderer
};


#endif // RENDERER_CONTROLLER_H
