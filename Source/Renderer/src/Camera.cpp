#include <Renderer/Camera.h>

namespace YAPT
{
    Camera::Camera(void)
        :m_up(0, 1, 0),
        m_fovy(glm::pi<float>() * 0.3f),
        m_aspectRatio(1),
        m_near(0.1f),
        m_far(1000.f),
        m_orthoSize(600.f),
        m_projectionType(PERSPECTIVE),
        m_projectionCacheValid(false)
    {
     
    }


    Camera::~Camera(void)
    {
    }


    const mat4& Camera::getProjectionMatrix() const
    {

        if (!m_projectionCacheValid)
        {
            calculateProjection();
        }

        return m_projMatrix;
    }


    const mat4& Camera::getViewMatrix() const
    {
        if (m_transform.isMatrixDirty())
        {
            const mat4& m = m_transform.getMatrix();

            m_viewMatrix = mat4(1.f);
            glm::translate(m_viewMatrix, -m_transform.getTranslation());

            mat4 rotMat = glm::mat4_cast(glm::inverse(m_transform.getOrientation()));;

            m_viewMatrix *= rotMat;
        }
        return m_viewMatrix;
    }

    void Camera::calculateProjection() const
    {
        if (m_projectionType == PERSPECTIVE)
        {
            m_projMatrix = glm::perspective(m_fovy, m_aspectRatio, m_near, m_far);
        }
        else 
        {
            float orthoHalf = m_orthoSize * 0.5f;
            m_projMatrix = glm::ortho(-orthoHalf * m_aspectRatio, orthoHalf * m_aspectRatio, -orthoHalf, orthoHalf, m_near, m_far);
        }

        m_projectionCacheValid = true;
    }

    void Camera::setProjectionPerspective(float fovY, float aspectRatio, float nearDist, float farDist)
    {
        m_fovy = fovY;
        m_aspectRatio = aspectRatio;
        m_near = nearDist;
        m_far = farDist;
        m_projectionCacheValid = false;
        m_projectionType = PERSPECTIVE;
    }

    void Camera::setProjectionOrthographic(float size, float aspectRatio, float nearDist, float farDist)
    {
        m_orthoSize = size;
        m_aspectRatio = aspectRatio;
        m_near = nearDist;
        m_far = farDist;
        m_projectionCacheValid = false;
        m_projectionType = ORTHOGRAPHIC;
    }

    void Camera::setOrthographicSize(float size)
    {

        m_projectionCacheValid = false;
        m_orthoSize = size;
    }

    void Camera::setProjectionType(ProjectionType type)
    {
        if (type != m_projectionType)
        {
            m_projectionCacheValid = false;
        }

        m_projectionType = type;

    }

    void Camera::setFOVY(float y)
    {
        m_fovy = y;
        m_projectionCacheValid = false;
    }
    void Camera::setAspectRatio(float ar)
    {
        m_aspectRatio = ar;
        m_projectionCacheValid = false;

    }
    void Camera::setNearPlane(float nearPlane)
    {
        m_near = nearPlane;
        m_projectionCacheValid = false;
    }
    void Camera::setFarPlane(float farPlane)
    {
        m_far = farPlane;
        m_projectionCacheValid = false;
    }

    float Camera::getFOVY() const
    {
        return m_fovy;
    }

    float Camera::getAspectRatio() const
    {
        return m_aspectRatio;
    }
    float Camera::getNearPlane() const
    {
        return m_near;
    }
    float Camera::getFarPlance() const
    {
        return m_far;
    }

    float Camera::getOrthographicSize() const
    {
        return m_orthoSize;
    }

    Camera::ProjectionType Camera::getProjectionType() const
    {
        return m_projectionType;
    }

   


    


  
}