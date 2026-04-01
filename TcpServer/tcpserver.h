#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QWidget>
#include "mytcpserver.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class TcpServer;
}
QT_END_NAMESPACE

class TcpServer : public QWidget
{
    Q_OBJECT

public:
    TcpServer(QWidget *parent = nullptr);
    ~TcpServer();
    void loadConfig();

private:
    Ui::TcpServer *ui;
    QString m_strIP;
    quint16 m_usPort;
};                           //服务器端的socket只负责和客户端通信，由MyTcpSerevr的父类QTcpServer负责监听客户端的连接请求

#endif // TCPSERVER_H
