#include "window.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QSlider>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
