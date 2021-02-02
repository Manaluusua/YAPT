#include <Scene/Impl/CModel.h>
#include <Scene/Impl/CScene.h>
#include <Renderer/Renderer.h>

namespace YAPT
{
	

	CModel::CModel(Renderer* renderer, CScene* scene)
		:m_scene(scene),
		m_renderer(renderer),
		m_rendererObject(nullptr),
		m_transform(CModel::transformChanged, this),
		m_dirtyMask(0)
	{

	}
	CModel::~CModel()
	{

	}

	void CModel::setMesh(Mesh* mesh)
	{
		m_mesh = mesh;
		setDirty(CModelDirtyFlag_Mesh);
	}
	void CModel::setMaterial(Material* material)
	{
		m_material = material;
		setDirty(CModelDirtyFlag_Material);
	}

	void CModel::setDirty(uint32_t reason)
	{
		if (m_dirtyMask == 0)
		{
			m_scene->needsRefresh(this);
		}
		m_dirtyMask |= reason;
	}

	void CModel::transformChanged(void* ptr)
	{
		static_cast<CModel*>(ptr)->setDirty(CModelDirtyFlag_Transform);
	}

	void CModel::refresh()
	{
		if ((CModelDirtyFlag_Mesh | CModelDirtyFlag_Material) & m_dirtyMask)
		{
			if (m_mesh.get() && m_material.get())
			{
				if (!m_rendererObject)
				{
					m_rendererObject = m_renderer->createRenderObject();
					m_rendererObject->setMesh(m_mesh.get());
					m_rendererObject->setMaterial(m_material.get());
					m_rendererObject->setTransform(m_transform.getMatrix());
					m_rendererObject->Release();
				}
				else
				{
					if (CModelDirtyFlag_Mesh & m_dirtyMask)
					{
						m_rendererObject->setMesh(m_mesh.get());
					}
					if (CModelDirtyFlag_Material & m_dirtyMask)
					{
						m_rendererObject->setMaterial(m_material.get());
					}
				}
			}
			else
			{
				m_rendererObject = nullptr;
			}
		}

		if (CModelDirtyFlag_Transform & m_dirtyMask)
		{
			if (m_rendererObject)
			{
				m_rendererObject->setTransform(m_transform.getMatrix());
			}
		}


		m_dirtyMask = 0;
	}
}