#include "mytcpserver.h"
#include <QDebug>

MyTcpServer::MyTcpServer() {

}

MyTcpServer &MyTcpServer::getInstance() {
    //使用静态成员函数，获得静态局部对象，无论调用多少次，有且只有一个对象
    static MyTcpServer instance;
    return instance;
}

void MyTcpServer::incomingConnection(qintptr socketDescriptor) {
    qDebug() << "new client connected";
    MyTcpSocket *pTcpSocket = new MyTcpSocket;
    pTcpSocket->setSocketDescriptor(socketDescriptor);
    m_tcpSocketList.append(pTcpSocket);

    connect(pTcpSocket, SIGNAL(offline(MyTcpSocket*)), this, SLOT(deleteSocket(MyTcpSocket*)));

}

void MyTcpServer::resend(const char *perName, PDU *pdu) {
    if (perName == nullptr || pdu == nullptr)
        return;
    QString strName = perName;

    for (auto i : m_tcpSocketList) {
        if (strName == i->getName()) {
            i->write((char *) pdu, pdu->uiPDULen);
            break;
        }

    }
}

void MyTcpServer::deleteSocket(MyTcpSocket *mysocket) {
    QList<MyTcpSocket *>::iterator iter = m_tcpSocketList.begin();
    for (; iter != m_tcpSocketList.end(); ++iter) {
        if (mysocket == *iter) {
            (*iter)->deleteLater();//删除对象,不可直接使用delete，会引起服务端闪退；延迟释放
            *iter = NULL;//删除内容
            m_tcpSocketList.erase(iter);//删除列表中的指针
            break;
        }
    }
    for (int i = 0; i < m_tcpSocketList.size(); ++i) {
        qDebug() << m_tcpSocketList.at(i)->getName()<<"deleteSocket";
    }
}
