#ifndef OPEWIDGET_H
#define OPEWIDGET_H

//主界面
#include <QWidget>
#include <QListWidget>
#include "book.h"
#include "friend.h"
#include <QStackedWidget>

class OpeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OpeWidget(QWidget *parent = nullptr);
    static OpeWidget &getInstance();             //单例模式
    Friend *getFriend();                         //获得m_pFriend
    Book *getBook();                             //获得m_pBook

signals:

private slots:
    void onItemClicked(QListWidgetItem *item);   //点击最左侧窗口切换框时触发的刷新操作

private:
    QListWidget *m_pListW;
    Friend *m_pFriend;
    Book *m_pBook;
    QStackedWidget *m_pSW;
};

#endif // OPEWIDGET_H
