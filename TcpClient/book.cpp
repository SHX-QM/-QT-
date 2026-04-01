#include "book.h"
#include "tcpclient.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QStyle>
#include <QApplication>
#include <QFileDialog>
#include "opewidget.h"
#include <QtEndian>

Book::Book(QWidget *parent)
    : QWidget{parent}
{
    m_strEnterDir.clear();

    m_bDownload=false;
    m_pTimer=new QTimer;

    m_pBookListW=new QListWidget;                //图书列表界面

    m_pReturnPB=new QPushButton("返回");
    m_pCreateDirPB=new QPushButton("创建文件夹");
    m_pDelPB=new QPushButton("删除");
    m_pRenamePB=new QPushButton("重命名");
    m_pFlushFilePB=new QPushButton("刷新");

    QVBoxLayout *pDirVBL=new QVBoxLayout;
    pDirVBL->addWidget(m_pReturnPB);
    pDirVBL->addWidget(m_pCreateDirPB);
    pDirVBL->addWidget(m_pDelPB);
    pDirVBL->addWidget(m_pRenamePB);
    pDirVBL->addWidget(m_pFlushFilePB);          //返回 创建文件夹 删除 重命名 刷新按钮绑定为垂直布局


    m_pUploadPB=new QPushButton("上传文件");
    m_pDownloadPB=new QPushButton("下载文件");
    m_pShareFilePB=new QPushButton("共享文件");
    m_pCopyPB=new QPushButton("复制");
    m_pCutPB=new QPushButton("剪切");
    m_pPastePB=new QPushButton("粘贴");
    m_pPastePB->setEnabled(false);

    QVBoxLayout *pFileVBL=new QVBoxLayout;
    pFileVBL->addWidget(m_pUploadPB);
    pFileVBL->addWidget(m_pDownloadPB);
    pFileVBL->addWidget(m_pShareFilePB);
    pFileVBL->addWidget(m_pCopyPB);
    pFileVBL->addWidget(m_pCutPB);
    pFileVBL->addWidget(m_pPastePB);             //上传文件 下载文件 共享文件 复制 剪切 粘贴绑定为垂直布局

    QHBoxLayout *pMain=new QHBoxLayout;
    pMain->addWidget(m_pBookListW);
    pMain->addLayout(pDirVBL);
    pMain->addLayout(pFileVBL);                  //图书列表 文件夹操作 文件操作设置为水平布局

    setLayout(pMain);                            //给Widget绑定布局管理器，让布局自动管理子控件的位置/大小；

    //文件夹操作
    connect(m_pCreateDirPB,&QPushButton::clicked,this,&Book::createDir);        //创建文件夹按钮+单击信号
    connect(m_pFlushFilePB,&QPushButton::clicked,this,&Book::flushFile);        //刷新按钮+单击信号
    connect(m_pDelPB,&QPushButton::clicked,this,&Book::del);                    //删除按钮+单击信号
    connect(m_pRenamePB,&QPushButton::clicked,this,&Book::rename);              //重命名按钮+单击信号
    connect(m_pBookListW,&QListWidget::doubleClicked,this,&Book::enterDir);     //图书列表+双击信号
    connect(m_pReturnPB,&QPushButton::clicked,this,&Book::returnPre);           //返回按钮+单击信号

    //文件操作
    connect(m_pUploadPB,&QPushButton::clicked,this,&Book::uploadFile);          //上传文件按钮+单击信号
    connect(m_pDownloadPB,&QPushButton::clicked,this,&Book::downloadFile);      //下载文件按钮+单击信号
    connect(m_pShareFilePB,&QPushButton::clicked,this,&Book::shareFile);        //共享文件按钮+单击信号
    connect(m_pCopyPB,&QPushButton::clicked,this,&Book::copy);                  //复制按钮+单击信号
    connect(m_pCutPB,&QPushButton::clicked,this,&Book::cut);                    //剪切按钮+单击信号
    connect(m_pPastePB,&QPushButton::clicked,this,&Book::paste);                //粘贴按钮+单击信号

    //定时器
    connect(m_pTimer,&QTimer::timeout,this,&Book::uploadFileData);              //绑定定时器信号+uploadFileData()
}

