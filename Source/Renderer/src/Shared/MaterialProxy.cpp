#include <Renderer/Shared/MaterialProxy.h>
#include <Renderer/Shared/TextureImpl.h>

namespace YAPT
{
	MaterialProxy::MaterialProxy(MaterialManager* mngr)
		:_id(InvalidMaterialId),
		m_mngr(mngr)
	{ 
	} 
	MaterialProxy::~MaterialProxy()
	{
	}
                                        
	void MaterialProxy::setFromMaterialPreset(MaterialPreset preset)
	{
		switch (preset)
		{
		case YAPT::MaterialPreset::BLANK:
			m_materialParams = MaterialParameters();
			break;
		case YAPT::MaterialPreset::METAL_GOLD:
			m_materialParams = MaterialParameters(glm::pow(vec3p(0.9451, 0.7294, 0.37255), vec3p(2.2f)), 0.0f, glm::pow(vec3p(1.0, 0.97255, 0.73333), vec3p(2.2f)), 1.0f,
				vec3p(0.f, 0.f, 0.f), 1.5f, vec3p(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 0.0f);
			break;
		case YAPT::MaterialPreset::METAL_SILVER:
			m_materialParams = MaterialParameters(glm::pow(vec3p(0.9607, 0.9490, 0.9176), vec3p(2.2f)), 0.0f, glm::pow(vec3p(1.0, 1.0, 1.0), vec3p(2.2f)), 1.0f,
				vec3p(0.f, 0.f, 0.f), 1.5f, vec3p(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 0.0f);
			break;
		case YAPT::MaterialPreset::METAL_ALUMINIUM:
			m_materialParams = MaterialParameters(glm::pow(vec3p(0.9137, 0.9137, 0.9137), vec3p(2.2f)), 0.0f, glm::pow(vec3p(0.9686, 0.98039, 0.9882), vec3p(2.2f)), 1.0f,
				vec3p(0.f, 0.f, 0.f), 1.5f, vec3p(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 0.0f);
			break;
		case YAPT::MaterialPreset::METAL_BRASS:
			m_materialParams = MaterialParameters(glm::pow(vec3p(0.903, 0.744, 0.473), vec3p(2.2f)), 0.0f, glm::pow(vec3p(0.975, 0.958, 0.918), vec3p(2.2f)), 1.0f,
				vec3p(0.f, 0.f, 0.f), 1.5f, vec3p(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 0.0f);
			break;
		case YAPT::MaterialPreset::METAL_COPPER:
			m_materialParams = MaterialParameters(glm::pow(vec3p(0.92549, 0.68627, 0.50196), vec3p(2.2)), 0.0f, glm::pow(vec3p(0.9960, 0.945098, 0.8196), vec3p(2.2)), 1.0f,
				vec3p(0.f, 0.f, 0.f), 1.5f, vec3p(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 0.0f);
			break;
		case YAPT::MaterialPreset::GLASS:
			m_materialParams = MaterialParameters(vec3p(1.f, 1.f, 1.f), 1.0f, vec3p(1.f, 1.f, 1.f), 0.0f,
				vec3p(0.0f, 0.0f, 0.0f), 1.5f, vec3p(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 0.0f);
			break;
		case YAPT::MaterialPreset::PLASTIC:
			m_materialParams = MaterialParameters(vec3p(1.f, 1.f, 1.f), 0.0f, vec3p(1.f, 1.f, 1.f), 0.0f,
				vec3p(0.0f, 0.0f, 0.0f), 1.5f, vec3p(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, 0.0f);
			break;
		default:
			break;
		}

		setDirty();
	}
  
	void MaterialProxy::allReferencesReleased()
	{
		m_mngr->materialReleased(this);
	}

	void MaterialProxy::setTransparency(float transparency)
	{
		m_materialParams.transparency = transparency;
		setDirty();
	}
	float MaterialProxy::getTransparency() const
	{
		return m_materialParams.transparency;
	}
    
	void MaterialProxy::setMetalness(float metalness)
	{
		m_materialParams.metalness = metalness;
		setDirty();
	}
	float MaterialProxy::getMetalness() const
	{
		return m_materialParams.metalness;
	}

	void MaterialProxy::setAlbedo(const vec3p& v)
	{
		m_materialParams.albedo = v;
		setDirty();
	}
	const vec3p& MaterialProxy::getAlbedo() const
	{
		return m_materialParams.albedo;
	}

	void MaterialProxy::setSpecularTint(const vec3p& v)
	{
		m_materialParams.specular = v;
		setDirty();
	}
	const vec3p& MaterialProxy::getSpecularTint() const
	{
		return m_materialParams.specular;
	}

	void MaterialProxy::setAbsorption(const vec3p& v)
	{
		m_materialParams.absorption = v;
		setDirty();
	}
	const vec3p& MaterialProxy::getAbsorption() const
	{
		return m_materialParams.absorption;
	}

	void MaterialProxy::setEmission(const vec3p& v)
	{
		m_materialParams.emissive = v;
		setDirty();
	}
	const vec3p& MaterialProxy::getEmission() const
	{
		return m_materialParams.emissive;
	}

	void MaterialProxy::setEmissionFocus(float focus)
	{
		m_materialParams.emissionFocus = focus;
		setDirty();
	}
	float MaterialProxy::getEmissionFocus() const
	{
		return m_materialParams.emissionFocus;
	}

	void MaterialProxy::setDielectricIOR(float ior)
	{
		m_materialParams.dielectricIOR = ior;
		setDirty();
	}
	float MaterialProxy::getDielectricIOR() const
	{
		return m_materialParams.dielectricIOR;
	}

	void MaterialProxy::setRoughness(float roughness)
	{
		m_materialParams.roughness = roughness;
		setDirty();
	}
	float MaterialProxy::getRoughness() const
	{
		return m_materialParams.roughness;
	}

	void MaterialProxy::setAnisotropy(float anisotropy)
	{
		m_materialParams.anisotropy = anisotropy;
		setDirty();
	}
	float MaterialProxy::getAnisotropy() const
	{
		return m_materialParams.anisotropy;
	}

	void MaterialProxy::setAnisotropyRotation(float rot)
	{
		m_materialParams.anisotropyRotation = rot;
		setDirty();
	}
	float MaterialProxy::getAnisotropyRotation() const
	{
		return m_materialParams.anisotropyRotation;
	}

	void MaterialProxy::setTwoSided(bool val)
	{
		if (val)
		{
			m_materialParams.materialMask |= MaterialMask_TwoSided;
		}
		else
		{
			m_materialParams.materialMask &= ~MaterialMask_TwoSided;
		}
		setDirty();
		
	}
	bool MaterialProxy::isTwoSided() const
	{
		return (m_materialParams.materialMask & MaterialMask_TwoSided) != 0;
	}


	void MaterialProxy::setSpecularAmount(float val)
	{
		m_materialParams.specularAmount = val;
		setDirty();
	}
	float MaterialProxy::getSpecularAmount() const
	{
		return m_materialParams.specularAmount;
	}

	void MaterialProxy::setClearCoatAmount(float val)
	{
		m_materialParams.clearCoatAmount = val;
		setDirty();
	}
	float MaterialProxy::getClearCoatAmount() const
	{
		return m_materialParams.clearCoatAmount;
	}

	void MaterialProxy::setClearCoatRoughness(float val)
	{
		m_materialParams.clearCoatRoughness = val;
		setDirty();
		
	}
	float MaterialProxy::getClearCoatRoughness() const
	{
		return m_materialParams.clearCoatRoughness;
	}

	void MaterialProxy::setClearCoatIOR(float val)
	{
		m_materialParams.clearCoatIOR = val;
		setDirty();
	}
	float MaterialProxy::getClearCoatIOR() const
	{
		return m_materialParams.clearCoatIOR;
	}
	void MaterialProxy::setSheenRoughness(float val)
	{
		m_materialParams.sheenRoughness = val;
		setDirty();
	}
	float MaterialProxy::getSheenRoughness() const
	{
		return m_materialParams.sheenRoughness;
	}

	void MaterialProxy::setSheenTint(const vec3p& v)
	{
		m_materialParams.sheenTint = v;
		setDirty();
	}
	const vec3p& MaterialProxy::getSheenTint() const
	{
		return m_materialParams.sheenTint;
	}

	void MaterialProxy::setSheenAmount(float val)
	{
		m_materialParams.sheenAmount = val;
		setDirty();
	}
	float MaterialProxy::getSheenAmount() const
	{
		return m_materialParams.sheenAmount; 
	}


	void MaterialProxy::setThinFilmThicknessNM(float val)
	{
		m_materialParams.thinFilmThicknessNM = val;
		setDirty();
	}
	float MaterialProxy::getThinFilmThicknessNM() const
	{
		return m_materialParams.thinFilmThicknessNM;
	}

	void MaterialProxy::setCauchysCoefficients(const vec2p& val)
	{
		m_materialParams.cauchysCoeffs = val;
		setDirty();
	}
	const vec2p& MaterialProxy::getCauchysCoefficients() const
	{
		return m_materialParams.cauchysCoeffs;
	}

	void MaterialProxy::setEnableDispersion(bool val)
	{
		if (val)
		{
			m_materialParams.materialMask |= MaterialMask_Dispersion;
		}
		else
		{
			m_materialParams.materialMask &= ~MaterialMask_Dispersion;
		}
		setDirty();
	}
	bool MaterialProxy::getEnableDispersion() const
	{
		return (m_materialParams.materialMask & MaterialMask_Dispersion) != 0;
	}

	void MaterialProxy::setAlbedoTexture(const TextureParameter& tex)
	{
		assignMaterialTextureParam(tex, m_materialParams.albedoTex);
		 
	}
	void MaterialProxy::setNormalTexture(const TextureParameter& tex)
	{
		assignMaterialTextureParam(tex, m_materialParams.normalTex);
	}
	void MaterialProxy::setORMTexture(const TextureParameter& tex)
	{
		assignMaterialTextureParam(tex, m_materialParams.ormTex);
	}
	void MaterialProxy::setEmissiveTexture(const TextureParameter& tex)
	{
		assignMaterialTextureParam(tex, m_materialParams.emissiveTex);
	}

	void MaterialProxy::assignMaterialTextureParam(const TextureParameter& source, MaterialParameterTexture& dest)
	{
		dest.textureIndex = getTextureViewIndex(source.texture.get());
		dest.setScale(source.scale.x, source.scale.y);
	}

	uint32_t MaterialProxy::getTextureViewIndex(Texture* tex)
	{
		return static_cast<TextureImpl*>(tex)->getBindlessResourceArrayIndex();
	}

	void MaterialProxy::setDirty()
	{
		m_mngr->materialChanged(this);
	}

}