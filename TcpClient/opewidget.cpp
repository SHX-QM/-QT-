#include "opewidget.h"

OpeWidget::OpeWidget(QWidget *parent)
    : QWidget{parent}
{
    m_pListW=new QListWidget(this);
    m_pListW->addItem("好友");
    m_pListW->addItem("图书");           //创建一个列表窗口，第一行选项为好友，第二行选项为图书

    m_pFriend=new Friend;
    m_pBook=new Book;

    m_pSW=new QStackedWidget;
    m_pSW->addWidget(m_pFriend);
    m_pSW->addWidget(m_pBook);         //栈窗口会给每个添加的界面分配唯一的索引（按添加顺序，第一个是 0，第二个是 1），这个索引和左侧 QListWidget 的行索引一一对应

    QHBoxLayout *pMain=new QHBoxLayout;
    pMain->addWidget(m_pListW);
    pMain->addWidget(m_pSW);           //列表窗口和栈窗口设置为水平布局

    setLayout(pMain);

    connect(m_pListW, &QListWidget::currentRowChanged, m_pSW, &QStackedWidget::setCurrentIndex);
         //实现 “选中行变 →窗口切换” 的自动联动：
         //选中左侧第0行（好友）→右侧显示 m_pFriend；
         //选中左侧第1行（图书）→右侧显示 m_pBook。
    connect(m_pListW, &QListWidget::itemClicked, this, &OpeWidget::onItemClicked);     //itemClicked(QListWidgetItem *item)当用户用鼠标点击QListWidget中的某一个列表项时，QListWidget会自动发出这个信号,参数指向 “被点击的那个列表项”
}

OpeWidget &OpeWidget::getInstance()    //保证始终只有一个OpeWidget对象
{
    static OpeWidget instance;
    return instance;
}

Friend* OpeWidget::getFriend()         //获取私有成员m_pFriend
{
    return m_pFriend;
}

Book* OpeWidget::getBook()             //获取私有成员m_pBook
{
    return m_pBook;
}

void OpeWidget::onItemClicked(QListWidgetItem *item)
{
    if (item==m_pListW->item(0))
    {
        m_pFriend->flushFriend();
    }
    else if (item==m_pListW->item(1))
    {
        m_pBook->flushFile();
    }
}
