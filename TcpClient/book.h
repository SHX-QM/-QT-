#ifndef BOOK_H
#define BOOK_H

//图书界面
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "protocol.h"
#include <QTimer>
#include "sharefile.h"

class Book : public QWidget
{
    Q_OBJECT
public:
    explicit Book(QWidget *parent = nullptr);
    void updateFileList(const PDU *pdu);    //更新文件列表
    void clearEnterDir();                   //清空m_strEnterDir
    QString getEnterDir();                  //获得m_strEnterDir
    void setDownloadStatus(bool status);    //设置下载状态
    bool getDownloadStatus();               //获得下载状态
    QString getSaveFilePath();              //获得文件保存路径
    QString getShareFileName();             //获得分享的文件名

    qint64 m_iTotal=0;                      //总的文件大小
    qint64 m_iRecved=0;                     //已收到多少

signals:

public slots:
    void flushFile();                       //刷新槽函数

public slots:
    //文件夹操作槽函数
    void createDir();                       //创建文件夹槽函数
    void del();                             //删除文件/文件夹槽函数
    void rename();                          //重命名槽函数
    void enterDir(const QModelIndex &index);//进入文件夹槽函数
    void returnPre();                       //返回槽函数

    //文件操作槽函数
    void uploadFile();                      //上传文件槽函数
    void downloadFile();                    //下载文件槽函数
    void shareFile();                       //分享文件槽函数
    void copy();                            //复制槽函数
    void cut();                             //剪切槽函数
    void paste();                           //粘贴槽函数

    void uploadFileData();                  //定时器槽函数

private:
    QListWidget *m_pBookListW;
    QPushButton *m_pReturnPB;
    QPushButton *m_pCreateDirPB;
    QPushButton *m_pDelPB;
    QPushButton *m_pRenamePB;
    QPushButton *m_pFlushFilePB;
    QPushButton *m_pUploadPB;
    QPushButton *m_pDownloadPB;
    QPushButton *m_pShareFilePB;
    QPushButton *m_pCopyPB;
    QPushButton *m_pCutPB;
    QPushButton *m_pPastePB;

    QString m_strEnterDir;
    QString m_strUploadFilePath;

    QTimer *m_pTimer;

    QString m_strSaveFilePath;
    bool m_bDownload;

    QString m_strShareFileName;

    QString m_strFileName;
    int CopyOrCut;
    QString m_strSrcPath;
    QString m_strDestdir;
};

#endif // BOOK_H
