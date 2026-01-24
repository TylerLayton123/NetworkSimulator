#ifndef NETSIM_H
#define NETSIM_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class NetSim;
}
QT_END_NAMESPACE

class NetSim : public QMainWindow
{
    Q_OBJECT

public:
    NetSim(QWidget *parent = nullptr);
    ~NetSim();

private:
    Ui::NetSim *ui;
};
#endif // NETSIM_H
