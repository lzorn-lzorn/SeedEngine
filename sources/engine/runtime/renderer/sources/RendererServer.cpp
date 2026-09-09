#include <renderer/RendererServer.hpp>

#include <renderer/ShaderBinding.hpp>
#include <RHIServer.hpp>

#include <fstream>
#include <stdexcept>
#include <vector>

namespace runtime::renderer
{
namespace
{

std::vector<std::byte> loadBinaryFile(const char* Path)
{
	std::ifstream stream(Path, std::ios::binary | std::ios::ate);
	if (!stream)
		throw std::runtime_error("Failed to open the SPIR-V reflection fixture.");
	const std::streamsize size = stream.tellg();
	if (size <= 0)
		throw std::runtime_error("SPIR-V reflection fixture is empty.");
	stream.seekg(0, std::ios::beg);
	std::vector<std::byte> result(static_cast<size_t>(size));
	if (!stream.read(reinterpret_cast<char*>(result.data()), size))
		throw std::runtime_error("Failed to read the SPIR-V reflection fixture.");
	return result;
}

} // namespace

RendererServer& RendererServer::self() noexcept
{
	static RendererServer instance;
	return instance;
}

RendererServer::~RendererServer()
{
	shutdown();
}

void RendererServer::initialize(
	rhi::ESupportedBackendAPI BackendAPI,
	const ui::GenericWindowPointer& Window)
{
	rhi::RenderServer::self().initialize(BackendAPI, Window);
}

void RendererServer::shutdown() noexcept
{
	auto& server = rhi::RenderServer::self();
	if (const auto& device = server.getDevice())
	{
		try
		{
			device->waitIdle();
		}
		catch (...)
		{
			// Destructors and failure cleanup must not throw; RHI shutdown remains safe.
		}
	}
	server.shutdown();
}

bool RendererServer::isInitialized() const noexcept
{
	return rhi::RenderServer::self().isInitialized();
}

const std::shared_ptr<rhi::RDevice>& RendererServer::getDevice() const noexcept
{
	return rhi::RenderServer::self().getDevice();
}

void RendererServer::runBindGroupSmokeTest()
{
	const auto& device = getDevice();
	if (!device)
		throw std::logic_error("Renderer must be initialized before running its BindGroup smoke test.");

	const auto byte_code = loadBinaryFile(SEED_SPIRV_REFLECT_SAMPLE_PATH);
	const ShaderInterface shader = SpirvShaderReflector::reflect(
		byte_code, rhi::EShaderStage_t::Pixel, "main");
	const PipelineInterface pipeline_interface = PipelineInterfaceBuilder::merge(
		std::span<const ShaderInterface>(&shader, 1));

	ResourceBindingTable resources;
	std::vector<std::shared_ptr<rhi::RBuffer>> buffers;
	std::shared_ptr<rhi::RImage> sampled_image;
	std::shared_ptr<rhi::RImageView> sampled_view;
	std::shared_ptr<rhi::RSampler> sampler;
	for (const auto& reflected : pipeline_interface.Resources)
	{
		switch (reflected.Type)
		{
		case rhi::EDescriptorType::UniformBuffer:
		case rhi::EDescriptorType::ReadOnlyStorageBuffer:
		case rhi::EDescriptorType::ReadWriteStorageBuffer:
		{
			rhi::EBufferUsage usage = reflected.Type == rhi::EDescriptorType::UniformBuffer
				? rhi::EBufferUsage(rhi::EBufferUsage_t::Uniform)
				: rhi::EBufferUsage(rhi::EBufferUsage_t::Storage);
			auto buffer = device->createBuffer(rhi::BufferDescriptor {
				.Size = reflected.BlockSize == 0 ? 256u : reflected.BlockSize,
				.Usage = usage,
				.MemoryProperty = rhi::EMemoryProperty_t::DeviceLocal,
				.DebugName = reflected.Name
			});
			resources.set(reflected.Id, reflected.Name, rhi::BufferBinding { buffer, 0, 0 });
			buffers.emplace_back(std::move(buffer));
			break;
		}
		case rhi::EDescriptorType::SampledTexture:
		{
			if (!sampled_image)
			{
				sampled_image = device->createImage(rhi::RImage::Descriptor_t {
					.Format = rhi::EFormat::RGBA8_UNorm,
					.Dimension = rhi::EImageDimension::Texture2D,
					.Width = 1,
					.Height = 1,
					.Depth = 1,
					.MipLevels = 1,
					.ArrayLayers = 1,
					.SharingMode = rhi::ESharingMode::Exclusive,
					.MemoryProperty = rhi::EMemoryProperty_t::DeviceLocal,
					.Usage = rhi::EImageUsage_t::Sampled,
					.SampleCount = rhi::ESampleCount::Count1
				});
				sampled_view = device->createImageView(rhi::RImageView::Descriptor_t {
					.Image = sampled_image
				});
			}
			resources.set(reflected.Id, reflected.Name, rhi::TextureBinding { sampled_view });
			break;
		}
		case rhi::EDescriptorType::Sampler:
		case rhi::EDescriptorType::ComparisonSampler:
		{
			if (!sampler)
				sampler = device->createSampler();
			resources.set(reflected.Id, reflected.Name, rhi::SamplerBinding { sampler });
			break;
		}
		default:
			throw std::runtime_error("Smoke-test fixture contains an unexpected resource type.");
		}
	}

	ShaderBindingResolver resolver(*device);
	auto resolved = resolver.resolve(pipeline_interface, resources);
	if (resolved.BindGroups.empty() || !resolved.PipelineLayout)
		throw std::runtime_error("Automatic BindGroup resolution returned an incomplete result.");

	auto command_list = device->createCommandList();
	command_list->begin();
	command_list->bindBindGroups(
		rhi::EPipelineType::Graphics,
		resolved.PipelineLayout,
		0,
		resolved.BindGroups);
	command_list->end();
}

} // namespace runtime::renderer
