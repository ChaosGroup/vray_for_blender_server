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
	Logger::log(Logger::Info, "workers.emplace(make_pair(clientId, move(wrapper)))");
	auto res = workers.emplace(make_pair(clientId, move(wrapper)));
	assert(res.second && "Failed to add worker!");
	Logger::log(Logger::Debug, "New client (", clientId, ") connected.");
}


bool ZmqProxyServer::checkForTimeouts(time_point now) {
	if (duration_cast<milliseconds>(now - lastTimeoutCheck).count() <= 100 || workers.empty()) {
		return false;
	}

	lastTimeoutCheck = now;
	bool signalReaper = false;
	for (auto workerIter = workers.begin(), end = workers.end(); workerIter != end; /*nop*/) {
		auto inactiveTime = duration_cast<milliseconds>(now - workerIter->second.lastKeepAlive).count();
		auto maxInactive = HEARBEAT_TIMEOUT;

		if (workerIter->second.clientType == ClientType::Exporter) {
			maxInactive = EXPORTER_TIMEOUT;
		}

		if (inactiveTime > maxInactive || !workerIter->second.worker->isRunning()) {
			Logger::log(Logger::Debug, "Client (", workerIter->first, ") timed out - stopping it's renderer");
			{
				lock_guard<mutex> lk(reaperMtx);
				deadRenderers.emplace_back(move(workerIter->second));
				workerIter = workers.erase(workerIter); // should be no-op since worker is moved in dead que
				signalReaper = true;
			}
		} else {
			++workerIter;
		}
	}

	if (signalReaper) {
		reaperCond.notify_all();
	}

	return true;
}

