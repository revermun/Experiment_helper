#pragma once

#ifndef UBLOX_H
#define UBLOX_H

#include <QVector>
typedef uint8_t U1;
typedef uint16_t U2;
typedef uint32_t U4;
typedef int8_t I1;
typedef int16_t I2;
typedef int32_t I4;
typedef uint8_t X1;
typedef uint16_t X2;
typedef uint32_t X4;
typedef float R4;
typedef double R8;
typedef char CH[6];

struct CFG
{
    struct DAT;
    struct _CFG;
    struct DGNSS;
    struct GNSS;
    struct MSG;
    struct NAV5;
    struct PRT;
    struct RATE;
    struct RST;
    struct ITFM;
};


struct NAV{
    struct CLOCK;
    struct DOP;
    struct ORB;
    struct DGPS;
    struct POSECEF;
    struct POSLLH;
    struct RELPOSNED;
    struct SAT;
    struct SOL;
    struct STATUS;
    struct VELECEF;
    struct VELNED;
};


struct RXM{
    struct RAWX;
    struct SFRBX;
};

struct Message {
    virtual ~Message() = default;
};

struct ACK{
    struct _ACK;
    struct NAK;
};

struct ACK::_ACK{
    inline static U1 classID = 0x05;
    inline static U1 messageID = 0x01;
};

struct ACK::NAK{
    inline static U1 classID = 0x05;
    inline static U1 messageID = 0x00;
};

template<class T>
class _iterator {
public:
    _iterator<T>(char* data, size_t size) :size(size), data(data), i(0) {}
    _iterator<T>(T& msg, size_t size) :size(size), data((char*)(&msg)), i(0) {}
    bool end() const {
        return i >= size;
    }
    void next() {
        i += sizeof(T);
    }
    T& operator*() const { return *((T*)(data+i)); }
    T* operator->() const { return (T*)(data+i); }
    operator char*() const { return data+i; }
private:
    size_t i, size;
    char* data;
};

/* GNSS system configuration. §31.11.10.
 */
struct CFG::GNSS : public Message
{
    U1 msgVer;
    U1 numTrkChHw;
    U1 numTrkChUse;
    U1 numConfigBlocks;

    struct Repeated {
        U1 gnssId;
        U1 resTrkCh;
        U1 maxTrkCh;
        U1 reserved;
        X4 flags;
    };

    QVector<CFG::GNSS::Repeated> repeatedList;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x3E;
};

struct CFG::DAT : public Message
{
    struct SET;
    struct GET;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x06;
};
struct CFG::DAT::SET : public Message
{
    R8 majA;
    R8 flat;
    R4 dX;
    R4 dY;
    R4 dZ;
    R4 rotX;
    R4 rotY;
    R4 rotZ;
    R4 scale;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x06;
};
///TODO: V
struct CFG::DAT::GET : public Message
{
    U2 datumNum;
    U4 datumName1;
    U2 datumName2;
    R8 majA;
    R8 flat;
    R4 dX;
    R4 dY;
    R4 dZ;
    R4 rotX;
    R4 rotY;
    R4 rotZ;
    R4 scale;
    
    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x06;
};

struct CFG::_CFG : public Message
{
    U4 clearMask;
    U4 saveMask;
    U4 loadMask;
    U1 deviceMask;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x09;
};

struct CFG::DGNSS : public Message
{
    U1 dgnssMode;
    U1 reserved1;
    U1 reserved2;
    U1 reserved3;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x70;
};

/// У UBX-CFG-MSG три варинта, отличающихся по длинне
struct CFG::MSG{
    struct MSGPOLL;
    struct MSGRATE;
    struct MSGRATES;

    U1 msgClass;
    U1 msgID;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x01;
};

struct CFG::MSG::MSGPOLL : public Message
{
    U1 msgClass;
    U1 msgID;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x01;
};

struct CFG::MSG::MSGRATE : public Message
{
    U1 msgClass;
    U1 msgID;
    U1 rate;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x01;
};

