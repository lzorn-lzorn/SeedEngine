file(GLOB_RECURSE SOURCES_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/*.hpp
)

add_internal_module(Application
    TYPE                STATIC
    SOURCES             ${SOURCES_LIST}
    INCLUDES            "${CMAKE_CURRENT_SOURCE_DIR}/includes"
    PRIVATE_INCLUDES    "${CMAKE_CURRENT_SOURCE_DIR}/sources"
    PRIVATE_DEPENDENCY  SDL3::SDL3 Core::Core
    ALIAS               Module::Application
)