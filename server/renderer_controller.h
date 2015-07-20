#ifndef RENDERER_CONTROLLER_H
#define RENDERER_CONTROLLER_H

#include <memory>
#include <vraysdk.hpp>
#include "zmq_wrapper.h"
#include "zmq_server_client.h"

class RendererController {
public:
	friend void vrayMessageDumpHandler(VRay::VRayRenderer &, const char * msg, int level, void * arg);

	RendererController(const std::string & port, uint64_t rendererId, bool showVFB);
	~RendererController();

	RendererController(const RendererController &) = delete;
	RendererController & operator=(const RendererController &) = delete;

private:

	void dispatcher(VRayMessage & message, ZmqWrapper * server);

	void pluginMessage(VRayMessage & message);
	void rendererMessage(VRayMessage & message, ZmqWrapper * server);
private:

	ZmqServerClient server;
	std::unique_ptr<VRay::VRayRenderer> renderer;
	bool showVFB;
};


#endif // RENDERER_CONTROLLER_H
