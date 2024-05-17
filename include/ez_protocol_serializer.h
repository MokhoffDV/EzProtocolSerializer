#ifndef EZ_PROTOCOL_SERIALIZER
#define EZ_PROTOCOL_SERIALIZER

#include <map>
#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include <type_traits>
#include <unordered_map>

namespace ez {

class protocol_serializer
{
public:
    enum class BUFFER_SOURCE {
        INTERNAL_BUFFER,
        EXTERNAL_BUFFER
    };

    enum class BASE {
        BIN,
        OCT,
        DEC,
        HEX
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

    struct field_metadata
    {
        field_metadata(const unsigned int firstBitInd, const unsigned int bitCount, const std::string& name, const ASSOCIATED_TYPE associatedType = ASSOCIATED_TYPE::SIGNED_INTEGER);
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

    using fields_t = std::list<std::string>;
    using fields_metadata_t = std::unordered_map<std::string, field_metadata>;

    protocol_serializer(const bool isLittleEndian = false,
                        const BUFFER_SOURCE bufferSource = BUFFER_SOURCE::INTERNAL_BUFFER,
                        unsigned char* const externalBuffer = nullptr);

    protocol_serializer(const std::vector<field>& fields,
                        const bool isLittleEndian = false,
                        const BUFFER_SOURCE bufferSource = BUFFER_SOURCE::INTERNAL_BUFFER,
                        unsigned char* const externalBuffer = nullptr);

    protocol_serializer(const protocol_serializer& other);
    protocol_serializer& operator=(const protocol_serializer& other);

    protocol_serializer(protocol_serializer&& other) noexcept;
    protocol_serializer& operator=(protocol_serializer&& other) noexcept;

    virtual ~protocol_serializer();

    void setIsLittleEndian(const bool isLittleEndian);
    bool getIsLittleEndian() const;

    void setBufferSource(const BUFFER_SOURCE bufferSource);
    BUFFER_SOURCE getBufferSource() const;

    const unsigned char* getInternalBuffer() const;
    unsigned int getInternalBufferLength() const;

    unsigned char* getExternalBuffer() const;
    void setExternalBuffer(unsigned char * const externalBuffer);
    void setInternalBufferValues(unsigned char * const bufferToCopy);

    unsigned char* getWorkingBuffer() const;

    fields_t getFields() const;
    field_metadata getFieldMetadata(const std::string& name) const;

    std::string getVisualization(bool drawHeader = true, int firstLineNum = 1, unsigned int horizontalBitMargin = 3, unsigned int nameLinesCount = 2, bool printValues = false) const;
    std::string getDataVisualization(int firstLineNumber = 1, unsigned int bytesPerLine = 2, BASE base = BASE::HEX, bool spacesBetweenBytes = true);

    unsigned char* getFieldBytePointer(const std::string &fieldName) const;

    bool appendField(const field &field);
    bool appendProtocol(const protocol_serializer& other);

    void removeLastField();
    void removeAllFields();
    void clearWorkingBuffer();

    template<class T>
    void setFieldValue(const std::string& fieldName, const T& value, std::string* errorString = nullptr)
    {
        m_prealloc_fieldMetadataItt = m_fieldsMetadata.find(fieldName);
        if(m_prealloc_fieldMetadataItt == m_fieldsMetadata.cend())
        {
            if(errorString != nullptr)
                *errorString = getStringFromFormat("Protocol::setFieldValue. There is no field '%s'!\n", fieldName.c_str());
            return;
        }
        _setFieldValue(m_prealloc_fieldMetadataItt->second, value, errorString);
    }

    template<class T>
    void setGhostFieldValue(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, const T& value, std::string* errorString = nullptr)
    {
        _setFieldValue(field_metadata(fieldFirstBit, fieldBitCount,"<field_ghost>"), value, errorString);
    }

 
    template<class T, size_t N>
    void setFieldValueAsArray(const std::string& fieldName, const T array[N], std::string* errorString = nullptr)
    {
        m_prealloc_fieldMetadataItt = m_fieldsMetadata.find(fieldName);
        if(m_prealloc_fieldMetadataItt == m_fieldsMetadata.cend())
        {
            if(errorString != nullptr)
                *errorString = getStringFromFormat("Protocol::setFieldValueAsArray.There is no field '%s'!\n", fieldName.c_str());
            return;
        }

        const field_metadata& field = m_prealloc_fieldMetadataItt->second;
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
        m_prealloc_fieldMetadataItt = m_fieldsMetadata.find(fieldName);
        if(m_prealloc_fieldMetadataItt == m_fieldsMetadata.cend())
        {
            if(errorString != nullptr)
                *errorString = getStringFromFormat("Protocol::readFieldValue. There is no field '%s'!\n", fieldName.c_str());
            return T{};
        }
        return _readFieldValue<T>(m_prealloc_fieldMetadataItt->second, errorString);
    }

