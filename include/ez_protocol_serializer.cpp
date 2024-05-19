#include "ez_protocol_serializer.h"

using ez::protocol_serializer;

protocol_serializer::protocol_serializer(const bool isLittleEndian, const protocol_serializer::BUFFER_SOURCE bufferSource, unsigned char* const externalBuffer)
    : m_isLittleEndian(isLittleEndian)
    , m_bufferSource(bufferSource)
{
    if (bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER)
        m_workingBuffer = m_internalBuffer.get();
    else
    {
        m_externalBuffer = externalBuffer;
        m_workingBuffer = m_externalBuffer;
    }
}

protocol_serializer::protocol_serializer(const std::vector<field>& fields, const bool isLittleEndian, const BUFFER_SOURCE bufferSource, unsigned char* const externalBuffer)
    : protocol_serializer(isLittleEndian, bufferSource, externalBuffer)
{
    for (const field& field : fields)
    {
        if (!appendField(field))
        {
            removeAllFields();
            return;
        }
    }
}

std::string ez::protocol_serializer::intToStringWithLeadingZeros(int value, size_t length) const
{
    std::string intString = std::to_string(value);
    size_t numLeadingZeros = length - intString.length();

    if (value < 0)
        numLeadingZeros -= 1;

    if (numLeadingZeros > 0)
        intString = std::string(numLeadingZeros, '0') + intString;

    return intString;
}

void ez::protocol_serializer::copyFrom(const protocol_serializer& other)
{
    // Drop internal buffer
    m_internalBuffer.reset(nullptr);
    m_internalBufferLength = 0;

    // Copy internal buffer
    if (other.m_internalBufferLength && other.m_internalBuffer != nullptr)
    {
        m_internalBufferLength = other.m_internalBufferLength;
        m_internalBuffer.reset(new unsigned char[m_internalBufferLength]);
        memcpy(m_internalBuffer.get(), other.m_internalBuffer.get(), m_internalBufferLength);
    }

    // Copy other things
    m_externalBuffer = other.m_externalBuffer;
    m_bufferSource = other.m_bufferSource;
    m_workingBuffer = m_bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER ? m_internalBuffer.get() : m_externalBuffer;

    m_fields = other.m_fields;
    m_fieldsMetadata = other.m_fieldsMetadata;
    m_isLittleEndian = other.m_isLittleEndian;
}

protocol_serializer::protocol_serializer(const protocol_serializer& other)
{
    copyFrom(other);
}

protocol_serializer& protocol_serializer::operator=(const protocol_serializer& other)
{
    copyFrom(other);
    return *this;
}

void ez::protocol_serializer::moveFrom(protocol_serializer&& other)
{
    // Move internal buffer
    m_internalBufferLength = other.m_internalBufferLength;
    m_internalBuffer = std::move(other.m_internalBuffer);

    // Move other things
    m_externalBuffer = other.m_externalBuffer;
    m_bufferSource = other.m_bufferSource;
    m_workingBuffer = other.m_workingBuffer;

    m_fields = std::move(other.m_fields);
    m_fieldsMetadata = std::move(other.m_fieldsMetadata);
    m_isLittleEndian = other.m_isLittleEndian;
}

protocol_serializer::protocol_serializer(protocol_serializer&& other) noexcept
{
    moveFrom(std::move(other));
}

protocol_serializer& protocol_serializer::operator=(protocol_serializer&& other) noexcept
{
    moveFrom(std::move(other));
    return *this;
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
    m_workingBuffer = m_bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER ? m_internalBuffer.get() : m_externalBuffer;
}

protocol_serializer::BUFFER_SOURCE protocol_serializer::getBufferSource() const
{
    return m_bufferSource;
}

const std::unique_ptr<unsigned char[]>& protocol_serializer::getInternalBuffer() const
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
    m_workingBuffer = m_bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER ? m_internalBuffer.get() : m_externalBuffer;
}

