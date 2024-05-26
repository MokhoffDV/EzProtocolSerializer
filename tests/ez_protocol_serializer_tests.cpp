#include <cmath>
#include <type_traits>
#include <gtest/gtest.h>
#include <ez_protocol_serializer.h>

using ez::protocol_serializer;
using buffer_source = ez::protocol_serializer::buffer_source;
using result_code = ez::protocol_serializer::result_code;

template<class T>
std::vector<T> generateEquallySpreadValues(T min, T max)
{
    // Generates vector which consists of: {min, max, <=20 values between min and max>...}
    std::vector<T> result{min, max};

    // No values in between min and max
    if (min == max || (min + 1 == max))
        return result;

    typename std::make_unsigned<T>::type count = 0;
    // Avoid signed type overflow in case of T::min() and T::max()
    if (std::is_signed<T>::value) {
        count = -min + max;
    } else {
        count = max - min - 1;
    }
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
        const T value = static_cast<T>(std::round(min + i * step));
        result.push_back(value);
    }

    return result;
}

template<class T>
void checkNumericLimitsOf(unsigned int offset)
{
    assert(offset != 0);

    constexpr T numericLimitMin = std::numeric_limits<T>::min();
    constexpr T numericLimitMax = std::numeric_limits<T>::max();

    ez::protocol_serializer ps({{"offset", offset}, {"min", sizeof(T) * 8}, {"max", sizeof(T) * 8}});
    ps.write("min", numericLimitMin);
    ps.write("max", numericLimitMax);
    EXPECT_EQ(ps.read<T>("min"), numericLimitMin);
    EXPECT_EQ(ps.read<T>("max"), numericLimitMax);
}

template<class T>
void checkMultibyteMirroring(unsigned int offset)
{
    for (const T hostValue : generateEquallySpreadValues(std::numeric_limits<T>::min(), std::numeric_limits<T>::max())) {
        protocol_serializer ps({{"offset", offset}, {"value", sizeof(T) * 8}});

        // Match protocol endiannes with host endiannes and write host value
        ps.set_is_little_endian(protocol_serializer::get_is_host_little_endian());
        ps.write("value", hostValue);

        // Make interpreting endiannes to differ from host
        ps.set_is_little_endian(!protocol_serializer::get_is_host_little_endian());
        const T reversedValue = ps.read<T>("value");

        // Make sure those values are a mirror version of each other
        unsigned char hostValueBytes[sizeof(T)];
        memcpy(hostValueBytes, &hostValue, sizeof(T));
        unsigned char reversedValueBytes[sizeof(T)];
        memcpy(reversedValueBytes, &reversedValue, sizeof(T));
        for (size_t i = 0; i < sizeof(T); ++i)
            EXPECT_EQ(hostValueBytes[i], reversedValueBytes[sizeof(T) - i - 1]);
    }
}

