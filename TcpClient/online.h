#ifndef ONLINE_H
#define ONLINE_H

//在线用户窗口
#include <QWidget>
#include <QPushButton>
#include "protocol.h"

namespace Ui {
class Online;
}

class Online : public QWidget
{
    Q_OBJECT
public:
    explicit Online(QWidget *parent = nullptr);
    ~Online();

    void showUsr(PDU *pdu);                      //显示出在线用户
    void setAddFriendBtn(bool status);           //更改加好友按钮可选状态

private slots:
    void on_addFriend_pb_clicked();              //加好友槽函数

private:
    Ui::Online *ui;
};

#endif // ONLINE_H