bool ZmqProxyServer::reportStats(time_point now) {
	auto dataReportDiff = duration_cast<milliseconds>(now - lastDataCheck).count();
	if (dataReportDiff <= 1000) {
		return false;
	}

	const int toFree = deadRenderers.size();
	if (toFree > 20) {
		Logger::log(Logger::Error, "Failing to free renderers fast enough:", toFree);
	} else if (toFree > 10) {
		Logger::log(Logger::Warning, "Failing to free renderers fast enough:", toFree);
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

std::pair<int, bool> ZmqProxyServer::checkSocketOpt(zmq::socket_t & socket, int option) {
	std::pair<int, bool> result;
	size_t more_size = sizeof (result.first);
	try {
		Logger::log(Logger::Info, "frontend.getsockopt(", option,", &result, &size)");
		socket.getsockopt(option, &result.first, &more_size);
	} catch (zmq::error_t & ex) {
		result.second = true;
		Logger::log(Logger::Error, "zmq::socket_t::getsockopt:", ex.what());
	}
	return result;
}

void ZmqProxyServer::reaperThreadBase() {
	while (reaperRunning) {
		if (deadRenderers.empty()) {
			unique_lock<mutex> lk(reaperMtx);
			if (deadRenderers.empty() && reaperRunning) {
				reaperCond.wait(lk, [this]() { return !deadRenderers.empty() || !reaperRunning; });
			}
		} else {
			unique_lock<mutex> lk(reaperMtx);
			// only this thread should remove items from deadRenderers
			assert(!deadRenderers.empty() && "There were some items in deadRenderers before lock and now there arent!");
			while (!deadRenderers.empty() && !deadRenderers.back().worker) {
				deadRenderers.pop_back(); // clear null ptrs since they are no-op
			}
			if (deadRenderers.empty()) {
				continue; // in case there we only null ptrs inside
			}
			auto worker = move(deadRenderers.back());
			lk.unlock();

			assert(!!worker.worker && "Already free-ed Renderer inside deadRenderers");
			Logger::log(Logger::Debug, "worker.worker->stop()");
			worker.worker->stop();
			Logger::log(Logger::Debug, "worker.worker.reset()");
			worker.worker.reset();
		}
	}
}


void ZmqProxyServer::run() {
	zmq::socket_t backend(context, ZMQ_ROUTER);
	zmq::socket_t frontend(context, ZMQ_ROUTER);

	try {
		backend.setsockopt(ZMQ_ROUTER_MANDATORY, 1);
		frontend.setsockopt(ZMQ_ROUTER_MANDATORY, 1);

		backend.setsockopt(ZMQ_SNDHWM, 0);
		frontend.setsockopt(ZMQ_SNDHWM, 0);

		int wait = SOCKET_IO_TIMEOUT;
		frontend.setsockopt(ZMQ_SNDTIMEO, &wait, sizeof(wait));
		wait = SOCKET_IO_TIMEOUT;
		frontend.setsockopt(ZMQ_RCVTIMEO, &wait, sizeof(wait));

		wait = SOCKET_IO_TIMEOUT;
		backend.setsockopt(ZMQ_SNDTIMEO, &wait, sizeof(wait));
		wait = SOCKET_IO_TIMEOUT;
		backend.setsockopt(ZMQ_RCVTIMEO, &wait, sizeof(wait));

		backend.bind("inproc://backend");
		frontend.bind((string("tcp://*:") + port).c_str());
	} catch (zmq::error_t & ex) {
		Logger::log(Logger::Error, "While initializing server:", ex.what());
		qApp->quit();
		return;
	}

	reaperRunning = true;
	reaperThread = thread(&ZmqProxyServer::reaperThreadBase, this);


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
			Logger::log(Logger::Info, "zmq::poll()");
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
					Logger::log(Logger::Info, "frontend.recv(&idMsg)");
					recv = recv && frontend.recv(&idMsg);
					assert(idMsg.more() && "Missing control frame and payload from client's message!");
					Logger::log(Logger::Info, "frontend.recv(&ctrlMsg)");
					recv = recv && frontend.recv(&ctrlMsg);
					assert(ctrlMsg.more() && "Missing payload from client's message!");
					Logger::log(Logger::Info, "frontend.recv(&payloadMsg)");
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
					if (frame.control == ControlMessage::HEARTBEAT_CONNECT_MSG || frame.control == ControlMessage::EXPORTER_CONNECT_MSG) {
						Logger::log(Logger::Info, "addWorker(clId, now, frame.type)");
						addWorker(clId, now, frame.type);
					}
				} else {
					workerIter->second.lastKeepAlive = now;
					lastHeartbeat = std::max(lastHeartbeat, now);
					try {
						Logger::log(Logger::Info, "backend.send(idMsg, ZMQ_SNDMORE)");
						backend.send(idMsg, ZMQ_SNDMORE);
						Logger::log(Logger::Info, "backend.send(ctrlMsg, ZMQ_SNDMORE)");
						backend.send(ctrlMsg, ZMQ_SNDMORE);
						Logger::log(Logger::Info, "backend.send(payloadMsg)");
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

				const auto moreCheck = checkSocketOpt(frontend, ZMQ_RCVMORE);
				if (moreCheck.first == 0 || moreCheck.second) {
					break;
				}

				const auto sndCheck = checkSocketOpt(backend, ZMQ_SNDMORE);
				if (!sndCheck.first || sndCheck.second) {
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
					Logger::log(Logger::Info, "backend.recv(&idMsg)");
					backend.recv(&idMsg);
					assert(idMsg.more() && "Missing control frame and payload from render's message!");

					Logger::log(Logger::Info, "backend.recv(&ctrlMsg)");
					backend.recv(&ctrlMsg);
					assert(ctrlMsg.more() && "Missing payload from render's message!");

					Logger::log(Logger::Info, "backend.recv(&payloadMsg)");
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
					Logger::log(Logger::Info, "frontend.send(idMsg, ZMQ_SNDMORE)");
					frontend.send(idMsg, ZMQ_SNDMORE);
					Logger::log(Logger::Info, "frontend.send(ctrlMsg, ZMQ_SNDMORE)");
					frontend.send(ctrlMsg, ZMQ_SNDMORE);
					Logger::log(Logger::Info, "frontend.send(payloadMsg)");
					frontend.send(payloadMsg);
				} catch (zmq::error_t & ex) {
					if (ex.num() == EHOSTUNREACH) {
						auto workerIter = workers.find(clId);
						if (workerIter != workers.end()) {
							Logger::log(Logger::Warning, "Renderer sending data to disconnected client - stopping it!");
							{
								lock_guard<mutex> lk(reaperMtx);
								deadRenderers.emplace_back(move(workerIter->second));
								workers.erase(workerIter); // should be no-op since worker is moved in dead que
							}
							reaperCond.notify_one();
						}
					} else {
						Logger::log(Logger::Error, "Error while handling renderer (", clId ,") message: ", ex.what());
					}
				}

				dataTransfered += sizeof(client_id_t) + payloadMsg.size();

				const auto moreCheck = checkSocketOpt(backend, ZMQ_RCVMORE);
				if (moreCheck.first == 0 || moreCheck.second) {
					break;
				}

				const auto sndCheck = checkSocketOpt(frontend, ZMQ_SNDMORE);
				if (!sndCheck.first || sndCheck.second) {
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

	Logger::log(Logger::Debug, "Stopping reaper thread.");
	reaperRunning = false;
	reaperCond.notify_all(); // reaper waits on this for items or flag
	reaperThread.join();

	Logger::log(Logger::Debug, "Closing server sockets.");
	// close sockets and context
	frontend.close();
	backend.close();
	context.close();

	Logger::log(Logger::Debug, "Server stopping all renderers.");
	deadRenderers.clear();
	workers.clear();

	Logger::log(Logger::Debug, "Server thread stopping.");
	qApp->quit();
}
