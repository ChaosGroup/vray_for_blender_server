#ifndef RENDERER_CONTROLLER_H
#define RENDERER_CONTROLLER_H

#include <vraysdk.hpp>
#include "../zmq-wrapper/zmq_wrapper.h"

class MainWindow;

class RendererController {
public:
    void setWindow(MainWindow * window);
private:

    void dispatcher(VRayMessage & message, ZmqWrapper * server);

    void pluginMessage(VRayMessage & message, std::unique_ptr<VRay::VRayRenderer> & renderer);
    void rendererMessage(VRayMessage & message, ZmqWrapper * server, std::unique_ptr<VRay::VRayRenderer> & renderer);
private:
    MainWindow * window;
};


#endif // RENDERER_CONTROLLER_H
