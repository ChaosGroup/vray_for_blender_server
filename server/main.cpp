
#include "renderer_controller.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QSlider>
#include <QCommandLineParser>
#include <QDebug>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	QCoreApplication::setApplicationName("multiple-values-program");
	QCoreApplication::setApplicationVersion("1.0");


	QCommandLineParser parser;

	QCommandLineOption portOption(QStringList() << "p" << "port", "Specify listening port", "port");
	parser.addOption(portOption);

	QCommandLineOption vfbOption(QStringList() << "vfb" << "display-vfb", "Option to display the VFB", "vfb");
	parser.addOption(vfbOption);

	parser.process(app);

	if (!parser.isSet(portOption)) {
		std::cerr << "No port specified";
		return 1;
	}
	bool showVFB = parser.isSet(vfbOption);

	RendererController ctrl(parser.value(portOption).toStdString(), showVFB);


	return app.exec();
}
