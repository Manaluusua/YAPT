#include <Renderer/Shared/MaterialInternal.h>

#define ROUGHNESS_MIN 0.001f

namespace YAPT
{
	MaterialInternal::MaterialInternal(MaterialIndex id)
		:m_id(id)
	{

	}
	MaterialInternal::~MaterialInternal()
	{

	}

	void MaterialInternal::setMaterialParams(const MaterialParameters& params)
	{
		m_materialParams = params;
		m_materialParams.roughness = std::max(ROUGHNESS_MIN, m_materialParams.roughness); //TODO: this should be done somewhere else (?)
	}
	const MaterialParameters& MaterialInternal::getMaterialParams() const
	{
		return m_materialParams;
	}
}