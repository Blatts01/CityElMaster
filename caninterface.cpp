#include "caninterface.h"


CanInterface* CanInterface::m_instance = NULL;

u_int16_t CanInterface::m_canSpeed = 125;
const u_int32_t CanInterface::m_index = 0;
bool CanInterface::m_canDeviceOnline = false;
bool CanInterface::m_canOnline = false;
const QByteArray CanInterface::m_libPath = "../libmhstcan.so";


CanInterface::CanInterface(QObject *parent) :
    QObject(parent)
{
}
bool CanInterface::startCanInterface()
{

    bool success = false;


    if (loadCanDriver() == false)
    {
        // Fehlerbehandlung
        return success = false;
    }

    // Initialisiere Treiber
    if (initCanDriver() == false)
    {
        // Fehlerbehandlung
        return success = false;
    }

    // Events setzen und freischalten
    initEvents();

    // Setze CAN-Speed
    CanSetSpeed(m_index, m_canSpeed);

    // Setze AutoConnect
    CanSetOptions("AutoConnect=1");

    // Schnittstelle PC <-> Tiny-Can öffnen
    if (openCanDevice() == false)
    {
        // Fehlerbehandlung
        return success = false;
    }
    // CAN-Bus starten und initialisieren
    if (startCanBus() == false)
    {
        // Fehlerbehandlung
        return success = false;
    }
    return success = true;
}
bool CanInterface::getCanOnline()
{
    return m_canOnline;
}

int CanInterface::getCanSpeed()
{
    return m_canSpeed;
}

bool CanInterface::startCanBus()
{
    int32_t ret = 0;
    bool success = false;

    ret = CanSetMode(m_index, OP_CAN_START, CAN_CMD_ALL_CLEAR);

    if (ret < 0)
    {
        // Setze Fehlermeldung ab
        m_canOnline = false;
        success = false;
    }
    else
    {
        // _canOnline nur setzen, wenn TinyCan angeschlossen
        if (m_canDeviceOnline)
        {
            m_canOnline = true;
        }
        success = true;
    }
    return success;
}

bool CanInterface::resetCanBus()
{
    //* Beschreibung *
    //*  Resetet den CAN-Bus und löscht alle Fehlerflags

    int32_t ret = 0;
    ret = CanSetMode(m_index, OP_CAN_RESET, CAN_CMD_ALL_CLEAR);

    if (ret < 0)
    {
        m_canOnline = false;
        return false;
    }
    else
    {
        // _canOnline nur setzen, wenn TinyCan angeschlossen
        if (m_canDeviceOnline)
        {
            m_canOnline = true;
        }
        return true;
    }
}

bool CanInterface::stopCanBus()
{
    //* Beschreibung *
    //*  Stoppt den CanBus

    int32_t ret = 0;
    ret = CanSetMode(m_index, OP_CAN_STOP, CAN_CMD_ALL_CLEAR);

    if (ret < 0)
    {
        m_canOnline = false;
        return false;
    }
    else
    {
        // _canOnline nur setzen, wenn TinyCan angeschlossen
        if (m_canDeviceOnline)
        {
            m_canOnline = false;
        }
        return true;
    }
}

bool CanInterface::loadCanDriver()
{
    //* Beschreibung *
    //*  Ladet den Tiny-Can Treiber
    //*  Sie muss nur einmal ausgeführt werden.

    int32_t ret = 0;
    bool success = false;
    // Bei compile für RasPi muss eine andere Lib verwendet werden.
    // Define für RASPICONST geschieht in .pro Datei
    // Methode ladet die Lib und initialisiert den Treiber.
    ret = LoadDriver(m_libPath.data());

    // Fehlerbehandlung
    if (ret < 0)
    {
        // Hole Fehlerbeschreibung und setzte Fehlermeldung ab
        success = false;
    }
    else
    {
        success = true;
    }
    return success;
}

bool CanInterface::initCanDriver()
{
    //* Beschreibung *
    //*  Initialisiert den CAN-Treiber

    int32_t ret = 0;
    bool success = false;

    ret = CanInitDriver(NULL);

    // Fehlerbehandlung
    if (ret < 0)
    {
        // Hole Fehlerbeschreibung und setzte Fehlermeldung ab
        success = false;
    }
    else
    {
        success = true;
    }
    return success;
}

void CanInterface::initEvents()
{
    //* Beschreibung *
    //*  Setzt die Event-Funktionen und schaltet die Events frei.

    // Event Funktionen setzen
    CanSetRxEventCallback(&canRxEvent);

    // Alle Events freigeben
    CanSetEvents(EVENT_ENABLE_ALL);
}

bool CanInterface::openCanDevice()
{
    //* Beschreibung *
    //*  Öffnet das TinyCanDevice

    int32_t ret = 0;
    bool success = false;

    ret = CanDeviceOpen(m_index, NULL);

    if (ret < 0)
    {
        // Setze Fehlermeldung ab
        success = false;
    }
    else
    {
        m_canDeviceOnline = true;
        success = true;
    }
    return success;
}


