#ifndef STRUCTS_H
#define STRUCTS_H

#include <QString>
#include <QMap>
#include <QDebug>
#include <QGroupBox>
#include <QTableWidget>

struct DeviceInfo{
    QString ID;
    QString deviceType;
    QString protocol;
    QString connType;
    struct SerialInfo{
        QString port;
        int baudrate;
        int dataBits;
        QString parity;
        int stopBits;
        QString tcpPort;
        int tcpCount;
    }serialInfo;
    struct CANInfo{
        int baudrate;
        QString type;
    }canInfo;
    struct TCPInfo{
        QString clientServer;
        QString adress;
        QString port;
    }tcpInfo;
};

struct NewData{
    DeviceInfo deviceInfo;
    QByteArray buff;
};

struct TableConnectionsFields{
    QTableWidgetItem* ID;
    QTableWidgetItem* connectionType;
    QTableWidgetItem* TCPPort;
    QTableWidgetItem* onOff;
    QTableWidgetItem* data;
    int dataTimer;
    int row;
    TableConnectionsFields() : dataTimer(0) {}
};

struct EventData{
    QString name;
    QString device;
    QString protocol;
    QString message;
    QString messageId;
    QString fieldName;
    int field;
    QString fieldType;
    QString text;
    struct IntTriggers{
        bool isGreater;
        bool isLesser;
        bool isEqual;
        int threshhold;
    }intTriggers;
    struct BitmapTriggers{
        int startBit;
        int endBit;
        uint bitValue;
        bool isEqual;
    }bitmapTriggers;
    struct CharTriggers{
        QString charValue;
    }charTriggers;
    int status;

    void setStandartText(){
        if (fieldType == "int" || fieldType == "uint" || fieldType == "float" || fieldType == "double"){
            int flags = (int)intTriggers.isEqual +
                        ((int)intTriggers.isGreater << 1) +
                        ((int)intTriggers.isLesser << 2);
            QString compType;
            switch (flags) {
            case 1: compType = "равенства";
            case 2: compType = "превышения";
            case 3: compType = "превышения или равенства";
            case 4: compType = "понижения";
            case 5: compType = "понижения или равенства";
            case 6: compType = "неравенства";
            default: compType = "";
            }
            text = QString("Событие %1 пар-ра %2 значения %3")
                             .arg(compType)
                             .arg(fieldName)
                             .arg(intTriggers.threshhold);
        }
        else if (fieldType == "bitmap"){
            QString compType;
            if (bitmapTriggers.isEqual) compType = "равенства";
            else compType = "неравенства";
            text = QString("Событие %1 бит [%2:%3] параметра %4 значению %5")
                             .arg(compType)
                             .arg(bitmapTriggers.startBit)
                             .arg(bitmapTriggers.endBit)
                             .arg(fieldName)
                             .arg(bitmapTriggers.bitValue);
        }
        else if (fieldType == "char"){
            text = QString("Событие равенства параметра %1 значению %2")
                             .arg(fieldName)
                             .arg(charTriggers.charValue);
        }
    }
};

struct Mess{
    QString name;
    QString description;
    QString id;
    QString type;
    QString protocol;
    struct Field{
        QString name;
        QString full_name;
        int index;
        QString type;
        bool isRepeated;
        int size;
        int offset;
        int min_value;
        int max_value;
        QString units;
        double scale;
    };
    QMap<QString,Field> fields;


    QStringList getSortedFieldKeys() const {
        QList<QPair<int, QString>> indexedKeys;

        for (auto it = fields.begin(); it != fields.end(); ++it) {
            indexedKeys.append(qMakePair(it.value().index, it.key()));
        }
        std::sort(indexedKeys.begin(), indexedKeys.end(),
                  [](const QPair<int, QString>& a, const QPair<int, QString>& b) {
                      return a.first < b.first;
                  });

        QStringList result;
        for (const auto& pair : indexedKeys) {
            if (pair.second.isEmpty()) continue;
            result.append(pair.second);
        }
        return result;
    }
};

struct ExperimentDeviceInfo
{
    QString id;
    QString model;
    QString type;
    QStringList outputs;
    struct IMUInfo{
        double instabBias;
        double randomWalk;
        double initialError;
    }imuInfo;
    struct CameraInfo{
        int fps;
        int width;
        int heigt;
    }cameraInfo;
    struct AntennaInfo{
        QString confFileDirectory;
    }antennaInfo;
};

struct ExperimentConnectionInfo
{
    QString device1;
    QString device2;
    QString port1;
    QString port2;
    QString connectionType;
    struct SerialInfo{
        int baudrate;
        QString parity;
        int dataBits;
        int stopBits;
    }serialInfo;
    struct CANInfo{
        int baudrate;
    }canInfo;
    struct CoaxialCableInfo{
        QString length;
        QString material;
        int dataLoss;
    }coaxCableInfo;
    struct TCPInfo{
        QString adress;
        QString port;
    }tcpInfo;
};

struct ExperimentGroupBoxParametersInfo
{
    QString deviceName;
    QGroupBox *group;
    int row;
    int column;
    bool positionCheck;
    bool boresightCheck;
    bool leverarmCheck;
};

#endif // STRUCTS_H
