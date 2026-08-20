#add_subdirectory(application)

file(GLOB Children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

foreach(Child ${Children})
	if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${Child} AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${Child}/CMakeLists.txt)
		add_subdirectory(${Child})
	endif()
endforeach()
