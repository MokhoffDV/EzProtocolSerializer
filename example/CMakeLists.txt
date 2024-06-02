cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_PREFIX_PATH <path_to_qt>/Qt/<qt_version>/<compiler>/lib/cmake)
project(EzProtocolSerializerExample)

# Set up Qt
find_package(Qt5 COMPONENTS Core Gui Widgets QUIET)
find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
if(Qt5_FOUND)
	message(STATUS "Qt5 found")
	set(EXAMPLE_QT_VERSION Qt5)
elseif(Qt6_FOUND)
	message(STATUS "Qt6 found")
	set(EXAMPLE_QT_VERSION Qt6)
else()
	message(FATAL_ERROR "Qt5 or Qt6 not found! If you have Qt5 or Qt6 installed, please set CMAKE_PREFIX_PATH to hold Qt cmake directory. Example:\n'set(CMAKE_PREFIX_PATH <path_to_qt>/Qt/<qt_version>/<compiler>/lib/cmake)'")
endif()

# Set up sources
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(EXAMPLE_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(CLASS_SOURCES_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/../src")
set(EXAMPLE_SOURCES	"${EXAMPLE_SOURCES_DIR}/main.cpp"
					"${EXAMPLE_SOURCES_DIR}/gui_elements/main_window.cpp"
					"${EXAMPLE_SOURCES_DIR}/gui_elements/creator_widget.cpp"
					"${EXAMPLE_SOURCES_DIR}/gui_elements/editor_widget.cpp"
					"${EXAMPLE_SOURCES_DIR}/gui_elements/visualizer_widget.cpp"
					"${EXAMPLE_SOURCES_DIR}/utils/validators.cpp"
					"${CLASS_SOURCES_DIR}/ez_protocol_serializer.cpp")
set(EXAMPLE_HEADERS	"${EXAMPLE_SOURCES_DIR}/gui_elements/main_window.h"
					"${EXAMPLE_SOURCES_DIR}/gui_elements/creator_widget.h"
					"${EXAMPLE_SOURCES_DIR}/gui_elements/editor_widget.h"
					"${EXAMPLE_SOURCES_DIR}/gui_elements/visualizer_widget.h"
					"${CLASS_SOURCES_DIR}/ez_protocol_serializer.h")
set(EXAMPLE_MOC_SOURCES	"${EXAMPLE_SOURCES_DIR}/gui_elements/main_window.h"
						"${EXAMPLE_SOURCES_DIR}/gui_elements/creator_widget.h"
						"${EXAMPLE_SOURCES_DIR}/gui_elements/editor_widget.h"
						"${EXAMPLE_SOURCES_DIR}/gui_elements/visualizer_widget.h"
						"${EXAMPLE_SOURCES_DIR}/utils/validators.h")
set(EXAMPLE_UIC_SOURCES	"${EXAMPLE_SOURCES_DIR}/gui_elements/main_window.ui"
						"${EXAMPLE_SOURCES_DIR}/gui_elements/visualizer_widget.ui")
source_group("Source Files" FILES ${EXAMPLE_SOURCES})
source_group("Header Files" FILES ${EXAMPLE_HEADERS})

# Set up executable
set(EXAMPLE_EXECUTABLE_NAME ${PROJECT_NAME})
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_CURRENT_SOURCE_DIR}/bin/${OUTPUTCONFIG}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_CURRENT_SOURCE_DIR}/bin/${OUTPUTCONFIG}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_CURRENT_SOURCE_DIR}/bin/${OUTPUTCONFIG}")
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
add_executable(${EXAMPLE_EXECUTABLE_NAME} ${EXAMPLE_SOURCES} ${EXAMPLE_HEADERS} ${EXAMPLE_MOC_SOURCES} ${EXAMPLE_UIC_SOURCES})
set_target_properties(${EXAMPLE_EXECUTABLE_NAME} PROPERTIES AUTOGEN_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/autogen")
target_link_libraries(${EXAMPLE_EXECUTABLE_NAME} ${EXAMPLE_QT_VERSION}::Core ${EXAMPLE_QT_VERSION}::Gui ${EXAMPLE_QT_VERSION}::Widgets)
target_include_directories(${EXAMPLE_EXECUTABLE_NAME} PRIVATE ${CLASS_SOURCES_DIR})

# Set up compiler options
if(MSVC)
    target_compile_options(${EXAMPLE_EXECUTABLE_NAME} PRIVATE
		/W4
		/WX
		/permissive
	)
else()
    target_compile_options(${EXAMPLE_EXECUTABLE_NAME} PRIVATE
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
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${EXAMPLE_EXECUTABLE_NAME})
endif()
