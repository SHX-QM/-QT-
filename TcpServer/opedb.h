#ifndef OPEDB_H
#define OPEDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>

class OpeDB : public QObject
{
    Q_OBJECT
public:
    static OpeDB& getInstance();            //OpeDB 单例类的 “全局唯一访问入口”
                                            //static:声明这是静态成员函数:1. 属于「类本身」，而非类的某个实例；2. 无需创建 OpeDB 对象就可以通过 OpeDB::getInstance() 调用；3. 能访问类的静态成员
                                            //OpeDB&:函数返回值是「OpeDB 类的引用」：1. 引用是变量的 “别名”，返回引用意味着返回的是实例本身，而非副本；2. 避免返回值拷贝（防止创建新实例），也避免返回指针导致的误销毁风险。
    void init();
    ~OpeDB();

    bool handleRegist(const QString& name,const QString& pwd);                 //注册
    int handleLogin(const QString& name,const QString& pwd);                   //登录
    int handleCancel(const QString& name,const QString& pwd);                  //注销
    void handleOffline(const QString& name);                                   //用户下线
    QStringList handleAllOnline(const QString& name);                          //查找在线用户
    int handleSearchUsr(const QString& name);                                  //查找用户
    int handleAddFriend(const QString& pername,const QString& name);           //加好友
    void handleAddFriendAgree(const QString& pername,const QString& name);     //A请求加B好友，B同意后修改数据库添加A B好友关系
    QStringList handleFlushFriend(const QString& name);                        //刷新好友
    bool handleDelFriend(const QString& selfName, const QString& friendName);  //删除好友

signals:

public slots:

private:
    explicit OpeDB(QObject *parent = nullptr);
    QSqlDatabase m_db;
};    //单例模式:构造函数私有化，禁用拷贝构造，禁用赋值，提供一个静态获取实例的方法，内部只创建一次实例


#endif // OPEDB_H
