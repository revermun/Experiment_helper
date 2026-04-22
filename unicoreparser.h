#ifndef UNICOREPARSER_H
#define UNICOREPARSER_H

#include <stdint.h>
#include <stddef.h>
#include <QMap>
#include <QString>
#include <QSerialPort>
#include <QTcpSocket>
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
        quint8 cpuIdle;
        quint16 messageId;
        quint16 messageLength;
        quint8 timeRef;
        quint8 timeStatus;
        quint16 wn;
        quint32 ms;
        quint32 reserved;
        quint8 version;
        quint8 leapSec;
        quint16 delayMs;
    } binaryHeader;

    struct AsciiHeader {
        QString messageName;
        quint8 cpuIdle;
        QString timeRef;
        QString timeStatus;
        quint16 wn;
        quint32 ms;
        quint32 reserved;
        quint8 version;
        quint8 leapSec;
        quint16 delayMs;
    } asciiHeader;


};



class UnicoreParser
{
public:
    UnicoreParser(QObject *connection);
    static ULONG calcCRC(QByteArray msg);
    UnicoreMessage parseMessage(QByteArray* buff);
    UnicoreMessage parseAsciiMessage(QByteArray* buff);
    UnicoreMessage parseBinaryMessage(QByteArray* buff);
    bool sendMessage(QString msg);
private:
    QObject* connection;
};

#endif // UNICOREPARSER_H
