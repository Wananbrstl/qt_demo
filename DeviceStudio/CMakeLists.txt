cmake_minimum_required(VERSION 3.20)

project(DeviceStudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Widgets Network Sql Test)
qt_standard_project_setup()

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
)

target_include_directories(device_core PUBLIC src)
target_link_libraries(device_core PUBLIC Qt6::Core Qt6::Network Qt6::Sql)
target_compile_options(device_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion -Wshadow>
)

qt_add_resources(device_core device_resources
    PREFIX "/"
    FILES
        config/application.json
        migrations/001_initial.sql
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

target_link_libraries(DeviceStudio PRIVATE device_core Qt6::Widgets)

add_executable(DeviceSimulator
    simulator/main.cpp
    simulator/DeviceSimulator.cpp
    simulator/DeviceSimulator.h
)
target_link_libraries(DeviceSimulator PRIVATE device_core Qt6::Core Qt6::Network)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

