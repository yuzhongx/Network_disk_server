#include "opedb.h"
#include <QDebug>
#include <QMessageBox>

OpeDB::OpeDB(QObject *parent)
        : QObject{parent} {
    m_db = QSqlDatabase::addDatabase("QSQLITE");

}

OpeDB &OpeDB::getInstance() {
    static OpeDB instance;
    return instance;
}

void OpeDB::init() {
    m_db.setHostName("localhost");
    m_db.setDatabaseName("/home/oi/TcpServer/cloud.db");
    if (m_db.open()) {
        QSqlQuery query;
        query.exec("Select * from usrInfo");
        while (query.next()) {
            QString data = QString("%1,%2,%3").arg(query.value(0).toString(), query.value(1).toString(),
                                                   query.value(2).toString());
            qDebug() << data;
        }
    } else {
        QMessageBox::critical(nullptr, "打开数据库", "打开数据库失败");

    }
}

OpeDB::~OpeDB() {
    m_db.close();
}

bool OpeDB::handleRegist(const char *name, const char *pwd) {
//    char caQuery[128] = {'\0'};
//    sprintf(caQuery," ");
    if (name == nullptr || pwd == nullptr) {
        return false;
    }
    QString data = QString("insert into usrInfo(name,pwd) values (\'%1\',\'%2\')").arg(name,
                                                                                       pwd);//密码为字符串格式，value 的值要加上引号

    QSqlQuery query;
    return query.exec(data);
}

bool OpeDB::handleLogin(const char *name, const char *pwd) {
    if (name == nullptr || pwd == nullptr) {
        return false;
    }
    QString data = QString("select * from usrInfo where name=\'%1\' and pwd=\'%2\' and online=0").arg(name, pwd);
    QSqlQuery query;
    query.exec(data);
    if (query.next()) {
        data = QString("update usrInfo set online=1 where name=\'%1\' and pwd=\'%2\'").arg(name, pwd);
        QSqlQuery query_1;
        query_1.exec(data);
        return true;
    } else
        return false;

//    return query.next();

}

void OpeDB::handleOffline(const char *name) {
    if (name == nullptr)
        return;
    QString data = QString("update usrInfo set online=0 where name=\'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);

}

QStringList OpeDB::handleAllOnlne() {
    QString data = QString("select name from usrInfo where online=1");

    QSqlQuery query;
    query.exec(data);
    QStringList result;
    result.clear();
    while (query.next()) {
        result.append(query.value(0).toString());

    }
    return result;
}

int OpeDB::handleSearchUsr(const char *name) {
    if (name == nullptr)
        return -1;
//    qDebug() << name;
    QString data = QString("select online from usrInfo where name=\'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);
    if (query.next()) {
        int ret = query.value(0).toInt();
        if (ret == 1) {
            return 1;
        } else if (ret == 0) {
            return 0;
        }
    } else {
        return -1;
    }

}

int OpeDB::handleAddFriend(const char *perName, const char *name) {
    if (perName == nullptr || name == nullptr)
        return -1;
    QString data = QString(
            "select * from friend where (id =(select id from usrInfo where name=\'%1\') and friendId = (select id from usrInfo where name=\'%2\')) "
            "or (id =(select id from usrInfo where name=\'%3\') and friendId = (select id from usrInfo where name=\'%4\'))").arg(
            perName, name, name, perName);
//    qDebug() << data;
    QSqlQuery query;
    query.exec(data);
    if (query.next()) {
        return 0; //双方已经为好友
    } else {
        data = QString("select online from usrInfo where name=\'%1\'").arg(perName);
//        qDebug() << data;
        QSqlQuery query_1;
        query_1.exec(data);
        if (query_1.next()) {
            int ret = query_1.value(0).toInt();
            if (ret == 1) {
                return 1;//在线
            } else if (ret == 0) {
                return 2;//不在线
            }
        } else {
            return 3;//不存在
        }
    }
}

QStringList OpeDB::handleFlushFriend(const char *name) {
    QStringList strFriendList;
    QVector<int> v_friend;
    strFriendList.clear();
    if (name == nullptr)
        return strFriendList;
    QString data1 = QString(
            "select id from friend where friendId=(select id from usrInfo where name=\'%1\')").arg(
            name);//此种写法每次只会得到一个id值进行检索

//    qDebug() << data1;
    QSqlQuery query1;
    query1.exec(data1);
    while (query1.next()) {
        v_friend.append(query1.value(0).toInt());
    }
    for (int i: v_friend) {
        QString data1_1 = QString("select name from usrInfo where online=1 and id=\'%1\'").arg(i);
        query1.exec(data1_1);
        while (query1.next()) {
            strFriendList.append(query1.value(0).toString());
//            qDebug() << query1.value(0).toString();
        }
    }
    query1.clear();
    v_friend.clear();

    QString data2 = QString(
            "select friendId from friend where id=(select id from usrInfo where name=\'%1\')").arg(
            name);
//    qDebug() << data2;
    QSqlQuery query_2;
    query_2.exec(data2);
    while (query_2.next()) {
        v_friend.append(query_2.value(0).toInt());
    }
    for (int i: v_friend) {
        QString data2_1 = QString(
                "select name from usrInfo where online=1 and id=\'%1\'").arg(i);
        query_2.exec(data2_1);
        while (query_2.next()){
            strFriendList.append(query_2.value(0).toString());
//            qDebug() << query_2.value(0).toString();
        }
    }
    query_2.clear();
    v_friend.clear();
    return strFriendList;
}

bool OpeDB::handleDelFriend(const char *selfName, const char *friendName) {
    if (selfName== nullptr||friendName== nullptr)
        return false;
    QString data =QString("delete from friend where (id=(select id from usrInfo where name=\'%1\') and friendId=(select id from usrInfo where name=\'%2\'));").arg(selfName,friendName);
    QSqlQuery query;
    query.exec(data);
    query.clear();
    data=QString("delete from friend where (id=(select id from usrInfo where name =\'%1\') and friendId=(select id from usrInfo where name =\'%2\'));").arg(friendName,selfName);
    query.exec(data);
    return true;


}

void OpeDB::handleAddFriendAgree(const char *perName, const char *name) {
    if (perName== nullptr||name== nullptr)
        return;
    QString data=QString("insert into friend(id,friendId) values((select id from usrInfo where name=\'%1\'),"
                         "(select id from usrInfo where name = \'%2\'));").arg(perName,name);
    qDebug()<<data;
    QSqlQuery query;
    query.exec(data);

}



