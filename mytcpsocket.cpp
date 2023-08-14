#include "mytcpsocket.h"
#include "qstring.h"
#include "mytcpserver.h"


MyTcpSocket::MyTcpSocket(QObject *parent)
        : QTcpSocket{parent} {
    connect(this, SIGNAL(readyRead()),
            this, SLOT(recvMsg()));
    connect(this, SIGNAL(disconnected()),
            this, SLOT(clientOffline()));
    m_bUpLoad = false;
    m_pTimer = new QTimer;
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(sendFileToClient()));
}

QString MyTcpSocket::getName() {
    return m_strName;

}

void MyTcpSocket::recvMsg() {
    if (!m_bUpLoad) {
        qDebug() << this->bytesAvailable();
        uint uiPDULen = 0;


        this->read((char *) &uiPDULen, sizeof(uint));

        uint uiMsgLen = uiPDULen - sizeof(PDU);
        PDU *pdu = mkPDU(uiMsgLen);
        this->read((char *) pdu + sizeof(uint), uiPDULen - sizeof(uint));
        switch (pdu->uiMsgType) {
            case ENUM_MSG_TYPE_REGIST_REQUEST: {
                char caName[32] = {'\0'};
                char caPwd[32] = {'\0'};
                strncpy(caName, pdu->caData, 32);
                strncpy(caPwd, pdu->caData + 32, 32);
                bool ret = OpeDB::getInstance().handleRegist(caName, caPwd);
                PDU *respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_REGIST_RESPOND;

                if (ret) {
                    strcpy(respdu->caData, REGIST_OK);
                    QDir dir;
                    qDebug() << "create dir:" << dir.mkdir(QString("./%1").arg(caName));//当前目录下创建文件夹

                } else {
                    strcpy(respdu->caData, REGIST_FAILED);
                }
                write((char *) respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;

                break;
            }
            case ENUM_MSG_TYPE_LOGIN_REQUEST: {
                char caName[32] = {'\0'};
                char caPwd[32] = {'\0'};
                strncpy(caName, pdu->caData, 32);
                strncpy(caPwd, pdu->caData + 32, 32);
                bool ret = OpeDB::getInstance().handleLogin(caName, caPwd);
                PDU *respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND;
                if (ret) {
                    strcpy(respdu->caData, LOGIN_OK);
                    m_strName = caName;//记录登陆的名字，以便注销退出
                } else {
                    strcpy(respdu->caData, LOGIN_FAILED);
                }
                write((char *) respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
                break;


            }
            case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST: {
                QStringList ret = OpeDB::getInstance().handleAllOnlne();

                uint uiMsgLen_1 = ret.size() * 32;
                PDU *respdu = mkPDU(uiMsgLen_1);
                respdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_RESPOND;
                for (int var = 0; var < ret.size(); ++var) {
                    memcpy((char *) (respdu->caMsg) + var * 32, ret.at(var).toStdString().c_str(),
                           ret.at(var).size());//需要转换为char*型，因为int类型指针进行地址偏移会每次乘以int的大小；
                }
                ret.clear();
                write((char *) respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;

                break;
            }
            case ENUM_MSG_TYPE_SREARCH_USR_REQUEST: {
                int ret = OpeDB::getInstance().handleSearchUsr(pdu->caData);
                PDU *respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_SREARCH_USR_RESPOND;
                if (ret == -1) {
                    strcpy(respdu->caData, SEARCH_USR_NO);
                } else if (ret == 1) {
                    strcpy(respdu->caData, SEARCH_USR_ONLINE);
                } else if (ret == 0) {
                    strcpy(respdu->caData, SEARCH_USR_OFFLINE);
                }
                write((char *) respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
                break;

            }
            case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST: {
                char caPerName[32] = {'\0'};
                char caName[32] = {'\0'};
                strncpy(caPerName, pdu->caData, 32);
                strncpy(caName, pdu->caData + 32, 32);

                int ret = OpeDB::getInstance().handleAddFriend(caPerName, caName);
//            qDebug() << caPerName;
//            qDebug() << caName;
//            qDebug() << ret;
                PDU *respdu = nullptr;
                if (ret == -1) {
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                    strcpy(respdu->caData, UNKNOW_ERROR);
                    write((char *) respdu, respdu->uiPDULen);
                    free(respdu);
                    respdu = nullptr;
                } else if (ret == 0) {
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                    strcpy(respdu->caData, EXISTED_FRIEND);
                    write((char *) respdu, respdu->uiPDULen);
                    free(respdu);
                    respdu = nullptr;
                } else if (ret == 1) {
                    MyTcpServer::getInstance().resend(caPerName, pdu);

                } else if (ret == 2) {
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                    strcpy(respdu->caData, ADD_FRIEND_OFFLINE);
                    write((char *) respdu, respdu->uiPDULen);
                    free(respdu);
                    respdu = nullptr;
                } else if (ret == 3) {
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                    strcpy(respdu->caData, ADD_FRIEND_NOEXIST);
                    write((char *) respdu, respdu->uiPDULen);
                    free(respdu);
                    respdu = nullptr;
                } else {
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REQUEST;
                    strcpy(pdu->caData, UNKNOW_ERROR);
                    write((char *) respdu, respdu->uiPDULen);
                    free(respdu);
                    respdu = nullptr;
                }
//            write((char *) respdu, respdu->uiPDULen);
//            free(respdu);
//            respdu = nullptr;
                break;
            }
            case ENUM_MSG_TYPE_ADD_FRIEND_AGREE: {
                char caPerName[32] = {'\0'};
                char caName[32] = {'\0'};
                strncpy(caPerName, pdu->caData, 32);
                strncpy(caName, pdu->caData + 32, 32);
                OpeDB::getInstance().handleAddFriendAgree(caPerName, caName);

                PDU *respdu = mkPDU(0);
                QString data = QString("添加好友\'%1\'成功").arg(caPerName);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, data.toStdString().c_str());

                MyTcpServer::getInstance().resend(caPerName, respdu);
                free(respdu);
                respdu = nullptr;
                break;
            }
            case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE: {
                char caName[32] = {'\0'};
                char caPerName[32] = {'\0'};
                strncpy(caPerName, pdu->caData, 32);
                strncpy(caName, pdu->caData + 32, 32);

                PDU *respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                QString data = QString("\'%1\'拒绝添加你为好友").arg(caPerName);
                strcpy(respdu->caData, data.toStdString().c_str());

                MyTcpServer::getInstance().resend(caName, respdu);
                free(respdu);
                respdu = nullptr;
                break;
            }
            case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST: {
                char caName[32] = {'\0'};
                strncpy(caName, pdu->caData, 32);
                QStringList ret = OpeDB::getInstance().handleFlushFriend(caName);
                uint uiMegLen_2 = ret.size() * 32;
                PDU *respdu = mkPDU(uiMegLen_2);
                respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;
                for (int i = 0; i < ret.size(); ++i) {
                    memcpy((char *) (respdu->caMsg) + i * 32,
                           ret.at(i).toStdString().c_str(), ret.at(i).size());
                }
                write((char *) respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
                break;
            }
            case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST: {
                char caSelfName[32] = {'\0'};
                char caFriendName[32] = {'\0'};
                strncpy(caSelfName, pdu->caData, 32);
                strncpy(caFriendName, pdu->caData + 32, 32);
                OpeDB::getInstance().handleDelFriend(caSelfName, caFriendName);

                PDU *respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND;
                strcpy(respdu->caData, DElETE_FRIEND_OK);
                write((char *) respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;

                MyTcpServer::getInstance().resend(caFriendName, pdu);

                break;
            }
            case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST: {
                char caChatName[32] = {'\0'};
                memcpy(caChatName, pdu->caData + 32, 32);
                MyTcpServer::getInstance().resend(caChatName, pdu);
//            qDebug()<<sizeof(pdu)<<"private";
//            qDebug() << caChatName<<"1";
                break;
            }
            case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST: {
                char caName[32] = {'\0'};
                strncpy(caName, pdu->caData, 32);
                QStringList onlineFriend = OpeDB::getInstance().handleFlushFriend(caName);
                for (const auto &i: onlineFriend) {
                    MyTcpServer::getInstance().resend(i.toStdString().c_str(), pdu);
                }
                MyTcpServer::getInstance().resend(caName, pdu);
                break;
            }
            case ENUM_MSG_TYPE_CREATE_DIR_REQUEST: {
                QDir dir;
                PDU *respdu = nullptr;
                QString strCurPath = QString("%1").arg((char *) pdu->caMsg);
                bool ret = dir.exists(strCurPath);
                if (ret) {
                    char caNewDir[32] = {'\0'};
                    memcpy(caNewDir, pdu->caData + 32, 32);
                    QString strNewPath = strCurPath + "/" + caNewDir;
                    qDebug() << strNewPath << "create dir";
                    ret = dir.exists(strNewPath);
                    if (ret) {
                        respdu = mkPDU(0);
                        respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                        strcpy(respdu->caData, FILE_NAME_EXIST);

                    } else {
                        dir.mkdir(strNewPath);
                        respdu = mkPDU(0);
                        respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                        strcpy(respdu->caData, CREATE_DIR_OK);
                    }
                } else {
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                    strcpy(respdu->caData, DIR_NO_EXIST);

                }
                write((char *) respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
                break;
            }
            case ENUM_MSG_TYPE_FLUSH_FILE_REQUEST: {
                char *pCurPath = new char[pdu->uiMsgLen];
                memcpy(pCurPath, pdu->caMsg, pdu->uiMsgLen);
                qDebug() << pCurPath << "flush";
                QDir dir(pCurPath);
                QFileInfoList fileInfoList = dir.entryInfoList(QDir::Filter::AllEntries, QDir::SortFlag::DirsFirst);
                int iFileCount = fileInfoList.size();
                PDU *respdu = mkPDU(sizeof(FileInfo) * (iFileCount));
                respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
                FileInfo *pFileInfo = nullptr;
                QString strFileName;
                for (int i = 0; i < iFileCount; ++i) {
//                if (QString(".")==fileInfoList[i].fileName()||QString("..")==fileInfoList[i].fileName())
//                    continue;
                    pFileInfo = (FileInfo *) (respdu->caMsg) + i;
                    strFileName = fileInfoList[i].fileName();
                    memcpy(pFileInfo->caFileName, strFileName.toStdString().c_str(), strFileName.size());
                    if (fileInfoList[i].isDir())
                        pFileInfo->iFileType = 0;
                    else if (fileInfoList[i].isFile())
                        pFileInfo->iFileType = 1;

                }
                write((char *) respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
                break;
            }
            case ENUM_MSG_TYPE_DEL_DIR_REQUEST: {
                char caDirName[32] = {'\0'};
                strcpy(caDirName, pdu->caData);
                char *pPath = new char[pdu->uiMsgLen];
                memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
                QString strPath = QString("%1/%2").arg(pPath, caDirName);

                QFileInfo fileInfo(strPath);
                bool ret = false;
                if (fileInfo.isDir()) {
                    QDir dir;
                    dir.setPath(strPath);
                    dir.removeRecursively();//删除文件夹，并包含其内部的文件
                    ret = true;
                } else if (fileInfo.isFile()) {
                    ret = false;
                }
                PDU *respdu = nullptr;
                if (!ret) {
                    respdu = mkPDU(strlen(DEL_MSG_FAILURED) + 1);
                    respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                    memcpy(respdu->caData, DEL_MSG_FAILURED, strlen(DEL_MSG_FAILURED));
                } else {
                    respdu = mkPDU(strlen(DEL_MSG_OK) + 1);
                    respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                    memcpy(respdu->caData, DEL_MSG_OK, strlen(DEL_MSG_OK));
                }
                write((char *) respdu, respdu->uiPDULen);
                free(pdu);
                pdu = nullptr;
                break;
            }
            case ENUM_MSG_TYPE_RENAME_FILE_REQUEST: {
                char caOldName[32] = {'\0'};
                char caNewName[32] = {'\0'};
                strncpy(caOldName, pdu->caData, 32);
                strncpy(caNewName, pdu->caData + 32, 32);
                char *pPath = new char[pdu->uiMsgLen];
                memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
                QString strOldPath = QString("%1/%2").arg(pPath, caOldName);
                QString strNewPath = QString("%1/%2").arg(pPath, caNewName);

                QDir dir;
                bool ret = dir.rename(strOldPath, strNewPath);

                PDU *respdu = nullptr;
                if (ret) {
                    respdu = mkPDU(strlen(RENAME_MSG_OK) + 1);
                    respdu->uiMsgType = ENUM_MSG_TYPE_RENAME_FILE_RESPOND;
                    memcpy(respdu->caData, RENAME_MSG_OK, strlen(RENAME_MSG_OK));
                } else {
                    respdu = mkPDU(strlen(RENAME_MSG_FAILURED));
                    respdu->uiMsgType = ENUM_MSG_TYPE_RENAME_FILE_RESPOND;
                    memcpy(respdu->caData, RENAME_MSG_FAILURED, strlen(RENAME_MSG_FAILURED));
                }

                write((char *) respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
                break;
            }
            case ENUM_MSG_TYPE_ENTER_DIR_REQUEST: {
                char strEnterName[32] = {'\0'};
//            char strCurPath[32]= {'\0'};
                strncpy(strEnterName, pdu->caData, 32);
                char *pPath = new char[pdu->uiMsgLen];
                memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
                QString strPath = QString("%1/%2").arg(pPath, strEnterName);
                qDebug() << strPath << "enter";

                QFileInfo fileInfo(strPath);
                PDU *respdu = nullptr;
                if (fileInfo.isDir()) {
                    QDir dir(strPath);
                    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Filter::AllEntries, QDir::SortFlag::DirsFirst);
                    int iFileCount = fileInfoList.size();
                    respdu = mkPDU(sizeof(FileInfo) * iFileCount);
                    respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
//                memcpy(respdu->caData,ENTER_DIR_OK, strlen(ENTER_DIR_OK));
                    FileInfo *pFileInfo = nullptr;
                    QString strFileName;
                    for (int i = 0; i < iFileCount; ++i) {
//                if (QString(".")==fileInfoList[i].fileName()||QString("..")==fileInfoList[i].fileName())
//                    continue;
                        pFileInfo = (FileInfo *) (respdu->caMsg) + i;
                        strFileName = fileInfoList[i].fileName();
                        memcpy(pFileInfo->caFileName, strFileName.toStdString().c_str(), strFileName.size());
                        if (fileInfoList[i].isDir())
                            pFileInfo->iFileType = 0;
                        else if (fileInfoList[i].isFile())
                            pFileInfo->iFileType = 1;
                    }
                } else if (fileInfo.isFile()) {
                    respdu = mkPDU(strlen(ENTER_DIR_FAILURED) + 1);
                    respdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_RESPOND;
                    memcpy(respdu->caData, ENTER_DIR_FAILURED, strlen(ENTER_DIR_FAILURED));
                }
                write((char *) respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
                break;
            }
            case ENUM_MSG_TYPE_DELETE_FILE_REQUEST: {
                char caFileName[32] = {'\0'};
                strcpy(caFileName, pdu->caData);
                char *pPath = new char[pdu->uiMsgLen];
                memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
                QString strPath = QString("%1/%2").arg(pPath, caFileName);

                QFileInfo fileInfo(strPath);
                bool ret = false;
                if (fileInfo.isDir()) {
                    ret = false;
                } else if (fileInfo.isFile()) {
                    QDir dir;
                    dir.remove(strPath);//删除文件
                    ret = true;
                }
                PDU *respdu = nullptr;
                if (!ret) {
                    respdu = mkPDU(strlen(DELETE_FILE_MSG_FAILURED) + 1);
                    respdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FILE_RESPOND;
                    memcpy(respdu->caData, DELETE_FILE_MSG_FAILURED, strlen(DELETE_FILE_MSG_FAILURED));
                } else {
                    respdu = mkPDU(strlen(DELETE_FILE_MSG_OK) + 1);
                    respdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FILE_RESPOND;
                    memcpy(respdu->caData, DELETE_FILE_MSG_OK, strlen(DELETE_FILE_MSG_OK));
                }
                write((char *) respdu, respdu->uiPDULen);
                free(pdu);
                pdu = nullptr;
                break;
            }
            case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST: {
                char caFileName[32] = {'\0'};
                qint64 fileSize = 0;
                sscanf(pdu->caData, "%s %lld", caFileName, &fileSize);
                char *pPath = new char[pdu->uiMsgLen];
                memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
                QString strPath = QString("%1/%2").arg(pPath, caFileName);
                delete[]pPath;
                pPath = nullptr;

                m_file.setFileName(strPath);
                //以只写的方式打开文件，若文件不存在，则会自动创建文件
                if (m_file.open(QIODevice::WriteOnly)) {
                    m_bUpLoad = true;
                    m_iTotal = fileSize;
                    m_iReceved = 0;

                }
                break;
            }
            case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST: {
                char caFileName[32] = {'\0'};
                strcpy(caFileName, pdu->caData);
                char *pPath = new char[pdu->uiMsgLen];
                memcpy(pPath, pdu->caMsg, pdu->uiMsgLen);
                QString strPath = QString("%1/%2").arg(pPath, caFileName);
                qDebug() << "strPath : " << strPath;
                delete[]pPath;
                pPath = nullptr;

                QFileInfo fileInfo(strPath);
                qint64 fileSize = fileInfo.size();
                PDU *respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND;

                sprintf(respdu->caData, "%s %lld", caFileName, fileSize);
                write((char *) respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
                m_file.setFileName(strPath);
                m_file.open(QIODevice::ReadOnly);

                m_pTimer->start(500);
                break;
            }
            case ENUM_MSG_TYPE_SHARE_FILE_REQUEST: {
                char caSendName[32] = {'\0'};
                int receivePersonNum = 0;
                sscanf(pdu->caData, "%s %d", caSendName, &receivePersonNum);
                int receivePersonSize = receivePersonNum * 32;

                PDU *respdu = mkPDU(pdu->uiMsgLen - receivePersonSize);
                respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_NOTE;
                strcpy(respdu->caData, caSendName);
                memcpy(respdu->caMsg, (char *) (pdu->caMsg) + receivePersonSize, pdu->uiMsgLen - receivePersonSize);
                char caReceiveName[32] = {'\0'};

                for (int i = 0; i < receivePersonNum; ++i) {
                    memcpy(caReceiveName, (char *) (pdu->caMsg) + i * 32, 32);
                    MyTcpServer::getInstance().resend(caReceiveName, respdu);
                }
                free(respdu);
                respdu = nullptr;

                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_RESPOND;
                memcpy(respdu->caData, "share file ok", strlen("share file ok"));
                write((char *) respdu, respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
                break;
            }
            case ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND: {
//                char caReceivePath[32] = {'\0'};
                QString strReceivePath = QString("./%1").arg(pdu->caData);
                QString strShareFilePath = QString("%1").arg((char *) (pdu->caMsg));
                int index = strShareFilePath.lastIndexOf('/');
                QString strFileName = strShareFilePath.right(strShareFilePath.size() - index - 1);
                strReceivePath = strReceivePath + '/' + strFileName;
                QFileInfo fileInfo(strShareFilePath);
                if (fileInfo.isFile()) {
                    QFile::copy(strShareFilePath, strReceivePath);

                } else if (fileInfo.isDir()) {

                    copyDir(strShareFilePath, strReceivePath);
                }
                break;
            }
            case ENUM_MSG_TYPE_MOVE_FILE_REQUEST: {
                char caFileName[32] = {'\0'};
                int srcLen = 0;
                int destLen = 0;
                sscanf(pdu->caData, "%d %d %s", &srcLen, &destLen, caFileName);

                char *pSrcPath = new char[srcLen + 1];
                char *pDestPath = new char[destLen + 1 + 32];
                memset(pSrcPath, '\0', srcLen + 1);
                memset(pDestPath, '\0', destLen + 1 + 32);
                memcpy(pSrcPath, pdu->caMsg, srcLen);
                memcpy(pDestPath, (char *) (pdu->caMsg) + (srcLen + 1), destLen);

                PDU *respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_MOVE_FILE_RESPOND;

                QFileInfo fileInfo(pDestPath);
                if (fileInfo.isDir()) {
                    strcat(pDestPath, "/");
                    strcat(pDestPath, caFileName);

                    bool ret = QFile::rename(pSrcPath, pDestPath);
                    if (ret) {
                        strcpy(respdu->caData, MOVE_FILE_OK);
                    } else
                        strcpy(respdu->caData, COMMON_ERROR);
                } else if (fileInfo.isFile()) {
                    strcpy(respdu->caData,MOVE_FILE_FAILURED);
                }
                write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu = nullptr;
                break;
            }
            default:
                break;
        }
        free(pdu);
        pdu = nullptr;
    } else {
        qDebug() << "上传文件------------>1";
        PDU *respdu = mkPDU(0);
        respdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
        QByteArray buff = readAll();
        m_file.write(buff);
        m_iReceved += buff.size();
        qDebug() << "上传文件------------>2" << m_iReceved;
        if (m_iTotal == m_iReceved) {
            m_file.close();
            m_bUpLoad = false;
            strcpy(respdu->caData, UPLOAD_FILE_OK);
            write((char *) respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;
        } else if (m_iTotal < m_iReceved) {
            m_file.close();
            m_bUpLoad = false;
            strcpy(respdu->caData, UPLOAD_FILE_FAILURED);
            write((char *) respdu, respdu->uiPDULen);
            free(respdu);
            respdu = nullptr;
        }
        qDebug() << "上传文件------------>2" << m_iReceved;
    }
}

void MyTcpSocket::clientOffline() {
    OpeDB::getInstance().handleOffline(m_strName.toStdString().c_str());
    emit offline(this);

}

void MyTcpSocket::sendFileToClient() {
    m_pTimer->stop();
    char *pData = new char[4096];
    qint64 ret = 0;
    qDebug() << m_file;
    qDebug() << "下载文件------------>1";

    while (true) {
        ret = m_file.read(pData, 4096);
        qDebug() << "下载文件--------------->2" << ret;
        if (ret > 0 && ret <= 4096) {
            write(pData, ret);
        } else if (ret == 0) {
            m_file.close();
            break;
        } else if (ret < 0) {
            qDebug() << "发送文件内容给客户端失败";
            m_file.close();
            break;
        }
    }
    delete[]pData;
    pData = nullptr;
}

void MyTcpSocket::copyDir(QString strSrcDir, QString strDestDir) {
    QDir dir;
    dir.mkdir(strDestDir);
    dir.setPath(strSrcDir);
//    dir.entryInfoList();
    QFileInfoList fileInfoList = dir.entryInfoList();
    QString srcTmp;
    QString destTmp;
    for (const auto &i: fileInfoList) {
        if (i.isDir()) {
            if (i.fileName() == ".." || i.fileName() == ".")
                continue;
            srcTmp = strSrcDir + '/' + i.fileName();
            destTmp = strDestDir + '/' + i.fileName();
            copyDir(srcTmp, destTmp);
        } else if (i.isFile()) {
//            fileInfoList[i].fileName();
            srcTmp = strSrcDir + '/' + i.fileName();
            destTmp = strDestDir + '/' + i.fileName();
            QFile::copy(srcTmp, destTmp);
        }
    }
}
