#pragma once
#include "CommonDefines.h"
#include <Math/Math.h>
#include <vector>

namespace YAPT
{
	typedef void(*TransformDirtyCallback)(void*);
	class Transform
	{
	public:
		inline static constexpr size_t CHILD_INDEX_INVALID = size_t(-1);

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

		const mat4& getMatrixWS() const;

		vec3 right();
		vec3 up();
		vec3 forward();

		void lookAt(const vec3& position, const vec3& at, const vec3& up);

		bool isDirty() const { return m_dirty; }

		void setParent(Transform* parent);
		Transform* getParent() const { return m_parent; }

		void setDirtyCallback(TransformDirtyCallback dirtyCallback, void* usrData);
	private:
		void setDirty();
		void calculateMatrixOS(mat4& matOut) const;

		void setChildIndex(size_t index);
		size_t addChild(Transform* c);
		void removeChild(size_t index);
		void removeFromParentChain();

		mutable mat4 m_worldMatrix;

		TransformDirtyCallback m_dirtyCallback;
		void* m_usrData;

		Transform* m_parent;
		std::vector<Transform*> m_children;
		size_t m_childIndex;

		quat m_orientation;
		vec3 m_scale;
		vec3 m_translation;


		mutable bool m_dirty;
	};
}


#include <Scene/Transform.inl>