void protocol_serializer::setInternalBufferValues(unsigned char* const bufferToCopy)
{
    memcpy(m_internalBuffer.get(), bufferToCopy, m_internalBufferLength);
}

unsigned char* protocol_serializer::getWorkingBuffer() const
{
    return m_workingBuffer;
}

protocol_serializer::fields_list_t protocol_serializer::getFields() const
{
    return m_fields;
}

ez::protocol_serializer::field_metadata ez::protocol_serializer::getFieldMetadata(const std::string& name) const
{
    const fields_metadata_t::const_iterator itt = m_fieldsMetadata.find(name);
    if (itt == m_fieldsMetadata.cend())
        return field_metadata(0,0,"");

    return itt->second;
}

std::string protocol_serializer::getVisualization(bool drawHeader, int firstLineNum, unsigned int horizontalBitMargin, unsigned int nameLinesCount, bool printValues) const
{
    if (m_fields.empty())
        return "Protocol::getVisualization(). Protocol is empty";

    horizontalBitMargin = horizontalBitMargin == 0 ? 1 : horizontalBitMargin;
    nameLinesCount = nameLinesCount == 0 ? 1 : nameLinesCount;
    
    // Identify length of line numbers
    const std::string firstLineNumStr = std::to_string(firstLineNum);
    const size_t lastLineNum = firstLineNum + (m_internalBufferLength / 2) + (m_internalBufferLength % 2) - 1;
    const std::string lastLineNumStr = std::to_string(lastLineNum);
    const size_t lineNumStrLength = std::max(firstLineNumStr.length(), lastLineNumStr.length());

    // Fill bits array
    std::vector<bool> bits(m_internalBufferLength * 8LL, 0);
    for (uint32_t i = 0; i < m_internalBufferLength; ++i)
    {
        char c = m_workingBuffer[i];
        for (int j = 7; j >= 0 && c; --j)
        {
            if (c & 0x1)
                bits[8LL * i + j] = 1;
            c >>= 1;
        }
    }

    std::string result;

    // Generate header
    if (drawHeader)
    {
        std::string lessSignificantHeader;
        std::string mostSignificantHeader;
        for (int i = 15; i >= 0; i--)
        {
            if (i < 8)
                lessSignificantHeader += std::string("|") + std::string(horizontalBitMargin - 1, ' ') + (i < 10 ? "0" : "") + std::to_string(i) + std::string(horizontalBitMargin, ' ');
            else
                mostSignificantHeader += std::string("|") + std::string(horizontalBitMargin - 1, ' ') + (i < 10 ? "0" : "") + std::to_string(i) + std::string(horizontalBitMargin, ' ');
        }

        if (firstLineNum >= 0)
            result = "|" + std::string(lineNumStrLength + 2, '_');
        if (!m_isLittleEndian)
            result += mostSignificantHeader + lessSignificantHeader + "|\n";
        else
            result += lessSignificantHeader + mostSignificantHeader + "|\n";
    }

    // Generate name lines, value line, bottom line
    // as continious lines through whole protocol
    const size_t bitTextLen = horizontalBitMargin * 2ULL + 2ULL;
    const size_t wordTextLen = bitTextLen * 16ULL;
    std::vector<std::string> nameLines(nameLinesCount);
    std::string valuesLine;
    std::string bitsLine;
    int currBitIndInsideBuffer = 0;
    for (const std::string& fieldName : m_fields)
    {
        const field_metadata& fieldMetadata = m_fieldsMetadata.find(fieldName)->second;
        const size_t availableFieldLength = fieldMetadata.bitCount * bitTextLen - 1;
        std::string name = fieldMetadata.name;
        std::vector<std::string> nameLinesForField(nameLinesCount);

        for (uint32_t i = 0; i < nameLinesCount; ++i)
        {
            std::string& fieldLine = nameLinesForField.at(i);
            fieldLine = name.substr(0, availableFieldLength);
            if (name.length() >= availableFieldLength)
                name = name.substr(availableFieldLength);
            else
                name = "";
            fieldLine = fieldLine + std::string(availableFieldLength - fieldLine.length(), ' ') + "|";
            nameLines[i] += fieldLine;
        }

        if (printValues)
        {
            std::string valueLine;
            if (fieldMetadata.associatedType == ASSOCIATED_TYPE::FLOATING_POINT && (fieldMetadata.bitCount == 32 || fieldMetadata.bitCount == 64))
            {
                if (fieldMetadata.bitCount == 32) valueLine = "=" + std::to_string(readFieldValue<float>(fieldMetadata.name));
                else if (fieldMetadata.bitCount == 64) valueLine = "=" + std::to_string(readFieldValue<double>(fieldMetadata.name));
            }
            else if (fieldMetadata.associatedType == ASSOCIATED_TYPE::SIGNED_INTEGER)
            {
                if (fieldMetadata.bitCount <= 8)  valueLine = "=" + std::to_string(readFieldValue<int8_t>(fieldMetadata.name));
                else if (fieldMetadata.bitCount <= 16) valueLine = "=" + std::to_string(readFieldValue<int16_t>(fieldMetadata.name));
                else if (fieldMetadata.bitCount <= 32) valueLine = "=" + std::to_string(readFieldValue<int32_t>(fieldMetadata.name));
                else if (fieldMetadata.bitCount <= 64) valueLine = "=" + std::to_string(readFieldValue<int64_t>(fieldMetadata.name));
            }
            else
            {
                if (fieldMetadata.bitCount <= 8)  valueLine = "=" + std::to_string(readFieldValue<uint8_t>(fieldMetadata.name));
                else if (fieldMetadata.bitCount <= 16) valueLine = "=" + std::to_string(readFieldValue<uint16_t>(fieldMetadata.name));
                else if (fieldMetadata.bitCount <= 32) valueLine = "=" + std::to_string(readFieldValue<uint32_t>(fieldMetadata.name));
                else if (fieldMetadata.bitCount <= 64) valueLine = "=" + std::to_string(readFieldValue<uint64_t>(fieldMetadata.name));
            }

            valueLine = valueLine.substr(0, availableFieldLength);
            valueLine = valueLine + std::string(availableFieldLength - valueLine.length(), ' ') + "|";
            valuesLine += valueLine;
        }

        std::string bitLineText;
        for (size_t j = 0; j < availableFieldLength; ++j)
        {
            if (j >= horizontalBitMargin && ((j - horizontalBitMargin) % bitTextLen) == 0)
                bitLineText += std::to_string((int)bits[currBitIndInsideBuffer++]);
            else if ((j + 1) % bitTextLen)
                bitLineText += "_";
            else
                bitLineText += "'";
        }
        bitLineText += "|";
        bitsLine += bitLineText;
    }

    // Split those lines into 16 bits per line
    unsigned int currentLineNum = 0;
    while (true)
    {
        if (nameLines.at(0).length() == 0)
            break;
        const int lineNum = firstLineNum + currentLineNum++;
        const std::string lineNumStr = intToStringWithLeadingZeros(lineNum, lineNumStrLength);
        const std::string lineNumNumberPart = "| " + lineNumStr + " |";
        const std::string lineNumEmptyPart = "|" + std::string(lineNumStr.length() + 2, ' ') + "|";
        const std::string lineNumBottomPart = "|" + std::string(lineNumStr.length() + 2, '_') + "|";

        if (nameLines.at(0).length() >= wordTextLen)
        {
            for (uint32_t i = 0; i < nameLinesCount; ++i)
            {
                const std::string lineWithoutLineNum = nameLines.at(i).substr(0, wordTextLen) + "\n";
                if (i == 0)
                    result += (firstLineNum >= 0 ? lineNumNumberPart : "|") + lineWithoutLineNum;
                else
                    result += (firstLineNum >= 0 ? lineNumEmptyPart : "|") + lineWithoutLineNum;

                nameLines[i] = nameLines[i].substr(wordTextLen);
            }
            if (printValues)
            {
                result += (firstLineNum >= 0 ? lineNumEmptyPart : "|") + valuesLine.substr(0, wordTextLen) + "\n";
                valuesLine = valuesLine.substr(wordTextLen);
            }
            result += (firstLineNum >= 0 ? lineNumBottomPart : "|") + bitsLine.substr(0, wordTextLen) + "\n";
            bitsLine = bitsLine.substr(wordTextLen);
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
                result += (firstLineNum >= 0 ? lineNumEmptyPart : "|") + valuesLine.substr(0, wordTextLen) + "\n";
            result += (firstLineNum >= 0 ? lineNumBottomPart : "|") + bitsLine.substr(0, wordTextLen) + "\n";
            break;
        }
    }

    return result;
}

