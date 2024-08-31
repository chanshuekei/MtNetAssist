#include "MtMainWin.h"
#include "ui_mtmainwin.h"
#include <QApplication>
#include <QMutexLocker>
#include <QHostAddress>
#include <QDateTime>
#include <QDebug>
#include "easylogging++.h"
#pragma execution_character_set("utf-8")

// 显示日志
#define LOG_MAX_COUNT 1024
#define SLOGAN X("Thinking Over The Moon And Typing Under The Moon.")

MtMainWin::MtMainWin(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MtMainWin)
{
    ui->setupUi(this);
    auto title = X("网络调试助手v%1,By@%2")
                     .arg(PROGRAM_VER)
                     .arg(AUTHOR);
    qDebug() << SLOGAN;
    setWindowTitle(title);
    // 设置显示日志最大行数
    ui->UdpLog->setMaximumBlockCount(LOG_MAX_COUNT);
    ui->TcpServerLog->setMaximumBlockCount(LOG_MAX_COUNT);
    ui->TcpClientLog->setMaximumBlockCount(LOG_MAX_COUNT);
    ui->UdpLog->setReadOnly(true);
    ui->TcpServerLog->setReadOnly(true);
    ui->TcpClientLog->setReadOnly(true);
    // 初始化
    m_udp = new QUdpSocket();
    m_tcpSrv = new QTcpServer();
    m_tcpCli = new QTcpSocket();
    // 绑定信号与槽
    connect(m_udp, &QUdpSocket::readyRead, this, &MtMainWin::onHaveUdpPendingData);
    connect(m_tcpCli, &QTcpSocket::readyRead, this, &MtMainWin::onHaveTcpClientPendingData);
    connect(m_tcpSrv, &QTcpServer::newConnection, this, &MtMainWin::onHaveTcpServerPendingData);
    connect(m_tcpCli, &QTcpSocket::stateChanged, this, &MtMainWin::onTcpClientStateChanged);
    // 显示信息绑定
    connect(this, &MtMainWin::appendUdpLog, this, &MtMainWin::onAppendUdpLog);
    connect(this, &MtMainWin::appendTcpSrvLog, this, &MtMainWin::onAppendTcpSrvLog);
    connect(this, &MtMainWin::appendTcpCliLog, this, &MtMainWin::onAppendTcpCliLog);
}

MtMainWin::~MtMainWin()
{
    delete ui;
}

void MtMainWin::onHaveUdpPendingData()
{
    QMutexLocker loker(&m_mutex);
    // 获取
    QByteArray data;
    data.resize(m_udp->pendingDatagramSize());
    QHostAddress remoteAddr;
    quint16 remotePort;
    auto ok = m_udp->readDatagram(data.data(), data.size(), &remoteAddr, &remotePort);
    if(ok <= 0)
    {
        emit appendUdpLog(X("读取数据异常!"));
        return;
    }
    else
    {
        ui->leUdpRemoteIP->setText(remoteAddr.toString());
        ui->leUdpRemotePort->setText(QString::number(remotePort));
        if(DataType::Hex == getUdpDataType())
        {
            // HEX
            emit appendUdpLog(X("接收到数据:"));
            emit appendUdpLog(data.toHex().toUpper());
        }
        else
        {
            // ASCII
            emit appendUdpLog(X("接收到数据:"));
            emit appendUdpLog(data);
        }
    }
}

void MtMainWin::onHaveTcpClientPendingData()
{
    QMutexLocker loker(&m_mutex);
    emit appendTcpCliLog(X("[服务端]:"));
    auto data = m_tcpCli->readAll();
    auto dataType = getTcpClientDataType();
    if(DataType::Hex == dataType)
    {
        auto fromHex = QByteArray::fromHex(data);
        emit appendTcpCliLog(fromHex);
    }
    else
    {
        emit appendTcpCliLog(data);
    }
}

void MtMainWin::onHaveTcpServerPendingData()
{
    QMutexLocker loker(&m_mutex);
    while(m_tcpSrv->hasPendingConnections())
    {
        auto newConnector = m_tcpSrv->nextPendingConnection();
        auto log = X("收到新连接:[%1:%2]")
                       .arg(newConnector->peerAddress().toString())
                       .arg(newConnector->peerPort());
        emit appendTcpSrvLog(log);
        // 绑定处理信息
        connect(newConnector, &QTcpSocket::readyRead, this, [this, newConnector]()
                {
                    // emit this->appendTcpSrvLog("QTcpSocket::readyRead");
                    emit this->appendTcpSrvLog(X("客户端[%1:%2]:").arg(newConnector->peerAddress().toString()).arg(newConnector->peerPort()));
                    auto data = newConnector->readAll();
                    if(data.isEmpty())
                    {
                        emit this->appendTcpSrvLog(X("数据为空!"));
                        return;
                    }
                    else
                    {
                        auto dataType = this->getTcpServerDataType();
                        if(DataType::Hex == dataType)
                        {
                            auto fromHex = QByteArray::fromHex(data);
                            emit this->appendTcpSrvLog(fromHex);
                        }
                        else
                        {
                            emit this->appendTcpSrvLog(data);
                        }
                    }
                    // 回复收到数据:当前时间
                    auto rst = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz").toLocal8Bit();
                    newConnector->write(rst);
                    newConnector->flush();
                });
        // 保存最新的客户端
        m_tcpConnList.append(newConnector);
    }
}

