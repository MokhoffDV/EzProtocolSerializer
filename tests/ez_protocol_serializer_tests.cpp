#include <gtest/gtest.h>
#include <ez_protocol_serializer.h>

using ez::protocol_serializer;
using buffer_source = ez::protocol_serializer::buffer_source;
using result_code = ez::protocol_serializer::result_code;

template<class T>
std::vector<T> generateEquallySpreadValues(T min, T max)
{
    // Generates vector which consists of: {min, max, <20 values between min and max>...}
    std::vector<T> result{min, max};

    // No values in between min and max
    if (max - min <= 1)
        return result;

    const T count = max - min - 1;
    if (count <= 20) {
        // If there are 20 or fewer values between min and max, return them all
        for (T i = min + 1; i < max; ++i) {
            result.push_back(i);
        }
        return result;
    }

    // If there are more than 20 values, compute 20 equally spread values
    const double step = static_cast<double>(count) / 21.0; // divide into 21 segments to get 20 intervals
    for (int i = 1; i <= 20; ++i) {
        const T value = min + static_cast<T>(std::round(i * step));
        result.push_back(value);
    }

    return result;
}

template<class T>
void checkNumericLimitsOf(unsigned int offset)
{
    EXPECT_NE(offset, 0);

    constexpr T numericLimitMin = std::numeric_limits<T>::min();
    constexpr T numericLimitMax = std::numeric_limits<T>::max();

    ez::protocol_serializer ps({{"offset", offset}, {"min", sizeof(T) * 8}, {"max", sizeof(T) * 8}});
    ps.write("min", numericLimitMin);
    ps.write("max", numericLimitMax);
    EXPECT_EQ(ps.read<T>("min"), numericLimitMin);
    EXPECT_EQ(ps.read<T>("max"), numericLimitMax);
}

void checkValuesRange(unsigned int offset, unsigned int bitCount)
{
    EXPECT_NE(offset, 0);
    EXPECT_NE(bitCount, 0);

    ez::protocol_serializer ps({{"offset", offset}, {"value", bitCount}});

    // Check unsigned limits for this bit count
    {
        uint64_t min = 0;
        uint64_t max = 0;
        for (unsigned int i = 0; i < bitCount - 1; ++i)
            max = (max << 1) + 1;

        for (const uint64_t value : generateEquallySpreadValues<uint64_t>(min, max)) {
            ps.write("value", value);
            EXPECT_EQ(ps.read<uint64_t>("value"), value);
        }
    }
    // Check signed limits for this bit count
    {
        int64_t min = 0;
        int64_t max = 0;
        for (unsigned int i = 0; i < bitCount - 1; ++i)
            max = (max << 1) + 1;
        min = -max - 1;

        for (const int64_t value : generateEquallySpreadValues<int64_t>(min, max)) {
            ps.write("value", value);
            EXPECT_EQ(ps.read<int64_t>("value"), value);
        }
    }
}

TEST(Constructing, DefaultConstructor)
{
    protocol_serializer ps;
    EXPECT_EQ(ps.get_is_little_endian(), false);
    EXPECT_EQ(ps.get_buffer_source(), protocol_serializer::buffer_source::internal);
    EXPECT_EQ(ps.get_internal_buffer().get(), ps.get_working_buffer());
    EXPECT_EQ(ps.get_internal_buffer_length(), 0);
    EXPECT_EQ(ps.get_fields_list().size(), 0);
}

TEST(Constructing, Constructor1)
{
    unsigned char externalBuffer[32];
    memset(externalBuffer, 1, 32);
    protocol_serializer ps({
                                {"field_1", 8},
                                {"field_2", 8},
                                {"field_3", 3},
                           }, true, buffer_source::external, externalBuffer);
    EXPECT_EQ(ps.get_is_little_endian(), true);
    EXPECT_EQ(ps.get_buffer_source(), protocol_serializer::buffer_source::external);
    EXPECT_NE(ps.get_internal_buffer().get(), nullptr);
    EXPECT_EQ(ps.get_external_buffer(), externalBuffer);
    EXPECT_EQ(ps.get_working_buffer(), ps.get_external_buffer());
    EXPECT_EQ(ps.get_internal_buffer_length(), 3);
    EXPECT_EQ(ps.get_fields_list().size(), 3);

    const int intialField2Value = ps.read<int>("field_2");
    memset(externalBuffer, 2, 32);
    const int newField2Value = ps.read<int>("field_2");
    EXPECT_NE(intialField2Value, newField2Value);
    ps.append_field({"field_4", 5}, true);
    const int newerField2Value = ps.read<int>("field_2");
    EXPECT_EQ(newField2Value, newerField2Value);
}