void Book::updateFileList(const PDU *pdu)    //更新文件列表:先清空再显示文件图标和名字
{
    if (pdu==NULL)
    {
        return;
    }

    m_pBookListW->clear();

    FileInfo *pFileInfo=NULL;
    int iCount=pdu->uiMsgLen/sizeof(FileInfo);

    QStyle *widgetStyle = m_pBookListW->style();
    QIcon folderIcon = widgetStyle->standardIcon(QStyle::SP_DirIcon);  // 文件夹图标
    QIcon fileIcon = widgetStyle->standardIcon(QStyle::SP_FileIcon);   // 常规文件图标


    for (int i=0;i<iCount;i++)
    {
        pFileInfo=(FileInfo*)(pdu->caMsg)+i;
        QListWidgetItem *pItem=new QListWidgetItem;

        if (pFileInfo->iFileType==0)
        {
            pItem->setIcon(folderIcon);
        }
        else if(pFileInfo->iFileType==1)
        {
            pItem->setIcon(fileIcon);
        }                                                               //按照文件类型设置图标

        pItem->setText(QString::fromUtf8(pFileInfo->caFileName));
        m_pBookListW->addItem(pItem);
    }
}

void Book::clearEnterDir()                   //清空m_strEnterDir
{
    m_strEnterDir.clear();
}

QString Book::getEnterDir()                  //双击进入文件夹时存储点击的文件夹名称，用于后续进入文件夹成功后更新strCurPath
{
    return m_strEnterDir;
}

void Book::setDownloadStatus(bool status)    //修改下载状态
{
    m_bDownload=status;
}

bool Book::getDownloadStatus()               //获取下载状态
{
    return m_bDownload;
}

QString Book::getSaveFilePath()              //获取下载文件时文件保存位置，m_strSaveFilePath在下载文件按钮槽函数中赋值
{
    return m_strSaveFilePath;
}

QString Book::getShareFileName()             //获取分享的文件名
{
    return m_strShareFileName;
}


void Book::createDir()                       //创建文件夹槽函数:首先新文件夹名字不能为空，名字不为空且名字长度<=32时将含有用户名字，新文件夹名字，当前文件路径的PDU发送过去
{
    QString strNewDir=QInputDialog::getText(this,"新建文件夹","新文件夹名字");     //弹出信息输入框，输入信息保存在strNewDir
    if (!strNewDir.isEmpty())                                                  //输入不能为空
    {
        if (strNewDir.size()>32)                                               //输入信息长度不能超过32
        {
            QMessageBox::warning(this,"新建文件夹","新文件夹名字不能超过32个字符");
        }
        else                                                                   //获取用户名和当前路径，用户名存入caData前32位/文件命名存入caData后32位/当前路径存入caMsg
        {
            QString strName=TcpClient::getInstance().loginName();
            QString strCurPath=TcpClient::getInstance().curPath();
            QByteArray strNameUtf8=strName.toUtf8();
            QByteArray strCurPathUtf8=strCurPath.toUtf8();
            QByteArray strNewDirUtf8=strNewDir.toUtf8();

            PDU *pdu=mkPDU(strCurPathUtf8.size()+1);                           //加一是为了存储字符串数组结束符'\0',这里没有显式赋值是因为在mkPDU时，memset(pdu,0,uiPDULen)把所有位置元素初始化为'\0'
            pdu->uiMsgType=ENUM_MSG_TYPE_CREATE_DIR_REQUEST;
            strncpy(pdu->caData,strNameUtf8.constData(),31);
            strncpy(pdu->caData+32,strNewDirUtf8.constData(),31);
            strncpy(pdu->caMsg,strCurPathUtf8.constData(),strCurPathUtf8.size());

            TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
            free(pdu);
            pdu=NULL;
        }
    }
    else
    {
        QMessageBox::warning(this,"新建文件夹","新文件夹名字不能为空");

    }
}

void Book::flushFile()                       //刷新槽函数:包含当前路径(存放于caMsg中)的pdu发送过去
{
    QString strCurPath=TcpClient::getInstance().curPath();
    QByteArray strCurPathUtf8=strCurPath.toUtf8();

    PDU *pdu=mkPDU(strCurPathUtf8.size()+1); //加一是为了存储字符串数组结束符'\0',这里没有显式赋值是因为在mkPDU时，memset(pdu,0,uiPDULen)把所有位置元素初始化为'\0'
    pdu->uiMsgType=ENUM_MSG_TYPE_FLUSH_FILE_REQUEST;
    strncpy(pdu->caMsg,strCurPathUtf8.constData(),strCurPathUtf8.size());

    TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
    free(pdu);
    pdu=NULL;
}

