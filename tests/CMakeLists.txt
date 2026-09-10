add_executable(SeedRHIContractTests todo.cpp)
find_package(Vulkan REQUIRED)
target_link_libraries(SeedRHIContractTests PRIVATE Module::RHI)
target_include_directories(SeedRHIContractTests PRIVATE
	${CMAKE_SOURCE_DIR}/sources/engine/modules/rhi/sources)
if(TARGET Vulkan::Vulkan)
	target_link_libraries(SeedRHIContractTests PRIVATE Vulkan::Vulkan)
endif()
add_test(NAME SeedRHI.Contract COMMAND SeedRHIContractTests)