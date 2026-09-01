#include <memory>
#include <new>
#include <exception>
#include <iostream>

#include "EngineMain.hpp"
#include <Application.hpp>
#include "Engine.hpp"
#include "RTGlobal.hpp"
#include "CommandLine.hpp"
#include "ModuleLoader.hpp"
#include <RHIServer.hpp>

int EngineMain(int argc, char** argv)
{
	try
	{
		CommandLine cmd(argc, argv);
		// 初始化内存分配器

		// 初始化模块加载器
		ModuleLoader::self();
		
		// 初始化引擎
		GSeedEngine.preinitialize();

		// 初始化通用窗口系统；当前工厂选择 SDL3 实现。
		auto generic_application = app::createGenericApplication();
		ui::WindowDescriptor window_descriptor {
			.Title = "SeedEngine RHI Test",
			.LeftTopPoint = ui::UIVector(100, 100),
			.Width = 1280,
			.Height = 720,
			.WindowType = ui::EWindowType::Windowed,
			.IsResizable = true,
			.IsVisiable = true,
			.HasBorder = true,
			.AcceptsInputs = true,
			.IsDialog = false
		};
		ui::GenericWindowPointer generic_window = generic_application->makeWindow(window_descriptor);
		generic_window->show();

		// RenderServer 只接收跨平台 GenericWindow，并在内部选择和初始化具体 RHI。
		auto& render_server = rhi::RenderServer::self();
		render_server.initialize(rhi::ESupportedBackendAPI::Vulkan, generic_window);
		if (!render_server.isInitialized())
		{
			throw std::runtime_error("RHI initialization did not complete.");
		}

		GSeedEngine.initialize();
		GSeedEngine.run();
		GSeedEngine.destroy();

		render_server.shutdown();
		generic_window.reset();
		generic_application.reset();
		return 0;
	}
	catch (const std::exception& exception)
	{
		rhi::RenderServer::self().shutdown();
		GSeedEngine.destroy();
		std::cerr << "SeedEngine startup failed: " << exception.what() << '\n';
		return 1;
	}
}