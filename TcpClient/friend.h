#ifndef FRIEND_H
#define FRIEND_H

//好友界面
#include <QWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "protocol.h"
#include "online.h"
#include "privatechat.h"

class Friend : public QWidget
{
    Q_OBJECT
public:
    explicit Friend(QWidget *parent = nullptr);
    void showAllOnlineUsr(PDU *pdu);        //转到online.cpp进行处理
    void updateFriendList(PDU *pdu);        //更新好友列表
    void updateGroupMsg(PDU *pdu);          //更新群聊文本显示框
    PrivateChat* getPrivateChat();          //获得m_pPrivateChat
    QListWidget* getFriendList();           //获得m_pFriendListWidget
    void setOnlinePB(bool status);          //修改加好友按钮可选状态

    QString m_strSearchName;

signals:

public slots:
    void showOnline();                      //显示在线用户槽函数
    void searchUsr();                       //查找用户槽函数
    void flushFriend();                     //刷新按钮槽函数
    void delFriend();                       //删除好友槽函数
    void privateChat();                     //私聊槽函数
    void groupChat();                       //群聊槽函数

private:
    QTextEdit *m_pShowMsgTE;
    QListWidget *m_pFriendListWidget;
    QLineEdit *m_pInputMsgLE;

    QPushButton *m_pDelFriendPB;
    QPushButton *m_pFlushFriendPB;
    QPushButton *m_pShowOnlineUsrPB;
    QPushButton *m_pSearchUsrPB;
    QPushButton *m_pMsgSendPB;
    QPushButton *m_pPrivateChatPB;

    Online* m_pOnline;
    PrivateChat* m_pPrivateChat;
};

#endif // FRIEND_H
