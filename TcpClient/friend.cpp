#include "friend.h"
#include "tcpclient.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QDebug>

Friend::Friend(QWidget *parent)
    : QWidget{parent}
{
    m_pShowMsgTE=new QTextEdit;                  //大文本框:显示聊天/系统消息
    m_pFriendListWidget=new QListWidget;         //好友列表:显示好友列表
    m_pInputMsgLE=new QLineEdit;                 //信息输入框:输入要发送的消息
    m_pShowMsgTE->setReadOnly(true);             //大文本框只读

    m_pDelFriendPB=new QPushButton("删除好友");
    m_pFlushFriendPB=new QPushButton("刷新好友");
    m_pShowOnlineUsrPB=new QPushButton("显示在线用户");
    m_pSearchUsrPB=new QPushButton("查找用户");
    m_pMsgSendPB=new QPushButton("发送");
    m_pPrivateChatPB=new QPushButton("私聊");

    QVBoxLayout *pRightPBVBL=new QVBoxLayout;
    pRightPBVBL->addWidget(m_pDelFriendPB);
    pRightPBVBL->addWidget(m_pFlushFriendPB);
    pRightPBVBL->addWidget(m_pShowOnlineUsrPB);
    pRightPBVBL->addWidget(m_pSearchUsrPB);
    pRightPBVBL->addWidget(m_pPrivateChatPB);    //设置删除好友 刷新好友 显示在线用户 查找用户 私聊按钮为垂直布局

    QHBoxLayout *pTopHBL=new QHBoxLayout;
    pTopHBL->addWidget(m_pShowMsgTE);
    pTopHBL->addWidget(m_pFriendListWidget);
    pTopHBL->addLayout(pRightPBVBL);             //大文本框 好友列表 好友操作按钮形成的水平布局设置为水平布局

    QHBoxLayout *pMsgHBL=new QHBoxLayout;
    pMsgHBL->addWidget(m_pInputMsgLE);
    pMsgHBL->addWidget(m_pMsgSendPB);            //信息输入框 发送按钮设置为水平布局


    m_pOnline=new Online;
    m_pPrivateChat=new PrivateChat;

    QVBoxLayout *pMain=new QVBoxLayout;
    pMain->addLayout(pTopHBL);
    pMain->addLayout(pMsgHBL);                   //大文本框与好友列表与好友操作按钮形成的水平布局 和 信息输入框与发送按钮组成的水平布局设置为垂直布局

    pMain->addWidget(m_pOnline,300);             //设置在线用户列表界面大小，否则默认0不显示
    m_pOnline->hide();

    setLayout(pMain);

    connect(m_pShowOnlineUsrPB,&QPushButton::clicked,this,&Friend::showOnline); //显示在线用户按钮对应的处理逻辑
    connect(m_pSearchUsrPB,&QPushButton::clicked,this,&Friend::searchUsr);      //查找用户按钮对应的处理逻辑
    connect(m_pFlushFriendPB,&QPushButton::clicked,this,&Friend::flushFriend);  //刷新按钮对应的处理逻辑
    connect(m_pDelFriendPB,&QPushButton::clicked,this,&Friend::delFriend);      //删除好友对应的处理逻辑
    connect(m_pPrivateChatPB,&QPushButton::clicked,this,&Friend::privateChat);  //私聊对应的处理逻辑
    connect(m_pMsgSendPB,&QPushButton::clicked,this,&Friend::groupChat);        //群聊对应的处理逻辑
}

void Friend::showAllOnlineUsr(PDU *pdu)          //转到online.cpp进行处理
{
    if (pdu==NULL)
    {
        return;
    }
    m_pOnline->showUsr(pdu);
}

void Friend::updateFriendList(PDU *pdu)          //更新好友列表
{
    m_pFriendListWidget->clear();

    if (pdu==NULL)
    {
        return;
    }

    uint uiSize=pdu->uiMsgLen/32;
    char caName[32]={'\0'};
    QString caNameUtf8;
    for (uint i=0;i<uiSize;i++)
    {
        memcpy(caName,(char *)(pdu->caMsg)+i*32,32);
        caNameUtf8=QString::fromUtf8(caName);
        m_pFriendListWidget->addItem(caNameUtf8);
    }
}

void Friend::updateGroupMsg(PDU *pdu)            //更新群聊文本显示框
{ 
    QString strMsg=QString("%1 says:%2").arg(QString::fromUtf8(pdu->caData),QString::fromUtf8(pdu->caMsg));
    m_pShowMsgTE->append(strMsg);
}

PrivateChat* Friend::getPrivateChat()            //获取私有变量m_pPrivateChat
{
    return m_pPrivateChat;
}

