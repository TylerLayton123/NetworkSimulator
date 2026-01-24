#include "netsim.h"
#include "ui_netsim.h"

NetSim::NetSim(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NetSim)
{
    ui->setupUi(this);
}

NetSim::~NetSim()
{
    delete ui;
}
