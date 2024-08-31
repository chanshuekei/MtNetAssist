#ifndef MTMAINWIN_H
#define MTMAINWIN_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QMutex>

QT_BEGIN_NAMESPACE
namespace Ui {
class MtMainWin;
}
QT_END_NAMESPACE

class MtMainWin : public QMainWindow
{
    Q_OBJECT
    enum DataType
    {
        Ascii,
        Hex,
    };
public:
    MtMainWin(QWidget *parent = nullptr);
    ~MtMainWin();
signals:
    void appendUdpLog(QString);
    void appendTcpSrvLog(QString);
    void appendTcpCliLog(QString);
private slots:
    void onHaveUdpPendingData();        // 处理UDP信息数据
    void onHaveTcpClientPendingData();  // 处理TCP客户端信息数据
    void onHaveTcpServerPendingData();  // 处理TCP服务端信息数据
    // Tcp功能
    void on_btnTcpClientConn_clicked();
    void on_btnTcpClientSend_clicked();
    void on_btnTcpServerListen_clicked();
    void on_btnTcpServerSend_clicked();
    // Udp功能
    void on_btnUdpUnbind_clicked();
    void on_btnUdpBind_clicked();
    void on_btnUdpSend_clicked();
    // 清理日志
    void on_btnTcpClientClrLog_clicked();
    void on_btnTcpServerClrLog_clicked();
    void on_btnUdpClrLog_clicked();
protected slots:
    void onAppendUdpLog(const QString &log);
    void onAppendTcpSrvLog(const QString &log);
    void onAppendTcpCliLog(const QString &log);
    // TCP客户端状态处理
    void onTcpClientStateChanged(QAbstractSocket::SocketState state);
public:
    DataType getUdpDataType() const;
    DataType getTcpClientDataType() const;
    DataType getTcpServerDataType() const;
private:
    Ui::MtMainWin *ui;
    QMutex m_mutex;
    QTcpServer *m_tcpSrv;
    QTcpSocket *m_tcpCli;
    QUdpSocket *m_udp;
    QList<QTcpSocket *> m_tcpConnList;
};
#endif // MTMAINWIN_H
