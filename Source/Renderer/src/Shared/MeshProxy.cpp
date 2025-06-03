#include <Renderer/Shared/MeshProxy.h>
#include <Renderer/Shared/MeshManager.h>
#include <Gfx/GfxBasicTypesUtility.h>


namespace YAPT
{
	MeshProxy::MeshProxy(MeshManager* mngr, const VertexBufferLayout* layouts, size_t numberOfVertexBufferLayouts, size_t numberOfVertices, size_t submeshCount, bool use16BitIndices)
		:m_mngr(mngr),
		m_use16BitIndices(use16BitIndices),
		_id(InvalidMeshIndex)
	{
		resetRequirements(numberOfVertexBufferLayouts, submeshCount, true);
		m_data.numberOfVertices = numberOfVertices;

		AttributeSemantic positionSemantic(AttributeSemanticName::POSITION);
		AttributeSemantic tangentSemantic(AttributeSemanticName::TANGENT);
		AttributeSemantic normalSemantic(AttributeSemanticName::NORMAL);
		AttributeSemantic uvSemantic(AttributeSemanticName::TEXCOORD);

		m_data.vertexBufferConfigurations.resize(numberOfVertexBufferLayouts);
		for (uint32_t i = 0; i < numberOfVertexBufferLayouts; ++i)
		{
			uint32_t totalSizeOfAttributes = 0;
			const VertexBufferLayout& layout = layouts[i];

			m_data.vertexBufferConfigurations[i].attributes.reserve(layout.attributeCount);
			m_data.vertexBufferConfigurations[i].offsetFromVertexStart.reserve(layout.attributeCount);
			for (uint32_t attInd = 0; attInd < layout.attributeCount; ++attInd)
			{
				m_data.vertexBufferConfigurations[i].attributes.push_back(layout.attributes[attInd]);
				m_data.vertexBufferConfigurations[i].offsetFromVertexStart.push_back(totalSizeOfAttributes);
				if (layout.attributes[attInd].semantic == positionSemantic)
				{
					m_data.position0.bufferIndex = i;
					m_data.position0.attributeIndex = attInd;
				}
				else if (layout.attributes[attInd].semantic == tangentSemantic)
				{
					m_data.tangent0.bufferIndex = i;
					m_data.tangent0.attributeIndex = attInd;
				}
				else if (layout.attributes[attInd].semantic == normalSemantic)
				{
					m_data.normal0.bufferIndex = i;
					m_data.normal0.attributeIndex = attInd;
				}
				else if (layout.attributes[attInd].semantic == uvSemantic)
				{
					m_data.uv0.bufferIndex = i;
					m_data.uv0.attributeIndex = attInd;
				}

				totalSizeOfAttributes += getFormatSizeInBytes(layout.attributes[attInd].format);
			}

			if (layout.vertexStrideInBytes == VERTEX_STRIDE_TIGHTLY_PACKED)
			{
				m_data.vertexBufferConfigurations[i].stride = totalSizeOfAttributes;
			}
			else
			{
				m_data.vertexBufferConfigurations[i].stride = layout.vertexStrideInBytes;
			}
		}

		m_vertexBuffers.resize(numberOfVertexBufferLayouts);
		m_submeshes.resize(submeshCount);
	}
	MeshProxy::~MeshProxy()
	{
		
	}

	void MeshProxy::setSubmesh(size_t submeshIndex, const SubmeshDefinition& def)
	{
		assert(submeshIndex < m_submeshes.size());
		m_submeshes[submeshIndex] = def;
		submeshRangeSet(submeshIndex);
		m_mngr->meshChanged(this);
	}

	void MeshProxy::setVertexBuffer(size_t bufferIndex, Buffer* buffer, size_t offsetInBytes)
	{
		assert(bufferIndex < m_vertexBuffers.size());
		m_vertexBuffers[bufferIndex] = { static_cast<BufferImpl*>(buffer),YAPT_NULL_HANDLE, offsetInBytes };
		vertexBufferBound(bufferIndex);
		m_mngr->meshChanged(this);
	}
	void MeshProxy::setIndexBuffer(Buffer* buffer, size_t offsetInBytes)
	{
		m_indexBuffer = { static_cast<BufferImpl*>(buffer), YAPT_NULL_HANDLE, offsetInBytes };
		indexBufferBound();
		m_mngr->meshChanged(this);
	}

	void MeshProxy::allReferencesReleased()
	{
		m_mngr->meshReleased(this);
	}

	void MeshProxy::resetRequirements(size_t numberOfBuffers, size_t numberOfSubMeshes, bool hasIndexbuffer)
	{
		assert(numberOfBuffers < 33);
		assert(numberOfSubMeshes < 33);

		m_vertexBuffersNotSet = 0xFFFFFFFF >> (32 - numberOfBuffers);
		m_submeshRangesNotSet = 0xFFFFFFFF >> (32 - numberOfSubMeshes);
		m_indexBufferNotSet = true;
	}


	void MeshProxy::vertexBufferBound(size_t index)
	{
		m_vertexBuffersNotSet &= ~(1 << index);
	}

	void MeshProxy::submeshRangeSet(size_t index)
	{
		m_submeshRangesNotSet &= ~(1 << index);
	}

	void MeshProxy::indexBufferBound()
	{
		m_indexBufferNotSet = false;
	}

}