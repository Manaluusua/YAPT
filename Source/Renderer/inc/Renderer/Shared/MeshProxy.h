#pragma once

#include <Renderer/Mesh.h>
#include <Renderer/Shared/MeshManager.h>
#include <Renderer/Buffer.h>
#include <Common/RCObjectPtr.h>

namespace YAPT
{

	class MeshProxy : public Mesh
	{
		friend class MeshManager;
	public:

		enum MeshState
		{
			MESHSTATE_NOCHANGES = 0,
			MESHSTATE_INCOMPLETE = YAPTBIT(1),
			MESHSTATE_CREATED = YAPTBIT(2),
			MESHSTATE_MODIFIED = YAPTBIT(3),
			MESHSTATE_DESTROYED = YAPTBIT(4)
		};

		MeshProxy(MeshManager* mngr, const VertexBufferLayout* layouts, size_t numberOfVertexBufferLayouts, size_t numberOfVertices, size_t submeshCount, bool use16BitIndices);

		virtual void setSubmesh(size_t submeshIndex, const SubmeshDefinition& range) final;
		virtual size_t getSubmeshCount() const final { return m_submeshes.size(); }
		virtual void setVertexBuffer(size_t bufferIndex, Buffer* buffer, size_t offsetInBytes) final;
		virtual void setIndexBuffer(Buffer* buffer, size_t offsetInBytes) final;
		virtual ~MeshProxy() override;

		const MeshLayoutInfo& getMeshLayoutInfo() const { return m_data; }

		MeshInternal* getMeshInternal() { return m_mngr->getMeshInternal(_id); }

		bool has16BitIndices() const { return m_use16BitIndices; }

		bool hasAllRequiredBuffers() const { return m_vertexBuffersNotSet == 0 && m_submeshRangesNotSet == 0 && !m_indexBufferNotSet; }

	protected:
		virtual void allReferencesReleased() final;

	private:

		void resetRequirements(size_t numberOfBuffers, size_t numberOfSubMeshes, bool hasIndexbuffer);
		void vertexBufferBound(size_t index);
		void submeshRangeSet(size_t index);
		void indexBufferBound();


		MeshManager* m_mngr;

		std::vector<MeshBufferBinding> m_vertexBuffers;
		MeshBufferBinding m_indexBuffer;
		std::vector<SubmeshDefinition> m_submeshes;

		MeshLayoutInfo m_data;
		uint32_t m_vertexBuffersNotSet;
		uint32_t m_submeshRangesNotSet;
		bool m_indexBufferNotSet;
		bool m_use16BitIndices;

		//Handled by MeshManager
		size_t _meshState;
		MeshIndex _id;
	};
}
