add_library(tcp_learning_core STATIC
    common/BackoffPolicy.cpp
    common/BackoffPolicy.h
    common/SessionConfigLoader.cpp
    common/SessionConfigLoader.h
    common/TcpTypes.cpp
    common/TcpTypes.h
    protocol/FrameCodec.cpp
    protocol/FrameCodec.h
    transport/BoundedWriteQueue.cpp
    transport/BoundedWriteQueue.h
    session/DeviceSession.cpp
    session/DeviceSession.h
)

target_include_directories(tcp_learning_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tcp_learning_core PUBLIC Qt5::Core Qt5::Network)
target_compile_options(tcp_learning_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion -Wshadow>
)

add_library(tcp_learning_simulator STATIC
    simulator/IndustrialDeviceSimulator.cpp
    simulator/IndustrialDeviceSimulator.h
)
target_include_directories(tcp_learning_simulator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tcp_learning_simulator PUBLIC tcp_learning_core Qt5::Core Qt5::Network)

add_executable(TcpDeviceSimulator simulator/main.cpp)
target_link_libraries(TcpDeviceSimulator PRIVATE tcp_learning_simulator)

add_executable(TcpConnectionProbe tools/TcpConnectionProbe.cpp)
target_link_libraries(TcpConnectionProbe PRIVATE tcp_learning_core Qt5::Core Qt5::Network)

add_executable(TcpWorkstation
    workstation/main.cpp
    workstation/TcpWorkbenchWindow.cpp
    workstation/TcpWorkbenchWindow.h
)
target_link_libraries(TcpWorkstation PRIVATE tcp_learning_core Qt5::Widgets)

if(BUILD_TESTING)
    add_executable(test_tcp_frame_codec tests/test_frame_codec.cpp)
    target_link_libraries(test_tcp_frame_codec PRIVATE tcp_learning_core Qt5::Test)
    add_test(NAME tcp_frame_codec COMMAND test_tcp_frame_codec)

    add_executable(test_tcp_policies tests/test_policies.cpp)
    target_link_libraries(test_tcp_policies PRIVATE tcp_learning_core Qt5::Test)
    add_test(NAME tcp_policies COMMAND test_tcp_policies)

    add_executable(test_tcp_integration tests/test_integration.cpp)
    target_link_libraries(test_tcp_integration PRIVATE
        tcp_learning_core tcp_learning_simulator Qt5::Test)
    add_test(NAME tcp_integration COMMAND test_tcp_integration)
endif()
