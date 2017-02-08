#define VRAY_RUNTIME_LOAD_SECONDARY
#include "zmq_proxy_server.h"
#include "utils/logger.h"
#include <chrono>
#include <random>
#include <exception>

#include <vraysdk.hpp>
#include <qapplication.h>

#include <cassert>

using namespace std;
using namespace std::chrono;
using namespace zmq;

ZmqProxyServer::WorkerWrapper::WorkerWrapper(std::unique_ptr<RendererController> worker, time_point lastKeepAlive, client_id_t id, ClientType clType)
    : worker(move(worker))
    , lastKeepAlive(lastKeepAlive)
    , id(std::move(id))
    , clientType(clType)
{

}

ZmqProxyServer::ZmqProxyServer(const string & port, bool showVFB, bool checkHeartbeat)
    : port(port)
    , context(1)
    , dataTransfered(0)
    , showVFB(showVFB)
    , checkHeartbeat(checkHeartbeat)
{
}

void ZmqProxyServer::addWorker(client_id_t clientId, time_point now, ClientType type) {
	WorkerWrapper wrapper = {
		unique_ptr<RendererController>(new RendererController(context, clientId, type, showVFB)),
		now, clientId, type
	};
	wrapper.worker->start();
	auto res = workers.emplace(make_pair(clientId, move(wrapper)));
	assert(res.second && "Failed to add worker!");
	Logger::log(Logger::Debug, "New client (", clientId, ") connected.");
}


bool ZmqProxyServer::checkForTimeouts(time_point now) {
	if (duration_cast<milliseconds>(now - lastTimeoutCheck).count() <= 100 || workers.empty()) {
		return false;
	}

	lastTimeoutCheck = now;
	for (auto workerIter = workers.begin(), end = workers.end(); workerIter != end; /*nop*/) {
		auto inactiveTime = duration_cast<milliseconds>(now - workerIter->second.lastKeepAlive).count();
		auto maxInactive = HEARBEAT_TIMEOUT;

		if (workerIter->second.clientType == ClientType::Exporter) {
			maxInactive = EXPORTER_TIMEOUT;
		}

		if (inactiveTime > maxInactive || !workerIter->second.worker->isRunning()) {
			Logger::log(Logger::Debug, "Client (", workerIter->first, ") timed out - stopping it's renderer");
			workerIter->second.worker->stop();
			workerIter = workers.erase(workerIter);
		} else {
			++workerIter;
		}
	}

	return true;
}

bool ZmqProxyServer::reportStats(time_point now) {
	auto dataReportDiff = duration_cast<milliseconds>(now - lastDataCheck).count();
	if (dataReportDiff <= 1000) {
		return false;
	}
	lastDataCheck = now;
	if (dataTransfered / 1024 > 1) {
		Logger::log(Logger::Debug, "Data transfered", dataTransfered / 1024., "KB for", dataReportDiff, "ms");
		dataTransfered = 0;
	}

	int exporterCount = 0;
	for (const auto & worker : workers) {
		if (worker.second.clientType == ClientType::Exporter) {
			++exporterCount;
		}
	}

	Logger::log(Logger::Debug, "Exporters:", exporterCount, "Active Blender instaces:", workers.size() - exporterCount);

	return true;
}