TEST(Constructing, Constructor2)
{
    protocol_serializer ps({
                                {"field_1", 8},
                                {"field_2", 8},
                                {"field_3", 3},
                           }, true, buffer_source::internal);
    EXPECT_EQ(ps.get_is_little_endian(), true);
    EXPECT_EQ(ps.get_buffer_source(), protocol_serializer::buffer_source::internal);
    EXPECT_NE(ps.get_internal_buffer().get(), nullptr);
    EXPECT_EQ(ps.get_external_buffer(), nullptr);
    EXPECT_EQ(ps.get_internal_buffer_length(), 3);
    EXPECT_EQ(ps.get_fields_list().size(), 3);
}

TEST(Constructing, Copy)
{
    protocol_serializer ps({
                                {"field_1", 8},
                                {"field_2", 8},
                                {"field_3", 3},
                           }, true, buffer_source::internal);
    EXPECT_EQ(ps.get_is_little_endian(), true);
    EXPECT_EQ(ps.get_buffer_source(), protocol_serializer::buffer_source::internal);
    EXPECT_NE(ps.get_internal_buffer().get(), nullptr);
    EXPECT_EQ(ps.get_external_buffer(), nullptr);
    EXPECT_EQ(ps.get_working_buffer(), ps.get_internal_buffer().get());
    EXPECT_EQ(ps.get_internal_buffer_length(), 3);
    EXPECT_EQ(ps.get_fields_list().size(), 3);

    // Copy constructor
    protocol_serializer psCopyConstructor(ps);
    EXPECT_EQ(psCopyConstructor.get_is_little_endian(), ps.get_is_little_endian());
    EXPECT_EQ(psCopyConstructor.get_buffer_source(), ps.get_buffer_source());
    EXPECT_NE(psCopyConstructor.get_internal_buffer().get(), ps.get_internal_buffer().get());
    EXPECT_EQ(psCopyConstructor.get_external_buffer(), ps.get_external_buffer());
    EXPECT_EQ(psCopyConstructor.get_internal_buffer_length(), ps.get_internal_buffer_length());
    EXPECT_EQ(psCopyConstructor.get_fields_list().size(), ps.get_fields_list().size());

    // Copy assignment operator
    protocol_serializer psCopyAssignment(psCopyConstructor);
    EXPECT_EQ(psCopyAssignment.get_is_little_endian(), psCopyConstructor.get_is_little_endian());
    EXPECT_EQ(psCopyAssignment.get_buffer_source(), psCopyConstructor.get_buffer_source());
    EXPECT_NE(psCopyAssignment.get_internal_buffer().get(), psCopyConstructor.get_internal_buffer().get());
    EXPECT_EQ(psCopyAssignment.get_external_buffer(), psCopyConstructor.get_external_buffer());
    EXPECT_EQ(psCopyAssignment.get_internal_buffer_length(), psCopyConstructor.get_internal_buffer_length());
    EXPECT_EQ(psCopyAssignment.get_fields_list().size(), psCopyConstructor.get_fields_list().size());
}

