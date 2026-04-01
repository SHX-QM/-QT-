#include "tcpserver.h"
#include "ui_tcpserver.h"
#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QHostAddress>
#include <QFile>

TcpServer::TcpServer(QWidget *parent)//监听功能要写在tcpserver.ccp而不是mytcpserevr.cpp是因为:main函数中TcpServer w；创建w时自动调用构造函数同时逻辑上要同时启动对客户端的监听
    : QWidget(parent)
    , ui(new Ui::TcpServer)
{
    ui->setupUi(this);

    loadConfig();
    MyTcpServer::getInstance().listen(QHostAddress(m_strIP),m_usPort);    //开始监听：监听到连接信号后由QTcpServer发出newConnection()信号，不是服务器端的socket发出的哦
                                                                          //服务器端的socket只负责和客户端通信，由QTcpServer负责监听客户端的连接请求
}


TcpServer::~TcpServer()
{
    delete ui;
}

void TcpServer::loadConfig()
{
    QFile file(":/server.config");
    if (file.open(QIODevice::ReadOnly)){
        QByteArray baData=file.readAll();
        QString strData=baData.toStdString().c_str();
        file.close();

        QStringList strList=strData.replace("\r\n"," ").split(" ");

        m_strIP=strList[0];
        m_usPort=strList[1].toUShort();
        qDebug()<<"ip:"<<m_strIP;
        qDebug()<<"port:"<<m_usPort;

    }
    else{
        QMessageBox::critical(this,"open config","open config failed");
    }
}
