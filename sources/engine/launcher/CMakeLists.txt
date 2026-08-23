
set(TARGET_NAME Launcher)

file(GLOB_RECURSE SOURCES_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)
add_executable(${TARGET_NAME} Main.cpp ${SOURCES_LIST})

target_compile_features(${TARGET_NAME} PUBLIC cxx_std_23)
target_link_libraries(${TARGET_NAME} PRIVATE 
	Module::Application 
	Core::Core
	Runtime::Runtime
)
