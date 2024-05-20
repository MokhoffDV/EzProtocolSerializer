#include "ez_protocol_serializer.h"

using ez::protocol_serializer;

protocol_serializer::protocol_serializer(const bool isLittleEndian, const protocol_serializer::buffer_source bufferSource, unsigned char* const externalBuffer)
    : m_is_little_endian(isLittleEndian)
    , m_buffer_source(bufferSource)
{
    if (bufferSource == buffer_source::internal)
        m_working_buffer = m_internal_buffer.get();
    else {
        m_external_buffer = externalBuffer;
        m_working_buffer = m_external_buffer;
    }
}

protocol_serializer::protocol_serializer(const std::vector<field_init>& fields, const bool isLittleEndian, const buffer_source bufferSource, unsigned char* const externalBuffer)
    : protocol_serializer(isLittleEndian, bufferSource, externalBuffer)
{
    for (const field_init& field_init : fields) {
        if (!append_field(field_init)) {
            clear_protocol();
            return;
        }
    }
}

void ez::protocol_serializer::copy_from(const protocol_serializer& other)
{
    // Drop internal buffer
    m_internal_buffer.reset(nullptr);
    m_internal_buffer_length = 0;

    // Copy internal buffer
    if (other.m_internal_buffer_length && other.m_internal_buffer != nullptr) {
        m_internal_buffer_length = other.m_internal_buffer_length;
        m_internal_buffer.reset(new unsigned char[m_internal_buffer_length]);
        memcpy(m_internal_buffer.get(), other.m_internal_buffer.get(), m_internal_buffer_length);
    }

    // Copy other things
    m_external_buffer = other.m_external_buffer;
    m_buffer_source = other.m_buffer_source;
    m_working_buffer = m_buffer_source == buffer_source::internal ? m_internal_buffer.get() : m_external_buffer;

    m_fields = other.m_fields;
    m_fields_metadata = other.m_fields_metadata;
    m_is_little_endian = other.m_is_little_endian;
}

protocol_serializer::protocol_serializer(const protocol_serializer& other)
{
    copy_from(other);
}

protocol_serializer& protocol_serializer::operator=(const protocol_serializer& other)
{
    copy_from(other);
    return *this;
}

void ez::protocol_serializer::move_from(protocol_serializer&& other)
{
    // Move internal buffer
    m_internal_buffer_length = other.m_internal_buffer_length;
    m_internal_buffer = std::move(other.m_internal_buffer);

    // Move other things
    m_external_buffer = other.m_external_buffer;
    m_buffer_source = other.m_buffer_source;
    m_working_buffer = other.m_working_buffer;

    m_fields = std::move(other.m_fields);
    m_fields_metadata = std::move(other.m_fields_metadata);
    m_is_little_endian = other.m_is_little_endian;
}

protocol_serializer::protocol_serializer(protocol_serializer&& other) noexcept
{
    move_from(std::move(other));
}

protocol_serializer& protocol_serializer::operator=(protocol_serializer&& other) noexcept
{
    move_from(std::move(other));
    return *this;
}

void protocol_serializer::set_is_little_endian(const bool isLittleEndian)
{
    m_is_little_endian = isLittleEndian;
}

bool protocol_serializer::get_is_little_endian() const
{
    return m_is_little_endian;
}

void protocol_serializer::set_buffer_source(const protocol_serializer::buffer_source bufferSource)
{
    m_buffer_source = bufferSource;
    m_working_buffer = m_buffer_source == buffer_source::internal ? m_internal_buffer.get() : m_external_buffer;
}

protocol_serializer::buffer_source protocol_serializer::get_buffer_source() const
{
    return m_buffer_source;
}

const std::unique_ptr<unsigned char[]>& protocol_serializer::get_internal_buffer() const
{
    return m_internal_buffer;
}

unsigned int protocol_serializer::get_internal_buffer_length() const
{
    return m_internal_buffer_length;
}

