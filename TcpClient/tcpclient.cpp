#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QByteArray>
#include <QDebug>
#include <QMessageBox>
#include <QHostAddress>

TcpClient::TcpClient(QWidget *parent)            //构造函数实现窗口大小重置，加载配置，连接信号与槽，发送连接服务器请求
    : QWidget(parent)
    , ui(new Ui::TcpClient)
{
    ui->setupUi(this);

    resize(500,350);
    loadConfig();

    connect(&m_tcpSocket, &QTcpSocket::connected, this, &TcpClient::showConnect);    //connected：客户端与服务器端TCP连接成功后，由客户端自己的socket发出的信号
    connect(&m_tcpSocket, &QTcpSocket::readyRead, this, &TcpClient::recvMsg);        //readyRead：服务器端通过write()将要发送的数据写入客户端的socket缓冲区后，客户端的socket自动检测到缓冲区有数据时发出的信号
                                                                                     //当服务器端发送信息到客户端时自动触发recvMsg()来处理收到的信息
    m_tcpSocket.connectToHost(QHostAddress(m_strIP),m_usPort);

    pBook=OpeWidget::getInstance().getBook();
    pFriend=OpeWidget::getInstance().getFriend();
    pPrivateChat=OpeWidget::getInstance().getFriend()->getPrivateChat();
}

TcpClient::~TcpClient()                          //析构函数
{
    delete ui;
}

