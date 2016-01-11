#ifndef RENDERER_CONTROLLER_H
#define RENDERER_CONTROLLER_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define VRAY_RUNTIME_LOAD_PRIMARY
#include <vraysdk.hpp>

#include <memory>
#include <unordered_set>
#include "zmq_wrapper.h"
#include "zmq_server_client.h"

class RendererController {
public:
	typedef std::function<void(VRayMessage && msg)> send_fn_t;

	RendererController(send_fn_t sendFn, bool showVFB);
	~RendererController();

	RendererController(const RendererController &) = delete;
	RendererController & operator=(const RendererController &) = delete;

	void handle(const VRayMessage & message);
private:
	void imageUpdate(VRay::VRayRenderer & renderer, VRay::VRayImage * img, void * arg);
	void imageDone(VRay::VRayRenderer & renderer, void * arg);
	void bucketReady(VRay::VRayRenderer & renderer, int x, int y, const char * host, VRay::VRayImage * img, void * arg);

	void vrayMessageDumpHandler(VRay::VRayRenderer &, const char * msg, int level, void * arg);

	void sendImages(VRay::VRayImage * img, VRayBaseTypes::AttrImage::ImageType fullImageType, VRayBaseTypes::ImageSourceType sourceType);

	void pluginMessage(const VRayMessage & message);
	void rendererMessage(const VRayMessage & message);
private:

	send_fn_t sendFn;
	std::unique_ptr<VRay::VRayRenderer> renderer;
	std::unordered_set<VRay::RenderElement::Type, std::hash<int>> elementsToSend;
	bool showVFB;
	VRayMessage::RendererType type;
	float currentFrame;
	int jpegQuality;

	/// Should protect all callbacks in order to assure they are executing with valid renderer
	std::mutex rendererMtx;
};


#endif // RENDERER_CONTROLLER_H