bool CanInterface::sendMessage(quint32 txIdent, QByteArray txData)
{
    //* Beschreibung *
    //*  Sendet die übergebene Nachricht mit dem übergebenen Identifier per CAN raus.
    //*  Dabei wird nur das Basis-Frame-Format erlaubt. Dies kann jedoch über
    //*  das Attribut '_enableEFF' jedoch entsperrt werden.

    bool success = false;
    struct TCanMsg message;

    message.MsgFlags = 0L; // Setze Flags zurück

//    // Bei Extended-Frame-Format
//    if (m_enableEFF)
//    {
//        message.Flags.Flag.EFF = 1; // Setze EFF-Flag
//    }

    // Identifier darf bei Basis-Frame-Format nur 11 Bit haben
    if (txIdent < 2048 /*&& !m_enableEFF*/)
    {
        message.Id = (u_int32_t)txIdent;
    }
//    // Identifier darf bei Extended-Frame-Format nur 29 Bit haben
//    else if (txIdent < 536870912)
//    {
//        message.Id = (u_int32_t)txIdent;
//    }
//    // Error: Identifier zu groß
//    else
//    {
//        return success = false;
//    }

    // Schreibe Größe der Daten, wenn es max. 8 Byte sind
    if (txData.size() <= 8)
    {
        message.MsgLen = txData.size();
    }
    // Daten größer als 8 Byte -> return
    else
    {
        return success = false;
    }

    // Drehe alle Bytes, damit Wertigekeit zum Versenden stimmt
    QByteArray txDataMirrored;

    for (int i = 0; i < txData.size(); i++)
    {
        txDataMirrored.append(txData.at(txData.size()-1 -i));
    }

    // Kopieren der Daten in SendeTelegramm
    memcpy(message.MsgData, txDataMirrored.constData(), txDataMirrored.size());

    // Wenn CAN Online -> Nachricht versenden
    if (m_canOnline)
    {
        int32_t ret = 0;

        ret = CanTransmit(m_index, &message, 1);

        // Fehlerbehandlung
        if (ret < 0)
        {
            // Hole Fehlerbeschreibung und setzte Fehlermeldung ab
            success = false;
        }
        else
        {

            success = true;
        }
    }
    // CAN Offline -> Fehlermeldung
    else
    {

    }

    return success;
}



void CanInterface::canRxEvent(u_int32_t index, TCanMsg *msg, int32_t count)
{
    //* Beschreibung *
    //*  Callback-Methode wird beim Empfang einer Nachricht aufgerufen.
    //*  Dabei werden alle eingegangenen Nachrichten ausgelesen und
    //*  es wird ein Signal emitiert, welches mit der "Empfangsmethode"
    //*  des CanParsers verbunden wird.
    //*
    //*  Folgende Nachrichten werden verworfen:
    //*   - Leere Nachrichten
    //*   - Extended-Frame-Format Nachrichten
    //*
    //*  Damit die while-Schleife nicht unendlich lange durchlaufen werden kann,
    //*  wird die Variable 'watchVar' bei jeder Schleifen-Iteration erhöht.
    //*  Beim Überschreiten von 'watchVarMax' wird die Schleife verlassen!

    struct TCanMsg message;
    quint32 rxIdent = 0;
    QByteArray rxData = 0;
    u_int32_t i;
    unsigned int watchVar = 0;
    unsigned int watchVarMax = 1500;

    // Lese solang Nachichten, bis keine mehr vorhanden
    while (CanReceive(0, &message, 1) > 0 && watchVar < watchVarMax)
    {
        // Erhöhe watchVar, zum Sicherstellen, dass while nicht unendlich lange durchlaufen wird
        watchVar++;

        // SenderID holen
        rxIdent = (quint32)message.Id;

        // Verwendet Extended-Frame-Format -> verwerfen
        if (message.Flags.Flag.EFF)
        {
            // Lese nächste Nachricht
            continue;
        }
        // Falls Daten vorhanden -> auslesen
        else if (message.MsgLen)
        {
            // Leere Datenfeld
            rxData.clear();

            for (i = 0; i < message.MsgLen; i++)
            {
                // Daten müssen gedreht werden, da rxData aneinander gehängt wird.
                // -1 da < MsgLen
                rxData.append(message.MsgData[message.MsgLen -1 - i]);
            }
        }
        // Leere Nachricht -> verwerfen
        else
        {
            // Lese nächste Nachricht
            continue;
        }
        // Versende Signal mit Daten
        // Quelle: http://stackoverflow.com/questions/9411153/sending-signal-from-static-class-method-in-qt
        emit m_instance->newMessage(rxIdent, rxData);
    }
}
