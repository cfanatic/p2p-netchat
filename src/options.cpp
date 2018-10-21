#include "options.h"

Options::Options(QWidget *parent) :
    QDialog(parent)
{
    // Initialize options window and set default values
    m_labelKey = new QLabel("Encryption Key:");
    m_lineKey = new QLineEdit("car-tree-subway-fish");
    m_layoutEncryption = new QVBoxLayout();
    m_layoutEncryption->addWidget(m_labelKey);
    m_layoutEncryption->addWidget(m_lineKey);
    m_verticalSpacer = new QSpacerItem(40, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_layoutEncryption->addItem(m_verticalSpacer);

    m_lineAddressLocal = new QLineEdit("255.255.255.255");
    m_linePortLocal = new QLineEdit("7000");
    m_layoutNetworkLocal = new QHBoxLayout();
    m_layoutNetworkLocal->addWidget(m_lineAddressLocal);
    m_layoutNetworkLocal->addWidget(m_linePortLocal);

    m_lineAddressServer = new QLineEdit("<remote_server>");
    m_linePortServer = new QLineEdit("7001");
    m_layoutNetworkServer = new QHBoxLayout();
    m_layoutNetworkServer->addWidget(m_lineAddressServer);
    m_layoutNetworkServer->addWidget(m_linePortServer);

    m_checkServerMode = new QCheckBox("Server Mode");
    m_buttonBind = new QPushButton("Open Connection");
    m_gridLayout = new QGridLayout();
    m_buttonBind->setFocusPolicy(Qt::NoFocus);
    m_gridLayout->addLayout(m_layoutEncryption, 1, 1, 1, 1);
    m_gridLayout->addLayout(m_layoutNetworkLocal, 2, 1, 1, 1);
    m_gridLayout->addLayout(m_layoutNetworkServer, 3, 1, 1, 1);
    m_gridLayout->addWidget(m_checkServerMode, 4, 1, 1, 1);
    m_gridLayout->addWidget(m_buttonBind, 5, 1, 1, 1);

    this->setFocus();
    this->setLayout(m_gridLayout);

    // Get local IP address of the host computer
    calcLocalAddress();
}

Options::~Options()
{
    delete m_labelKey;
    delete m_lineKey;
    delete m_layoutEncryption;
    delete m_lineAddressLocal;
    delete m_linePortLocal;
    delete m_layoutNetworkLocal;
    delete m_lineAddressServer;
    delete m_linePortServer;
    delete m_layoutNetworkServer;
    delete m_checkServerMode;
    delete m_buttonBind;
    delete m_gridLayout;
}

void Options::calcLocalAddress()
{
    // IP address is considered to be in the subnet of 192.168.0.0/24
    QList<QHostAddress> hostAddresses = QNetworkInterface::allAddresses();
    for (QList<QHostAddress>::iterator it = hostAddresses.begin(); it != hostAddresses.end(); ++it)
    {
        if ((*it).isInSubnet(QHostAddress::parseSubnet("192.168.0.0/24")))
        {
            // IP address is found which will be set to the default socket address
            m_lineAddressLocal->setText((*it).toString());
            break;
        }
    }
}
