#include <Renderer/Vk/RendererVk.h>
#include <Renderer/Vk/ResourceManagerVk.h>
#include <Common/Logger.h>
#include <Common/CommonUtilities.h>
#include <Renderer/Vk/SwapChainVk.h>

#define ENABLE_VK_DEBUG_LAYERS
#define PRINT_AVAILABLE_INSTANCE_EXTENSIONS
#define PRINT_QUEUE_FAMILY_PROPERTIES

#define LOAD_INSTANCE_PROC_TO_VAR(var, name) var = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(m_instance, #name))
#define LOAD_INSTANCE_PROC(name) LOAD_INSTANCE_PROC_TO_VAR(name, name)
#define LOAD_DEVICE_PROC_TO_VAR(var, name) var = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(m_device, #name))
#define LOAD_DEVICE_PROC(name) LOAD_DEVICE_PROC_TO_VAR(name, name)

PFN_vkCreateDebugReportCallbackEXT s_createDebugReportCallback = NULL;

VKAPI_ATTR VkBool32 VKAPI_CALL dbgCallback(
	VkDebugReportFlagsEXT       flags,
	VkDebugReportObjectTypeEXT  objectType,
	uint64_t                    object,
	size_t                      location,
	int32_t                     messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData)
{
	YAPT_LOG_DEBUG("%s\n", pMessage);
	return VK_FALSE;  
}

namespace YAPT
{
	RendererVk::RendererVk(const GfxApiInitConfig& config)
		:m_gfxConfig(config),
		m_instance(VK_NULL_HANDLE),
		m_device(VK_NULL_HANDLE),
		m_surface(VK_NULL_HANDLE),
		m_resourceManager(nullptr),
		m_selectedPhysicalDeviceIndex(-1)
	{

	}
	RendererVk::~RendererVk()
	{
		deinitialize();
	}

	

	void RendererVk::deinitialize()
	{
		m_submissionThread.deinitialize();

		if (m_resourceManager)
		{
			delete m_resourceManager;
			m_resourceManager = nullptr;
		}

		m_syncUtility.deinitialize();

		if (m_device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(m_device, nullptr);
			m_device = VK_NULL_HANDLE;
		}

		if (m_surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
			m_surface = VK_NULL_HANDLE;
		}

		if (m_instance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(m_instance, nullptr);
			m_instance = VK_NULL_HANDLE;
		}

		
	}

