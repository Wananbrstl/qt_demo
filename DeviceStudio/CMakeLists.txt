cmake_minimum_required(VERSION 3.20)

project(DeviceStudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets Network Sql Test)

add_library(device_core STATIC
    src/domain/Types.cpp
    src/domain/Types.h
    src/protocol/FrameCodec.cpp
    src/protocol/FrameCodec.h
    src/network/DeviceSession.cpp
    src/network/DeviceSession.h
    src/persistence/DatabaseWorker.cpp
    src/persistence/DatabaseWorker.h
    src/application/DeviceApplicationService.cpp
    src/application/DeviceApplicationService.h
    resources.qrc
)

target_include_directories(device_core PUBLIC src)
target_link_libraries(device_core PUBLIC Qt5::Core Qt5::Network Qt5::Sql)
target_compile_options(device_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion -Wshadow>
)

add_executable(DeviceStudio
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/DocumentManager.cpp
    src/ui/DocumentManager.h
    src/ui/docks/DeviceExplorerDock.cpp
    src/ui/docks/DeviceExplorerDock.h
    src/ui/docks/DiagnosticsDock.cpp
    src/ui/docks/DiagnosticsDock.h
    src/ui/docks/PropertyInspectorDock.cpp
    src/ui/docks/PropertyInspectorDock.h
    src/ui/documents/DeviceMonitorDocument.cpp
    src/ui/documents/DeviceMonitorDocument.h
    src/ui/documents/HistoryDocument.cpp
    src/ui/documents/HistoryDocument.h
    src/ui/documents/ProtocolInspectorDocument.cpp
    src/ui/documents/ProtocolInspectorDocument.h
    src/ui/models/DeviceTreeModel.cpp
    src/ui/models/DeviceTreeModel.h
    src/ui/models/TelemetryTableModel.cpp
    src/ui/models/TelemetryTableModel.h
    src/ui/widgets/TelemetryChartWidget.cpp
    src/ui/widgets/TelemetryChartWidget.h
    src/ui/UiMetrics.h
)

target_link_libraries(DeviceStudio PRIVATE device_core Qt5::Widgets)

add_executable(DeviceSimulator
    simulator/main.cpp
    simulator/DeviceSimulator.cpp
    simulator/DeviceSimulator.h
)
target_link_libraries(DeviceSimulator PRIVATE device_core Qt5::Core Qt5::Network)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

option(DEVICESTUDIO_BUILD_TCP_LABS
       "Build the production-style TCP learning laboratory" ON)
if(DEVICESTUDIO_BUILD_TCP_LABS)
    add_subdirectory(examples/tcp)
endif()

option(DEVICESTUDIO_BUILD_DATABASE_LABS
       "Build the production-style database learning laboratory" ON)
if(DEVICESTUDIO_BUILD_DATABASE_LABS)
    add_subdirectory(examples/database)
endif()
