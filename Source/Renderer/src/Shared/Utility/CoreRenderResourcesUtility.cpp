#include <Renderer/Shared/Utility/CoreRenderResourcesUtility.h>
#include <Renderer/Shared/GfxApi.h>
#include <Common/FileSystemPath.h>
#include <Renderer/Shared/Utility/RenderAPIAbstractionUtility.h>
#include <Renderer/Shared/Utility/PipelineStateDescriptionUtility.h>
#include <Renderer/Shared/CRenderer.h>

#define NOISE_TEXTURE_DIM 128

namespace YAPT
{

	CoreRenderResourcesUtility::CoreRenderResourcesUtility(CRenderer* renderer)
		:m_renderer(renderer),
		m_gfx(renderer->getGfxHandle()),
		m_postProcessPool(m_gfx)
	{
		initialize();
	}
	CoreRenderResourcesUtility::~CoreRenderResourcesUtility()
	{
		deinitialize();
	}


	SamplerHandle CoreRenderResourcesUtility::getDefaultSampler(DefaultSamplerType type) const
	{
		switch (type)
		{
		case YAPT::DefaultSamplerType::LINEAR_REPEAT:
			return m_linearSamplerRepeat;
		case YAPT::DefaultSamplerType::NEAREST_REPEAT:
			return m_nearestSamplerRepeat;
		case YAPT::DefaultSamplerType::LINEAR_CLAMP:
			return m_linearSamplerClamp;
		case YAPT::DefaultSamplerType::NEAREST_CLAMP:
			return m_nearestSamplerClamp;
		default:
			return YAPT_NULL_HANDLE;
		}
	}
	BufferViewHandle CoreRenderResourcesUtility::getDefaultBufferView(DefaultBufferType type) const
	{
		switch (type)
		{
		case YAPT::DefaultBufferType::FULLSCREEN_PRIMITIVE_INDICES:
			return m_fullscreenPrimIndexBuffer.bufferView;
		case YAPT::DefaultBufferType::FULLSCREEN_PRIMITIVE_POS:
			return m_fullscreenPrimVertexBufferPos.bufferView;
		case YAPT::DefaultBufferType::FULLSCREEN_PRIMITIVE_POS_UV:
			return m_fullscreenPrimVertexBufferPosUv.bufferView;
		default:
			return YAPT_NULL_HANDLE;
		}
	}
	TextureViewHandle CoreRenderResourcesUtility::getDefaultTextureView(DefaultTextureType type) const
	{
		switch (type)
		{
		case YAPT::DefaultTextureType::BLACK:
			return m_blackTex.textureView;
		case YAPT::DefaultTextureType::WHITE:
			return m_whiteTex.textureView;
		case YAPT::DefaultTextureType::NOISE:
			return m_noiseTex.textureView;
		default:
			return YAPT_NULL_HANDLE;
		}
	}

	const VertexBufferDefinition* CoreRenderResourcesUtility::getDefaultVertexBufferDefinition(DefaultBufferType type) const
	{
		switch (type)
		{
		case YAPT::DefaultBufferType::FULLSCREEN_PRIMITIVE_POS:
		{
			static VertexInputAttribute attributes[] = { {AttributeSemanticName::POSITION, ResourceFormat::RGB32_SFLOAT, 0} };
			static VertexBufferDefinition vertexBufferDef{ attributes, 1, sizeof(vec3p), 0 };
			return &vertexBufferDef;
		}
			
		case YAPT::DefaultBufferType::FULLSCREEN_PRIMITIVE_POS_UV:
		{
			static VertexInputAttribute attributes[] = { {AttributeSemanticName::POSITION, ResourceFormat::RGB32_SFLOAT, 0}, {AttributeSemanticName::TEXCOORD, ResourceFormat::RG32_SFLOAT, sizeof(vec3p)} };
			static VertexBufferDefinition vertexBufferDef{ attributes, 2, sizeof(vec3p) + sizeof(vec2p), 0 };
			return &vertexBufferDef;
		}
			
		default:
			return nullptr;
		}
	}


	void CoreRenderResourcesUtility::createBuffers()
	{
		const FullScreenPrimitive& fullscreenPrim = getFullscreenPrimitive();
		size_t fullscreenPrimitivePosSize = sizeof(vec3p) * fullscreenPrim.vertexCount;
		size_t fullscreenPrimitiveUvSize = sizeof(vec2p) * fullscreenPrim.vertexCount;
		size_t fullscreenPrimitiveIndexSize = sizeof(uint32_t) * fullscreenPrim.indexCount;
		{
			BufferDesc vertexBufferDesc( RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_VERTEX_BUFFER, fullscreenPrimitivePosSize );
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_WRITE, SHADERSTAGE_NONE };
			m_fullscreenPrimVertexBufferPos.buffer = m_postProcessPool.requestBuffer(vertexBufferDesc, resState);

			vertexBufferDesc.sizeInBytes += fullscreenPrimitiveUvSize;
			m_fullscreenPrimVertexBufferPosUv.buffer = m_postProcessPool.requestBuffer(vertexBufferDesc, resState);

		}

