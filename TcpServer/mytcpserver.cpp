#include "mytcpserver.h"
#include <qDebug>

MyTcpServer::MyTcpServer() {}                                  //函数体为空的原因：MyTcpServer 继承自 QTcpServer，父类 QTcpServer 的构造函数已经完成了 TCP 服务器的基础初始化（比如底层网络资源分配），你暂时不需要在子类构造中添加额外逻辑（若需加日志、初始化参数等，可在此补充）

MyTcpServer &MyTcpServer::getInstance()                        //MyTcpServer &:返回值类型为MyTcpServer 对象的引用（别名），保证单一实例
{                                                              //static:  只初始化一次：第一次调用 getInstance() 时创建，后续调用不再创建；
    static MyTcpServer instance;
    return instance;
}

void MyTcpServer::incomingConnection(qintptr socketDescriptor) //核心作用:为每一个与服务器成功建立 TCP 连接的客户端，创建一个专属的socket来管理两端的通信
{                                                              //socketDescriptor实现了区别与不同客户端通信的socket，且由系统自动生成并传入,本质是一个整数ID
    qDebug()<<"new client connected";
    MyTcpSocket *pTcpSocket=new MyTcpSocket;
    pTcpSocket->setSocketDescriptor(socketDescriptor);
    m_tcpSocketList.append(pTcpSocket);

    connect(pTcpSocket,&MyTcpSocket::offline,this,&MyTcpServer::deleteSocket);
}


void MyTcpServer::resend(const QString& pername, PDU *pdu)     //信息转发
{
    if (pername.isEmpty()||pdu==NULL)
    {
        return;
    }

    QString strName=pername;                          //加好友被请求方
    for (int i=0;i<m_tcpSocketList.size();i++)
    {
        if (m_tcpSocketList.at(i)->getName()==strName)//找到与被请求方通信的socket
        {
            m_tcpSocketList.at(i)->write((char *)pdu,pdu->uiPDULen);
            break;
        }
    }
}

void MyTcpServer::deleteSocket(MyTcpSocket *mysocket)          //客户下线时删除服务器端对应的socket
{
    QList<MyTcpSocket*>::iterator iter=m_tcpSocketList.begin();
    for(;iter!=m_tcpSocketList.end();iter++)
    {
        if (mysocket==*iter)
        {
            delete *iter;
            *iter=NULL;                                        //这两行是删除与客户端连接的socket,释放目标 Socket 对象的内存
            m_tcpSocketList.erase(iter);                       //调用erase()从列表中移除迭代器指向的元素，列表长度减 1
            break;
        }
    }

    for (int i=0;i<m_tcpSocketList.size();i++)
    {
        qDebug()<<m_tcpSocketList.at(i)->getName();
    }
}
