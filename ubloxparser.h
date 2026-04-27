#ifndef UBLOXPARSER_H
#define UBLOXPARSER_H

#include <stdint.h>
#include <stddef.h>
#include <QMap>
#include <QString>
#include <QSerialPort>
#include <QTcpSocket>
#include <QDebug>
#include <QDataStream>
#include <QBuffer>

#include "ublox.h"
/// протоколы
/// ubx-cfg-dat - SET реализация в первой вкладке
/// ubx-cfg-cfg - Command
/// ubx-cfg-dgnss - GET/SET
/// ubx-cfg-gnss - GET/SET
/// ubx-cfg-msg - Poll Request
/// ubx-cfg-nav5 - Get/Set
/// ubx-cfg-prt - Poll Request
/// ubx-cfg-rate - GET/SET
/// ubx-cfg-rst - command
///
/// ubx-nav-clock - Periodic/Polled
/// ubx-nav-dop - Periodic/Polled
/// ubx-nav-orb - Periodic/Polled
/// ubx-nav-dgnss - Periodic/Polled
/// ubx-nav-posecef - Periodic/Polled
/// ubx-nav-posllh - Periodic/Polled
/// ubx-nav-relposned - Periodic/Polled
/// ubx-nav-sat - Periodic/Polled
/// ubx-nav-sol - Periodic/Polled
/// ubx-nav-status - Periodic/Polled
/// ubx-nav-velecef - Periodic/Polled
/// ubx-nav-velned - Periodic/Polled
///
/// Единицы измерения:
/// U1 - uint8_t
/// I1 - int8_t
/// X1/2/4 - uint8/16/32_t
/// U2 - uint16_t
/// I2 - int16_t
/// U4 - uint32_t
/// I4 - int32_t
/// R4 - float
/// R8 - double
/// CH - ascii char


enum messageIndexes{
    INDEX_MSG_HDR1,
    INDEX_MSG_HDR2,
    INDEX_MSG_CLASS,
    INDEX_MSG_ID,
    INDEX_MSG_LEN1,
    INDEX_MSG_LEN2
};
/// TODO:: переделать парсер под структуру
struct UbloxMessage{
    uint8_t messId;
    uint8_t messClass;
    uint16_t messLen;
    QByteArray header;
    QByteArray data;
    QByteArray crc;
};


class UbloxParser
{
public:
    UbloxParser(QObject *connection);
    ~UbloxParser();
    QMap<QString,QByteArray> parseMessage(QByteArray* buff);
    UbloxMessage parseMessage(QByteArray *buff);
    bool sendMessage(QByteArray msg);
    static QByteArray calcCheckSum(QByteArray msg);
    static Message* decode(QMap<QString,QByteArray>);
    static QByteArray createPollMessage(uint8_t byteClass, uint8_t byteID);
    Message* decodeMessage(QMap<QString,QByteArray> msg);

private:
    QObject *connection;
};

#endif // UBLOXPARSER_H
