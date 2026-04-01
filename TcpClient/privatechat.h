#ifndef PRIVATECHAT_H
#define PRIVATECHAT_H

//私聊窗口
#include <QWidget>
#include "protocol.h"

namespace Ui {
class PrivateChat;
}

class PrivateChat : public QWidget
{
    Q_OBJECT
public:
    explicit PrivateChat(QWidget *parent = nullptr);
    ~PrivateChat();

    void setChatName(QString strName);      //设置私聊对象的用户名
    void updateMsg(const PDU *pdu);         //更新私聊窗口的文本显示框

private slots:
    void on_sendMsg_pb_clicked();           //私聊窗口的发送按钮槽函数

private:
    Ui::PrivateChat *ui;
    QString m_strChatName;
    QString m_strLoginName;
};

#endif // PRIVATECHAT_H