unsigned char* protocol_serializer::get_external_buffer() const
{
    return m_external_buffer;
}

void protocol_serializer::set_external_buffer(unsigned char* const externalBuffer)
{
    m_external_buffer = externalBuffer;
    m_working_buffer = m_buffer_source == buffer_source::internal ? m_internal_buffer.get() : m_external_buffer;
}

unsigned char* protocol_serializer::get_working_buffer() const
{
    return m_working_buffer;
}

protocol_serializer::fields_list_t protocol_serializer::get_fields_list() const
{
    return m_fields;
}

ez::protocol_serializer::field_metadata ez::protocol_serializer::get_field_metadata(const std::string& name) const
{
    const fields_metadata_t::const_iterator itt = m_fields_metadata.find(name);
    if (itt == m_fields_metadata.cend())
        return field_metadata(0, 0, "");

    return itt->second;
}

std::string protocol_serializer::get_visualization(const visualization_params& vp) const
{
    if (m_fields.empty())
        return "Protocol::getVisualization(). Protocol is empty";

    const_cast<visualization_params&>(vp).horizontal_bit_margin = vp.horizontal_bit_margin == 0 ? 1 : vp.horizontal_bit_margin;
    const_cast<visualization_params&>(vp).name_lines_count = vp.name_lines_count == 0 ? 1 : vp.name_lines_count;

    // Identify length of line numbers
    const std::string firstLineNumStr = std::to_string(vp.first_line_num);
    const size_t lastLineNum = vp.first_line_num + (m_internal_buffer_length / 2) + (m_internal_buffer_length % 2) - 1;
    const std::string lastLineNumStr = std::to_string(lastLineNum);
    const size_t lineNumStrLength = std::max(firstLineNumStr.length(), lastLineNumStr.length());

    // Fill bits array
    std::vector<bool> bits(m_internal_buffer_length * 8LL, 0);
    for (uint32_t i = 0; i < m_internal_buffer_length; ++i) {
        char c = m_working_buffer[i];
        for (int j = 7; j >= 0 && c; --j) {
            if (c & 0x1)
                bits[8LL * i + j] = 1;
            c >>= 1;
        }
    }

    std::string result;

    // Generate header
    if (vp.draw_header) {
        std::string lessSignificantHeader;
        std::string mostSignificantHeader;
        for (int i = 15; i >= 0; i--) {
            if (i < 8)
                lessSignificantHeader += "|" + std::string(vp.horizontal_bit_margin - 1, ' ') + int_to_str_leading_zeros(i, 2) + std::string(vp.horizontal_bit_margin, ' ');
            else
                mostSignificantHeader += "|" + std::string(vp.horizontal_bit_margin - 1, ' ') + int_to_str_leading_zeros(i, 2) + std::string(vp.horizontal_bit_margin, ' ');
        }

        if (vp.first_line_num >= 0)
            result = "|" + std::string(lineNumStrLength + 2, '_');
        if (!m_is_little_endian)
            result += mostSignificantHeader + lessSignificantHeader + "|\n";
        else
            result += lessSignificantHeader + mostSignificantHeader + "|\n";
    }

    // Generate name lines, value line, bits line
    // as continious lines through whole protocol
    const size_t bitTextLen = vp.horizontal_bit_margin * 2ULL + 2ULL;
    const size_t wordTextLen = bitTextLen * 16ULL;
    std::vector<std::string> nameLines(vp.name_lines_count);
    std::string valuesLine;
    std::string bitsLine;
    int currBitIndInsideBuffer = 0;
    for (const std::string& fieldName : m_fields) {
        const field_metadata& fieldMetadata = m_fields_metadata.find(fieldName)->second;
        const size_t availableFieldLength = fieldMetadata.bitCount * bitTextLen - 1;
        std::string name = fieldMetadata.name;
        std::vector<std::string> nameLinesForField(vp.name_lines_count);

        for (uint32_t i = 0; i < vp.name_lines_count; ++i) {
            std::string& fieldLine = nameLinesForField.at(i);
            fieldLine = name.substr(0, availableFieldLength);
            if (name.length() >= availableFieldLength)
                name = name.substr(availableFieldLength);
            else
                name = "";
            fieldLine = fieldLine + std::string(availableFieldLength - fieldLine.length(), ' ') + "|";
            nameLines[i] += fieldLine;
        }

        if (vp.print_values) {
            std::string valueLine;
            if (fieldMetadata.associatedType == associated_type::floating_point && (fieldMetadata.bitCount == 32 || fieldMetadata.bitCount == 64)) {
                if (fieldMetadata.bitCount == 32) valueLine = "=" + std::to_string(read<float>(fieldMetadata.name));
                else if (fieldMetadata.bitCount == 64) valueLine = "=" + std::to_string(read<double>(fieldMetadata.name));
            } else if (fieldMetadata.associatedType == associated_type::signed_integer) {
                if (fieldMetadata.bitCount <= 8)  valueLine = "=" + std::to_string(read<int8_t>(fieldMetadata.name));
                else if (fieldMetadata.bitCount <= 16) valueLine = "=" + std::to_string(read<int16_t>(fieldMetadata.name));
                else if (fieldMetadata.bitCount <= 32) valueLine = "=" + std::to_string(read<int32_t>(fieldMetadata.name));
                else if (fieldMetadata.bitCount <= 64) valueLine = "=" + std::to_string(read<int64_t>(fieldMetadata.name));
            } else {
                if (fieldMetadata.bitCount <= 8)  valueLine = "=" + std::to_string(read<uint8_t>(fieldMetadata.name));
                else if (fieldMetadata.bitCount <= 16) valueLine = "=" + std::to_string(read<uint16_t>(fieldMetadata.name));
                else if (fieldMetadata.bitCount <= 32) valueLine = "=" + std::to_string(read<uint32_t>(fieldMetadata.name));
                else if (fieldMetadata.bitCount <= 64) valueLine = "=" + std::to_string(read<uint64_t>(fieldMetadata.name));
            }

            valueLine = valueLine.substr(0, availableFieldLength);
            valueLine = valueLine + std::string(availableFieldLength - valueLine.length(), ' ') + "|";
            valuesLine += valueLine;
        }

        std::string bitLineText;
        for (size_t j = 0; j < availableFieldLength; ++j) {
            if (j >= vp.horizontal_bit_margin && ((j - vp.horizontal_bit_margin) % bitTextLen) == 0)
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
    while (true) {
        if (nameLines.at(0).length() == 0)
            break;
        const int lineNum = vp.first_line_num + currentLineNum++;
        const std::string lineNumStr = int_to_str_leading_zeros(lineNum, lineNumStrLength);
        const std::string lineNumNumberPart = "| " + lineNumStr + " |";
        const std::string lineNumEmptyPart = "|" + std::string(lineNumStr.length() + 2, ' ') + "|";
        const std::string lineNumBottomPart = "|" + std::string(lineNumStr.length() + 2, '_') + "|";

        if (nameLines.at(0).length() >= wordTextLen) {
            for (uint32_t i = 0; i < vp.name_lines_count; ++i) {
                const std::string lineWithoutLineNum = nameLines.at(i).substr(0, wordTextLen) + "\n";
                if (i == 0)
                    result += (vp.first_line_num >= 0 ? lineNumNumberPart : "|") + lineWithoutLineNum;
                else
                    result += (vp.first_line_num >= 0 ? lineNumEmptyPart : "|") + lineWithoutLineNum;

                nameLines[i] = nameLines[i].substr(wordTextLen);
            }
            if (vp.print_values) {
                result += (vp.first_line_num >= 0 ? lineNumEmptyPart : "|") + valuesLine.substr(0, wordTextLen) + "\n";
                valuesLine = valuesLine.substr(wordTextLen);
            }
            result += (vp.first_line_num >= 0 ? lineNumBottomPart : "|") + bitsLine.substr(0, wordTextLen) + "\n";
            bitsLine = bitsLine.substr(wordTextLen);
        } else {
            for (uint32_t i = 0; i < vp.name_lines_count; ++i) {
                if (i == 0)
                    result += (vp.first_line_num >= 0 ? lineNumNumberPart : "|") + nameLines.at(i) + "\n";
                else
                    result += (vp.first_line_num >= 0 ? lineNumEmptyPart : "|") + nameLines.at(i) + "\n";
            }
            if (vp.print_values)
                result += (vp.first_line_num >= 0 ? lineNumEmptyPart : "|") + valuesLine.substr(0, wordTextLen) + "\n";
            result += (vp.first_line_num >= 0 ? lineNumBottomPart : "|") + bitsLine.substr(0, wordTextLen) + "\n";
            break;
        }
    }

    return result;
}

std::string protocol_serializer::get_data_visualization(const data_visualization_params& dvp) const
{
    if (m_fields.empty())
        return "Protocol::getDataVisualization(). Protocol is empty";

    const_cast<data_visualization_params&>(dvp).bytes_per_line = dvp.bytes_per_line == 0 ? 1 : dvp.bytes_per_line;
    const std::string firstLineNumStr = std::to_string(dvp.first_line_num);
    const size_t lastLineNum = dvp.first_line_num + m_internal_buffer_length / dvp.bytes_per_line + ((m_internal_buffer_length % dvp.bytes_per_line) ? 1 : 0) - 1;
    const std::string lastLineNumStr = std::to_string(lastLineNum);
    const size_t lineNumStrLength = std::max(firstLineNumStr.length(), lastLineNumStr.length());
    unsigned int currentBytesOnLine = 0;

    std::string currentLineText;
    std::string result;
    unsigned int currentLineNumber = 0;
    for (unsigned int i = 0; i < m_internal_buffer_length + 1; ++i) {
        bool itIsFirstByteInLine = false;
        if (currentBytesOnLine == dvp.bytes_per_line || i == 0 || i == m_internal_buffer_length) {
            itIsFirstByteInLine = true;
            if (currentLineText.length())
                result += (result.empty() ? "" : "\n") + currentLineText;

            if (i == m_internal_buffer_length)
                break;

            currentBytesOnLine = 0;

            currentLineText = (dvp.first_line_num >= 0) ? (int_to_str_leading_zeros(dvp.first_line_num + currentLineNumber++, lineNumStrLength) + ": ") : "";
        }

        char byteTextValue[32];
        memset(byteTextValue, 0, sizeof(byteTextValue));
        if (dvp.base_system == data_visualization_params::base::hex)
            sprintf_s(byteTextValue, "%x", m_working_buffer[i]);
        else if (dvp.base_system == data_visualization_params::base::dec)
            sprintf_s(byteTextValue, "%d", m_working_buffer[i]);
        else if (dvp.base_system == data_visualization_params::base::oct)
            sprintf_s(byteTextValue, "%o", m_working_buffer[i]);
        else if (dvp.base_system == data_visualization_params::base::bin)
            sprintf_s(byteTextValue, "%s%s", get_half_byte_binary()[m_working_buffer[i] >> 4].c_str(), get_half_byte_binary()[m_working_buffer[i] & 0x0F].c_str());

        std::string byteTextValueStr(byteTextValue);
        // Add leading zeros to add up to length of 2 for HEX and length of 3 for DEC and OCT
        if(dvp.base_system == data_visualization_params::base::hex)
            byteTextValueStr = std::string(2 - byteTextValueStr.length(), '0') + byteTextValueStr;
        else if(dvp.base_system != data_visualization_params::base::bin)
            byteTextValueStr = std::string(3 - byteTextValueStr.length(), '0') + byteTextValueStr;

        currentLineText += (dvp.spaces_between_bytes ? (itIsFirstByteInLine ? "" : " ") : "") + byteTextValueStr;
        currentBytesOnLine++;
    }

    return result;
}

unsigned char* protocol_serializer::get_field_pointer(const std::string& name) const
{
    m_prealloc_metadata_itt = m_fields_metadata.find(name);

    if (m_prealloc_metadata_itt == m_fields_metadata.cend()) {
        printf("Protocol::getFieldFirstBytePointer. There is no field '%s'!\n", name.c_str());
        return nullptr;
    }

    const field_metadata& field_init = m_prealloc_metadata_itt->second;
    return m_working_buffer + field_init.firstByteInd;
}

bool protocol_serializer::append_field(const field_init& field_init, bool preserveInternalBufferValues)
{
    if (m_fields_metadata.find(field_init.name) != m_fields_metadata.cend())
        return false;

    if (field_init.bitCount == 0)
        return false;

    if (field_init.name.empty())
        return false;

    if (field_init.associatedType == associated_type::floating_point && field_init.bitCount != 32 && field_init.bitCount != 64)
        return false;

    unsigned int firstBitIndex = 0;
    if (!m_fields.empty()) {
        const field_metadata lastFieldMetadata = m_fields_metadata.at(m_fields.back());
        firstBitIndex = lastFieldMetadata.firstBitInd + lastFieldMetadata.bitCount;
    }

    m_fields.push_back(field_init.name);
    m_fields_metadata.insert(fields_metadata_t::value_type(field_init.name, field_metadata(firstBitIndex, field_init.bitCount, field_init.name, field_init.associatedType)));

    if (preserveInternalBufferValues)
        update_internal_buffer();
    else
        reallocate_internal_buffer();

    return true;
}

bool protocol_serializer::append_protocol(const protocol_serializer& other, bool preserveInternalBufferValues)
{
    for (fields_metadata_t::const_iterator itt = other.m_fields_metadata.cbegin(); itt != other.m_fields_metadata.cend(); ++itt)
        if (m_fields_metadata.find(itt->first) != m_fields_metadata.cend())
            return false;

    for (const std::string& fieldName : other.m_fields) {
        const field_metadata& fieldMetadata = other.m_fields_metadata.find(fieldName)->second;
        append_field(protocol_serializer::field_init{fieldMetadata.name, fieldMetadata.bitCount}, preserveInternalBufferValues);
    }

    return true;
}

void ez::protocol_serializer::remove_field(const std::string& name, bool preserveInternalBufferValues)
{
    m_prealloc_metadata_itt = m_fields_metadata.find(name);
    if (m_prealloc_metadata_itt == m_fields_metadata.end())
        return;

    for (auto itt = m_fields.begin(); itt != m_fields.end(); ++itt) {
        if (*itt == name) {
            m_fields.erase(itt);
            m_fields_metadata.erase(name);

            if (preserveInternalBufferValues)
                update_internal_buffer();
            else
                reallocate_internal_buffer();

            return;
        }
    }
}

void protocol_serializer::remove_last_field(bool preserveInternalBufferValues)
{
    if (m_fields.empty())
        return;

    m_fields_metadata.erase(m_fields_metadata.find(m_fields.back()));
    m_fields.pop_back();

    if (preserveInternalBufferValues)
        update_internal_buffer();
    else
        reallocate_internal_buffer();
}

void protocol_serializer::clear_protocol()
{
    if (m_fields.empty())
        return;

    m_fields.clear();
    m_fields_metadata.clear();

    reallocate_internal_buffer();
}

void protocol_serializer::clear_working_buffer()
{
    if (m_fields.empty())
        return;

    memset(m_working_buffer, 0, m_internal_buffer_length);
}

void protocol_serializer::shift_right(unsigned char* buf, int len, unsigned char shift)
{
    if (len <= 0)
        return;

    unsigned char tmp = 0x00, tmp2 = 0x00;
    for (int k = 0; k <= len; ++k) {
        if (k == 0) {
            tmp = buf[k];
            buf[k] >>= shift;
        } else {
            tmp2 = buf[k];
            buf[k] >>= shift;
            buf[k] |= ((tmp & get_right_masks().at(shift)) << (8 - shift));
            if (k != len)
                tmp = tmp2;
        }
    }
}

void protocol_serializer::shift_left(unsigned char* buf, int len, unsigned char shift)
{
    if (len <= 0)
        return;

    unsigned char tmp = 0x00, tmp2 = 0x00;
    for (int k = len; k >= 0; k--) {
        if (k == len) {
            tmp = buf[k];
            buf[k] <<= shift;
        } else {
            tmp2 = buf[k];
            buf[k] <<= shift;
            buf[k] |= ((tmp & get_left_masks().at(shift)) >> (8 - shift));
            tmp = tmp2;
        }
    }
}

const std::map<unsigned char, unsigned char>& protocol_serializer::get_right_masks()
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

const std::map<unsigned char, unsigned char>& protocol_serializer::get_left_masks()
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

const std::vector<std::string>& protocol_serializer::get_half_byte_binary()
{
    static const std::vector<std::string> halfByteBinary = {
        "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111",
        "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"
    };
    return halfByteBinary;
}

std::string ez::protocol_serializer::int_to_str_leading_zeros(int value, size_t length) const
{
    std::string intString = std::to_string(value);
    int64_t numLeadingZeros = length - static_cast<int64_t>(intString.length());

    if (value < 0)
        numLeadingZeros -= 1;

    if (numLeadingZeros > 0)
        intString = std::string(numLeadingZeros, '0') + intString;

    return intString;
}

void protocol_serializer::reallocate_internal_buffer()
{
    // Drop internal buffer
    m_internal_buffer.reset(nullptr);
    m_internal_buffer_length = 0;
    if (m_fields.empty())
        return;

    // Reallocate internal buffer
    const field_metadata& lastFieldMetadata = m_fields_metadata.find(m_fields.back())->second;
    const unsigned int bits = lastFieldMetadata.firstBitInd + lastFieldMetadata.bitCount;
    m_internal_buffer_length = bits / 8 + ((bits % 8) ? 1 : 0);
    m_internal_buffer.reset(new unsigned char[m_internal_buffer_length]);
    memset(m_internal_buffer.get(), 0, m_internal_buffer_length);

    m_working_buffer = m_buffer_source == buffer_source::internal ? m_internal_buffer.get() : m_external_buffer;
}

void protocol_serializer::update_internal_buffer()
{
    // Create a copy of current internal buffer
    const unsigned int oldBufferLength = m_internal_buffer_length;
    std::unique_ptr<unsigned char[]> oldBufferCopy;
    if (m_internal_buffer != nullptr) {
        oldBufferCopy.reset(new unsigned char[oldBufferLength]);
        memcpy(oldBufferCopy.get(), m_internal_buffer.get(), oldBufferLength);
    }

    // Reallocate internal buffer
    reallocate_internal_buffer();

    // Fill with old values
    if (m_internal_buffer_length > 0 && oldBufferLength > 0)
        memcpy(m_internal_buffer.get(), oldBufferCopy.get(), std::min(m_internal_buffer_length, oldBufferLength));
}

protocol_serializer::field_metadata::field_metadata(const unsigned int firstBitInd, const unsigned int bitCount, const std::string& name, const associated_type associatedType)
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

    if (touchedBytesCount == 1) {
        firstMask = ~(get_left_masks().at(leftSpacing) | get_right_masks().at(rightSpacing));
        return;
    }

    if (leftSpacing)
        firstMask = get_right_masks().at(8 - leftSpacing);

    if (rightSpacing)
        lastMask = get_left_masks().at(8 - rightSpacing);
}
