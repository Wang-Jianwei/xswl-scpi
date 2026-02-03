# tests/CMakeLists.txt
# CTest integration for project tests

# Testing is enabled from the top-level CMakeLists
enable_testing()

# Ensure tests are built with the same include paths
include_directories(${CMAKE_SOURCE_DIR}/include)

# Phase 2 测试
add_executable(easy_test_phase2 test_phase2.cpp)
target_link_libraries(easy_test_phase2 scpi_parser)
add_test(NAME TestPhase2 COMMAND $<TARGET_FILE:easy_test_phase2>)
set_tests_properties(TestPhase2 PROPERTIES LABELS "phase2")

# Phase 3 测试
add_executable(easy_test_phase3 test_phase3.cpp)
target_link_libraries(easy_test_phase3 scpi_parser)
add_test(NAME TestPhase3 COMMAND $<TARGET_FILE:easy_test_phase3>)
set_tests_properties(TestPhase3 PROPERTIES LABELS "phase3")

# Phase 4 测试
add_executable(easy_test_phase4 test_phase4.cpp)
target_link_libraries(easy_test_phase4 scpi_parser)
add_test(NAME TestPhase4 COMMAND $<TARGET_FILE:easy_test_phase4>)
set_tests_properties(TestPhase4 PROPERTIES LABELS "phase4")

# Phase 5 测试
add_executable(easy_test_phase5 test_phase5.cpp)
target_link_libraries(easy_test_phase5 scpi_parser)
add_test(NAME TestPhase5 COMMAND $<TARGET_FILE:easy_test_phase5>)
set_tests_properties(TestPhase5 PROPERTIES LABELS "phase5")

# Phase 6 测试
add_executable(easy_test_phase6 test_phase6.cpp)
target_link_libraries(easy_test_phase6 scpi_parser)
add_test(NAME TestPhase6 COMMAND $<TARGET_FILE:easy_test_phase6>)
set_tests_properties(TestPhase6 PROPERTIES LABELS "phase6")

# Demo (not part of tests by default, but convenient to build)
add_executable(easy_demo_phase5 demo_phase5.cpp)
target_link_libraries(easy_demo_phase5 scpi_parser)

# Convenience target to run all tests from build system
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS easy_test_phase2 easy_test_phase3 easy_test_phase4 easy_test_phase5 easy_test_phase6
)
