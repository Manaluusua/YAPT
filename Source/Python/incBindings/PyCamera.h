#pragma once
#include <PyBindingsCommon.h>
#include <Scene/Camera.h>
#include <PyTransform.h>
namespace YAPT
{

	class Camera;


	class PyCamera
	{
	public:

		DECLARE_BINDING_CLASS(PyCamera);

        PyCamera(Camera* camera)
            : m_camera(camera),
            m_transform(&m_camera->getTransform())
        {}
        ~PyCamera() = default;

        void setProjectionPerspective(float fovY, float aspectRatio, float nearDist, float farDist) 
        {
            m_camera->setProjectionPerspective(fovY, aspectRatio, nearDist, farDist);
        }

        void setProjectionOrthographic(float size, float aspectRatio, float nearDist, float farDist)
        {
            m_camera->setProjectionOrthographic(size, aspectRatio, nearDist, farDist);
        }

        void setFOVY(float y) 
        {
            m_camera->setFOVY(y);
        }

        void setAspectRatio(float ar) 
        {
            m_camera->setAspectRatio(ar);
        }

        void setNearPlane(float nearPlane) 
        {
            m_camera->setNearPlane(nearPlane);
        }

        void setFarPlane(float farPlane) 
        {
            m_camera->setFarPlane(farPlane);
        }

        void setOrthographicSize(float size)
        {
            m_camera->setOrthographicSize(size);
        }

        void setProjectionType(Camera::ProjectionType type)
        {
            m_camera->setProjectionType(type);
        }

        float getFOVY() const 
        {
            return m_camera->getFOVY();
        }

        float getAspectRatio() const 
        {
            return m_camera->getAspectRatio();
        }

        float getNearPlane() const 
        {
            return m_camera->getNearPlane();
        }

        float getFarPlance() const 
        {
            return m_camera->getFarPlance();
        }

        float getOrthographicSize() const 
        {
            return m_camera->getOrthographicSize();
        }

        Camera::ProjectionType getProjectionType() const
        {
            return m_camera->getProjectionType();
        }

        PyTransform& getTransform() 
        {
            return m_transform;
        }

	private:
		Camera* m_camera;
        PyTransform m_transform;
	};
}