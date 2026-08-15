qt_add_executable(test_protocol test_protocol.cpp)
target_link_libraries(test_protocol PRIVATE device_core Qt6::Test)
add_test(NAME protocol COMMAND test_protocol)

qt_add_executable(test_database test_database.cpp)
target_link_libraries(test_database PRIVATE device_core Qt6::Test)
add_test(NAME database COMMAND test_database)

