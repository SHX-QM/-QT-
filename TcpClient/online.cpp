#include "online.h"
#include "ui_online.h"
#include <QDebug>
#include "tcpclient.h"

Online::Online(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Online)
{
    ui->setupUi(this);
}

Online::~Online()
{
    delete ui;
}

void Online::showUsr(PDU *pdu)              //服务器查询到所有在线用户后把所有在线用户的用户名发送过来，客户端将其显示在在线用户列表窗口
{
    ui->online_lw->clear();

    if (pdu==NULL)
    {
        return;
    }

    uint uiSize=pdu->uiMsgLen/32;
    char caTmp[32]={'\0'};
    QString caTmpUtf8;
    for (uint i=0;i<uiSize;i++)
    {
        memcpy(caTmp,(char *)(pdu->caMsg)+i*32,32);
        caTmpUtf8=QString::fromUtf8(caTmp);
        ui->online_lw->addItem(caTmpUtf8);
    }
}

void Online::setAddFriendBtn(bool status)   //更改加好友按钮可选状态
{
    ui->addFriend_pb->setEnabled(status);
}

void Online::on_addFriend_pb_clicked()      //从ui设计界面转到槽过来的，未显式写出connect
{
    QListWidgetItem *pItem=ui->online_lw->currentItem();          //指向选中的用户名
    if (pItem==NULL)
    {
        return;
    }

    QString strPerUsrName=pItem->text();                          //加好友被请求方，我想加的人
    QString strLoginName=TcpClient::getInstance().loginName();    //加好友请求方，我自己
    QByteArray strPerUsrNameUtf8=strPerUsrName.toUtf8();
    QByteArray strLoginNameUtf8=strLoginName.toUtf8();

    PDU *pdu=mkPDU(0);
    pdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_REQUEST;
    strncpy(pdu->caData,strPerUsrNameUtf8.constData(),31);
    strncpy(pdu->caData+32,strLoginNameUtf8.constData(),31);

    TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
    free(pdu);
    pdu=NULL;

    ui->addFriend_pb->setEnabled(false);
}
