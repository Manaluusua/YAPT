

namespace YAPT
{
    inline Camera::Camera(void)
        :m_fovy(glm::pi<float>() * 0.3f),
        m_aspectRatio(1),
        m_near(0.1f),
        m_far(1000.f),
        m_orthoSize(600.f),
        m_projectionType(PERSPECTIVE),
        m_projectionCacheValid(false)
    {

    }


    inline Camera::~Camera(void)
    {
    }


    inline const mat4& Camera::getProjectionMatrix() const
    {

        if (!m_projectionCacheValid)
        {
            calculateProjection();
        }

        return m_projMatrix;
    }


    inline const mat4& Camera::getViewMatrix() const
    {

            m_viewMatrix = glm::inverse(getTransform().getMatrixWS());
        return m_viewMatrix;
    }

    inline void Camera::calculateProjection() const
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

    inline void Camera::setProjectionPerspective(float fovY, float aspectRatio, float nearDist, float farDist)
    {
        m_fovy = fovY;
        m_aspectRatio = aspectRatio;
        m_near = nearDist;
        m_far = farDist;
        m_projectionCacheValid = false;
        m_projectionType = PERSPECTIVE;
    }

    inline void Camera::setProjectionOrthographic(float size, float aspectRatio, float nearDist, float farDist)
    {
        m_orthoSize = size;
        m_aspectRatio = aspectRatio;
        m_near = nearDist;
        m_far = farDist;
        m_projectionCacheValid = false;
        m_projectionType = ORTHOGRAPHIC;
    }

    inline void Camera::setOrthographicSize(float size)
    {

        m_projectionCacheValid = false;
        m_orthoSize = size;
    }

    inline void Camera::setProjectionType(ProjectionType type)
    {
        if (type != m_projectionType)
        {
            m_projectionCacheValid = false;
        }

        m_projectionType = type;

    }

    inline void Camera::setFOVY(float y)
    {
        m_fovy = y;
        m_projectionCacheValid = false;
    }
    inline void Camera::setAspectRatio(float ar)
    {
        m_aspectRatio = ar;
        m_projectionCacheValid = false;

    }
    inline void Camera::setNearPlane(float nearPlane)
    {
        m_near = nearPlane;
        m_projectionCacheValid = false;
    }
    inline void Camera::setFarPlane(float farPlane)
    {
        m_far = farPlane;
        m_projectionCacheValid = false;
    }

    inline float Camera::getFOVY() const
    {
        return m_fovy;
    }

    inline float Camera::getAspectRatio() const
    {
        return m_aspectRatio;
    }
    inline float Camera::getNearPlane() const
    {
        return m_near;
    }
    inline float Camera::getFarPlance() const
    {
        return m_far;
    }

    inline float Camera::getOrthographicSize() const
    {
        return m_orthoSize;
    }

    inline Camera::ProjectionType Camera::getProjectionType() const
    {
        return m_projectionType;
    }

   


    


  
}