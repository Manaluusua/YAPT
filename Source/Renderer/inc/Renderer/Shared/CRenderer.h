#pragma once
#include <Common/RCObjectPtr.h>
#include <Common/JobSystem.h>
#include <Common/ArenaAllocator.h>
#include <Renderer/Renderer.h>
#include <Renderer/Shared/MeshManager.h>
#include <Renderer/Shared/MaterialManager.h>
#include <Renderer/Shared/RenderObjectManager.h>
#include <Gfx/GfxApi.h>
#include <Renderer/Shared/Utility/ShaderLoader.h>
#include <Renderer/Shared/CRendererConfiguration.h>
#include <Renderer/Shared/RenderView.h>
#include <thread>
#include <condition_variable>
#include <mutex>

#define RVARNAME_RENDER_RESOLUTION "Generic.RenderResolution"
#define RVARNAME_SKYBOX "World.Skycube"

#define RVARNAME_ACTIVE_RENDERPIPELINE "RenderPipeline"
#define RVARNAME_DENOISE_MODE "Denoise.Mode"
#define RVARNAME_DENOISE_DEPTH_SIGMA_SCALE "Denoise.DepthSigmaScale"
#define RVARNAME_DENOISE_NORMAL_SIGMA "Denoise.NormalSigma"
#define RVARNAME_DENOISE_MATERIAL_DIFFERENCE_SIGMA "Denoise.MaterialDiffSigma"
#define RVARNAME_ACCUMULATION_DISABLE "_DEBUG.Accumulation.Disable"
#define RVARNAME_DEBUG_BDPT_LIGHTPATHNODES "_DEBUG.BDPT.EffectiveLightPathNodes"
#define RVARNAME_DEBUG_BDPT_CAMERAPATHNODES "_DEBUG.BDPT.EffectiveCameraPathNodes"

#define RVARNAME_TONEMAP_TOE "Tonemap.Toe"
#define RVARNAME_TONEMAP_MID "Tonemap.Mid"
#define RVARNAME_TONEMAP_SHOULDER "Tonemap.Shoulder"
#define RVARNAME_TONEMAP_USE_AUTOEXPOSURE "Tonemap.UseAutoExposure"
#define RVARNAME_TONEMAP_MANUALEXPOSURE "Tonemap.ManualExposure"
#define RVARNAME_TONEMAP_EXPOSURE_COMPENSATION "Tonemap.ExposureCompensation"
#define RVARNAME_TONEMAP_EYE_ADAPT_SPEED "Tonemap.EyeAdaptationSpeed"


namespace YAPT
{
	class CoreRenderResourcesUtility;
	class RenderPipelineManager;
	class TextureImpl;
	class BufferImpl;
	class BindlessTextureManager;
	class BindlessBufferManager;
	class LightManager;

	using RendererFrameAllocator = ArenaAllocator;

	class CRenderer : public Renderer
	{
	public:

		CRenderer();
		virtual ~CRenderer() override;

		//ICRenderer
		virtual void prepare() final;
		virtual void render(const RenderParameters& renderParams) final;
		virtual bool initialize(const RendererInitializeConfig& config) final;
		virtual bool setRenderOutputToSurface(const WindowSurfaceDefinition& windowSurface) final;
		virtual void resetRenderOutput() final;
		
		virtual Texture* createTexture(const char* name, ResourceDimension dimensions, ResourceFormat format, ResourceUsage resourceUsage, uint32_t width, uint32_t height, uint32_t mips = 1, uint32_t depthOrSlices = 1) final;
		virtual Buffer* createBuffer(const char* name, ResourceUsage resourceUsage, size_t size) final;

		virtual Mesh* createMesh(const VertexBufferLayout* layouts, size_t numberOfVertexBufferLayouts, size_t vertexCount, size_t submeshCount, bool use16BitIndices) final;
		virtual Material* createMaterial() final;
		virtual RenderObject* createRenderObject() final;

		virtual RendererConfiguration* getRendererConfiguration() final;

		GfxApiHandle getGfxHandle() { return m_gfxHandle; };
		CoreRenderResourcesUtility* getCoreResources() { return m_coreResourcesUtility; };
		ShaderLoader* getShaderLoader() { return m_shaderLoader; };

		size_t getPipelineLength() const { return m_pipelineLength; };

		const RenderView& getCurrentRenderView() const { return *m_currentRenderView; }
		MeshManager& getMeshManager() { return *m_meshMngr; }
		MaterialManager& getMaterialManager() { return *m_materialMngr; };
		RenderObjectManager& getRenderObjectManager() { return *m_renderObjectManager; }

		void textureToBeCreated(TextureDesc& desc);
		void textureCreated(TextureImpl* t);
		void textureReleased(TextureImpl* t);

		void bufferToBeCreated(BufferDesc& desc);
		void bufferCreated(BufferImpl* t);
		void bufferReleased(BufferImpl* t);

		BindlessTextureManager* getTextureManager() { return m_textureManager; }
		BindlessBufferManager* getBufferManager() { return m_bufferManager; }
		LightManager* getLightManager() { return m_lightManager; }
		CRendererConfiguration& getConcreteRendererConfiguration() { return m_rendererConfig; }

		uint64_t getFrameIndex() const { return m_frameIndex; }

		bool hasViewMoved() const { return m_hasViewMoved; }

		JobSystem& getJobSystem() { return m_jobSystem; }
		RendererFrameAllocator& getRenderFrameAllocator() { return m_perFrameAllocator; }

		RendererCacheProvider* getCacheProvider() const { return m_cacheProvider; }

		float getFrameDeltaInSeconds() const { return m_frameDeltaInSeconds; }

	private:

		void initVariables();

		void deinit();

		static void renderLoopEntry(CRenderer* CRenderer);
		void renderLoop();
		void waitForRenderThreadIdle();
		void executeFrame();

		const size_t m_pipelineLength = 3;
		uint64_t m_frameIndex;

		GfxApiHandle m_gfxHandle;
		SwapChainHandle m_swapChain;

		RenderView* m_currentRenderView;
		
		MeshManager* m_meshMngr;
		MaterialManager* m_materialMngr;
		RenderObjectManager* m_renderObjectManager;
		LightManager* m_lightManager;
		BindlessTextureManager* m_textureManager;
		BindlessBufferManager* m_bufferManager;

		RenderPipelineManager* m_renderPipelineMngr;
		CoreRenderResourcesUtility* m_coreResourcesUtility;
		ShaderLoader* m_shaderLoader;

		RendererCacheProvider* m_cacheProvider;

		CRendererConfiguration m_rendererConfig;

		JobSystem m_jobSystem;
		RendererFrameAllocator m_perFrameAllocator;

		std::thread m_renderWorkerThread;
		std::condition_variable m_renderWorkerCondition;
		std::mutex m_renderWorkerMutex;

		float m_frameDeltaInSeconds;

		bool m_renderWorkPending;
		bool m_shutDownRequested;
		bool m_hasViewMoved;
	};
}
