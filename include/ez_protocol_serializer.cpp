#include <ez_protocol_serializer.h>

using ez::protocol_serializer;

protocol_serializer::protocol_serializer(const bool is_little_endian, const protocol_serializer::buffer_source buffer_source, byte_ptr_t const external_buffer)
    : m_is_little_endian(is_little_endian)
    , m_buffer_source(buffer_source)
{
    if (buffer_source == buffer_source::internal)
        m_working_buffer = m_internal_buffer.get();
    else {
        m_external_buffer = external_buffer;
        m_working_buffer = m_external_buffer;
    }
}

protocol_serializer::protocol_serializer(const std::vector<field_init>& fields, const bool is_little_endian, const buffer_source buffer_source, byte_ptr_t const external_buffer)
    : protocol_serializer(is_little_endian, buffer_source, external_buffer)
{
    for (const field_init& field_init : fields) {
        if (append_field(field_init) != result_code::ok) {
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
    other.m_internal_buffer_length = 0;

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

void protocol_serializer::set_is_little_endian(const bool is_little_endian)
{
    m_is_little_endian = is_little_endian;
}

bool protocol_serializer::get_is_little_endian() const
{
    return m_is_little_endian;
}

void protocol_serializer::set_buffer_source(const protocol_serializer::buffer_source buffer_source)
{
    m_buffer_source = buffer_source;
    m_working_buffer = m_buffer_source == buffer_source::internal ? m_internal_buffer.get() : m_external_buffer;
}

protocol_serializer::buffer_source protocol_serializer::get_buffer_source() const
{
    return m_buffer_source;
}

const ez::protocol_serializer::internal_buffer_ptr_t& protocol_serializer::get_internal_buffer() const
{
    return m_internal_buffer;
}

unsigned int protocol_serializer::get_internal_buffer_length() const
{
    return m_internal_buffer_length;
}

ez::protocol_serializer::byte_ptr_t protocol_serializer::get_external_buffer() const
{
    return m_external_buffer;
}

void protocol_serializer::set_external_buffer(byte_ptr_t const external_buffer)
{
    m_external_buffer = external_buffer;
    m_working_buffer = m_buffer_source == buffer_source::internal ? m_internal_buffer.get() : m_external_buffer;
}

ez::protocol_serializer::byte_ptr_t protocol_serializer::get_working_buffer() const
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
        return field_metadata(0, 0);

    return itt->second;
}

std::string protocol_serializer::get_visualization(const visualization_params& vp) const
{
    if (m_fields.empty())
        return "";

    const_cast<visualization_params&>(vp).horizontal_bit_margin = vp.horizontal_bit_margin == 0 ? 1 : vp.horizontal_bit_margin;
    const_cast<visualization_params&>(vp).name_lines_count = vp.name_lines_count == 0 ? 1 : vp.name_lines_count;

    // Identify length of line numbers
    const std::string first_line_num_str = std::to_string(vp.first_line_num);
    const size_t last_line_num = vp.first_line_num + (m_internal_buffer_length / 2) + (m_internal_buffer_length % 2) - 1;
    const std::string last_line_numStr = std::to_string(last_line_num);
    const size_t line_num_str_length = std::max(first_line_num_str.length(), last_line_numStr.length());

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
        std::string less_significant_header;
        std::string most_significant_header;
        for (int i = 15; i >= 0; i--) {
            const std::string bit_segment = "|" + std::string(vp.horizontal_bit_margin - 1, ' ') + int_to_str_leading_zeros(i, 2) + std::string(vp.horizontal_bit_margin, ' ');
            if (i < 8)
                less_significant_header += bit_segment;
            else
                most_significant_header += bit_segment;
        }

        if (vp.first_line_num >= 0)
            result = "|" + std::string(line_num_str_length + 2, '_');
        if (!m_is_little_endian)
            result += most_significant_header + less_significant_header + "|\n";
        else
            result += less_significant_header + most_significant_header + "|\n";
    }

    // Generate name lines, value line, bits line
    // as continious lines through whole protocol
    const size_t bit_text_len = vp.horizontal_bit_margin * 2ULL + 2ULL;
    const size_t word_text_len = bit_text_len * 16ULL;
    std::vector<std::string> name_lines(vp.name_lines_count);
    std::string values_line;
    std::string bits_line;
    int curr_bit_ind_inside_buffer = 0;
    for (const std::string& field_name : m_fields) {
        const field_metadata& field_metadata = m_fields_metadata.find(field_name)->second;
        const size_t available_field_length = field_metadata.bit_count * bit_text_len - 1;
        std::string name = field_name;
        std::vector<std::string> name_linesForField(vp.name_lines_count);

        for (uint32_t i = 0; i < vp.name_lines_count; ++i) {
            std::string& field_line = name_linesForField.at(i);
            field_line = name.substr(0, available_field_length);
            if (name.length() >= available_field_length)
                name = name.substr(available_field_length);
            else
                name = "";
            field_line = field_line + std::string(available_field_length - field_line.length(), ' ') + "|";
            name_lines[i] += field_line;
        }

        if (vp.print_values) {
            std::string value_line;
            if (field_metadata.vis_type == visualization_type::floating_point) {
                if (field_metadata.bit_count == 32)
                    value_line = "=" + std::to_string(read<float>(field_name));
                else if (field_metadata.bit_count == 64)
                    value_line = "=" + std::to_string(read<double>(field_name));
            } else if (field_metadata.vis_type == visualization_type::signed_integer) {
                value_line = "=" + std::to_string(read<int64_t>(field_name));
            } else {
                value_line = "=" + std::to_string(read<uint64_t>(field_name));
            }

            value_line = value_line.substr(0, available_field_length);
            value_line = value_line + std::string(available_field_length - value_line.length(), ' ') + "|";
            values_line += value_line;
        }

        std::string bit_line_text;
        for (size_t j = 0; j < available_field_length; ++j) {
            if (j >= vp.horizontal_bit_margin && ((j - vp.horizontal_bit_margin) % bit_text_len) == 0)
                bit_line_text += std::to_string((int)bits[curr_bit_ind_inside_buffer++]);
            else if ((j + 1) % bit_text_len)
                bit_line_text += "_";
            else
                bit_line_text += "'";
        }
        bit_line_text += "|";
        bits_line += bit_line_text;
    }

    // Split those lines into 16 bits per line
    unsigned int current_line_num = 0;
    while (true) {
        if (name_lines.at(0).length() == 0)
            break;
        const int line_num = vp.first_line_num + current_line_num++;
        const std::string line_numStr = int_to_str_leading_zeros(line_num, line_num_str_length);
        const std::string line_numNumberPart = "| " + line_numStr + " |";
        const std::string line_numEmptyPart = "|" + std::string(line_numStr.length() + 2, ' ') + "|";
        const std::string line_numBottomPart = "|" + std::string(line_numStr.length() + 2, '_') + "|";

        if (name_lines.at(0).length() >= word_text_len) {
            for (uint32_t i = 0; i < vp.name_lines_count; ++i) {
                const std::string line_without_line_num = name_lines.at(i).substr(0, word_text_len) + "\n";
                if (i == 0)
                    result += (vp.first_line_num >= 0 ? line_numNumberPart : "|") + line_without_line_num;
                else
                    result += (vp.first_line_num >= 0 ? line_numEmptyPart : "|") + line_without_line_num;

                name_lines[i] = name_lines[i].substr(word_text_len);
            }
            if (vp.print_values) {
                result += (vp.first_line_num >= 0 ? line_numEmptyPart : "|") + values_line.substr(0, word_text_len) + "\n";
                values_line = values_line.substr(word_text_len);
            }
            result += (vp.first_line_num >= 0 ? line_numBottomPart : "|") + bits_line.substr(0, word_text_len) + "\n";
            bits_line = bits_line.substr(word_text_len);
        } else {
            for (uint32_t i = 0; i < vp.name_lines_count; ++i) {
                if (i == 0)
                    result += (vp.first_line_num >= 0 ? line_numNumberPart : "|") + name_lines.at(i) + "\n";
                else
                    result += (vp.first_line_num >= 0 ? line_numEmptyPart : "|") + name_lines.at(i) + "\n";
            }
            if (vp.print_values)
                result += (vp.first_line_num >= 0 ? line_numEmptyPart : "|") + values_line.substr(0, word_text_len) + "\n";
            result += (vp.first_line_num >= 0 ? line_numBottomPart : "|") + bits_line.substr(0, word_text_len) + "\n";
            break;
        }
    }

    return result;
}

std::string protocol_serializer::get_data_visualization(const data_visualization_params& dvp) const
{
    if (m_fields.empty())
        return "";

    const_cast<data_visualization_params&>(dvp).bytes_per_line = dvp.bytes_per_line == 0 ? 1 : dvp.bytes_per_line;
    const std::string first_line_num_str = std::to_string(dvp.first_line_num);
    const size_t last_line_num = dvp.first_line_num + m_internal_buffer_length / dvp.bytes_per_line + ((m_internal_buffer_length % dvp.bytes_per_line) ? 1 : 0) - 1;
    const std::string last_line_numStr = std::to_string(last_line_num);
    const size_t line_num_str_length = std::max(first_line_num_str.length(), last_line_numStr.length());
    unsigned int current_bytes_on_line = 0;

    std::string current_line_text;
    std::string result;
    unsigned int current_line_number = 0;
    for (unsigned int i = 0; i < m_internal_buffer_length + 1; ++i) {
        bool it_is_first_byte_in_line = false;
        if (current_bytes_on_line == dvp.bytes_per_line || i == 0 || i == m_internal_buffer_length) {
            it_is_first_byte_in_line = true;
            if (current_line_text.length())
                result += (result.empty() ? "" : "\n") + current_line_text;

            if (i == m_internal_buffer_length)
                break;

            current_bytes_on_line = 0;

            current_line_text = (dvp.first_line_num >= 0) ? (int_to_str_leading_zeros(dvp.first_line_num + current_line_number++, line_num_str_length) + ": ") : "";
        }

        char byte_text_value[32];
        memset(byte_text_value, 0, sizeof(byte_text_value));
        if (dvp.base_system == data_visualization_params::base::hex)
            sprintf_s(byte_text_value, "%x", m_working_buffer[i]);
        else if (dvp.base_system == data_visualization_params::base::dec)
            sprintf_s(byte_text_value, "%d", m_working_buffer[i]);
        else if (dvp.base_system == data_visualization_params::base::oct)
            sprintf_s(byte_text_value, "%o", m_working_buffer[i]);
        else if (dvp.base_system == data_visualization_params::base::bin)
            sprintf_s(byte_text_value, "%s%s", get_half_byte_binary()[m_working_buffer[i] >> 4].c_str(), get_half_byte_binary()[m_working_buffer[i] & 0x0F].c_str());

        std::string byte_text_valueStr(byte_text_value);
        // Add leading zeros to add up to length of 2 for HEX and length of 3 for DEC and OCT
        if(dvp.base_system == data_visualization_params::base::hex)
            byte_text_valueStr = std::string(2 - byte_text_valueStr.length(), '0') + byte_text_valueStr;
        else if(dvp.base_system != data_visualization_params::base::bin)
            byte_text_valueStr = std::string(3 - byte_text_valueStr.length(), '0') + byte_text_valueStr;

        current_line_text += (dvp.spaces_between_bytes ? (it_is_first_byte_in_line ? "" : " ") : "") + byte_text_valueStr;
        current_bytes_on_line++;
    }

    return result;
}

ez::protocol_serializer::byte_ptr_t protocol_serializer::get_field_pointer(const std::string& name) const
{
    m_prealloc_metadata_itt = m_fields_metadata.find(name);

    if (m_prealloc_metadata_itt == m_fields_metadata.cend()) {
        printf("Protocol::get_field_first_byte_pointer. There is no field '%s'!\n", name.c_str());
        return nullptr;
    }

    const field_metadata& field_init = m_prealloc_metadata_itt->second;
    return m_working_buffer + field_init.first_byte_ind;
}

ez::protocol_serializer::result_code protocol_serializer::append_field(const field_init& field_init, bool preserve_internal_buffer_values)
{
    if (m_fields_metadata.find(field_init.name) != m_fields_metadata.cend())
        return result_code::bad_input;

    if (field_init.bit_count == 0 || field_init.name.empty())
        return result_code::bad_input;

    if (field_init.vis_type == visualization_type::floating_point && field_init.bit_count != 32 && field_init.bit_count != 64)
        return result_code::not_applicable;

    unsigned int first_bit_index = 0;
    if (!m_fields.empty()) {
        const field_metadata last_field_metadata = m_fields_metadata.at(m_fields.back());
        first_bit_index = last_field_metadata.first_bit_ind + last_field_metadata.bit_count;
    }

    m_fields.push_back(field_init.name);
    m_fields_metadata.insert(fields_metadata_t::value_type(field_init.name, field_metadata(first_bit_index, field_init.bit_count, field_init.vis_type)));

    if (preserve_internal_buffer_values)
        update_internal_buffer();
    else
        reallocate_internal_buffer();

    return result_code::ok;
}

ez::protocol_serializer::result_code protocol_serializer::append_protocol(const protocol_serializer& other, bool preserve_internal_buffer_values)
{
    for (fields_metadata_t::const_iterator itt = other.m_fields_metadata.cbegin(); itt != other.m_fields_metadata.cend(); ++itt)
        if (m_fields_metadata.find(itt->first) != m_fields_metadata.cend())
            return result_code::bad_input;

    for (const std::string& field_name : other.m_fields) {
        const field_metadata& field_metadata = other.m_fields_metadata.find(field_name)->second;
        append_field(protocol_serializer::field_init{field_name, field_metadata.bit_count}, preserve_internal_buffer_values);
    }

    return result_code::ok;
}

ez::protocol_serializer::result_code ez::protocol_serializer::remove_field(const std::string& name, bool preserve_internal_buffer_values)
{
    m_prealloc_metadata_itt = m_fields_metadata.find(name);
    if (m_prealloc_metadata_itt == m_fields_metadata.end())
        return result_code::field_not_found;

    bool removed = false;
    unsigned int first_bit_index = 0;
    for (auto name_itt = m_fields.begin(); name_itt != m_fields.end();) {
        // Iterate until we find field to remove
        if (!removed) {
            if (*name_itt == name) {
                name_itt = m_fields.erase(name_itt);
                first_bit_index = m_fields_metadata.find(name)->second.first_bit_ind;
                m_fields_metadata.erase(name);
                removed = true;
            } else {
                ++name_itt;
            }
            continue;
        }

        // Field was removed. Recalculate metadata of all subsequent fields
        //(operator[] will not work here since field_metadata has no default constructor)
        fields_metadata_t::iterator metadata_itt = m_fields_metadata.find(*name_itt);
        // Should never happen
        if (metadata_itt == m_fields_metadata.end())
            return result_code::bad_input;

        field_metadata& metadata_ref = metadata_itt->second;
        metadata_ref = field_metadata(first_bit_index, metadata_ref.bit_count, metadata_ref.vis_type);
        first_bit_index += metadata_ref.bit_count;
        ++name_itt;
    }

    if (preserve_internal_buffer_values)
        update_internal_buffer();
    else
        reallocate_internal_buffer();

    return result_code::ok;
}

ez::protocol_serializer::result_code protocol_serializer::remove_last_field(bool preserve_internal_buffer_values)
{
    if (m_fields.empty())
        return result_code::not_applicable;

    m_fields_metadata.erase(m_fields_metadata.find(m_fields.back()));
    m_fields.pop_back();

    if (preserve_internal_buffer_values)
        update_internal_buffer();
    else
        reallocate_internal_buffer();

    return result_code::ok;
}

ez::protocol_serializer::result_code protocol_serializer::clear_protocol()
{
    if (m_fields.empty())
        return result_code::not_applicable;

    m_fields.clear();
    m_fields_metadata.clear();

    reallocate_internal_buffer();

    return result_code::ok;
}

void protocol_serializer::clear_working_buffer()
{
    if (m_fields.empty())
        return;

    memset(m_working_buffer, 0, m_internal_buffer_length);
}

void protocol_serializer::shift_right(byte_ptr_t buf, int len, unsigned char shift)
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

void protocol_serializer::shift_left(byte_ptr_t buf, int len, unsigned char shift)
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
    static const std::map<unsigned char, unsigned char> right_masks{
        {0,0x00},
        {1,0x01},
        {2,0x03},
        {3,0x07},
        {4,0x0F},
        {5,0x1F},
        {6,0x3F},
        {7,0x7F}
    };
    return right_masks;
}

