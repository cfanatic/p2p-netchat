#include "netchat.h"
#include "ui_netchat.h"

const QString Netchat::m_appName = "Netchat";

Netchat::Netchat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Netchat),
    m_socket(new QUdpSocket(this)),
    m_systemTray(new QSystemTrayIcon(this)),
    m_layoutMessageLog(new QGridLayout(this)),
    m_scrollMessageLog(new QScrollArea(this)),
    m_textTimer(new QTimer(this))
{
    // Initialize main window
    ui->setupUi(this);
    ui->textMessage->setAttribute(Qt::WA_MacShowFocusRect, 0);
    this->setFixedSize(m_width, m_height);
    this->setFocusPolicy(Qt::StrongFocus);

    // Initialize message log of the main window
    QWidget *widgetMessageLog = new QWidget();
    widgetMessageLog->setStyleSheet("background-color: white");
    widgetMessageLog->setLayout(m_layoutMessageLog);
    m_layoutMessageLog->setAlignment(Qt::AlignTop);
    m_scrollMessageLog->setWidgetResizable(true);
    m_scrollMessageLog->setGeometry(10, 10, 480, 177);
    m_scrollMessageLog->setWidget(widgetMessageLog);
    m_scrollMessageBar = m_scrollMessageLog->verticalScrollBar();

    // Initialize options window
    m_dialogOptions = new Options(this);

    // Initialize encryption salt
    Botan::AutoSeeded_RNG rng;
    m_salt = rng.random_vec(16);
    qDebug() << "N::Netchat():  Salt\t" << m_salt;

    // Generate random user name consisting of two bytes in hex notation
    quint16 result = 0;
    do
    {
        Botan::SecureVector<uint8_t> randNumbers;
        randNumbers = rng.random_vec(2);
        result = randNumbers[0] * randNumbers[1];
        m_user = QString::number(result, 16).toUpper();
    }
    while (result < 4096);
    qDebug() << "N::Netchat():  User\t" << m_user;

    // Generate salt data and salt size as datatype String in order to be sent over UDP socket
    for (Botan::SecureVector<quint8>::iterator it = m_salt.begin(); it != m_salt.end(); ++it)
        m_saltData.push_back((QString::number(*it).toStdString() + ".").c_str());
    m_saltData.push_front(QString::number(m_saltData.size()).toStdString().c_str());

    // Print local address of the host computer
    qDebug() << "N::Netchat():  IP\t" << m_dialogOptions->m_lineAddressLocal->text();

    // Print operating system of the host computer
    qDebug() << "N::Netchat():  OS\t" << getOsName() << endl;

#ifdef Q_OS_WIN
    // Configure the notification system for Windows platforms
    m_systemTray->setIcon(QIcon(":/resources/icon.png"));
#endif

    // Connect signals with slots
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(getDatagrams()));
    connect(ui->buttonOptions, SIGNAL(clicked(bool)), this, SLOT(showOptions()));
    connect(m_dialogOptions->m_buttonBind, SIGNAL(clicked(bool)), this, SLOT(bindAddress()));
    connect(ui->textMessage, SIGNAL(returnPressed()), this, SLOT(sendMessage()));
    connect(ui->textMessage, SIGNAL(textEdited(QString)), this, SLOT(setTyping(QString)));
    connect(m_systemTray, SIGNAL(messageClicked()), this, SLOT(resetNotification()));
    connect(m_scrollMessageBar, SIGNAL(rangeChanged(int,int)), this, SLOT(moveScrollBarToBottom(int, int)));
    connect(m_textTimer, SIGNAL(timeout()), SLOT(resetTyping()));
    connect(this, SIGNAL(receivedMessage(QString, QString)), this, SLOT(setNotification(QString, QString)));
    connect(this, SIGNAL(sendConnect()), this, SLOT(connectHandler()));
    connect(this, SIGNAL(sendDisconnect()), this, SLOT(disconnectHandler()));
    connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)), this, SLOT(resetNotification()));
}

Netchat::~Netchat()
{
    delete ui;
}

