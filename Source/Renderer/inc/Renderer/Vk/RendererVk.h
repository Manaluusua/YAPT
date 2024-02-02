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

		struct PhysicalDeviceInfo
		{
			PhysicalDeviceInfo()
				:baseFeatures{},
				descIndexingFeatures{},
				rayTracePipelineFeatures{},
				rayQueryFeatures{},
				deviceProperties2{},
				raytracePipelineProperties{}
			{
				baseFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
				descIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
				rayTracePipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
				rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;

				deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
				raytracePipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

				//features
				baseFeatures.pNext = &descIndexingFeatures;
				descIndexingFeatures.pNext = &rayTracePipelineFeatures;
				rayTracePipelineFeatures.pNext = &rayQueryFeatures;

				//properties
				deviceProperties2.pNext = &raytracePipelineProperties;

			}

			VkPhysicalDeviceFeatures2 baseFeatures;
			VkPhysicalDeviceDescriptorIndexingFeatures descIndexingFeatures;
			VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracePipelineFeatures;
			VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures;

			VkPhysicalDeviceProperties2 deviceProperties2;
			VkPhysicalDeviceRayTracingPipelinePropertiesKHR  raytracePipelineProperties;
		};

		RendererVk(const GfxApiInitConfig& config);
		~RendererVk();
		
		bool initialize();

		void prepare();
		void renderBegin();
		void executeBegin();
		void executeEnd();

		void waitForAllFramesDone();

		ResourceManagerVk* getResourceManager() { return m_resourceManager; }

		VkDevice getDevice() const { return m_device; }
		VkPhysicalDevice getPhysicalDevice() const { return m_physicalDeviceInfos.devices[m_selectedPhysicalDeviceIndex]; }
		const PhysicalDeviceInfo& getPhysicalDeviceInfo() const { return m_physicalDeviceInfos.features[m_selectedPhysicalDeviceIndex]; }


		SwapChainVk* createSwapChain(const WindowSurfaceDefinition& windowSurface);
		void destroySwapChain(SwapChainVk* swapChain);
		void present(SwapChainVk* swapChain);

		void submitCommandLists(CommandBufferHandle* buffers, size_t numberOfBuffers);

		const QueueDefinitionVk& getGraphicsQueue() const { return m_graphicsQueue; }
		const QueueDefinitionVk& getComputeQueue() const { return m_computeQueue; }
		const QueueDefinitionVk& getCopyQueue() const { return m_copyQueue; }

		SubmissionThreadVk& getSubmissionThread() { return m_submissionThread; }
		

		size_t getFrameNumber() const { return m_syncUtility.getFrameCount(); }
		size_t getFramePipelineIndex() const { return m_syncUtility.getFrameIndex(); }
		size_t getFramePipelineLength() const { return m_syncUtility.getFramesInFlight(); }
		ResourceFormat getSwapChainFormat() const { return m_selectedSwapChainFormat;}

		//extension functions. For now use expose here, could move these somewhere else or or vk loader 
		PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
		PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
		PFN_vkCmdTraceRaysIndirectKHR vkCmdTraceRaysIndirectKHR;
		PFN_vkGetRayTracingShaderGroupStackSizeKHR vkGetRayTracingShaderGroupStackSizeKHR;
		PFN_vkCmdSetRayTracingPipelineStackSizeKHR vkCmdSetRayTracingPipelineStackSizeKHR;
	private:

		

		struct PhysicalDeviceInfos
		{
			std::vector<VkPhysicalDevice> devices;
			std::vector<PhysicalDeviceInfo> features;
			std::vector<std::vector<VkQueueFamilyProperties> > queueFamilyProperties;
			std::vector<std::vector<VkBool32>> supportsPresent;
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

		const PhysicalDeviceInfo* getRequiredPhysicalDeviceFeatures();
		bool hasRequiredPhysicalDeviceFeatures(const PhysicalDeviceInfo& features);

		void deinitialize();

		GfxApiInitConfig m_gfxConfig;
		VkInstance m_instance;
		VkDevice m_device;
		VkSurfaceKHR m_surface;
		PhysicalDeviceInfos m_physicalDeviceInfos;
		uint32_t m_selectedPhysicalDeviceIndex;
		SubmissionThreadVk m_submissionThread;
		SubmissionThreadVk::SubmissionId m_lastSubmitId;

		QueueDefinitionVk m_copyQueue;
		QueueDefinitionVk m_computeQueue;
		QueueDefinitionVk m_graphicsQueue;

		ResourceManagerVk* m_resourceManager;
		RingSyncUtility m_syncUtility;
		ResourceFormat m_selectedSwapChainFormat;

		std::vector<VkCommandBuffer> m_submittedCommandBuffers;
		std::vector<SwapChainVk*> m_swapChainsToPresent;
		std::vector<VkSemaphore> m_semaphoresToSignal;
		std::vector<VkSemaphore> m_semaphoresToWait;
		struct SubmittedSwapChains 
		{
			SwapChainVk** swapchainsToPresent;
			size_t numberOfSwapchains;
			VkQueue presentQueue;
		} m_submittedSwapChainPresents;
	};



}
