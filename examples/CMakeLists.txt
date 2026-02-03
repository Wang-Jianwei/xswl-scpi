# examples/CMakeLists.txt

# 示例程序会链接到 scpi_parser 库
set(EXAMPLES
    basic_usage
    signal_generator
    oscilloscope
    multichannel_dmm
    custom_instrument
)

foreach(ex ${EXAMPLES})
    add_executable(easy_${ex} ${ex}.cpp)
    target_link_libraries(easy_${ex} PRIVATE scpi_parser)
endforeach()

# Convenience target to build all examples at once
add_custom_target(examples_all DEPENDS ${EXAMPLES})
foreach(ex ${EXAMPLES})
    add_dependencies(examples_all easy_${ex})
endforeach()
