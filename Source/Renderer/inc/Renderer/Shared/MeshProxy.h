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

		MeshProxy(MeshManager* mngr, const VertexBufferLayout* layouts, size_t numberOfVertexBufferLayouts, size_t numberOfVertices);

		virtual void setVertexBuffer(size_t bufferIndex, Buffer* buffer, size_t offsetInBytes) final;
		virtual void setIndexBuffer(Buffer* buffer, size_t offsetInBytes, size_t numberOfPrimitives) final;
		virtual ~MeshProxy() override;

		const MeshLayoutInfo& getMeshLayoutInfo() const { return m_data; }

		MeshInternal* getMeshInternal() { return m_mngr->getMeshInternal(_id); }

		bool hasAllRequiredBuffers() const { return m_meshRequirementsNotSet == 0; }

	protected:
		virtual void allReferencesReleased() final;

	private:

		void resetRequirements(size_t numberOfBuffers, bool hasIndexbuffer);
		void vertexBufferBound(size_t index);
		void indexBufferBound();


		MeshManager* m_mngr;

		std::vector<MeshBufferBinding> m_vertexBuffers;
		MeshBufferBinding m_indexBuffer;
		size_t m_primitiveCount;

		MeshLayoutInfo m_data;
		uint32_t m_meshRequirementsNotSet;

		//Handled by MeshManager
		size_t _meshState;
		MeshIndex _id;
	};
}