void Book::del()                             //删除槽函数:读取选中的文件名字，把要删除的文件名字和路径打包进pdu
{
    QString strCurPath=TcpClient::getInstance().curPath();
    QListWidgetItem *pItem=m_pBookListW->currentItem();                        //指向选中文件地址的指针
    if (pItem==NULL)
    {
        QMessageBox::warning(this,"删除文件","请选择要删除的文件");
    }
    else
    {
        QString strDelName=pItem->text();                                      //选中的文件名
        QByteArray strDelNameUtf8=strDelName.toUtf8();
        QByteArray strCurPathUtf8=strCurPath.toUtf8();

        PDU *pdu=mkPDU(strCurPathUtf8.size()+1);                               //加一是为了存储字符串数组结束符'\0',这里没有显式赋值是因为在mkPDU时，memset(pdu,0,uiPDULen)把所有位置元素初始化为'\0'
        pdu->uiMsgType=ENUM_MSG_TYPE_DEL_REQUEST;
        strncpy(pdu->caData,strDelNameUtf8.constData(),31);
        strncpy(pdu->caMsg,strCurPathUtf8.constData(),strCurPathUtf8.size());  //选中的文件名存入caData,路径存入caMsg

        TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;
    }

}

void Book::rename()                          //重命名槽函数:选中文件，把文件旧名字 新名字 路径打包发送过去
{
    QString strCurPath=TcpClient::getInstance().curPath();
    QListWidgetItem *pItem=m_pBookListW->currentItem();
    if (pItem!=NULL)
    {
        QString strOldName=pItem->text();
        QString strNewName=QInputDialog::getText(this,"重命名","请输入新的文件名");
        if (!strNewName.isEmpty())
        {
            if (strNewName.size()>32)
            {
                QMessageBox::warning(this,"重命名","新名字不能超过32个字符");
            }
            else
            {
                QByteArray strOldNameUtf8=strOldName.toUtf8();
                QByteArray strNewNameUtf8=strNewName.toUtf8();
                QByteArray strCurPathUtf8=strCurPath.toUtf8();

                PDU *pdu=mkPDU(strCurPathUtf8.size()+1);  //加一是为了存储字符串数组结束符'\0',这里没有显式赋值是因为在mkPDU时，memset(pdu,0,uiPDULen)把所有位置元素初始化为'\0'
                pdu->uiMsgType=ENUM_MSG_TYPE_RENAME_REQUEST;
                strncpy(pdu->caData,strOldNameUtf8.constData(),31);
                strncpy(pdu->caData+32,strNewNameUtf8.constData(),31);
                strncpy(pdu->caMsg,strCurPathUtf8.constData(),strCurPath.size());

                TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
                free(pdu);
                pdu=NULL;
            }
        }
        else
        {
            QMessageBox::warning(this,"重命名","新文件名字不能为空");
        }
    }
    else
    {
        QMessageBox::warning(this,"重命名","请选择要重命名的文件");
    }
}

void Book::enterDir(const QModelIndex &index)//进入文件夹槽函数:双击文件夹后把路径和文件名打包发送过去
{
    QString strEnterName=index.data().toString();            //获取双击的文件夹名称

    m_strEnterDir=strEnterName;

    QString strCurPath=TcpClient::getInstance().curPath();
    QByteArray strEnterNameUtf8=strEnterName.toUtf8();
    QByteArray strCurPathUtf8=strCurPath.toUtf8();

    PDU *pdu=mkPDU(strCurPathUtf8.size()+1);                   //加一是为了存储字符串数组结束符'\0',这里没有显式赋值是因为在mkPDU时，memset(pdu,0,uiPDULen)把所有位置元素初始化为'\0'
    pdu->uiMsgType=ENUM_MSG_TYPE_ENTER_DIR_REQUEST;
    strncpy(pdu->caData,strEnterNameUtf8.constData(),31);
    strncpy(pdu->caMsg,strCurPathUtf8.constData(),strCurPathUtf8.size());//选中的文件夹名存入caData,路径存入caMsg

    TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
    free(pdu);
    pdu=NULL;
}