std::string protocol_serializer::getDataVisualization(int firstLineNumber, unsigned int bytesPerLine, BASE base, bool spacesBetweenBytes)
{
    if (m_fields.empty())
        return "Protocol::getDataVisualization(). Protocol is empty";

    bytesPerLine = bytesPerLine == 0 ? 1 : bytesPerLine;

    unsigned int currentBytesOnLine = 0;

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
                result += (i == 0 ? "" : "\n") + currentLineText;

            if (i == m_internalBufferLength)
                break;

            currentBytesOnLine = 0;

            currentLineText = (firstLineNumber >= 0) ? (std::to_string(firstLineNumber + currentLineNumber++) + ": ") : "";
        }

        char byteTextValue[32];
        memset(byteTextValue, 0, sizeof(byteTextValue));
        if (base == BASE::HEX)
            sprintf_s(byteTextValue, "%x", m_workingBuffer[i]);
        else if (base == BASE::DEC)
            sprintf_s(byteTextValue, "%d", m_workingBuffer[i]);
        else if (base == BASE::OCT)
            sprintf_s(byteTextValue, "%o", m_workingBuffer[i]);
        else if (base == BASE::BIN)
            sprintf_s(byteTextValue, "%s%s", getHalfByteBinary()[m_workingBuffer[i] >> 4].c_str(), getHalfByteBinary()[m_workingBuffer[i] & 0x0F].c_str());

        currentLineText += (spacesBetweenBytes ? (itIsFirstByteInLine ? "" : " ") : "") + std::string(byteTextValue, strlen(byteTextValue));
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

    const field_metadata& field = m_prealloc_fieldMetadataItt->second;
    return m_workingBuffer + field.firstByteInd;
}

