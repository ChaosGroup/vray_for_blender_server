#include <iostream>
#include "window.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->horizontalSlider, &QSlider::sliderMoved, this, &MainWindow::send);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::send()
{
    std::cout << this->ui->horizontalSlider->value() << std::endl;
}
