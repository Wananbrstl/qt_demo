add_executable(test_protocol test_protocol.cpp)
target_link_libraries(test_protocol PRIVATE device_core Qt5::Test)
add_test(NAME protocol COMMAND test_protocol)

add_executable(test_database test_database.cpp)
target_link_libraries(test_database PRIVATE device_core Qt5::Test)
add_test(NAME database COMMAND test_database)
