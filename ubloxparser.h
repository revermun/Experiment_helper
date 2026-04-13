#ifndef UBLOXPARSER_H
#define UBLOXPARSER_H

#include <stdint.h>
#include <stddef.h>
#include <QMap>
#include <QString>
#include <QSerialPort>
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

///parseMessage должен возвращать словарь содержащий всю информацию о сообщении
/// обязательны поля class ID, далее по ним можно определять конкретные значения
///sendMessage должен принимать название сообщения и его параметры, если нет параметров то это GET
/// Если есть то это SET
///
/// Расшифрока сообщений: для приложения мы хотим считать конкретные сообщения.
/// Скорее всего нужно в класс добавить по экземпляру каждой структуры и изменять их по необходимости(при выборе вкладки и нажатии на кнопку запроса)
/// для того чтобы получить информацию нужно сначала отправить запрос (сообщение без payload) затем считывать сообщения до тех пор пока не найдется искомое
/// В парсере достаточно реализовать для этого функцию отправки сообщения и в конфигурации устройств при нажатии на кнопку провести отправку запроса,
/// затем в цикле парсить сообщения до тех пор пока не найдется искомое, считать его и изменить соответствующие поля
///

enum messageIndexes{
    INDEX_MSG_HDR1,
    INDEX_MSG_HDR2,
    INDEX_MSG_CLASS,
    INDEX_MSG_ID,
    INDEX_MSG_LEN1,
    INDEX_MSG_LEN2
};


class UbloxParser
{
public:
    UbloxParser(QSerialPort *connection);
    ~UbloxParser();
    QMap<QString,QByteArray> parseMessage(QByteArray* buff);
    bool sendMessage(QByteArray msg);
    static QByteArray calcCheckSum(QByteArray msg);
    static Message* decode(QMap<QString,QByteArray>);
    static QByteArray createPollMessage(uint8_t byteClass, uint8_t byteID);
    Message* decodeMessage(QMap<QString,QByteArray> msg);

private:
    QSerialPort *connection;
};

#endif // UBLOXPARSER_H
