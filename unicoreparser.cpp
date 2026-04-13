#include "unicoreparser.h"

UnicoreParser::UnicoreParser(QSerialPort* connection)
{
    this->connection = connection;
}

ULONG UnicoreParser::calcCRC(QByteArray msg)
{
    std::string string = msg.toStdString();
    return CalculateCRC32(string);
}

ULONG parseAsciiCRC(const QByteArray& crcHexStr) {
    // crcHexStr должен содержать 8 hex символов (4 байта)
    if (crcHexStr.size() < 8) return 0;

    QString hexStr = QString::fromLatin1(crcHexStr.left(8));
    bool ok;
    ULONG crc = hexStr.toUInt(&ok, 16);
    if (!ok) return 0;

    return crc;
}

UnicoreMessage UnicoreParser::parseMessage(QByteArray* buff)
{
    UnicoreMessage res;

    int size = buff->size();
    if (!buff || buff->isEmpty()) return res;
    /// Для бинарных сообщений crc считается для заголовка и тела
    while (size > 24) {
        // Поиск синхро-байтов
        if ((uint8_t)buff->at(0) != 0xaa && (uint8_t)buff->at(0) != '#' && (uint8_t)buff->at(0) != '$') {
            buff->remove(0,1);
            continue;
        }
        if ((uint8_t)buff->at(0) == 0xaa) {
            if ((uint8_t)buff->at(1) != 0x44) {
                continue;
            }

            if ((uint8_t)buff->at(2) != 0xb5) {
                continue;
            }
            uint16_t MessageLength  = (uint8_t)buff->at(6) | ((uint8_t)buff->at(7) << 8);
            int totalMsgLen = 24 + MessageLength + 4;
            if (totalMsgLen > size) {
                break;
            }
            uint8_t* buffdata = reinterpret_cast<uint8_t*>(buff->data());
            if (rtk_crc32(buffdata, MessageLength + 24) == U4(buffdata + MessageLength + 24)){
                UnicoreMessage::BinaryHeader binHeader;
                binHeader.cpuIdle = (uint8_t)buff->at(3);
                binHeader.messageId = (uint8_t)buff->at(4) | ((uint8_t)buff->at(5) << 8);
                binHeader.messageLength  = (uint8_t)buff->at(6) | ((uint8_t)buff->at(7) << 8);
                binHeader.timeRef = (uint8_t)buff->at(8);
                binHeader.timeStatus = (uint8_t)buff->at(9);
                binHeader.wn = (uint8_t)buff->at(10) | ((uint8_t)buff->at(11) << 8);
                binHeader.ms = (uint8_t)buff->at(12) | ((uint8_t)buff->at(13) << 8) |
                               ((uint8_t)buff->at(14) << 16) | ((uint8_t)buff->at(15) << 24);
                binHeader.reserved = (uint8_t)buff->at(16) | ((uint8_t)buff->at(17) << 8) |
                                     ((uint8_t)buff->at(18) << 16) | ((uint8_t)buff->at(19) << 24);
                binHeader.version = (uint8_t)buff->at(20);
                binHeader.leapSec = (uint8_t)buff->at(21);
                binHeader.delayMs = (uint8_t)buff->at(22) | ((uint8_t)buff->at(23) << 8);
                res.binaryHeader = binHeader;
                res.header = buff->left(24);
                res.data = buff->mid(24,MessageLength);
                res.crc = buff->mid(MessageLength+24,4);
                res.isAscii = false;
                // qDebug() << res.header.toHex(' ') << res.data.toHex(' ') << res.crc.toHex(' ');
                buff->remove(0, MessageLength + 24 + 4);
                return res;
            }
        }
        else if ((uint8_t)buff->at(0) == '#'){
            if (buff->left(11) == "#UNILOGLIST"){
                int endPos = buff->indexOf("\r\n\r\n",0);
                if (endPos == -1) break;
                int pos = 1;
                QByteArray header;
                do{
                    header.append((uint8_t)buff->at(pos));
                    pos++;
                } while ((uint8_t)buff->at(pos) != ';');
                QByteArray data;
                pos++;
                do{
                    data.append((uint8_t)buff->at(pos));
                    pos++;
                } while (pos != endPos);
                res.isAscii = true;
                res.isCommand = false;
                res.header = header;
                res.data = data;
                UnicoreMessage::AsciiHeader asciiHeader;
                QList<QByteArray> fields = header.split(',');
                asciiHeader.messageName = QString::fromLatin1(fields.at(0));
                asciiHeader.cpuIdle = static_cast<quint8>(fields.at(1).toUInt(nullptr));
                asciiHeader.timeRef = QString::fromLatin1(fields.at(2));
                asciiHeader.timeStatus = QString::fromLatin1(fields.at(3));
                asciiHeader.wn = static_cast<quint16>(fields.at(4).toUInt(nullptr));
                asciiHeader.ms = fields.at(5).toUInt(nullptr);
                asciiHeader.reserved = fields.at(6).toUInt(nullptr);
                asciiHeader.version = static_cast<quint8>(fields.at(7).toUInt(nullptr));
                asciiHeader.leapSec = static_cast<quint8>(fields.at(8).toUInt(nullptr));
                asciiHeader.delayMs = static_cast<quint16>(fields.at(9).toUInt(nullptr));
                res.asciiHeader = asciiHeader;
                qDebug() << data;
                buff->remove(0, pos);
                return res;
            }
            int astericsPos = buff->lastIndexOf('*');
            int dotcommaPos = buff->indexOf(';',0);
            if (astericsPos == -1 || (astericsPos >= 0 && astericsPos >= (buff->count() - 8)) || (dotcommaPos > astericsPos)) {break;}
            QByteArray header;
            int pos = 1;
            if (dotcommaPos == -1) {buff->remove(0,1); continue;}
            do{
                header.append((uint8_t)buff->at(pos));
                pos++;
            } while ((uint8_t)buff->at(pos) != ';');
            QByteArray data;
            pos++;
            do{
                data.append((uint8_t)buff->at(pos));
                pos++;
            } while ((uint8_t)buff->at(pos) != '*');
            int MessageLength = data.count() + header.count();
            QByteArray CRC;
            int crclen = buff->indexOf('\r') - buff->indexOf('*') - 1;
            qDebug() << buff->indexOf('\r') << buff->indexOf('*') << crclen;
            if (crclen != 2 && crclen != 8) {buff->remove(0,1); continue;}
            for(int i = 0; i<crclen; i++){
                pos++;
                CRC.append((uint8_t)buff->at(pos));
            }
            uint8_t* buffdata = reinterpret_cast<uint8_t*>(buff->data());

            bool crcCheck = crclen == 8?  CRC.toUInt(nullptr,16) == rtk_crc32(buffdata + 1, MessageLength + 1):
                            crclen == 2?  CRC.toUInt(nullptr,16) == calculateCommandChecksum(buffdata, MessageLength + 2):false;
            if (crcCheck){
                res.isAscii = true;
                res.isCommand = true;
                res.header = header;
                res.data = data;
                res.crc = CRC;
                UnicoreMessage::AsciiHeader asciiHeader;
                QList<QByteArray> fields = header.split(',');
                asciiHeader.messageName = QString::fromLatin1(fields.at(0));
                asciiHeader.cpuIdle = static_cast<quint8>(fields.at(1).toUInt(nullptr));
                asciiHeader.timeRef = QString::fromLatin1(fields.at(2));
                asciiHeader.timeStatus = QString::fromLatin1(fields.at(3));
                asciiHeader.wn = static_cast<quint16>(fields.at(4).toUInt(nullptr));
                asciiHeader.ms = fields.at(5).toUInt(nullptr);
                asciiHeader.reserved = fields.at(6).toUInt(nullptr);
                asciiHeader.version = static_cast<quint8>(fields.at(7).toUInt(nullptr));
                asciiHeader.leapSec = static_cast<quint8>(fields.at(8).toUInt(nullptr));
                asciiHeader.delayMs = static_cast<quint16>(fields.at(9).toUInt(nullptr));
                res.asciiHeader = asciiHeader;
                buff->remove(0, pos + 3);
                return res;
            }
            buff->remove(0, pos);
        }
        else if ((uint8_t)buff->at(0) == '$'){
            int astericsPos = buff->lastIndexOf('*');
            if (astericsPos == -1 || (astericsPos >= 0 && astericsPos >= (buff->count() - 2)))  {break;}
            int pos = 1;
            QByteArray data;
            do{
                data.append((uint8_t)buff->at(pos));
                pos++;
            } while ((uint8_t)buff->at(pos) != '*');
            QByteArray CRC;
            int crclen = buff->indexOf('\r') - buff->indexOf('*') - 1;
            if (crclen != 2) {buff->remove(0,1); continue;}
            for(int i = 0; i<2; i++){
                pos++;
                CRC.append((uint8_t)buff->at(pos));
            }
            uint8_t* buffdata = reinterpret_cast<uint8_t*>(buff->data());
            if (CRC.toUInt(nullptr,16) == calculateCommandChecksum(buffdata, data.count() + 1)){
                res.isAscii = true;
                res.isCommand = true;
                res.data = data;
                res.crc = CRC;

                buff->remove(0,pos + 2);
                return res;
            }
            buff->remove(0, pos + 2);
        }

        buff->remove(0,1);
        size = buff->size();
    }

    return res;
}

bool UnicoreParser::sendMessage(QString msg)
{
    msg+= "\r\n";
    connection->write(msg.toLatin1());
    bool res = !connection->flush();
    if (res) qDebug() << "send failed!";
    return res;
}

