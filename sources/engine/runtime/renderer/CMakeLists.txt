file(GLOB_RECURSE SOURCES_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/includes/*.hpp
)

add_internal_module(Renderer
    TYPE                STATIC
    SOURCES             ${SOURCES_LIST}
    INCLUDES            "${CMAKE_CURRENT_SOURCE_DIR}/includes"
						"${CMAKE_CURRENT_SOURCE_DIR}/includes/renderer"
    PRIVATE_INCLUDES    "${CMAKE_CURRENT_SOURCE_DIR}/sources"
    PRIVATE_DEPENDENCY  Core::Core
    ALIAS               Runtime::Renderer
)
add_library(Runtime_Renderer INTERFACE)
target_link_libraries(Runtime_Renderer INTERFACE Renderer)
