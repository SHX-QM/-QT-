#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdlib.h>
#include <unistd.h>

typedef unsigned int uint;             //typedef是类型别名（只用于数据类型起别名）

#define REGIST_OK "regist ok"          //define是文本宏替换（只用于文本的替换，不能是数据类型）
#define REGIST_FAILED "regist failed:name existed"

#define LOGIN_OK "login ok"
#define NAME_NOT_REGIET "name not regist"
#define LOGIN_FAILED_PWD "login failed:pwd error"
#define LOGIN_FAILED_ONLINE "login failed:relogin"

#define CANCEL_OK "cancel ok"
#define CANCEL_FAILED "cancel failed"

#define SEARCH_USR_NO "no such people"
#define SEARCH_USR_ONLINE "online"
#define SEARCH_USR_OFFLINE "offline"

#define UNKNOW_ERROR "unknow error"
#define EXISTED_FRIEND "friend exist"
#define ADD_FRIEND_OFFLINE "usr offline"
#define ADD_FRIEND_NO_EXIST "usr not exist"
#define ADD_FRIEND_OK "add friend ok"

#define DEL_FRIEND_OK "delete friend ok"
#define DEL_FRIEND_FAILED "delete friend failed"

#define DIR_NO_EXIST "cur dir not exist"
#define FILE_NAME_EXIST "file name exist"
#define CREATE_DIR_OK "create dir ok"

#define DEL_DIR_OK "delete dir ok"
#define DEL_FILE_OK "delete file ok"

#define RENAME_OK "rename ok"
#define RENAME_FAILED "rename failed"

#define ENTER_DIR_FAILED "enter dir failed:is reguler file"

#define UPLOAD_FILE_OK "upload file ok"
#define UPLOAD_FILE_FAILED "upload file failed"

#define SHARE_FILE_OK "share file ok"

#define PASTE_SUCCESS "paste success"
#define PASTE_FAILED "paste failed"
#define FILE_EXISTED "file existed"
#define ERROR "not file"


enum ENUM_MSG_TYPE
{
    ENUM_MSG_TYPE_MIN=0,

    ENUM_MSG_TYPE_REGIST_REQUEST,                //注册请求
    ENUM_MSG_TYPE_REGIST_RESPOND,                //注册回复

    ENUM_MSG_TYPE_LOGIN_REQUEST,                 //登录请求
    ENUM_MSG_TYPE_LOGIN_RESPOND,                 //登录回复

    ENUM_MSG_TYPE_CANCEL_REQUEST,                //注销请求
    ENUM_MSG_TYPE_CANCEL_RESPOND,                //注销回复

    ENUM_MSG_TYPE_ALL_ONLINE_REQUEST,            //在线用户请求
    ENUM_MSG_TYPE_ALL_ONLINE_RESPOND,            //在线用户回复

    ENUM_MSG_TYPE_SEARCH_USR_REQUEST,            //搜索用户请求
    ENUM_MSG_TYPE_SEARCH_USR_RESPOND,            //搜索用户回复

    ENUM_MSG_TYPE_ADD_FRIEND_REQUEST,            //加好友请求
    ENUM_MSG_TYPE_ADD_FRIEND_RESPOND,            //加好友回复

    ENUM_MSG_TYPE_ADD_FRIEND_AGREE,              //同意加好友
    ENUM_MSG_TYPE_ADD_FRIEND_REFUSE,             //拒绝加好友

    ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST,          //刷新好友请求
    ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND,          //刷新好友回复

    ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST,         //删除好友请求
    ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND,         //删除好友回复

    ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST,          //私聊请求
    ENUM_MSG_TYPE_PRIVATE_CHAT_RESPOND,          //私聊回复

    ENUM_MSG_TYPE_GROUP_CHAT_REQUEST,            //群聊请求
    ENUM_MSG_TYPE_GROUP_CHAT_RESPOND,            //群聊回复

    ENUM_MSG_TYPE_CREATE_DIR_REQUEST,            //创建文件夹请求
    ENUM_MSG_TYPE_CREATE_DIR_RESPOND,            //创建文件夹回复

    ENUM_MSG_TYPE_FLUSH_FILE_REQUEST,            //刷新文件请求
    ENUM_MSG_TYPE_FLUSH_FILE_RESPOND,            //刷新文件回复

    ENUM_MSG_TYPE_DEL_REQUEST,                   //删除文件请求
    ENUM_MSG_TYPE_DEL_RESPOND,                   //删除文件回复

    ENUM_MSG_TYPE_RENAME_REQUEST,                //重命名请求
    ENUM_MSG_TYPE_RENAME_RESPOND,                //重命名回复

    ENUM_MSG_TYPE_ENTER_DIR_REQUEST,             //进入文件夹请求
    ENUM_MSG_TYPE_ENTER_DIR_REQUEST_SUCCESS,     //进入文件夹请求成功
    ENUM_MSG_TYPE_ENTER_DIR_RESPOND,             //进入文件夹回复

    ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST,           //上传文件请求
    ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND,           //上传文件回复

    ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST,         //下载文件请求
    ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND,         //下载文件回复

    ENUM_MSG_TYPE_SHARE_FILE_REQUEST,            //共享文件请求
    ENUM_MSG_TYPE_SHARE_FILE_RESPOND,            //共享文件回复
    ENUM_MSG_TYPE_SHARE_FILE_NOTE,               //共享文件通知
    ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND,       //共享文件通知回复

    ENUM_MSG_TYPE_PASTE_REQUEST,                 //粘贴请求
    ENUM_MSG_TYPE_PASTE_RESPOND,                 //粘贴回复

    ENUM_MSG_TYPE_MAX=0x00ffffff
};


struct FileInfo
{
    char caFileName[32];     //文件名字
    int iFileType;           //文件类型
};


struct PDU
{
    uint uiPDULen;           //总的协议数据单元大小
    uint uiMsgType;          //消息类型
    char caData[64];
    uint uiMsgLen;           //实际消息长度
    char caMsg[];            //实际消息
};

PDU *mkPDU(uint uiMsgLen);
#endif // PROTOCOL_H
