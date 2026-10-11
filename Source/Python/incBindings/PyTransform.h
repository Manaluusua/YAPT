#pragma once
#include <PyBindingsCommon.h>
#include <Math/Math.h>
namespace YAPT
{
	
	class Transform;
	class PyTransform
	{
	public:
		DECLARE_BINDING_CLASS(PyTransform);

		PyTransform(Transform* t);
		~PyTransform();

		void setTranslation(const vec3p& translation);
		void addTranslation(const vec3p& translation);
		const vec3p getTranslation() const;

		void setScale(const vec3p& scale);
		void addScale(const vec3p& scale);
		const vec3p getScale() const;

		void setOrientation(const vec4p& orientation);
		void addOrientation(const vec4p& orientation);
		const vec4p getOrientation() const;

		void lookAt(const vec3p& eye, const vec3p& at, const vec3p& up);

		void setParent(PyTransform* transform);

		// opaque identity of the wrapped transform and of its parent (0 if none), lets python rebuild the hierarchy
		uintptr_t getId() const;
		uintptr_t getParentId() const;

		vec3p right();
		vec3p up();
		vec3p forward();

	private:
		Transform* m_transform;
	};
}