const std::map<unsigned char, unsigned char>& protocol_serializer::get_left_masks()
{
    static const std::map<unsigned char, unsigned char> left_masks{
        {0,0x00},
        {1,0x80},
        {2,0xC0},
        {3,0xE0},
        {4,0xF0},
        {5,0xF8},
        {6,0xFC},
        {7,0xFE}
    };
    return left_masks;
}

const std::vector<std::string>& protocol_serializer::get_half_byte_binary()
{
    static const std::vector<std::string> half_byte_binary = {
        "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111",
        "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"
    };
    return half_byte_binary;
}

std::string ez::protocol_serializer::int_to_str_leading_zeros(int value, size_t length) const
{
    std::string int_string = std::to_string(value);
    int64_t num_leading_zeros = length - static_cast<int64_t>(int_string.length());

    if (value < 0)
        num_leading_zeros -= 1;

    if (num_leading_zeros > 0)
        int_string = std::string(num_leading_zeros, '0') + int_string;

    return int_string;
}

void protocol_serializer::reallocate_internal_buffer()
{
    // Drop internal buffer
    m_internal_buffer.reset(nullptr);
    m_internal_buffer_length = 0;
    if (m_fields.empty())
        return;

    // Reallocate internal buffer
    const field_metadata& last_field_metadata = m_fields_metadata.find(m_fields.back())->second;
    const unsigned int bits = last_field_metadata.first_bit_ind + last_field_metadata.bit_count;
    m_internal_buffer_length = bits / 8 + ((bits % 8) ? 1 : 0);
    m_internal_buffer.reset(new unsigned char[m_internal_buffer_length]);
    memset(m_internal_buffer.get(), 0, m_internal_buffer_length);

    m_working_buffer = m_buffer_source == buffer_source::internal ? m_internal_buffer.get() : m_external_buffer;
}

