#include "ez_protocol_serializer.h"

using ez::protocol_serializer;

protocol_serializer::protocol_serializer(const bool isLittleEndian, const protocol_serializer::BUFFER_SOURCE bufferSource, unsigned char* const externalBuffer)
    : m_isLittleEndian(isLittleEndian)
    , m_bufferSource(bufferSource)
{
    if (bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER)
        m_workingBuffer = m_internalBuffer;
    else
    {
        m_externalBuffer = externalBuffer;
        m_workingBuffer = m_externalBuffer;
    }
}

protocol_serializer::protocol_serializer(const std::vector<field>& fields, const bool isLittleEndian, const BUFFER_SOURCE bufferSource, unsigned char* const externalBuffer)
    : protocol_serializer(isLittleEndian, bufferSource, externalBuffer)
{
    bool error = false;
    for (const field& field : fields)
    {
        if (!appendField(field))
        {
            error = true;
            break;
        }
    }
    if (error)
        removeAllFields();
}

void ez::protocol_serializer::copyFrom(const protocol_serializer& other)
{
    m_internalBuffer = nullptr;
    m_internalBufferLength = other.m_internalBufferLength;

    if (m_internalBufferLength)
    {
        m_internalBuffer = new unsigned char[m_internalBufferLength];
        if (other.m_internalBuffer != nullptr)
            memcpy(m_internalBuffer, other.m_internalBuffer, m_internalBufferLength);
    }

    m_externalBuffer = other.m_externalBuffer;

    m_fields = other.m_fields;
    m_fieldsMetadata = other.m_fieldsMetadata;
    m_isLittleEndian = other.m_isLittleEndian;
    m_bufferSource = other.m_bufferSource;
    m_workingBuffer = m_bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER ? m_internalBuffer : m_externalBuffer;
}

protocol_serializer::protocol_serializer(const protocol_serializer& other)
{
    copyFrom(other);
}

protocol_serializer& protocol_serializer::operator=(const protocol_serializer& other)
{
    if (m_internalBuffer != nullptr)
        delete[] m_internalBuffer;
    m_internalBuffer = nullptr;

    copyFrom(other);
    return *this;
}

void ez::protocol_serializer::moveFrom(protocol_serializer&& other)
{
    m_internalBuffer = nullptr;

    m_internalBufferLength = other.m_internalBufferLength;
    m_internalBuffer = other.m_internalBuffer;
    other.m_internalBuffer = nullptr;

    m_externalBuffer = other.m_externalBuffer;

    m_fields = std::move(other.m_fields);
    m_fieldsMetadata = std::move(other.m_fieldsMetadata);
    m_isLittleEndian = other.m_isLittleEndian;
    m_bufferSource = other.m_bufferSource;
    m_workingBuffer = other.m_workingBuffer;
}

protocol_serializer::protocol_serializer(protocol_serializer&& other) noexcept
{
    moveFrom(std::move(other));
}

protocol_serializer& protocol_serializer::operator=(protocol_serializer&& other) noexcept
{
    if (m_internalBuffer != nullptr)
        delete[] m_internalBuffer;
    m_internalBuffer = nullptr;

    moveFrom(std::move(other));
    return *this;
}

protocol_serializer::~protocol_serializer()
{
    if (m_internalBuffer != nullptr)
        delete[] m_internalBuffer;
    m_internalBuffer = nullptr;
}


void protocol_serializer::setIsLittleEndian(const bool isLittleEndian)
{
    m_isLittleEndian = isLittleEndian;
}

bool protocol_serializer::getIsLittleEndian() const
{
    return m_isLittleEndian;
}

void protocol_serializer::setBufferSource(const protocol_serializer::BUFFER_SOURCE bufferSource)
{
    m_bufferSource = bufferSource;
    if (bufferSource == protocol_serializer::BUFFER_SOURCE::INTERNAL_BUFFER)
    {
        m_workingBuffer = m_internalBuffer;
    }
    else
    {
        m_workingBuffer = m_externalBuffer;
    }
}

protocol_serializer::BUFFER_SOURCE protocol_serializer::getBufferSource() const
{
    return m_bufferSource;
}

const unsigned char* protocol_serializer::getInternalBuffer() const
{
    return m_internalBuffer;
}

unsigned int protocol_serializer::getInternalBufferLength() const
{
    return m_internalBufferLength;
}

