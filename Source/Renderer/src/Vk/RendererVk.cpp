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
		VkSemaphore s = VK_NULL_HANDLE;
		if (m_syncUtility.getFrameCount() >= m_gfxConfig.pipelineLength)
		{
			size_t frameToSync = m_syncUtility.getFrameCount() - m_gfxConfig.pipelineLength;
			s = m_syncUtility.getSemaphoreForFrameNumber(frameToSync);
		}
		m_resourceManager->prepare(s);

	}
	void RendererVk::renderBegin()
	{
		m_resourceManager->flushPreFrameUploads();
	}
	void RendererVk::executeBegin()
	{
		m_resourceManager->flushFrameUploads();
		m_syncUtility.nextFrame();
		m_resourceManager->resetCommandBufferPools(m_syncUtility.getFrameIndex());
	}
	void RendererVk::executeEnd()
	{
		assert(!"TODO");
	}

	void RendererVk::waitForAllFramesDone()
	{
		assert(!"TODO");
	}

	bool RendererVk::initialize()
	{
		bool success = false;
		if (createInstance())
		{
			if (m_gfxConfig.renderSurfaceHandle != YAPT_NULL_HANDLE)
			{
				m_surface = createVkSurface(m_instance, m_gfxConfig.renderSurfaceHandle);
			}

			queryPhysicalDeviceInfos();

			if (m_surface != VK_NULL_HANDLE)
			{
				queryPresentInfosForSurface(m_surface);
			}

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
			std::vector<VkQueue> queues[SubmissionThreadVk::COMMANDQUEUETYPE_COUNT];
			queues[SubmissionThreadVk::COMMANDQUEUETYPE_GRAPHICS].push_back(m_graphicsQueue.queue);
			queues[SubmissionThreadVk::COMMANDQUEUETYPE_COMPUTE].push_back(m_computeQueue.queue);
			queues[SubmissionThreadVk::COMMANDQUEUETYPE_COPY].push_back(m_copyQueue.queue);

			for (size_t i = 0; i < SubmissionThreadVk::COMMANDQUEUETYPE_COUNT; ++i)
			{
				submitThreadConfig.numberOfQueues[i] = queues[i].size();
				submitThreadConfig.queues[i] = queues[i].data();
			}
			//TODO: fill
			m_submissionThread.initialize(submitThreadConfig);

			m_resourceManager = new ResourceManagerVk(m_physicalDeviceInfos.devices[m_selectedPhysicalDeviceIndex], m_submissionThread, m_copyQueue,  m_device, m_gfxConfig.pipelineLength);
			m_syncUtility.initialize(m_device, m_gfxConfig.pipelineLength, true, true);
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
		

		res = vkEnumeratePhysicalDevices(m_instance, &deviceCount, m_physicalDeviceInfos.devices.data());
		checkForVkError(res);


		for (uint32_t deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {

			uint32_t queueFamilyCount;

			const VkPhysicalDevice& physicalDevice = m_physicalDeviceInfos.devices[deviceIndex];
			vkGetPhysicalDeviceProperties(physicalDevice, &m_physicalDeviceInfos.properties[deviceIndex]);
			vkGetPhysicalDeviceFeatures(physicalDevice, &m_physicalDeviceInfos.features[deviceIndex]);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);


			std::vector<VkQueueFamilyProperties>& queueFamilyProps = m_physicalDeviceInfos.queueFamilyProperties[deviceIndex];
			queueFamilyProps.resize(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProps.data());
		}
	}

	void RendererVk::queryPresentInfosForSurface(VkSurfaceKHR surface)
	{
		uint32_t deviceCount = (uint32_t)m_physicalDeviceInfos.devices.size();
		m_presentInfos.presentSupport.resize(deviceCount);
		m_presentInfos.supportedSurfaceFormats.resize(deviceCount);
		m_presentInfos.surfaceCapabilities.resize(deviceCount);

		for (uint32_t deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {

			uint32_t queueFamilyCount = (uint32_t)m_physicalDeviceInfos.queueFamilyProperties[deviceIndex].size();
			const VkPhysicalDevice& physicalDevice = m_physicalDeviceInfos.devices[deviceIndex];

			std::vector<VkBool32>& queueFamilyPresentSupport = m_presentInfos.presentSupport[deviceIndex];
			std::vector<VkSurfaceFormatKHR>& supportedFormats = m_presentInfos.supportedSurfaceFormats[deviceIndex];

			queueFamilyPresentSupport.resize(queueFamilyCount);

			VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &m_presentInfos.surfaceCapabilities[deviceIndex]);
			checkForVkError(res);
			for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
			{
				res = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, &(queueFamilyPresentSupport[queueFamilyIndex]));
				checkForVkError(res);
			}

			uint32_t supportedFormatCount = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &supportedFormatCount, NULL);
			supportedFormats.resize(supportedFormatCount);

			res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &supportedFormatCount, supportedFormats.data());
			checkForVkError(res);
			
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
					if (!m_presentInfos.presentSupport[deviceIndex][queueFamilyIndex])
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
			YAPT_LOG_DEBUG("Found device %s with %d queue families", m_physicalDeviceInfos.properties[deviceIndex].deviceName, m_physicalDeviceInfos.queueFamilyProperties[deviceIndex].size());
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

	const VkPhysicalDeviceFeatures& RendererVk::getRequiredPhysicalDeviceFeatures()
	{
		static VkPhysicalDeviceFeatures features{};
		features.shaderSampledImageArrayDynamicIndexing = true;
		features.shaderStorageBufferArrayDynamicIndexing = true;
		features.shaderStorageImageArrayDynamicIndexing = true;
		features.shaderUniformBufferArrayDynamicIndexing = true;
		
		return features;
	}

	bool RendererVk::hasRequiredPhysicalDeviceFeatures(const VkPhysicalDeviceFeatures& features)
	{
		if (
			features.shaderSampledImageArrayDynamicIndexing &&
			features.shaderStorageBufferArrayDynamicIndexing &&
			features.shaderStorageImageArrayDynamicIndexing &&
			features.shaderUniformBufferArrayDynamicIndexing
			)
		{
			return true;
		}
		return false;

	}
}