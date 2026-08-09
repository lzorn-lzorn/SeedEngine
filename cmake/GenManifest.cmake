# cmake/GenerateManifest.cmake
# 用途：扫描指定的库输出目录，生成 manifest.cmake
# 需要由 cmake -P 调用，并传入以下变量：
#   lib_dir   - 第三方库编译后的输出目录（绝对路径）
#   manifest  - 要生成的 manifest 文件完整路径
#   is_shared - TRUE/FALSE，本次构建是否为动态库

set(imported_loc "")
set(imported_implib "")

if(is_shared)
    set(lib_type SHARED)
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        file(GLOB all_files "${lib_dir}/*")
        foreach(f ${all_files})
            get_filename_component(ext "${f}" LAST_EXT)
            if(ext STREQUAL ".dll")
                set(imported_loc "${f}")
            elseif(ext STREQUAL ".lib")
                set(imported_implib "${f}")
            endif()
        endforeach()
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        file(GLOB all_files "${lib_dir}/*")
        foreach(f ${all_files})
            if(f MATCHES "\\.dylib$")
                set(imported_loc "${f}")
                break()
            endif()
        endforeach()
    else() # Linux 等
        file(GLOB all_files "${lib_dir}/*")
        foreach(f ${all_files})
            if(f MATCHES "\\.so(\\.[0-9]+)*$")
                set(imported_loc "${f}")
                break()
            endif()
        endforeach()
    endif()
else()
    set(lib_type STATIC)
    file(GLOB all_files "${lib_dir}/*")
    foreach(f ${all_files})
        get_filename_component(ext "${f}" LAST_EXT)
        if(ext STREQUAL ".lib" OR ext STREQUAL ".a")
            set(imported_loc "${f}")
            break()
        endif()
    endforeach()
endif()

if(NOT imported_loc)
    message(FATAL_ERROR "[GenerateManifest] Could not determine library output in ${lib_dir}")
endif()

# 写入 manifest.cmake
file(WRITE "${manifest}"
    "set(PREBUILT_TYPE ${lib_type})\n"
    "set(PREBUILT_IMPORTED_LOCATION \"${imported_loc}\")\n"
)
if(imported_implib)
    file(APPEND "${manifest}"
        "set(PREBUILT_IMPORTED_IMPLIB \"${imported_implib}\")\n"
    )
endif()
file(APPEND "${manifest}" "set(PREBUILT_VALID TRUE)\n")
message("[GenerateManifest] ${manifest} generated successfully.")