template<class SmallType, class BigType>
void checkTypeOverflowOf(unsigned int offset)
{
    assert(offset != 0);
    assert(!std::is_signed<SmallType>::value);
    assert(!std::is_signed<BigType>::value);

    const std::vector<BigType> bigValues = generateEquallySpreadValues(std::numeric_limits<BigType>::min(), std::numeric_limits<BigType>::max());
    for (const uint64_t bigValue : bigValues) {
        ez::protocol_serializer ps({{"offset", offset}, {"big_value", sizeof(uint64_t) * 8}});
        ps.write("big_value", bigValue);
        EXPECT_EQ(ps.read<SmallType>("big_value"), static_cast<SmallType>(bigValue));
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
    // Check if host endiannes is determined correctly
    unsigned char shortBytes[2];
    const uint16_t shortValue = 1;
    memcpy(shortBytes, &shortValue, 2);
    EXPECT_EQ(static_cast<bool>(shortBytes[0]), protocol_serializer::get_is_host_little_endian());

    // Check if protocol endiannes is being correctly changed
    protocol_serializer ps;
    ps.set_is_little_endian(true);
    EXPECT_EQ(ps.get_is_little_endian(), true);
    ps.set_is_little_endian(false);
    EXPECT_EQ(ps.get_is_little_endian(), false);
    ps.set_is_little_endian(true);
    EXPECT_EQ(ps.get_is_little_endian(), true);
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

TEST(ReadWrite, Endiannes)
{
    // Check if multi-byte values are interpreted correctly depending on endiannes
    for (unsigned int offset = 1; offset <= 64; ++offset) {
        checkMultibyteMirroring<int16_t>(offset);
        checkMultibyteMirroring<int32_t>(offset);
        checkMultibyteMirroring<int64_t>(offset);
        checkMultibyteMirroring<uint16_t>(offset);
        checkMultibyteMirroring<uint32_t>(offset);
        checkMultibyteMirroring<uint64_t>(offset);
    }
}

TEST(ReadWrite, NumericLimitsInRespectiveFieldLength)
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

TEST(ReadWrite, ValuesRangeInVariableFieldLength)
{
    // Specify offset for min and max fields for extra checks
    // of whether weird alignment breaks it or not
    for (unsigned int offset = 1; offset <= 64; ++offset) {
        for (unsigned int bitCount = 1; bitCount <= 64; ++bitCount) {
            assert(offset != 0);
            assert(bitCount != 0);

            ez::protocol_serializer ps({{"offset", offset}, {"value", bitCount}});

            // Check unsigned limits for this bit count
            {
                uint64_t max = 0;
                for (unsigned int i = 0; i < bitCount - 1; ++i)
                    max = (max << 1) + 1;

                for (const uint64_t value : generateEquallySpreadValues<uint64_t>(0, max)) {
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
    }
}

TEST(ReadWrite, Arrays)
{
    // Specify offset for min and max fields for extra checks
    // of whether weird alignment breaks it or not
    for (unsigned int offset = 1; offset <= 64; ++offset) {
        for (unsigned int bitCount = 1; bitCount <= 64; ++bitCount) {
            assert(offset != 0);
            assert(bitCount != 0);

            // Check unsigned limits for this bit count
            {
                uint64_t max = 0;
                for (unsigned int i = 0; i < bitCount - 1; ++i)
                    max = (max << 1) + 1;

                const std::vector<uint64_t> values = generateEquallySpreadValues<uint64_t>(0, max);
                // Create a protocol with array which gives bitCount for each value
                ez::protocol_serializer ps({{"offset", offset}, {"array", bitCount * static_cast<unsigned int>(values.size())}});

                // Write array
                std::unique_ptr<uint64_t[]> writtenArray = std::make_unique<uint64_t[]>(values.size());
                for (int i = 0; i < values.size(); ++i)
                    writtenArray[i] = values.at(i);
                ps.write_array<uint64_t>("array", writtenArray.get(), values.size());

                // Read array
                std::unique_ptr<uint64_t[]> readArray = std::make_unique<uint64_t[]>(values.size());
                ps.read_array<uint64_t>("array", readArray.get(), values.size());
                for (int i = 0; i < values.size(); ++i) {
                    EXPECT_EQ(values[i], writtenArray[i]);
                    EXPECT_EQ(writtenArray[i], readArray[i]);
                }
            }
            // Check signed limits for this bit count
            {
                int64_t min = 0;
                int64_t max = 0;
                for (unsigned int i = 0; i < bitCount - 1; ++i)
                    max = (max << 1) + 1;
                min = -max - 1;

                const std::vector<int64_t> values = generateEquallySpreadValues<int64_t>(min, max);
                // Create a protocol with array which gives bitCount for each value
                ez::protocol_serializer ps({{"offset", offset}, {"array", bitCount * static_cast<unsigned int>(values.size())}});

                // Write array
                std::unique_ptr<int64_t[]> writtenArray = std::make_unique<int64_t[]>(values.size());
                for (int i = 0; i < values.size(); ++i)
                    writtenArray[i] = values.at(i);
                ps.write_array<int64_t>("array", writtenArray.get(), values.size());

                // Read array
                std::unique_ptr<int64_t[]> readArray = std::make_unique<int64_t[]>(values.size());
                ps.read_array<int64_t>("array", readArray.get(), values.size());
                for (int i = 0; i < values.size(); ++i) {
                    EXPECT_EQ(values[i], writtenArray[i]);
                    EXPECT_EQ(writtenArray[i], readArray[i]);
                }
            }
        }
    }
}


TEST(ReadWrite, TypeOverflow)
{
    // Specify offset for min and max fields for extra checks
    // of whether weird alignment breaks it or not
    for (unsigned int offset = 1; offset < 64; ++offset) {
        checkTypeOverflowOf<uint8_t, uint16_t>(offset);
        checkTypeOverflowOf<uint8_t, uint32_t>(offset);
        checkTypeOverflowOf<uint8_t, uint64_t>(offset);
        checkTypeOverflowOf<uint16_t, uint32_t>(offset);
        checkTypeOverflowOf<uint16_t, uint64_t>(offset);
        checkTypeOverflowOf<uint32_t, uint64_t>(offset);
    }
}