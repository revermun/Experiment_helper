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


#endif // CONVERTORS_H
