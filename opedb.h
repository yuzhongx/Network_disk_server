#ifndef OPEDB_H
#define OPEDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QVector>


class OpeDB : public QObject
{
    Q_OBJECT
public:
    explicit OpeDB(QObject *parent = nullptr);
    static OpeDB& getInstance();
    void init();
    ~OpeDB();

    bool handleRegist(const char *name, const char *pwd);
    bool handleLogin(const char *name, const char *pwd);

    void handleOffline(const char *name);
    QStringList handleAllOnlne();
    int handleSearchUsr(const char *name);
    int handleAddFriend(const char *perName, const char *name);
    void handleAddFriendAgree(const char *perName,const char *name);

    bool handleDelFriend(const char *selfName,const char *friendName);
    QStringList handleFlushFriend(const char *name);
signals:
public slots :
private:
    QSqlDatabase m_db;//连接数据库



};

#endif // OPEDB_H
