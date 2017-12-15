#pragma once

#include "zmq_wrapper.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <vraysdk.hpp>

struct NullLogger;
struct StreamLogger;

#define LOG()
namespace VRay
{
template <typename LogType = NullLogger>
class LoggingVRayRenderer: private VRayRenderer {

public:

	using BaseClass = VRay::VRayRenderer;
	using BaseClass::vfb;

		
	LoggingVRayRenderer(): VRayRenderer() {
		LOG("LoggingVRayRenderer");
	}
	LoggingVRayRenderer(const RendererOptions &rendererOptions): VRayRenderer(rendererOptions) {
		LOG("LoggingVRayRenderer", rendererOptions);
	}
	~LoggingVRayRenderer() {
		LOG("~LoggingVRayRenderer", );	
	}

	Error getLastError() const {
		LOG("getLastError");
		return BaseClass::getLastError();
	}

	bool setRenderMode(RendererOptions::RenderMode mode) {
		LOG("setRenderMode", mode);
		return BaseClass::setRenderMode(mode);
	}
	bool setOptions(const RendererOptions &options) {
		LOG("setOptions", options);
		return BaseClass::setOptions(options);
	}
	RendererOptions getOptions() {
		LOG("RendererOptions");
		return BaseClass::getOptions();
	}
	bool reset() {
		LOG("reset");
		return BaseClass::reset();
	}
	bool reset(const RendererOptions &options) {
		LOG("reset", options);
		return BaseClass::reset(options);
	}
	bool serializeScene() {
		LOG("serializeScene");
		return BaseClass::serializeScene();
	}
	bool setImprovedDefaultSettings() {
		LOG("setImprovedDefaultSettings");
		return BaseClass::setImprovedDefaultSettings();
	}
	void setCurrentFrame(int frame) {
		LOG("setCurrentFrame", frame);
		return BaseClass::setCurrentFrame(frame);
	}
	void setCurrentTime(double time) {
		LOG("setCurrentTime", time);
		return BaseClass::setCurrentTime(time);
	}
	bool clearAllPropertyValuesUpToTime(double time) {
		LOG("clearAllPropertyValuesUpToTime", time);
		return BaseClass::clearAllPropertyValuesUpToTime(time);
	}
	void useAnimatedValues(bool on) {
		LOG("useAnimatedValues", on);
		return BaseClass::useAnimatedValues(on);
	}
	bool setCamera(const Plugin& plugin) {
		LOG("setCamera", plugin);
		return BaseClass::setCamera(plugin);
	}
	bool setWidth(int width, bool resetCropRegion = true, bool resetRenderRegion = true) {
		LOG("setWidth", width, resetCropRegion, resetRenderRegion);
		return BaseClass::setWidth(width, resetCropRegion, resetRenderRegion);
	}
	bool setHeight(int height, bool resetCropRegion = true, bool resetRenderRegion = true) {
		LOG("setHeight", height, resetCropRegion, resetRenderRegion);
		return BaseClass::setHeight(height, resetCropRegion, resetRenderRegion);
	}
	bool setImageSize(int width, int height, bool resetCropRegion = true, bool resetRenderRegion = true) {
		LOG("setImageSize", width, height, resetCropRegion, resetRenderRegion);
		return BaseClass::setImageSize(width, height, resetCropRegion, resetRenderRegion);
	}
	bool getRenderRegion(int& rgnLeft, int& rgnTop, int& rgnWidth, int& rgnHeight) {
		LOG("getRenderRegion", rgnLeft, rgnTop, rgnWidth, rgnHeight);
		return BaseClass::getRenderRegion(rgnLeft, rgnTop, rgnWidth, rgnHeight);
	}
	bool setRenderRegion(int rgnLeft, int rgnTop, int rgnWidth, int rgnHeight) {
		LOG("setRenderRegion", rgnLeft, rgnTop, rgnWidth, rgnHeight);
		return BaseClass::setRenderRegion(rgnLeft, rgnTop, rgnWidth, rgnHeight);
	}
	bool getCropRegion(int& srcWidth, int& srcHeight, float& rgnLeft, float& rgnTop, float& rgnWidth, float& rgnHeight) {
		LOG("getCropRegion", srcWidth, srcHeight, rgnLeft, rgnTop, rgnWidth, rgnHeight);
		return BaseClass::getCropRegion(srcWidth, srcHeight, rgnLeft, rgnTop, rgnWidth, rgnHeight);
	}
	bool setCropRegion(int srcWidth, int srcHeight, float rgnLeft, float rgnTop) {
		LOG("setCropRegion", srcWidth, srcHeight, rgnLeft, rgnTop);
		return BaseClass::setCropRegion(srcWidth, srcHeight, rgnLeft, rgnTop);
	}
	bool setCropRegion(int srcWidth, int srcHeight, float rgnLeft, float rgnTop, float rgnWidth, float rgnHeight) {
		LOG("setCropRegion", srcWidth, srcHeight, rgnLeft, rgnTop, rgnWidth, rgnHeight);
		return BaseClass::setCropRegion(srcWidth, srcHeight, rgnLeft, rgnTop, rgnWidth, rgnHeight);
	}
	int loadFiltered(const char* fileName, bool(*callback)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*), const void* userData = NULL) {
		LOG("loadFiltered", fileName, callback, userData);
		return BaseClass::loadFiltered(fileName, callback, userData);
	}
	int loadFiltered(const std::string& fileName, bool(*callback)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*), const void* userData = NULL) {
		LOG("loadFiltered", fileName, callback, userData);
		return BaseClass::loadFiltered(fileName, callback, userData);
	}
	int appendFiltered(const char* fileName, bool(*callback)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*), const void* userData = NULL) {
		LOG("appendFiltered", fileName, callback, userData);
		return BaseClass::appendFiltered(fileName, callback, userData);
	}
	int appendFiltered(const std::string& fileName, bool(*callback)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*), const void* userData = NULL) {
		LOG("appendFiltered", fileName, callback, userData);
		return BaseClass::appendFiltered(fileName, callback, userData);
	}
	int loadAsTextFiltered(const char* fileName, bool(*callback)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*), const void* userData = NULL) {
		LOG("loadAsTextFiltered", fileName, callback, userData);
		return BaseClass::loadAsTextFiltered(fileName, callback, userData);
	}
	int loadAsTextFiltered(const std::string& fileName, bool(*callback)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*), const void* userData = NULL) {
		LOG("loadAsTextFiltered", fileName, callback, userData);
		return BaseClass::loadAsTextFiltered(fileName, callback, userData);
	}
	int appendAsTextFiltered(const char* fileName, bool(*callback)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*), const void* userData = NULL) {
		LOG("appendAsTextFiltered", fileName, callback, userData);
		return BaseClass::appendAsTextFiltered(fileName, callback, userData);
	}
	int appendAsTextFiltered(const std::string& fileName, bool(*callback)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*), const void* userData = NULL) {
		LOG("appendAsTextFiltered", fileName, callback, userData);
		return BaseClass::appendAsTextFiltered(fileName, callback, userData);
	}
	bool commit(bool force = false) {
		LOG("commit", force);
		return BaseClass::commit(force);
	}
	Plugin getInstanceOrCreate(const std::string& pluginType) {
		LOG("getInstanceOrCreate", pluginType);
		return BaseClass::getInstanceOrCreate(pluginType);
	}
	Plugin getInstanceOrCreate(const std::string& pluginName, const char* pluginType) {
		LOG("getInstanceOrCreate", pluginName, pluginType);
		return BaseClass::getInstanceOrCreate(pluginName, pluginType);
	}
	Plugin getInstanceOrCreate(const char* pluginName, const std::string& pluginType) {
		LOG("getInstanceOrCreate", pluginName, pluginType);
		return BaseClass::getInstanceOrCreate(pluginName, pluginType);
	}
	Plugin getInstanceOrCreate(const std::string& pluginName, const std::string& pluginType) {
		LOG("getInstanceOrCreate", pluginName, pluginType);
		return BaseClass::getInstanceOrCreate(pluginName, pluginType);
	}
	void setOnImageReady(void(*callback)(VRayRenderer&, void*), const void* userData = NULL) {
		LOG("setOnImageReady", callback, userData);
		return BaseClass::setOnImageReady(callback, userData);
	}
	void setOnRendererClose(void(*callback)(VRayRenderer&, void*), const void* userData = NULL) {
		LOG("setOnRendererClose", callback, userData);
		return BaseClass::setOnRendererClose(callback, userData);
	}
	void setOnRTImageUpdated(void(*callback)(VRayRenderer&, VRayImage* img, void*), const void* userData = NULL) {
		LOG("setOnRTImageUpdated", callback, userData);
		return BaseClass::setOnRTImageUpdated(callback, userData);
	}
	void setOnDumpMessage(void(*callback)(VRayRenderer&, const char* msg, int level, void*), const void* userData = NULL) {
		LOG("setOnDumpMessage", callback, userData);
		return BaseClass::setOnDumpMessage(callback, userData);
	}
	void setOnBucketInit(void(*callback)(VRayRenderer&, int x, int y, int width, int height, const char* host, void*), const void* userData = NULL) {
		LOG("setOnBucketInit", callback, userData);
		return BaseClass::setOnBucketInit(callback, userData);
	}
	void setOnBucketReady(void(*callback)(VRayRenderer&, int x, int y, const char* host, VRayImage* img, void*), const void* userData = NULL) {
		LOG("setOnBucketReady", callback, userData);
		return BaseClass::setOnBucketReady(callback, userData);
	}
	void setOnBucketFailed(void(*callback)(VRayRenderer&, int x, int y, int width, int height, const char* host, void*), const void* userData = NULL) {
		LOG("setOnBucketFailed", callback, userData);
		return BaseClass::setOnBucketFailed(callback, userData);
	}
	void setOnSequenceStart(void(*callback)(VRayRenderer&, void*), const void* userData = NULL) {
		LOG("setOnSequenceStart", callback, userData);
		return BaseClass::setOnSequenceStart(callback, userData);
	}
	void setOnSequenceDone(void(*callback)(VRayRenderer&, void*), const void* userData = NULL) {
		LOG("setOnSequenceDone", callback, userData);
		return BaseClass::setOnSequenceDone(callback, userData);
	}
	void setOnProgress(void(*callback)(VRayRenderer&, const char* msg, int elementNumber, int elementsCount, void*), const void* userData = NULL) {
		LOG("setOnProgress", callback, userData);
		return BaseClass::setOnProgress(callback, userData);
	}
	void setOnRenderViewChanged(void(*callback)(VRayRenderer&, const char* propName, void*), const void* userData = NULL) {
		LOG("setOnRenderViewChanged", callback, userData);
		return BaseClass::setOnRenderViewChanged(callback, userData);
	}
	void setOnRenderLast(void(*callback)(VRayRenderer&, bool isRendering, void*), const void* userData = NULL) {
		LOG("setOnRenderLast", callback, userData);
		return BaseClass::setOnRenderLast(callback, userData);
	}
	void setOnVFBClosed(void(*callback)(VRayRenderer&, void*), const void* userData = NULL) {
		LOG("setOnVFBClosed", callback, userData);
		return BaseClass::setOnVFBClosed(callback, userData);
	}
	void setOnPostEffectsUpdated(void(*callback)(VRayRenderer&, void*), const void* userData = NULL) {
		LOG("setOnPostEffectsUpdated", callback, userData);
		return BaseClass::setOnPostEffectsUpdated(callback, userData);
	}
	void setOnShowMessagesWindow(void(*callback)(VRayRenderer&, void*), const void* userData = NULL) {
		LOG("setOnShowMessagesWindow", callback, userData);
		return BaseClass::setOnShowMessagesWindow(callback, userData);
	}
	void setOnHostConnected(void(*callback)(VRayRenderer&, const char* hostName, void*), const void* userData = NULL) {
		LOG("setOnHostConnected", callback, userData);
		return BaseClass::setOnHostConnected(callback, userData);
	}
	void setOnHostDisconnected(void(*callback)(VRayRenderer&, const char* hostName, void*), const void* userData = NULL) {
		LOG("setOnHostDisconnected", callback, userData);
		return BaseClass::setOnHostDisconnected(callback, userData);
	}
	float setRTImageUpdateDifference(float difference) {
		LOG("setRTImageUpdateDifference", difference);
		return BaseClass::setRTImageUpdateDifference(difference);
	}
	unsigned long setRTImageUpdateTimeout(unsigned long timeout) {
		LOG("unsigned", timeout);
		return BaseClass::setRTImageUpdateTimeout(timeout);
	}
	void setKeepRTframesInCallback(bool keep) {
		LOG("setKeepRTframesInCallback", keep);
		return BaseClass::setKeepRTframesInCallback(keep);
	}
	void setKeepBucketsInCallback(bool keep) {
		LOG("setKeepBucketsInCallback", keep);
		return BaseClass::setKeepBucketsInCallback(keep);
	}
	Plugin newPlugin(const char* pluginName, const char* pluginType) {
		LOG("newPlugin", pluginName, pluginType);
		return BaseClass::newPlugin(pluginName, pluginType);
	}
	Plugin newPlugin(const std::string& pluginName, const std::string& pluginType) {
		LOG("newPlugin", pluginName, pluginType);
		return BaseClass::newPlugin(pluginName, pluginType);
	}
	Plugin newPlugin(const char* pluginName, const std::string& pluginType) {
		LOG("newPlugin", pluginName, pluginType);
		return BaseClass::newPlugin(pluginName, pluginType);
	}
	Plugin newPlugin(const std::string& pluginName, const char* pluginType) {
		LOG("newPlugin", pluginName, pluginType);
		return BaseClass::newPlugin(pluginName, pluginType);
	}
	Plugin newPlugin(const std::string &pluginType) {
		LOG("newPlugin", pluginType);
		return BaseClass::newPlugin(pluginType);
	}
	Plugin getOrCreatePlugin(const std::string& pluginName, const char* pluginType) {
		LOG("getOrCreatePlugin", pluginName, pluginType);
		return BaseClass::getOrCreatePlugin(pluginName, pluginType);
	}
	Plugin getOrCreatePlugin(const char* pluginName, const std::string& pluginType) {
		LOG("getOrCreatePlugin", pluginName, pluginType);
		return BaseClass::getOrCreatePlugin(pluginName, pluginType);
	}
	Plugin getOrCreatePlugin(const std::string& pluginName, const std::string& pluginType) {
		LOG("getOrCreatePlugin", pluginName, pluginType);
		return BaseClass::getOrCreatePlugin(pluginName, pluginType);
	}
	bool removePlugin(const Plugin& plugin) {
		LOG("removePlugin", plugin);
		return BaseClass::removePlugin(plugin);
	}
	bool removePlugin(const std::string& pluginName) {
		LOG("removePlugin", pluginName);
		return BaseClass::removePlugin(pluginName);
	}
	bool removePlugin(const char* pluginName) {
		LOG("removePlugin", pluginName);
		return BaseClass::removePlugin(pluginName);
	}
	bool replacePlugin(const Plugin& oldPlugin, const Plugin& newPlugin) {
		LOG("replacePlugin", oldPlugin, newPlugin);
		return BaseClass::replacePlugin(oldPlugin, newPlugin);
	}
	void setAutoCommit(bool autoCommit) {
		LOG("setAutoCommit", autoCommit);
		return BaseClass::setAutoCommit(autoCommit);
	}
	RenderElements getRenderElements() {
		LOG("RenderElements");
		return BaseClass::getRenderElements();
	}
	bool createProxyFileMesh(const Plugin &geomStaticMeshPlugin, const Transform *transform, const ProxyCreateParams &params) {
		LOG("createProxyFileMesh", geomStaticMeshPlugin, transform, params);
		return BaseClass::createProxyFileMesh(geomStaticMeshPlugin, transform, params);
	}
	bool createCombinedProxyFileMesh(const std::vector<Plugin> &geomStaticMeshPlugins, const std::vector<Transform> *transforms, const ProxyCreateParams &params) {
		LOG("createCombinedProxyFileMesh", geomStaticMeshPlugins, transforms, params);
		return BaseClass::createCombinedProxyFileMesh(geomStaticMeshPlugins, transforms, params);
	}
	size_t getIrradianceMapSize() {
		LOG("getIrradianceMapSize");
		return BaseClass::getIrradianceMapSize();
	}
	bool saveIrradianceMapFile(const std::string &fileName) {
		LOG("saveIrradianceMapFile", fileName);
		return BaseClass::saveIrradianceMapFile(fileName);
	}
	bool saveIrradianceMapFile(const char* fileName) {
		LOG("saveIrradianceMapFile", fileName);
		return BaseClass::saveIrradianceMapFile(fileName);
	}
	size_t getLightCacheSize() {
		LOG("getLightCacheSize");
		return BaseClass::getLightCacheSize();
	}
	bool saveLightCacheFile(const std::string &fileName) {
		LOG("saveLightCacheFile", fileName);
		return BaseClass::saveLightCacheFile(fileName);
	}
	bool saveLightCacheFile(const char* fileName) {
		LOG("saveLightCacheFile", fileName);
		return BaseClass::saveLightCacheFile(fileName);
	}
	size_t getPhotonMapSize() {
		LOG("getPhotonMapSize");
		return BaseClass::getPhotonMapSize();
	}
	bool savePhotonMapFile(const std::string &fileName) {
		LOG("savePhotonMapFile", fileName);
		return BaseClass::savePhotonMapFile(fileName);
	}
	bool savePhotonMapFile(const char* fileName) {
		LOG("savePhotonMapFile", fileName);
		return BaseClass::savePhotonMapFile(fileName);
	}
	size_t getCausticsSize() {
		LOG("getCausticsSize");
		return BaseClass::getCausticsSize();
	}
	bool saveCausticsFile(const std::string &fileName) {
		LOG("saveCausticsFile", fileName);
		return BaseClass::saveCausticsFile(fileName);
	}
	bool saveCausticsFile(const char* fileName) {
		LOG("saveCausticsFile", fileName);
		return BaseClass::saveCausticsFile(fileName);
	}
	bool setComputeDevicesCUDA(const std::vector<int> &indices) {
		LOG("setComputeDevicesCUDA", indices);
		return BaseClass::setComputeDevicesCUDA(indices);
	}
	bool setComputeDevicesOpenCL(const std::vector<int> &indices) {
		LOG("setComputeDevicesOpenCL", indices);
		return BaseClass::setComputeDevicesOpenCL(indices);
	}
	bool setComputeDevicesCurrentEngine(const std::vector<int> &indices) {
		LOG("setComputeDevicesCurrentEngine", indices);
		return BaseClass::setComputeDevicesCurrentEngine(indices);
	}
	bool setResumableRendering(bool enable, const ResumableRenderingOptions *options) {
		LOG("setResumableRendering", enable, options);
		return BaseClass::setResumableRendering(enable, options);
	}
	bool denoiseNow() {
		LOG("denoiseNow");
		return BaseClass::denoiseNow();
	}
	void setIncludePaths(const char* includePaths, bool overwrite = true) {
		LOG("setIncludePaths", includePaths, overwrite);
		return BaseClass::setIncludePaths(includePaths, overwrite);
	}
	void setIncludePaths(const std::string &includePaths, bool overwrite = true) {
		LOG("setIncludePaths", includePaths, overwrite);
		return BaseClass::setIncludePaths(includePaths, overwrite);
	}




	int getCurrentFrame() const {
		LOG("getCurrentFrame");
		return BaseClass::getCurrentFrame();
	}
	double getCurrentTime() const {
		LOG("getCurrentTime");
		return BaseClass::getCurrentTime();
	}
	bool getUseAnimatedValuesState() const {
		LOG("getUseAnimatedValuesState");
		return BaseClass::getUseAnimatedValuesState();
	}
	int getSequenceFrame() const {
		LOG("getSequenceFrame");
		return BaseClass::getSequenceFrame();
	}
	Box getBoundingBox() const {
		LOG("getBoundingBox");
		return BaseClass::getBoundingBox();
	}
	Box getBoundingBox(bool& ok) const {
		LOG("getBoundingBox", ok);
		return BaseClass::getBoundingBox(ok);
	}
	Plugin getCamera() const {
		LOG("getCamera");
		return BaseClass::getCamera();
	}
	RTStatistics getRTStatistics() const {
		LOG("getRTStatistics");
		return BaseClass::getRTStatistics();
	}
	bool pause() const {
		LOG("pause");
		return BaseClass::pause();
	}
	bool resume() const {
		LOG("resume");
		return BaseClass::resume();
	}
	int getWidth() const {
		LOG("getWidth");
		return BaseClass::getWidth();
	}
	int getHeight() const {
		LOG("getHeight");
		return BaseClass::getHeight();
	}
	bool getImageSize(int& width, int& height) const {
		LOG("getImageSize", width, height);
		return BaseClass::getImageSize(width, height);
	}
	int load(const char* fileName) const {
		LOG("load", fileName);
		return BaseClass::load(fileName);
	}
	int load(const std::string& fileName) const {
		LOG("load", fileName);
		return BaseClass::load(fileName);
	}
	int loadAsText(const char* text) const {
		LOG("loadAsText", text);
		return BaseClass::loadAsText(text);
	}
	int loadAsText(const std::string& text) const {
		LOG("loadAsText", text);
		return BaseClass::loadAsText(text);
	}
	int append(const char* fileName) const {
		LOG("append", fileName);
		return BaseClass::append(fileName);
	}
	int append(const std::string& fileName) const {
		LOG("append", fileName);
		return BaseClass::append(fileName);
	}
	int appendAsText(const char* text) const {
		LOG("appendAsText", text);
		return BaseClass::appendAsText(text);
	}
	int appendAsText(const std::string& text) const {
		LOG("appendAsText", text);
		return BaseClass::appendAsText(text);
	}
	void start() const {
		LOG("start");
		return BaseClass::start();
	}
	void startSync() const {
		LOG("startSync");
		return BaseClass::startSync();
	}
	void renderSequence() const {
		LOG("renderSequence");
		return BaseClass::renderSequence();
	}
	void renderSequence(SubSequenceDesc descriptions[], size_t count) const {
		LOG("renderSequence", descriptions, count);
		return BaseClass::renderSequence(descriptions, count);
	}
	void continueSequence() const {
		LOG("continueSequence");
		return BaseClass::continueSequence();
	}
	void stop() const {
		LOG("stop");
		return BaseClass::stop();
	}
	int exportScene(const char* filePath) const {
		LOG("exportScene", filePath);
		return BaseClass::exportScene(filePath);
	}
	int exportScene(const std::string& filePath) const {
		LOG("exportScene", filePath);
		return BaseClass::exportScene(filePath);
	}
	int exportScene(const char* filePath, const VRayExportSettings& settings) const {
		LOG("exportScene", filePath, settings);
		return BaseClass::exportScene(filePath, settings);
	}
	int exportScene(const std::string& filePath, const VRayExportSettings& settings) const {
		LOG("exportScene", filePath, settings);
		return BaseClass::exportScene(filePath, settings);
	}
	void bucketRenderNearest(int x, int y) const {
		LOG("bucketRenderNearest", x, y);
		return BaseClass::bucketRenderNearest(x, y);
	}
	AddHostsResult addHosts(const char* hosts) const {
		LOG("addHosts", hosts);
		return BaseClass::addHosts(hosts);
	}
	AddHostsResult addHosts(const std::string& hosts) const {
		LOG("addHosts", hosts);
		return BaseClass::addHosts(hosts);
	}
	int removeHosts(const char* hosts) const {
		LOG("removeHosts", hosts);
		return BaseClass::removeHosts(hosts);
	}
	int removeHosts(const std::string& hosts) const {
		LOG("removeHosts", hosts);
		return BaseClass::removeHosts(hosts);
	}
	int resetHosts(const char* hosts = NULL) const {
		LOG("resetHosts", hosts);
		return BaseClass::resetHosts(hosts);
	}
	int resetHosts(const std::string& hosts) const {
		LOG("resetHosts", hosts);
		return BaseClass::resetHosts(hosts);
	}
	VRayImage* getImage() const {
		LOG(" getImage");
		return BaseClass::getImage();
	}
	VRayImage* getImage(const GetImageOptions &options) const {
		LOG(" getImage", options);
		return BaseClass::getImage(options);
	}
	bool saveImage(const char* fileName) const {
		LOG("saveImage", fileName);
		return BaseClass::saveImage(fileName);
	}
	bool saveImage(const std::string& fileName) const {
		LOG("saveImage", fileName);
		return BaseClass::saveImage(fileName);
	}
	bool saveImage(const char* fileName, const ImageWriterOptions& options) const {
		LOG("saveImage", fileName, options);
		return BaseClass::saveImage(fileName, options);
	}
	bool saveImage(const std::string& fileName, const ImageWriterOptions& options) const {
		LOG("saveImage", fileName, options);
		return BaseClass::saveImage(fileName, options);
	}
	RendererState getState() const {
		LOG("getState");
		return BaseClass::getState();
	}
	bool isImageReady() const {
		LOG("isImageReady");
		return BaseClass::isImageReady();
	}
	bool isAborted() const {
		LOG("isAborted");
		return BaseClass::isAborted();
	}
	bool isSequenceDone() const {
		LOG("isSequenceDone");
		return BaseClass::isSequenceDone();
	}
	bool waitForImageReady() const {
		LOG("waitForImageReady");
		return BaseClass::waitForImageReady();
	}
	bool waitForImageReady(const int timeout) const {
		LOG("waitForImageReady", timeout);
		return BaseClass::waitForImageReady(timeout);
	}
	bool waitForSequenceDone() const {
		LOG("waitForSequenceDone");
		return BaseClass::waitForSequenceDone();
	}
	bool waitForSequenceDone(const int timeout) const {
		LOG("waitForSequenceDone", timeout);
		return BaseClass::waitForSequenceDone(timeout);
	}
	Plugin getInstanceOf(const char* pluginType) const {
		LOG("getInstanceOf", pluginType);
		return BaseClass::getInstanceOf(pluginType);
	}
	Plugin getInstanceOf(const std::string& pluginType) const {
		LOG("getInstanceOf", pluginType);
		return BaseClass::getInstanceOf(pluginType);
	}
	Plugin getPlugin(const char *pluginName) const {
		LOG("getPlugin", pluginName);
		return BaseClass::getPlugin(pluginName);
	}
	Plugin getPlugin(const std::string &pluginName) const {
		LOG("getPlugin", pluginName);
		return BaseClass::getPlugin(pluginName);
	}
	Plugin pickPlugin(int x, int y, int timeout) const {
		LOG("pickPlugin", x, y, timeout);
		return BaseClass::pickPlugin(x, y, timeout);
	}
	bool pluginExists(const char* pluginName) const {
		LOG("pluginExists", pluginName);
		return BaseClass::pluginExists(pluginName);
	}
	bool pluginExists(const std::string& pluginName) const {
		LOG("pluginExists", pluginName);
		return BaseClass::pluginExists(pluginName);
	}
	bool getAutoCommit() const {
		LOG("getAutoCommit");
		return BaseClass::getAutoCommit();
	}
	bool readProxyPreviewGeometry(const Plugin &proxyPlugin, VectorList &vertices, IntList &indices, int countHint = 0) const {
		LOG("readProxyPreviewGeometry", proxyPlugin, vertices, indices, countHint);
		return BaseClass::readProxyPreviewGeometry(proxyPlugin, vertices, indices, countHint);
	}
	bool readProxyPreviewHair(const Plugin &proxyPlugin, VectorList &vertices, IntList &lengths, int countHint = 0) const {
		LOG("readProxyPreviewHair", proxyPlugin, vertices, lengths, countHint);
		return BaseClass::readProxyPreviewHair(proxyPlugin, vertices, lengths, countHint);
	}
	bool readProxyPreviewParticles(const Plugin &proxyPlugin, VectorList &positions, int countHint = 0) const {
		LOG("readProxyPreviewParticles", proxyPlugin, positions, countHint);
		return BaseClass::readProxyPreviewParticles(proxyPlugin, positions, countHint);
	}
	bool readProxyPreviewData(const Plugin &proxyPlugin, ProxyReadData &readData) const {
		LOG("readProxyPreviewData", proxyPlugin, readData);
		return BaseClass::readProxyPreviewData(proxyPlugin, readData);
	}
	bool readProxyFullData(const Plugin &proxyPlugin, ProxyReadData &readData) const {
		LOG("readProxyFullData", proxyPlugin, readData);
		return BaseClass::readProxyFullData(proxyPlugin, readData);
	}



	std::vector<Plugin> getPlugins(const std::string& pluginClassName) const {
		LOG("getPlugins", pluginClassName);
		return BaseClass::getPlugins(pluginClassName);
	}
	std::vector<PluginDistancePair> pickPlugins(double x, double y, int maxcount = 0, int timeout = -1) const {
		LOG("pickPlugins", x, y, maxcount, timeout);
		return BaseClass::pickPlugins(x, y, maxcount, timeout);
	}
	std::string getAllHosts() const {
		LOG("getAllHosts");
		return BaseClass::getAllHosts();
	}
	std::string getActiveHosts() const {
		LOG("getActiveHosts");
		return BaseClass::getActiveHosts();
	}
	std::string getInactiveHosts() const {
		LOG("getInactiveHosts");
		return BaseClass::getInactiveHosts();
	}
	std::vector<std::string> getPluginNames() const {
		LOG("getPluginNames");
		return BaseClass::getPluginNames();
	}
	std::vector<Plugin> getPlugins() const {
		LOG("getPlugins");
		return BaseClass::getPlugins();
	}
	std::vector<std::string> getAvailablePlugins() const {
		LOG("getAvailablePlugins");
		return BaseClass::getAvailablePlugins();
	}
	RendererOptions::RenderMode getRenderMode() {
		LOG("getRenderMode");
		return BaseClass::getRenderMode();
	}
	std::vector<ComputeDeviceInfo> getComputeDevicesCUDA() {
		LOG("getComputeDevicesCUDA");
		return BaseClass::getComputeDevicesCUDA();
	}
	std::vector<ComputeDeviceInfo> getComputeDevicesOpenCL() {
		LOG("getComputeDevicesOpenCL");
		return BaseClass::getComputeDevicesOpenCL();
	}
	std::vector<ComputeDeviceInfo> getComputeDevicesCurrentEngine() {
		LOG("getComputeDevicesCurrentEngine");
		return BaseClass::getComputeDevicesCurrentEngine();
	}

	std::vector<Plugin> getPlugins(const char* pluginClassName) const {
		LOG("getPlugins", pluginClassName);
		return BaseClass::getPlugins(pluginClassName);
	}

	template<class T, bool (T::*TMethod)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*)> int loadFiltered(const char* fileName, T& obj, const void* userData = NULL) {
		LOG("loadFiltered", fileName, obj, userData)
		return BaseClass::loadFiltered<T, T::TMethod>(fileName, obj, userData);
	}
	template<class T, bool (T::*TMethod)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*)> int loadFiltered(const std::string& fileName, T& obj, const void* userData = NULL) {
		LOG("loadFiltered", fileName, obj, userData)
		return BaseClass::loadFiltered<T, T::TMethod>(fileName, obj, userData);
	}
	template<class T, bool (T::*TMethod)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*)> int appendFiltered(const char* fileName, T& obj, const void* userData = NULL) {
		LOG("appendFiltered", fileName, obj, userData)
		return BaseClass::appendFiltered<T, T::TMethod>(fileName, obj, userData);
	}
	template<class T, bool (T::*TMethod)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*)> int appendFiltered(const std::string& fileName, T& obj, const void* userData = NULL) {
		LOG("appendFiltered", fileName, obj, userData)
		return BaseClass::appendFiltered<T, T::TMethod>(fileName, obj, userData);
	}
	template<class T, bool (T::*TMethod)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*)> int loadAsTextFiltered(const char* fileName, T& obj, const void* userData = NULL) {
		LOG("loadAsTextFiltered", fileName, obj, userData)
		return BaseClass::loadAsTextFiltered<T, T::TMethod>(fileName, obj, userData);
	}
	template<class T, bool (T::*TMethod)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*)> int loadAsTextFiltered(const std::string& fileName, T& obj, const void* userData = NULL){
		LOG("loadAsTextFiltered", fileName, obj, userData)
		return BaseClass::loadAsTextFiltered<T, T::TMethod>(fileName, obj, userData);
	}
	template<class T, bool (T::*TMethod)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*)> int appendAsTextFiltered(const char* fileName, T& obj, const void* userData = NULL) {
		LOG("appendAsTextFiltered", fileName, obj, userData)
		return BaseClass::appendAsTextFiltered<T, T::TMethod>(fileName, obj, userData);
	}
	template<class T, bool (T::*TMethod)(VRayRenderer&, const char* pluginType, std::string& pluginName, void*)> int appendAsTextFiltered(const std::string& fileName, T& obj, const void* userData = NULL) {
		LOG("appendAsTextFiltered", fileName, obj, userData)
		return BaseClass::appendAsTextFiltered<T, T::TMethod>(fileName, obj, userData);
	}
	template<class T> std::vector<T> getPlugins() const {
		LOG("getPlugins");
		return BaseClass::getPlugins();
	}
	template<class T> T getInstanceOf() const {
		LOG("getInstanceOf");
		return BaseClass::getInstanceOf<T>();
	}
	template<class T> T getInstanceOrCreate() {
		LOG("getInstanceOrCreate");
		return BaseClass::getInstanceOrCreate<T>();
	}
	template<class T> T getInstanceOrCreate(const char* pluginName) {
		LOG("getInstanceOrCreate", pluginName);
		return BaseClass::getInstanceOrCreate<T>(pluginName);
	}
	template<class T> T getInstanceOrCreate(const std::string& pluginName) {
		LOG("getInstanceOrCreate", pluginName);
		return BaseClass::getInstanceOrCreate<T>(pluginName);
	}
	template<class T> T getPlugin(const char *pluginName) const {
		LOG("getPlugin", pluginName);
		return BaseClass::getPlugin<T>(pluginName);
	}
	template<class T> T getPlugin(const std::string &pluginName) const {
		LOG("getPlugin", pluginName);
		return BaseClass::getPlugin<T>(pluginName);
	}
	template<class T> T newPlugin(const char* pluginName) {
		LOG("newPlugin", pluginName);
		return BaseClass::newPlugin<T>(pluginName);
	}
	template<class T> T newPlugin(const std::string& pluginName) {
		LOG("newPlugin", pluginName);
		return BaseClass::newPlugin<T>(pluginName);
	}
	template<class T> T newPlugin() {
		LOG("newPlugin");
		return BaseClass::newPlugin<T>();
	}
	Plugin getOrCreatePlugin(const char* pluginName, const char* pluginType) {
		LOG("newPlugin", pluginName, pluginType);
		return BaseClass::newPlugin<T>(pluginName, pluginType);
	}
	template<class T> T getOrCreatePlugin(const char* pluginName) {
		LOG("getOrCreatePlugin", pluginName);
		return BaseClass::getOrCreatePlugin<T>(pluginName);
	}

	
	template<class T, void (T::*TMethod)(VRayRenderer&, void*)> void setOnRenderStart(T& cls, const void* userData = NULL) {
		LOG("setOnRenderStart")
		return BaseClass::setOnRenderStart<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, void*)> void setOnImageReady(T& cls, const void* userData = NULL) {
		LOG("setOnImageReady")
		return BaseClass::setOnImageReady<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, void*)> void setOnRendererClose(T& cls, const void* userData = NULL) {
		LOG("setOnRendererClose")
		return BaseClass::setOnRendererClose<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, VRayImage* img, void*)> void setOnRTImageUpdated(T& cls, const void* userData = NULL) {
		LOG("setOnRTImageUpdated")
		return BaseClass::setOnRTImageUpdated<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, const char* msg, int level, void*)> void setOnDumpMessage(T& cls, const void* userData = NULL) {
		LOG("setOnDumpMessage")
		return BaseClass::setOnDumpMessage<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, int x, int y, int width, int height, const char* host, void*)> void setOnBucketInit(T& cls, const void* userData = NULL) {
		LOG("setOnBucketInit")
		return BaseClass::setOnBucketInit<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, int x, int y, const char* host, VRayImage* img, void*)> void setOnBucketReady(T& cls, const void* userData = NULL) {
		LOG("setOnBucketReady")
		return BaseClass::setOnBucketReady<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, int x, int y, int width, int height, const char* host, void*)> void setOnBucketFailed(T& cls, const void* userData = NULL) {
		LOG("setOnBucketFailed")
		return BaseClass::setOnBucketFailed<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, void*)> void setOnSequenceStart(T& cls, const void* userData = NULL) {
		LOG("setOnSequenceStart")
		return BaseClass::setOnSequenceStart<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, void*)> void setOnSequenceDone(T& cls, const void* userData = NULL) {
		LOG("setOnSequenceDone")
		return BaseClass::setOnSequenceDone<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, const char* msg, int elementNumber, int elementsCount, void*)> void setOnProgress(T& cls, const void* userData = NULL) {
		LOG("setOnProgress")
		return BaseClass::setOnProgress<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, const char* propName, void*)> void setOnRenderViewChanged(T& cls, const void* userData = NULL) {
		LOG("setOnRenderViewChanged")
		return BaseClass::setOnRenderViewChanged<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, bool isRendering, void*)> void setOnRenderLast(T& cls, const void* userData = NULL) {
		LOG("setOnRenderLast")
		return BaseClass::setOnRenderLast<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, void*)> void setOnVFBClosed(T& cls, const void* userData = NULL) {
		LOG("setOnVFBClosed")
		return BaseClass::setOnVFBClosed(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, void*)> void setOnPostEffectsUpdated(T& cls, const void* userData = NULL) {
		LOG("setOnPostEffectsUpdated")
		return BaseClass::setOnPostEffectsUpdated<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, void*)> void setOnShowMessagesWindow(T& cls, const void* userData = NULL) {
		LOG("setOnShowMessagesWindow")
		return BaseClass::setOnShowMessagesWindow<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, const char* hostName, void*)> void setOnHostConnected(T& cls, const void* userData = NULL) {
		LOG("setOnHostConnected")
		return BaseClass::setOnHostConnected<T, T::TMethod>(cls, userData);
	}
	template<class T, void (T::*TMethod)(VRayRenderer&, const char* hostName, void*)> void setOnHostDisconnected(T& cls, const void* userData = NULL) {
		LOG("setOnHostDisconnected")
		return BaseClass::setOnHostDisconnected<T, T::TMethod>(cls, userData);
	}

};	
}

typedef VRay::LoggingVRayRenderer<NullLogger> VRayRenderer;