	void RendererVk::prepare()
	{
		
		m_resourceManager->prepare();

	}
	void RendererVk::renderBegin()
	{
		VkSemaphore s = VK_NULL_HANDLE;
		if (m_syncUtility.getFrameCount() >= m_gfxConfig.pipelineLength)
		{
			size_t frameToSync = m_syncUtility.getFrameCount() - m_gfxConfig.pipelineLength;
			s = m_syncUtility.getSemaphoreForFrameNumber(frameToSync);
		}

		m_resourceManager->flushPreFrameUploads(s);
		//Since our swapchain submit is currently a bit naive, we need to wait here to make sure that the submit thread has processed the request before we might be getting new calls to present swapchain
		while(m_submissionThread.isPending(m_lastSubmitId))
		{
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}
	void RendererVk::executeBegin()
	{
		m_resourceManager->flushFrameUploads();
		m_syncUtility.waitForNextFrame();
		m_resourceManager->resetCommandBufferPools(m_syncUtility.getFrameIndex());
	}
	void RendererVk::executeEnd()
	{
		//issue waits for uploads and swapchains before the commandlist submit
		//TODO: here we are making all commands wait for swapchain images being free. In reality, they are usually only needed at the very end. Should make the rendergraph somehow handle/detect this and break the submission so that the swapchain waits are only for before commandbuffers that need it. Need a mechanism to communicate this tho
		//also should maybe directly just submit to submitthread or call this differently to emphasize that these are just deferred commandlists to be submitted at the end of RG execution

		{
			
			m_semaphoresToSignal.push_back(m_syncUtility.getSemaphoreForThisFrame());

			for (size_t i = 0; i < m_swapChainsToPresent.size(); ++i)
			{
				m_semaphoresToWait.push_back(m_swapChainsToPresent[i]->getBeforeUsageSemaphoreCurrentFrame());
				m_semaphoresToSignal.push_back(m_swapChainsToPresent[i]->getAfterUsageSemaphoreCurrentFrame());
			}
			
			m_semaphoresToWait.push_back(m_resourceManager->getLastSignaledSemaphore());

			SubmissionThreadVk::Submission submission{};
			submission.commandLists = m_submittedCommandBuffers.data();
			submission.commandListsCount = m_submittedCommandBuffers.size();
			submission.semaphoresToWait = m_semaphoresToWait.data();
			submission.semaphoresToWaitCount = m_semaphoresToWait.size();
			submission.semaphoresToSignal = m_semaphoresToSignal.data();
			submission.semaphoresToSignalCount = m_semaphoresToSignal.size();
			submission.fenceToSignal = m_syncUtility.getFenceForThisFrame();

			m_submissionThread.submit(COMMANDQUEUETYPE_GRAPHICS, 0, submission);

			m_semaphoresToWait.clear();
			m_semaphoresToSignal.clear();
			m_submittedCommandBuffers.clear();
		}
		
		//quick and dirty present. Should in reality handle presents just like commandbuffers: add them to some sequential commandlist and process it with submits. 
		
		auto cb = [](void* ptr)
		{
			SubmittedSwapChains* sw = static_cast<SubmittedSwapChains*>(ptr);
			VkQueue presentQueue = sw->presentQueue;
			for (size_t i = 0; i < sw->numberOfSwapchains; ++i)
			{
				sw->swapchainsToPresent[i]->present(presentQueue);
			}
		};

		m_submittedSwapChainPresents.numberOfSwapchains = m_swapChainsToPresent.size();
		m_submittedSwapChainPresents.swapchainsToPresent = m_swapChainsToPresent.data();
		m_submittedSwapChainPresents.presentQueue = getGraphicsQueue().queue;

		m_lastSubmitId = m_submissionThread.issueCallback(cb, &m_submittedSwapChainPresents);
		
		m_swapChainsToPresent.clear();
		m_syncUtility.markThisFrameSyncDataIssued();
		m_syncUtility.nextFrame();
	}


	void RendererVk::waitForAllFramesDone()
	{
		vkDeviceWaitIdle(getDevice());
	}


	void RendererVk::submitCommandLists(CommandBufferHandle* buffers, size_t numberOfBuffers)
	{
		m_submittedCommandBuffers.reserve(m_submittedCommandBuffers.size() + numberOfBuffers);
		for (size_t i = 0; i < numberOfBuffers; ++i)
		{
			m_submittedCommandBuffers.push_back(buffers[i]);
		}
	}

	SwapChainVk* RendererVk::createSwapChain(const WindowSurfaceDefinition& windowSurface)
	{
		SwapChainVk* sc = SwapChainVk::createSwapChain(this, m_selectedSwapChainFormat, windowSurface.width, windowSurface.height, getFramePipelineLength(), m_surface);
		assert(sc && "Failed to create swapchain");
		return sc;
	}
	void RendererVk::destroySwapChain(SwapChainVk* swapChain)
	{
		delete swapChain;
	}
	void RendererVk::present(SwapChainVk* swapChain)
	{
		m_swapChainsToPresent.push_back(swapChain);
	}

	bool RendererVk::initialize()
	{
		bool success = false;
		if (createInstance())
		{
			if (m_gfxConfig.renderSurfaceHandle != YAPT_NULL_HANDLE)
			{
				m_surface = SwapChainVk::createVkSurface(m_instance, m_gfxConfig.renderSurfaceHandle);
			}

			queryPhysicalDeviceInfos();

			SelectedDeviceConfiguration config;
			selectPhysicalDevice(config);

			if (config.selectedPhysicalDeviceIndex != uint32_t(-1))
			{
				success = createDevice(config);
				m_selectedPhysicalDeviceIndex = config.selectedPhysicalDeviceIndex;
			}

		}

		if (success)
		{
			loadExtensionMethods();

			SubmissionThreadVk::Configuration submitThreadConfig;
			std::vector<VkQueue> queues[COMMANDQUEUETYPE_COUNT];
			std::vector<uint32_t> queueFamilies[COMMANDQUEUETYPE_COUNT];
			queues[COMMANDQUEUETYPE_GRAPHICS].push_back(m_graphicsQueue.queue);
			queues[COMMANDQUEUETYPE_COMPUTE].push_back(m_computeQueue.queue);
			queues[COMMANDQUEUETYPE_COPY].push_back(m_copyQueue.queue);

			queueFamilies[COMMANDQUEUETYPE_GRAPHICS].push_back(m_graphicsQueue.queueFamilyIndex);
			queueFamilies[COMMANDQUEUETYPE_COMPUTE].push_back(m_computeQueue.queueFamilyIndex);
			queueFamilies[COMMANDQUEUETYPE_COPY].push_back(m_copyQueue.queueFamilyIndex);


			for (size_t i = 0; i < COMMANDQUEUETYPE_COUNT; ++i)
			{
				submitThreadConfig.numberOfQueues[i] = queues[i].size();
				submitThreadConfig.queues[i] = queues[i].data();
				submitThreadConfig.queueFamily[i] = queueFamilies[i].data();
			}
			//TODO: fill
			m_submissionThread.initialize(submitThreadConfig);

			m_resourceManager = new ResourceManagerVk(m_physicalDeviceInfos.devices[m_selectedPhysicalDeviceIndex], m_submissionThread, m_copyQueue,  m_device, m_gfxConfig.pipelineLength);
			m_syncUtility.initialize(m_device, m_gfxConfig.pipelineLength, true, true);
		}

		if (success)
		{
			m_selectedSwapChainFormat =  SwapChainVk::getPreferredSurfaceFormat(this, m_surface);
		}

		return success;
	}


	void RendererVk::fetchInstanceExtensions(std::vector<VkExtensionProperties>& extensions)
	{

		uint32_t extCount = 0;
		VkResult res = vkEnumerateInstanceExtensionProperties(NULL, &extCount, NULL);
		checkForVkError(res);

		extensions.resize(extCount);

		res = vkEnumerateInstanceExtensionProperties(NULL, &extCount, extensions.data());
		checkForVkError(res);
	}

	bool RendererVk::createInstance()
	{
		std::vector<VkExtensionProperties> availableExtensions;
		fetchInstanceExtensions(availableExtensions);
#ifdef PRINT_AVAILABLE_INSTANCE_EXTENSIONS
		{
			for (size_t i = 0; i < availableExtensions.size(); i++) {
				YAPT_LOG_DEBUG("Instance extension %s found", availableExtensions[i].extensionName);
			}
		}
#endif


		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "YAPT";
		appInfo.engineVersion = 1;
		appInfo.apiVersion = VK_API_VERSION_1_2;

		std::vector<const char*> instanceExtensions = {
	#ifdef ENABLE_VK_DEBUG_LAYERS
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
	#endif        
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		};


		std::vector <const char*>instanceLayers = {
#ifdef ENABLE_VK_DEBUG_LAYERS    
			"VK_LAYER_KHRONOS_validation"
#endif
		};
    

		VkInstanceCreateInfo instanceInfo = {};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pApplicationInfo = &appInfo;
  
		instanceInfo.enabledLayerCount = (uint32_t)instanceLayers.size();
		instanceInfo.ppEnabledLayerNames = instanceLayers.data();

		instanceInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceInfo.ppEnabledExtensionNames = instanceExtensions.data();

		VkResult res = vkCreateInstance(&instanceInfo, NULL, &m_instance);
		checkForVkError(res);

#ifdef ENABLE_VK_DEBUG_LAYERS
		LOAD_INSTANCE_PROC_TO_VAR(s_createDebugReportCallback, vkCreateDebugReportCallbackEXT);
		//s_createDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT"));

		VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
		callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		callbackCreateInfo.pNext = NULL;
		callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		callbackCreateInfo.pfnCallback = &dbgCallback;
		callbackCreateInfo.pUserData = NULL;

		VkDebugReportCallbackEXT callback;
		res = s_createDebugReportCallback(m_instance, &callbackCreateInfo, NULL, &callback);
		checkForVkError(res);
#endif    
		return true;
	}

	void RendererVk::queryPhysicalDeviceInfos()
	{
		uint32_t deviceCount = 0;

		VkResult res = vkEnumeratePhysicalDevices(m_instance, &deviceCount, NULL);
		checkForVkError(res);

		m_physicalDeviceInfos.devices.resize(deviceCount);
		m_physicalDeviceInfos.properties.resize(deviceCount);
		m_physicalDeviceInfos.features.resize(deviceCount);
		m_physicalDeviceInfos.queueFamilyProperties.resize(deviceCount);
		m_physicalDeviceInfos.supportsPresent.resize(deviceCount);
		

		res = vkEnumeratePhysicalDevices(m_instance, &deviceCount, m_physicalDeviceInfos.devices.data());
		checkForVkError(res);


		for (uint32_t deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {

			uint32_t queueFamilyCount;

			m_physicalDeviceInfos.properties[deviceIndex].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

			const VkPhysicalDevice& physicalDevice = m_physicalDeviceInfos.devices[deviceIndex];
			vkGetPhysicalDeviceProperties2(physicalDevice, &m_physicalDeviceInfos.properties[deviceIndex]);
			vkGetPhysicalDeviceFeatures2(physicalDevice, &m_physicalDeviceInfos.features[deviceIndex].baseFeatures);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);


			std::vector<VkQueueFamilyProperties>& queueFamilyProps = m_physicalDeviceInfos.queueFamilyProperties[deviceIndex];
			queueFamilyProps.resize(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProps.data());


			//check for present support
			std::vector<VkBool32>& queueFamilyPresentSupport = m_physicalDeviceInfos.supportsPresent[deviceIndex];
			queueFamilyPresentSupport.resize(queueFamilyCount, VK_FALSE);
			if (m_surface != VK_NULL_HANDLE)
			{
				for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
				{
					res = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, m_surface, &(queueFamilyPresentSupport[queueFamilyIndex]));
					checkForVkError(res);
				}
			}
		}
	}

	

