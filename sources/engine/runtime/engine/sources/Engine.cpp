#include "engine/Engine.hpp"

namespace runtime
{
void Engine::preinitialize() 
{
	// 初始化日志系统
    
	// 初始化底层 UI 系统, 获得窗口和事件分发

	// 初始化 TimerManager

	// 初始化 RHI

}
void Engine::initialize() 
{
	// 初始化网络模块

	// 初始化音频视频组件

	// 初始化物理模块

	// 初始化渲染线程
	RenderThread = std::make_unique<runtime::RenderThread>();

	// 加载整个引擎的配置

	// 加载 Game 或者启动 Editor 模式 

}
void Engine::postinitialize() {}
void Engine::run()
{
	if (!ShouldQuit)
	{
		if (IsFirstFrame)
		{
			postinitialize();
			IsFirstFrame = false;
		}

		// 计算 DeltaTime

		// 处理输入事件, 向 UI 逐层传递, UI 不处理再传递给世界

		// 更新世界

		// 渲染

		
		// 处理日志
	}
}
void Engine::tick(float DeltaTime) {}
void Engine::render() {}
void Engine::quit() { ShouldQuit = true; }
void Engine::destroy() 
{
}

}