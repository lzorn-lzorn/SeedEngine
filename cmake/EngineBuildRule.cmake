
# 使用 SDL 作为窗口和程序入口点的后端
option(USE_SDL "Enable SDL support" ON)

# 使用 SDL_GPU 作为渲染RHI的后端
option(USE_SDLGPU_AS_RHI "Enable SDL_GPU support as RHI" ON)

# 使用 Vulkan 作为渲染RHI的后端
option(USE_VULKAN_AS_RHI "Enable Vulkan support as RHI" ON)

# 使用编辑器
option(USE_EDITOR "Enable Editor support" ON)

# 启用 RHI 后端的附加诊断。语义和内存安全检查不会因该选项关闭而移除。
option(RHI_ENABLE_VALIDATION "Enable additional RHI backend validation" ON)

# 收集 Pipeline 可执行文件统计。默认关闭，避免发布构建承担驱动查询与热路径计数成本。
option(RHI_ENABLE_PIPELINE_STATISTICS "Enable RHI pipeline compilation statistics" OFF)