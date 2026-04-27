#ifndef CONVERTORS_H
#define CONVERTORS_H

template<typename T>
inline bool getBit(T num, int index){
    int numberOfBits = sizeof(num)*8 - 1;
    if (index > numberOfBits) {
        index = numberOfBits;
    }
    bool res = (bool)((T)(num << (numberOfBits - index)) >> (numberOfBits));
    return res;
}

template<typename T>
inline T getBits(T num, int indexStart, int indexEnd){
    int numberOfBits = sizeof(num)*8 - 1;
    if (indexEnd > numberOfBits) {
        indexEnd = numberOfBits;
    }
    T res = (T)(num << (numberOfBits - indexEnd)) >> (numberOfBits - (indexEnd - indexStart));
    return res;
}

template<typename T>
inline bool compareValue(QDataStream& stream, double thresh, int flags, const QString& typeName) {
    T value;
    stream >> value;
    qDebug() << "data: " << value << "event value: " << thresh;

    switch (flags) {
    case 1: return value == thresh;
    case 2: return value > thresh;
    case 3: return value >= thresh;
    case 4: return value < thresh;
    case 5: return value <= thresh;
    case 6: return value != thresh;
    default: return false;
    }
}


// Специализация для float
template<>
inline bool compareValue<float>(QDataStream& stream, double thresh, int flags, const QString& typeName) {
    float value;
    stream >> value;
    qDebug() << "data: " << value << "event value: " << thresh;

    switch (flags) {
    case 1: return qFuzzyCompare(value, (float)thresh);
    case 2: return value > thresh;
    case 3: return value >= thresh;
    case 4: return value < thresh;
    case 5: return value <= thresh;
    case 6: return !qFuzzyCompare(value, (float)thresh);
    default: return false;
    }
}

// Специализация для double
template<>
inline bool compareValue<double>(QDataStream& stream, double thresh, int flags, const QString& typeName) {
    double value;
    stream >> value;
    qDebug() << "data: " << value << "event value: " << thresh;
    switch (flags) {
    case 1: return qFuzzyCompare(value, thresh);
    case 2: return value > thresh;
    case 3: return value >= thresh;
    case 4: return value < thresh;
    case 5: return value <= thresh;
    case 6: return !qFuzzyCompare(value, thresh);
    default: return false;
    }
}

template<typename T>
inline bool compareBitmap(QDataStream& stream, int startBit, int endBit, unsigned int value) {
    T bits;
    stream >> bits;
    qDebug() << "data: " << bits << "event value: " << value;

    return getBits(bits, startBit, endBit) == value;
}

template<typename T>
inline T getValueFromQByteArray(QByteArray data, QDataStream::ByteOrder order = QDataStream::BigEndian){
    T value;
    QDataStream stream(data);
    stream.setByteOrder(order);
    stream >> value;
    return value;
}

inline double getFieldValue(QByteArray data, QString type, int size, QDataStream::ByteOrder order){

    double res;
    if      (type == "int" && size == 1)  res = (double)getValueFromQByteArray<int8_t>(data, order);
    else if (type == "uint" && size == 1) res = (double)getValueFromQByteArray<uint8_t>(data, order);
    else if (type == "int" && size == 2)  res = (double)getValueFromQByteArray<int16_t>(data, order);
    else if (type == "uint" && size == 2) res = (double)getValueFromQByteArray<uint16_t>(data, order);
    else if (type == "int" && size == 4)  res = (double)getValueFromQByteArray<int32_t>(data, order);
    else if (type == "uint" && size == 4) res = (double)getValueFromQByteArray<uint32_t>(data, order);
    else if (type == "int" && size == 8)  res = (double)getValueFromQByteArray<int64_t> (data, order);
    else if (type == "uint" && size == 8) res = (double)getValueFromQByteArray<int64_t>(data, order);
    else if (type == "float") res = (double)getValueFromQByteArray<float>(data, order);
    else if (type == "double") res = getValueFromQByteArray<double>(data, order);
    else res = 0;
    return res;
}


#endif // CONVERTORS_H
