#ifndef CANINTERFACE_H
#define CANINTERFACE_H
#include "can_drv.h"
#include <QObject>

class CanInterface : public QObject
{
    Q_OBJECT

public:
    static CanInterface* getInstance();

private:
    explicit CanInterface(QObject *parent = 0);
    static CanInterface* m_instance;

public:
    bool startCanInterface();
    bool getCanOnline();
    int getCanSpeed();
    static bool startCanBus();
    static bool resetCanBus();
    static bool stopCanBus();

private:
    bool loadCanDriver();
    static bool initCanDriver();
    void initEvents();
    static bool openCanDevice();
    static bool readConfigFile();

    static bool m_canDeviceOnline;
    static bool m_canOnline;
    static u_int16_t m_canSpeed;
    static const u_int32_t m_index;
    static const QByteArray m_libPath;

public slots:
    bool sendMessage(quint32 txIdent, QByteArray txData);

signals:
    void newMessage(quint32 rxIdent, QByteArray rxData);

private:
    static void canRxEvent(u_int32_t index, struct TCanMsg* msg, int32_t count);
};
#endif // CANINTERFACE_H
