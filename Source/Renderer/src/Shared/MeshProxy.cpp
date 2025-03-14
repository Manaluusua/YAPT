#include <Renderer/Shared/MeshProxy.h>
#include <Renderer/Shared/MeshManager.h>
#include <Gfx/GfxBasicTypesUtility.h>


namespace YAPT
{
	MeshProxy::MeshProxy(MeshManager* mngr, const VertexBufferLayout* layouts, size_t numberOfVertexBufferLayouts, size_t numberOfVertices)
		:m_mngr(mngr),
		_id(InvalidMeshIndex)
	{
		resetRequirements(numberOfVertexBufferLayouts, true);
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
	}
	MeshProxy::~MeshProxy()
	{
		
	}

	void MeshProxy::setVertexBuffer(size_t bufferIndex, Buffer* buffer, size_t offsetInBytes)
	{
		assert(bufferIndex < m_vertexBuffers.size());
		m_vertexBuffers[bufferIndex] = { static_cast<BufferImpl*>(buffer),YAPT_NULL_HANDLE, offsetInBytes };
		vertexBufferBound(bufferIndex);
		m_mngr->meshChanged(this);
	}
	void MeshProxy::setIndexBuffer(Buffer* buffer, size_t offsetInBytes, size_t numberOfPrimitives)
	{
		m_indexBuffer = { static_cast<BufferImpl*>(buffer), YAPT_NULL_HANDLE, offsetInBytes };
		m_primitiveCount = numberOfPrimitives;
		indexBufferBound();
		m_mngr->meshChanged(this);
	}

	void MeshProxy::allReferencesReleased()
	{
		m_mngr->meshReleased(this);
	}

	void MeshProxy::resetRequirements(size_t numberOfBuffers, bool hasIndexbuffer)
	{
		assert(numberOfBuffers < 32);
		m_meshRequirementsNotSet = 0xFFFFFFFF >> (32 - numberOfBuffers);
		m_meshRequirementsNotSet |= 1 << 31;
	}
	void MeshProxy::vertexBufferBound(size_t index)
	{
		m_meshRequirementsNotSet &= ~(1 << index);
	}
	void MeshProxy::indexBufferBound()
	{
		m_meshRequirementsNotSet &= ~(1 << 31);
	}

}