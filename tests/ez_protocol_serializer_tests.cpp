#include <gtest/gtest.h>
#include "ez_protocol_serializer.h"

using ez::protocol_serializer;
using buffer_source = ez::protocol_serializer::buffer_source;
TEST(Constructing, Constructors) {
    // Default
    protocol_serializer psDefault;
    EXPECT_EQ(psDefault.get_is_little_endian(), false);
    EXPECT_EQ(psDefault.get_buffer_source(), protocol_serializer::buffer_source::internal);
    EXPECT_EQ(psDefault.get_internal_buffer().get(), psDefault.get_working_buffer());
    EXPECT_EQ(psDefault.get_internal_buffer_length(), 0);
    EXPECT_EQ(psDefault.get_fields_list().size(), 0);

    // Constructor 1
    unsigned char externalBuffer[32];
    protocol_serializer psConstructor1(true, buffer_source::external, externalBuffer);
    EXPECT_EQ(psConstructor1.get_is_little_endian(), true);
    EXPECT_EQ(psConstructor1.get_buffer_source(), protocol_serializer::buffer_source::external);
    EXPECT_NE(psConstructor1.get_internal_buffer().get(), psConstructor1.get_working_buffer());
    EXPECT_EQ(psConstructor1.get_external_buffer(), psConstructor1.get_working_buffer());
    EXPECT_EQ(psConstructor1.get_internal_buffer_length(), 0);
    EXPECT_EQ(psConstructor1.get_fields_list().size(), 0);

    // Constructor 2
    protocol_serializer psConstructor2({
                                {"field_1", 8},
                                {"field_2", 8},
                                {"field_3", 4},
                            }, true, buffer_source::internal, externalBuffer);
    EXPECT_EQ(psConstructor2.get_is_little_endian(), true);
    EXPECT_EQ(psConstructor2.get_buffer_source(), protocol_serializer::buffer_source::internal);
    EXPECT_NE(psConstructor2.get_internal_buffer().get(), nullptr);
    EXPECT_EQ(psConstructor2.get_external_buffer(), nullptr);
    EXPECT_EQ(psConstructor2.get_internal_buffer_length(), 3);
    EXPECT_EQ(psConstructor2.get_fields_list().size(), 3);

    // Copy constructor
    protocol_serializer psCopyConstructor(psConstructor2);
    EXPECT_EQ(psCopyConstructor.get_is_little_endian(), psConstructor2.get_is_little_endian());
    EXPECT_EQ(psCopyConstructor.get_buffer_source(), psConstructor2.get_buffer_source());
    EXPECT_NE(psCopyConstructor.get_internal_buffer().get(), psConstructor2.get_internal_buffer().get());
    EXPECT_EQ(psCopyConstructor.get_external_buffer(), psConstructor2.get_external_buffer());
    EXPECT_EQ(psCopyConstructor.get_internal_buffer_length(), psConstructor2.get_internal_buffer_length());
    EXPECT_EQ(psCopyConstructor.get_fields_list().size(), psConstructor2.get_fields_list().size());

    // Copy assignment operator
    protocol_serializer psCopyAssignment(psCopyConstructor);
    EXPECT_EQ(psCopyAssignment.get_is_little_endian(), psCopyConstructor.get_is_little_endian());
    EXPECT_EQ(psCopyAssignment.get_buffer_source(), psCopyConstructor.get_buffer_source());
    EXPECT_NE(psCopyAssignment.get_internal_buffer().get(), psCopyConstructor.get_internal_buffer().get());
    EXPECT_EQ(psCopyAssignment.get_external_buffer(), psCopyConstructor.get_external_buffer());
    EXPECT_EQ(psCopyAssignment.get_internal_buffer_length(), psCopyConstructor.get_internal_buffer_length());
    EXPECT_EQ(psCopyAssignment.get_fields_list().size(), psCopyConstructor.get_fields_list().size());

    // Move constructor
    protocol_serializer psMoveConstructor(std::move(psConstructor2));
    EXPECT_EQ(psMoveConstructor.get_is_little_endian(), true);
    EXPECT_EQ(psMoveConstructor.get_buffer_source(), protocol_serializer::buffer_source::internal);
    EXPECT_NE(psMoveConstructor.get_internal_buffer().get(), nullptr);
    EXPECT_EQ(psMoveConstructor.get_external_buffer(), nullptr);
    EXPECT_EQ(psMoveConstructor.get_internal_buffer_length(), 3);
    EXPECT_EQ(psMoveConstructor.get_fields_list().size(), 3);
    // Check if it is now empty
    EXPECT_EQ(psConstructor2.get_is_little_endian(), true);
    EXPECT_EQ(psConstructor2.get_buffer_source(), protocol_serializer::buffer_source::internal);
    EXPECT_EQ(psConstructor2.get_internal_buffer().get(), nullptr);
    EXPECT_EQ(psConstructor2.get_external_buffer(), nullptr);
    EXPECT_EQ(psConstructor2.get_internal_buffer_length(), 0);
    EXPECT_EQ(psConstructor2.get_fields_list().size(), 0);

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