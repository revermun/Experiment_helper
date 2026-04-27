#include "ubloxparser.h"

UbloxParser::UbloxParser(QObject* connection) {
    this->connection = connection;
    // this->connection->open(QSerialPort::ReadWrite);
}

UbloxParser::~UbloxParser(){
    // this->connection->close();
}
/// Довольно часто CheckSum расчитанный по этой функции не совпадает с CheckSum в сообщении.
/// У меня проблема в расчетах или Ublox часто выдает сообщения с неправильной checkSum?
QByteArray UbloxParser::calcCheckSum(QByteArray msg){
    uint8_t CK_A = 0;
    uint8_t CK_B = 0;
    for(int i = 0; i<msg.count(); i++){
        CK_A += (uint8_t)msg.at(i);
        CK_B += CK_A;
    }
    QByteArray res;
    res.resize(2);
    res[0] = CK_A;
    res[1] = CK_B;
    return res;
}

QByteArray UbloxParser::createPollMessage(uint8_t byteClass, uint8_t byteID){
    QByteArray hdr;
    hdr.resize(2);
    hdr[0] = 0xb5;
    hdr[1] = 0x62;
    QByteArray en;
    en.resize(4);
    en[0] = byteClass;
    en[1] = byteID;
    en[2] = 0x00;
    en[3] = 0x00;
    QByteArray msg = hdr + en;
    QByteArray checkSum = UbloxParser::calcCheckSum(en);
    msg.append(checkSum);
    return msg;
}

UbloxMessage UbloxParser::parseMessage(QByteArray *buff)
{
    UbloxMessage res;

    int index = 0;
    int size = buff->size();

    while (index <= size - 8) { // Минимум 8 байт: 2(синхро) + 4(заг) + 2(CRC)
        // Поиск синхро-байтов
        if ((uint8_t)buff->at(index) != 0xb5) {
            index++;
            continue;
        }

        if ((uint8_t)buff->at(index + 1) != 0x62) {
            index += 2;
            continue;
        }

        // Читаем длину payload
        uint16_t payloadLen = (uint8_t)buff->at(index + 4) | ((uint8_t)buff->at(index + 5) << 8);
        int totalMsgLen = 2 + 4 + payloadLen + 2; // синхро(2) + заг(4) + payload + CRC(2)

        // Проверяем, хватает ли данных
        if (index + totalMsgLen > size) {
            // Неполное сообщение - оставляем в буфере
            break;
        }

        // Извлекаем компоненты сообщения
        uint8_t byteClass = (uint8_t)buff->at(index + 2);
        uint8_t byteID = (uint8_t)buff->at(index + 3);

        QByteArray data;
        if (payloadLen > 0) {
            data = buff->mid(index + 6, payloadLen);
        }

        QByteArray checkSum = buff->mid(index + 6 + payloadLen, 2);
        QByteArray msg = buff->mid(index + 2, 4 + payloadLen); // без синхро-байтов

        // Проверка контрольной суммы
        if (calcCheckSum(msg) == checkSum) {
            // Валидное сообщение найдено
            res.messId = byteID;
            res.messClass = byteClass;
            res.messLen = payloadLen;
            res.header = buff->mid(index, 6);
            res.data = data;
            res.crc = checkSum;

            // Удаляем обработанные данные из буфера (ВКЛЮЧАЯ текущее сообщение)
            buff->remove(0, index + totalMsgLen);

            return res;
        }

        // Если CRC не совпал, продолжаем поиск со следующего байта
        index++;
    }

    // Если дошли сюда - валидное сообщение не найдено
    // Но нужно удалить все байты до index (мусорные данные)
    if (index > 0) {
        buff->remove(0, index);
    }

    return res; // пустая карта
}


bool UbloxParser::sendMessage(QByteArray msg)
{
    if (qobject_cast<QSerialPort*>(connection)){
        QSerialPort* serialCon = qobject_cast<QSerialPort*>(connection);
        serialCon->write(msg);
        bool res = serialCon->flush();
        if (!res) qDebug() << "send failed!";
        return res;
    }
    else if (qobject_cast<QTcpSocket*>(connection)){
        QTcpSocket* tcpCon = qobject_cast<QTcpSocket*>(connection);
        tcpCon->write(msg);
        bool res = tcpCon->flush();
        if (!res) qDebug() << "send failed!";
        return res;
    }
    else if (qobject_cast<QIODevice*>(connection)){
        QIODevice* ioCon = qobject_cast<QIODevice*>(connection);
        bool res = ioCon->write(msg);
        qDebug() << ioCon->write(msg);
        return res;
    }
    else{
        qDebug() << "uncknown connection type";
        return false;
    }
}


