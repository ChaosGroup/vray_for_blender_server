#ifndef RENDERER_CONTROLLER_H
#define RENDERER_CONTROLLER_H

#include <memory>
#include <vraysdk.hpp>
#include "../zmq-wrapper/zmq_wrapper.h"
#include "zmq_server_client.h"

class RendererController {
public:
    
	RendererController(const std::string & port, uint64_t rendererId);

	RendererController(const RendererController &) = delete;
	RendererController & operator=(const RendererController &) = delete;

private:

    void dispatcher(VRayMessage & message, ZmqWrapper * server);

    void pluginMessage(VRayMessage & message);
    void rendererMessage(VRayMessage & message, ZmqWrapper * server);
private:

	ZmqServerClient server;
	std::unique_ptr<VRay::VRayInit> vray;
	std::unique_ptr<VRay::VRayRenderer> renderer;
	bool showVFB;
};


#endif // RENDERER_CONTROLLER_H
