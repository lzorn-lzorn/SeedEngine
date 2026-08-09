
# 实际上很多库是无法使用和项目一样的编译选项的, 尤其是一些比较老的库, 使用最严格的编译检
# 查选项会导致编译失败, 所以这里提供了一个宏来移除一些编译选项, 以便在编译外部库时使用.
# 见 BuildTools.cmake

# TEST add_external_module
# add_external_module(
# 	NAME SDL3::SDL3
# 	SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/SDL"
# 	TARGET_IF_BUILT SDL3::SDL3
# 	INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/SDL/include"
# 	BUILD_SHARED_LIBS ON
# )

add_subdirectory(SDL)
add_subdirectory(json) # 对于 nlohmann json 库, 实际上有 head-only 版本
add_subdirectory(SPIRV-Reflect)

if (TARGET DXC::Compiler)
    target_link_libraries(${LibarayName}
        PRIVATE
            DXC::Compiler
    )
else()
    add_subdirectory(DirectXShaderCompiler)
endif()

remove_strict_flags()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  remove_c_and_cxx_flag(
    "-Wextra"
    "-Wmisleading-indentation"
  )
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
  remove_c_and_cxx_flag(
    "-Weverything"
    "-Wextra"
    "-Wnull-pointer-subtraction"
    "-Wcast-function-type-mismatch"
    "-Wignored-qualifiers"
  )
endif()