#include "server.h"

Server::Server(QHostAddress serverAddr, quint16 serverPort, QObject *parent) :
    QObject(parent),
    m_socket(new QUdpSocket(parent)),
    m_serverAddr(serverAddr),
    m_serverPort(serverPort)
{
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(getDatagram()));
}

Server::~Server()
{
    delete m_socket;
}

void Server::startServer()
{
    // Close already established socket connection
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        stopServer();

    // Open a new socket connection
    m_socket->bind(m_serverAddr, m_serverPort, QAbstractSocket::DefaultForPlatform);
}

void Server::stopServer()
{
    // Close socket connection
    m_socket->abort();
}

void Server::getDatagram()
{
    QByteArray senderDatagram;
    QHostAddress senderAddress;
    quint16 senderPort;

    // Read incoming raw message from socket
    senderDatagram.resize(m_socket->pendingDatagramSize());
    m_socket->readDatagram(senderDatagram.data(), senderDatagram.size(), &senderAddress, &senderPort);

    // Check if this is a new client connection or an incoming message from one of two already known clients
    if ((senderDatagram[0] == '$') && (senderDatagram[1] == '$') && (m_connection.size() < 2))
    {
        // Store address and port of the new connection
        QList<QByteArray> tmp = senderDatagram.split('_');
        QString userName = tmp[1];
        quint16 userPort = tmp[2].toInt();
        QMap<QString, quint16> connection;
        connection.insert(senderAddress.toString(), userPort);
        m_connection.append(connection);
        qDebug() << "S::getDatagrams(): " << m_connection;
    }
    else
    {
        // New message received which will be forwarded to both clients
        sendDatagram(senderDatagram.simplified());
    }
}

void Server::sendDatagram(QByteArray senderDatagram)
{
    if (m_socket->state() == QAbstractSocket::BoundState)
    {
        // Send message to all clients that are connected to the server
        for(QVector<QMap<QString, quint16>>::iterator it = m_connection.begin(); it != m_connection.end(); ++it)
        {
            m_socket->writeDatagram(senderDatagram, QHostAddress(it->keys()[0]), it->values()[0]);
        }
    }
}
