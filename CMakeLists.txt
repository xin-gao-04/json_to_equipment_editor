cmake_minimum_required(VERSION 3.16)
project(EquipmentConfig)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set Qt path (Windows uses a default prefix; other platforms rely on system Qt)
if (WIN32)
    set(QT_WINDOWS_PREFIX "C:/Qt/Qt5.5.1/lib/cmake/Qt5")
    if (NOT DEFINED CMAKE_PREFIX_PATH)
        set(CMAKE_PREFIX_PATH "${QT_WINDOWS_PREFIX}")
    endif()
endif()

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# Enable Qt's automatic MOC, UIC and RCC processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Source files
set(SOURCES
    src/main.cpp
    src/EquipmentConfigWidget.cpp
    src/DeviceTabWidget.cpp
    src/WorkStateTabWidget.cpp
    src/ParameterItem.cpp
    src/EquipmentType.cpp
    src/DeviceInstance.cpp
    src/WorkStateTemplate.cpp
)

set(HEADERS
    src/EquipmentConfigWidget.h
    src/DeviceTabWidget.h
    src/WorkStateTabWidget.h
    src/ParameterItem.h
    src/EquipmentType.h
    src/DeviceInstance.h
    src/WorkStateTemplate.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Set target properties for Qt
set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Link Qt5 libraries
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Widgets)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy config files to build directory
configure_file(${CMAKE_SOURCE_DIR}/config/equipment_config.json ${CMAKE_BINARY_DIR}/bin/equipment_config.json COPYONLY) 
