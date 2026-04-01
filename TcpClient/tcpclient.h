#ifndef TCPCLIENT_H
#define TCPCLIENT_H

//初始登录界面
#include <QWidget>
#include <QFile>
#include <QTcpSocket>
#include "protocol.h"
#include "opewidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class TcpClient;
}
QT_END_NAMESPACE

class TcpClient : public QWidget
{
    Q_OBJECT
public:
    TcpClient(QWidget *parent = nullptr);
    ~TcpClient();

    void loadConfig();                      //读取配置文件
    static TcpClient &getInstance();        //单例模式
    QTcpSocket &getTcpSocket();             //获取m_tcpSocket
    QString loginName();                    //获取用户名m_strLoginName
    QString curPath();                      //获取当前路径m_strCurPath
    void setCurPath(QString strCurPath);    //设置当前路径

public slots:
    void showConnect();                     //与服务器连接成功槽函数
    void recvMsg();                         //接收到服务器发送信息槽函数

private slots:
    void on_regist_pb_clicked();            //注册按钮槽函数
    void on_login_pb_clicked();             //登录按钮槽函数
    void on_cancel_pb_clicked();            //注销按钮槽函数

private:
    Ui::TcpClient *ui;
    QString m_strIP;
    quint16 m_usPort;

    QTcpSocket m_tcpSocket;                 //客户端的socket既负责连接又负责通信
    QString m_strLoginName;

    QString m_strCurPath;                   //当前路径

    Book *pBook;
    Friend *pFriend;
    PrivateChat *pPrivateChat;
    QFile m_file;
};

#endif // TCPCLIENT_H
