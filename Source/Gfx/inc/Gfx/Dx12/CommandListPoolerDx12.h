#ifndef YAPT_DX12_COMMANDLISTPOOLER_H
#define YAPT_DX12_COMMANDLISTPOOLER_H

#include <Gfx/Dx12/Dx12CommonIncludes.h>
#include <Gfx/GfxTypes.h>
#include <vector>
namespace YAPT
{
	class ResourceManagerDx12;
	class CommandListPoolerDx12
	{
	public:

		CommandListPoolerDx12(ID3D12Device5& device, size_t pipelineLength);
		~CommandListPoolerDx12();

		bool initialize(D3D12_COMMAND_LIST_TYPE type, size_t numberOfListsPerFrame);
		void reset();

		size_t getNumberOfCommandListsPerFrame() const;
		RCPtr<ID3D12CommandAllocator>& getAllocator(size_t index);
		RCPtr<GraphicsCommandListDx12>& getCommandList(size_t index);
		CommandBufferHandleDx12* getHandle(size_t index) { return &m_handles[index]; }

		void incrementFrame();

	private:

		

		ID3D12Device5& m_device;
		size_t m_pipelineLength;
		size_t m_currentFrame;

		size_t m_numberOfListsPerFrame;

		std::vector<RCPtr<GraphicsCommandListDx12>> m_commandList;
		std::vector<RCPtr<ID3D12CommandAllocator>> m_allocators;
		std::vector<CommandBufferHandleDx12> m_handles;
	};
}


#endif