set(SOURCES_LIST
    sources/Application.cpp
    sources/sdl/SDLApplication.cpp
    sources/sdl/SDLApplication.hpp
    includes/Application.hpp
)

add_internal_module(Application
    TYPE                STATIC
    SOURCES             ${SOURCES_LIST}
    INCLUDES            "${CMAKE_CURRENT_SOURCE_DIR}/includes"
    PRIVATE_INCLUDES    "${CMAKE_CURRENT_SOURCE_DIR}/sources"
    PRIVATE_DEPENDENCY  SDL3::SDL3
)