#pragma once

#include <Common/RCObject.h>
#include <Scene/Transform.h>
namespace YAPT
{
	class SceneObject : public RCObject
	{
	public:
		const Transform& getTransform() const;
		Transform& getTransform();
		virtual ~SceneObject() {}

	private:
		Transform m_transform;
	};
}

#include "SceneObject.inl"