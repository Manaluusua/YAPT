#pragma once
#include "CommonDefines.h"
#include <Math/Math.h>

namespace YAPT
{
	typedef void(*TransformDirtyCallback)(void*);

	class Transform
	{
	public:
		Transform(TransformDirtyCallback dirtyCallback = nullptr, void* usrData = nullptr);
		~Transform();


		void setTranslation(const vec3& translation);
		void addTranslation(const vec3& translation);
		const vec3& getTranslation() const;

		void setScale(const vec3& scale);
		void addScale(const vec3& scale);
		const vec3& getScale() const;

		void setOrientation(const quat& orientation);
		void addOrientation(const quat& orientation);
		const quat& getOrientation() const;

		const mat4& getMatrix() const;

		vec3 right();
		vec3 up();
		vec3 forward();

		void lookAt(const vec3& position, const vec3& at, const vec3& up);

		bool isMatrixDirty() const { return m_dirty; }
	private:
		void setDirty();
		void recalculateMatrix() const;

		const TransformDirtyCallback m_dirtyCallback;
		void* m_usrData;

		mutable mat4 m_matrix;

		quat m_orientation;
		vec3 m_scale;
		vec3 m_translation;

		mutable bool m_dirty;
	};
}


#include <Scene/Transform.inl>
