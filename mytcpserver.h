#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QTcpServer>
#include <QList>
#include "mytcpsocket.h"


class MyTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    MyTcpServer();

    static MyTcpServer &getInstance();//单例模式，静态成员函数，返回值为MyTcpServer的引用

    void incomingConnection(qintptr socketDescriptor);

    void resend(const char *perName, PDU *pdu);

public slots:
    void deleteSocket(MyTcpSocket *mysocket);

private:
    QList<MyTcpSocket*> m_tcpSocketList;


};

#endif // MYTCPSERVER_H
