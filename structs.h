#ifndef STRUCTS_H
#define STRUCTS_H

#include <QString>
#include <QMap>
#include <QDebug>

struct tableConnectionsFields{
    QString ID;
    QString connectionType;
    QString TCPPort;
    int onOff;
    int data;
};

struct eventData{
    QString name;
    QString device;
    QString protocol;
    QString message;
    int messageId;
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
    }bitmapTriggers;
    struct CharTriggers{
        QString charValue;
    }charTriggers;
    int status;
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
        int size;
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

#endif // STRUCTS_H