void MtMainWin::onAppendUdpLog(const QString &log)
{
    if(log.isEmpty())
    {
        return;
    }
    LOG(INFO) << "[UDP]:" << log;
    ui->UdpLog->appendPlainText(log);
    ui->UdpLog->moveCursor(QTextCursor::End);
}

void MtMainWin::onAppendTcpSrvLog(const QString &log)
{
    if(log.isEmpty())
    {
        return;
    }
    LOG(INFO) << "[TCP SERVRT]:" << log;
    ui->TcpServerLog->appendPlainText(log);
    ui->TcpServerLog->moveCursor(QTextCursor::End);
}

void MtMainWin::onAppendTcpCliLog(const QString &log)
{
    if(log.isEmpty())
    {
        return;
    }
    LOG(INFO) << "[TCP CLIENT]:" << log;
    ui->TcpClientLog->appendPlainText(log);
    ui->TcpClientLog->moveCursor(QTextCursor::End);
}

void MtMainWin::onTcpClientStateChanged(QAbstractSocket::SocketState state)
{
    switch(state)
    {
    case QAbstractSocket::ConnectingState:
        // 连接中
        emit appendTcpCliLog(X("正在连接TCP服务器..."));
        break;
    case QAbstractSocket::ConnectedState:
        // 连接成功
        emit appendTcpCliLog(X("连接TCP服务器成功!"));
        ui->btnTcpClientConn->setText(X("断开"));
        break;
    case QAbstractSocket::ClosingState:
        // 断开中
        emit appendTcpCliLog(X("正在断开TCP服务器连接..."));
        break;
    case QAbstractSocket::UnconnectedState:
        // 断开成功
        emit appendTcpCliLog(X("已断开TCP服务器连接!"));
        ui->btnTcpClientConn->setText(X("连接"));
        break;
    default:
        break;
    }
}

MtMainWin::DataType MtMainWin::getUdpDataType() const
{
    auto type = ui->cbUdpData->currentText();
    if(type.compare("HEX", Qt::CaseInsensitive) == 0)
    {
        return DataType::Hex;
    }
    else
    {
        return DataType::Ascii;
    }
}

MtMainWin::DataType MtMainWin::getTcpClientDataType() const
{
    auto type = ui->cbTcpClient->currentText();
    if(type.compare("HEX", Qt::CaseInsensitive) == 0)
    {
        return DataType::Hex;
    }
    else
    {
        return DataType::Ascii;
    }
}

MtMainWin::DataType MtMainWin::getTcpServerDataType() const
{
    auto type = ui->cbTcpServer->currentText();
    if(type.compare("HEX", Qt::CaseInsensitive) == 0)
    {
        return DataType::Hex;
    }
    else
    {
        return DataType::Ascii;
    }
}

void MtMainWin::on_btnTcpClientConn_clicked()
{
    if(m_tcpCli->state() == QTcpSocket::ConnectedState)
    {
        // 断开连接
        m_tcpCli->disconnectFromHost();
        emit appendTcpCliLog(X("断开连接"));
    }
    else
    {
        // 连接
        auto ip = ui->leTcpClientIp->text().trimmed();
        auto port = ui->leTcpClientPort->text().toInt();
        m_tcpCli->connectToHost(ip, port);
        // 等待连接
        if(m_tcpCli->waitForConnected(2000))
        {
            // 连接成功
            emit appendTcpCliLog(X("连接成功!"));
        }
        else
        {
            //连接失败
            emit appendTcpCliLog(X("连接失败!"));
        }
    }
}

void MtMainWin::on_btnTcpClientSend_clicked()
{
    QMutexLocker loker(&m_mutex);
    auto dataType = getTcpClientDataType();
    auto data = ui->leTcpClientData->text().trimmed();
    if(data.isEmpty())
    {
        emit appendTcpCliLog(X("发送数据不能为空!"));
        return;
    }
    QByteArray ba;
    if(dataType == DataType::Ascii)
    {
        ba = data.toLocal8Bit();
    }
    else
    {
        ba =  QByteArray::fromHex(data.toLocal8Bit());
    }
    // 发送数据
    if(m_tcpCli->state() == QTcpSocket::ConnectedState)
    {
        auto log = X("发送数据:") + ba;
        emit appendTcpCliLog(log);
        auto ok = (m_tcpCli->write(ba) > 0 && m_tcpCli->flush());
        log = (ok ? X("发送成功!") : X("发送失败!"));
        emit appendTcpCliLog(log);
    }
    else
    {
        emit appendTcpCliLog(X("TCP客户端网络状态异常!"));
    }
}


