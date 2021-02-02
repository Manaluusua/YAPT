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
			MESHSTATE_CREATED = YAPTBIT(1),
			MESHSTATE_MODIFIED = YAPTBIT(2),
			MESHSTATE_DESTROYED = YAPTBIT(4)
		};

		MeshProxy(MeshManager* mngr, const VertexBufferLayout* layouts, size_t numberOfVertexBufferLayouts, size_t numberOfVertices);

		virtual void setVertexBuffer(size_t bufferIndex, Buffer* buffer, size_t offsetInBytes) final;
		virtual void setIndexBuffer(Buffer* buffer, size_t offsetInBytes, size_t numberOfPrimitives) final;
		virtual ~MeshProxy() override;

		const MeshLayoutInfo& getMeshLayoutInfo() const { return m_data; }

		MeshInternal* getMeshInternal() { return m_mngr->getMeshInternal(_id); }

	protected:
		virtual void allReferencesReleased() final;

	private:
		MeshManager* m_mngr;

		std::vector<MeshBufferBinding> m_vertexBuffers;
		MeshBufferBinding m_indexBuffer;
		size_t m_primitiveCount;

		MeshLayoutInfo m_data;

		//Handled by MeshManager
		size_t _meshState;
		MeshIndex _id;
	};
}
