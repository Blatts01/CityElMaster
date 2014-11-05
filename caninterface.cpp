#include "caninterface.h"

CanInterface::CanInterface(QObject *parent) :
    QObject(parent)
{
}
bool CanInterface::startCanInterface()
{
    //* Beschreibung *
    //*  Enthält die Funktionen und Fehlerbehandlung zum Starten des
    //*  CAN-Interfaces.

    bool success = false;

    // Lade Can/API Treiber
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
    CanSetSpeed(_index, _canSpeed);

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
bool CanHandler::getCanOnline()
{
    return _canOnline;
}

int CanHandler::getCanSpeed()
{
    return _canSpeed;
}

bool CanHandler::startCanBus()
{
    int32_t ret = 0;
    bool success = false;

    ret = CanSetMode(_index, OP_CAN_START, CAN_CMD_ALL_CLEAR);

    if (ret < 0)
    {
        // Setze Fehlermeldung ab
        _canOnline = false;
        success = false;
    }
    else
    {
        // _canOnline nur setzen, wenn TinyCan angeschlossen
        if (_canDeviceOnline)
        {
            _canOnline = true;
        }
        success = true;
    }
    return success;
}

bool CanHandler::resetCanBus()
{
    //* Beschreibung *
    //*  Resetet den CAN-Bus und löscht alle Fehlerflags

    int32_t ret = 0;
    ret = CanSetMode(_index, OP_CAN_RESET, CAN_CMD_ALL_CLEAR);

    if (ret < 0)
    {
        _canOnline = false;
        return false;
    }
    else
    {
        // _canOnline nur setzen, wenn TinyCan angeschlossen
        if (_canDeviceOnline)
        {
            _canOnline = true;
        }
        return true;
    }
}

bool CanHandler::stopCanBus()
{
    //* Beschreibung *
    //*  Stoppt den CanBus

    int32_t ret = 0;
    ret = CanSetMode(_index, OP_CAN_STOP, CAN_CMD_ALL_CLEAR);

    if (ret < 0)
    {
        _canOnline = false;
        return false;
    }
    else
    {
        // _canOnline nur setzen, wenn TinyCan angeschlossen
        if (_canDeviceOnline)
        {
            _canOnline = false;
        }
        return true;
    }
}

bool CanHandler::loadCanDriver()
{
    //* Beschreibung *
    //*  Ladet den Tiny-Can Treiber
    //*  Sie muss nur einmal ausgeführt werden.

    int32_t ret = 0;
    bool success = false;
    // Bei compile für RasPi muss eine andere Lib verwendet werden.
    // Define für RASPICONST geschieht in .pro Datei
    // Methode ladet die Lib und initialisiert den Treiber.
    ret = LoadDriver(_libPathARM.data());

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

bool CanHandler::initCanDriver()
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

void CanHandler::initEvents()
{
    //* Beschreibung *
    //*  Setzt die Event-Funktionen und schaltet die Events frei.

    // Event Funktionen setzen
    CanSetPnPEventCallback(&canPnPEvent);
    CanSetStatusEventCallback(&canStatusEvent);
    CanSetRxEventCallback(&canRxEvent);

    // Alle Events freigeben
    CanSetEvents(EVENT_ENABLE_ALL);
}

bool CanHandler::openCanDevice()
{
    //* Beschreibung *
    //*  Öffnet das TinyCanDevice

    int32_t ret = 0;
    bool success = false;

    ret = CanDeviceOpen(_index, NULL);

    if (ret < 0)
    {
        // Setze Fehlermeldung ab
        createErrorMsg(ret, "openCanDevice()");
        success = false;
    }
    else
    {
        _canDeviceOnline = true;
        success = true;
    }
    return success;
}


bool CanHandler::sendMessage(quint32 txIdent, QByteArray txData)
{
    //* Beschreibung *
    //*  Sendet die übergebene Nachricht mit dem übergebenen Identifier per CAN raus.
    //*  Dabei wird nur das Basis-Frame-Format erlaubt. Dies kann jedoch über
    //*  das Attribut '_enableEFF' jedoch entsperrt werden.

    bool success = false;
    struct TCanMsg message;

    message.MsgFlags = 0L; // Setze Flags zurück

    // Bei Extended-Frame-Format
    if (_enableEFF)
    {
        message.Flags.Flag.EFF = 1; // Setze EFF-Flag
    }

    // Identifier darf bei Basis-Frame-Format nur 11 Bit haben
    if (txIdent < 2048 && !_enableEFF)
    {
        message.Id = (u_int32_t)txIdent;
    }
    // Identifier darf bei Extended-Frame-Format nur 29 Bit haben
    else if (txIdent < 536870912)
    {
        message.Id = (u_int32_t)txIdent;
    }
    // Error: Identifier zu groß
    else
    {
        QObject sender;
        sender.setObjectName(_objectName);
        QString errMsg = "";
        errMsg.append("Identifier: ").append(QString("%1").arg(txIdent)).append(" out of Range!");
        _errorHandle->newMessage(&sender, "sendMessage()", "Error", errMsg, _errorLevel);

        return success = false;
    }

    // Schreibe Größe der Daten, wenn es max. 8 Byte sind
    if (txData.size() <= 8)
    {
        message.MsgLen = txData.size();
    }
    // Daten größer als 8 Byte -> return
    else
    {
        QObject sender;
        sender.setObjectName(_objectName);
        QString errMsg = "";
        errMsg.append("Data from Identifier: ").append(QString("%1").arg(txIdent)).append(" more than 8 Byte! Message not send!");
        _errorHandle->newMessage(&sender, "sendMessage()", "Error", errMsg, _errorLevel);

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
    if (_canOnline)
    {
        int32_t ret = 0;

        ret = CanTransmit(_index, &message, 1);

        // Fehlerbehandlung
        if (ret < 0)
        {
            // Hole Fehlerbeschreibung und setzte Fehlermeldung ab
            createErrorMsg(ret, "sendMessage()");
            success = false;
        }
        else
        {
            QObject sender;
            sender.setObjectName(_objectName);
            QString errMsg = "";
            errMsg.append("Send CAN-Message with Ident: ")
                  .append(QString("%1").arg(txIdent))
                  .append(" with Data: ")
                  .append(txData.toHex());
            _errorHandle->newMessage(&sender, "sendMessage()", "Warning", errMsg, _ioLevel);
            success = true;
        }
    }
    // CAN Offline -> Fehlermeldung
    else
    {
        QObject sender;
        sender.setObjectName(_objectName);
        QString errMsg = "";
        errMsg.append("Can't send CAN-Message with Ident: ").append(QString("%1").arg(txIdent)).append(" ! CAN Bus offline!");
        _errorHandle->newMessage(&sender, "sendMessage()", "Error", errMsg, _errorLevel);
    }

    return success;
}



void CanHandler::canRxEvent(u_int32_t index, TCanMsg *msg, int32_t count)
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
            QObject sender;
            sender.setObjectName(_objectName);
            QString debugMsg = "";
            debugMsg.append("CAN-Message from Ident: ")
                    .append(QString("%1").arg(rxIdent))
                    .append(" uses Extended-Frame-Format! Message discarded!");
            _errorHandle->newMessage(&sender, "canRxEvent()", "Warning", debugMsg, _debugLevel);

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
            QObject sender;
            sender.setObjectName(_objectName);
            QString debugMsg = "";
            debugMsg.append("CAN-Message from Ident: ")
                    .append(QString("%1").arg(rxIdent))
                    .append(" contains no Data! Message discarded!");
            _errorHandle->newMessage(&sender, "canRxEvent()", "Warning", debugMsg, _debugLevel);

            // Lese nächste Nachricht
            continue;
        }

        QObject sender;
        sender.setObjectName(_objectName);
        QString errMsg = "";
        errMsg.append("New CAN-Message from Ident: ")
              .append(QString("%1").arg(rxIdent))
              .append(" with Data: ")
              .append(rxData.toHex());
        _errorHandle->newMessage(&sender, "canRxEvent()", "Warning", errMsg, _ioLevel);

        // Versende Signal mit Daten
        // Quelle: http://stackoverflow.com/questions/9411153/sending-signal-from-static-class-method-in-qt
        emit _instance->newMessage(rxIdent, rxData);
    }
}
