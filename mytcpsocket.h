#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QTcpSocket>
#include <QDir>
#include <QFileInfoList>
#include <QFile>
#include <QTimer>
#include "protocol.h"
#include "opedb.h"


class MyTcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit MyTcpSocket(QObject *parent = nullptr);
    QString getName();
    void copyDir(QString strSrcDir,QString strDestDir);


signals:
    void offline(MyTcpSocket *mysocket);

public slots:
    void recvMsg();
    void clientOffline();
    void sendFileToClient();

private:
    QString m_strName;
    QFile m_file;
    qint64 m_iTotal{};//文件总大小
    qint64 m_iReceved{};//文件接受的大小
    bool m_bUpLoad;//是处于上传文件中的状态，还是接受完的状态
    QTimer *m_pTimer;

};


#endif // MYTCPSOCKET_H