void Netchat::bindAddress()
{
    // Generate encryption key for AES-256
    try
    {
        QString passphrase = m_dialogOptions->m_lineKey->text();
        m_pbkdf = Botan::get_pbkdf("PBKDF2(SHA-256)");
        Botan::SymmetricKey key = m_pbkdf->derive_key(32, passphrase.toStdString(), &m_salt[0], m_salt.size(), 100000);
        m_aes256_keys.insert(m_user, key);
    }
    catch(std::exception& e)
    {
        QMessageBox::critical(this, "Encryption Error", QString("Exception caught:\n\n%1!").arg(e.what()), QMessageBox::Ok);
        return;
    }

    // Initialize host connection
    m_localAddr.setAddress(m_dialogOptions->m_lineAddressLocal->text());
    m_localPort = m_dialogOptions->m_linePortLocal->text().toInt();
    m_socket->bind(m_localAddr, m_localPort);

    // Create a message which will identify the client as a new incoming connection to the server
    QByteArray udpPaket;
    udpPaket.append("$$_");
    udpPaket.append(m_user);
    udpPaket.append("_");
    udpPaket.append(QString::number(m_localPort));

    if (m_dialogOptions->m_checkServerMode->isChecked() == true)
    {
        // Open server connection (server mode enabled)
        m_serverAddr = m_localAddr;
        m_serverPort = m_dialogOptions->m_linePortServer->text().toInt();
        m_server = new Server(m_serverAddr, m_serverPort, this);
        m_server->startServer();
    }
    else
    {
        // Initialize server connection (server mode disabled)
        QHostInfo hostAddress = QHostInfo::fromName(m_dialogOptions->m_lineAddressServer->text());
        m_serverAddr.setAddress(hostAddress.addresses().first().toString());
        m_serverPort = m_dialogOptions->m_linePortServer->text().toInt();
    }

    // Notify server of a new client connection
    m_socket->writeDatagram(udpPaket, m_serverAddr, m_serverPort);
    m_dialogOptions->close();
    emit sendConnect();
}

void Netchat::sendMessage(QString message)
{
    static quint64 numText = 0;

    if (m_socket->state() == QAbstractSocket::BoundState)
    {
        if (!ui->textMessage->text().isEmpty() || !message.isEmpty())
        {
            // Set message text according to current action use case
            QString text;
            if (message.isEmpty())
            {
                // Use workaround for carriage return bug
                text = ui->textMessage->text();
                numText++;
                ui->textMessage->clear();
            }
            else if (message == "CONNECT")
            {
                text = QString("<i>User %1 connected</i>").arg(m_user);
            }
            else if (message == "QUIT")
            {
                text = QString("<i>User %1 disconnected</i>").arg(m_user);
            }
            else if (message == "TYPING")
            {
                text = QString("<TYPING>");
            }
            else if (message == "NOT_TYPING")
            {
                text = QString("<NOT_TYPING>");
            }

            // Encrypt text message using AES-256
            try
            {
                Botan::Pipe encrypt(Botan::get_cipher("AES-256/EAX", m_aes256_keys[m_user], Botan::ENCRYPTION), new Botan::Hex_Encoder);
                encrypt.process_msg(text.toStdString());
                text = QString::fromStdString(encrypt.read_all_as_string(0));
            }
            catch (std::exception& e)
            {
                QMessageBox::critical(this, "Encryption Error", QString("Exception caught:\n\n%1!").arg(e.what()), QMessageBox::Ok);
                return;
            }

            // Prepare message to be sent to the server
            QByteArray udpPaket;
            udpPaket.append(m_user);
            udpPaket.append(m_saltData);
            udpPaket.append(QString::number(numText).rightJustified(19, '0'));
            udpPaket.append(text);

            // Send message to the server
            m_socket->writeDatagram(udpPaket, m_serverAddr, m_serverPort);
        }
    }
    else
    {
        qDebug() << "N::sendMessage():  Socket not bound to an address and port!";
    }
}

