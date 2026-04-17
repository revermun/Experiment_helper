#ifndef STRUCTS_H
#define STRUCTS_H

#include <QString>

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
    QString fieldName;
    int field;
    QString text;
    struct Triggers{
        bool isGreater;
        bool isLesser;
        bool isEqual;
        int threshhold;
    }triggers;
};

#endif // STRUCTS_H