void ZmqProxyServer::run() {
	zmq::socket_t backend(context, ZMQ_ROUTER);
	zmq::socket_t frontend(context, ZMQ_ROUTER);

	try {
		backend.setsockopt(ZMQ_ROUTER_MANDATORY, 1);
		frontend.setsockopt(ZMQ_ROUTER_MANDATORY, 1);

		int wait = EXPORTER_TIMEOUT * 2;
		frontend.setsockopt(ZMQ_SNDTIMEO, &wait, sizeof(wait));
		wait = EXPORTER_TIMEOUT * 2;
		frontend.setsockopt(ZMQ_RCVTIMEO, &wait, sizeof(wait));

		wait = EXPORTER_TIMEOUT * 2;
		backend.setsockopt(ZMQ_SNDTIMEO, &wait, sizeof(wait));
		wait = EXPORTER_TIMEOUT * 2;
		backend.setsockopt(ZMQ_RCVTIMEO, &wait, sizeof(wait));

		backend.bind("inproc://backend");
		frontend.bind((string("tcp://*:") + port).c_str());
	} catch (zmq::error_t & ex) {
		Logger::log(Logger::Error, "While initializing server:", ex.what());
		qApp->quit();
		return;
	}

	zmq::pollitem_t pollItems[] = {
		{frontend, 0, ZMQ_POLLIN, 0},
		{backend, 0, ZMQ_POLLIN, 0}
	};

	auto now = high_resolution_clock::now();
	lastTimeoutCheck = now;
	lastDataCheck = now;
	lastHeartbeat = now;

	while (true) {
		bool didWork = false;
		now = high_resolution_clock::now();

		int pollResult = 0;
		try {
			pollResult = zmq::poll(pollItems, 2, 100);
		} catch (zmq::error_t & ex) {
			Logger::log(Logger::Error, "zmq::poll:", ex.what());
			qApp->quit();
			return;
		}

		if (pollItems[0].revents & ZMQ_POLLIN) {
			didWork = true;
			bool stopServing = false;
			while (true) {
				zmq::message_t idMsg, ctrlMsg, payloadMsg;
				bool recv = true;
				try {
					recv = recv && frontend.recv(&idMsg);
					assert(idMsg.more() && "Missing control frame and payload from client's message!");
					recv = recv && frontend.recv(&ctrlMsg);
					assert(ctrlMsg.more() && "Missing payload from client's message!");
					recv = recv && frontend.recv(&payloadMsg);
					assert(!payloadMsg.more() && "Unexpected parts after client's payload!");
				} catch (zmq::error_t & ex) {
					Logger::log(Logger::Error, "zmq::socket_t::recv:", ex.what());
					break;
				}

				if (!recv) {
					Logger::log(Logger::Warning, "Timeout recv");
				}

				ControlFrame frame(ctrlMsg);

				assert(!!frame && "Client sent malformed control frame");
				assert(idMsg.size() == sizeof(client_id_t) && "ID frame with unexpected size");

				if (frame.control == ControlMessage::STOP_MSG) {
					Logger::log(Logger::Debug, "Client requested server to stop!");
					stopServing = true;
				}

				client_id_t clId = *reinterpret_cast<client_id_t*>(idMsg.data());

				auto workerIter = workers.find(clId);

				if (workerIter == workers.end()) {
					if (frame.type == ClientType::Exporter) {
						assert(frame.control == ControlMessage::EXPORTER_CONNECT_MSG && "Exporter did not send correct handshake");
					} else if (frame.type == ClientType::Heartbeat) {
						assert(frame.control == ControlMessage::HEARTBEAT_CONNECT_MSG && "Heartbeat did not send correct handshake");
					}
					assert(payloadMsg.size() == 0 && "Missing empty frame after handshake!");
					addWorker(clId, now, frame.type);
				} else {
					workerIter->second.lastKeepAlive = now;
					lastHeartbeat = std::max(lastHeartbeat, now);
					try {
						backend.send(idMsg, ZMQ_SNDMORE);
						backend.send(ctrlMsg, ZMQ_SNDMORE);
						backend.send(payloadMsg);
					} catch (zmq::error_t & ex) {
						if (ex.num() == EHOSTUNREACH) {
							assert(!"Client sending data to inexistent renderer");
						} else {
							Logger::log(Logger::Error, "Error while handling client (", clId ,") message: ", ex.what());
						}
					}
				}

				dataTransfered += sizeof(client_id_t) + payloadMsg.size();

				int more = 0;
				size_t more_size = sizeof (more);
				try {
					frontend.getsockopt(ZMQ_RCVMORE, &more, &more_size);
				} catch (zmq::error_t & ex) {
					Logger::log(Logger::Error, "zmq::socket_t::getsockopt:", ex.what());
					break;
				}
				if (!more) {
					break;
				}
			}

			if (stopServing) {
				break;
			}
		}

		if (pollItems[1].revents & ZMQ_POLLIN) {
			didWork = true;
			while (true) {
				zmq::message_t idMsg, ctrlMsg, payloadMsg;

				try {
					backend.recv(&idMsg);
					assert(idMsg.more() && "Missing control frame and payload from render's message!");
					backend.recv(&ctrlMsg);
					assert(ctrlMsg.more() && "Missing payload from render's message!");
					backend.recv(&payloadMsg);
					assert(!payloadMsg.more() && "Unexpected parts after renderer's payload!");
				} catch (zmq::error_t & ex) {
					Logger::log(Logger::Error, ex.what());
					break;
				}

				ControlFrame frame(ctrlMsg);

				assert(idMsg.size() == sizeof(client_id_t) && "ID frame with unexpected size");
				assert(!!frame && "Malformed frame sent from renderer/heartbeat");

				client_id_t clId = *reinterpret_cast<client_id_t*>(idMsg.data());

				// check for routing here
				try {
					frontend.send(idMsg, ZMQ_SNDMORE);
					frontend.send(ctrlMsg, ZMQ_SNDMORE);
					frontend.send(payloadMsg);
				} catch (zmq::error_t & ex) {
					if (ex.num() == EHOSTUNREACH) {
						auto workerIter = workers.find(clId);
						if (workerIter != workers.end()) {
							Logger::log(Logger::Warning, "Renderer sending data to disconnected client - stopping it!");
							workerIter->second.worker->stop();
							workers.erase(workerIter);
						}
					} else {
						Logger::log(Logger::Error, "Error while handling renderer (", clId ,") message: ", ex.what());
					}
				}

				dataTransfered += sizeof(client_id_t) + payloadMsg.size();

				int more = 0;
				size_t more_size = sizeof (more);
				try {
					backend.getsockopt(ZMQ_RCVMORE, &more, &more_size);
				} catch (zmq::error_t & ex) {
					Logger::log(Logger::Error, ex.what());
					break;
				}
				if (!more) {
					break;
				}
			}
		}

		if (reportStats(now)) {
			didWork = true;
		}
		if (checkForTimeouts(now)) {
			didWork = true;
		}

		if (checkHeartbeat) {
			if (duration_cast<milliseconds>(now - lastHeartbeat).count() > EXPORTER_TIMEOUT) {
				Logger::log(Logger::Error, "No active blender instaces for more than", HEARBEAT_TIMEOUT, "ms Shutting down");
				break;
			}
		}

		if (pollResult == 0 && !didWork) {
			this_thread::sleep_for(chrono::milliseconds(1));
		}
	}

	Logger::log(Logger::Debug, "Server stopping all renderers.");

	frontend.close();
	backend.close();
	context.close();
	for (auto & w : workers) {
		w.second.worker->stop();
	}
	workers.clear();

	Logger::log(Logger::Debug, "Server thread stopping.");
	qApp->quit();
}
