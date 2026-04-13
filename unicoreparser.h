#ifndef UNICOREPARSER_H
#define UNICOREPARSER_H

#include <stdint.h>
#include <stddef.h>
#include <QMap>
#include <QString>
#include <QSerialPort>
#include <QDebug>
#include <QDataStream>
#include <QBuffer>
#include <QtEndian>

#include "unicore.h"


struct UnicoreMessage{
    QByteArray header;
    QByteArray data;
    QByteArray crc;
    bool isAscii;
    bool isCommand;

    struct BinaryHeader {
        quint8 cpuIdle;         // CPU idle 0-100
        quint16 messageId;      // Message ID
        quint16 messageLength;  // Message length (длина данных)
        quint8 timeRef;         // Reference time (GPST or BDST)
        quint8 timeStatus;      // Time status
        quint16 wn;             // Week number
        quint32 ms;             // Seconds of week (ms)
        quint32 reserved;       // Reserved
        quint8 version;         // Release version
        quint8 leapSec;         // Leap second
        quint16 delayMs;        // Output delay
    } binaryHeader;

    struct AsciiHeader {
        QString messageName;    // Имя сообщения (например, "PVT", "OBS" и т.д.)
        quint8 cpuIdle;         // CPU idle
        QString timeRef;         // Reference time
        QString timeStatus;      // Time status
        quint16 wn;             // Week number
        quint32 ms;             // Seconds of week (ms)
        quint32 reserved;       // Reserved
        quint8 version;         // Version
        quint8 leapSec;         // Leap second
        quint16 delayMs;        // Output delay
    } asciiHeader;


};



class UnicoreParser
{
public:
    UnicoreParser(QSerialPort *connection);
    static ULONG calcCRC(QByteArray msg);
    UnicoreMessage parseMessage(QByteArray* buff);
    bool sendMessage(QString msg);
private:
    QSerialPort* connection;
};

#endif // UNICOREPARSER_H
