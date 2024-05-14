#ifndef EZ_PROTOCOL_SERIALIZER
#define EZ_PROTOCOL_SERIALIZER

#include <map>
#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include <type_traits>

namespace ez {

class protocol_serializer
{
public:
    enum class P_BYTE_ORDER {
        P_BIG_ENDIAN,
        P_LITTLE_ENDIAN
    };

    enum class BUFFER_SOURCE {
        INTERNAL_BUFFER,
        EXTERNAL_BUFFER
    };

    enum class BASE {
        HEX,
        DEC,
        OCT,
        BIN
    };

    enum class ASSOCIATED_TYPE {
        SIGNED_INTEGER,
        UNSIGNED_INTEGER,
        FLOATING_POINT
    };

    struct field {
        std::string name;
        unsigned int bitCount;
        ASSOCIATED_TYPE associatedType = ASSOCIATED_TYPE::UNSIGNED_INTEGER;
    };

    protocol_serializer(const P_BYTE_ORDER byteOrder = P_BYTE_ORDER::P_BIG_ENDIAN,
                        const BUFFER_SOURCE bufferSource = BUFFER_SOURCE::INTERNAL_BUFFER,
                        unsigned char* const externalBuffer = nullptr);

    protocol_serializer(const std::vector<field>& fields,
                        const P_BYTE_ORDER byteOrder = P_BYTE_ORDER::P_BIG_ENDIAN,
                        const BUFFER_SOURCE bufferSource = BUFFER_SOURCE::INTERNAL_BUFFER,
                        unsigned char* const externalBuffer = nullptr);

    protocol_serializer(const protocol_serializer& wrapper);
    protocol_serializer& operator=(const protocol_serializer& wrapper);

    protocol_serializer(protocol_serializer&& wrapper);
    protocol_serializer& operator=(protocol_serializer&& wrapper);

    virtual ~protocol_serializer();

    void setByteOrder(const P_BYTE_ORDER byteOrder);
    P_BYTE_ORDER getByteOrder() const;

    void setBufferSource(const BUFFER_SOURCE bufferSource);
    BUFFER_SOURCE getBufferSource() const;

    const unsigned char* getInternalBuffer() const;
    unsigned int getInternalBufferLength() const;

    unsigned char* getExternalBuffer() const;
    void setExternalBuffer(unsigned char * const externalBuffer);
    void setInternalBufferValues(unsigned char * const bufferToCopy);

    unsigned char* getWorkingBuffer() const;

    using NameToIndMap = std::map<const std::string, const unsigned int>;
    NameToIndMap getFields() const;

    struct field_metadata {
        field_metadata(const unsigned int firstBitInd, const unsigned int bitCount, const std::string &name, const ASSOCIATED_TYPE associatedType = ASSOCIATED_TYPE::SIGNED_INTEGER);
        unsigned int firstByteInd;
        unsigned int bytesCount;
        unsigned int touchedBytesCount;
        unsigned int firstBitInd;
        unsigned int bitCount;
        unsigned char leftSpacing;
        unsigned char rightSpacing;
        unsigned char firstMask;
        unsigned char lastMask;
        std::string name;
        ASSOCIATED_TYPE associatedType;
    };

    using NameToIndItt = std::map<const std::string, const unsigned int>::const_iterator;
    using IndToFieldItt = std::map<const unsigned int, const field_metadata>::const_iterator;

    using IndToFieldMap = std::map<const unsigned int, const field_metadata>;

    const std::map<const std::string, const unsigned int>& getNameToIndMap() const;
    const std::map<const unsigned int, const field_metadata>& getIndToFieldMap() const;

    std::string getVisualization(bool drawHeader = true, int firstLineNum = 1, unsigned int horizontalBitMargin = 3, unsigned int nameLinesCount = 2, bool printValues = false) const;
    std::string getDataVisualization(int firstLineNumber = 1, unsigned int bytesPerLine = 2, BASE base = BASE::HEX, bool spacesBetweenBytes = true);

    unsigned char* getFieldBytePointer(const std::string &fieldName) const;

    bool appendField(const field &field);
    bool appendProtocol(const protocol_serializer& wrapper);

    void removeLastField();
    void removeAllFields();
    void clearAllValues();

    template<class T>
    void setFieldValue(const std::string& fieldName, const T& value, std::string* errorString = nullptr)
    {
        mService_fieldItt = getFieldByNameItt(fieldName);
        if(mService_fieldItt == m_indToFieldMap.cend()) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. There is no field '%s'!\n", fieldName.c_str());
            return;
        }
        const field_metadata& field = mService_fieldItt->second;
        _setFieldValue(field, value, errorString);
    }

