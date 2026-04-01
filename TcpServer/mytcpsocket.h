#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QTcpSocket>
#include "protocol.h"
#include "opedb.h"
#include <QDir>
#include <QFile>
#include <QTimer>

class MyTcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit MyTcpSocket(QObject *parent = nullptr);
    QString getName();                      //获得私有变量m_strName
    void copyDir(QString strSrcDir, QString strDestDir);

signals:
    void offline(MyTcpSocket *mysocket);    //用户下线信号

public slots:
    void recvMsg();
    void clientOffline();                   //disconnected用户断开连接槽函数
    void sendFileToClient();                //定时器槽函数

private:
    QString m_strName;

    QFile m_file;
    qint64 m_iTotal=0;
    qint64 m_iRecved=0;
    bool m_bUpload;

    QTimer *m_pTimer;                       //定时器
};

#endif // MYTCPSOCKET_H