void Netchat::getDatagrams()
{
    QByteArray datagram;
    QHostAddress senderAddress;
    quint16 senderPort;

    while (m_socket->hasPendingDatagrams())
    {
        // Read incoming raw message from socket
        datagram.resize(m_socket->pendingDatagramSize());
        m_socket->readDatagram(datagram.data(), m_socket->pendingDatagramSize(), &senderAddress, &senderPort);

        // Extract user name
        QString user;
        user = datagram.left(4);

        // Extract salt size and salt data
        Botan::SecureVector<quint8> salt;
        QList<QByteArray> saltData;
        quint8 saltSize;
        saltSize = datagram.mid(4, 2).toInt();
        saltData = datagram.mid(6, saltSize).split('.');
        saltData.removeLast();
        for (QList<QByteArray>::iterator it = saltData.begin(); it != saltData.end(); ++it)
            salt.push_back((*it).toInt());

        // Extract text message counter
        quint64 numText;
        numText = datagram.mid(6 + saltSize, 19).toLongLong();

        // Extract encrypted text message
        QString text;
        text = datagram.mid(6 + saltSize + 19);

        qDebug() << "N::getDiagrams(): " << user << "\t" << saltSize << salt << text;

        // Decrypt text message using AES-256
        try
        {
            if (ui->checkDecryption->isChecked())
            {
                Botan::SymmetricKey key;
                if (m_aes256_keys.contains(user))
                {
                    // Long process of generating a symmetric key can be skipped
                    key = m_aes256_keys[user];
                }
                else
                {
                    // Generate a new symmetric key for the user
                    key = m_pbkdf->derive_key(32, m_dialogOptions->m_lineKey->text().toStdString(), &salt[0], salt.size(), 100000);
                    m_aes256_keys.insert(user, key);
                }
                Botan::Pipe decrypt(new Botan::Hex_Decoder, Botan::get_cipher("AES-256/EAX", m_aes256_keys[user], Botan::DECRYPTION));
                decrypt.process_msg(text.toStdString());
                text = QString::fromStdString(decrypt.read_all_as_string(0));
            }
        }
        catch (std::exception& e)
        {
            QMessageBox::critical(this, "Decryption Error", QString("Exception caught:\n\n%1!").arg(e.what()), QMessageBox::Ok);
            return;
        }

        // Update window title according to the typing status, otherwise print the text message
        if (text == "<TYPING>")
        {
            if (user != m_user)
            {
                this->setWindowTitle(m_appName + " - " + user + " typing...");
            }
        }
        else if (text == "<NOT_TYPING>")
        {
            if (user != m_user)
            {
                this->setWindowTitle(m_appName);
            }
        }
        else
        {
            // Set mouse tooltip information for the text message
            QLabel *labelText = new QLabel();
            labelText->setText(QString("  %1  ").arg(text));
            labelText->setToolTip("[" + QDateTime::currentDateTime().toString("hh:mm:ss") + "] " + "#" + QString::number(numText));

#if defined(Q_OS_UNIX)
            QFont font("Helvetica", 13);
#elif defined(Q_OS_WIN)
            QFont font("MS Shell Dlg 2", 8);
#endif
            QFontMetrics fontText(font);
            qint32 lengthText = fontText.width(labelText->text());

            // Word wrap if text message width exceeds 75% of the message log window width
            if ((lengthText + 20) > (0.75 * m_scrollMessageLog->width()))
            {
                const quint16 blockSize = 50;
                QString blockText = labelText->text();
                for (quint16 i = blockSize; i < blockText.length(); i += blockSize)
                {
                    // TODO: This needs to be fixed, because it crashes if there is only one word which is too long!
                    if (ui->checkDecryption->isChecked())
                    {
                        while (blockText[i] != ' ')
                            i--;
                        blockText.insert(i, " \n ");
                    }
                }
                labelText->setText(blockText);
            }

            // Set text message style depending on whether the message was sent from the host user or not
            if (m_user == user)
            {
                // Print text message as a grey bubble on the left window side
                labelText->setStyleSheet("border-radius: 6px; background-color: #E5E5EA; color: black;");
                labelText->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                m_layoutMessageLog->addWidget(labelText);
                m_layoutMessageLog->setAlignment(labelText, Qt::AlignLeft);
            }
            else
            {
                // Print text message as a blue bubble on the right window side
                labelText->setStyleSheet("border-radius: 6px; background-color: #0B93F6; color: white;");
                labelText->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                m_layoutMessageLog->addWidget(labelText);
                m_layoutMessageLog->setAlignment(labelText, Qt::AlignRight);
            }

            // Set vertical scrollbar to the bottom
            m_scrollMessageLog->verticalScrollBar()->setValue(m_scrollMessageLog->verticalScrollBar()->value() + 10);

            // Use host computer notification system that a new message has been received
            emit receivedMessage(user, text);
        }
        qDebug() << "N::getDiagrams(): \t\t" << text;
    }
}