void MtMainWin::on_btnTcpServerListen_clicked()
{
    if(m_tcpSrv->isListening())
    {
        // 断开连接
        m_tcpSrv->close();
        emit appendTcpSrvLog(X("关闭TCP服务!"));
        // 删除连接的客户端
        while(!m_tcpConnList.isEmpty())
        {
            auto it = m_tcpConnList.takeFirst();
            it->close();
            it->deleteLater();
        }
        ui->btnTcpServerListen->setText(X("启动监听"));
    }
    else
    {
        auto port = ui->leTcpServerPort->text().toInt();
        // 监听端口
        if(m_tcpSrv->listen(QHostAddress::Any, port))
        {
            emit appendTcpSrvLog(X("启动TCP服务成功:%1").arg(port));
            ui->btnTcpServerListen->setText(X("停止监听"));
        }
        else
        {
            emit appendTcpSrvLog(X("启动TCP服务失败:%1").arg(m_tcpSrv->errorString()));
            ui->btnTcpServerListen->setText(X("启动监听"));
        }
    }
}


void MtMainWin::on_btnTcpServerSend_clicked()
{
    QMutexLocker loker(&m_mutex);
    auto dataType = getTcpServerDataType();
    auto data = ui->leTcpServerData->text().trimmed();
    if(data.isEmpty())
    {
        emit appendTcpSrvLog(X("发送数据不能为空!"));
        return;
    }
    QByteArray ba;
    if(dataType == DataType::Ascii)
    {
        ba = data.toLocal8Bit();
    }
    else
    {
        ba = QByteArray::fromHex(data.toLocal8Bit());
    }
    // 状态判断
    if(m_tcpSrv->isListening())
    {
        // 判断连接客户端个数
        if(m_tcpConnList.isEmpty()) {return;}
        // 发送数据
        for(auto it = m_tcpConnList.begin(); it != m_tcpConnList.end(); it++)
        {
            auto ok = it.i->t()->write(ba);
            it.i->t()->flush();
            auto addr = it.i->t()->localAddress();
            auto port = it.i->t()->localPort();
            auto log = X("%1:%2:发送%3字节").arg(addr.toString()).arg(port).arg(ok);
            emit appendTcpSrvLog(log);
        }
    }
    else
    {
        emit appendTcpSrvLog(X("TCP服务端网络状态异常!"));
    }
}


void MtMainWin::on_btnUdpUnbind_clicked()
{
    m_udp->abort();
    emit appendUdpLog(X("解绑UDP端口!"));
}


void MtMainWin::on_btnUdpBind_clicked()
{
    auto port = ui->leUdpPort->text().toInt();
    if(m_udp->bind(QHostAddress::Any, port))
    {
        emit appendUdpLog(X("绑定UDP端口:%1").arg(port));
    }
    else
    {
        emit appendUdpLog(X("绑定UDP端口失败!"));
    }
}


void MtMainWin::on_btnUdpSend_clicked()
{
    QMutexLocker loker(&m_mutex);
    auto dataType = getUdpDataType();
    auto data = ui->leUdpSendData->text().trimmed();
    auto ip = ui->leUdpRemoteIP->text().trimmed();
    auto port = ui->leUdpRemotePort->text().toInt();
    if(ip.isEmpty() || port <= 0)
    {
        emit appendUdpLog(X("远端UDP地址异常!"));
        return;
    }
    if(data.isEmpty())
    {
        emit appendUdpLog(X("发送数据不能为空!"));
        return;
    }
    QByteArray ba;
    if(dataType == DataType::Ascii)
    {
        ba = data.toLocal8Bit();
    }
    else
    {
        ba = QByteArray::fromHex(data.toLocal8Bit());
    }
    // 判断UDP状态
    if(m_udp->isValid())
    {
        auto ok = m_udp->writeDatagram(ba, QHostAddress(ip), port);
        emit appendUdpLog(X("发送IP:%1,端口:%2,字节大小:%3").arg(ip).arg(port).arg(ok));
        if(ok == -1)
        {
            // 发送失败
            auto log = X("发送失败:") + m_udp->errorString();
            emit appendUdpLog(log);
        }
        else
        {
            emit appendUdpLog(X("发送成功!"));
        }
    }
    else
    {
        emit appendUdpLog(X("UDP状态异常!"));
    }
}

void MtMainWin::on_btnTcpClientClrLog_clicked()
{
    ui->TcpClientLog->clear();
}

void MtMainWin::on_btnTcpServerClrLog_clicked()
{
    ui->TcpServerLog->clear();
}


void MtMainWin::on_btnUdpClrLog_clicked()
{
    ui->UdpLog->clear();
}