void protocol_serializer::update_internal_buffer()
{
    // Create a copy of current internal buffer
    const unsigned int old_buffer_length = m_internal_buffer_length;
    internal_buffer_ptr_t old_buffer_copy;
    if (m_internal_buffer != nullptr) {
        old_buffer_copy.reset(new unsigned char[old_buffer_length]);
        memcpy(old_buffer_copy.get(), m_internal_buffer.get(), old_buffer_length);
    }

    // Reallocate internal buffer
    reallocate_internal_buffer();

    // Fill with old values
    if (m_internal_buffer_length > 0 && old_buffer_length > 0)
        memcpy(m_internal_buffer.get(), old_buffer_copy.get(), std::min(m_internal_buffer_length, old_buffer_length));
}

void ez::protocol_serializer::set_result(result_code* result_ptr, const result_code code) const
{
    if (result_ptr == nullptr)
        return;
    *result_ptr = code;
}

protocol_serializer::field_metadata::field_metadata(const unsigned int first_bit_ind, const unsigned int bit_count, const visualization_type vis_type)
{
    this->vis_type = vis_type;
    this->first_bit_ind = first_bit_ind;
    this->bit_count = bit_count;
    bytes_count = bit_count / 8 + ((bit_count % 8) ? 1 : 0);
    first_byte_ind = first_bit_ind / 8;
    unsigned int last_byte_ind = (first_bit_ind + bit_count - 1) / 8;
    touched_bytes_count = last_byte_ind - first_byte_ind + 1;
    left_spacing = first_bit_ind % 8;
    right_spacing = (8 - (first_bit_ind + bit_count) % 8) % 8;

    first_mask = 0xFF;
    last_mask = 0xFF;

    if (touched_bytes_count == 1) {
        first_mask = ~(get_left_masks().at(left_spacing) | get_right_masks().at(right_spacing));
        return;
    }

    if (left_spacing)
        first_mask = get_right_masks().at(8 - left_spacing);

    if (right_spacing)
        last_mask = get_left_masks().at(8 - right_spacing);
}
