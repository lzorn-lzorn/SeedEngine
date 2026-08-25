
set(TARGET_NAME Launcher)

add_executable(${TARGET_NAME})

target_sources(${TARGET_NAME} 
PRIVATE
	EngineMain.hpp
	EngineMain.cpp
	CommandLine.hpp
	ModuleLoader.hpp
)

target_include_directories(${TARGET_NAME}
PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

if(USE_SDL)
	target_sources(${TARGET_NAME} 
	PRIVATE
		entries/sdl/Main.cpp
	)
	target_link_libraries(${TARGET_NAME} PRIVATE SDL3::SDL3)
else()
	 if(WIN32)
        target_sources(${TARGET_NAME}
		PRIVATE
			entries/windows/Main.cpp
        )

    elseif(APPLE)
        target_sources(${TARGET_NAME}
		PRIVATE
			entries/macos/Main.cpp
        )
    elseif(UNIX)

	else()
		message(FATAL_ERROR "Unsupported platform")
    endif()
endif()

target_link_libraries(${TARGET_NAME} PRIVATE 
	Module::Application 
	Core::Core
	Runtime::Runtime
)
