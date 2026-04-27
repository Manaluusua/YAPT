#pragma once
#include <Common/RCObjectPtr.h>
#include <PyBindingsCommon.h>
#include <Scene/Scene.h>
#include <Scene/SceneObject.h>
#include <PyTransform.h>
#include <memory>
#include <vector>

namespace YAPT
{
	class SceneObject;
	class PySceneObject
	{
	public:
		DECLARE_BINDING_CLASS(PySceneObject);

		PySceneObject(const char* name, SceneObject* obj);
		~PySceneObject();

		PyTransform* getTransform();
		const char* getName() const;

	protected:
		SceneObject* getWrappedObject() const { return m_obj.get(); }
	private:
	
		std::string m_name;
		RCObjectPtr<SceneObject> m_obj;
		PyTransform m_transform;
	};
}