unsigned char* protocol_serializer::getExternalBuffer() const
{
    return m_externalBuffer;
}

void protocol_serializer::setExternalBuffer(unsigned char* const externalBuffer)
{
    m_externalBuffer = externalBuffer;
    if (m_bufferSource == BUFFER_SOURCE::EXTERNAL_BUFFER)
    {
        m_workingBuffer = m_externalBuffer;
    }
}

void protocol_serializer::setInternalBufferValues(unsigned char* const bufferToCopy)
{
    memcpy(m_internalBuffer, bufferToCopy, m_internalBufferLength);
}

unsigned char* protocol_serializer::getWorkingBuffer() const
{
    return m_workingBuffer;
}

protocol_serializer::fields_t protocol_serializer::getFields() const
{
    return m_fields;
}

std::string protocol_serializer::getVisualization(bool drawHeader, int firstLineNum, unsigned int horizontalBitMargin, unsigned int nameLinesCount, bool printValues) const
{
    if (horizontalBitMargin <= 0) horizontalBitMargin = 1;
    if (nameLinesCount <= 0) nameLinesCount = 1;

    if (m_fields.empty()) return "Protocol::getVisualization(). Protocol is empty";

    std::vector<bool> bits(m_internalBufferLength * 8, 0);

    for (uint32_t i = 0; i < m_internalBufferLength; ++i)
    {
        char c = m_workingBuffer[i];
        for (int j = 7; j >= 0 && c; --j)
        {
            if (c & 0x1)
                bits[8 * i + j] = 1;
            c >>= 1;
        }
    }

    std::string result;

    std::string lessSignificantHeader;
    std::string mostSignificantHeader;
    for (int i = 15; i >= 0; i--)
    {
        if (i < 8)
            lessSignificantHeader += std::string("|") + std::string(horizontalBitMargin - 1, ' ') + (i < 10 ? "0" : "") + std::to_string(i) + std::string(horizontalBitMargin, ' ');
        else
            mostSignificantHeader += std::string("|") + std::string(horizontalBitMargin - 1, ' ') + (i < 10 ? "0" : "") + std::to_string(i) + std::string(horizontalBitMargin, ' ');
    }
    unsigned char bitTextLen = static_cast<unsigned char>(lessSignificantHeader.length() / 8);

    if (drawHeader)
    {
        if (firstLineNum >= 0) result = "|_____";
        if (!m_isLittleEndian)
            result += mostSignificantHeader + lessSignificantHeader + "|\n";
        else
            result += lessSignificantHeader + mostSignificantHeader + "|\n";
    }

    std::vector<std::string> nameLines(nameLinesCount);
    std::string valuesLine;
    std::string bottomLine;
    int currBitIndInsideBuffer = 0;
    for (const std::string& fieldName : m_fields)
    {
        const field_metadata& field = m_fieldsMetadata.find(fieldName)->second;
        unsigned int availableNameLength = field.bitCount * bitTextLen - 1;
        std::string name = field.name;
        std::vector<std::string> nameLinesForField(nameLinesCount);

        for (uint32_t i = 0; i < nameLinesCount; ++i)
        {
            std::string& fieldLine = nameLinesForField.at(i);
            fieldLine = name.substr(0, availableNameLength);
            if (name.length() >= availableNameLength)
                name = name.substr(availableNameLength);
            else
                name = "";
            fieldLine = fieldLine + std::string(availableNameLength - fieldLine.length(), ' ') + "|";
            nameLines[i] += fieldLine;
        }

        if (printValues)
        {
            std::string valueLine;
            if (field.associatedType == ASSOCIATED_TYPE::FLOATING_POINT && (field.bitCount == 32 || field.bitCount == 64))
            {
                if (field.bitCount == 32) valueLine = "=" + std::to_string(readFieldValue<float>(field.name));
                else if (field.bitCount == 64) valueLine = "=" + std::to_string(readFieldValue<double>(field.name));
            }
            else if (field.associatedType == ASSOCIATED_TYPE::SIGNED_INTEGER)
            {
                if (field.bitCount <= 8)  valueLine = "=" + std::to_string(readFieldValue<int8_t>(field.name));
                else if (field.bitCount <= 16) valueLine = "=" + std::to_string(readFieldValue<int16_t>(field.name));
                else if (field.bitCount <= 32) valueLine = "=" + std::to_string(readFieldValue<int32_t>(field.name));
                else if (field.bitCount <= 64) valueLine = "=" + std::to_string(readFieldValue<int64_t>(field.name));
            }
            else
            {
                if (field.bitCount <= 8)  valueLine = "=" + std::to_string(readFieldValue<uint8_t>(field.name));
                else if (field.bitCount <= 16) valueLine = "=" + std::to_string(readFieldValue<uint16_t>(field.name));
                else if (field.bitCount <= 32) valueLine = "=" + std::to_string(readFieldValue<uint32_t>(field.name));
                else if (field.bitCount <= 64) valueLine = "=" + std::to_string(readFieldValue<uint64_t>(field.name));
            }

            valueLine = valueLine.substr(0, availableNameLength);
            valueLine = valueLine + std::string(availableNameLength - valueLine.length(), ' ') + "|";
            valuesLine += valueLine;
        }

        std::string bottomLineText;
        for (uint32_t j = 0; j < availableNameLength; ++j)
        {
            if (j >= horizontalBitMargin && ((j - horizontalBitMargin) % bitTextLen) == 0)
            {
                bottomLineText += std::to_string((int)bits[currBitIndInsideBuffer++]);
            }
            else if ((j + 1) % bitTextLen)
                bottomLineText += "_";
            else
                bottomLineText += "'";
        }
        bottomLineText += "|";
        bottomLine += bottomLineText;
    }

    std::string wordNumStr;
    unsigned int currentLineNum = 0;
    while (true)
    {
        if (nameLines.at(0).length() == 0) break;
        int num = firstLineNum + currentLineNum++;
        wordNumStr = (num < 100 ? std::string("0") : "") + (num < 10 ? "0" : "") + std::to_string(num);

        std::string lineNumNumberPart = "| " + wordNumStr + " |";
        std::string lineNumEmptyPart = "|     |";
        std::string lineNumBottomPart = "|_____|";

        if (nameLines.at(0).length() >= bitTextLen * 16)
        {
            for (uint32_t i = 0; i < nameLinesCount; ++i)
            {
                std::string lineWithoutLineNum = nameLines.at(i).substr(0, bitTextLen * 16) + "\n";
                if (i == 0)
                    result += (firstLineNum >= 0 ? lineNumNumberPart : "|") + lineWithoutLineNum;
                else
                    result += (firstLineNum >= 0 ? lineNumEmptyPart : "|") + lineWithoutLineNum;

                nameLines[i] = nameLines[i].substr(bitTextLen * 16);
            }
            if (printValues)
            {
                result += (firstLineNum >= 0 ? lineNumEmptyPart : "|") + valuesLine.substr(0, bitTextLen * 16) + "\n";
                valuesLine = valuesLine.substr(bitTextLen * 16);
            }
            result += (firstLineNum >= 0 ? lineNumBottomPart : "|") + bottomLine.substr(0, bitTextLen * 16) + "\n";
            bottomLine = bottomLine.substr(bitTextLen * 16);
        }
        else
        {
            for (uint32_t i = 0; i < nameLinesCount; ++i)
            {
                if (i == 0)
                    result += (firstLineNum >= 0 ? lineNumNumberPart : "|") + nameLines.at(i) + "\n";
                else
                    result += (firstLineNum >= 0 ? lineNumEmptyPart : "|") + nameLines.at(i) + "\n";
            }
            if (printValues)
            {
                result += (firstLineNum >= 0 ? lineNumEmptyPart : "|") + valuesLine.substr(0, bitTextLen * 16) + "\n";
            }
            result += (firstLineNum >= 0 ? lineNumBottomPart : "|") + bottomLine.substr(0, bitTextLen * 16) + "\n";
            break;
        }
    }

    return result;
}

