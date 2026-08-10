set(SOURCES_LIST)

add_internal_module(Application
    TYPE                STATIC
    SOURCES             SOURCES_LIST
    INCLUDES            include
    PRIVATE_DEPENDENCY  SDL3::SDL3
)