#include <Gfx/Vk/DownloadHelperVk.h>
#include <Gfx/Vk/ResourceManagerVk.h>
#include <Gfx/Vk/ResourceHandlesVk.h>
#include <Gfx/Vk/YaptToVkConversions.h>


namespace YAPT
{
	//the readback targets are read through their mapped memory once the copy is done, which is only valid from GENERAL
	static const VkImageLayout HOST_READABLE_LAYOUT = VK_IMAGE_LAYOUT_GENERAL;

	DownloadHelperVk::DownloadHelperVk(ResourceManagerVk& resourceMngr, SubmissionThreadVk& submitThread, size_t numberOfPartitions, size_t numberOfDownloadsPerFrame)
		:CopyHelperVk(resourceMngr, submitThread)
	{
		initialize(numberOfPartitions, numberOfDownloadsPerFrame);
	}


	DownloadHelperVk::~DownloadHelperVk()
	{

	}

	void DownloadHelperVk::prepareNextDownloadBatch()
	{
		prepareNextFrame();
	}

	bool DownloadHelperVk::issueDownloads(ReadbackDefinitions& def, VkSemaphore* semaphoresToWait, size_t semaphoresToWaitCount, VkSemaphore& signaledSemaphore, FenceHandle fenceToSignal)
	{

		const VkImageLayout srcCopyLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		const VkImageLayout dstCopyLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		if (def.textureReadbackCount == 0 && def.bufferReadbackCount == 0) return false;

		//clear all previous barriers
		clearBarriers();

		//image barriers
		{
			//the sources are only borrowed for the copy and go back to whatever they were being used for, the readback
			//targets are left host readable
			for (uint32_t i = 0; i < def.textureReadbackCount; ++i)
			{
				const TextureReadbackDefinition& info = def.textureReadbackDefinitions[i];

				addBarriersForImageCopy(info.src, info.def.srcSubresource.mip, info.def.srcSubresource.arraySliceOffset, info.def.srcSubresource.arraySliceCount, srcCopyLayout, RESTORE_LAYOUT_BEFORE_COPY);
				addBarriersForImageCopy(info.dst, info.def.dstSubresource.mip, info.def.dstSubresource.arraySliceOffset, info.def.dstSubresource.arraySliceCount, dstCopyLayout, HOST_READABLE_LAYOUT);
			}
		}

		//buffer barriers
		{
			for (uint32_t i = 0; i < def.bufferReadbackCount; ++i)
			{
				const BufferReadbackDefinition& info = def.bufferReadbackDefinitions[i];

				addBarriersForBufferCopy(info.src->buffer, info.src->owningQueueFamily, info.def.srcOffset, info.def.size);
				addBarriersForBufferCopy(info.dst->buffer, info.dst->owningQueueFamily, info.def.dstOffset, info.def.size);
			}

		}

		VkCommandBuffer cmdBuff = beginCopyBatch(semaphoresToWait, semaphoresToWaitCount);

		// Buffer copies
		{

			//copy
			for (uint32_t i = 0; i < def.bufferReadbackCount; ++i)
			{
				const BufferReadbackDefinition& info = def.bufferReadbackDefinitions[i];

				VkBufferCopy copyDesc;
				copyDesc.srcOffset = info.def.srcOffset;
				copyDesc.dstOffset = info.def.dstOffset;
				copyDesc.size = info.def.size;

				vkCmdCopyBuffer(cmdBuff, info.src->buffer, info.dst->buffer, 1, &copyDesc);
			}

		}

		//image copies
		{

			//issue texture copies
			for (uint32_t i = 0; i < def.textureReadbackCount; ++i)
			{
				const TextureReadbackDefinition& info = def.textureReadbackDefinitions[i];

				VkImageCopy copyDesc{};
				copyDesc.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyDesc.srcSubresource.mipLevel = info.def.srcSubresource.mip;
				copyDesc.srcSubresource.baseArrayLayer = info.def.srcSubresource.arraySliceOffset;
				copyDesc.srcSubresource.layerCount = info.def.srcSubresource.arraySliceCount;
				copyDesc.srcOffset = { info.def.srcOffset.x, info.def.srcOffset.y, info.def.srcOffset.z };

				copyDesc.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyDesc.dstSubresource.mipLevel = info.def.dstSubresource.mip;
				copyDesc.dstSubresource.baseArrayLayer = info.def.dstSubresource.arraySliceOffset;
				copyDesc.dstSubresource.layerCount = info.def.dstSubresource.arraySliceCount;
				copyDesc.dstOffset = { info.def.dstOffset.x, info.def.dstOffset.y, info.def.dstOffset.z };

				copyDesc.extent = { info.def.extent.width, info.def.extent.height, info.def.extent.depth };

				vkCmdCopyImage(cmdBuff, info.src->image, srcCopyLayout, info.dst->image, dstCopyLayout, 1, &copyDesc);
			}
		}

		//the fence is what tells the readback manager the mapped memory is safe to read, so it is signaled right after
		//the copies, not after the resources have been handed back to their owners
		signaledSemaphore = endCopyBatch(cmdBuff, m_resMngr.getFence(fenceToSignal));

		return true;
	}
}
