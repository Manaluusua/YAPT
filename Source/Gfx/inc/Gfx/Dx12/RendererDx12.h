#ifndef YAPT_DX12_RENDERERDX12_H
#define YAPT_DX12_RENDERERDX12_H


#include <Gfx/GfxTypes.h>
#include <Gfx/Dx12/Dx12CommonIncludes.h>
#include <Gfx/Dx12/FenceHelperDx12.h>
#include <Gfx/Dx12/SubmissionThreadDx12.h>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <memory>


struct IDXGIFactory4;
struct IDXGISwapChain3;
struct IDXGIAdapter;

namespace YAPT
{
	class SwapChainDx12;
	class ResourceManagerDx12;
	class AccelerationStructureBuilder;
	class RenderPipeline;
	class RendererDx12
	{
	public:
		RendererDx12(const GfxApiInitConfig& config);
		~RendererDx12();
		
		bool initialize();

		void prepare();
		void renderBegin();
		void executeBegin();
		void executeEnd();

		void waitForAllFramesDone();

		

		SwapChainDx12* createSwapChain(const WindowSurfaceDefinition& windowSurface);
		void destroySwapChain(SwapChainDx12* swapChain);
		void present(SwapChainDx12* swapChain);

		CommandListPoolerDx12* createCommandListPooler();
		void destroyCommandListPooler(CommandListPoolerDx12* pooler);

		void submitCommandLists(CommandBufferHandle* buffers, size_t numberOfBuffers);

		ResourceManagerDx12& getResourceManager() const { return *m_resourceManager; }
		AccelerationStructureBuilder& getAccelerationStructureBuilder() const { return *m_accStructureBuilder; }
	private:

		bool createDevice();
		bool createCoreResources();

		void deinitialize();

		FrameCycleFenceHelper m_fenceHelper;
		SubmissionThreadDx12 m_submitThread;

		RCPtr<IDXGIFactory4> m_factory;
		RCPtr<ID3D12Device5> m_device;
		RCPtr<IDXGIAdapter> m_adapter;
		RCPtr<ID3D12CommandQueue> m_mainGraphicsQueue;
		RCPtr<ID3D12CommandQueue> m_copyQueue;
		

		std::unique_ptr<ResourceManagerDx12> m_resourceManager;
		std::unique_ptr<AccelerationStructureBuilder> m_accStructureBuilder;
		

		std::vector<CommandListPoolerDx12*> m_commandListPoolers;
		std::vector<ID3D12CommandList*> m_submittedCommandLists;

		std::vector<SwapChainDx12*> m_swapChainsToPresentAfterFrameEnd;

		SubmissionThreadDx12::SubmissionId m_lastSubmitId;
		GfxApiInitConfig m_config;

	};



}



#endif