void Netchat::setNotification(QString user, QString text)
{
    // Set desktop notification for macOS/Windows which shows that a new message has been received
#if defined(Q_OS_UNIX)
    Q_UNUSED(user);
    Q_UNUSED(text);

    if (m_socket->state() == QAbstractSocket::BoundState)
    {
        if (this->isActiveWindow() == true)
        {
            QtMac::setBadgeLabelText("");
        }
        else
        {
            quint16 msgCounter;
            msgCounter = QtMac::badgeLabelText().toInt();
            msgCounter++;
            QtMac::setBadgeLabelText(QString::number(msgCounter));
        }
    }
#elif defined(Q_OS_WIN)
    if (m_socket->state() == QAbstractSocket::BoundState)
    {
        if (this->isActiveWindow() == true)
        {
            // TODO: Implement code. Message box shall be deactivated once a new message arrives.
        }
        else
        {
            m_systemTray->setVisible(true);
            m_systemTray->showMessage("<" + user + ">", text);
        }
    }
#endif
}

void Netchat::resetNotification()
{
    // Disable desktop notification if either the notification message was clicked or the application focus was reset
#if defined(Q_OS_UNIX)
    if (m_socket->state() == QAbstractSocket::BoundState)
    {
        if (this->isActiveWindow() == true)
        {
            QtMac::setBadgeLabelText("");
        }
    }
#elif defined(Q_OS_WIN)
    if (m_socket->state() == QAbstractSocket::BoundState)
    {
        m_systemTray->setVisible(false);
    }
#endif
}

void Netchat::showOptions()
{
#if defined(Q_OS_UNIX)
    m_dialogOptions->setGeometry(QRect(this->x() + 90, this->y() + 22, 320, 230));
#elif defined(Q_OS_WIN)
    m_dialogOptions->setGeometry(QRect(this->x() + 100, this->y() + 38, 320, 230));
#endif
    m_dialogOptions->exec();
}

void Netchat::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        // Close application if escape button is pressed
        case Qt::Key_Escape:
        {
            if (m_socket->state() == QAbstractSocket::BoundState)
            {
                // Send QUIT message and wait one second for the delivery
                emit sendDisconnect();
                QTimer::singleShot(1000, qApp, SLOT(quit()));
            }
            else
            {
                qApp->quit();
            }
        }
        // Set focus to the text field if carriage return is pressed
        case Qt::Key_Return:
        {
            ui->textMessage->setFocus();
        }
    }
}

void Netchat::mousePressEvent(QMouseEvent *event)
{
    // Set focus to the text field if application window is pressed by the mouse
    event->button();
    ui->textMessage->setFocus();
}

void Netchat::connectHandler()
{
    // Send CONNECT message to server
    sendMessage("CONNECT");
}

void Netchat::disconnectHandler()
{
    // Send QUIT message to server
    sendMessage("QUIT");
}

void Netchat::moveScrollBarToBottom(int min, int max)
{
    Q_UNUSED(min);

    // Scroll to bottom window, because a new message has been received
    m_scrollMessageLog->verticalScrollBar()->setValue(max);
}

void Netchat::setTyping(QString text)
{
    Q_UNUSED(text);

    // Update typing status since host user started typing
    if (m_socket->state() == QAbstractSocket::BoundState)
    {
        if (!m_textTimer->isActive())
        {
            sendMessage("TYPING");
        }
        // Typing status will be reset if host user has not been typing for more than 700ms
        m_textTimer->start(700);
    }
}

void Netchat::resetTyping()
{
    // Update typing status since host user stopped typing
    m_textTimer->stop();
    sendMessage("NOT_TYPING");
}

QString Netchat::getOsName()
{
    // Determine the operating system of the host computer
#if defined(Q_OS_ANDROID)
    return QLatin1String("android");
#elif defined(Q_OS_BLACKBERRY)
    return QLatin1String("blackberry");
#elif defined(Q_OS_IOS)
    return QLatin1String("ios");
#elif defined(Q_OS_MACOS)
    return QLatin1String("macos");
#elif defined(Q_OS_TVOS)
    return QLatin1String("tvos");
#elif defined(Q_OS_WATCHOS)
    return QLatin1String("watchos");
#elif defined(Q_OS_WINCE)
    return QLatin1String("wince");
#elif defined(Q_OS_WIN)
    return QLatin1String("windows");
#elif defined(Q_OS_LINUX)
    return QLatin1String("linux");
#elif defined(Q_OS_UNIX)
    return QLatin1String("unix");
#else
    return QLatin1String("unknown");
#endif
}
