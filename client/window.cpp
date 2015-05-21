#include <iostream>
#include "window.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow), address("127.0.0.1:5555"), pl_name("OBCube_001")
{
	ui->setupUi(this);

	connect(ui->horizontalSlider, &QSlider::sliderMoved, this, &MainWindow::send);
	connect(ui->horizontalSlider_2, &QSlider::sliderMoved, this, &MainWindow::send);
	this->ui->label->resize(640, 480);

	client.setCallback([this](VRayMessage & message, ZmqWrapper * client) {
		const auto img = message.getValue<VRayBaseTypes::AttrImage>();
		QPixmap map;

		std::cout << "IMG TYPE:" << img->imageType << "  " << img->size << "  " << message.getMessage().size() << std::endl;
		if (img->imageType == VRayBaseTypes::AttrImage::ImageType::JPG) {
			map.loadFromData(reinterpret_cast<const uchar *>(img->data.get()), img->size, nullptr);
		} else {
			const float * data = reinterpret_cast<const float *>(img->data.get());

			QImage image(img->width, img->height, QImage::Format_ARGB32);

			for (int w = 0; w < img->width; ++w) {
				for (int h = 0; h < img->height; ++h) {
					const float * pixel = data + img->width * h * 4 + w * 4;
					QRgb col = qRgba(pixel[0] * 255.0f,
									  pixel[1] * 255.0f,
									  pixel[2] * 255.0f,
									  pixel[3] * 255.0f);

					image.setPixel(w, h, col);
				}
			}
			map = QPixmap::fromImage(image);
		}

		this->ui->label->setPixmap(map);
	});
	this->connectServer();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::connectServer() {
	QString address("tcp://" + this->address);
	client.connect(address.toStdString().c_str());

	client.send(VRayMessage::createMessage(VRayMessage::RendererAction::LoadScene, "D:/dev/cornellbox.vrscene"));
	client.send(VRayMessage::createMessage(VRayMessage::RendererAction::Start));
}

void MainWindow::send()
{
	const char * tr_name = "transform";

	int value;
	if (!strcmp(pl_name, "OBSphere")) {
		value = this->ui->horizontalSlider->value();
	} else {
		value = this->ui->horizontalSlider_2->value();
	}

	VRayBaseTypes::AttrTransformBase * transform = reinterpret_cast<VRayBaseTypes::AttrTransformBase *>(tr_data);
	transform->offs.x = ((float)value - 50) / 50;

	VRayMessage msg = VRayMessage::createMessage(pl_name, tr_name, *transform);
	this->client.send(msg);
}

void MainWindow::on_pushButton_clicked()
{
	this->connectServer();
}

void MainWindow::on_horizontalSlider_2_sliderPressed()
{
	static unsigned char data[] = {0x17, 0xee, 0x68, 0x3f, 0x32, 0x69, 0xd4, 0x3e, 0x0, 0x0, 0x0, 0x0, 0x32, 0x69, 0xd4, 0xbe, 0x17, 0xee, 0x68, 0x3f, 0x0, 0x0,0x0, 0x0,
								   0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0x3f, 0x9b, 0x5c, 0xff, 0xbe, 0xa, 0x80, 0x64, 0x3e, 0x27, 0x4d, 0x40, 0x3e};
	memcpy(tr_data, data, sizeof(data));
	pl_name = "OBCube_001";
}

void MainWindow::on_horizontalSlider_sliderPressed()
{
	static unsigned char data[] = {0xf2, 0xca, 0x8e, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf2, 0xca, 0x8e, 0x3f, 0x00, 0x00, 0x00, 0x00,
								   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf2, 0xca, 0x8e, 0x3f, 0xcd, 0xcc, 0x4c, 0x3e, 0x59, 0x4a, 0x08, 0x3f, 0xc0, 0x68, 0x02, 0x3e};
	memcpy(tr_data, data, sizeof(data));
	pl_name = "OBSphere";
}
