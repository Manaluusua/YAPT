#include <Scene/Impl/CLightObject.h>
#include <Scene/Impl/CScene.h>
#include <Renderer/Renderer.h>

namespace YAPT
{
	CLightObject::CLightObject(Renderer* renderer, CScene* scene, LightType type)
		:m_scene(scene),
		m_pendingRefresh(false)
	{
		m_light = renderer->createLight(type);
		m_light->Release();
		m_light->setTransform(getTransform().getMatrixWS());
		getTransform().setDirtyCallback(CLightObject::transformChanged, this);
	}

	CLightObject::~CLightObject()
	{
		if (m_pendingRefresh)
		{
			m_scene->cancelRefresh(this);
		}
	}

	void CLightObject::transformChanged(void* ptr)
	{
		CLightObject* obj = static_cast<CLightObject*>(ptr);
		if (!obj->m_pendingRefresh)
		{
			obj->m_pendingRefresh = true;
			obj->m_scene->needsRefresh(obj);
		}
	}

	void CLightObject::refresh()
	{
		m_light->setTransform(getTransform().getMatrixWS());
		m_pendingRefresh = false;
	}
}
