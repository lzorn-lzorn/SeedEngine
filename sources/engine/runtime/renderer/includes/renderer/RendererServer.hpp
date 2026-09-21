#pragma once

#include <RHI.hpp>

#include <memory>
#include <functional>
#include <optional>

/**
 * @brief RendererServer 是一个单例渲染服务, 其面向外部提供统一的渲染服务. 
 *  1. 负责 RHI 的生命周期管理, 包括设备创建, 资源管理, 渲染循环等.
 *  2. 统一封装了 2D渲染器(UI 渲染) 和 3D渲染器(场景渲染, 角色渲染)
 */
namespace runtime::renderer
{

/**
 * @brief Renderer facade that owns the application-facing RHI lifecycle.
 *
 * UI/launcher code depends on this service instead of constructing backend objects.
 * Shader reflection and automatic BindGroup resolution stay in Renderer while native
 * descriptor allocation remains inside RHI.
 */
class RendererServer final
{
public:
	enum class EFrameStatus : uint8_t
	{
		Rendered,
		Skipped,
		SwapchainRecreated,
		SurfaceLost,
		DeviceLost
	};

	using FrameRecorder = std::function<void(
		rhi::RCommandList&,
		const std::shared_ptr<rhi::RImageView>&,
		uint32_t,
		uint32_t)>;
	[[nodiscard]] static RendererServer& self() noexcept;

	RendererServer(const RendererServer&) = delete;
	RendererServer& operator=(const RendererServer&) = delete;

	/**
	 * @brief Initializes the selected backend for a window.
	 * @param BackendAPI Graphics API implementation to create.
	 * @param Window Platform-neutral output window.
	 */
	void initialize(
		rhi::ESupportedBackendAPI BackendAPI,
		const ui::GenericWindowPointer& Window);

	/** @brief Waits for outstanding GPU work and destroys renderer-owned RHI state. */
	void shutdown() noexcept;

	[[nodiscard]] bool isInitialized() const noexcept;
	[[nodiscard]] const std::shared_ptr<rhi::RDevice>& getDevice() const noexcept;
	/** @brief Returns the most recently completed GPU frame interval measured by timestamp queries. @return Nanoseconds, or std::nullopt before the first result or when unsupported. */
	[[nodiscard]] std::optional<double> getLastGPUFrameTimeNanoseconds() const noexcept;

	/** @brief Retains an object until all renderer submissions made so far have completed. */
	[[nodiscard]] bool deferRelease(std::shared_ptr<void> Resource);

	/**
	 * @brief Exercises reflection, automatic resource resolution and descriptor binding.
	 * @throws std::exception if any BindGroup layer is invalid.
	 * @note This deterministic startup smoke test uses the repository SPIR-V fixture.
	 */
	void runBindGroupSmokeTest();

	/**
	 * @brief 录制并提交一帧. 回调在默认颜色渲染作用域内执行. 
	 *
	 * 无回调时执行一次清屏；有回调时可绑定 2D/3D Pipeline, BindGroup 和几何数据. 
	 */
	[[nodiscard]] EFrameStatus renderFrame(const FrameRecorder& Recorder = {});

	/** @brief 通知 Renderer 输出尺寸变化；0 尺寸表示窗口最小化.  */
	void resize(uint32_t Width, uint32_t Height);

	/** @brief Recreates only the native surface and swapchain after SurfaceLost. DeviceLost requires full application/RHI resource rebuild. */
	[[nodiscard]] bool recoverSurface();

private:
	RendererServer();
	~RendererServer();
	struct Implementation;
	std::unique_ptr<Implementation> Impl;
};

} // namespace runtime::renderer