struct CFG::MSG::MSGRATES : public Message
{
    U1 msgClass;
    U1 msgID;
    U1 rate1;
    U1 rate2;
    U1 rate3;
    U1 rate4;
    U1 rate5;
    U1 rate6;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x01;
};

struct CFG::NAV5 : public Message
{
    X2 mask;
    U1 dynModel;
    U1 fixMode;
    I4 fixedAlt;
    U4 fixedAltVar;
    I1 minElev;
    U1 drLimit;
    U2 pDop;
    U2 tDop;
    U2 pAcc;
    U2 tAcc;
    U1 staticHoldThresh;
    U1 dgnssTimeout;
    U1 cnoThreshNumSVs;
    U1 cnoThresh;
    U1 reserved1;
    U1 reserved2;
    U2 staticHoldMaxDist;
    U1 utcStandard;
    U1 reserved3;
    U1 reserved4;
    U1 reserved5;
    U1 reserved6;
    U1 reserved7;


    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x24;
};

struct CFG::PRT : public Message
{
    U1 portID;
    struct CFG::PRT::USB;
    struct CFG::PRT::UART;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x00;
};

struct CFG::PRT::USB : public Message
{
    U1 portID;
    U1 reserved1;
    X2 txReady;
    U4 reserved2;
    U4 reserved3;
    X2 inProtoMask;
    X2 outProtoMask;
    U4 reserved4;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x00;
};

struct CFG::PRT::UART : public Message
{
    U1 portID;
    U1 reserved1;
    X2 txReady;
    X4 mode;
    U4 baudRate;
    X2 inProtoMask;
    X2 outProtoMask;
    X2 flags;
    U2 reserved2;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x00;
};

struct CFG::RATE : public Message
{
    U2 measRate;
    U2 navRate;
    U2 timeRef;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x08;
};

struct CFG::RST : public Message
{
    X2 navBbrMask;
    U1 resetMode;
    U1 reserved1;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x04;
};

struct CFG::ITFM : public Message
{
    X4 config;
    X4 config2;

    inline static U1 classID = 0x06;
    inline static U1 messageID = 0x39;
};

struct NAV::CLOCK : public Message
{
    U4 iTOW;
    I4 clkB;
    I4 clkD;
    U4 tAcc;
    U4 fAcc;

    inline static U1 classID = 0x01;
    inline static U1 messageID = 0x22;
};

struct NAV::DOP : public Message
{
    U4 iTOW;
    U2 gDOP;
    U2 pDOP;
    U2 tDOP;
    U2 vDOP;
    U2 hDOP;
    U2 nDOP;
    U2 eDOP;

    inline static U1 classID = 0x01;
    inline static U1 messageID = 0x04;
};

struct NAV::ORB : public Message
{
    U4 iTOW;
    U1 version;
    U1 numSv;
    U1 reserved1;
    U1 reserved2;

    struct Repeated {
        U1 gnssId;
        U1 svId;
        X1 svFlag;
        X1 eph;
        X1 alm;
        X1 otherOrb;
    };

    QVector<NAV::ORB::Repeated> repeatedList;

    inline static U1 classID = 0x01;
    inline static U1 messageID = 0x34;
};

struct NAV::DGPS : public Message
{
    U4 iTOW;
    I4 age;
    I2 baseId;
    I2 baseHealth;
    U1 numCh;
    U1 status;
    U1 reserved1;
    U1 reserved2;

    struct Repeated {
        U1 svId;
        X1 flags;
        U2 ageC;
        R4 prc;
        R4 prrc;
    };

    QVector<NAV::DGPS::Repeated> repeatedList;

    inline static U1 classID = 0x01;
    inline static U1 messageID = 0x31;
};

struct NAV::POSECEF : public Message
{
    U4 iTOW;
    I4 ecefX;
    I4 ecefY;
    I4 ecefZ;
    U4 pAcc;

    inline static U1 classID = 0x01;
    inline static U1 messageID = 0x01;
};

struct NAV::POSLLH : public Message
{
    U4 iTOW;
    I4 lon;
    I4 lat;
    I4 height;
    I4 hMSL;
    U4 hAcc;
    U4 vAcc;

