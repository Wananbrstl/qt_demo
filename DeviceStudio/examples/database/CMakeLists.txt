add_library(database_learning_core STATIC
    common/DbTypes.cpp
    common/DbTypes.h
    common/DatabaseConfigLoader.cpp
    common/DatabaseConfigLoader.h
    infrastructure/DatabaseSession.cpp
    infrastructure/DatabaseSession.h
    infrastructure/MigrationRunner.cpp
    infrastructure/MigrationRunner.h
    infrastructure/TransactionGuard.cpp
    infrastructure/TransactionGuard.h
    repositories/HistorianRepository.cpp
    repositories/HistorianRepository.h
    schema/HistorianSchema.cpp
    schema/HistorianSchema.h
    workers/HistorianWorker.cpp
    workers/HistorianWorker.h
    workers/MysqlSyncWorker.cpp
    workers/MysqlSyncWorker.h
)

target_include_directories(database_learning_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(database_learning_core PUBLIC Qt5::Core Qt5::Sql)
target_compile_options(database_learning_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion -Wshadow>
)

add_executable(DatabaseDriverProbe tools/DatabaseDriverProbe.cpp)
target_link_libraries(DatabaseDriverProbe PRIVATE database_learning_core Qt5::Core Qt5::Sql)

add_executable(DatabaseWorkbench
    workstation/main.cpp
    workstation/DatabaseWorkbenchWindow.cpp
    workstation/DatabaseWorkbenchWindow.h
)
target_link_libraries(DatabaseWorkbench PRIVATE
    database_learning_core Qt5::Widgets)

if(TARGET tcp_learning_core)
    add_executable(HistorianCollector collector/HistorianCollector.cpp)
    target_link_libraries(HistorianCollector PRIVATE
        database_learning_core tcp_learning_core Qt5::Core)
endif()

if(BUILD_TESTING)
    add_executable(test_database_learning_core tests/test_database_core.cpp)
    target_link_libraries(test_database_learning_core PRIVATE
        database_learning_core Qt5::Test)
    add_test(NAME database_learning_core COMMAND test_database_learning_core)

    add_executable(test_database_sqlite_concurrency
        tests/test_sqlite_concurrency.cpp)
    target_link_libraries(test_database_sqlite_concurrency PRIVATE
        database_learning_core Qt5::Test)
    add_test(NAME database_sqlite_concurrency
             COMMAND test_database_sqlite_concurrency)

    add_executable(test_database_worker tests/test_historian_worker.cpp)
    target_link_libraries(test_database_worker PRIVATE
        database_learning_core Qt5::Test)
    add_test(NAME database_worker COMMAND test_database_worker)

    add_executable(test_database_mysql_contract tests/test_mysql_contract.cpp)
    target_link_libraries(test_database_mysql_contract PRIVATE
        database_learning_core Qt5::Test)
    add_test(NAME database_mysql_contract COMMAND test_database_mysql_contract)
endif()