    template<class T>
    void setGhostFieldValue(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, const T& value, std::string* errorString = nullptr)
    {
        _setFieldValue(field_metadata(fieldFirstBit, fieldBitCount,"<field_ghost>"), value, errorString);
    }

 
    template<class T, size_t N>
    void setFieldValueAsArray(const std::string& fieldName, const T array[N], std::string* errorString = nullptr)
    {
        IndToFieldItt fieldItt = getFieldByNameItt(fieldName);
        if(fieldItt == m_indToFieldMap.cend()) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValueAsArray.There is no field '%s'!\n", fieldName.c_str());
            return;
        }
        const field_metadata& field = fieldItt->second;

        if(field.bitCount % N) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValueAsArray. Field length (%d bits) is not divisible between %d elements!",
                    fieldName.c_str(), field.bitCount, N);
            return;
        }

        const unsigned char ghostFieldLength = field.bitCount / N;

        std::string localerror;
        for(unsigned int i = 0; i < N; ++i) {
            if(localerror.length()) break;

            unsigned int firstBitInd = field.firstBitInd + i*ghostFieldLength;
            setGhostFieldValue(firstBitInd, ghostFieldLength, array[i], &localerror);
        }
        if(localerror.length())
            *errorString = localerror;
    }

    template<class T, size_t N>
    void setGhostFieldValueAsArray(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, const T array[N], std::string* errorString = nullptr)
    {
        if(fieldBitCount % N) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValueAsArray. Ghost field length (%d bits) is not divisible between %d elements!",
                    fieldBitCount, N);
            return;
        }

        const unsigned char ghostFieldLength = fieldBitCount / N;

        std::string localerror;
        for(unsigned int i = 0; i < N; ++i) {
            if(localerror.length()) break;

            unsigned int firstBitInd = fieldFirstBit + i*ghostFieldLength;
            setGhostFieldValue(firstBitInd, ghostFieldLength, array[i], &localerror);
        }
        if(localerror.length())
            *errorString = localerror;
    }

    template<class T>
    T readFieldValue(const std::string &fieldName, std::string* errorString = nullptr) const
    {
        IndToFieldItt m_fieldItt = getFieldByNameItt(fieldName);
        if(m_fieldItt == m_indToFieldMap.cend()) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::readFieldValue. There is no field '%s'!\n", fieldName.c_str());
            return T{};
        }
        const field_metadata& field = m_fieldItt->second;

        return _readFieldValue<T>(field, errorString);
    }

    template<class T>
    T readGhostFieldValue(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, std::string* errorString = nullptr) const
    {
        return _readFieldValue<T>(field_metadata(fieldFirstBit, fieldBitCount, "<field_ghost>"), errorString);
    }

    template<class T, size_t N>
    void readFieldValueAsArray(const std::string &fieldName, T array[N], std::string* errorString = nullptr) const
    {
        IndToFieldItt fieldItt = getFieldByNameItt(fieldName);
        if(fieldItt == m_indToFieldMap.cend()) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::readFieldValueAsArray. There is no field '%s'!\n", fieldName.c_str());
            return;
        }
        const field_metadata& field = fieldItt->second;

        if(field.bitCount % N) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::readFieldValueAsArray. Field length of '%s', equals to %d bits, is not divisible between %d elements!",
                    fieldName.c_str(), field.bitCount, N);
            return;
        }

        const unsigned char ghostFieldLength = field.bitCount / N;

        std::string localerror;
        for(unsigned int i = 0; i < N; ++i) {
            if(localerror.length()) break;

            unsigned int firstBitInd = field.firstBitInd + i*ghostFieldLength;
            array[i] = readGhostFieldValue<T>(firstBitInd, ghostFieldLength, &localerror);
        }
        if(localerror.length())
            *errorString = localerror;
    }

    template<class T, size_t N>
    void readGhostFieldValueAsArray(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, T array[N], std::string* errorString = nullptr)
    {
        if(fieldBitCount % N) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::readFieldValueAsArray. Ghost field length (%d bits) is not divisible between %d elements!",
                    fieldBitCount, N);
            return;
        }

        const unsigned char ghostFieldLength = fieldBitCount / N;

        std::string localerror;
        for(unsigned int i = 0; i < N; ++i) {
            if(localerror.length()) break;

            unsigned int firstBitInd = fieldFirstBit + i*ghostFieldLength;
            array[i] = readGhostFieldValue<T>(firstBitInd, ghostFieldLength, &localerror);
        }
        if(localerror.length())
            *errorString = localerror;
    }

