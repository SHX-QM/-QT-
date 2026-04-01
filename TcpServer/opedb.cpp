#include "opedb.h"
#include <QMessageBox>

OpeDB::OpeDB(QObject *parent)
    : QObject{parent}
{
    m_db=QSqlDatabase::addDatabase("QSQLITE");
}

OpeDB &OpeDB::getInstance()
{
    static OpeDB instance;
    return instance;
}

void OpeDB::init()                                                             //数据库初始化函数:配置并连接指定路径的 SQLite 数据库（cloud.db）；
{
    m_db.setDatabaseName("D:\\qtcode\\TcpServer\\cloud.db");
    if (!m_db.open())
    {
        QMessageBox::critical(NULL,"打开数据库","打开数据库失败");
    }
}

OpeDB::~OpeDB()
{
    m_db.close();
}

bool OpeDB::handleRegist(const QString& name, const QString& pwd)              //数据库处理注册请求的逻辑:在数据库插入name和pwd
{
    if (name.isEmpty()||pwd.isEmpty())
    {
        return false;
    }

    QString data=QString("insert into usrInfo(name,pwd) values(\'%1\',\'%2\')").arg(name,pwd);
    QSqlQuery query;
    return query.exec(data);    //指令运行成功返回1否则0
}

int OpeDB::handleLogin(const QString& name,const QString& pwd)                 //数据库处理登录请求的逻辑:验证用户名是否存在，密码是否正确，online是否为0.登录成功后改online=1
{
    if (name.isEmpty()||pwd.isEmpty())
    {
        return -2;
    }

    QString data=QString("select * from usrInfo where name=\'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);
    if (query.next())              //该用户名存在
    {
        data=QString("select * from usrInfo where name=\'%1\' and pwd=\'%2\'").arg(name,pwd);
        query.exec(data);
        if (query.next())          //密码正确
        {
            data=QString("select * from usrInfo where name=\'%1\' and pwd=\'%2\' and online=0").arg(name,pwd);
            query.exec(data);
            if (query.next())      //用户名 密码正确且online=0，可登录
            {
                data=QString("update usrInfo set online=1 where name=\'%1\'").arg(name);
                query.exec(data);
                return 2;
            }
            else                   //online=1，relogin
            {
                return 1;
            }
        }
        else                       //密码错误
        {
            return 0;
        }
    }
    else                           //该用户名不存在，需提示注册
    {
        return -1;
    }
}

int OpeDB::handleCancel(const QString& name,const QString& pwd)               //数据库处理注销请求的逻辑:在数据库删除该用户名及信息
{
    if (name.isEmpty()||pwd.isEmpty())
    {
        return -1;
    }

    QString data=QString("select from usrInfo where name=\'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);
    if (query.next())              //用户存在可执行注销操作
    {
        data=QString("delete from usrInfo where name=\'%1\' and pwd=\'%2\'").arg(name,pwd);
        return query.exec(data);
    }
    else                           //用户不存在
    {
        return 2;
    }
}

void OpeDB::handleOffline(const QString& name)                                 //处理用户下线后的逻辑:用户下线后将数据库中的online设置为0(下线状态)
{
    if (name.isEmpty())
    {
        return;
    }

    QString data=QString("update usrInfo set online=0 where name=\'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);
}

QStringList OpeDB::handleAllOnline(const QString& name)                        //处理显示在线用户的逻辑:将数据库中online=1的用户名保存到result中并返回
{
    QStringList result;
    result.clear();

    if (name.isEmpty())
    {
        return result;
    }

    QString data=QString("select name from usrInfo where online=1 and name!=\'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);

    while(query.next())
    {
        result.append(query.value(0).toString());
    }
    return result;
}

int OpeDB::handleSearchUsr(const QString& name)                                //查找用户处理逻辑:在数据库中查询目标用户的online值并返回
{
    if (name.isEmpty())
    {
        return -1;
    }

    QString data=QString("select online from usrInfo where name=\'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);
    if (query.next())
    {
        return query.value(0).toInt();
    }
    else
    {
        return -1;
    }
}

int OpeDB::handleAddFriend(const QString& pername, const QString& name)        //加好友处理逻辑
{
    if (pername.isEmpty()||name.isEmpty())
    {
        return -1;
    }

    QString data=QString("select * from friend where (id=(select id from usrInfo where name=\'%1\') and friendId =(select id from usrInfo where name=\'%2\')) "
                           "or (id =(select id from usrInfo where name=\'%3\') and friendId=(select id from usrInfo where name=\'%4\'));").arg(pername,name,name,pername);
    QSqlQuery query;
    query.exec(data);

    if (query.next())
    {
        return 0;                  //双方已是好友
    }
    else
    {
        data=QString("select online from usrInfo where name=\'%1\'").arg(pername);
        QSqlQuery query;
        query.exec(data);
        if (query.next())
        {
            int ret=query.value(0).toInt();
            if (ret==1)
            {
                return 1;          //非好友但在线
            }
            else if (ret==0)
            {
                return 2;          //非好友不在线
            }
        }
        else
        {
            return 3;              //该用户不存在（加好友被请求方）
        }
    }
}

void OpeDB::handleAddFriendAgree(const QString& pername, const QString& name)        //加好友请求被（被请求方）同意后在数据库添加两人好友关系
{
    if (pername.isEmpty()||name.isEmpty())
    {
        return;
    }

    QString pernameData=QString("select id from usrInfo where name=\'%1\'").arg(pername);
    QSqlQuery query;
    query.exec(pernameData);
    int pernameID=-1;
    if (query.next())
    {
        pernameID=query.value(0).toInt();
    }

    QString nameData=QString("select id from usrInfo where name=\'%1\'").arg(name);
    query.exec(nameData);
    int nameID=-1;
    if (query.next())
    {
        nameID=query.value(0).toInt();
    }

    QString data=QString("insert into friend(id,friendId) values(%1,%2)").arg(pernameID).arg(nameID);
    query.exec(data);
}

QStringList OpeDB::handleFlushFriend(const QString& name)                      //刷新好友处理逻辑:数据库中找到所有在线且是目前账户好友的用户名，打包返回出去
{
    QStringList strFriendList;
    strFriendList.clear();

    if (name.isEmpty())
    {
        return strFriendList;
    }

    QString data= QString("select name from usrInfo where online=1 and id IN (select id from friend where friendId=(select id from usrInfo where name=\'%1\')) ").arg(name);
    //用IN才能接收多组数据
    QSqlQuery query;
    query.exec(data);
    while (query.next())
    {
        if (!strFriendList.contains(query.value(0).toString()))
        {
            strFriendList.append(query.value(0).toString());
        }
    }

    data= QString("select name from usrInfo where online=1 and id IN (select friendId from friend where id=(select id from usrInfo where name=\'%1\')) ").arg(name);
    query.exec(data);
    while (query.next())
    {
        if (!strFriendList.contains(query.value(0).toString()))
        {
            strFriendList.append(query.value(0).toString());
        }

    }

    return strFriendList;
}

bool OpeDB::handleDelFriend(const QString& selfName, const QString& friendName)//删除好友处理逻辑:根据双方名字在数据库中删除好友关系
{
    if (selfName.isEmpty()||friendName.isEmpty())
    {
        return false;
    }

    QString data=QString("delete from friend where id=(select id from usrInfo where name=\'%1\') and friendId=(select id from usrInfo where name=\'%2\')").arg(selfName,friendName);
    QSqlQuery query;
    query.exec(data);

    data=QString("delete from friend where id=(select id from usrInfo where name=\'%1\') and friendId=(select id from usrInfo where name=\'%2\')").arg(friendName,selfName);
    query.exec(data);
    return true;
}
