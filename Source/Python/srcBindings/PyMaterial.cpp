#include <PyMaterial.h>
#include <PyRenderer.h>
#include <PyTexture.h>
#include <Renderer/Renderer.h>
#include <Renderer/Material.h>
#include <assert.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS(PyMaterial);

	PyMaterial::PyMaterial(Renderer* rend, const char* name)
        :m_name(name)
	{
		m_material = rend->createMaterial();
	}
	PyMaterial::~PyMaterial()
	{
        m_material = nullptr;
	}

    const char* PyMaterial::getName() const
    {
        return m_name.c_str();
    }


    void PyMaterial::setFromMaterialPreset(MaterialPreset preset)
    {
        m_material->setFromMaterialPreset(preset);
    }

    void PyMaterial::setTransparency(float transparency)
    {
        m_material->setTransparency(transparency);
    }

    float PyMaterial::getTransparency() const
    {
        return m_material->getTransparency();
    }

    void PyMaterial::setMetalness(float metalness)
    {
        m_material->setMetalness(metalness);
    }

    float PyMaterial::getMetalness() const
    {
        return m_material->getMetalness();
    }

    void PyMaterial::setAlbedo(const vec3p& v)
    {
        m_material->setAlbedo(v);
    }

    const vec3p& PyMaterial::getAlbedo() const
    {
        return m_material->getAlbedo();
    }

    void PyMaterial::setSpecularTint(const vec3p& v)
    {
        m_material->setSpecularTint(v);
    }

    const vec3p& PyMaterial::getSpecularTint() const
    {
        return m_material->getSpecularTint();
    }

    void PyMaterial::setAbsorption(const vec3p& v)
    {
        m_material->setAbsorption(v);
    }

    const vec3p& PyMaterial::getAbsorption() const
    {
        return m_material->getAbsorption();
    }

    void PyMaterial::setEmission(const vec3p& v)
    {
        m_material->setEmission(v);
    }

    const vec3p& PyMaterial::getEmission() const
    {
        return m_material->getEmission();
    }

    void PyMaterial::setEmissionFocus(float focus)
    {
        m_material->setEmissionFocus(focus);
    }

    float PyMaterial::getEmissionFocus() const
    {
        return m_material->getEmissionFocus();
    }

    void PyMaterial::setDielectricIOR(float ior)
    {
        m_material->setDielectricIOR(ior);
    }

    float PyMaterial::getDielectricIOR() const
    {
        return m_material->getDielectricIOR();
    }

    void PyMaterial::setRoughness(float roughness)
    {
        m_material->setRoughness(roughness);
    }

    float PyMaterial::getRoughness() const
    {
        return m_material->getRoughness();
    }

    void PyMaterial::setAnisotropy(float anisotropy)
    {
        m_material->setAnisotropy(anisotropy);
    }

    float PyMaterial::getAnisotropy() const
    {
        return m_material->getAnisotropy();
    }

    void PyMaterial::setAnisotropyRotation(float rot)
    {
        m_material->setAnisotropyRotation(rot);
    }

    float PyMaterial::getAnisotropyRotation() const
    {
        return m_material->getAnisotropyRotation();
    }

    void PyMaterial::setTwoSided(bool val)
    {
        m_material->setTwoSided(val);
    }

    bool PyMaterial::getTwoSided() const
    {
        return m_material->isTwoSided();
    }

    void PyMaterial::setSpecularAmount(float val)
    {
        m_material->setSpecularAmount(val);
    }

    float PyMaterial::getSpecularAmount() const
    {
        return m_material->getSpecularAmount();
    }

    void PyMaterial::setClearCoatAmount(float val)
    {
        m_material->setClearCoatAmount(val);
    }

    float PyMaterial::getClearCoatAmount() const
    {
        return m_material->getClearCoatAmount();
    }

    void PyMaterial::setClearCoatRoughness(float val)
    {
        m_material->setClearCoatRoughness(val);
    }

    float PyMaterial::getClearCoatRoughness() const
    {
        return m_material->getClearCoatRoughness();
    }

    void PyMaterial::setClearCoatIOR(float val)
    {
        m_material->setClearCoatIOR(val);
    }

    float PyMaterial::getClearCoatIOR() const
    {
        return m_material->getClearCoatIOR();
    }

    void PyMaterial::setSheenRoughness(float val)
    {
        m_material->setSheenRoughness(val);
    }

    float PyMaterial::getSheenRoughness() const
    {
        return m_material->getSheenRoughness();
    }

    void PyMaterial::setSheenTint(const vec3p& v)
    {
        m_material->setSheenTint(v);
    }

    const vec3p& PyMaterial::getSheenTint() const
    {
        return m_material->getSheenTint();
    }

    void PyMaterial::setSheenAmount(float val)
    {
        m_material->setSheenAmount(val);
    }

    float PyMaterial::getSheenAmount() const
    {
        return m_material->getSheenAmount();
    }

    void PyMaterial::setThinFilmThicknessNM(float val)
    {
        m_material->setThinFilmThicknessNM(val);
    }

    float PyMaterial::getThinFilmThicknessNM() const
    {
        return m_material->getThinFilmThicknessNM();
    }

    void PyMaterial::setCauchysCoefficients(const vec2p& val)
    {
        m_material->setCauchysCoefficients(val);
    }
    const vec2p& PyMaterial::getCauchysCoefficients() const
    {
        return m_material->getCauchysCoefficients();
    }

    void PyMaterial::setEnableDispersion(bool val)
    {
        m_material->setEnableDispersion(val);
    }
    bool PyMaterial::getEnableDispersion() const
    {
        return m_material->getEnableDispersion();
    }

    void PyMaterial::setAlbedoTexture(const PyTexture& tex, const vec2p& scale)
    {
        Material::TextureParameter texParam;
        texParam.texture = tex.getTexture();
        texParam.scale = scale;
        m_material->setAlbedoTexture(texParam);
    }
    void PyMaterial::setNormalTexture(const PyTexture& tex, const vec2p& scale)
    {
        Material::TextureParameter texParam;
        texParam.texture = tex.getTexture();
        texParam.scale = scale;
        m_material->setNormalTexture(texParam);
    }
    void PyMaterial::setORMTexture(const PyTexture& tex, const vec2p& scale)
    {
        Material::TextureParameter texParam;
        texParam.texture = tex.getTexture();
        texParam.scale = scale;
        m_material->setORMTexture(texParam);
    }
    void PyMaterial::setEmissiveTexture(const PyTexture& tex, const vec2p& scale)
    {
        Material::TextureParameter texParam;
        texParam.texture = tex.getTexture();
        texParam.scale = scale;
        m_material->setEmissiveTexture(texParam);
    }


	BINDING_FUNC(PyMaterial, m)
	{

		auto mat = pybind11::class_<PyMaterial, std::shared_ptr<PyMaterial>>(m, "Material");
        mat.def("setFromMaterialPreset", &PyMaterial::setFromMaterialPreset)
            .def("getName", &PyMaterial::getName)
            .def("setTransparency", &PyMaterial::setTransparency)
            .def("getTransparency", &PyMaterial::getTransparency)
            .def("setMetalness", &PyMaterial::setMetalness)
            .def("getMetalness", &PyMaterial::getMetalness)
            .def("setAlbedo", &PyMaterial::setAlbedo)
            .def("getAlbedo", &PyMaterial::getAlbedo, pybind11::return_value_policy::reference_internal)
            .def("setSpecularTint", &PyMaterial::setSpecularTint)
            .def("getSpecularTint", &PyMaterial::getSpecularTint, pybind11::return_value_policy::reference_internal)
            .def("setAbsorption", &PyMaterial::setAbsorption)
            .def("getAbsorption", &PyMaterial::getAbsorption, pybind11::return_value_policy::reference_internal)
            .def("setEmission", &PyMaterial::setEmission)
            .def("getEmission", &PyMaterial::getEmission, pybind11::return_value_policy::reference_internal)
            .def("setEmissionFocus", &PyMaterial::setEmissionFocus)
            .def("getEmissionFocus", &PyMaterial::getEmissionFocus)
            .def("setDielectricIOR", &PyMaterial::setDielectricIOR)
            .def("getDielectricIOR", &PyMaterial::getDielectricIOR)
            .def("setRoughness", &PyMaterial::setRoughness)
            .def("getRoughness", &PyMaterial::getRoughness)
            .def("setAnisotropy", &PyMaterial::setAnisotropy)
            .def("getAnisotropy", &PyMaterial::getAnisotropy)
            .def("setAnisotropyRotation", &PyMaterial::setAnisotropyRotation)
            .def("getAnisotropyRotation", &PyMaterial::getAnisotropyRotation)
            .def("setTwoSided", &PyMaterial::setTwoSided)
            .def("getTwoSided", &PyMaterial::getTwoSided)
            .def("setSpecularAmount", &PyMaterial::setSpecularAmount)
            .def("getSpecularAmount", &PyMaterial::getSpecularAmount)
            .def("setClearCoatAmount", &PyMaterial::setClearCoatAmount)
            .def("getClearCoatAmount", &PyMaterial::getClearCoatAmount)
            .def("setClearCoatRoughness", &PyMaterial::setClearCoatRoughness)
            .def("getClearCoatRoughness", &PyMaterial::getClearCoatRoughness)
            .def("setClearCoatIOR", &PyMaterial::setClearCoatIOR)
            .def("getClearCoatIOR", &PyMaterial::getClearCoatIOR)
            .def("setSheenRoughness", &PyMaterial::setSheenRoughness)
            .def("getSheenRoughness", &PyMaterial::getSheenRoughness)
            .def("setSheenTint", &PyMaterial::setSheenTint)
            .def("getSheenTint", &PyMaterial::getSheenTint, pybind11::return_value_policy::reference_internal)
            .def("setSheenAmount", &PyMaterial::setSheenAmount)
            .def("getSheenAmount", &PyMaterial::getSheenAmount)
            .def("setThinFilmThicknessNM", &PyMaterial::setThinFilmThicknessNM)
            .def("getThinFilmThicknessNM", &PyMaterial::getThinFilmThicknessNM)
            .def("setCauchysCoefficients", &PyMaterial::setCauchysCoefficients)
            .def("getCauchysCoefficients", &PyMaterial::getCauchysCoefficients)
            .def("setEnableDispersion", &PyMaterial::setEnableDispersion)
            .def("getEnableDispersion", &PyMaterial::getEnableDispersion)
            .def("setAlbedoTexture", &PyMaterial::setAlbedoTexture)
            .def("setNormalTexture", &PyMaterial::setNormalTexture)
            .def("setORMTexture", &PyMaterial::setORMTexture)
            .def("setEmissiveTexture", &PyMaterial::setEmissiveTexture);

		pybind11::enum_<MaterialPreset>(mat, "MaterialPreset")
			.value("BLANK", MaterialPreset::BLANK)
			.value("METAL_GOLD", MaterialPreset::METAL_GOLD)
			.value("METAL_SILVER", MaterialPreset::METAL_SILVER)
			.value("METAL_COPPER", MaterialPreset::METAL_COPPER)
			.value("METAL_BRASS", MaterialPreset::METAL_BRASS)
			.value("METAL_ALUMINIUM", MaterialPreset::METAL_ALUMINIUM)
			.value("GLASS", MaterialPreset::GLASS)
			.value("PLASTIC", MaterialPreset::PLASTIC)
			.export_values();
	}
}
