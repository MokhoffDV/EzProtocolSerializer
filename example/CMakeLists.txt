cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_PREFIX_PATH <path_to_qt>/Qt/<qt_version>/<compiler>/lib/cmake)
set(CMAKE_PREFIX_PATH "D:/Programms/Qt/6.5.2/msvc2019_64/lib/cmake")
project(EzProtocolSerializerExample)

# Set up Qt
find_package(Qt5 COMPONENTS Core Gui Widgets QUIET)
find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
if(Qt5_FOUND)
	message(STATUS "Qt5 found")
	set(EZ_FOUND_QT_VERSION Qt5)
elseif(Qt6_FOUND)
	message(STATUS "Qt6 found")
	set(EZ_FOUND_QT_VERSION Qt6)
else()
	message(FATAL_ERROR "Qt5 or Qt6 not found! If you have Qt5 or Qt6 installed, please set CMAKE_PREFIX_PATH to hold Qt cmake directory. Example:\n'set(CMAKE_PREFIX_PATH <path_to_qt>/Qt/<qt_version>/<compiler>/lib/cmake)'")
endif()

# Set up build type
set(EZ_BUILD_TYPE "Release" CACHE STRING "Sets build type: 'Debug' or 'Release'")
if(${EZ_BUILD_TYPE} STREQUAL "Debug")
    set(CMAKE_BUILD_TYPE Debug)
elseif(${EZ_BUILD_TYPE} STREQUAL "Release")
    set(CMAKE_BUILD_TYPE Release)
else()
    message(FATAL_ERROR "Invalid build type '${EZ_BUILD_TYPE}' specified. Use 'Debug' or 'Release'")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}. You can change this by specifying -DEZ_BUILD_TYPE='Debug' or 'Release'")

# Set up sources
set(EZ_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(EZ_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(EZ_SOURCES "${EZ_SOURCES_DIR}/main.cpp"
			   "${EZ_SOURCES_DIR}/main_window.cpp"
			   "${EZ_INCLUDE_DIR}/ez_protocol_serializer.cpp")
set(EZ_HEADERS "${EZ_SOURCES_DIR}/main_window.h"
			   "${EZ_INCLUDE_DIR}/ez_protocol_serializer.h")
set(EZ_MOC_SOURCES "${EZ_SOURCES_DIR}/main_window.h")
set(EZ_UIC_SOURCES "${EZ_SOURCES_DIR}/main_window.ui")
source_group("Source Files" FILES ${EZ_SOURCES})
source_group("Header Files" FILES ${EZ_HEADERS})

# Set up executable
set(EZ_EXECUTABLE_NAME ${PROJECT_NAME})
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_CURRENT_SOURCE_DIR}/bin/${OUTPUTCONFIG}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_CURRENT_SOURCE_DIR}/bin/${OUTPUTCONFIG}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_CURRENT_SOURCE_DIR}/bin/${OUTPUTCONFIG}")
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
add_executable(${EZ_EXECUTABLE_NAME} ${EZ_SOURCES} ${EZ_HEADERS} ${EZ_MOC_SOURCES} ${EZ_UIC_SOURCES})
set_target_properties(${EZ_EXECUTABLE_NAME} PROPERTIES AUTOGEN_BUILD_DIR "${EZ_SOURCES_DIR}/autogen")
target_link_libraries(${EZ_EXECUTABLE_NAME} ${EZ_FOUND_QT_VERSION}::Core ${EZ_FOUND_QT_VERSION}::Gui ${EZ_FOUND_QT_VERSION}::Widgets)

# Set up compiler options
if(MSVC)
    target_compile_options(${EZ_EXECUTABLE_NAME} PRIVATE
		/W4
		/WX
		/permissive
	)
else()
    target_compile_options(${EZ_EXECUTABLE_NAME} PRIVATE
        -Wall
        -Wextra
        -Werror
        -pedantic
        -Wno-unused-variable
        -Wno-unused-parameter
        -Wshadow
        -Wformat=2
    )
endif()

# Set up startup project for Visual Studio
if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${EZ_EXECUTABLE_NAME})
endif()