void TcpClient::loadConfig()                     //加载配置实现
{
    QFile file(":/client.config");
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

TcpClient &TcpClient::getInstance()              //保证始终只有一个TcpClient实例
{
    static TcpClient instance;
    return instance;
}

QTcpSocket &TcpClient::getTcpSocket()            //获取私有成员m_tcpSocket
{
    return m_tcpSocket;
}

QString TcpClient::loginName()                   //获取私有成员m_strLoginName
{
    return m_strLoginName;
}

QString TcpClient::curPath()                     //获取私有成员m_strCurPath
{
    return m_strCurPath;
}

void TcpClient::setCurPath(QString strCurPath)   //修改私有成员m_strCurPath
{
    m_strCurPath=strCurPath;
}

void TcpClient::showConnect()                    //connected信号槽函数:客户端与服务器端连接成功后发出提示
{
    QMessageBox::information(this,"连接服务器","连接服务器成功");    //出现连接服务器成功的提示框
}


void TcpClient::recvMsg()                        //readReady槽函数:客户端收到服务器端信息后根据消息类型对应处理
{
    if (!pBook->getDownloadStatus())
    {
        //qDebug()<<m_tcpSocket.bytesAvailable();
        uint uiPDULen=0;
        m_tcpSocket.read((char*)&uiPDULen,sizeof(uint));                  //获得uiPDULen
        uint uiMsgLen=uiPDULen-sizeof(PDU);                               //获得uiMsgLen
        PDU *pdu=mkPDU(uiMsgLen);                                         //将pdu所在内存全部置零并赋值pdu->uiPDULen/pdu->uiMsgLen
        m_tcpSocket.read(((char*)pdu)+sizeof(uint),uiPDULen-sizeof(uint));//将respdu->uiPDULen后的内容读入pdu


        switch(pdu->uiMsgType)
        {
        case ENUM_MSG_TYPE_REGIST_RESPOND:             //注册回复
        {
            QMessageBox::information(this,"注册",pdu->caData);
            break;
        }

        case ENUM_MSG_TYPE_LOGIN_RESPOND:              //登录回复
        {
            if (strcmp(pdu->caData,LOGIN_OK)==0)       //登录成功:保存用户根目录路径/显示主窗口/隐藏登录窗口
            {
                m_strCurPath=QString("./%1").arg(m_strLoginName);
                OpeWidget::getInstance().show();
                hide();
            }
            else                                       //显示登录失败原因:用户未注册/密码错误/重复登录
            {
                QMessageBox::warning(this,"登录",pdu->caData);
            }

            break;
        }

        case ENUM_MSG_TYPE_CANCEL_RESPOND:             //注销回复
        {
            QMessageBox::information(this,"注销",pdu->caData);
            break;
        }

        case ENUM_MSG_TYPE_ALL_ONLINE_RESPOND:         //显示在线用户回复
        {
            pFriend->showAllOnlineUsr(pdu);
            break;
        }

        case ENUM_MSG_TYPE_SEARCH_USR_RESPOND:         //查找用户回复
        {
            QMessageBox::information(this,"搜索",QString("%1:%2").arg(pFriend->m_strSearchName,pdu->caData));
            break;
        }

        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:         //发向加好友被请求方的，比如起初A向B发送加好友请求，最后B接收到这个请求
        {
            char caNameUtf8[32]={'\0'};
            strncpy(caNameUtf8,pdu->caData+32,32);         //加好友请求方
            QString caName=QString::fromUtf8(caNameUtf8);
            QMessageBox::StandardButton ret=QMessageBox::question(this,"添加好友",QString("%1 want to add you as friend?").arg(caName),QMessageBox::Yes|QMessageBox::No);//弹出有yes no选项的提示框

            PDU *respdu=mkPDU(0);
            memcpy(respdu->caData,pdu->caData,32);         //加好友被请求方
            memcpy(respdu->caData+32,pdu->caData+32,32);   //加好友请求方
            if (ret==QMessageBox::Yes)                     //B同意A的加好友请求，则请求服务器修改数据库，添加A B好友关系
            {
                respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_AGREE;

                m_tcpSocket.write((char *)respdu,respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            else                                           //B不同意A的加好友请求
            {
                free(respdu);
                respdu=NULL;
            }
            break;
        }

        case ENUM_MSG_TYPE_ADD_FRIEND_RESPOND:         //未知错误/双方已是好友/不是好友在线/不是好友不在线/不存在该用户
        {
            QMessageBox::information(this,"添加好友",pdu->caData);
            pFriend->setOnlinePB(true);
            break;
        }

        case ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND:       //刷新好友回复，更新好友列表
        {
            pFriend->updateFriendList(pdu);
            break;
        }

        case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST:      //删除好友请求，提示给删除好友被请求方
        {
            char caNameUtf8[32]={'\0'};
            memcpy(caNameUtf8,pdu->caData,32);
            QString caName=QString::fromUtf8(caNameUtf8);

            QMessageBox::information(this,"删除好友",QString("%1删除了你作为他的好友").arg(caName));
            pFriend->flushFriend();
            break;
        }

        case ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND:      //删除好友回复，回复给删除好友请求方
        {
            QMessageBox::information(this,"删除好友",pdu->caData);
            pFriend->flushFriend();
            break;
        }

        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:       //私聊请求
        {
            if (pPrivateChat->isHidden())
            {
                pPrivateChat->show();
            }

            char caSendNameUtf8[32]={'\0'};
            memcpy(caSendNameUtf8,pdu->caData,32);
            QString caSendName=QString::fromUtf8(caSendNameUtf8);

            pPrivateChat->setChatName(caSendName);
            pPrivateChat->updateMsg(pdu);
            break;
        }

        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:         //群聊请求
        {
            pFriend->updateGroupMsg(pdu);
            break;
        }

        case ENUM_MSG_TYPE_CREATE_DIR_RESPOND:         //创建文件夹回复
        {
            QMessageBox::information(this,"创建文件夹",pdu->caData);
            pBook->flushFile();
            break;
        }

        case ENUM_MSG_TYPE_FLUSH_FILE_RESPOND:         //刷新文件夹回复(刷新按钮/返回按钮的回复都会转到这里)
        {
            pBook->updateFileList(pdu);
            break;
        }

        case ENUM_MSG_TYPE_DEL_RESPOND:                //删除文件回复
        {
            QMessageBox::information(this,"删除文件",pdu->caData);
            pBook->flushFile();
            break;
        }

        case ENUM_MSG_TYPE_RENAME_RESPOND:             //重命名回复
        {
            QMessageBox::information(this,"重命名",pdu->caData);
            pBook->flushFile();
            break;
        }

        case ENUM_MSG_TYPE_ENTER_DIR_REQUEST_SUCCESS:  //进入文件夹请求成功
        {
            pBook->updateFileList(pdu);               //刷新文件显示

            QString strEnterDir=pBook->getEnterDir();
            m_strCurPath=m_strCurPath+"/"+strEnterDir;//更新当前路径并保存
            pBook->clearEnterDir();                   //清除双击时m_strEnterDir保存的信息

            qDebug()<<"enter dir:"<<m_strCurPath;
            break;
        }

        case ENUM_MSG_TYPE_ENTER_DIR_RESPOND:          //进入文件夹回复
        {
            pBook->clearEnterDir();
            QMessageBox::information(this,"进入文件夹",pdu->caData);
            break;
        }

        case ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND:        //上传文件回复
        {
            QMessageBox::information(this,"上传文件",pdu->caData);
            pBook->flushFile();
            break;
        }

        case ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND:      //下载文件回复
        {
            char caFileName[32]={'\0'};
            sscanf(pdu->caData,"%s %lld",caFileName,&(pBook->m_iTotal));
            if (strlen(caFileName)>0&&pBook->m_iTotal>0)//要下载的文件名不为空且大小大于0
            {
                pBook->setDownloadStatus(true);         //修改m_bDownload下载状态

                m_file.setFileName(pBook->getSaveFilePath());
                if (!m_file.open(QIODevice::WriteOnly))                              //以只写的方式打开文件，若文件不存在则自动创建(通常要下载的文件不在本地存在，所以要自动创建同名文件）
                {
                    QMessageBox::warning(this,"下载文件","获得文件保存路径失败");
                }
            }
            break;
        }

        case ENUM_MSG_TYPE_SHARE_FILE_RESPOND:         //分享文件回复:由分享文件发起者接收的回复
        {
            QMessageBox::information(this,"分享文件",pdu->caData);
            break;
        }

        case ENUM_MSG_TYPE_SHARE_FILE_NOTE:            //分享文件通知:由分享文件接收方接收的通知
        {
            char caSendNameUtf8[32]={'\0'};
            strncpy(caSendNameUtf8,pdu->caData,32);
            QString caSendName=QString::fromUtf8(caSendNameUtf8);    //获得分享文件发起方

            char *pPathUtf8=new char[pdu->uiMsgLen];
            memcpy(pPathUtf8,pdu->caMsg,pdu->uiMsgLen);
            QString strPath=QString::fromUtf8(pPathUtf8);            //获得文件具体路径

            int lastSlashIndex=strPath.lastIndexOf('/');
            QString fileName;
            if(lastSlashIndex!=-1)
            {
                fileName=strPath.mid(lastSlashIndex + 1);
            }

            if (!fileName.isEmpty())
            {
                QString strNote=QString("%1 share %2 to you.\n Do you accept?").arg(caSendName,fileName);

                int ret=QMessageBox::question(this,"共享文件",strNote);  //给出yes no选项
                if (QMessageBox::Yes==ret)
                {
                    PDU *respdu=mkPDU(pdu->uiMsgLen);
                    respdu->uiMsgType=ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND;
                    memcpy(respdu->caMsg,pdu->caMsg,pdu->uiMsgLen);

                    QString strName=TcpClient::getInstance().loginName();
                    strncpy(respdu->caData,strName.toUtf8().constData(),strName.toUtf8().size());
                    m_tcpSocket.write((char *)respdu,respdu->uiPDULen);
                    free(respdu);
                    respdu=NULL;
                }
            }

            delete[] pPathUtf8;
            pPathUtf8=nullptr;
            break;
        }

        case ENUM_MSG_TYPE_PASTE_RESPOND:              //粘贴文件回复
        {
            QMessageBox::information(this,"粘贴文件",pdu->caData);
            pBook->flushFile();
            break;
        }

        default:
            break;
        }

        free(pdu);
        pdu=NULL;
    }
    else
    {
        while (m_tcpSocket.bytesAvailable()>0)                       //循环读取服务器的发送
        {
            char pBuffer[4096]={'\0'};
            qint64 remainLen=pBook->m_iTotal-pBook->m_iRecved;       //每接收到一次数据后还需读取的大小
            qint64 readSize=qMin(remainLen, (qint64)4096);
            qint64 recvLen=m_tcpSocket.read(pBuffer, readSize);      //实际读取的大小

            if (recvLen>0)
            {
                m_file.write(pBuffer,recvLen);
                pBook->m_iRecved+=recvLen;
            }
        }

        if (pBook->m_iTotal==pBook->m_iRecved)                       //读取成功:m_bUpload值置为false,关闭文件,给客户端发送提示
        {
            pBook->setDownloadStatus(false);
            QMessageBox::information(this,"分享文件","DOWNLOAD_FILE_OK");
            m_file.close();
        }
        else if (pBook->m_iTotal<pBook->m_iRecved)
        {
            pBook->setDownloadStatus(false);
            QMessageBox::information(this,"分享文件","DOWNLOAD_FILE_FAILED");
            m_file.close();
        }
    }
}


void TcpClient::on_regist_pb_clicked()           //注册按钮槽函数:把用户名和密码发送到服务器
{
    QString strName=ui->name_le->text();
    QString strPwd=ui->pwd_le->text();
    if (!strName.isEmpty()&&!strPwd.isEmpty())
    {
        PDU* pdu=mkPDU(0);
        pdu->uiMsgType=ENUM_MSG_TYPE_REGIST_REQUEST;
        QByteArray strNameUtf8=strName.toUtf8();
        QByteArray strPwdUtf8=strPwd.toUtf8();
        if (strNameUtf8.size()<=31&&strPwdUtf8.size()<=31)
        {
            strncpy(pdu->caData,strNameUtf8.constData(),31);
            strncpy(pdu->caData+32,strPwdUtf8.constData(),31);

            m_tcpSocket.write((char*)pdu,pdu->uiPDULen);
            free(pdu);
            pdu=NULL;
        }
        else
        {
            QMessageBox::critical(this,"注册","注册失败：用户名或密码过长");
        }

    }
    else
    {
        QMessageBox::critical(this,"注册","注册失败：用户名或密码为空");
    }
}

void TcpClient::on_login_pb_clicked()            //登录按钮槽函数:把用户名和密码发送到服务器
{
    QString strName=ui->name_le->text();
    QString strPwd=ui->pwd_le->text();
    if (!strName.isEmpty()&&!strPwd.isEmpty())
    {
        m_strLoginName=strName;                  //保存用户名方便后续使用

        PDU* pdu=mkPDU(0);
        pdu->uiMsgType=ENUM_MSG_TYPE_LOGIN_REQUEST;
        QByteArray strNameUtf8=strName.toUtf8();
        QByteArray strPwdUtf8=strPwd.toUtf8();
        if (strNameUtf8.size()<=31&&strPwdUtf8.size()<=31)
        {
            strncpy(pdu->caData,strNameUtf8.constData(),31);
            strncpy(pdu->caData+32,strPwdUtf8.constData(),31);

            m_tcpSocket.write((char*)pdu,pdu->uiPDULen);
            free(pdu);
            pdu=NULL;
        }
        else
        {
            QMessageBox::critical(this,"登录","登录失败：用户名或密码过长");
        }

    }
    else
    {
        QMessageBox::critical(this,"登录","登录失败：用户名或密码为空");
    }
}

void TcpClient::on_cancel_pb_clicked()           //注销按钮槽函数:确认注销操作后把用户名和密码发送到服务器
{
    QMessageBox::StandardButton ret=QMessageBox::question(this,"注销","注销后您将永久失去账号信息",QMessageBox::Yes|QMessageBox::No);//弹出有yes no选项的提示框

    if (ret==QMessageBox::Yes)                   //确认注销
    {
        QString strName=ui->name_le->text();
        QString strPwd=ui->pwd_le->text();
        if (!strName.isEmpty()&&!strPwd.isEmpty())
        {
            PDU* pdu=mkPDU(0);
            pdu->uiMsgType=ENUM_MSG_TYPE_CANCEL_REQUEST;
            QByteArray strNameUtf8=strName.toUtf8();
            QByteArray strPwdUtf8=strPwd.toUtf8();
            if (strNameUtf8.size()<=31&&strPwdUtf8.size()<=31)
            {
                strncpy(pdu->caData,strNameUtf8.constData(),31);
                strncpy(pdu->caData+32,strPwdUtf8.constData(),31);

                m_tcpSocket.write((char*)pdu,pdu->uiPDULen);
                free(pdu);
                pdu=NULL;
            }
            else
            {
                QMessageBox::critical(this,"注销","注销失败：用户名或密码过长");
            }
        }
        else
        {
            QMessageBox::critical(this,"注销","注销失败：用户名或密码为空");
        }
    }
    else if (ret==QMessageBox::No)               //取消注销操作
    {
        return;
    }
}
