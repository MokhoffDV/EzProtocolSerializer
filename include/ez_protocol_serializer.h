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

    struct field_init
    {
        std::string name;
        unsigned int bit_count;
        visualization_type vis_type = visualization_type::unsigned_integer;
    };

    struct field_metadata
    {
        field_metadata(const unsigned int first_bit_ind, const unsigned int bit_count, const std::string& name, const visualization_type vis_type = visualization_type::signed_integer);
        unsigned int first_byte_ind;
        unsigned int bytes_count;
        unsigned int touched_bytes_count;
        unsigned int first_bit_ind;
        unsigned int bit_count;
        unsigned char left_spacing;
        unsigned char right_spacing;
        unsigned char first_mask;
        unsigned char last_mask;
        std::string name;
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
    protocol_serializer(const bool isLittleEndian = false,
                        const buffer_source bufferSource = buffer_source::internal,
                        byte_ptr_t const externalBuffer = nullptr);

    protocol_serializer(const std::vector<field_init>& fields,
                        const bool isLittleEndian = false,
                        const buffer_source bufferSource = buffer_source::internal,
                        byte_ptr_t const externalBuffer = nullptr);
    protocol_serializer(const protocol_serializer& other);
    protocol_serializer& operator=(const protocol_serializer& other);
    protocol_serializer(protocol_serializer&& other) noexcept;
    protocol_serializer& operator=(protocol_serializer&& other) noexcept;

    // Protocol description
    bool           append_field(const field_init& field_init, bool preserveInternalBufferValues = true);
    bool           append_protocol(const protocol_serializer& other, bool preserveInternalBufferValues = true);
    void           remove_field(const std::string& name, bool preserveInternalBufferValues = true);
    void           remove_last_field(bool preserveInternalBufferValues = true);
    void           clear_protocol();
    fields_list_t  get_fields_list() const;
    field_metadata get_field_metadata(const std::string& name) const;

    // Byte order for multi-byte integers
    void set_is_little_endian(const bool isLittleEndian);
    bool get_is_little_endian() const;

    // Buffers
    void                         set_buffer_source(const buffer_source bufferSource);
    buffer_source                get_buffer_source() const;
    const internal_buffer_ptr_t& get_internal_buffer() const;
    unsigned int                 get_internal_buffer_length() const;
    byte_ptr_t                   get_external_buffer() const;
    void                         set_external_buffer(byte_ptr_t const externalBuffer);
    byte_ptr_t                   get_working_buffer() const;
    void                         clear_working_buffer();
    byte_ptr_t                   get_field_pointer(const std::string& name) const;

    // Visualization
    std::string get_visualization(const visualization_params& vp) const;
    std::string get_data_visualization(const data_visualization_params& dvp) const;
    
    // Reading/writing
    template<class T>
    void write(const std::string& name, const T& value, std::string* errorString = nullptr)
    {
        m_prealloc_metadata_itt = m_fields_metadata.find(name);
        if (m_prealloc_metadata_itt == m_fields_metadata.cend()) {
            if (errorString != nullptr)
                *errorString = format_string("Protocol::setFieldValue. There is no field '%s'!\n", name.c_str());
            return;
        }
        _write(m_prealloc_metadata_itt->second, value, errorString);
    }

    template<class T>
    void writeGhost(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, const T& value, std::string* errorString = nullptr)
    {
        _write(field_metadata(fieldFirstBit, fieldBitCount, "<field_ghost>"), value, errorString);
    }

    template<class T, size_t N>
    void writeArray(const std::string& name, const T array[N], std::string* errorString = nullptr)
    {
        m_prealloc_metadata_itt = m_fields_metadata.find(name);
        if (m_prealloc_metadata_itt == m_fields_metadata.cend()) {
            if (errorString != nullptr)
                *errorString = format_string("Protocol::setFieldValueAsArray.There is no field '%s'!\n", name.c_str());
            return;
        }

        const field_metadata& field_init = m_prealloc_metadata_itt->second;
        if (field_init.bit_count % N) {
            if (errorString != nullptr) *errorString = format_string("Protocol::setFieldValueAsArray. Field length (%d bits) is not divisible between %d elements!",
                                                                           name.c_str(), field_init.bit_count, N);
            return;
        }

        const unsigned char ghostFieldLength = field_init.bit_count / N;

        std::string localerror;
        for (unsigned int i = 0; i < N; ++i) {
            if (localerror.length()) break;

            unsigned int first_bit_ind = field_init.first_bit_ind + i * ghostFieldLength;
            setGhost(first_bit_ind, ghostFieldLength, array[i], &localerror);
        }
        if (localerror.length())
            *errorString = localerror;
    }

    template<class T, size_t N>
    void writeGhostArray(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, const T array[N], std::string* errorString = nullptr)
    {
        if (fieldBitCount % N) {
            if (errorString != nullptr) *errorString = format_string("Protocol::setFieldValueAsArray. Ghost field length (%d bits) is not divisible between %d elements!",
                                                                           fieldBitCount, N);
            return;
        }

        const unsigned char ghostFieldLength = fieldBitCount / N;

        std::string localerror;
        for (unsigned int i = 0; i < N; ++i) {
            if (localerror.length()) break;

            unsigned int first_bit_ind = fieldFirstBit + i * ghostFieldLength;
            setGhost(first_bit_ind, ghostFieldLength, array[i], &localerror);
        }
        if (localerror.length())
            *errorString = localerror;
    }

    template<class T>
    T read(const std::string& name, std::string* errorString = nullptr) const
    {
        m_prealloc_metadata_itt = m_fields_metadata.find(name);
        if (m_prealloc_metadata_itt == m_fields_metadata.cend()) {
            if (errorString != nullptr)
                *errorString = format_string("Protocol::readFieldValue. There is no field '%s'!\n", name.c_str());
            return T{};
        }
        return _read<T>(m_prealloc_metadata_itt->second, errorString);
    }

    template<class T>
    T readGhost(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, std::string* errorString = nullptr) const
    {
        return _read<T>(field_metadata(fieldFirstBit, fieldBitCount, "<field_ghost>"), errorString);
    }

    template<class T, size_t N>
    void readArray(const std::string& name, T array[N], std::string* errorString = nullptr) const
    {
        m_prealloc_metadata_itt = m_fields_metadata.find(name);
        if (m_prealloc_metadata_itt == m_fields_metadata.cend()) {
            if (errorString != nullptr)
                *errorString = format_string("Protocol::readFieldValueAsArray. There is no field '%s'!\n", name.c_str());
            return;
        }

        const field_metadata& field_init = m_prealloc_metadata_itt->second;
        if (field_init.bit_count % N) {
            if (errorString != nullptr) *errorString = format_string("Protocol::readFieldValueAsArray. Field length of '%s', equals to %d bits, is not divisible between %d elements!",
                                                                           name.c_str(), field_init.bit_count, N);
            return;
        }

        const unsigned char ghostFieldLength = field_init.bit_count / N;

        std::string localerror;
        for (unsigned int i = 0; i < N; ++i) {
            if (localerror.length()) break;

            unsigned int first_bit_ind = field_init.first_bit_ind + i * ghostFieldLength;
            array[i] = readGhost<T>(first_bit_ind, ghostFieldLength, &localerror);
        }
        if (localerror.length())
            *errorString = localerror;
    }

    template<class T, size_t N>
    void readGhostArray(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, T array[N], std::string* errorString = nullptr)
    {
        if (fieldBitCount % N) {
            if (errorString != nullptr)
                *errorString = format_string("Protocol::readFieldValueAsArray. Ghost field length (%d bits) is not divisible between %d elements!", fieldBitCount, N);
            return;
        }

        const unsigned char ghostFieldLength = fieldBitCount / N;

        std::string localerror;
        for (unsigned int i = 0; i < N; ++i) {
            if (localerror.length()) break;

            unsigned int first_bit_ind = fieldFirstBit + i * ghostFieldLength;
            array[i] = readGhost<T>(first_bit_ind, ghostFieldLength, &localerror);
        }
        if (localerror.length())
            *errorString = localerror;
    }

private:
    template<class T>
    void _write(const field_metadata& fieldMetadata, const T& value, std::string* errorString = nullptr)
    {
        static_assert(std::is_arithmetic<T>(), "Protocol::setFieldValue. T should be arithmetic!");

        if (m_is_little_endian && fieldMetadata.bit_count > 8 && fieldMetadata.bit_count % 8) {
            if (errorString != nullptr) *errorString = format_string("Protocol::setFieldValue. Field '%s' (length %d) is longer than 8 bits, and is not divisible by 8!", fieldMetadata.name.c_str(), fieldMetadata.bit_count);
            return;
        }

        if (std::is_floating_point<T>::value) {
            if (fieldMetadata.bit_count != 32 && fieldMetadata.bit_count != 64) {
                if (errorString != nullptr)
                    *errorString = format_string("Protocol::setFieldValue. Field '%s' (length %d) is being written as floating point, while having length of not 32 or 64!", fieldMetadata.name.c_str(), fieldMetadata.bit_count);
                return;
            }
        }

        if (m_working_buffer == nullptr) {
            if (errorString != nullptr) *errorString = format_string("Protocol::setFieldValue. Current buffer (buffer_source::%s) == nullptr!", m_buffer_source == buffer_source::external ? "external" : "internal");
            return;
        }

        if (fieldMetadata.bit_count > 64) {
            if (errorString != nullptr) *errorString = format_string("Protocol::setFieldValue. Field '%s' is longer tha 64 bits!", fieldMetadata.name.c_str(), fieldMetadata.bit_count);
            return;
        }

        memset(m_prealloc_raw_bytes, 0, 65);
        if (std::is_integral<T>::value) {
            m_prealloc_val = value;
            m_prealloc_ptr_to_first_copyable_msb = (byte_ptr_t)&m_prealloc_val;
            if (!get_is_host_little_endian()) {
                m_prealloc_ptr_to_first_copyable_msb += sizeof(uint64_t) - fieldMetadata.bytes_count;
            }
            memcpy(m_prealloc_raw_bytes, m_prealloc_ptr_to_first_copyable_msb, fieldMetadata.bytes_count);
            if (get_is_host_little_endian() != m_is_little_endian)
                for (uint32_t i = 0; i < fieldMetadata.bytes_count / 2; ++i)
                    std::swap(m_prealloc_raw_bytes[i], m_prealloc_raw_bytes[fieldMetadata.bytes_count - 1 - i]);
        } else if (std::is_floating_point<T>::value) {
            if (fieldMetadata.bytes_count == 4) {
                float val = value;
                memcpy(m_prealloc_raw_bytes, &val, 4);
            } else if (fieldMetadata.bytes_count == 8) {
                double val = value;
                memcpy(m_prealloc_raw_bytes, &val, 8);
            }
        }

        if (fieldMetadata.left_spacing == 0 && fieldMetadata.right_spacing == 0) {
            memcpy(m_working_buffer + fieldMetadata.first_byte_ind, m_prealloc_raw_bytes, fieldMetadata.bytes_count);
            return;
        }

        m_prealloc_final_bytes = m_prealloc_raw_bytes;
        if (fieldMetadata.right_spacing) {
            shift_right(m_prealloc_raw_bytes, fieldMetadata.bytes_count + 1, 8 - fieldMetadata.right_spacing);
            if (unsigned char transferableBitsCount = fieldMetadata.bit_count % 8)
                if (8 - fieldMetadata.right_spacing >= transferableBitsCount)
                    m_prealloc_final_bytes = m_prealloc_raw_bytes + 1;
        }

        unsigned char mask = 0;
        for (uint32_t i = 0; i < fieldMetadata.touched_bytes_count; ++i) {
            mask = i == 0 ? fieldMetadata.first_mask : i != fieldMetadata.touched_bytes_count - 1 ? 0xFF : fieldMetadata.last_mask;
            m_working_buffer[fieldMetadata.first_byte_ind + i] &= ~mask;
            m_working_buffer[fieldMetadata.first_byte_ind + i] |= m_prealloc_final_bytes[i] & mask;
        }
    }

    template<class T>
    T _read(const field_metadata& fieldMetadata, std::string* errorString = nullptr) const
    {
        static_assert(std::is_arithmetic<T>(), "Protocol::readFieldValue. T should be arithmetic!");

        if (m_is_little_endian && fieldMetadata.bit_count > 8 && fieldMetadata.bit_count % 8) {
            if (errorString != nullptr) *errorString = format_string("Protocol::readFieldValue. Field '%s' (length %d) is longer than 8 bits, and is not divisible by 8!", fieldMetadata.name.c_str(), fieldMetadata.bit_count);
            return T{};
        }
        if (std::is_floating_point<T>::value) {
            if (fieldMetadata.bit_count != 32 && fieldMetadata.bit_count != 64) {
                if (errorString != nullptr) *errorString = format_string("Protocol::readFieldValue. Field '%s' (length %d) is being read as floating point, while having length of not 32 or 64!", fieldMetadata.name.c_str(), fieldMetadata.bit_count);
                return T{};
            }
        }

        if (m_working_buffer == nullptr) {
            if (errorString != nullptr) *errorString = format_string("Protocol::setFieldValue. Current buffer (buffer_source::%s) == nullptr!", m_buffer_source == buffer_source::external ? "external" : "internal");
            return T{};
        }

        if (fieldMetadata.bit_count > 64) {
            if (errorString != nullptr) *errorString = format_string("Protocol::setFieldValue. Field '%s' is longer tha 64 bits!", fieldMetadata.name.c_str(), fieldMetadata.bit_count);
            return T{};
        }

        memset(m_prealloc_raw_bytes, 0, 65);
        memcpy(m_prealloc_raw_bytes, m_working_buffer + fieldMetadata.first_byte_ind, fieldMetadata.touched_bytes_count);

        m_prealloc_final_bytes = m_prealloc_raw_bytes;
        m_prealloc_final_bytes_count = fieldMetadata.bytes_count;
        if (fieldMetadata.right_spacing || fieldMetadata.left_spacing) {
            m_prealloc_raw_bytes[0] &= fieldMetadata.first_mask;
            if (fieldMetadata.touched_bytes_count > 1)
                m_prealloc_raw_bytes[fieldMetadata.touched_bytes_count - 1] &= fieldMetadata.last_mask;

            if (fieldMetadata.right_spacing) {
                shift_right(m_prealloc_raw_bytes, fieldMetadata.touched_bytes_count, fieldMetadata.right_spacing);
                m_prealloc_final_bytes = m_prealloc_raw_bytes + fieldMetadata.touched_bytes_count - fieldMetadata.bytes_count;
            }
        }

        if (std::is_floating_point<T>::value) {
            if (fieldMetadata.bytes_count == 4)
                return static_cast<T>(*reinterpret_cast<float*>(m_prealloc_final_bytes));
            else if (fieldMetadata.bytes_count == 8)
                return static_cast<T>(*reinterpret_cast<double*>(m_prealloc_final_bytes));
        }

        if (get_is_host_little_endian() != m_is_little_endian)
            for (uint32_t i = 0; i < fieldMetadata.bytes_count / 2; ++i)
                std::swap(m_prealloc_final_bytes[i], m_prealloc_final_bytes[fieldMetadata.bytes_count - i - 1]);

        if (fieldMetadata.bytes_count > sizeof(T)) {
            if (!get_is_host_little_endian()) {
                m_prealloc_final_bytes += fieldMetadata.bytes_count - sizeof(T);
                m_prealloc_final_bytes_count -= fieldMetadata.bytes_count - sizeof(T);
            }
        }

        if (std::is_signed_v<T>) {
            if (!get_is_host_little_endian()) {
                if (m_prealloc_final_bytes[0] & (1 << (7 - (fieldMetadata.left_spacing + fieldMetadata.right_spacing) % 8)))
                    return (*reinterpret_cast<T*>(m_prealloc_final_bytes)) - ((uint64_t)1 << (std::min(m_prealloc_final_bytes_count * 8, fieldMetadata.bit_count)));
            } else {
                if (fieldMetadata.bit_count < 8) {
                    if (m_prealloc_final_bytes[m_prealloc_final_bytes_count - 1] & (1 << (7 - (fieldMetadata.left_spacing + fieldMetadata.right_spacing) % 8)))
                        return (*reinterpret_cast<T*>(m_prealloc_final_bytes)) - ((uint64_t)1 << (std::min(m_prealloc_final_bytes_count * 8, fieldMetadata.bit_count)));
                } else {
                    if (m_prealloc_final_bytes[m_prealloc_final_bytes_count - 1] & (1 << (7 - (fieldMetadata.left_spacing + fieldMetadata.right_spacing) % 8))) {
                        return (*reinterpret_cast<T*>(m_prealloc_final_bytes)) - ((uint64_t)1 << (std::min(m_prealloc_final_bytes_count * 8, fieldMetadata.bit_count)));
                    }
                }
            }
        }

        return *reinterpret_cast<T*>(m_prealloc_final_bytes);
    }

    static constexpr bool get_is_host_little_endian()
    {
        constexpr uint16_t value = 0x1234;
        return static_cast<uint8_t>(value) == 0x34;
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
