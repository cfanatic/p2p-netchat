#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QLineEdit>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QGridLayout>
#include <QList>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QCloseEvent>
#include <QDebug>

class Options : public QDialog
{
    Q_OBJECT

public:
    explicit Options(QWidget *parent = 0);
    ~Options();

    QLabel *m_labelKey;
    QLineEdit *m_lineKey;
    QVBoxLayout *m_layoutEncryption;
    QSpacerItem *m_verticalSpacer;
    QLineEdit *m_lineAddressLocal;
    QLineEdit *m_linePortLocal;
    QHBoxLayout *m_layoutNetworkLocal;
    QLineEdit *m_lineAddressServer;
    QLineEdit *m_linePortServer;
    QHBoxLayout *m_layoutNetworkServer;
    QCheckBox *m_checkServerMode;
    QPushButton *m_buttonBind;
    QGridLayout *m_gridLayout;

private:
    void calcLocalAddress();

};

#endif // OPTIONS_H
