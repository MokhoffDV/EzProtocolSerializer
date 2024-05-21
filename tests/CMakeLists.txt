cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(EzProtocolSerializerTests)

# Set up google test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()

# Set up executable
set(EZ_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EZ_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include")
set(EZ_SOURCES 	"${EZ_SOURCES_DIR}/ez_protocol_serializer_tests.cpp"
				"${EZ_INCLUDE_DIR}/ez_protocol_serializer.cpp")
set(EZ_HEADERS 	"${EZ_INCLUDE_DIR}/ez_protocol_serializer.h")
set(EXECUTABLE_NAME ${PROJECT_NAME})
add_executable(${EXECUTABLE_NAME} ${EZ_SOURCES} ${EZ_HEADERS})
target_link_libraries(${EXECUTABLE_NAME} GTest::gtest_main)
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${EZ_INCLUDE_DIR})

# Discover tests
include(GoogleTest)
gtest_discover_tests(${EXECUTABLE_NAME})

# Set up startup project for Visual Studio
if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${EXECUTABLE_NAME})
endif()