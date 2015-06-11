#ifndef RENDERER_CONTROLLER_H
#define RENDERER_CONTROLLER_H

#include <memory>
#include <vraysdk.hpp>
#include "../zmq-wrapper/zmq_wrapper.h"

class MainWindow;

class RendererController {
public:
    
	RendererController(const std::string & port, bool showVFB = false);
	void setWindow(MainWindow * window);

private:

    void dispatcher(VRayMessage & message, ZmqWrapper * server);

    void pluginMessage(VRayMessage & message);
    void rendererMessage(VRayMessage & message, ZmqWrapper * server);
private:

	ZmqServer server;
	std::unique_ptr<VRay::VRayInit> vray;
	std::unique_ptr<VRay::VRayRenderer> renderer;
	bool showVFB;
};


#endif // RENDERER_CONTROLLER_H