TEST(Constructing, Move)
{
    protocol_serializer ps({
                                {"field_1", 8},
                                {"field_2", 8},
                                {"field_3", 3},
                           }, true, buffer_source::internal);
    EXPECT_EQ(ps.get_is_little_endian(), true);
    EXPECT_EQ(ps.get_buffer_source(), protocol_serializer::buffer_source::internal);
    EXPECT_NE(ps.get_internal_buffer().get(), nullptr);
    EXPECT_EQ(ps.get_external_buffer(), nullptr);
    EXPECT_EQ(ps.get_internal_buffer_length(), 3);
    EXPECT_EQ(ps.get_fields_list().size(), 3);

    // Move constructor
    protocol_serializer psMoveConstructor(std::move(ps));
    EXPECT_EQ(psMoveConstructor.get_is_little_endian(), true);
    EXPECT_EQ(psMoveConstructor.get_buffer_source(), protocol_serializer::buffer_source::internal);
    EXPECT_NE(psMoveConstructor.get_internal_buffer().get(), nullptr);
    EXPECT_EQ(psMoveConstructor.get_external_buffer(), nullptr);
    EXPECT_EQ(psMoveConstructor.get_internal_buffer_length(), 3);
    EXPECT_EQ(psMoveConstructor.get_fields_list().size(), 3);
    // Check if it is now empty
    EXPECT_EQ(ps.get_is_little_endian(), true);
    EXPECT_EQ(ps.get_buffer_source(), protocol_serializer::buffer_source::internal);
    EXPECT_EQ(ps.get_internal_buffer().get(), nullptr);
    EXPECT_EQ(ps.get_external_buffer(), nullptr);
    EXPECT_EQ(ps.get_internal_buffer_length(), 0);
    EXPECT_EQ(ps.get_fields_list().size(), 0);

    // Move assignment operator
    protocol_serializer psMoveAssignment;
    psMoveAssignment = std::move(psMoveConstructor);
    EXPECT_EQ(psMoveAssignment.get_is_little_endian(), true);
    EXPECT_EQ(psMoveAssignment.get_buffer_source(), protocol_serializer::buffer_source::internal);
    EXPECT_NE(psMoveAssignment.get_internal_buffer().get(), nullptr);
    EXPECT_EQ(psMoveAssignment.get_external_buffer(), nullptr);
    EXPECT_EQ(psMoveAssignment.get_internal_buffer_length(), 3);
    EXPECT_EQ(psMoveAssignment.get_fields_list().size(), 3);
    // Check if it is now empty
    EXPECT_EQ(psMoveConstructor.get_is_little_endian(), true);
    EXPECT_EQ(psMoveConstructor.get_buffer_source(), protocol_serializer::buffer_source::internal);
    EXPECT_EQ(psMoveConstructor.get_internal_buffer().get(), nullptr);
    EXPECT_EQ(psMoveConstructor.get_external_buffer(), nullptr);
    EXPECT_EQ(psMoveConstructor.get_internal_buffer_length(), 0);
    EXPECT_EQ(psMoveConstructor.get_fields_list().size(), 0);
}

TEST(Modifying, Endiannes)
{
    protocol_serializer ps;

    // Check if host endiannes is determined correctly
    unsigned char shortBytes[2];
    const uint16_t shortValue = 1;
    memcpy(shortBytes, &shortValue, 2);
    EXPECT_EQ(static_cast<bool>(shortBytes[0]), protocol_serializer::get_is_host_little_endian());

    // Check if protocol endiannes is being correctly changed
    ps.set_is_little_endian(true);
    EXPECT_EQ(ps.get_is_little_endian(), true);
    ps.set_is_little_endian(false);
    EXPECT_EQ(ps.get_is_little_endian(), false);
    ps.set_is_little_endian(true);
    EXPECT_EQ(ps.get_is_little_endian(), true);

    // Check if values are interpreted correctly depending on endiannes
    ps.set_is_little_endian(true);
    ps.append_field({"field", 16});
    ps.write("field", shortValue);
    ps.set_is_little_endian(false);
    EXPECT_EQ(ps.read<uint16_t>("field"), 256);
}

