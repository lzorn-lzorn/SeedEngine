
set(TARGET_NAME Launcher)

add_executable(${TARGET_NAME} Main.cpp)

target_compile_features(${TARGET_NAME} PUBLIC cxx_std_23)
target_link_libraries(${TARGET_NAME} PRIVATE Module::Application)