QListWidget* Friend::getFriendList()             //获取私有变量m_pFriendListWidget
{
    return m_pFriendListWidget;
}

void Friend::setOnlinePB(bool status)            //修改加好友按钮可选状态
{
    m_pOnline->setAddFriendBtn(status);
}

void Friend::showOnline()                        //显示在线用户按钮对应的处理逻辑:显示在线用户列表窗口，把用户名和请求发送给服务器
{
    if (m_pOnline->isHidden())
    {
        m_pOnline->show();
        PDU *pdu=mkPDU(0);
        pdu->uiMsgType=ENUM_MSG_TYPE_ALL_ONLINE_REQUEST;
        QString strName=TcpClient::getInstance().loginName();
        QByteArray strNameUtf8=strName.toUtf8();
        strncpy(pdu->caData,strNameUtf8.constData(),31);

        TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;
    }
    else
    {
        m_pOnline->hide();
    }
}

void Friend::searchUsr()                         //查找用户按钮对应的处理逻辑:点击按钮后弹出对话框，输入要查找的用户名，然后将封装了该用户名的pdu发送到服务器
{
    m_strSearchName=QInputDialog::getText(this,"搜索","用户名：");
    if (!m_strSearchName.isEmpty())
    {
        PDU *pdu=mkPDU(0);
        QByteArray m_strSearchNameUtf8=m_strSearchName.toUtf8();
        strncpy(pdu->caData,m_strSearchNameUtf8.constData(),31);
        pdu->uiMsgType=ENUM_MSG_TYPE_SEARCH_USR_REQUEST;

        TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;
    }
}

void Friend::flushFriend()                       //刷新按钮对应的处理逻辑:将带有用户名的pdu发送到服务器
{
    QString strName=TcpClient::getInstance().loginName();
    PDU *pdu=mkPDU(0);
    pdu->uiMsgType=ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST;
    QByteArray strNameUtf8=strName.toUtf8();
    strncpy(pdu->caData,strNameUtf8.constData(),31);

    TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
    free(pdu);
    pdu=NULL;
}

void Friend::delFriend()                         //删除好友对应的处理逻辑:把含有好友名字和自己名字的pdu发送到服务器
{
    if (m_pFriendListWidget->currentItem()!=NULL)//不选中好友列表的一个好友则该指针为空
    {
        QString strSelfName=TcpClient::getInstance().loginName();            //自己名字
        QString strFriendName=m_pFriendListWidget->currentItem()->text();    //好友名字
        QByteArray strSelfNameUtf8=strSelfName.toUtf8();
        QByteArray strFriendNameUtf8=strFriendName.toUtf8();

        PDU *pdu=mkPDU(0);
        pdu->uiMsgType=ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST;
        strncpy(pdu->caData,strSelfNameUtf8.constData(),31);
        strncpy(pdu->caData+32,strFriendNameUtf8.constData(),31);

        TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;
    }
    else
    {
        return;
    }
}

void Friend::privateChat()                       //私聊对应的处理逻辑:选中一个好友后显示出privatechat对话框
{
    if (m_pFriendListWidget->currentItem()!=NULL)
    {
        QString strChatName=m_pFriendListWidget->currentItem()->text();
        m_pPrivateChat->setChatName(strChatName);
        if (m_pPrivateChat->isHidden())
        {
            m_pPrivateChat->show();
        }
    }
    else
    {
        QMessageBox::warning(this,"私聊","请选择私聊的对象");
    }
}

void Friend::groupChat()                         //群聊对应的处理逻辑
{
    QString strMsg=m_pInputMsgLE->text();        //获取输入内容
    m_pInputMsgLE->clear();                      //清除输入框输入
    if (!strMsg.isEmpty())
    {
        QString strLoginName=TcpClient::getInstance().loginName();
        QString myMsg=QString("%1 says:%2").arg(strLoginName,strMsg);
        m_pShowMsgTE->append(myMsg);

        QByteArray strLoginNameUtf8=strLoginName.toUtf8();
        QByteArray strMsgUtf8=strMsg.toUtf8();

        PDU *pdu=mkPDU(strMsgUtf8.size()+1);
        pdu->uiMsgType=ENUM_MSG_TYPE_GROUP_CHAT_REQUEST;
        strncpy(pdu->caData,strLoginNameUtf8.constData(),31);
        strncpy(pdu->caMsg,strMsgUtf8.constData(),strMsgUtf8.size());

        TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;
    }
    else
    {
        QMessageBox::warning(this,"群聊","发送信息不能为空");
    }
}
