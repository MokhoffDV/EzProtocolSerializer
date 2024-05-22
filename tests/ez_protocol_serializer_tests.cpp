#include <gtest/gtest.h>
#include <ez_protocol_serializer.h>

using ez::protocol_serializer;
using buffer_source = ez::protocol_serializer::buffer_source;
using result_code = ez::protocol_serializer::result_code;
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
    protocol_serializer psMoveAssignment(std::move(psMoveConstructor));
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
