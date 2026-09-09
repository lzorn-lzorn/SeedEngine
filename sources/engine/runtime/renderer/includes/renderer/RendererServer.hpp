#pragma once

#include <RHI.h>

#include <memory>

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

	/**
	 * @brief Exercises reflection, automatic resource resolution and descriptor binding.
	 * @throws std::exception if any BindGroup layer is invalid.
	 * @note This deterministic startup smoke test uses the repository SPIR-V fixture.
	 */
	void runBindGroupSmokeTest();

private:
	RendererServer() = default;
	~RendererServer();
};

} // namespace runtime::renderer
