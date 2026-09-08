#pragma once

#include "RHI.h"

#include <cstdint>
#include <future>
#include <memory>

namespace rhi
{

struct PipelineManagerDescriptor
{
	/** 固定数量后台编译线程；0 表示至少创建一个工作线程。 */
	uint32_t WorkerCount { 1 };
};

struct PipelineManagerStatistics
{
	uint64_t CacheHits { 0 };
	uint64_t CacheMisses { 0 };
	uint64_t PendingCompilations { 0 };
	uint64_t CachedPipelines { 0 };
};

/**
 * @brief 引擎级语义 Pipeline 缓存与有界异步编译管理器。
 *
 * Manager 在创建 Native Pipeline 前从完整、规范化 Descriptor 构造碰撞安全 Key。
 * 同一 Key 的并发请求共享一个 shared_future；驱动编译期间不持有缓存锁。
 * RDevice 由 Manager 共享持有，因此所有后台任务结束前设备不会提前销毁。
 */
class PipelineManager final
{
public:
	explicit PipelineManager(
		std::shared_ptr<RDevice> Device,
		const PipelineManagerDescriptor& Desc = {});
	~PipelineManager();

	PipelineManager(const PipelineManager&) = delete;
	PipelineManager& operator=(const PipelineManager&) = delete;

	[[nodiscard]] std::shared_ptr<RPipeline> getOrCreateGraphics(
		const GraphicsPipelineDescriptor& Desc);
	[[nodiscard]] std::shared_ptr<RPipeline> getOrCreateCompute(
		const ComputePipelineDescriptor& Desc);
	[[nodiscard]] std::shared_future<std::shared_ptr<RPipeline>> getOrCreateGraphicsAsync(
		GraphicsPipelineDescriptor Desc);
	[[nodiscard]] std::shared_future<std::shared_ptr<RPipeline>> getOrCreateComputeAsync(
		ComputePipelineDescriptor Desc);

	/** 清除已完成缓存；正在编译的请求继续完成，但结果不再写回已清除代际。 */
	void clear();
	[[nodiscard]] PipelineManagerStatistics getStatistics() const noexcept;

private:
	class Impl;
	std::unique_ptr<Impl> Implementation;
};

} // namespace rhi
