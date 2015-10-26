#ifndef RENDERER_CONTROLLER_H
#define RENDERER_CONTROLLER_H

#include <memory>
#include <unordered_set>
#include <vraysdk.hpp>
#include "zmq_wrapper.h"
#include "zmq_server_client.h"

class RendererController {
public:
	typedef std::function<void(VRayMessage && msg)> send_fn_t;

	RendererController(send_fn_t sendFn, bool showVFB);
	~RendererController();

	RendererController(const RendererController &) = delete;
	RendererController & operator=(const RendererController &) = delete;

	void handle(VRayMessage & message);
private:
	void imageUpdate(VRay::VRayRenderer & renderer, VRay::VRayImage * img, void * arg);
	void imageDone(VRay::VRayRenderer & renderer, void * arg);
	void vrayMessageDumpHandler(VRay::VRayRenderer &, const char * msg, int level, void * arg);

	void sendImages(VRay::VRayImage * img, VRayBaseTypes::AttrImage::ImageType fullImageType);

	void pluginMessage(VRayMessage & message);
	void rendererMessage(VRayMessage & message);
private:

	send_fn_t sendFn;
	std::unique_ptr<VRay::VRayRenderer> renderer;
	std::unordered_set<VRay::RenderElement::Type> elementsToSend;
	bool showVFB;
	VRayMessage::RendererType type;
	float currentFrame;
};


#endif // RENDERER_CONTROLLER_H
