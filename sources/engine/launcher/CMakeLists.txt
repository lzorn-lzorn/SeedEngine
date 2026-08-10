
set(TARGET_NAME Launcher)

add_executable(${TARGET_NAME} Main.cpp)

target_link_libraries(${TARGET_NAME} PRIVATE Module::Application)