	void RendererVk::selectPhysicalDevice(SelectedDeviceConfiguration& config)
	{
#ifdef PRINT_QUEUE_FAMILY_PROPERTIES
		printQueueFamilyCapabilities();
#endif
		
		bool needPresent = m_surface != VK_NULL_HANDLE;

		auto findQueueFamilyIndex = [this](uint32_t deviceIndex, const size_t* reservedQueues, VkFlags queueFlags, VkFlags flagsIgnoredInExactMatch, bool needsExactFlagsMatch, bool needsToSupportPresent) -> size_t
		{
			for (size_t queueFamilyIndex = 0; queueFamilyIndex < m_physicalDeviceInfos.queueFamilyProperties[deviceIndex].size(); ++queueFamilyIndex)
			{
				const VkQueueFamilyProperties& prop = m_physicalDeviceInfos.queueFamilyProperties[deviceIndex][queueFamilyIndex];
				VkQueueFlags flags = prop.queueFlags;

				if (needsToSupportPresent)
				{
					if (!m_physicalDeviceInfos.supportsPresent[deviceIndex][queueFamilyIndex])
					{
						continue;
					}
				}

				if (needsExactFlagsMatch)
				{
					VkFlags f = flags & (~flagsIgnoredInExactMatch);

					if (f == queueFlags)
					{
						if (reservedQueues[queueFamilyIndex] < prop.queueCount)
						{
							return queueFamilyIndex;
						}
					}
				}
				else
				{
					if ((flags & queueFlags) == queueFlags)
					{
						if (reservedQueues[queueFamilyIndex] < prop.queueCount)
						{
							return queueFamilyIndex;
						}
					}
				}
			}

			return ~0;
		};

		std::vector<size_t> reservedQueues;
		config.selectedPhysicalDeviceIndex = uint32_t(-1);
		for (uint32_t deviceIndex = 0; deviceIndex < m_physicalDeviceInfos.devices.size(); ++deviceIndex) 
		{
			//check for device features support
			if (!hasRequiredPhysicalDeviceFeatures(m_physicalDeviceInfos.features[deviceIndex]))
			{
				continue;
			}

			//check for RT support (implies support for descriptorIndexing and bufferDeviceAddress )
			reservedQueues.clear();
			reservedQueues.resize(m_physicalDeviceInfos.queueFamilyProperties[deviceIndex].size(), 0);

			//graphics, one for now
			size_t graphicsQueueFamilyIndex = findQueueFamilyIndex(deviceIndex, reservedQueues.data(), VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, VK_QUEUE_SPARSE_BINDING_BIT, false, needPresent);
			if (graphicsQueueFamilyIndex == ~0)
			{
				continue;
			}
			reservedQueues[graphicsQueueFamilyIndex] += 1;
			
			//compute
			size_t computeQueueFamilyIndex = findQueueFamilyIndex(deviceIndex, reservedQueues.data(), VK_QUEUE_COMPUTE_BIT, VK_QUEUE_SPARSE_BINDING_BIT, true, false);

			if (computeQueueFamilyIndex == ~0)
			{
				computeQueueFamilyIndex = findQueueFamilyIndex(deviceIndex, reservedQueues.data(), VK_QUEUE_COMPUTE_BIT, VK_QUEUE_SPARSE_BINDING_BIT, false, false);
				if (computeQueueFamilyIndex == ~0)
				{
					continue;
				}
			}
			reservedQueues[computeQueueFamilyIndex] += 1;

			//copy
			size_t copyQueueFamilyIndex = findQueueFamilyIndex(deviceIndex, reservedQueues.data(), VK_QUEUE_TRANSFER_BIT, VK_QUEUE_SPARSE_BINDING_BIT, true, false);

			if (copyQueueFamilyIndex == ~0)
			{
				copyQueueFamilyIndex = findQueueFamilyIndex(deviceIndex, reservedQueues.data(), VK_QUEUE_TRANSFER_BIT, VK_QUEUE_SPARSE_BINDING_BIT, false, false);
				if (copyQueueFamilyIndex == ~0)
				{
					continue;
				}
			}
			reservedQueues[copyQueueFamilyIndex] += 1;

			config.graphicsQueueFamilyIndex = (uint32_t)graphicsQueueFamilyIndex;
			config.computeQueueFamilyIndex = (uint32_t)computeQueueFamilyIndex;
			config.transferQueueFamilyIndex = (uint32_t)copyQueueFamilyIndex;

			config.selectedPhysicalDeviceIndex = (uint32_t)deviceIndex;
			break;
		}

#ifdef PRINT_QUEUE_FAMILY_PROPERTIES
		if (config.selectedPhysicalDeviceIndex != uint32_t(-1))
		{
			YAPT_LOG_DEBUG("Selected device %d", config.selectedPhysicalDeviceIndex);
		}
		else
		{
			YAPT_LOG_DEBUG("Failed to find suitable VkPhysicalDevice");
		}
#endif

	}

