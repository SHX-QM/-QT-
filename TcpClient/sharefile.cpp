#include "sharefile.h"
#include "tcpclient.h"
#include "opewidget.h"

ShareFile::ShareFile(QWidget *parent)
    : QWidget{parent}
{
    m_pSelectAllPB=new QPushButton("全选");
    m_pCancelSelectAllPB=new QPushButton("取消全选");
    m_pOKPB=new QPushButton("确定");
    m_pCancelPB=new QPushButton("取消");

    m_pSA=new QScrollArea;                       //滚动区域
    m_pFriendW=new QWidget;                      //好友列表
    m_pFriendWVBL=new QVBoxLayout(m_pFriendW);   //好友按照竖直排列
    m_pButtonGroup=new QButtonGroup(m_pFriendW); //选中按钮的集合，实现一起控制
    m_pButtonGroup->setExclusive(false);         //允许多选


    QHBoxLayout *pTopHBL=new QHBoxLayout;        //上方水平布局
    pTopHBL->addWidget(m_pSelectAllPB);
    pTopHBL->addWidget(m_pCancelSelectAllPB);
    pTopHBL->addStretch();                       //全选 取消全选 弹簧设置为水平布局

    QHBoxLayout *pDownHBL=new QHBoxLayout;       //下方水平布局
    pDownHBL->addWidget(m_pOKPB);
    pDownHBL->addWidget(m_pCancelPB);            //确定 取消设置为水平布局

    QVBoxLayout *pMainVBL=new QVBoxLayout;
    pMainVBL->addLayout(pTopHBL);                //上方水平布局
    pMainVBL->addWidget(m_pSA);                  //中间滚动区域
    pMainVBL->addLayout(pDownHBL);               //下方水平布局
    setLayout(pMainVBL);


    connect(m_pSelectAllPB,&QPushButton::clicked,this,&ShareFile::selectAll);                      //全选按钮+单击信号
    connect(m_pCancelSelectAllPB,&QPushButton::clicked,this,&ShareFile::cancelSelectAll);          //取消全选按钮+单击信号
    connect(m_pOKPB,&QPushButton::clicked,this,&ShareFile::okShare);                               //确定按钮+单击信号
    connect(m_pCancelPB,&QPushButton::clicked,this,&ShareFile::cancelShare);                       //取消按钮+单击信号
}

ShareFile &ShareFile::getInstance()              //单例模式
{
    static ShareFile instance;
    return instance;
}

void ShareFile::updateFriend(QListWidget *pFriendList)     //刷新可分享文件的好友(每次点击分享按钮弹出窗口中的好友列表部分实时更新，不会出现删除的好友还在可选选项中)，根据friend界面的好友列表显示内容填充分享界面的好友列表
{
    if (pFriendList==NULL)
    {
        return;
    }

    QList<QAbstractButton*> preFriendList=m_pButtonGroup->buttons();//QAbstractButton是按钮的基类，QList<QAbstractButton*>是储存所有QCheckBox对象指针的列表
    foreach (QAbstractButton *btn, preFriendList) {                 //先清空好友列表,foreach遍历的是副本，适合边遍历边删除的情况
        m_pFriendWVBL->removeWidget(btn);
        m_pButtonGroup->removeButton(btn);
        delete btn;
    }

    QCheckBox* pCB=NULL;                         //具体控件:选择框加文字
    for (int i=0;i<pFriendList->count();i++)     //读取所有好友逐个加入滚动区域
    {
        pCB=new QCheckBox(pFriendList->item(i)->text());
        m_pFriendWVBL->addWidget(pCB);
        m_pButtonGroup->addButton(pCB);
    }

    m_pSA->setWidget(m_pFriendW);
    m_pSA->setWidgetResizable(true);             //让滚动区自适应面板大小
}

void ShareFile::selectAll()                 //全选按钮槽函数
{
    QList<QAbstractButton*> cbList=m_pButtonGroup->buttons();
    for (int i=0;i<cbList.size();i++)
    {
        if (!cbList[i]->isChecked())
        {
            cbList[i]->setChecked(true);
        }
    }
}

void ShareFile::cancelSelectAll()           //取消全选按钮槽函数
{
    QList<QAbstractButton*> cbList=m_pButtonGroup->buttons();
    for (int i=0;i<cbList.size();i++)
    {
        if (cbList[i]->isChecked())
        {
            cbList[i]->setChecked(false);
        }
    }
}

void ShareFile::okShare()                   //确认按钮槽函数:pdu->caData里存放自己的用户名+选中好友个数，pdu->caMsg存放所有选中好友的名字+要分享文件的具体路径
{
    QString strName=TcpClient::getInstance().loginName();
    QString strCurPath=TcpClient::getInstance().curPath();
    QString strShareFileName=OpeWidget::getInstance().getBook()->getShareFileName();
    QString strPath=strCurPath+"/"+strShareFileName;

    QByteArray strNameUtf8=strName.toUtf8();
    QByteArray strPathUtf8=strPath.toUtf8();

    QList<QAbstractButton*> cbList=m_pButtonGroup->buttons();
    int num=0;
    for (int i=0;i<cbList.size();i++)
    {
        if (cbList[i]->isChecked())
        {
            num++;
        }
    }                                            //统计选中要分享文件的好友个数

    PDU *pdu=mkPDU(32*num+strPathUtf8.size()+1);
    pdu->uiMsgType=ENUM_MSG_TYPE_SHARE_FILE_REQUEST;
    sprintf(pdu->caData,"%s %d",strNameUtf8.constData(),num);
    int j=0;
    for (int i=0;i<cbList.size();i++)
    {
        if (cbList[i]->isChecked())
        {
            memcpy(pdu->caMsg+32*j,cbList[i]->text().toUtf8().constData(),cbList[i]->text().toUtf8().size());
            j++;
        }
    }

    memcpy(pdu->caMsg+32*num,strPathUtf8.constData(),strPathUtf8.size());

    TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
    free(pdu);
    pdu=NULL;
}

void ShareFile::cancelShare()               //取消按钮槽函数:隐藏窗口
{
    hide();
}
