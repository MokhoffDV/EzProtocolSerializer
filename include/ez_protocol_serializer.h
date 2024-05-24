#ifndef EZ_PROTOCOL_SERIALIZER
#define EZ_PROTOCOL_SERIALIZER

#include <map>
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
        field_metadata(const unsigned int first_bit_ind, const unsigned int bit_count, const visualization_type vis_type = visualization_type::signed_integer);
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
                        const buffer_source buffer_source = buffer_source::internal,
                        byte_ptr_t const external_buffer = nullptr);

    protocol_serializer(const std::vector<field_init>& fields,
                        const bool is_little_endian = false,
                        const buffer_source buffer_source = buffer_source::internal,
                        byte_ptr_t const external_buffer = nullptr);
    protocol_serializer(const protocol_serializer& other);
    protocol_serializer& operator=(const protocol_serializer& other);
    protocol_serializer(protocol_serializer&& other) noexcept;
    protocol_serializer& operator=(protocol_serializer&& other) noexcept;

    // Protocol description
    result_code     append_field(const field_init& field_init, bool preserve_internal_buffer_values = true);
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
        constexpr uint16_t value = 0x1234;
        return static_cast<uint8_t>(value) == 0x34;
    }

    // Buffers
    void                         set_buffer_source(const buffer_source buffer_source);
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

    template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    result_code write_array(const std::string& name, const T* array, const size_t size)
    {
        if (size == 0)
            return result_code::bad_input;

        m_prealloc_metadata_itt = m_fields_metadata.find(name);
        if (m_prealloc_metadata_itt == m_fields_metadata.cend())
            return result_code::field_not_found;

        const field_metadata& field_init = m_prealloc_metadata_itt->second;
        if (field_init.bit_count % size)
            return result_code::not_applicable;

        const unsigned char ghost_field_length = field_init.bit_count / size;
        for (unsigned int i = 0; i < size; ++i) {
            const unsigned int first_bit_ind = field_init.first_bit_ind + i * ghost_field_length;
            const result_code result = write_ghost(first_bit_ind, ghost_field_length, array[i]);
            if (result != result_code::ok)
                return result;
        }

        return result_code::ok;
    }

    template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    result_code write_ghost_array(const unsigned int field_first_bit, const unsigned int field_bit_count, const T* array, const size_t size)
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

    template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    void read_array(const std::string& name, T* array, const size_t size, result_code* result = nullptr) const
    {
        m_prealloc_metadata_itt = m_fields_metadata.find(name);
        if (m_prealloc_metadata_itt == m_fields_metadata.cend()) {
            set_result(result, result_code::field_not_found);
            return;
        }

        const field_metadata& field_init = m_prealloc_metadata_itt->second;
        if (field_init.bit_count % size) {
            set_result(result, result_code::not_applicable);
            return;
        }

        const unsigned char ghost_field_length = field_init.bit_count / size;
        for (unsigned int i = 0; i < size; ++i) {
            const unsigned int first_bit_ind = field_init.first_bit_ind + i * ghost_field_length;
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
    void read_ghost_array(const unsigned int field_first_bit, const unsigned int field_bit_count, T* array, const size_t size, result_code* result = nullptr)
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

private:
    template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    result_code _write(const field_metadata& field_metadata, const T& value)
    {
        if (m_is_little_endian && field_metadata.bit_count > 8 && field_metadata.bit_count % 8)
            return result_code::not_applicable;

        if (std::is_floating_point<T>::value)
            if (field_metadata.bit_count != 32 && field_metadata.bit_count != 64)
                return result_code::not_applicable;

        if (m_working_buffer == nullptr)
            return result_code::bad_input;

        if (field_metadata.bit_count > 64)
            return result_code::not_applicable;

        memset(m_prealloc_raw_bytes, 0, 65);
        if (std::is_integral<T>::value) {
            m_prealloc_val = value;
            m_prealloc_ptr_to_first_copyable_msb = (byte_ptr_t)&m_prealloc_val;
            if (!get_is_host_little_endian()) {
                m_prealloc_ptr_to_first_copyable_msb += sizeof(uint64_t) - field_metadata.bytes_count;
            }
            memcpy(m_prealloc_raw_bytes, m_prealloc_ptr_to_first_copyable_msb, field_metadata.bytes_count);
            if (get_is_host_little_endian() != m_is_little_endian)
                for (uint32_t i = 0; i < field_metadata.bytes_count / 2; ++i)
                    std::swap(m_prealloc_raw_bytes[i], m_prealloc_raw_bytes[field_metadata.bytes_count - 1 - i]);
        } else if (std::is_floating_point<T>::value) {
            if (field_metadata.bytes_count == 4) {
                float val = value;
                memcpy(m_prealloc_raw_bytes, &val, 4);
            } else if (field_metadata.bytes_count == 8) {
                double val = value;
                memcpy(m_prealloc_raw_bytes, &val, 8);
            }
        }

        if (field_metadata.left_spacing == 0 && field_metadata.right_spacing == 0) {
            memcpy(m_working_buffer + field_metadata.first_byte_ind, m_prealloc_raw_bytes, field_metadata.bytes_count);
            return result_code::ok;
        }

        m_prealloc_final_bytes = m_prealloc_raw_bytes;
        if (field_metadata.right_spacing) {
            shift_right(m_prealloc_raw_bytes, field_metadata.bytes_count + 1, 8 - field_metadata.right_spacing);
            if (unsigned char transferable_bits_count = field_metadata.bit_count % 8)
                if (8 - field_metadata.right_spacing >= transferable_bits_count)
                    m_prealloc_final_bytes = m_prealloc_raw_bytes + 1;
        }

        unsigned char mask = 0;
        for (uint32_t i = 0; i < field_metadata.touched_bytes_count; ++i) {
            mask = i == 0 ? field_metadata.first_mask : i != field_metadata.touched_bytes_count - 1 ? 0xFF : field_metadata.last_mask;
            m_working_buffer[field_metadata.first_byte_ind + i] &= ~mask;
            m_working_buffer[field_metadata.first_byte_ind + i] |= m_prealloc_final_bytes[i] & mask;
        }

        return result_code::ok;
    }

    template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    T _read(const field_metadata& field_metadata, result_code* result = nullptr) const
    {
        if (m_is_little_endian && field_metadata.bit_count > 8 && field_metadata.bit_count % 8) {
            set_result(result, result_code::not_applicable);
            return T{};
        }
        if (std::is_floating_point<T>::value) {
            if (field_metadata.bit_count != 32 && field_metadata.bit_count != 64) {
                set_result(result, result_code::not_applicable);
                return T{};
            }
        }

        if (m_working_buffer == nullptr) {
            set_result(result, result_code::bad_input);
            return T{};
        }

        if (field_metadata.bit_count > 64) {
            set_result(result, result_code::not_applicable);
            return T{};
        }

        memset(m_prealloc_raw_bytes, 0, 65);
        memcpy(m_prealloc_raw_bytes, m_working_buffer + field_metadata.first_byte_ind, field_metadata.touched_bytes_count);

        m_prealloc_final_bytes = m_prealloc_raw_bytes;
        m_prealloc_final_bytes_count = field_metadata.bytes_count;
        if (field_metadata.right_spacing || field_metadata.left_spacing) {
            m_prealloc_raw_bytes[0] &= field_metadata.first_mask;
            if (field_metadata.touched_bytes_count > 1)
                m_prealloc_raw_bytes[field_metadata.touched_bytes_count - 1] &= field_metadata.last_mask;

            if (field_metadata.right_spacing) {
                shift_right(m_prealloc_raw_bytes, field_metadata.touched_bytes_count, field_metadata.right_spacing);
                m_prealloc_final_bytes = m_prealloc_raw_bytes + field_metadata.touched_bytes_count - field_metadata.bytes_count;
            }
        }

        if (std::is_floating_point<T>::value) {
            if (field_metadata.bytes_count == 4)
                return static_cast<T>(*reinterpret_cast<float*>(m_prealloc_final_bytes));
            else if (field_metadata.bytes_count == 8)
                return static_cast<T>(*reinterpret_cast<double*>(m_prealloc_final_bytes));
        }

        if (get_is_host_little_endian() != m_is_little_endian)
            for (uint32_t i = 0; i < field_metadata.bytes_count / 2; ++i)
                std::swap(m_prealloc_final_bytes[i], m_prealloc_final_bytes[field_metadata.bytes_count - i - 1]);

        if (field_metadata.bytes_count > sizeof(T)) {
            if (!get_is_host_little_endian()) {
                m_prealloc_final_bytes += field_metadata.bytes_count - sizeof(T);
                m_prealloc_final_bytes_count -= field_metadata.bytes_count - sizeof(T);
            }
        }

        if (std::is_signed<T>::value) {
            const unsigned char shift_to_reach_most_significant_bit = 7 - (field_metadata.left_spacing + field_metadata.right_spacing) % 8;
            const bool regular_cast_is_enough = m_prealloc_final_bytes_count == sizeof(T) && shift_to_reach_most_significant_bit == 7;
            if (!regular_cast_is_enough) {
                if (!get_is_host_little_endian()) {
                    if (m_prealloc_final_bytes[0] & (1 << shift_to_reach_most_significant_bit)) {
                        set_result(result, result_code::ok);
                        return *reinterpret_cast<T*>(m_prealloc_final_bytes) - ((uint64_t)1 << (std::min(m_prealloc_final_bytes_count * 8, field_metadata.bit_count)));
                    }
                } else {
                    if (m_prealloc_final_bytes[m_prealloc_final_bytes_count - 1] & (1 << shift_to_reach_most_significant_bit)) {
                        set_result(result, result_code::ok);
                        return *reinterpret_cast<T*>(m_prealloc_final_bytes) - ((uint64_t)1 << (std::min(m_prealloc_final_bytes_count * 8, field_metadata.bit_count)));
                    }
                }
            }
        }

        set_result(result, result_code::ok);
        return *reinterpret_cast<T*>(m_prealloc_final_bytes);
    }

    template<typename... Args>
    static std::string format_string(const std::string& format, Args... args)
    {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
        if (size_s <= 0) return format;
        std::unique_ptr<char[]> buf(new char[size_s]);
        std::snprintf(buf.get(), size_s, format.c_str(), args...);
        return std::string(buf.get(), buf.get() + size_s - 1);
    }

    std::string int_to_str_leading_zeros(int value, size_t length) const;

    void copy_from(const protocol_serializer& other);
    void move_from(protocol_serializer&& other);

    static void shift_left(byte_ptr_t buf, int len, unsigned char shift);
    static void shift_right(byte_ptr_t buf, int len, unsigned char shift);

    static const std::map<unsigned char, unsigned char>& get_right_masks();
    static const std::map<unsigned char, unsigned char>& get_left_masks();
    static const std::vector<std::string>&               get_half_byte_binary();

    void reallocate_internal_buffer();
    void update_internal_buffer();

    void set_result(result_code* result_ptr, const result_code code) const;

    internal_buffer_ptr_t m_internal_buffer;
    unsigned int m_internal_buffer_length = 0;
    byte_ptr_t m_external_buffer = nullptr;
    byte_ptr_t m_working_buffer = nullptr;
    buffer_source m_buffer_source;

    mutable byte_ptr_t m_prealloc_final_bytes = nullptr;
    mutable unsigned int m_prealloc_final_bytes_count = 0;
    mutable uint64_t m_prealloc_val = 0;
    mutable byte_ptr_t m_prealloc_ptr_to_first_copyable_msb = nullptr; //msb - "Most significant byte"
    mutable unsigned char m_prealloc_raw_bytes[65] = "";
    mutable fields_metadata_t::const_iterator m_prealloc_metadata_itt;

    fields_list_t m_fields;
    fields_metadata_t m_fields_metadata;
    bool m_is_little_endian;
};

}

#endif // EZ_PROTOCOL_SERIALIZER
