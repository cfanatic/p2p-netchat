#ifndef NETCHAT_H
#define NETCHAT_H

#ifndef Q_OS_WIN
#include <QtMac>
#endif
#include <QWidget>
#include <QUdpSocket>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMessageBox>
#include <QTextDocument>
#include <QString>
#include <QTextStream>
#include <QtAlgorithms>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QScrollArea>
#include <QScrollBar>
#include <QGridLayout>
#include <QDateTime>
#include "server.h"
#include "options.h"
#include "botan/init.h"
#include "botan/auto_rng.h"
#include "botan/pbkdf.h"
#include "botan/aes.h"
#include "botan/pipe.h"
#include "botan/filters.h"
#include "botan/version.h"

namespace Ui {
class Netchat;
}

class Netchat : public QWidget
{
    Q_OBJECT

public:
    explicit Netchat(QWidget *parent = nullptr);
    ~Netchat();

private:
    Ui::Netchat *ui;
    Options *m_dialogOptions;
    QUdpSocket *m_socket;
    Server *m_server;
    QHostAddress m_serverAddr;
    quint16 m_serverPort;
    QHostAddress m_localAddr;
    quint16 m_localPort;
    QString m_user;
    Botan::PBKDF *m_pbkdf;
    Botan::SecureVector<quint8> m_salt;
    QByteArray m_saltData;
    QMap<QString, Botan::SymmetricKey> m_aes256_keys;
    QSystemTrayIcon *m_systemTray;
    QGridLayout *m_layoutMessageLog;
    QScrollArea *m_scrollMessageLog;
    QScrollBar *m_scrollMessageBar;
    QTimer *m_textTimer;
    static const QString m_appName;

public slots:

private slots:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    void bindAddress();
    void sendMessage(QString message = "");
    void getMessage();
    void showOptions();
    void setNotification(QString user, QString text);
    void resetNotification();
    void connectHandler();
    void disconnectHandler();
    void setTyping(QString text);
    void resetTyping();
    void moveScrollBarToBottom(int min, int max);
    QString getOsName();

signals:
    void execMessage(QString user, QString text);
    void execConnect();
    void execDisconnect();

};

#endif // NETCHAT_H
