cmake_minimum_required(VERSION 3.8)
project(MyThon)

set(CMAKE_CXX_STANDARD 17)
option(BUILD_TESTS "Set ON to build tests" OFF)

set(parser_files parse.h parse.cpp)
set(runtime_files runtime.cpp runtime.h)
set(statement_files statement.cpp statement.h)
set(lexer_files lexer.h lexer.cpp)

set(main_files ${parser_files} ${runtime_files} ${statement_files} ${lexer_files})
set(tests_files tests/lexer_test.cpp  tests/main_test.cpp tests/parse_test.cpp tests/runtime_test.cpp tests/statement_test.cpp tests/test_runner.h)


if(BUILD_TESTS)
    message(STATUS "Building tests!")
    set(USING_FILES ${main_files} ${tests_files} )
else()
    set(USING_FILES ${main_files} main.cpp)
endif()

add_executable(mython ${USING_FILES})

