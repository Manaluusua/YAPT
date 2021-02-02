#include <Renderer/Shared/MaterialInternal.h>

namespace YAPT
{
	MaterialInternal::MaterialInternal(size_t id)
		:m_id(id)
	{

	}
	MaterialInternal::~MaterialInternal()
	{

	}

	void MaterialInternal::setMaterialParams(const MaterialParameters& params)
	{
		m_materialParams = params;
	}
	const MaterialParameters& MaterialInternal::getMaterialParams() const
	{
		return m_materialParams;
	}
}