std::string protocol_serializer::getDataVisualization(int firstLineNumber, unsigned int bytesPerLine, BASE base, bool spacesBetweenBytes)
{
    if (m_fields.empty()) return "Protocol::getDataVisualization(). Protocol is empty";

    if (bytesPerLine == 0)
        bytesPerLine = 1;

    unsigned int currentBytesOnLine = 0;
    unsigned char* workingBuffer = getWorkingBuffer();

    std::string currentLineText;
    std::string result;
    unsigned int currentLineNumber = 0;

    for (unsigned int i = 0; i < m_internalBufferLength + 1; ++i)
    {
        bool itIsFirstByteInLine = false;
        if (currentBytesOnLine == bytesPerLine || i == 0 || i == m_internalBufferLength)
        {
            itIsFirstByteInLine = true;
            if (currentLineText.length())
            {
                result += (i == 0 ? "" : "\n") + currentLineText;
            }

            if (i == m_internalBufferLength) break;

            currentBytesOnLine = 0;

            currentLineText = (firstLineNumber >= 0) ? (std::to_string(firstLineNumber + currentLineNumber++) + ": ") : "";
        }

        char byteTextValue[32]; byteTextValue[0] = 0;
        if (base == BASE::HEX)
            sprintf_s(byteTextValue, "%x", workingBuffer[i]);
        if (base == BASE::DEC)
            sprintf_s(byteTextValue, "%d", workingBuffer[i]);
        if (base == BASE::OCT)
            sprintf_s(byteTextValue, "%o", workingBuffer[i]);
        if (base == BASE::BIN)
            sprintf_s(byteTextValue, "%s%s", getHalfByteBinary()[workingBuffer[i] >> 4], getHalfByteBinary()[workingBuffer[i] & 0x0F]);

        currentLineText += (spacesBetweenBytes ? (itIsFirstByteInLine ? "" : " ") : "") + std::string(byteTextValue);

        currentBytesOnLine++;
    }

    return result;
}