void Book::returnPre()                       //返回按钮槽函数:处于根目录时不可返回，可返回时更新strCurPath并向服务器发送更新后strCurPath路径的刷新请求
{
    QString strCurPath=TcpClient::getInstance().curPath();             //获取当前路径
    QString strRootPath="./"+TcpClient::getInstance().loginName();     //根目录路径
    if (strCurPath==strRootPath)
    {
        QMessageBox::warning(this,"返回","返回失败:根目录不可返回");
    }
    else
    {
        int index=strCurPath.lastIndexOf('/');                         //找到最后一个'\'并截取之前的部分
        strCurPath.remove(index,strCurPath.size()-index);
        TcpClient::getInstance().setCurPath(strCurPath);               //更新strCurPath
        qDebug()<<"return dir："<<strCurPath;

        clearEnterDir();                                               //清空enterDir中可能的对m_strEnterDir的赋值

        flushFile();
    }
}





void Book::uploadFile()                      //上传文件按钮槽函数:获得选中文件的绝对路径后提取文件名 文件大小 文件存储路径(当前路径）
{
    m_strUploadFilePath=QFileDialog::getOpenFileName();           //选中文件后得到该文件的绝对路径

    if (!m_strUploadFilePath.isEmpty())
    {
        int index=m_strUploadFilePath.lastIndexOf('/');
        QString strFileName=m_strUploadFilePath.right(m_strUploadFilePath.size()-index-1);//从绝对路径中从右向左截取选中的文件的名称

        QFile file(m_strUploadFilePath);
        qint64 fileSize=file.size();

        QString strCurPath=TcpClient::getInstance().curPath();
        QByteArray strCurPathUtf8=strCurPath.toUtf8();
        QByteArray strFileNameUtf8=strFileName.toUtf8();
        quint64 netFileSize=qToBigEndian(fileSize);

        PDU *pdu=mkPDU(strCurPathUtf8.size()+1);
        pdu->uiMsgType=ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST;
        memcpy(pdu->caMsg,strCurPathUtf8.constData(),strCurPathUtf8.size());
        sprintf(pdu->caData,"%s %lld",strFileNameUtf8.constData(),netFileSize);

        TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;

        m_pTimer->start(1000);
    }
    else
    {
        QMessageBox::warning(this,"上传文件","上传文件名字不能为空");
    }
}

void Book::uploadFileData()                  //定时器槽函数:启动文件传输
{
    m_pTimer->stop();

    QFile file(m_strUploadFilePath);         //绑定本地文件绝对路径
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this,"上传文件","打开文件失败");
        return;
    }

    char *pBuffer=new char[4096];            //大小为4096时传输效率较高
    qint64 ret=0;
    while(true)
    {
        ret=file.read(pBuffer,4096);         //返回实际读取到的数据大小
        if (ret>0&&ret<=4096)
        {
            TcpClient::getInstance().getTcpSocket().write(pBuffer,ret);//从pBuffer里取ret个字节发送给服务器
        }                                                              //不能用write(pBuffer),否则会导致:文件里有\0时，数据直接被截断(这个形式的write重载读到\0才停止)
                                                                       //最后一次读不满 4096 时，发送垃圾数据   根本不知道文件什么时候发完
        else if (ret==0)
        {
            break;
        }
        else
        {
            QMessageBox::warning(this,"上传文件","上传文件失败:读取失败");
            break;
        }
    }

    file.close();
    delete[] pBuffer;
    pBuffer=nullptr;
}

void Book::downloadFile()                    //下载文件按钮槽函数:获取选中文件名，弹出窗口获取文件下载路径并保存，将当前路径和要下载的文件名发送到服务器
{
    QListWidgetItem *pItem=m_pBookListW->currentItem();             //指向选中文件地址的指针
    if (pItem==NULL)
    {
        QMessageBox::warning(this,"下载文件","请选择要下载的文件");
    }
    else
    {
        QString strSaveFilePath=QFileDialog::getSaveFileName();     //弹出获得文件保存位置的窗口
        if (strSaveFilePath.isEmpty())
        {
            QMessageBox::warning(this,"下载文件","请指定文件保存位置");
            m_strSaveFilePath.clear();
            return;
        }
        else
        {
            m_strSaveFilePath=strSaveFilePath;                      //把文件保存位置另存到私有变量m_strSaveFilePath
        }

        QString strCurPath=TcpClient::getInstance().curPath();      //当前所在路径
        PDU *pdu=mkPDU(strCurPath.size()+1);
        pdu->uiMsgType=ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST;
        QString strFileName=pItem->text();                          //要下载的文件名
        strncpy(pdu->caData,strFileName.toStdString().c_str(),strFileName.size());
        memcpy(pdu->caMsg,strCurPath.toStdString().c_str(),strCurPath.size());

        TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;
    }
}

