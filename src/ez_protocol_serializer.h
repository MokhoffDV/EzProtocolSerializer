// MIT License
//
// Copyright(c) 2024 Danila Mokhov (mokhoffdv@gmail.com)
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
//  the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef EZ_PROTOCOL_SERIALIZER
#define EZ_PROTOCOL_SERIALIZER

#include <list>
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
    enum class buffer_source
    {
        internal,
        external
    };

    enum class visualization_type
    {
        signed_integer,
        unsigned_integer,
        floating_point
    };

    enum class result_code
    {
        ok,
        bad_input,
        not_applicable,
        field_not_found
    };

    struct field_init
    {
        std::string name;
        unsigned int bit_count;
        visualization_type vis_type = visualization_type::unsigned_integer;
    };

    struct field_metadata
    {
        field_metadata(const unsigned int first_bit_index, const unsigned int bits_count, const visualization_type type = visualization_type::signed_integer);
        unsigned int first_byte_ind;
        unsigned int bytes_count;
        unsigned int touched_bytes_count;
        unsigned int first_bit_ind;
        unsigned int bit_count;
        unsigned char left_spacing;
        unsigned char right_spacing;
        unsigned char first_mask;
        unsigned char last_mask;
        visualization_type vis_type;
    };

    struct visualization_params
    {
        visualization_params& set_draw_header(const bool draw) { this->draw_header = draw; return *this; }
        visualization_params& set_first_line_num(const int num) { this->first_line_num = num; return *this; }
        visualization_params& set_horizontal_bit_margin(const unsigned int margin) { this->horizontal_bit_margin = margin; return *this; }
        visualization_params& set_name_lines_count(const unsigned int count) { this->name_lines_count = count; return *this; }
        visualization_params& set_print_values(const bool print) { this->print_values = print; return *this; }
        bool draw_header = true;
        int first_line_num = 1;
        unsigned int horizontal_bit_margin = 3;
        unsigned int name_lines_count = 2;
        bool print_values = false;
    };

    struct data_visualization_params
    {
        enum class base
        {
            bin,
            oct,
            dec,
            hex
        };

        data_visualization_params& set_first_line_num(const int num) { this->first_line_num = num; return *this; }
        data_visualization_params& set_bytes_per_line(const unsigned int count) { this->bytes_per_line = count; return *this; }
        data_visualization_params& set_base(const base b) { this->base_system = b; return *this; }
        data_visualization_params& set_spaces_between_bytes(const bool yes) { this->spaces_between_bytes = yes; return *this; }
        int first_line_num = 1;
        unsigned int bytes_per_line = 2;
        base base_system = base::hex;
        bool spaces_between_bytes = true;
    };

    using fields_list_t = std::list<std::string>;
    using fields_metadata_t = std::unordered_map<std::string, field_metadata>;
    using internal_buffer_ptr_t = std::unique_ptr<unsigned char[]>;
    using byte_ptr_t = unsigned char*;

    // Creation
    protocol_serializer(const bool is_little_endian = false,
                        const buffer_source source = buffer_source::internal,
                        byte_ptr_t const external_buffer = nullptr);
    protocol_serializer(const std::vector<field_init>& fields,
                        const bool is_little_endian = false,
                        const buffer_source source = buffer_source::internal,
                        byte_ptr_t const external_buffer = nullptr);
    protocol_serializer(const protocol_serializer& other);
    protocol_serializer& operator=(const protocol_serializer& other);
    protocol_serializer(protocol_serializer&& other) noexcept;
    protocol_serializer& operator=(protocol_serializer&& other) noexcept;

    // Protocol description
    result_code     append_field(const field_init& init, bool preserve_internal_buffer_values = true);
    result_code     append_protocol(const protocol_serializer& other, bool preserve_internal_buffer_values = true);
    result_code     remove_field(const std::string& name, bool preserve_internal_buffer_values = true);
    result_code     remove_last_field(bool preserve_internal_buffer_values = true);
    result_code     clear_protocol();
    fields_list_t   get_fields_list() const;
    field_metadata  get_field_metadata(const std::string& name) const;

    // Byte order for multi-byte integers
    void set_is_little_endian(const bool is_little_endian);
    bool get_is_little_endian() const;
    static constexpr bool get_is_host_little_endian()
    {
        union
        {
            uint16_t value;
            uint8_t bytes[2];
        } test = {0x1234};

        return test.bytes[0] == 0x34;
    }

    // Buffers
    void                         set_buffer_source(const buffer_source source);
    buffer_source                get_buffer_source() const;
    const internal_buffer_ptr_t& get_internal_buffer() const;
    unsigned int                 get_internal_buffer_length() const;
    byte_ptr_t                   get_external_buffer() const;
    void                         set_external_buffer(byte_ptr_t const external_buffer);
    byte_ptr_t                   get_working_buffer() const;
    void                         clear_working_buffer();
    byte_ptr_t                   get_field_pointer(const std::string& name) const;

    // Visualization
    std::string get_visualization(const visualization_params& vp) const;
    std::string get_data_visualization(const data_visualization_params& dvp) const;
    
    // Reading/writing
    template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    result_code write(const std::string& name, const T& value)
    {
        m_prealloc_metadata_itt = m_fields_metadata.find(name);
        if (m_prealloc_metadata_itt == m_fields_metadata.cend())
            return result_code::field_not_found;

        return _write(m_prealloc_metadata_itt->second, value);
    }

    template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    result_code write_ghost(const unsigned int field_first_bit, const unsigned int field_bit_count, const T& value)
    {
        return _write(field_metadata(field_first_bit, field_bit_count), value);
    }

    template<class Array>
    result_code write_array(const std::string& name, Array& array, const size_t size)
    {
        if (size == 0)
            return result_code::bad_input;

        m_prealloc_metadata_itt = m_fields_metadata.find(name);
        if (m_prealloc_metadata_itt == m_fields_metadata.cend())
            return result_code::field_not_found;

        const field_metadata& metadata = m_prealloc_metadata_itt->second;
        if (metadata.bit_count % size)
            return result_code::not_applicable;

        const unsigned char ghost_field_length = metadata.bit_count / size;
        for (unsigned int i = 0; i < size; ++i) {
            const unsigned int first_bit_ind = metadata.first_bit_ind + i * ghost_field_length;
            const result_code result = write_ghost(first_bit_ind, ghost_field_length, array[i]);
            if (result != result_code::ok)
                return result;
        }

        return result_code::ok;
    }

    template<class Array>
    result_code write_ghost_array(const unsigned int field_first_bit, const unsigned int field_bit_count, Array& array, const size_t size)
    {
        if (field_bit_count % size)
            return result_code::not_applicable;

        const unsigned char ghost_field_length = field_bit_count / size;
        for (unsigned int i = 0; i < size; ++i) {
            const unsigned int first_bit_ind = field_first_bit + i * ghost_field_length;
            const result_code result = set_ghost(first_bit_ind, ghost_field_length, array[i]);
            if (result != result_code::ok)
                return result;
        }
        
        return result_code::ok;
    }

    template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    T read(const std::string& name, result_code* result = nullptr) const
    {
        m_prealloc_metadata_itt = m_fields_metadata.find(name);
        if (m_prealloc_metadata_itt == m_fields_metadata.cend()) {
            set_result(result, result_code::field_not_found);
            return T{};
        }
        return _read<T>(m_prealloc_metadata_itt->second, result);
    }

    template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    T read_ghost(const unsigned int field_first_bit, const unsigned int field_bit_count, result_code* result = nullptr) const
    {
        return _read<T>(field_metadata(field_first_bit, field_bit_count), result);
    }

    template<class Array>
    void read_array(const std::string& name, Array& array, const size_t size, result_code* result = nullptr) const
    {
        using ElementType = typename std::decay<decltype(std::declval<Array>()[0])>::type;
        _read_array<Array, ElementType>(name, array, size, result);
    }

    template<class Array>
    void _read_ghost_array(const unsigned int field_first_bit, const unsigned int field_bit_count, Array& array, const size_t size, result_code* result = nullptr)
    {
        using ElementType = typename std::decay<decltype(std::declval<Array>()[0])>::type;
        _read_ghost_array<Array, ElementType>(field_first_bit, field_bit_count, array, size, result);
    }

