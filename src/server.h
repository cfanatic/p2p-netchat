#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QUdpSocket>
#include <QVector>
#include <QMap>

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QHostAddress serverAddr, quint16 serverPort, QObject *parent = 0);
    ~Server();

public slots:
    void startServer();
    void stopServer();

private:
    QUdpSocket *m_socket;
    QHostAddress m_serverAddr;
    quint16 m_serverPort;
    QVector<QMap<QString, quint16>> m_connection;

private slots:
    void getDatagrams();
    void sendMessage(QByteArray senderDatagram);

signals:

};

#endif // SERVER_H
