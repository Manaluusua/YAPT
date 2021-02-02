#pragma once

#include <Common/RCObject.h>
#include <Scene/Transform.h>
namespace YAPT
{
	class Camera : public RCObject
	{

	public:

		enum ProjectionType
		{
			PERSPECTIVE,
			ORTHOGRAPHIC
		};

		Camera();
		virtual ~Camera();

		void setProjectionPerspective(float fovY, float aspectRatio, float nearDist, float farDist);
		void setProjectionOrthographic(float size, float aspectRatio, float nearDist, float farDist);

		const mat4& getProjectionMatrix() const;
		const mat4& getViewMatrix() const;

		void moveCameraViewRelative(const vec3& v);
		void rotateCameraViewRelative(const vec3& v);

		void lookAt(const vec3& eye, const vec3& at);

		void setFOVY(float y);
		void setAspectRatio(float ar);
		void setNearPlane(float nearPlane);
		void setFarPlane(float farPlane);
		void setOrthographicSize(float size);
		void setProjectionType(ProjectionType type);

		float getFOVY() const;
		float getAspectRatio() const;
		float getNearPlane() const;
		float getFarPlance() const;
		float getOrthographicSize() const;


		ProjectionType getProjectionType() const;

		const Transform& getTransform() const { return m_transform; }
		Transform& getTransform() { return m_transform; }

	protected:

		void calculateProjection() const;

		Transform m_transform;

		mutable mat4 m_viewMatrix;
		mutable mat4 m_projMatrix;

		float m_fovy;
		float m_aspectRatio;
		float m_near;
		float m_far;
		float m_orthoSize;

		ProjectionType m_projectionType;

		mutable bool m_projectionCacheValid;

	};
}

#include "Camera.inl"