private:
    template<class T>
    void _setFieldValue(const field_metadata& field, const T& value, std::string* errorString = nullptr)
    {
        static_assert(std::is_arithmetic<T>(), "Protocol::setFieldValue. T should be arithmetic!");

        if(m_protocolByteOrder == P_BYTE_ORDER::P_LITTLE_ENDIAN && field.bitCount > 8 && field.bitCount%8) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Field '%s' (length %d) is longer than 8 bits, and is not divisible by 8!", field.name.c_str(), field.bitCount);
            return;
        }

        if(std::is_floating_point<T>::value && field.bitCount != 32 && field.bitCount != 64) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Field '%s' (length %d) is being written as floating point, while having length of not 32 or 64!", field.name.c_str(), field.bitCount);
            return;
        }

        if(m_workingBuffer == nullptr) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Current buffer (BUFFER_SOURCE::%s) == nullptr!", m_bufferSource==BUFFER_SOURCE::EXTERNAL_BUFFER?"EXTERNAL_BUFFER":"INTERNAL_BUFFER");
            return;
        }

        if(field.bitCount > 64) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Field '%s' is longer tha 64 bits!", field.name.c_str(), field.bitCount);
            return;
        }

        memset(mService_rawBytes, 0, 65);
        if(std::is_integral<T>::value) {
            mService_val = value;
            mService_ptrToFirstCopyableMostSignificantByte = (unsigned char*)&mService_val;
            if(getMachineByteOrder() == P_BYTE_ORDER::P_BIG_ENDIAN) {
                mService_ptrToFirstCopyableMostSignificantByte += sizeof(uint64_t) - field.bytesCount;
            }
            memcpy(mService_rawBytes, mService_ptrToFirstCopyableMostSignificantByte, field.bytesCount);
            if(getMachineByteOrder() != m_protocolByteOrder)
                for(uint32_t i = 0; i < field.bytesCount/2; ++i)
                    std::swap(mService_rawBytes[i], mService_rawBytes[field.bytesCount-1-i]);
        } else if(std::is_floating_point<T>::value) {
            if(field.bytesCount == 4) {
                float val = value;
                memcpy(mService_rawBytes, &val, 4);
            } else if(field.bytesCount == 8) {
                double val = value;
                memcpy(mService_rawBytes, &val, 8);
            }
        }

        if(field.leftSpacing == 0 && field.rightSpacing == 0) {
            memcpy(m_workingBuffer + field.firstByteInd, mService_rawBytes, field.bytesCount);
            return;
        }

        mService_finalBytes = mService_rawBytes;
        if(field.rightSpacing) {
            shiftRight(mService_rawBytes, field.bytesCount + 1, 8-field.rightSpacing);
            if(unsigned char transferableBitsCount = field.bitCount%8)
                if(8-field.rightSpacing >= transferableBitsCount)
                    mService_finalBytes = mService_rawBytes+1;
        }

        unsigned char mask = 0;
        for(uint32_t i = 0; i < field.touchedBytesCount; ++i) {
            mask = i==0?field.firstMask:i!=field.touchedBytesCount-1?0xFF:field.lastMask;
            m_workingBuffer[field.firstByteInd + i] &= ~mask;
            m_workingBuffer[field.firstByteInd + i] |= mService_finalBytes[i] & mask;
        }
    }

    template<class T>
    T _readFieldValue(const field_metadata& field, std::string* errorString = nullptr) const
    {
        static_assert(std::is_arithmetic<T>(), "Protocol::readFieldValue. T should be arithmetic!");

        if(m_protocolByteOrder == P_BYTE_ORDER::P_LITTLE_ENDIAN && field.bitCount > 8 && field.bitCount%8) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::readFieldValue. Field '%s' (length %d) is longer than 8 bits, and is not divisible by 8!", field.name.c_str(), field.bitCount);
            return T{};
        }
        if(std::is_floating_point<T>::value) {
            if (field.bitCount != 32 && field.bitCount != 64)
            {
                if (errorString != nullptr) *errorString = getStringFromFormat("Protocol::readFieldValue. Field '%s' (length %d) is being read as floating point, while having length of not 32 or 64!", field.name.c_str(), field.bitCount);
                return T{};
            }
        }

        if(m_workingBuffer == nullptr) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Current buffer (BUFFER_SOURCE::%s) == nullptr!", m_bufferSource==BUFFER_SOURCE::EXTERNAL_BUFFER?"EXTERNAL_BUFFER":"INTERNAL_BUFFER");
            return T{};
        }

        if(field.bitCount > 64) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Field '%s' is longer tha 64 bits!", field.name.c_str(), field.bitCount);
            return T{};
        }

        memset(mService_rawBytes, 0, 65);
        memcpy(mService_rawBytes, m_workingBuffer + field.firstByteInd, field.touchedBytesCount);

        mService_finalBytes = mService_rawBytes;
        mService_finalBytesCount = field.bytesCount;
        if(field.rightSpacing || field.leftSpacing) {
            mService_rawBytes[0] &= field.firstMask;
            if(field.touchedBytesCount > 1)
                mService_rawBytes[field.touchedBytesCount-1] &= field.lastMask;

            if(field.rightSpacing) {
                shiftRight(mService_rawBytes, field.touchedBytesCount, field.rightSpacing);
                mService_finalBytes = mService_rawBytes + field.touchedBytesCount - field.bytesCount;
            }
        }

        if(std::is_floating_point<T>::value) {
            if(field.bytesCount == 4)
                return static_cast<T>(*reinterpret_cast<float*>(mService_finalBytes));
            else if(field.bytesCount == 8)
                return static_cast<T>(*reinterpret_cast<double*>(mService_finalBytes));
        }

        if(getMachineByteOrder() != m_protocolByteOrder)
            for(uint32_t i = 0; i < field.bytesCount/2; ++i)
                std::swap(mService_finalBytes[i], mService_finalBytes[field.bytesCount-i-1]);

        if(field.bytesCount > sizeof(T)) {
            if(getMachineByteOrder() == P_BYTE_ORDER::P_BIG_ENDIAN) {
                mService_finalBytes += field.bytesCount - sizeof(T);
                mService_finalBytesCount -= field.bytesCount - sizeof(T);
            }
        }

        if(std::is_signed_v<T>) {
            if(getMachineByteOrder() == P_BYTE_ORDER::P_BIG_ENDIAN) {
                if(mService_finalBytes[0] & (1 << (7 - (field.leftSpacing + field.rightSpacing)%8)))
                    return (*reinterpret_cast<T*>(mService_finalBytes)) - ((uint64_t)1 << (std::min(mService_finalBytesCount*8, field.bitCount)));
            } else {
                if(field.bitCount < 8) {
                    if(mService_finalBytes[mService_finalBytesCount - 1] & (1 << (7 - (field.leftSpacing + field.rightSpacing)%8)))
                        return (*reinterpret_cast<T*>(mService_finalBytes)) - ((uint64_t)1 << (std::min(mService_finalBytesCount*8, field.bitCount)));
                } else {
                    if(mService_finalBytes[mService_finalBytesCount - 1] & (1 << (7 - (field.leftSpacing + field.rightSpacing)%8))) {
                        return (*reinterpret_cast<T*>(mService_finalBytes)) - ((uint64_t)1 << (std::min(mService_finalBytesCount*8, field.bitCount)));
                    }
                }
            }
        }

        return *reinterpret_cast<T*>(mService_finalBytes);
    }

    static void shiftLeft(unsigned char* buf, int len, unsigned char shift);
    static void shiftRight(unsigned char* buf, int len, unsigned char shift);

    P_BYTE_ORDER m_protocolByteOrder;

    BUFFER_SOURCE m_bufferSource;

    static const P_BYTE_ORDER& getMachineByteOrder();
    static const std::map<unsigned char, unsigned char>& getRightMasks();
    static const std::map<unsigned char, unsigned char>& getLeftMasks();
    static const unsigned char** getHalfByteBinary();

    void reallocateInternalBuffer();
    void updateInternalBuffer();
    IndToFieldItt getFieldByNameItt(const std::string& name) const;

    template<typename... Args>
    static std::string getStringFromFormat(const std::string& format, Args... args)
    {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
        if(size_s <= 0) return format;
        std::unique_ptr<char[]> buf(new char[size_s]);
        std::snprintf(buf.get(), size_s, format.c_str(), args...);
        return std::string(buf.get(), buf.get() + size_s - 1);
    }

    unsigned char* m_internalBuffer;
    unsigned int m_internalBufferLength;
    unsigned char* m_externalBuffer;
    unsigned char* m_workingBuffer;

    mutable unsigned char* mService_finalBytes;
    mutable unsigned int mService_finalBytesCount;
    mutable IndToFieldItt mService_fieldItt;
    mutable uint64_t mService_val;
    mutable unsigned char* mService_ptrToFirstCopyableMostSignificantByte;
    mutable unsigned char mService_rawBytes[65];

    NameToIndMap m_nameToIndMap;
    using NameToIndPair = std::pair<const std::string, const unsigned int>;

private:
    IndToFieldMap m_indToFieldMap;
    using IndToFieldPair = std::pair<const unsigned int, const field_metadata>;
};

}

#endif // EZ_PROTOCOL_SERIALIZER
