#include "window.h"
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

	bool ok;
	int port = parser.value(portOption).toInt(&ok);
	if (!ok) {
		std::cerr << "Failed to set port ";
		return 1;
	}

	MainWindow w;
	w.setListeningPort(port);

	if (parser.isSet(vfbOption)) {
		w.setShowVFB(true);
	}

	if (!w.start()) {
		return 1;
	}

	//w.show();

	return app.exec();
}
