#include "tcpserver.h"
#include "./ui_tcpserver.h"
#include "mytcpserver.h"
#include <QMessageBox>
#include <QByteArray>
#include <QDebug>
#include <QHostAddress>
#include <QFile>


TcpServer::TcpServer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TcpServer)
{
    ui->setupUi(this);

    loadConfig();
    //此处应注意传入ip地址和端口
    MyTcpServer::getInstance().listen(QHostAddress(m_strIP),m_usPort);

}

TcpServer::~TcpServer()
{
    delete ui;
}

void TcpServer::loadConfig()
{
    QFile file(":/server.config");
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray baData = file.readAll();
        QString strData = baData.toStdString().c_str();
        file.close();
        strData.replace("\n"," ");

        QStringList strList=strData.split(" ");

        m_strIP = strList.at(0);
        m_usPort=strList.at(1).toUShort();
        qDebug()<< "ip:"<<m_strIP<<"port:"<<m_usPort;
    }
    else {
        QMessageBox::critical(this,"open config","open config failed");

    }
}