	void RendererVk::printQueueFamilyCapabilities()
	{
		for (size_t deviceIndex = 0; deviceIndex < m_physicalDeviceInfos.devices.size(); ++deviceIndex)
		{
			YAPT_LOG_DEBUG("Found device %s with %d queue families", m_physicalDeviceInfos.properties[deviceIndex].properties.deviceName, m_physicalDeviceInfos.queueFamilyProperties[deviceIndex].size());
			for (size_t queueFamilyIndex = 0; queueFamilyIndex < m_physicalDeviceInfos.queueFamilyProperties[deviceIndex].size(); ++queueFamilyIndex)
			{
				VkQueueFamilyProperties& prop = m_physicalDeviceInfos.queueFamilyProperties[deviceIndex][queueFamilyIndex];
				
				VkQueueFlags flags = prop.queueFlags;

				YAPT_LOG_DEBUG("Queue family index %d has %d queues and supports: %s%s%s%s",
					queueFamilyIndex, prop.queueCount,
					(flags & VK_QUEUE_GRAPHICS_BIT) ? "Graphics " : " ",
					(flags & VK_QUEUE_COMPUTE_BIT) ? "Compute " : " ",
					(flags & VK_QUEUE_TRANSFER_BIT) ? "Transfer " : " ",
					(flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Sparse binding " : " ");

	
			}

		}
	}

	void RendererVk::loadExtensionMethods()
	{
		LOAD_DEVICE_PROC(vkCreateRayTracingPipelinesKHR);
		LOAD_DEVICE_PROC(vkCmdTraceRaysKHR);
		LOAD_DEVICE_PROC(vkCmdTraceRaysIndirectKHR);
		LOAD_DEVICE_PROC(vkGetRayTracingShaderGroupStackSizeKHR);
		LOAD_DEVICE_PROC(vkCmdSetRayTracingPipelineStackSizeKHR);
	}

	bool RendererVk::createDevice(const SelectedDeviceConfiguration& config)
	{
		bool needsSwapChain = m_surface != VK_NULL_HANDLE;

		float queuePriorities[3] = { 1.f, 1.f, 1.f };

		std::vector<VkDeviceQueueCreateInfo> queueInfo;
		uint32_t graphicsQueueIndex = 0;
		uint32_t computeQueueIndex = 0;
		uint32_t copyQueueIndex = 0;


		{
			VkDeviceQueueCreateInfo info{};

			info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			info.queueFamilyIndex = config.graphicsQueueFamilyIndex;
			info.queueCount = 1;
			info.pQueuePriorities = queuePriorities;

			if (config.graphicsQueueFamilyIndex == config.computeQueueFamilyIndex)
			{
				computeQueueIndex = info.queueCount;
				++info.queueCount;
			}

			if (config.graphicsQueueFamilyIndex == config.transferQueueFamilyIndex)
			{
				copyQueueIndex = info.queueCount;
				++info.queueCount;
			}

			queueInfo.push_back(info);
		}

		
		if (config.graphicsQueueFamilyIndex != config.computeQueueFamilyIndex)
		{
			VkDeviceQueueCreateInfo info{};

			info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			info.queueFamilyIndex = config.computeQueueFamilyIndex;
			info.queueCount = 1;
			info.pQueuePriorities = queuePriorities;

			if (config.computeQueueFamilyIndex == config.transferQueueFamilyIndex)
			{
				copyQueueIndex = info.queueCount;
				++info.queueCount;
			}
			queueInfo.push_back(info);
		}

		if (config.graphicsQueueFamilyIndex != config.transferQueueFamilyIndex && config.computeQueueFamilyIndex != config.transferQueueFamilyIndex)
		{
			VkDeviceQueueCreateInfo info{};

			info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			info.queueFamilyIndex = config.transferQueueFamilyIndex;
			info.queueCount = 1;
			info.pQueuePriorities = queuePriorities;

			queueInfo.push_back(info);
		}


		std::vector<const char*> extensions
		{
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_RAY_QUERY_EXTENSION_NAME, //need this because DXC handles acceleration structures as needing rayquery instead of raytracing pipeline like before (and this extension is not present with emulation).

			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_KHR_SPIRV_1_4_EXTENSION_NAME,
			VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME

		};

		std::vector<const char*> layers
		{
			
		};

		if (needsSwapChain)
		{
			extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		VkDeviceCreateInfo devInfo = {};
		devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		devInfo.enabledExtensionCount = (uint32_t)extensions.size();
		devInfo.ppEnabledExtensionNames = extensions.data();
		devInfo.enabledLayerCount = (uint32_t)layers.size();
		devInfo.ppEnabledLayerNames = layers.data();
		devInfo.queueCreateInfoCount = (uint32_t)queueInfo.size();
		devInfo.pQueueCreateInfos = queueInfo.data();
		
		//TODO: enable required features
		devInfo.pNext = getRequiredPhysicalDeviceFeatures();


		VkResult res = vkCreateDevice(m_physicalDeviceInfos.devices[config.selectedPhysicalDeviceIndex], &devInfo, NULL, &m_device);
		bool success = res == VK_SUCCESS;
		if (success)
		{
			m_copyQueue.queueFamilyIndex = config.transferQueueFamilyIndex;
			m_graphicsQueue.queueFamilyIndex = config.graphicsQueueFamilyIndex;
			m_computeQueue.queueFamilyIndex = config.computeQueueFamilyIndex;

			m_copyQueue.queueIndex = copyQueueIndex;
			m_graphicsQueue.queueIndex = graphicsQueueIndex;
			m_computeQueue.queueIndex = computeQueueIndex;

			m_copyQueue.props = m_physicalDeviceInfos.queueFamilyProperties[config.selectedPhysicalDeviceIndex][m_copyQueue.queueFamilyIndex];
			m_graphicsQueue.props = m_physicalDeviceInfos.queueFamilyProperties[config.selectedPhysicalDeviceIndex][m_graphicsQueue.queueFamilyIndex];
			m_computeQueue.props = m_physicalDeviceInfos.queueFamilyProperties[config.selectedPhysicalDeviceIndex][m_computeQueue.queueFamilyIndex];

			vkGetDeviceQueue(m_device, m_copyQueue.queueFamilyIndex, m_copyQueue.queueIndex, &m_copyQueue.queue);
			vkGetDeviceQueue(m_device, m_graphicsQueue.queueFamilyIndex, m_graphicsQueue.queueIndex, &m_graphicsQueue.queue);
			vkGetDeviceQueue(m_device, m_computeQueue.queueFamilyIndex, m_computeQueue.queueIndex, &m_computeQueue.queue);
		}
		return checkForVkError(res);
	}

	const RendererVk::PhysicalDeviceFeatures* RendererVk::getRequiredPhysicalDeviceFeatures()
	{
		static PhysicalDeviceFeatures f;


		//vanilla vulkan features
		f.baseFeatures.features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
		f.baseFeatures.features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
		f.baseFeatures.features.shaderStorageImageArrayDynamicIndexing = VK_TRUE;
		f.baseFeatures.features.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
		f.baseFeatures.features.shaderStorageImageReadWithoutFormat = VK_TRUE;
		//descriptor indexing features

		f.descIndexingFeatures.shaderInputAttachmentArrayDynamicIndexing = VK_TRUE;
		f.descIndexingFeatures.shaderUniformTexelBufferArrayDynamicIndexing = VK_TRUE;
		f.descIndexingFeatures.shaderStorageTexelBufferArrayDynamicIndexing = VK_TRUE;
		f.descIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
		f.descIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		f.descIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
		f.descIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
		f.descIndexingFeatures.shaderInputAttachmentArrayNonUniformIndexing = VK_TRUE;
		f.descIndexingFeatures.shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE;
		f.descIndexingFeatures.shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE;
		//f.descIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
		f.descIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		f.descIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
		f.descIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
		f.descIndexingFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE;
		f.descIndexingFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE;
		f.descIndexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
		f.descIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
		f.descIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
		f.descIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
		
		//rt pipeline features
		f.rayTracePipelineFeatures.rayTracingPipeline = VK_TRUE;
		f.rayQueryFeatures.rayQuery = VK_TRUE;
		
		return &f;
	}

	bool RendererVk::hasRequiredPhysicalDeviceFeatures(const RendererVk::PhysicalDeviceFeatures& features)
	{
		//TODO: actually check all the required elements
		if (
			features.baseFeatures.features.shaderSampledImageArrayDynamicIndexing &&
			features.baseFeatures.features.shaderStorageBufferArrayDynamicIndexing &&
			features.baseFeatures.features.shaderStorageImageArrayDynamicIndexing &&
			features.baseFeatures.features.shaderUniformBufferArrayDynamicIndexing &&
			features.baseFeatures.features.shaderStorageImageReadWithoutFormat &&
			features.rayQueryFeatures.rayQuery &&
			features.rayTracePipelineFeatures.rayTracingPipeline
			)
		{
			return true;
		}
		return false;

	}
}