#include "mytcpsocket.h"
#include <QDebug>
#include <stdio.h>
#include "mytcpserver.h"
#include <QDir>
#include <QFileInfoList>
#include <QtEndian>

MyTcpSocket::MyTcpSocket(QObject *parent)
    : QTcpSocket{parent}
{
    connect(this,&QTcpSocket::readyRead,this,&MyTcpSocket::recvMsg);
    connect(this,&QTcpSocket::disconnected,this,&MyTcpSocket::clientOffline);   //disconnected信号由与客户端绑定的MyTcpSocket（继承 QTcpSocket）对象和服务器断开连接时发出（每个客户端对应一个独立的MyTcpSocket，谁的连接断了，谁就发这个信号）；

    m_bUpload=false;

    m_pTimer=new QTimer;
    connect(m_pTimer,&QTimer::timeout,this,&MyTcpSocket::sendFileToClient);     //绑定定时器+sendFileToClient
}

QString MyTcpSocket::getName()       //获得私有变量m_strName
{
    return m_strName;
}

void MyTcpSocket::copyDir(QString strSrcDir,QString strDestDir)
{
    QDir dir;
    dir.mkdir(strDestDir);

    dir.setPath(strSrcDir);
    QDir::Filters filters = QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot;//1.过滤规则：显示文件夹，显示普通文件，排除.(当前目录)和..(上级目录){操作系统在返回某个目录下的 “所有条目” 时，会默认把这两个标识符包含在内}
    QDir::SortFlags sortFlags = QDir::Name|QDir::DirsFirst;             //2.文件/文件夹排序规则：按名称排序，文件夹优先
    QFileInfoList fileInfoList = dir.entryInfoList(filters, sortFlags); //获取当前路径下所有文件/文件夹信息，返回的fileInfoList是一个包含多个QFileInfo对象的列表，每个对象对应目录里一个文件/文件夹的完整信息（名称、类型、大小、路径等）

    QString srcTmp;
    QString destTmp;
    for (int i=0;i<fileInfoList.size();i++)
    {
        if (fileInfoList[i].isFile())
        {
            srcTmp=strSrcDir+'/'+fileInfoList[i].fileName();
            destTmp=strDestDir+'/'+fileInfoList[i].fileName();
            QFile::copy(srcTmp,destTmp);
        }
        else if (fileInfoList[i].isDir())
        {
            srcTmp=strSrcDir+'/'+fileInfoList[i].fileName();
            destTmp=strDestDir+'/'+fileInfoList[i].fileName();
            copyDir(srcTmp,destTmp);
        }
    }
}

