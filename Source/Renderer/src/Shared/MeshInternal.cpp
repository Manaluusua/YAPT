#include <Renderer/Shared/MeshInternal.h>
#include <assert.h>
namespace YAPT
{
	MeshInternal::MeshInternal(size_t id, GfxApiHandle gfx, MeshLayoutID layoutID, const MeshLayoutInfo& data, size_t submeshCount, bool use16BitIndices)
		:m_id(id),
		m_gfx(gfx),
		m_meshLayoutId(id),
		m_indexFormat(use16BitIndices ? ResourceFormat::R16_UINT : ResourceFormat::R32_UINT),
		m_info(data)
	{
		m_vertexBuffers.resize(m_info.vertexBufferConfigurations.size());
		m_SubmeshRanges.resize(submeshCount);
	}

	void MeshInternal::setVertexBuffer(size_t bufferIndex, BufferImpl* buffer, size_t offsetInBytes)
	{
		assert(bufferIndex < m_vertexBuffers.size());
		BufferViewDesc viewDesc{};
		viewDesc.offsetInBytes = offsetInBytes;
		BufferViewHandle view = Gfx::getBufferView(m_gfx, buffer->getResourceHandle(), viewDesc);

		m_vertexBuffers[bufferIndex] = { buffer, view, offsetInBytes };
	}

	void MeshInternal::setIndexBuffer(BufferImpl* buffer, size_t offsetInBytes)
	{
		BufferViewDesc viewDesc{};
		viewDesc.offsetInBytes = offsetInBytes;
		viewDesc.nonStructuredFormat = m_indexFormat;
		BufferViewHandle view = Gfx::getBufferView(m_gfx, buffer->getResourceHandle(), viewDesc);

		m_indexBuffer = { buffer,view, offsetInBytes };
	}

	const MeshBufferBinding& MeshInternal::getVertexBuffer(size_t index) const
	{
		return m_vertexBuffers[index];
	}
	const MeshBufferBinding& MeshInternal::getIndexBuffer() const
	{
		return m_indexBuffer;
	}

	const MeshLayoutInfo& MeshInternal::getLayoutInfo() const
	{
		return m_info;
	}
}