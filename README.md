# SeedEngine

SeedEngine 当前可用图形后端为 Vulkan 1.3. 现代 RHI 已形成 Acquire → Record → Submit →
Synchronize → Present → Retire 闭环，并实现显式 Barrier, Dynamic Rendering, BindGroup, 
现代内存子分配, BDA, 扩展动态状态, Secondary inheritance, Query, 可选 Ray Tracing 和
WSI/HDR 状态分类；所有可选路径必须以 `DeviceFeatures` 为准. 

完整 2D batching, 3D RenderGraph, 多后端, 自动 SBT/AS compaction 和高级 GPU 集成测试仍未完成. 
文档入口：

- [现代 RHI 与 2D/3D 渲染全流程（实现状态唯一来源）](documents/modern_rhi_rendering_flow_zh.md)
- [Pipeline, CommandList 与 Dynamic Rendering 契约](documents/rhi_pipeline.md)
- [BindGroup, SPIR-V 反射与语义绑定](documents/rhi_bind_groups.md)