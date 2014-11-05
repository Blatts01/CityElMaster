#ifndef CANINTERFACE_H
#define CANINTERFACE_H

#include <QObject>

class CanInterface : public QObject
{
    Q_OBJECT

public:
    static CanInterface* getInstance();

private:
    explicit CanInterface(QObject *parent = 0);
    ~CanInterface();
    static CanInterface* _instance;

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

public slots:
    bool sendMessage(quint32 txIdent, QByteArray txData);

signals:
    void newMessage(quint32 rxIdent, QByteArray rxData);

private:
    static void canRxEvent(u_int32_t index, struct TCanMsg* msg, int32_t count);
};
#endif // CANINTERFACE_H
