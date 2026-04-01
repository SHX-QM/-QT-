#include "tcpserver.h"
#include "opedb.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    OpeDB::getInstance().init();    //初始化数据库单例：保证整个服务器程序中只有一个数据库连接实例

    TcpServer w;
    w.show();
    return a.exec();
}