		{
			BufferDesc indexBufferDesc( RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_INDEX_BUFFER, fullscreenPrimitiveIndexSize );
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_WRITE, SHADERSTAGE_NONE };
			m_fullscreenPrimIndexBuffer.buffer = m_postProcessPool.requestBuffer(indexBufferDesc, resState);

		}
	}
	void CoreRenderResourcesUtility::createTextures()
	{
		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_2D, ResourceFormat::RGBA8_UNORM, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, 1, 1, 1, 1);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_WRITE, SHADERSTAGE_NONE };

			m_blackTex.texture = m_postProcessPool.requestTexture(texDesc, resState, "blackTex");
			m_whiteTex.texture = m_postProcessPool.requestTexture(texDesc, resState, "whiteTex");
		}
		

		{
			TextureDesc texDesc(ResourceDimension::TEXTURE_2D, ResourceFormat::RGBA8_UNORM, RESOURCE_USAGE_COPY_DESTINATION | RESOURCE_USAGE_SAMPLED_TEXTURE, NOISE_TEXTURE_DIM, NOISE_TEXTURE_DIM, 1, 1);
			ResourceStateDescription resState{ RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_WRITE, SHADERSTAGE_NONE };
			m_noiseTex.texture = m_postProcessPool.requestTexture(texDesc, resState, "noiseTex");

		}
		m_multiScatteringLUTs.initializeLUTStorage(m_renderer, m_postProcessPool);
		

	}

	void CoreRenderResourcesUtility::createBufferViews()
	{
		m_fullscreenPrimVertexBufferPos.bufferView = Gfx::getBufferView(m_gfx, m_fullscreenPrimVertexBufferPos.buffer, {});
		m_fullscreenPrimVertexBufferPosUv.bufferView = Gfx::getBufferView(m_gfx, m_fullscreenPrimVertexBufferPosUv.buffer, {});
		m_fullscreenPrimIndexBuffer.bufferView = Gfx::getBufferView(m_gfx, m_fullscreenPrimIndexBuffer.buffer, { 0, YAPT_BUFFER_WHOLE_RESOURCE, 0, ResourceFormat::R32_UINT });
	}
	void CoreRenderResourcesUtility::createTextureViews()
	{
		m_blackTex.textureView = Gfx::getTextureView(m_gfx, m_blackTex.texture, { ResourceFormat::RGBA8_UNORM });
		m_whiteTex.textureView = Gfx::getTextureView(m_gfx, m_whiteTex.texture, { ResourceFormat::RGBA8_UNORM });
		m_noiseTex.textureView = Gfx::getTextureView(m_gfx, m_noiseTex.texture, { ResourceFormat::RGBA8_UNORM });
	}

	void CoreRenderResourcesUtility::uploadBuffers()
	{
		//upload
		const FullScreenPrimitive& fullscreenPrim = getFullscreenPrimitive();
		size_t fullscreenPrimitivePosSize = sizeof(vec3p) * fullscreenPrim.vertexCount;
		size_t fullscreenPrimitiveUvSize = sizeof(vec2p) * fullscreenPrim.vertexCount;
		size_t fullscreenPrimitiveIndexSize = sizeof(uint32_t) * fullscreenPrim.indexCount;


		std::vector<float> buff;
		buff.reserve((fullscreenPrimitivePosSize + fullscreenPrimitiveUvSize) / 4);

		{
			for (size_t i = 0; i < fullscreenPrim.vertexCount; ++i)
			{
				buff.push_back(fullscreenPrim.positions[i].x);
				buff.push_back(fullscreenPrim.positions[i].y);
				buff.push_back(fullscreenPrim.positions[i].z);
			}

			Gfx::uploadBuffer(m_gfx, m_fullscreenPrimVertexBufferPosUv.buffer, 0, fullscreenPrimitivePosSize, buff.data(), GpuUploadStage::BEFORE_RENDER);

			buff.clear();
		}

		{
			for (size_t i = 0; i < fullscreenPrim.vertexCount; ++i)
			{
				buff.push_back(fullscreenPrim.positions[i].x);
				buff.push_back(fullscreenPrim.positions[i].y);
				buff.push_back(fullscreenPrim.positions[i].z);

				buff.push_back(fullscreenPrim.uvCoordinates[i].x);
				buff.push_back(fullscreenPrim.uvCoordinates[i].y);
			}

			Gfx::uploadBuffer(m_gfx, m_fullscreenPrimVertexBufferPosUv.buffer, 0, fullscreenPrimitivePosSize + fullscreenPrimitiveUvSize, buff.data(), GpuUploadStage::BEFORE_RENDER);
		}

		Gfx::uploadBuffer(m_gfx, m_fullscreenPrimIndexBuffer.buffer, 0, fullscreenPrimitiveIndexSize, fullscreenPrim.indices, GpuUploadStage::BEFORE_RENDER);

	}
	void CoreRenderResourcesUtility::uploadTextures()
	{
		{
			const uint8_t datablack[] = { 0u,0u,0u,255u };
			const uint8_t datawhite[] = { 255u,255u,255u,255u };
			const TextureDataDefinition data[] = { {4, datablack}, {4, datawhite} };
			Gfx::uploadTexture(m_gfx, m_blackTex.texture, 0, 1, 0, 1, &data[0], GpuUploadStage::BEFORE_RENDER);
			Gfx::uploadTexture(m_gfx, m_whiteTex.texture, 0, 1, 0, 1, &data[1], GpuUploadStage::BEFORE_RENDER);
		}
		{
			RendererCacheProvider* cache = m_renderer->getCacheProvider();
			bool loadedFromCache = false;
			std::vector<uint8_t> data(NOISE_TEXTURE_DIM * NOISE_TEXTURE_DIM * 4);
			RendererCacheProvider::ImageDefinition def;
			def.width = NOISE_TEXTURE_DIM;
			def.height = NOISE_TEXTURE_DIM;
			def.depthOrSlices = 1;
			def.format = ResourceFormat::RGBA8_UNORM;
			def.dimension = ResourceDimension::TEXTURE_2D;
			def.mips = 1;

			if (cache)
			{
				loadedFromCache = cache->loadImage("noise", def, data.data());
			}

			if (!loadedFromCache)
			{
				for (size_t i = 0; i < NOISE_TEXTURE_DIM * NOISE_TEXTURE_DIM; ++i)
				{
					size_t componentIndex = i * 4;
					data[componentIndex++] = uint8_t(((float(std::rand()) / RAND_MAX) * 255.f));
					data[componentIndex++] = uint8_t(((float(std::rand()) / RAND_MAX) * 255.f));
					data[componentIndex++] = uint8_t(((float(std::rand()) / RAND_MAX) * 255.f));
					data[componentIndex++] = uint8_t(((float(std::rand()) / RAND_MAX) * 255.f));
				}
			}

			if (cache)
			{
				cache->storeImage("noise", def, data.data());
			}
			
			TextureDataDefinition texDataDef;
			texDataDef.data = data.data();
			texDataDef.rowPitchInBytes = NOISE_TEXTURE_DIM;
			Gfx::uploadTexture(m_gfx, m_noiseTex.texture, 0, 1, 0, 1, &texDataDef, GpuUploadStage::BEFORE_RENDER);
		}
		
		m_multiScatteringLUTs.initializeLUTContents();
	}

	void CoreRenderResourcesUtility::createSamplers()
	{
		SamplerDescription desc;
		fillDefaults(desc);

		m_nearestSamplerRepeat = Gfx::createSampler(m_gfx, desc);

		desc.addressModeU = desc.addressModeV = desc.addressModeW = SamplerAddressMode::CLAMP_TO_EDGE;
		m_nearestSamplerClamp = Gfx::createSampler(m_gfx, desc);

		desc.magFilter = desc.minFilter = Filter::LINEAR;
		m_linearSamplerClamp = Gfx::createSampler(m_gfx, desc);
		desc.addressModeU = desc.addressModeV = desc.addressModeW = SamplerAddressMode::REPEAT;
		m_linearSamplerRepeat = Gfx::createSampler(m_gfx, desc);
		
	}

	void CoreRenderResourcesUtility::unloadSamplers()
	{
		Gfx::destroySampler(m_gfx, m_linearSamplerRepeat);
		Gfx::destroySampler(m_gfx, m_nearestSamplerRepeat);
		Gfx::destroySampler(m_gfx, m_linearSamplerClamp);
		Gfx::destroySampler(m_gfx, m_nearestSamplerClamp);

	}


	void CoreRenderResourcesUtility::initialize()
	{
		createBuffers();
		createTextures();
		createSamplers();
		m_postProcessPool.allocate();
		createBufferViews();
		createTextureViews();
		uploadBuffers();
		uploadTextures();

	}
	void CoreRenderResourcesUtility::deinitialize()
	{
		m_postProcessPool.deallocate();
		unloadSamplers();
	}



}