    template<class T>
    T readGhostFieldValue(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, std::string* errorString = nullptr) const
    {
        return _readFieldValue<T>(field_metadata(fieldFirstBit, fieldBitCount, "<field_ghost>"), errorString);
    }

    template<class T, size_t N>
    void readFieldValueAsArray(const std::string &fieldName, T array[N], std::string* errorString = nullptr) const
    {
        m_prealloc_fieldMetadataItt = m_fieldsMetadata.find(fieldName);
        if(m_prealloc_fieldMetadataItt == m_fieldsMetadata.cend())
        {
            if(errorString != nullptr)
                *errorString = getStringFromFormat("Protocol::readFieldValueAsArray. There is no field '%s'!\n", fieldName.c_str());
            return;
        }

        const field_metadata& field = m_prealloc_fieldMetadataItt->second;
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
            if(errorString != nullptr)
                *errorString = getStringFromFormat("Protocol::readFieldValueAsArray. Ghost field length (%d bits) is not divisible between %d elements!", fieldBitCount, N);
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
    void copyFrom(const protocol_serializer& other);
    void moveFrom(protocol_serializer&& other);

    template<class T>
    void _setFieldValue(const field_metadata& field, const T& value, std::string* errorString = nullptr)
    {
        static_assert(std::is_arithmetic<T>(), "Protocol::setFieldValue. T should be arithmetic!");

        if(m_isLittleEndian && field.bitCount > 8 && field.bitCount%8) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Field '%s' (length %d) is longer than 8 bits, and is not divisible by 8!", field.name.c_str(), field.bitCount);
            return;
        }

        if (std::is_floating_point<T>::value)
        {
            if (field.bitCount != 32 && field.bitCount != 64)
            {
                if (errorString != nullptr)
                    *errorString = getStringFromFormat("Protocol::setFieldValue. Field '%s' (length %d) is being written as floating point, while having length of not 32 or 64!", field.name.c_str(), field.bitCount);
                return;
            }
        }

        if(m_workingBuffer == nullptr) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Current buffer (BUFFER_SOURCE::%s) == nullptr!", m_bufferSource==BUFFER_SOURCE::EXTERNAL_BUFFER?"EXTERNAL_BUFFER":"INTERNAL_BUFFER");
            return;
        }

        if(field.bitCount > 64) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Field '%s' is longer tha 64 bits!", field.name.c_str(), field.bitCount);
            return;
        }

        memset(m_prealloc_rawBytes, 0, 65);
        if(std::is_integral<T>::value) {
            m_prealloc_val = value;
            m_prealloc_ptrToFirstCopyableMostSignificantByte = (unsigned char*)&m_prealloc_val;
            if(!getIsMachineLittleEndian()) {
                m_prealloc_ptrToFirstCopyableMostSignificantByte += sizeof(uint64_t) - field.bytesCount;
            }
            memcpy(m_prealloc_rawBytes, m_prealloc_ptrToFirstCopyableMostSignificantByte, field.bytesCount);
            if(getIsMachineLittleEndian() != m_isLittleEndian)
                for(uint32_t i = 0; i < field.bytesCount/2; ++i)
                    std::swap(m_prealloc_rawBytes[i], m_prealloc_rawBytes[field.bytesCount-1-i]);
        } else if(std::is_floating_point<T>::value) {
            if(field.bytesCount == 4) {
                float val = value;
                memcpy(m_prealloc_rawBytes, &val, 4);
            } else if(field.bytesCount == 8) {
                double val = value;
                memcpy(m_prealloc_rawBytes, &val, 8);
            }
        }

        if(field.leftSpacing == 0 && field.rightSpacing == 0) {
            memcpy(m_workingBuffer + field.firstByteInd, m_prealloc_rawBytes, field.bytesCount);
            return;
        }

        m_prealloc_finalBytes = m_prealloc_rawBytes;
        if(field.rightSpacing) {
            shiftRight(m_prealloc_rawBytes, field.bytesCount + 1, 8-field.rightSpacing);
            if(unsigned char transferableBitsCount = field.bitCount%8)
                if(8-field.rightSpacing >= transferableBitsCount)
                    m_prealloc_finalBytes = m_prealloc_rawBytes+1;
        }

        unsigned char mask = 0;
        for(uint32_t i = 0; i < field.touchedBytesCount; ++i) {
            mask = i==0?field.firstMask:i!=field.touchedBytesCount-1?0xFF:field.lastMask;
            m_workingBuffer[field.firstByteInd + i] &= ~mask;
            m_workingBuffer[field.firstByteInd + i] |= m_prealloc_finalBytes[i] & mask;
        }
    }

