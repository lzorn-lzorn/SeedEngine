
# 使用 SDL 作为窗口和程序入口点的后端
option(USE_SDL "Enable SDL support" ON)

# 使用 SDL_GPU 作为渲染RHI的后端
option(USE_SDLGPU_AS_RHI "Enable SDL_GPU support as RHI" ON)

# 使用 Vulkan 作为渲染RHI的后端
option(USE_VULKAN_AS_RHI "Enable Vulkan support as RHI" ON)

# 使用编辑器
option(USE_EDITOR "Enable Editor support" ON)