TEST(Modifying, ProtocolLayout)
{
    protocol_serializer ps;

    // Appending fields
    EXPECT_NE(ps.append_field({"field_1", 0}), result_code::ok);
    EXPECT_NE(ps.append_field({"", 8}), result_code::ok);
    EXPECT_EQ(ps.get_fields_list().size(), 0);
    EXPECT_EQ(ps.append_field({"field_1", 8}), result_code::ok);
    EXPECT_EQ(ps.get_fields_list().size(), 1);
    EXPECT_NE(ps.append_field({"field_1", 8}), result_code::ok);
    EXPECT_EQ(ps.get_fields_list().size(), 1);
    EXPECT_EQ(ps.append_field({"field_2", 11}), result_code::ok);
    EXPECT_EQ(ps.get_fields_list().size(), 2);
    EXPECT_EQ(ps.append_field({"field_3", 15}), result_code::ok);
    EXPECT_EQ(ps.append_field({"field_4", 45}), result_code::ok);
    EXPECT_EQ(ps.append_field({"field_5", 12}), result_code::ok);
    EXPECT_EQ(ps.append_field({"field_6", 3}), result_code::ok);
    EXPECT_EQ(ps.get_fields_list().size(), 6);
    EXPECT_EQ(ps.get_internal_buffer_length(), 12);

    // Preserving and not preserving internal buffer values upon field appending
    ps.write("field_6", 1);
    EXPECT_EQ(ps.read<int>("field_6"), 1);
    // Appending should not affect value of field_6
    ps.append_field({"field_7", 5});
    EXPECT_EQ(ps.read<int>("field_6"), 1);
    // Appending should erase value of field_6
    ps.append_field({"field_8", 5}, false);
    EXPECT_NE(ps.read<int>("field_6"), 1);

    // Check that every field has different first bit index
    int lastFirstBitInd = -1;
    for (const std::string& name : ps.get_fields_list()) {
        EXPECT_NE(ps.get_field_metadata(name).first_bit_ind, lastFirstBitInd);
        lastFirstBitInd = ps.get_field_metadata(name).first_bit_ind;
    }

    // Appending protocol
    protocol_serializer psSecond;
    EXPECT_EQ(psSecond.append_protocol(ps), result_code::ok);
    EXPECT_EQ(psSecond.get_internal_buffer_length(), ps.get_internal_buffer_length());
    EXPECT_EQ(psSecond.get_fields_list(), ps.get_fields_list());
    EXPECT_EQ(psSecond.append_protocol(ps), result_code::bad_input);

    // Removing field
    EXPECT_EQ(psSecond.remove_field("non_existing_field"), result_code::field_not_found);
    EXPECT_EQ(psSecond.remove_field("field_4"), result_code::ok);
    EXPECT_EQ(psSecond.get_fields_list().size(), ps.get_fields_list().size() - 1);
    EXPECT_NE(psSecond.get_internal_buffer_length(), ps.get_internal_buffer_length());
    EXPECT_EQ(psSecond.get_field_metadata(psSecond.get_fields_list().back()).first_bit_ind,
              ps.get_field_metadata(ps.get_fields_list().back()).first_bit_ind - ps.get_field_metadata("field_4").bit_count);

    // Clearing protocol
    EXPECT_EQ(psSecond.clear_protocol(), result_code::ok);
    EXPECT_EQ(psSecond.get_fields_list().size(), 0);
    EXPECT_EQ(psSecond.get_internal_buffer_length(), 0);
    EXPECT_EQ(psSecond.clear_protocol(), result_code::not_applicable);
}

TEST(ReadWrite, NumericLimits)
{
    // Specify offset for min and max fields for extra checks
    // of whether weird alignment breaks it or not
    for (unsigned int offset = 1; offset < 64; ++offset) {
        checkNumericLimitsOf<float>(offset);
        checkNumericLimitsOf<double>(offset);
        checkNumericLimitsOf<int8_t>(offset);
        checkNumericLimitsOf<int16_t>(offset);
        checkNumericLimitsOf<int32_t>(offset);
        checkNumericLimitsOf<int64_t>(offset);
        checkNumericLimitsOf<uint8_t>(offset);
        checkNumericLimitsOf<uint16_t>(offset);
        checkNumericLimitsOf<uint32_t>(offset);
        checkNumericLimitsOf<uint64_t>(offset);
    }
}

TEST(ReadWrite, ValuesRange)
{
    // Specify offset for min and max fields for extra checks
    // of whether weird alignment breaks it or not
    for (unsigned int offset = 1; offset <= 64; ++offset) {
        for (unsigned int bitCount = 1; bitCount <= 64; ++bitCount) {
            checkValuesRange(offset, bitCount);
        }
    }
}