private:
    template<class Array, class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    void _read_array(const std::string& name, Array& array, const size_t size, result_code* result = nullptr) const
    {
        m_prealloc_metadata_itt = m_fields_metadata.find(name);
        if (m_prealloc_metadata_itt == m_fields_metadata.cend()) {
            set_result(result, result_code::field_not_found);
            return;
        }

        const field_metadata& metadata = m_prealloc_metadata_itt->second;
        if (metadata.bit_count % size) {
            set_result(result, result_code::not_applicable);
            return;
        }

        const unsigned char ghost_field_length = metadata.bit_count / size;
        for (unsigned int i = 0; i < size; ++i) {
            const unsigned int first_bit_ind = metadata.first_bit_ind + i * ghost_field_length;
            result_code local_result = result_code::ok;
            array[i] = read_ghost<T>(first_bit_ind, ghost_field_length, &local_result);
            if (local_result != result_code::ok) {
                set_result(result, local_result);
                return;
            }
        }

        set_result(result, result_code::ok);
    }

    template<class Array, class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    void _read_ghost_array(const unsigned int field_first_bit, const unsigned int field_bit_count, Array& array, const size_t size, result_code* result = nullptr)
    {
        if (field_bit_count % size) {
            set_result(result, result_code::not_applicable);
            return;
        }

        const unsigned char ghost_field_length = field_bit_count / size;
        for (unsigned int i = 0; i < size; ++i) {
            const unsigned int first_bit_ind = field_first_bit + i * ghost_field_length;
            result_code local_result = result_code::ok;
            array[i] = read_ghost<T>(first_bit_ind, ghost_field_length, &local_result);
            if (local_result != result_code::ok) {
                set_result(result, local_result);
                return;
            }
        }

        set_result(result, result_code::ok);
    }

    template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    result_code _write(const field_metadata& metadata, const T& value)
    {
        if (m_is_little_endian && metadata.bit_count > 8 && metadata.bit_count % 8)
            return result_code::not_applicable;

        if (std::is_floating_point<T>::value)
            if (metadata.bit_count != 32 && metadata.bit_count != 64)
                return result_code::not_applicable;

        if (metadata.bit_count > 64)
            return result_code::not_applicable;

        if (m_working_buffer == nullptr)
            return result_code::bad_input;

        memset(m_prealloc_raw_bytes, 0, 65);
        if (std::is_integral<T>::value) {
            m_prealloc_val = value;
            m_prealloc_ptr_to_first_copyable_msb = (byte_ptr_t)&m_prealloc_val;
            if (!get_is_host_little_endian()) {
                m_prealloc_ptr_to_first_copyable_msb += sizeof(uint64_t) - metadata.bytes_count;
            }
            memcpy(m_prealloc_raw_bytes, m_prealloc_ptr_to_first_copyable_msb, metadata.bytes_count);
            if (get_is_host_little_endian() != m_is_little_endian)
                for (uint32_t i = 0; i < metadata.bytes_count / 2; ++i)
                    std::swap(m_prealloc_raw_bytes[i], m_prealloc_raw_bytes[metadata.bytes_count - 1 - i]);
        } else if (std::is_floating_point<T>::value) {
            if (metadata.bytes_count == 4) {
                float val = value;
                memcpy(m_prealloc_raw_bytes, &val, 4);
            } else if (metadata.bytes_count == 8) {
                double val = value;
                memcpy(m_prealloc_raw_bytes, &val, 8);
            }
        }

        if (metadata.left_spacing == 0 && metadata.right_spacing == 0) {
            memcpy(m_working_buffer + metadata.first_byte_ind, m_prealloc_raw_bytes, metadata.bytes_count);
            return result_code::ok;
        }

        m_prealloc_final_bytes = m_prealloc_raw_bytes;
        if (metadata.right_spacing) {
            shift_right(m_prealloc_raw_bytes, metadata.bytes_count + 1, 8 - metadata.right_spacing);
            if (unsigned char transferable_bits_count = metadata.bit_count % 8)
                if (8 - metadata.right_spacing >= transferable_bits_count)
                    m_prealloc_final_bytes = m_prealloc_raw_bytes + 1;
        }

        unsigned char mask = 0;
        for (uint32_t i = 0; i < metadata.touched_bytes_count; ++i) {
            mask = i == 0 ? metadata.first_mask : i != metadata.touched_bytes_count - 1 ? 0xFF : metadata.last_mask;
            m_working_buffer[metadata.first_byte_ind + i] &= ~mask;
            m_working_buffer[metadata.first_byte_ind + i] |= m_prealloc_final_bytes[i] & mask;
        }

        return result_code::ok;
    }

    template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    T _read(const field_metadata& metadata, result_code* result = nullptr) const
    {
        if (m_is_little_endian && metadata.bit_count > 8 && metadata.bit_count % 8) {
            set_result(result, result_code::not_applicable);
            return T{};
        }
        if (std::is_floating_point<T>::value) {
            if (metadata.bit_count != 32 && metadata.bit_count != 64) {
                set_result(result, result_code::not_applicable);
                return T{};
            }
        }

        if (metadata.bit_count > 64) {
            set_result(result, result_code::not_applicable);
            return T{};
        }

        if (m_working_buffer == nullptr) {
            set_result(result, result_code::bad_input);
            return T{};
        }

        // Copy raw data into reinterpretable buffer
        memset(m_prealloc_raw_bytes, 0, 65);
        if (get_is_host_little_endian())
            m_prealloc_final_bytes = m_prealloc_raw_bytes;
        else
            m_prealloc_final_bytes = m_prealloc_raw_bytes + 64 - metadata.touched_bytes_count;
        memcpy(m_prealloc_final_bytes, m_working_buffer + metadata.first_byte_ind, metadata.touched_bytes_count);

        // Apply masks and shift if necessary in order to align less significant bit of copied value with real less significant bit
        if (metadata.right_spacing || metadata.left_spacing) {
            m_prealloc_final_bytes[0] &= metadata.first_mask;
            if (metadata.touched_bytes_count > 1)
                m_prealloc_final_bytes[metadata.touched_bytes_count - 1] &= metadata.last_mask;

            if (metadata.right_spacing) {
                shift_right(m_prealloc_final_bytes, metadata.touched_bytes_count, metadata.right_spacing);
                m_prealloc_final_bytes += metadata.touched_bytes_count - metadata.bytes_count;
            }
        }

        // Return floating point value
        if (std::is_floating_point<T>::value) {
            if (metadata.bytes_count == 4)
                return static_cast<T>(*reinterpret_cast<float*>(m_prealloc_final_bytes));
            else if (metadata.bytes_count == 8)
                return static_cast<T>(*reinterpret_cast<double*>(m_prealloc_final_bytes));
        }

        // Swap bytes if byte orders do not match
        if (get_is_host_little_endian() != m_is_little_endian)
            for (uint32_t i = 0; i < metadata.bytes_count / 2; ++i)
                std::swap(m_prealloc_final_bytes[i], m_prealloc_final_bytes[metadata.bytes_count - i - 1]);

        // Back the pointer off for correct reinterpret cast
        if (!get_is_host_little_endian())
            m_prealloc_final_bytes = m_prealloc_raw_bytes + 64 - sizeof(T);

        // If we read a signed value, then reinterpret cast will only work if most significant bit (which determines sign)
        // of the value exactly matches expected position for type T (we could read a 3-bit signed value, then cast will not work)
        if (std::is_signed<T>::value) {
            const unsigned char shift_to_reach_most_significant_bit = 7 - (metadata.left_spacing + metadata.right_spacing) % 8;
            const bool regular_cast_is_enough = metadata.bytes_count == sizeof(T) && shift_to_reach_most_significant_bit == 7;
            if (!regular_cast_is_enough) {
                if (!get_is_host_little_endian())
                    m_prealloc_msb = m_prealloc_raw_bytes[64 - std::min(metadata.bytes_count, static_cast<unsigned int>(sizeof(T)))];
                else
                    m_prealloc_msb = m_prealloc_final_bytes[std::min(metadata.bytes_count, static_cast<unsigned int>(sizeof(T))) - 1];

                // If most significant bit is 1 then we need a little trick to return negative value (Two's complement method of representing signed integers)
                if (m_prealloc_msb & (1 << shift_to_reach_most_significant_bit)) {
                    set_result(result, result_code::ok);
                    return *reinterpret_cast<T*>(m_prealloc_final_bytes) - ((uint64_t)1 << (std::min(metadata.bit_count, static_cast<unsigned int>(sizeof(T)) * 8)));
                }
            }
        }

        set_result(result, result_code::ok);
        return *reinterpret_cast<T*>(m_prealloc_final_bytes);
    }

    std::string int_to_str_leading_zeros(int value, size_t length) const;

    void copy_from(const protocol_serializer& other);
    void move_from(protocol_serializer&& other);

    static void shift_left (byte_ptr_t buf, int len, unsigned char shift);
    static void shift_right(byte_ptr_t buf, int len, unsigned char shift);

    static const std::unordered_map<unsigned char, unsigned char>& get_right_masks();
    static const std::unordered_map<unsigned char, unsigned char>& get_left_masks();
    static const std::vector<std::string>& get_half_byte_binary();

    void reallocate_internal_buffer();
    void update_internal_buffer();

    void set_result(result_code* result_ptr, const result_code code) const;

    internal_buffer_ptr_t m_internal_buffer;
    unsigned int          m_internal_buffer_length = 0;
    byte_ptr_t            m_external_buffer = nullptr;
    byte_ptr_t            m_working_buffer = nullptr;
    buffer_source         m_buffer_source;

    using metadata_const_itt = fields_metadata_t::const_iterator;
    mutable byte_ptr_t         m_prealloc_final_bytes = nullptr;
    mutable uint64_t           m_prealloc_val = 0;
    mutable byte_ptr_t         m_prealloc_ptr_to_first_copyable_msb = nullptr; //msb - "Most significant byte"
    mutable unsigned char      m_prealloc_raw_bytes[65] = "";
    mutable unsigned char      m_prealloc_msb = 0;
    mutable metadata_const_itt m_prealloc_metadata_itt;

    fields_list_t     m_fields;
    fields_metadata_t m_fields_metadata;
    bool              m_is_little_endian;
};

}

#endif // EZ_PROTOCOL_SERIALIZER
