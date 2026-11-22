#include <Renderer/Shared/LightProxy.h>
#include <Renderer/Shared/LightManager.h>

namespace YAPT
{
	LightProxy::LightProxy(LightManager* mngr, LightType type)
		:m_type(type),
		m_transform(1.f),
		m_color(1.f, 1.f, 1.f),
		m_intensity(1.f),
		m_spotInnerAngle(0.f),
		m_spotOuterAngle(glm::pi<float>() * 0.25f),
		m_mngr(mngr),
		_lightState(LIGHTSTATE_NOCHANGES),
		_id(size_t(-1))
	{

	}

	LightProxy::~LightProxy()
	{

	}

	void LightProxy::setType(LightType type)
	{
		m_type = type;
		setDirty();
	}

	void LightProxy::setTransform(const mat4& transform)
	{
		m_transform = transform;
		setDirty();
	}

	void LightProxy::setColor(const vec3p& color)
	{
		m_color = color;
		setDirty();
	}

	void LightProxy::setIntensity(float intensity)
	{
		m_intensity = intensity;
		setDirty();
	}

	void LightProxy::setSpotConeAngles(float innerAngle, float outerAngle)
	{
		m_spotOuterAngle = glm::clamp(outerAngle, 0.f, glm::pi<float>() * 0.5f);
		m_spotInnerAngle = glm::clamp(innerAngle, 0.f, m_spotOuterAngle);
		setDirty();
	}

	void LightProxy::setDirty()
	{
		m_mngr->lightChanged(this);
	}

	void LightProxy::allReferencesReleased()
	{
		m_mngr->lightReleased(this);
	}
}
