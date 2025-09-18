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
		validateMaterial();
	}
	const MaterialParameters& MaterialInternal::getMaterialParams() const
	{
		return m_materialParams;
	}

	void MaterialInternal::validateMaterial()
	{
		m_materialParams.roughness = std::max(ROUGHNESS_MIN, m_materialParams.roughness); 

		//coating must sum to 1
		float coatingSum = m_materialParams.clearCoatAmount + m_materialParams.sheenAmount;
		if (coatingSum > 1)
		{
			float coatingSumInv = 1.f / coatingSum;
			m_materialParams.clearCoatAmount *= coatingSumInv;
			m_materialParams.sheenAmount *= coatingSumInv;
		}
	}
}