void MyTcpSocket::recvMsg()
{
    if (!m_bUpload)
    {
        //qDebug()<<this->bytesAvailable();                         //查看设备「输入缓冲区」里的 “待读取字节数”（ QTcpSocket 收到客户端发来的PDU数据，会先存在缓冲区，bytesAvailable()就是缓冲区里还没读的字节数）；
        uint uiPDULen=0;
        this->read((char*)&uiPDULen,sizeof(uint));                  //获得uiPDULen
        uint uiMsgLen=uiPDULen-sizeof(PDU);                         //获得uiMsgLen
        PDU *pdu=mkPDU(uiMsgLen);                                   //将pdu所在内存全部置零并赋值pdu->uiPDULen/pdu->uiMsgLen
        this->read(((char*)pdu)+sizeof(uint),uiPDULen-sizeof(uint));//将客户端pdu->uiPDULen后的内容读入pdu


        switch(pdu->uiMsgType)
        {
        case ENUM_MSG_TYPE_REGIST_REQUEST:            //注册请求
        {
            char caNameUtf8[32]={'\0'};                   //定义数组并把数组32个字节都初始化为'\0'
            char caPwdUtf8[32]={'\0'};
            strncpy(caNameUtf8,pdu->caData,32);           //提取用户名信息(接收到的是UTF8字节流）
            strncpy(caPwdUtf8,pdu->caData+32,32);         //提取密码信息(接收到的是UTF8字节流）
            QString caName=QString::fromUtf8(caNameUtf8); //转QString使用的UTF-16
            QString caPwd=QString::fromUtf8(caPwdUtf8);   //转QString使用的UTF-16

            bool ret=OpeDB::getInstance().handleRegist(caName,caPwd);

            PDU* respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_REGIST_RESPOND;
            if (ret)
            {
                strncpy(respdu->caData,REGIST_OK,strlen(REGIST_OK));
                QDir dir;
                qDebug()<<"create dir:"<<dir.mkdir(QString("./%1").arg(caName));    //注册成功时创建以该用户名为目录的文件夹
            }
            else
            {
                strncpy(respdu->caData,REGIST_FAILED,strlen(REGIST_FAILED));
            }

            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_LOGIN_REQUEST:             //登录请求
        {
            char caNameUtf8[32]={'\0'};
            char caPwdUtf8[32]={'\0'};
            strncpy(caNameUtf8,pdu->caData,32);
            strncpy(caPwdUtf8,pdu->caData+32,32);
            QString caName=QString::fromUtf8(caNameUtf8);
            QString caPwd=QString::fromUtf8(caPwdUtf8);

            int ret=OpeDB::getInstance().handleLogin(caName,caPwd);

            PDU* respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_LOGIN_RESPOND;
            if (ret==-1)                              //用户未注册
            {
                strncpy(respdu->caData,NAME_NOT_REGIET,strlen(NAME_NOT_REGIET));
            }
            else if (ret==0)                          //密码错误
            {
                strncpy(respdu->caData,LOGIN_FAILED_PWD,strlen(LOGIN_FAILED_PWD));
            }
            else if (ret==1)                          //重复登录
            {
                strncpy(respdu->caData,LOGIN_FAILED_ONLINE,strlen(LOGIN_FAILED_ONLINE));
            }
            else if (ret==2)                          //登录成功
            {
                strncpy(respdu->caData,LOGIN_OK,strlen(LOGIN_OK));
                m_strName=caName;                     //保存用户名
            }

            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_CANCEL_REQUEST:            //注销请求
        {
            char caNameUtf8[32]={'\0'};
            char caPwdUtf8[32]={'\0'};
            strncpy(caNameUtf8,pdu->caData,32);
            strncpy(caPwdUtf8,pdu->caData+32,32);
            QString caName=QString::fromUtf8(caNameUtf8);
            QString caPwd=QString::fromUtf8(caPwdUtf8);

            int ret=OpeDB::getInstance().handleCancel(caName,caPwd);

            PDU* respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_CANCEL_RESPOND;
            if (ret==1)                                  //数据库删除该用户信息后删除其文件
            {
                strncpy(respdu->caData,CANCEL_OK,strlen(CANCEL_OK));
                QString rootPath="./"+QString(caName);
                QDir dir(rootPath);
                dir.removeRecursively();
            }
            else if(ret==0)
            {
                strncpy(respdu->caData,CANCEL_FAILED,strlen(CANCEL_FAILED));
            }
            else if (ret==2)
            {
                strncpy(respdu->caData,NAME_NOT_REGIET,strlen(NAME_NOT_REGIET));
            }
            else
            {
                strncpy(respdu->caData,UNKNOW_ERROR,strlen(UNKNOW_ERROR));
            }

            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST:        //显示在线用户请求
        {
            char caNameUtf8[32]={'\0'};
            strncpy(caNameUtf8,pdu->caData,32);
            QString caName=QString::fromUtf8(caNameUtf8);

            QStringList ret=OpeDB::getInstance().handleAllOnline(caName);  //返回列表(存放着数据库中所有online=1的用户名)

            uint uiMsgLen=ret.size()*32;
            PDU *respdu=mkPDU(uiMsgLen);
            respdu->uiMsgType=ENUM_MSG_TYPE_ALL_ONLINE_RESPOND;
            QString userName;
            QByteArray userNameUtf8;
            for(int i=0;i<ret.size();i++)
            {
                userName=ret.at(i);
                userNameUtf8=userName.toUtf8();
                memcpy(respdu->caMsg+i*32,userNameUtf8.constData(),31);
            }

            write((char *)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_SEARCH_USR_REQUEST:        //查找用户请求
        {
            char caSearchNameUtf8[32]={'\0'};
            strncpy(caSearchNameUtf8,pdu->caData,32);
            QString caSearchName=QString::fromUtf8(caSearchNameUtf8);

            int ret=OpeDB::getInstance().handleSearchUsr(caSearchName);
            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_SEARCH_USR_RESPOND;
            if (ret==-1)        //查找的用户不存在
            {
                strncpy(respdu->caData,SEARCH_USR_NO,strlen(SEARCH_USR_NO));
            }
            else if (ret==1)    //查找的用户在线
            {
                strncpy(respdu->caData,SEARCH_USR_ONLINE,strlen(SEARCH_USR_ONLINE));
            }
            else if (ret==0)    //查找的用户不在线
            {
                strncpy(respdu->caData,SEARCH_USR_OFFLINE,strlen(SEARCH_USR_OFFLINE));
            }

            write((char *)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:        //加好友请求
        {
            char caPerNameUtf8[32]={'\0'};
            char caNameUtf8[32]={'\0'};
            strncpy(caPerNameUtf8,pdu->caData,32);    //加好友被请求方
            strncpy(caNameUtf8,pdu->caData+32,32);    //加好友请求方
            QString caPerName=QString::fromUtf8(caPerNameUtf8);
            QString caName=QString::fromUtf8(caNameUtf8);

            int ret=OpeDB::getInstance().handleAddFriend(caPerName,caName);
            PDU *respdu=NULL;
            if (ret==-1)     //未知错误
            {
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strncpy(respdu->caData,UNKNOW_ERROR,strlen(UNKNOW_ERROR));
                write((char *)respdu,respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            else if (ret==0)    //双方已是好友
            {
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strncpy(respdu->caData,EXISTED_FRIEND,strlen(EXISTED_FRIEND));
                write((char *)respdu,respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            else if (ret==1)    //不是好友但在线:转发给加好友被请求方寻求同意/拒绝
            {
                MyTcpServer::getInstance().resend(caPerName,pdu);
            }
            else if (ret==2)    //不是好友不在线
            {
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strncpy(respdu->caData,ADD_FRIEND_OFFLINE,strlen(ADD_FRIEND_OFFLINE));
                write((char *)respdu,respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            else if (ret==3)    //不存在该用户（加好友被请求方）
            {
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strncpy(respdu->caData,ADD_FRIEND_NO_EXIST,strlen(ADD_FRIEND_NO_EXIST));
                write((char *)respdu,respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            break;
        }

        case ENUM_MSG_TYPE_ADD_FRIEND_AGREE:          //B同意A的加好友请求，修改数据库
        {
            char caPerNameUtf8[32]={'\0'};
            char caNameUtf8[32]={'\0'};
            strncpy(caPerNameUtf8,pdu->caData,32);    //加好友被请求方
            strncpy(caNameUtf8,pdu->caData+32,32);    //加好友请求方
            QString caPerName=QString::fromUtf8(caPerNameUtf8);
            QString caName=QString::fromUtf8(caNameUtf8);

            if (caPerName!=caName)
            {
               OpeDB::getInstance().handleAddFriendAgree(caPerName,caName);
            }

            PDU* respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            strncpy(respdu->caData,ADD_FRIEND_OK,strlen(ADD_FRIEND_OK));

            MyTcpServer::getInstance().resend(caName,respdu);
            break;
        }

        case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:      //刷新好友请求
        {
            char caNameUtf8[32]={'\0'};
            strncpy(caNameUtf8,pdu->caData,32);
            QString caName=QString::fromUtf8(caNameUtf8);

            QStringList ret=OpeDB::getInstance().handleFlushFriend(caName);

            uint uiMsgLen=ret.size()*32;
            PDU *respdu=mkPDU(uiMsgLen);
            respdu->uiMsgType=ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;
            QString userName;
            QByteArray userNameUtf8;
            for(int i=0;i<ret.size();i++)
            {
                userName=ret.at(i);
                userNameUtf8=userName.toUtf8();
                memcpy((char *)(respdu->caMsg)+i*32,userNameUtf8.constData(),31);//显式转char*更保险
            }

            write((char *)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST:     //删除好友请求
        {
            char caSelfNameUtf8[32]={'\0'};
            char caFriendNameUtf8[32]={'\0'};
            strncpy(caSelfNameUtf8,pdu->caData,32);
            strncpy(caFriendNameUtf8,pdu->caData+32,32);
            QString caSelfName=QString::fromUtf8(caSelfNameUtf8);
            QString caFriendName=QString::fromUtf8(caFriendNameUtf8);

            bool ret=OpeDB::getInstance().handleDelFriend(caSelfName,caFriendName);

            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND;
            if (ret)
            {
                strncpy(respdu->caData,DEL_FRIEND_OK,strlen(DEL_FRIEND_OK));
            }
            else
            {
                strncpy(respdu->caData,DEL_FRIEND_FAILED,strlen(DEL_FRIEND_FAILED));
            }

            write((char *)respdu,respdu->uiPDULen);                  //发送给删除好友请求方
            free(respdu);
            respdu=NULL;

            MyTcpServer::getInstance().resend(caFriendName,pdu);     //发送给删除好友被请求方，给出被删除方提示
            break;
        }

        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:      //私聊请求
        {
            char caPerNameUtf8[32]={'\0'};
            strncpy(caPerNameUtf8,pdu->caData+32,32);
            QString caPerName=QString::fromUtf8(caPerNameUtf8);

            MyTcpServer::getInstance().resend(caPerName,pdu);
            break;
        }

        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:        //群聊请求
        {
            char caNameUtf8[32]={'\0'};
            strncpy(caNameUtf8,pdu->caData,32);
            QString caName=QString::fromUtf8(caNameUtf8);

            QStringList onlineFriend=OpeDB::getInstance().handleFlushFriend(caName);

            QString tmp;
            for (int i=0;i<onlineFriend.size();i++)
            {
                tmp=onlineFriend.at(i);
                MyTcpServer::getInstance().resend(tmp.toStdString().c_str(),pdu);
            }
            break;
        }

        case ENUM_MSG_TYPE_CREATE_DIR_REQUEST:        //创建文件夹请求
        {
            QDir dir;
            PDU *respdu=NULL;

            QString strCurPath=QString::fromUtf8(pdu->caMsg);
            bool ret=dir.exists(strCurPath);
            if (ret)         //当前目录(新文件夹存放的位置)存在
            {
                char caNewDirUtf8[32]={'\0'};
                strncpy(caNewDirUtf8,pdu->caData+32,32);
                QString caNewDir=QString::fromUtf8(caNewDirUtf8);

                QString strNewPath=strCurPath+"/"+caNewDir;
                ret=dir.exists(strNewPath);
                if (ret)     //创建的文件已存在
                {
                    respdu=mkPDU(0);
                    respdu->uiMsgType=ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                    strncpy(respdu->caData,FILE_NAME_EXIST,strlen(FILE_NAME_EXIST));
                }
                else         //创建的文件不存在
                {
                    dir.mkdir(strNewPath);
                    respdu=mkPDU(0);
                    respdu->uiMsgType=ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                    strncpy(respdu->caData,CREATE_DIR_OK,strlen(CREATE_DIR_OK));
                }
            }
            else             //当前目录不存在
            {
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                strncpy(respdu->caData,DIR_NO_EXIST,strlen(DIR_NO_EXIST));
            }

            write((char *)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_FLUSH_FILE_REQUEST:        //刷新文件夹请求
        {
            char *pCurPath=new char[pdu->uiMsgLen+1];
            memset(pCurPath,'\0',pdu->uiMsgLen+1);
            memcpy(pCurPath, pdu->caMsg, pdu->uiMsgLen);   //取出当前路径
            QString strCurPath=QString::fromUtf8(pCurPath);

            QDir dir(strCurPath);
            QDir::Filters filters = QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot;//1.过滤规则：显示文件夹，显示普通文件，排除.(当前目录)和..(上级目录){操作系统在返回某个目录下的 “所有条目” 时，会默认把这两个标识符包含在内}
            QDir::SortFlags sortFlags = QDir::Name|QDir::DirsFirst;             //2.文件/文件夹排序规则：按名称排序，文件夹优先
            QFileInfoList fileInfoList = dir.entryInfoList(filters, sortFlags); //获取当前路径下所有文件/文件夹信息，返回的fileInfoList是一个包含多个QFileInfo对象的列表，每个对象对应目录里一个文件/文件夹的完整信息（名称、类型、大小、路径等）

            int iFileCount=fileInfoList.size();            //文件个数
            PDU *respdu=mkPDU(sizeof(FileInfo)*iFileCount);
            respdu->uiMsgType=ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;

            FileInfo *pFileInfo=NULL;
            QString strFileName;
            QByteArray strFileNameUtf8;
            for ( int i=0;i<fileInfoList.size();i++)       //遍历目录下所有文件/文件夹
            {
                pFileInfo=(FileInfo *)(respdu->caMsg)+i;   //每个FileInfo结构体在pdu->caMsg填充的位置

                strFileName=fileInfoList[i].fileName();
                strFileNameUtf8=strFileName.toUtf8();
                memcpy(pFileInfo->caFileName,strFileNameUtf8.constData(),strFileNameUtf8.size());
                if (fileInfoList[i].isDir())
                {
                    pFileInfo->iFileType=0;                 //0代表文件夹
                }
                else if (fileInfoList[i].isFile())
                {
                    pFileInfo->iFileType=1;                 //1代表文件
                }
            }

            delete[] pCurPath;
            pCurPath=nullptr;                             //new出来的变量要释放掉，否则内存泄露

            write((char *)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_DEL_REQUEST:               //删除请求
        {
            char caDelNameUtf8[32]={'\0'};
            strncpy(caDelNameUtf8,pdu->caData,32);    //要删除的文件名
            QString caDelName=QString::fromUtf8(caDelNameUtf8);

            char *pCurPath=new char[pdu->uiMsgLen+1];
            memset(pCurPath,'\0',pdu->uiMsgLen+1);
            memcpy(pCurPath, pdu->caMsg, pdu->uiMsgLen);//取出当前路径
            QString strCurPath=QString::fromUtf8(pCurPath);

            QString strPath=QString("%1/%2").arg(strCurPath,caDelName);

            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_DEL_RESPOND;
            QFileInfo fileInfo(strPath);
            if (fileInfo.isDir())
            {
                QDir dir(strPath);                    //设置文件操作对象的操作路径
                dir.removeRecursively();              //底层实现是递归删除，包括子文件.子文件夹
                qDebug()<<"delete dir:"<<strPath;
                memcpy(respdu->caData,DEL_DIR_OK,strlen(DEL_DIR_OK));
            }
            else if (fileInfo.isFile())
            {
                QDir dir;
                dir.remove(strPath);
                qDebug()<<"delete file:"<<strPath;
                memcpy(respdu->caData,DEL_FILE_OK,strlen(DEL_FILE_OK));
            }

            delete[] pCurPath;
            pCurPath=nullptr;

            write((char *)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_RENAME_REQUEST:            //重命名请求
        {
            char caOldNameUtf8[32]={'\0'};
            char caNewNameUtf8[32]={'\0'};
            strncpy(caOldNameUtf8,pdu->caData,32);
            strncpy(caNewNameUtf8,pdu->caData+32,32);
            QString caOldName=QString::fromUtf8(caOldNameUtf8);
            QString caNewName=QString::fromUtf8(caNewNameUtf8);

            char *pCurPath=new char[pdu->uiMsgLen+1];
            memset(pCurPath,'\0',pdu->uiMsgLen+1);
            memcpy(pCurPath, pdu->caMsg, pdu->uiMsgLen);//取出当前路径
            QString strCurPath=QString::fromUtf8(pCurPath);

            QString strOldPath=QString("%1/%2").arg(strCurPath,caOldName);
            QString strNewPath=QString("%1/%2").arg(strCurPath,caNewName);

            QDir dir;
            bool ret=dir.rename(strOldPath,strNewPath);
            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_RENAME_RESPOND;
            if (ret)
            {
                memcpy(respdu->caData,RENAME_OK,strlen(RENAME_OK));         //在定义#define RENAME_OK "rename ok"时编译器就自动在RENAME_OK的存储内存中加上了\0，所以显式拷贝\0只需要strlen(RENAME_OK)+1即可实现
            }
            else
            {
                memcpy(respdu->caData,RENAME_FAILED,strlen(RENAME_FAILED));
            }

            delete[] pCurPath;
            pCurPath=nullptr;

            write((char *)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_ENTER_DIR_REQUEST:         //进入文件夹请求
        {
            char caEnterNameUtf8[32]={'\0'};
            strncpy(caEnterNameUtf8,pdu->caData,32);      //双击选中的文件/文件夹名
            QString caEnterName=QString::fromUtf8(caEnterNameUtf8);

            char *pCurPath=new char[pdu->uiMsgLen+1];
            memset(pCurPath,'\0',pdu->uiMsgLen+1);
            strncpy(pCurPath, pdu->caMsg, pdu->uiMsgLen);//取出当前路径
            QString strCurPath=QString::fromUtf8(pCurPath);

            QString strPath=QString("%1/%2").arg(strCurPath,caEnterName);//拼接新路径(即选中文件夹内部的路径)
            QFileInfo fileInfo(strPath);              //通过指定的路径strPath，创建一个QFileInfo类的实例fileInfo；这个实例会绑定到strPath对应的文件/文件夹，可以通过它读取该文件/文件夹的所有信息

            PDU *respdu=NULL;
            if (fileInfo.isDir())                     //文件夹可以进入
            {
                QDir dir(strPath);                    //拼接后的新路径，将要访问的路径
                QDir::Filters filters = QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot;//1.过滤规则：显示文件夹，显示普通文件，排除.(当前目录)和..(上级目录){操作系统在返回某个目录下的 “所有条目” 时，会默认把这两个标识符包含在内}
                QDir::SortFlags sortFlags = QDir::Name|QDir::DirsFirst;             //2.文件/文件夹排序规则：按名称排序，文件夹优先
                QFileInfoList fileInfoList = dir.entryInfoList(filters, sortFlags); //获取当前路径下所有文件/文件夹信息，返回的fileInfoList是一个包含多个QFileInfo对象的列表，每个对象对应目录里一个文件/文件夹的完整信息（名称、类型、大小、路径等）

                int iFileCount=fileInfoList.size();
                respdu=mkPDU(sizeof(FileInfo)*iFileCount);
                respdu->uiMsgType=ENUM_MSG_TYPE_ENTER_DIR_REQUEST_SUCCESS;
                FileInfo *pFileInfo=NULL;
                QString strFileName;
                QByteArray strFileNameUtf8;
                for ( int i=0;i<fileInfoList.size();i++)
                {
                    pFileInfo=(FileInfo *)(respdu->caMsg)+i;
                    strFileName=fileInfoList[i].fileName();
                    strFileNameUtf8=strFileName.toUtf8();

                    strncpy(pFileInfo->caFileName,strFileNameUtf8.constData(),strFileNameUtf8.size());
                    if (fileInfoList[i].isDir())
                    {
                        pFileInfo->iFileType=0;
                    }
                    else if (fileInfoList[i].isFile())
                    {
                        pFileInfo->iFileType=1;
                    }
                }

                delete[] pCurPath;
                pCurPath=nullptr;

                write((char *)respdu,respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            else if (fileInfo.isFile())                     //文件不可以进入
            {
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_ENTER_DIR_RESPOND;
                strncpy(respdu->caData,ENTER_DIR_FAILED,strlen(ENTER_DIR_FAILED));

                delete[] pCurPath;
                pCurPath=nullptr;

                write((char *)respdu,respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            break;
        }

        case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST:       //上传文件请求
        {
            char caFileNameUtf8[32]={'\0'};
            quint64 netFileSize;
            sscanf(pdu->caData,"%s %lld",caFileNameUtf8,&netFileSize);
            QString caFileName=QString::fromUtf8(caFileNameUtf8);
            qint64 fileSize=qFromBigEndian(netFileSize);

            char *pCurPath=new char[pdu->uiMsgLen+1];
            memset(pCurPath,'\0',pdu->uiMsgLen+1);
            memcpy(pCurPath, pdu->caMsg, pdu->uiMsgLen);//取出当前路径
            QString strCurPath=QString::fromUtf8(pCurPath);

            QString strPath=QString("%1/%2").arg(strCurPath,caFileName);
            qDebug()<<"uploadFile:"<<strPath;

            delete[] pCurPath;
            pCurPath=nullptr;

            m_file.setFileName(strPath);
            if (m_file.open(QIODevice::WriteOnly))      //以只写的方式打开文件，若文件不存在则自动创建(通常上传文件不在客户网盘，所以要自动创建同名文件）
            {
                m_bUpload=true;
                m_iTotal=fileSize;
                m_iRecved=0;
            }
            break;
        }

        case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST:     //下载文件请求
        {
            char caFileName[32]={'\0'};
            strncpy(caFileName,pdu->caData,32);           //要下载的文件名

            char *pPath=new char[pdu->uiMsgLen+1];
            memset(pPath,'\0',pdu->uiMsgLen+1);
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);   //要下载的文件所在文件夹路径

            QString strPath=QString("%1/%2").arg(pPath).arg(caFileName);
            qDebug()<<"downloadFile:"<<strPath;       //拼接得到要下载的文件路径

            delete[] pPath;
            pPath=nullptr;

            QFileInfo fileInfo(strPath);              //QFileInfo更适合用来获得文件大小
            qint64 fileSize=fileInfo.size();

            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND;
            sprintf(respdu->caData,"%s %lld",caFileName,fileSize);//要下载的文件名 文件大小

            write((char *)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;

            m_file.setFileName(strPath);//m_file绑定目标文件路径

            m_pTimer->start(1000);
            break;
        }

        case ENUM_MSG_TYPE_SHARE_FILE_REQUEST:        //共享文件请求
        {
            char caSendNameUtf8[32]={'\0'};
            int num=0;
            sscanf(pdu->caData,"%s %d",caSendNameUtf8,&num);//获取发出文件共享的用户名+要分享给好友的个数

            PDU *respdu=mkPDU(pdu->uiMsgLen-num*32);
            respdu->uiMsgType=ENUM_MSG_TYPE_SHARE_FILE_NOTE;
            strncpy(respdu->caData,caSendNameUtf8,31);      //把文件共享发起者名字 文件具体路径写入转发pdu中
            memcpy(respdu->caMsg,pdu->caMsg+num*32,pdu->uiMsgLen-num*32);

            char caRecvNameUtf8[32]={'\0'};
            QString caRecvName;
            for (int i=0;i<num;i++)                         //遍历所有选中好友一一转发
            {
                memcpy(caRecvNameUtf8,pdu->caMsg+i*32,32);
                caRecvName=QString::fromUtf8(caRecvNameUtf8);
                MyTcpServer::getInstance().resend(caRecvName,respdu);
            }
            free(respdu);
            respdu=NULL;


            respdu=mkPDU(0);                                //回复分享文件成功
            respdu->uiMsgType=ENUM_MSG_TYPE_SHARE_FILE_RESPOND;
            strncpy(respdu->caData,SHARE_FILE_OK,strlen(SHARE_FILE_OK));
            write((char *)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }

        case ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND:   //共享文件通知回复
        {
            QString strRecvPath="./"+QString::fromUtf8(pdu->caData);
            QString strShareFilePath=QString::fromUtf8(pdu->caMsg);

            int index=strShareFilePath.lastIndexOf('/');
            QString strFileName=strShareFilePath.right(strShareFilePath.size()-index-1);
            strRecvPath=strRecvPath+'/'+strFileName;

            QFileInfo fileInfo(strShareFilePath);
            if (fileInfo.isFile())
            {
                QFile::copy(strShareFilePath,strRecvPath);
            }
            else if (fileInfo.isDir())
            {
                copyDir(strShareFilePath,strRecvPath);
            }

            break;
        }

        case ENUM_MSG_TYPE_PASTE_REQUEST:             //粘贴文件请求
        {
            int srcPathLen=0;                         //源路径长度
            int destPathLen=0;                        //目的路径长度
            int CopyOrCut=0;                          //复制/剪切标志位
            char caFileNameUtf8[32]={'\0'};           //复制/剪切文件名
            sscanf(pdu->caData,"%d %d %d %s",&srcPathLen,&destPathLen,&CopyOrCut,caFileNameUtf8);
            QString caFileName=QString::fromUtf8(caFileNameUtf8);

            char* pSrcPathUtf8=new char[srcPathLen+1];
            char* pDestPathUtf8=new char[destPathLen+1];
            memset(pSrcPathUtf8,'\0',srcPathLen+1);
            memset(pDestPathUtf8,'\0',destPathLen+1);
            strncpy(pSrcPathUtf8,pdu->caMsg,srcPathLen+1);
            strncpy(pDestPathUtf8,pdu->caMsg+(srcPathLen+1),destPathLen+1);
            QString strSrcPath=QString::fromUtf8(pSrcPathUtf8);      //源路径
            QString strDestPath=QString::fromUtf8(pDestPathUtf8);    //目的路径

            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_PASTE_RESPOND;

            QFileInfo fileInfo(strDestPath);                         //绑定目的路径，目的路径为文件夹可粘贴，为文件返回错误
            if (fileInfo.isDir())                                    //该路径为文件夹，可执行复制/剪切
            {
                if (QFile::exists(strDestPath+'/'+caFileName))       //要复制/剪切的文件已存在
                {
                    strncpy(respdu->caData,FILE_EXISTED,strlen(FILE_EXISTED));
                }
                else                                                 //可执行复制/剪切操作
                {
                    bool ret=false;
                    if (CopyOrCut==1)                                    //复制
                    {
                        ret=QFile::copy(strSrcPath,strDestPath+'/'+caFileName);//与下方rename用法一致，区别在于操作后源文件是否依旧存在
                    }
                    else                                                 //剪切
                    {
                        ret=QFile::rename(strSrcPath,strDestPath+'/'+caFileName);//static bool QFile::rename(const QString &oldName, const QString &newName);oldName:源文件/文件夹的完整路径；newName:目标文件/文件夹的完整路径
                    }                                                            //例如把11.jpeg文件移动到"李明"文件夹里面，两者均在根目录下
                    //"./用户名/11.jpeg", "./用户名/李明/11.jpeg",两参数都必须是完整路径，这样写两个参数才是对的

                    if (ret)
                    {
                        strncpy(respdu->caData,PASTE_SUCCESS,strlen(PASTE_SUCCESS));
                    }
                    else
                    {
                        strncpy(respdu->caData,ERROR,strlen(ERROR));
                    }
                }
            }
            else if (fileInfo.isFile())                                   //该路径为文件，报错
            {
                strncpy(respdu->caData,PASTE_FAILED,strlen(PASTE_FAILED));
            }

            delete[] pSrcPathUtf8;
            delete[] pDestPathUtf8;
            pSrcPathUtf8=nullptr;
            pDestPathUtf8=nullptr;

            write((char *)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }


        default:
            break;
        }


        free(pdu);
        pdu=NULL;
    }
    else                                              //处理完上传文件请求后m_bUpload被修改为true从而进入else实现文件上传
    {
        while (this->bytesAvailable()>0)              //循环读取客户端的发送
        {
            char pBuffer[4096]={'\0'};
            qint64 remainLen=m_iTotal-m_iRecved;      //每接收到一次数据后还需读取的大小
            qint64 readSize=qMin(remainLen, (qint64)4096);
            qint64 recvLen=this->read(pBuffer, readSize);//实际读取的大小

            if (recvLen>0)
            {
                m_file.write(pBuffer,recvLen);
                m_iRecved+=recvLen;
            }
        }

        if (m_iTotal==m_iRecved)                      //读取成功:m_bUpload值置为false,关闭文件,给客户端发送提示
        {
            m_bUpload=false;
            qDebug()<<"UPLOAD_FILE_OK";
            m_file.close();

            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
            strncpy(respdu->caData,UPLOAD_FILE_OK,strlen(UPLOAD_FILE_OK));

            write((char *)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
        }
        else if (m_iTotal<m_iRecved)
        {
            m_bUpload=false;
            qDebug()<<"UPLOAD_FILE_FAILED";
            m_file.close();

            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
            strncpy(respdu->caData,UPLOAD_FILE_FAILED,strlen(UPLOAD_FILE_FAILED));

            write((char *)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
        }
    }
}


void MyTcpSocket::clientOffline()    //实现两个功能，修改数据库中用户online状态和删除与该用户相关的服务器端socket
{
    OpeDB::getInstance().handleOffline(m_strName);
    emit offline(this);
}

void MyTcpSocket::sendFileToClient() //定时器槽函数:每次给客户端发送4096字节直到读取完
{
    m_pTimer->stop();

    if (!m_file.open(QIODevice::ReadOnly))
    {
        qDebug()<<"下载失败";
        return;
    }

    char *pData=new char[4096];
    qint64 ret=0;
    while (true)
    {
        ret=m_file.read(pData,4096);
        if (ret>0&&ret<=4096)
        {
            write(pData,ret);
        }
        else if (ret==0)
        {
            break;
        }
        else if (ret<0)
        {
            qDebug()<<"发送文件给客户端失败";
            break;
        }
    }

    m_file.close();
    delete[] pData;
    pData=nullptr;
}