Message* UbloxParser::decode(UbloxMessage msg)
{
    uint8_t byteClass = msg.messClass;
    uint8_t byteID = msg.messId;
    if (byteClass == CFG::MSG::classID && byteID == CFG::MSG::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        CFG::MSG::MSGRATES* res = new CFG::MSG::MSGRATES();

        stream >> res->msgClass >> res->msgID >> res->rate1 >> res->rate2 >> res->rate3 >> res->rate4 >> res->rate5 >> res->rate6;
        
        return res;
    }
    if (byteClass == CFG::DAT::GET::classID && byteID == CFG::DAT::GET::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        CFG::DAT::GET* res = new CFG::DAT::GET();

        stream >> res->datumNum >> res->datumName1 >> res->datumName2 >> res->majA >> res->flat >> res->dX >> res->dY >> res->dZ >> res->rotX >> res->rotY >> res->rotZ >> res->scale;

        return res;
    }
    else if (byteClass == CFG::GNSS::classID && byteID == CFG::GNSS::messageID){
        
        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        CFG::GNSS* res = new CFG::GNSS();
        
        stream >> res->msgVer >> res->numTrkChHw >> res->numTrkChUse >> res->numConfigBlocks;
        
        res->repeatedList.reserve(res->numConfigBlocks);
        
        for (int i = 0; i < res->numConfigBlocks; ++i) {
            CFG::GNSS::Repeated rep;
            stream >> rep.gnssId;
            stream >> rep.resTrkCh;
            stream >> rep.maxTrkCh;
            stream >> rep.reserved;
            stream >> rep.flags;
            res->repeatedList.append(rep);
        }
        
        return res;
    }
    else if (byteClass == CFG::_CFG::classID && byteID == CFG::_CFG::messageID){
        
        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        CFG::_CFG* res = new CFG::_CFG();
        
        stream >> res->clearMask >> res->saveMask >> res->loadMask >> res->deviceMask;
        
        return res;
    }
    else if (byteClass == CFG::DGNSS::classID && byteID == CFG::DGNSS::messageID){
        
        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        CFG::DGNSS* res = new CFG::DGNSS();
        
        stream >> res->dgnssMode >> res->reserved1 >> res->reserved2 >> res->reserved3;
        
        return res;
    }
    else if (byteClass == CFG::DAT::SET::classID && byteID == CFG::DAT::SET::messageID){
        
        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        CFG::DAT::SET* res = new CFG::DAT::SET();
        
        stream >> res->majA >> res->flat >> res->dX >> res->dY >> res->dZ >> res->rotX >> res->rotY >> res->rotZ >> res->scale;
        
        return res;
    }
    else if (byteClass == CFG::NAV5::classID && byteID == CFG::NAV5::messageID){
        
        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        CFG::NAV5* res = new CFG::NAV5();
        
        stream >> res->mask >> res->dynModel >> res->fixMode >> res->fixedAlt >> res->fixedAltVar >> res->minElev
               >> res->drLimit >> res->pDop >> res->tDop >> res->pAcc >> res->tAcc >> res->staticHoldThresh >> res->dgnssTimeout
               >> res->cnoThreshNumSVs >> res->cnoThresh >> res->reserved1 >> res->reserved2 >> res->staticHoldMaxDist >> res->utcStandard
               >> res->reserved3 >> res->reserved4 >> res->reserved5 >> res->reserved6 >> res->reserved7;
        
        return res;
    }
    else if (byteClass == CFG::PRT::classID && byteID == CFG::PRT::messageID){
        
        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);
        CFG::PRT* mes = new CFG::PRT();
        U1 portID;
        stream >> portID;
        mes->portID = portID;
        if (portID == 1 || portID == 2){
            CFG::PRT::UART* res = new CFG::PRT::UART();
            res->portID = portID;
            stream >> res->reserved1 >> res->txReady >> res->mode >> res->baudRate >> res->inProtoMask
                >> res->outProtoMask >> res->flags >> res->reserved2;
            return res;
        }
        else if(portID == 3){
            CFG::PRT::USB* res = new CFG::PRT::USB();
            res->portID = portID;
            stream >> res->reserved1 >> res->txReady >> res->reserved2 >> res->reserved3
                >> res->inProtoMask >> res->outProtoMask >> res->reserved4;
            return res;
        }
        return mes;
    }
    else if (byteClass == CFG::RATE::classID && byteID == CFG::RATE::messageID){
        
        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        CFG::RATE* res = new CFG::RATE();
        
        stream >> res->measRate >> res->navRate >> res->timeRef;
        
        return res;
    }
    else if (byteClass == CFG::RST::classID && byteID == CFG::RST::messageID){
        
        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        CFG::RST* res = new CFG::RST();
        
        stream >> res->navBbrMask >> res->resetMode >> res->reserved1;
        
        return res;
    }
    else if (byteClass == CFG::ITFM::classID && byteID == CFG::ITFM::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        CFG::ITFM* res = new CFG::ITFM();

        stream >> res->config >>res->config2;

        return res;
    }
    else if (byteClass == NAV::POSECEF::classID && byteID == NAV::POSECEF::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        NAV::POSECEF* res = new NAV::POSECEF();

        stream >> res->iTOW >> res->ecefX >> res->ecefY >> res->ecefZ >> res->pAcc;

        return res;
    }
    else if (byteClass == NAV::CLOCK::classID && byteID == NAV::CLOCK::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        NAV::CLOCK* res = new NAV::CLOCK();

        stream >> res->iTOW >> res->clkB >> res->clkD >> res->tAcc >> res->fAcc;

        return res;
    }
    else if (byteClass == NAV::DOP::classID && byteID == NAV::DOP::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        NAV::DOP* res = new NAV::DOP();

        stream >> res->iTOW >> res->gDOP >> res->pDOP >> res->tDOP >> res->vDOP >> res->hDOP >> res->nDOP >> res->eDOP;

        return res;
    }
    else if (byteClass == NAV::ORB::classID && byteID == NAV::ORB::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        NAV::ORB* res = new NAV::ORB();

        stream >> res->iTOW >> res->version >> res->numSv >> res->reserved1 >> res->reserved2;

        int maxPossibleSv = (payload.size() - sizeof(NAV::ORB)) / sizeof(NAV::ORB::Repeated);
        if (res->numSv > maxPossibleSv) {
            qWarning() << "Invalid numSv:" << res->numSv << "max possible:" << maxPossibleSv;
            res->numSv = maxPossibleSv;
        }


        res->repeatedList.reserve(res->numSv);

        for (int i = 0; i < res->numSv; ++i) {
            NAV::ORB::Repeated rep;
            stream >> rep.gnssId;
            stream >> rep.svId;
            stream >> rep.svFlag;
            stream >> rep.eph;
            stream >> rep.alm;
            stream >> rep.otherOrb;
            res->repeatedList.append(rep);
        }
        return res;
    }
    else if (byteClass == NAV::DGPS::classID && byteID == NAV::DGPS::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        NAV::DGPS* res = new NAV::DGPS();

        stream >> res->iTOW >> res->age >> res->baseId >> res->baseHealth >> res->numCh >> res->status >> res->reserved1 >> res->reserved2;

        int maxPossibleCh = (payload.size() - sizeof(NAV::DGPS)) / sizeof(NAV::DGPS::Repeated);
        if (res->numCh > maxPossibleCh) {
            qWarning() << "Invalid numCh:" << res->numCh << "max possible:" << maxPossibleCh;
            res->numCh = maxPossibleCh;
        }

        res->repeatedList.reserve(res->numCh);

        for (int i = 0; i < res->numCh; ++i) {
            NAV::DGPS::Repeated rep;
            stream >> rep.svId;
            stream >> rep.flags;
            stream >> rep.ageC;
            stream >> rep.prc;
            stream >> rep.prrc;
            res->repeatedList.append(rep);
        }

        return res;
    }
    else if (byteClass == NAV::POSLLH::classID && byteID == NAV::POSLLH::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        NAV::POSLLH* res = new NAV::POSLLH();

        stream >> res->iTOW >> res->lon >> res->lat >> res->height >> res->hMSL >> res->hAcc >> res->vAcc;

        return res;
    }
    else if (byteClass == NAV::RELPOSNED::classID && byteID == NAV::RELPOSNED::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        NAV::RELPOSNED* res = new NAV::RELPOSNED();

        stream >> res->version >> res->reserved1 >> res->refStationId >> res->iTOW >> res->relPosN >> res->relPosE >> res->relPosD >> res->relPosLength
               >> res->relPosHeading >> res->reserved2 >> res->reserved3 >> res->reserved4 >> res->reserved5 >> res->relPosHPN >> res->relPosHPE
               >> res->relPosHPD >> res->relPosHPLength >> res->accN >> res->accE >> res->accD >> res->accLength >> res->accHeading >> res->reserved6
               >> res->reserved7 >> res->reserved8 >> res->reserved9 >> res->flags;

        return res;
    }
    else if (byteClass == NAV::SAT::classID && byteID == NAV::SAT::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        NAV::SAT* res = new NAV::SAT();

        stream >> res->iTOW >> res->version >> res->numSvs >> res->reserved1 >> res->reserved2;

        // int maxPossibleSv = (payload.size() - sizeof(NAV::SAT)) / sizeof(NAV::SAT::Repeated);
        // if (res->numSvs > maxPossibleSv) {
        //     qWarning() << "Invalid numCh:" << res->numSvs << "max possible:" << maxPossibleSv;
        //     res->numSvs = maxPossibleSv;
        // }

        res->repeatedList.reserve(res->numSvs);

        for (int i = 0; i < res->numSvs; ++i) {
            NAV::SAT::Repeated rep;
            stream >> rep.gnssId >> rep.svId >> rep.cno >> rep.elev >> rep.azim >> rep.prRes >> rep.flags;
            res->repeatedList.append(rep);
        }

        return res;
    }
    else if (byteClass == NAV::SOL::classID && byteID == NAV::SOL::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        NAV::SOL* res = new NAV::SOL();

        stream >> res->iTOW >> res->fTOW >> res->week >> res->gpsFix >> res->flags >> res->ecefX >> res->ecefY >> res->ecefZ
               >> res->pAcc >> res->ecefVX >> res->ecefVY >> res->ecefVZ >> res->sAcc >> res->pDOP >> res->reserved1 >> res->numSV >> res->reserved;

        return res;
    }
    else if (byteClass == NAV::STATUS::classID && byteID == NAV::STATUS::messageID){
        
        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        NAV::STATUS* res = new NAV::STATUS();
        
        stream >> res->iTOW >> res->gpsFix >> res->flags >> res->fixStat >> res->flags2 >> res->ttff >> res->msss;
        
        return res;
    }
    else if (byteClass == NAV::VELECEF::classID && byteID == NAV::VELECEF::messageID){
        
        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        NAV::VELECEF* res = new NAV::VELECEF();
        
        stream >> res->iTOW >> res->ecefVX >> res->ecefVY >> res->ecefVZ >> res->sAcc;
        
        return res;
    }
    else if (byteClass == NAV::VELNED::classID && byteID == NAV::VELNED::messageID){
        
        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);
        
        NAV::VELNED* res = new NAV::VELNED();
        
        stream >> res->iTOW >> res->velN >> res->velE >> res->velD >> res->speed >> res->gSpeed >> res->heading >> res->sAcc >> res->cAcc;
        
        return res;
    }
    else if (byteClass == RXM::RAWX::classID && byteID == RXM::RAWX::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        RXM::RAWX* res = new RXM::RAWX();

        stream >> res->rcvTow >> res->week >> res->leapS >> res->numMeas >> res->recStat >> res->reserved1 >> res->reserved2 >> res->reserved3;

        res->repeatedList.reserve(res->numMeas);

        for (int i = 0; i < res->numMeas; ++i) {
            RXM::RAWX::Repeated rep;
            stream >> rep.prMes >> rep.cpMes >> rep.doMes >> rep.gnssId >> rep.svId >> rep.reserved1 >> rep.freqId >> rep.locktime
                >> rep.cno >> rep.prStdev >> rep.cpStdev >> rep.doStdev >> rep.trkStat >> rep.reserved2;
            res->repeatedList.append(rep);
        }

        return res;
    }
    else if (byteClass == RXM::SFRBX::classID && byteID == RXM::SFRBX::messageID){

        QByteArray payload = msg.data;
        QDataStream stream(payload);
        stream.setByteOrder(QDataStream::LittleEndian);

        RXM::SFRBX* res = new RXM::SFRBX();

        stream >> res->gnssId >> res->svId >> res->reserved1 >> res->freqId >> res->numWords >> res->reserved2 >> res->version >> res->reserved3;

        res->repeatedList.reserve(res->numWords);

        for (int i = 0; i < res->numWords; ++i) {
            RXM::SFRBX::Repeated rep;
            stream >> rep.dwrd;
            res->repeatedList.append(rep);
        }

        return res;
    }
    else{
        Message* res = new Message();
        return res;
    }
}
