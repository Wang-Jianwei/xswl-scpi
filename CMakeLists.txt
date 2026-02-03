# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(scpi-parser VERSION 1.0.0 LANGUAGES CXX)

# C++11 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 库源文件
set(SCPI_SOURCES
    # Phase 1
    src/error_codes.cpp
    src/error_queue.cpp
    src/lexer.cpp

    # Phase 2
    src/command_node.cpp
    src/command_tree.cpp
    src/pattern_parser.cpp

    # Phase 3
    src/keywords.cpp
    src/units.cpp
    src/parameter.cpp
    
    # Phase 4
    src/command_splitter.cpp
    src/path_context.cpp
    src/path_resolver.cpp

    # Phase 5
    src/context.cpp
    src/parser.cpp
    src/ieee488_commands.cpp

    # Phase 6
    src/status_register.cpp
)

add_custom_target(1-docs SOURCES help.md scpi-parser.md)

# 创建静态库
add_library(scpi_parser
    STATIC
    ${SCPI_SOURCES}
    include/scpi/command.h
    include/scpi/command_node.h
    include/scpi/command_splitter.h
    include/scpi/command_tree.h
    include/scpi/context.h
    include/scpi/error_codes.h
    include/scpi/error_queue.h
    include/scpi/keywords.h
    include/scpi/lexer.h
    include/scpi/node_param.h
    include/scpi/parameter.h
    include/scpi/parser.h
    include/scpi/path_context.h
    include/scpi/path_resolver.h
    include/scpi/pattern_parser.h
    include/scpi/scpi.h
    include/scpi/status_register.h
    include/scpi/token.h
    include/scpi/types.h
    include/scpi/units.h
)

# 安装头文件
install(DIRECTORY include/scpi DESTINATION include)

# 安装库
install(TARGETS scpi_parser DESTINATION lib)

# 可选: 构建测试
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    
    # Phase 2 测试
    add_executable(test_phase2 tests/test_phase2.cpp)
    target_link_libraries(test_phase2 scpi_parser)
    add_test(NAME TestPhase2 COMMAND test_phase2)
    
    # Phase 3 测试
    add_executable(test_phase3 tests/test_phase3.cpp)
    target_link_libraries(test_phase3 scpi_parser)
    add_test(NAME TestPhase3 COMMAND test_phase3)

    # Phase 4 测试
    add_executable(test_phase4 tests/test_phase4.cpp)
    target_link_libraries(test_phase4 scpi_parser)
    add_test(NAME TestPhase4 COMMAND test_phase4)

    # Phase 5 测试
    add_executable(test_phase5 tests/test_phase5.cpp)
    target_link_libraries(test_phase5 scpi_parser)
    add_test(NAME TestPhase5 COMMAND test_phase5)
    
    # Phase 6 测试
    add_executable(test_phase6 tests/test_phase6.cpp)
    target_link_libraries(test_phase6 scpi_parser)
    add_test(NAME TestPhase6 COMMAND test_phase6)

    # Phase 5 demo
    add_executable(demo_phase5 tests/demo_phase5.cpp)
    target_link_libraries(demo_phase5 scpi_parser)
endif()
