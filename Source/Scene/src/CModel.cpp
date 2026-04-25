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

	void CModel::setMaterials(Material** material, size_t materialCount)
	{
		m_materials.resize(materialCount);

		for (size_t i = 0; i < materialCount; ++i)
		{
			m_materials[i] = material[i];
		}

		setDirty(CModelDirtyFlag_Material);
	}
	void CModel::setMaterial(Material* material, size_t materialIndex)
	{
		if (m_materials.size() <= materialIndex)
		{
			m_materials.resize(materialIndex + 1, nullptr);
		}

		m_materials[materialIndex] = material;
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
			if (m_mesh.get() && m_materials.size() > 0)
			{
				if (!m_rendererObject)
				{
					m_rendererObject = m_renderer->createRenderObject();
					m_rendererObject->setMesh(m_mesh.get());
					m_rendererObject->setMaterials(m_materials.data(), m_materials.size());
					m_rendererObject->setTransform(m_transform.getMatrixWS());
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
						m_rendererObject->setMaterials(m_materials.data(), m_materials.size());
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
				m_rendererObject->setTransform(m_transform.getMatrixWS());
			}
		}


		m_dirtyMask = 0;
	}
}