bool protocol_serializer::appendField(const field& field, bool preserveInternalBufferValues)
{
    if (m_fieldsMetadata.find(field.name) != m_fieldsMetadata.cend())
        return false;

    if (field.bitCount == 0)
        return false;

    if (field.name.empty())
        return false;

    if (field.associatedType == ASSOCIATED_TYPE::FLOATING_POINT && field.bitCount != 32 && field.bitCount != 64)
        return false;
    
    unsigned int firstBitIndex = 0;
    if (!m_fields.empty())
    {
        const field_metadata lastFieldMetadata = m_fieldsMetadata.at(m_fields.back());
        firstBitIndex = lastFieldMetadata.firstBitInd + lastFieldMetadata.bitCount;
    }

    m_fields.push_back(field.name);
    m_fieldsMetadata.insert(fields_metadata_t::value_type(field.name, field_metadata(firstBitIndex, field.bitCount, field.name, field.associatedType)));
    
    if (preserveInternalBufferValues)
        updateInternalBuffer();
    else
        reallocateInternalBuffer();

    return true;
}

bool protocol_serializer::appendProtocol(const protocol_serializer& other, bool preserveInternalBufferValues)
{
    for (fields_metadata_t::const_iterator itt = other.m_fieldsMetadata.cbegin(); itt != other.m_fieldsMetadata.cend(); ++itt)
        if (m_fieldsMetadata.find(itt->first) != m_fieldsMetadata.cend())
            return false;

    for (const std::string& fieldName : other.m_fields)
    {
        const field_metadata& fieldMetadata = other.m_fieldsMetadata.find(fieldName)->second;
        appendField(protocol_serializer::field{fieldMetadata.name, fieldMetadata.bitCount}, preserveInternalBufferValues);
    }

    return true;
}

