#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QTcpServer>
#include <QList>
#include "mytcpsocket.h"

class MyTcpServer : public QTcpServer       //QTcpServer只负责监听
{
    Q_OBJECT
public:
    static MyTcpServer &getInstance();

    void incomingConnection(qintptr socketDescriptor);     //函数何时调用：1.服务器调用 MyTcpServer::listen(QHostAddress::Any, 8888)，开始监听端口；
                                                           //2.客户端调用 QTcpSocket::connectToHost("服务器IP", "服务器端口号")，发起 TCP 连接请求；
                                                           //3.服务器操作系统完成 TCP 三次握手，确认新连接建立；
                                                           //4.QTcpServer内部检测到这个新连接，自动调用重写的incomingConnection函数，并把 “新连接的描述符” 作为参数传进来；
                                                           //5.你在incomingConnection里创建 MyTcpSocket，绑定描述符 —— 至此，服务器和该客户端的通信通道正式打通。
                                                           //核心作用:为每一个与服务器成功建立 TCP 连接的客户端，创建一个专属的socket来管理两端的通信
    void resend(const QString& pername,PDU *pdu);

public slots:
    void deleteSocket(MyTcpSocket * mysocket);

private:
    MyTcpServer();

    QList<MyTcpSocket*> m_tcpSocketList;
};    //单例模式保证程序运行期间只有一个 TCP 服务器实例，即保证服务器的同一个端口只能被一个进程的一个实例监听。

#endif // MYTCPSERVER_H
