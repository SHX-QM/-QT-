#include "privatechat.h"
#include "ui_privatechat.h"
#include "tcpclient.h"
#include <QMessageBox>

PrivateChat::PrivateChat(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PrivateChat)
{
    ui->setupUi(this);
}

PrivateChat::~PrivateChat()
{
    delete ui;
}

void PrivateChat::setChatName(QString strName)   //修改私有变量m_strChatName
{
    m_strChatName=strName;
}

void PrivateChat::updateMsg(const PDU *pdu)      //更新文本显示框
{
    if (pdu==NULL)
    {
        return;
    }
    char caSendNameUtf8[32]={'\0'};
    memcpy(caSendNameUtf8,pdu->caData,32);
    QString caSendName=QString::fromUtf8(caSendNameUtf8);

    QString strMsg=QString("%1 says:%2").arg(caSendName).arg((char *)(pdu->caMsg));
    ui->showMsg_te->append(strMsg);
}

void PrivateChat::on_sendMsg_pb_clicked()        //发送槽函数，转到槽得到的，不显式connect
{
    QString strMsg=ui->inputMsg_le->text();      //获取输入内容
    ui->inputMsg_le->clear();                    //清除输入框输入
    if (!strMsg.isEmpty())
    {
        m_strLoginName=TcpClient::getInstance().loginName();
        QString myMsg=QString("%1 says:%2").arg(m_strLoginName).arg(strMsg);
        ui->showMsg_te->append(myMsg);

        QByteArray m_strLoginNameUtf8=m_strLoginName.toUtf8();
        QByteArray m_strChatNameUtf8=m_strChatName.toUtf8();
        QByteArray strMsgUtf8=strMsg.toUtf8();

        PDU *pdu=mkPDU(strMsgUtf8.size()+1);
        pdu->uiMsgType=ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST;
        strncpy(pdu->caData,m_strLoginNameUtf8.constData(),31);
        strncpy(pdu->caData+32,m_strChatNameUtf8.constData(),31);
        strncpy(pdu->caMsg,strMsgUtf8.constData(),strMsgUtf8.size());

        TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;
    }
    else
    {
        QMessageBox::warning(this,"私聊","发送的聊天信息不能为空");
    }
}
