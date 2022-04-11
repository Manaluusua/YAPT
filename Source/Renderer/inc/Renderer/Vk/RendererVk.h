#pragma once

#include <Renderer/Renderer.h>
#include <Renderer/Shared/GfxTypes.h>


#include <Renderer/Shared/MeshManager.h>
#include <Renderer/Shared/MaterialManager.h>
#include <Renderer/Vk/SubmissionThreadVk.h>
#include <Renderer/Vk/SyncUtilities.h>

#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <memory>


namespace YAPT
{
	class ResourceManagerVk;
	class RendererVk
	{
	public:

		RendererVk(const GfxApiInitConfig& config);
		~RendererVk();
		
		bool initialize();

		void prepare();
		void renderBegin();
		void executeBegin();
		void executeEnd();

		void waitForAllFramesDone();

		VkDevice getDevice() const { return m_device; }
		ResourceManagerVk* getResourceManager() { return m_resourceManager; }
		const VkPhysicalDeviceProperties2& getDeviceProps() const { return m_physicalDeviceInfos.properties[m_selectedPhysicalDeviceIndex]; }

		//SwapChainDx12* createSwapChain(const WindowSurfaceDefinition& windowSurface);
		//void destroySwapChain(SwapChainDx12* swapChain);
		//void present(SwapChainDx12* swapChain);

		//CommandListPoolerDx12* createCommandListPooler();
		//void destroyCommandListPooler(CommandListPoolerDx12* pooler);

		//void submitCommandLists(CommandBufferHandle* buffers, size_t numberOfBuffers);

		const QueueDefinitionVk& getGraphicsQueue() const { return m_graphicsQueue; }
		const QueueDefinitionVk& getComputeQueue() const { return m_computeQueue; }
		const QueueDefinitionVk& getCopyQueue() const { return m_copyQueue; }

		size_t getFrameNumber() const { return m_syncUtility.getFrameCount(); }
		size_t getFramePipelineIndex() const { return m_syncUtility.getFrameIndex(); }
		size_t getFramePipelineLength() const { return m_syncUtility.getFramesInFlight(); }

		//extension functions. For now use expose here, could move these somewhere else or or vk loader 
		PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
		PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
		PFN_vkCmdTraceRaysIndirectKHR vkCmdTraceRaysIndirectKHR;
		PFN_vkGetRayTracingShaderGroupStackSizeKHR vkGetRayTracingShaderGroupStackSizeKHR;
		PFN_vkCmdSetRayTracingPipelineStackSizeKHR vkCmdSetRayTracingPipelineStackSizeKHR;
	private:

		struct PhysicalDeviceFeatures
		{
			PhysicalDeviceFeatures()
				:baseFeatures{},
				descIndexingFeatures{},
				rtFeatures{}
			{
				baseFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
				descIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
				rtFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
				baseFeatures.pNext = &descIndexingFeatures;
				descIndexingFeatures.pNext = &rtFeatures;
				
			}

			VkPhysicalDeviceFeatures2 baseFeatures;
			VkPhysicalDeviceDescriptorIndexingFeatures descIndexingFeatures;
			VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures;
		};

		struct PhysicalDeviceInfos
		{
			std::vector<VkPhysicalDevice> devices;
			std::vector<VkPhysicalDeviceProperties2> properties;
			std::vector<PhysicalDeviceFeatures> features;
			std::vector<std::vector<VkQueueFamilyProperties> > queueFamilyProperties;
		};

		struct PresentInfos
		{
			std::vector<std::vector<VkBool32> > presentSupport;
			std::vector<std::vector<VkSurfaceFormatKHR> > supportedSurfaceFormats;
			std::vector<VkSurfaceCapabilitiesKHR> surfaceCapabilities;
		};

		struct SelectedDeviceConfiguration
		{
			uint32_t selectedPhysicalDeviceIndex;
			uint32_t graphicsQueueFamilyIndex;
			uint32_t computeQueueFamilyIndex;
			uint32_t transferQueueFamilyIndex;
		};

		void fetchInstanceExtensions(std::vector<VkExtensionProperties>& extensions);
		bool createInstance();
		void queryPhysicalDeviceInfos();
		void queryPresentInfosForSurface(VkSurfaceKHR surface);
		void selectPhysicalDevice(SelectedDeviceConfiguration& config);
		void loadExtensionMethods();
		void printQueueFamilyCapabilities();

		bool createDevice(const SelectedDeviceConfiguration& config);

		const PhysicalDeviceFeatures* getRequiredPhysicalDeviceFeatures();
		bool hasRequiredPhysicalDeviceFeatures(const PhysicalDeviceFeatures& features);

		//bool createDevice();
		//bool createCoreResources();

		void deinitialize();

		GfxApiInitConfig m_gfxConfig;
		VkInstance m_instance;
		VkDevice m_device;
		VkSurfaceKHR m_surface;
		PhysicalDeviceInfos m_physicalDeviceInfos;
		PresentInfos m_presentInfos;
		uint32_t m_selectedPhysicalDeviceIndex;
		SubmissionThreadVk m_submissionThread;

		QueueDefinitionVk m_copyQueue;
		QueueDefinitionVk m_computeQueue;
		QueueDefinitionVk m_graphicsQueue;

		ResourceManagerVk* m_resourceManager;
		SubmitSyncUtility m_syncUtility;

	};



}