    inline static U1 classID = 0x01;
    inline static U1 messageID = 0x02;
};

struct NAV::RELPOSNED : public Message
{
    U1 version;
    U1 reserved1;
    U2 refStationId;
    U4 iTOW;
    I4 relPosN;
    I4 relPosE;
    I4 relPosD;
    I4 relPosLength;
    I4 relPosHeading;
    U1 reserved2;
    U1 reserved3;
    U1 reserved4;
    U1 reserved5;
    I1 relPosHPN;
    I1 relPosHPE;
    I1 relPosHPD;
    I1 relPosHPLength;
    U4 accN;
    U4 accE;
    U4 accD;
    U4 accLength;
    U4 accHeading;
    U1 reserved6;
    U1 reserved7;
    U1 reserved8;
    U1 reserved9;
    X4 flags;

    inline static U1 classID = 0x01;
    inline static U1 messageID = 0x3C;
};

struct NAV::SAT : public Message
{
    U4 iTOW;
    U1 version;
    U1 numSvs;
    U1 reserved1;
    U1 reserved2;

    struct Repeated {
        U1 gnssId;
        U1 svId;
        U1 cno;
        I1 elev;
        I2 azim;
        I2 prRes;
        X4 flags;
    };

    QVector<NAV::SAT::Repeated> repeatedList;

    inline static U1 classID = 0x01;
    inline static U1 messageID = 0x35;
};

struct NAV::SOL : public Message
{
    U4 iTOW;
    I4 fTOW;
    I2 week;
    U1 gpsFix;
    X1 flags;
    I4 ecefX;
    I4 ecefY;
    I4 ecefZ;
    U4 pAcc;
    I4 ecefVX;
    I4 ecefVY;
    I4 ecefVZ;
    U4 sAcc;
    U2 pDOP;
    U1 reserved1;
    U1 numSV;
    U4 reserved;
    

    inline static U1 classID = 0x01;
    inline static U1 messageID = 0x06;
};

struct NAV::STATUS : public Message
{
    U4 iTOW;
    U1 gpsFix;
    X1 flags;
    X1 fixStat;
    X1 flags2;
    U4 ttff;
    U4 msss;

    inline static U1 classID = 0x01;
    inline static U1 messageID = 0x03;
};

struct NAV::VELECEF : public Message
{
    U4 iTOW;
    I4 ecefVX;
    I4 ecefVY;
    I4 ecefVZ;
    U4 sAcc;

    inline static U1 classID = 0x01;
    inline static U1 messageID = 0x11;
};

struct NAV::VELNED : public Message
{
    U4 iTOW;
    I4 velN;
    I4 velE;
    I4 velD;
    U4 speed;
    U4 gSpeed;
    I4 heading;
    U4 sAcc;
    U4 cAcc;

    inline static U1 classID = 0x01;
    inline static U1 messageID = 0x12;
};


struct RXM::RAWX : public Message
{
    R8 rcvTow;
    U2 week;
    I1 leapS;
    U1 numMeas;
    X1 recStat;
    U1 reserved1;
    U1 reserved2;
    U1 reserved3;

    struct Repeated {
        R8 prMes;
        R8 cpMes;
        R4 doMes;
        U1 gnssId;
        U1 svId;
        U1 reserved1;
        U1 freqId;
        U2 locktime;
        U1 cno;
        X1 prStdev;
        X1 cpStdev;
        X1 doStdev;
        X1 trkStat;
        U1 reserved2;
    };

    QVector<RXM::RAWX::Repeated> repeatedList;

    inline static U1 classID = 0x02;
    inline static U1 messageID = 0x15;
};

struct RXM::SFRBX : public Message
{
    U1 gnssId;
    U1 svId;
    U1 reserved1;
    U1 freqId;
    U1 numWords;
    U1 reserved2;
    U1 version;
    U1 reserved3;

    struct Repeated {
        U4 dwrd;
    };

    QVector<RXM::SFRBX::Repeated> repeatedList;

    inline static U1 classID = 0x02;
    inline static U1 messageID = 0x13;
};

#endif // UBLOX_H