unsigned char* protocol_serializer::getFieldBytePointer(const std::string& fieldName) const
{
    m_prealloc_fieldMetadataItt = m_fieldsMetadata.find(fieldName);

    if (m_prealloc_fieldMetadataItt == m_fieldsMetadata.cend())
    {
        printf("Protocol::getFieldFirstBytePointer. There is no field '%s'!\n", fieldName.c_str());
        return nullptr;
    }

    unsigned char* workingBuffer = m_bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER ? m_internalBuffer : m_externalBuffer;
    const field_metadata& field = m_prealloc_fieldMetadataItt->second;
    return workingBuffer + field.firstByteInd;
}

bool protocol_serializer::appendField(const field& field)
{
    if (m_fieldsMetadata.find(field.name) != m_fieldsMetadata.cend())
        return false;

    if (field.bitCount == 0)
        return false;

    m_fields.push_back(field.name);
    m_fieldsMetadata.insert(fields_metadata_t::value_type(field.name, field_metadata(0, field.bitCount, field.name, field.associatedType)));
    updateInternalBuffer();

    return true;
}

bool protocol_serializer::appendProtocol(const protocol_serializer& other)
{
    for (fields_metadata_t::const_iterator itt = other.m_fieldsMetadata.cbegin(); itt != other.m_fieldsMetadata.cend(); ++itt)
    {
        if (m_fieldsMetadata.find(itt->first) != m_fieldsMetadata.cend())
            return false;
    }

    for (const std::string& fieldName : other.m_fields)
    {
        const field_metadata& fieldMetadata = other.m_fieldsMetadata.find(fieldName)->second;
        appendField(protocol_serializer::field{fieldMetadata.name, fieldMetadata.bitCount});
    }

    return true;
}

void protocol_serializer::removeLastField()
{
    if (m_fields.empty())
        return;

    m_fieldsMetadata.erase(m_fieldsMetadata.find(m_fields.back()));
    m_fields.pop_back();

    reallocateInternalBuffer();
}

void protocol_serializer::removeAllFields()
{
    m_fields.clear(); m_fieldsMetadata.clear(); reallocateInternalBuffer();
}

void protocol_serializer::clearWorkingBuffer()
{
    if (m_fields.empty())
        return;

    memset(getWorkingBuffer(), 0, m_internalBufferLength);
}

void protocol_serializer::shiftRight(unsigned char* buf, int len, unsigned char shift)
{
    unsigned char tmp = 0x00, tmp2 = 0x00;
    for (int k = 0; k <= len; ++k)
    {
        if (k == 0)
        {
            tmp = buf[k];
            buf[k] >>= shift;
        }
        else
        {
            tmp2 = buf[k];
            buf[k] >>= shift;
            buf[k] |= ((tmp & getRightMasks().at(shift)) << (8 - shift));
            if (k != len)
                tmp = tmp2;
        }
    }
}