    template<class T>
    T _readFieldValue(const field_metadata& field, std::string* errorString = nullptr) const
    {
        static_assert(std::is_arithmetic<T>(), "Protocol::readFieldValue. T should be arithmetic!");

        if(m_isLittleEndian && field.bitCount > 8 && field.bitCount%8) {
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

        memset(m_prealloc_rawBytes, 0, 65);
        memcpy(m_prealloc_rawBytes, m_workingBuffer + field.firstByteInd, field.touchedBytesCount);

        m_prealloc_finalBytes = m_prealloc_rawBytes;
        m_prealloc_finalBytesCount = field.bytesCount;
        if(field.rightSpacing || field.leftSpacing) {
            m_prealloc_rawBytes[0] &= field.firstMask;
            if(field.touchedBytesCount > 1)
                m_prealloc_rawBytes[field.touchedBytesCount-1] &= field.lastMask;

            if(field.rightSpacing) {
                shiftRight(m_prealloc_rawBytes, field.touchedBytesCount, field.rightSpacing);
                m_prealloc_finalBytes = m_prealloc_rawBytes + field.touchedBytesCount - field.bytesCount;
            }
        }

        if(std::is_floating_point<T>::value) {
            if(field.bytesCount == 4)
                return static_cast<T>(*reinterpret_cast<float*>(m_prealloc_finalBytes));
            else if(field.bytesCount == 8)
                return static_cast<T>(*reinterpret_cast<double*>(m_prealloc_finalBytes));
        }

        if(getIsMachineLittleEndian() != m_isLittleEndian)
            for(uint32_t i = 0; i < field.bytesCount/2; ++i)
                std::swap(m_prealloc_finalBytes[i], m_prealloc_finalBytes[field.bytesCount-i-1]);

        if(field.bytesCount > sizeof(T)) {
            if(!getIsMachineLittleEndian()) {
                m_prealloc_finalBytes += field.bytesCount - sizeof(T);
                m_prealloc_finalBytesCount -= field.bytesCount - sizeof(T);
            }
        }

        if(std::is_signed_v<T>) {
            if(!getIsMachineLittleEndian()) {
                if(m_prealloc_finalBytes[0] & (1 << (7 - (field.leftSpacing + field.rightSpacing)%8)))
                    return (*reinterpret_cast<T*>(m_prealloc_finalBytes)) - ((uint64_t)1 << (std::min(m_prealloc_finalBytesCount*8, field.bitCount)));
            } else {
                if(field.bitCount < 8) {
                    if(m_prealloc_finalBytes[m_prealloc_finalBytesCount - 1] & (1 << (7 - (field.leftSpacing + field.rightSpacing)%8)))
                        return (*reinterpret_cast<T*>(m_prealloc_finalBytes)) - ((uint64_t)1 << (std::min(m_prealloc_finalBytesCount*8, field.bitCount)));
                } else {
                    if(m_prealloc_finalBytes[m_prealloc_finalBytesCount - 1] & (1 << (7 - (field.leftSpacing + field.rightSpacing)%8))) {
                        return (*reinterpret_cast<T*>(m_prealloc_finalBytes)) - ((uint64_t)1 << (std::min(m_prealloc_finalBytesCount*8, field.bitCount)));
                    }
                }
            }
        }

        return *reinterpret_cast<T*>(m_prealloc_finalBytes);
    }

    static void shiftLeft(unsigned char* buf, int len, unsigned char shift);
    static void shiftRight(unsigned char* buf, int len, unsigned char shift);

    bool m_isLittleEndian;
    BUFFER_SOURCE m_bufferSource;

    static constexpr bool getIsMachineLittleEndian()
    {
        constexpr uint16_t value = 0x1234;
        return static_cast<uint8_t>(value) == 0x34;
    }

    static const std::map<unsigned char, unsigned char>& getRightMasks();
    static const std::map<unsigned char, unsigned char>& getLeftMasks();
    static const std::vector<std::string>& getHalfByteBinary();

    void reallocateInternalBuffer();
    void updateInternalBuffer();

    template<typename... Args>
    static std::string getStringFromFormat(const std::string& format, Args... args)
    {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
        if(size_s <= 0) return format;
        std::unique_ptr<char[]> buf(new char[size_s]);
        std::snprintf(buf.get(), size_s, format.c_str(), args...);
        return std::string(buf.get(), buf.get() + size_s - 1);
    }

    unsigned char* m_internalBuffer = nullptr;
    unsigned int m_internalBufferLength = 0;
    unsigned char* m_externalBuffer = nullptr;
    unsigned char* m_workingBuffer = nullptr;

    mutable unsigned char* m_prealloc_finalBytes = nullptr;
    mutable unsigned int m_prealloc_finalBytesCount = 0;
    mutable uint64_t m_prealloc_val = 0;
    mutable unsigned char* m_prealloc_ptrToFirstCopyableMostSignificantByte = nullptr;
    mutable unsigned char m_prealloc_rawBytes[65] = "";
    mutable fields_metadata_t::const_iterator m_prealloc_fieldMetadataItt;

private:
    fields_t m_fields;
    fields_metadata_t m_fieldsMetadata;
};

}

#endif // EZ_PROTOCOL_SERIALIZER