void Book::shareFile()                       //共享文件槽函数:(支持文件和文件夹)获取选中文件名并保存，按照friend界面的好友列表更新可分享文件的好友，显示出分享窗口
{
    QListWidgetItem *pItem=m_pBookListW->currentItem();//指向选中文件地址的指针
    if (pItem==NULL)
    {
        QMessageBox::warning(this,"分享文件","请选择要分享的文件");
        return;
    }
    else
    {
        m_strShareFileName=pItem->text();
    }

    Friend *pFriend=OpeWidget::getInstance().getFriend();
    pFriend->flushFriend();
    QListWidget *pFriendList=pFriend->getFriendList();
    ShareFile::getInstance().updateFriend(pFriendList);

    if (ShareFile::getInstance().isHidden())
    {
        ShareFile::getInstance().show();
    }
}

void Book::copy()                            //复制槽函数:获得要复制文件的路径
{
    QListWidgetItem *pCurItem=m_pBookListW->currentItem();    //指向选中文件地址的指针
    if (pCurItem!=NULL)
    {
        m_strFileName=pCurItem->text();                       //要复制的文件名
        QString strCurPath=TcpClient::getInstance().curPath();//当前路径
        m_strSrcPath.clear();                                 //清空m_strSrcPath
        m_strSrcPath=strCurPath+'/'+m_strFileName;            //拼接出要复制文件的路径

        CopyOrCut=1;
        m_pCopyPB->setEnabled(false);
        m_pCutPB->setEnabled(false);
        m_pPastePB->setEnabled(true);
    }
    else
    {
        QMessageBox::warning(this,"复制","请选择文件");
    }
}

void Book::cut()                             //剪切槽函数:获得要剪切文件的路径
{
    QListWidgetItem *pCurItem=m_pBookListW->currentItem();    //指向选中文件地址的指针
    if (pCurItem!=NULL)
    {
        m_strFileName=pCurItem->text();                        //要剪切的文件名
        QString strCurPath=TcpClient::getInstance().curPath();//当前路径
        m_strSrcPath.clear();                                 //清空m_strSrcPath
        m_strSrcPath=strCurPath+'/'+m_strFileName;             //拼接出要剪切文件的路径

        CopyOrCut=0;
        m_pCopyPB->setEnabled(false);
        m_pCutPB->setEnabled(false);
        m_pPastePB->setEnabled(true);
    }
    else
    {
        QMessageBox::warning(this,"剪切","请选择文件");
    }
}

void Book::paste()                           //粘贴槽函数:转编码的(要剪切/复制的文件/文件夹)的路径长度+转编码的当前路径长度+要剪切/复制的文件名UTF8编码写入pdu->caData
{                                            //要剪切/复制的文件/文件夹的路径UTF8编码+当前路径UTF8编码写入pdu->caMsg
    QString strDestPath=TcpClient::getInstance().curPath();//当前路径
    QByteArray strDestPathUtf8=strDestPath.toUtf8();       //当前路径UTF8编码
    QByteArray strSrcPathUtf8=m_strSrcPath.toUtf8();       //要剪切/复制的文件/文件夹的路径UTF8编码
    QByteArray strSrcNameUtf8=m_strFileName.toUtf8();      //要剪切/复制的文件名UTF8编码
    int srcPathLen=strSrcPathUtf8.size();                  //(要剪切/复制的文件/文件夹)的路径长度
    int destPathLen=strDestPathUtf8.size();                //当前路径长度

    PDU* pdu=mkPDU(srcPathLen+1+destPathLen+1);
    pdu->uiMsgType=ENUM_MSG_TYPE_PASTE_REQUEST;
    sprintf(pdu->caData,"%d %d %d %s",srcPathLen,destPathLen,CopyOrCut,strSrcNameUtf8.constData());
    strncpy(pdu->caMsg,strSrcPathUtf8.constData(),srcPathLen);
    strncpy(pdu->caMsg+(srcPathLen+1),strDestPathUtf8.constData(),destPathLen);

    TcpClient::getInstance().getTcpSocket().write((char *)pdu,pdu->uiPDULen);
    free(pdu);
    pdu=NULL;

    m_pCopyPB->setEnabled(true);
    m_pCutPB->setEnabled(true);
    m_pPastePB->setEnabled(false);
}