void ez::protocol_serializer::removeField(const std::string& name, bool preserveInternalBufferValues)
{
    m_prealloc_fieldMetadataItt = m_fieldsMetadata.find(name);
    if (m_prealloc_fieldMetadataItt == m_fieldsMetadata.end())
        return;

    for (auto itt = m_fields.begin(); itt != m_fields.end(); ++itt)
    {
        if (*itt == name)
        {
            m_fields.erase(itt);
            m_fieldsMetadata.erase(name);

            if (preserveInternalBufferValues)
                updateInternalBuffer();
            else
                reallocateInternalBuffer();

            return;
        }
    }
}

void protocol_serializer::removeLastField(bool preserveInternalBufferValues)
{
    if (m_fields.empty())
        return;

    m_fieldsMetadata.erase(m_fieldsMetadata.find(m_fields.back()));
    m_fields.pop_back();

    if (preserveInternalBufferValues)
        updateInternalBuffer();
    else
        reallocateInternalBuffer();
}

void protocol_serializer::removeAllFields()
{
    if (m_fields.empty())
        return;

    m_fields.clear();
    m_fieldsMetadata.clear();

    reallocateInternalBuffer();
}

void protocol_serializer::clearWorkingBuffer()
{
    if (m_fields.empty())
        return;

    memset(m_workingBuffer, 0, m_internalBufferLength);
}

void protocol_serializer::shiftRight(unsigned char* buf, int len, unsigned char shift)
{
    if (len <= 0)
        return;

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

void protocol_serializer::shiftLeft(unsigned char* buf, int len, unsigned char shift)
{
    if (len <= 0)
        return;

    unsigned char tmp = 0x00, tmp2 = 0x00;
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

const std::map<unsigned char, unsigned char>& protocol_serializer::getRightMasks()
{
    static const std::map<unsigned char, unsigned char> rightMasks {
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
    static const std::map<unsigned char, unsigned char> leftMasks {
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

const std::vector<std::string>& protocol_serializer::getHalfByteBinary()
{
    static const std::vector<std::string> halfByteBinary = {
        "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111",
        "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"
    };
    return halfByteBinary;
}

void protocol_serializer::reallocateInternalBuffer()
{
    // Drop internal buffer
    m_internalBuffer.reset(nullptr);
    m_internalBufferLength = 0;
    if (m_fields.empty())
        return;

    // Reallocate internal buffer
    const field_metadata& lastFieldMetadata = m_fieldsMetadata.find(m_fields.back())->second;
    const unsigned int bits = lastFieldMetadata.firstBitInd + lastFieldMetadata.bitCount;
    m_internalBufferLength = bits / 8 + ((bits % 8) ? 1 : 0);
    m_internalBuffer.reset(new unsigned char[m_internalBufferLength]);
    memset(m_internalBuffer.get(), 0, m_internalBufferLength);

    m_workingBuffer = m_bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER ? m_internalBuffer.get() : m_externalBuffer;
}

void protocol_serializer::updateInternalBuffer()
{
    // Create a copy of current internal buffer
    const unsigned int oldBufferLength = m_internalBufferLength;
    std::unique_ptr<unsigned char[]> oldBufferCopy;
    if (m_internalBuffer != nullptr)
    {
        oldBufferCopy.reset(new unsigned char[oldBufferLength]);
        memcpy(oldBufferCopy.get(), m_internalBuffer.get(), oldBufferLength);
    }

    // Reallocate internal buffer
    reallocateInternalBuffer();

    // Fill with old values
    if (m_internalBufferLength > 0 && oldBufferLength > 0)
        memcpy(m_internalBuffer.get(), oldBufferCopy.get(), std::min(m_internalBufferLength, oldBufferLength));
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
