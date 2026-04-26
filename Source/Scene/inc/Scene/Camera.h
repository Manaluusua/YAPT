#pragma once

#include "SceneObject.h"
namespace YAPT
{
	class Camera : public SceneObject
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

	protected:

		void calculateProjection() const;

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