constexpr bool protocol_serializer::getIsMachineLittleEndian()
{
    constexpr uint16_t value = 0x1234;
    return static_cast<uint8_t>(value) == 0x34;
}

const std::map<unsigned char, unsigned char>& protocol_serializer::getRightMasks()
{
    static const std::map<unsigned char, unsigned char> rightMasks{
        {0,0x00},
        {1,0x01},
        {2,0x03},
        {3,0x07},
        {4,0x0F},
        {5,0x1F},
        {6,0x3F},
        {7,0x7F}
    };
    return rightMasks;
}

const std::map<unsigned char, unsigned char>& protocol_serializer::getLeftMasks()
{
    static const std::map<unsigned char, unsigned char> leftMasks{
        {0,0x00},
        {1,0x80},
        {2,0xC0},
        {3,0xE0},
        {4,0xF0},
        {5,0xF8},
        {6,0xFC},
        {7,0xFE}
    };
    return leftMasks;
}

const unsigned char** protocol_serializer::getHalfByteBinary()
{
    static const unsigned char halfByteBinary[16][5] = {
     "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"
    };
    return (const unsigned char**)(halfByteBinary);
}

void protocol_serializer::shiftLeft(unsigned char* buf, int len, unsigned char shift)
{
    char tmp = 0x00, tmp2 = 0x00;
    for (int k = len; k >= 0; k--)
    {
        if (k == len)
        {
            tmp = buf[k];
            buf[k] <<= shift;
        }
        else
        {
            tmp2 = buf[k];
            buf[k] <<= shift;
            buf[k] |= ((tmp & getLeftMasks().at(shift)) >> (8 - shift));
            tmp = tmp2;
        }
    }
}

void protocol_serializer::reallocateInternalBuffer()
{
    if (m_internalBuffer != nullptr)
    {
        delete[] m_internalBuffer;
        m_internalBuffer = nullptr;
        m_internalBufferLength = 0;
    }

    if (m_fields.empty())
        return;

    const field_metadata& lastField = m_fieldsMetadata.find(m_fields.back())->second;

    unsigned int bits = lastField.firstBitInd + lastField.bitCount;
    m_internalBufferLength = bits / 8 + ((bits % 8) ? 1 : 0);
    m_internalBuffer = new unsigned char[m_internalBufferLength];
    memset(m_internalBuffer, 0, m_internalBufferLength);

    if (m_bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER)
        m_workingBuffer = m_internalBuffer;
}

void protocol_serializer::updateInternalBuffer()
{
    unsigned int oldBufferLength = m_internalBufferLength;
    unsigned char* oldBuffer = nullptr;
    if (m_internalBuffer != nullptr)
    {
        oldBuffer = new unsigned char[oldBufferLength];
        memcpy(oldBuffer, m_internalBuffer, m_internalBufferLength);
    }

    reallocateInternalBuffer();

    if (m_internalBuffer != nullptr && oldBuffer != nullptr)
        memcpy(m_internalBuffer, oldBuffer, std::min(oldBufferLength, m_internalBufferLength));

    if (oldBuffer != nullptr)
        delete[] oldBuffer;
}

protocol_serializer::field_metadata::field_metadata(const unsigned int firstBitInd, const unsigned int bitCount, const std::string& name, const ASSOCIATED_TYPE associatedType)
{
    this->associatedType = associatedType;
    this->name = name;
    this->firstBitInd = firstBitInd;
    this->bitCount = bitCount;
    bytesCount = bitCount / 8 + ((bitCount % 8) ? 1 : 0);
    firstByteInd = firstBitInd / 8;
    unsigned int lastByteInd = (firstBitInd + bitCount - 1) / 8;
    touchedBytesCount = lastByteInd - firstByteInd + 1;
    leftSpacing = firstBitInd % 8;
    rightSpacing = (8 - (firstBitInd + bitCount) % 8) % 8;

    firstMask = 0xFF;
    lastMask = 0xFF;

    if (touchedBytesCount == 1)
    {
        firstMask = ~(getLeftMasks().at(leftSpacing) | getRightMasks().at(rightSpacing));
        return;
    }

    if (leftSpacing)
        firstMask = getRightMasks().at(8 - leftSpacing);

    if (rightSpacing)
        lastMask = getLeftMasks().at(8 - rightSpacing);
}
