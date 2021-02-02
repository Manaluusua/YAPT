
namespace YAPT
{
    inline Transform::Transform(TransformDirtyCallback dirtyCallback, void* usrData)
        :m_dirtyCallback(dirtyCallback),
        m_usrData(usrData),
        m_translation(0, 0, 0),
        m_scale(1, 1, 1),
        m_orientation(1.f, 0.f, 0.f, 0.f),
        m_dirty(true)
    {
        
    }
    inline Transform::~Transform()
    {

    }

    inline void Transform::setDirty()
    {
        if (!m_dirty)
        {
            if (m_dirtyCallback)
            {
                m_dirtyCallback(m_usrData);
            }
        }
        m_dirty = true;
    }

    inline const mat4& Transform::getMatrix() const
    {
        if (m_dirty)
        {
            recalculateMatrix();
            m_dirty = false;
        }
        return m_matrix;
    }

    inline void Transform::recalculateMatrix() const
    {
        m_matrix = mat4(1.f);
        m_matrix[0][0] = m_scale.x;
        m_matrix[1][1] = m_scale.y;
        m_matrix[2][2] = m_scale.z;

        mat4 rotationMat = glm::mat4_cast(m_orientation);

        m_matrix *= rotationMat;
        m_matrix[3][0] = m_translation.x;
        m_matrix[3][1] = m_translation.y;
        m_matrix[3][2] = m_translation.z;
    }

    inline void Transform::setTranslation(const vec3& v)
    {
        m_translation = v;
        setDirty();
    }
    inline void Transform::addTranslation(const vec3& v)
    {
        m_translation += v;
        setDirty();
    }

    inline const vec3& Transform::getTranslation() const
    {
        return m_translation;
    }

    inline void Transform::setScale(const vec3& scale)
    {
        m_scale = scale;
        setDirty();
    }
    inline void Transform::addScale(const vec3& scale)
    {
        m_scale = scale;
        setDirty();
    }

    inline const vec3& Transform::getScale() const
    {
        return m_scale;
    }


    inline const quat& Transform::getOrientation() const
    {
        return m_orientation;
    }


    inline void Transform::setOrientation(const quat& orientation)
    {
        m_orientation = orientation;
        setDirty();
    }


    inline void  Transform::addOrientation(const quat& orientation)
    {
        m_orientation *= orientation;
        setDirty();
    }

    inline vec3 Transform::right()
    {
        return mat3(getMatrix()) * s_worldRight;
    }
    inline vec3 Transform::up()
    {
        return mat3(getMatrix()) * s_worldUp;
    }
    inline vec3 Transform::forward()
    {
        return mat3(getMatrix()) * s_worldForward;
    }

    inline void Transform::lookAt(const vec3& position, const vec3& at, const vec3& up)
    {
        m_matrix = glm::inverse(glm::lookAt(position, at, up));
        m_translation = position;
        m_orientation = glm::quat_cast(m_matrix);
        m_scale = vec3(1.0f, 1.0f, 1.0f);

        m_dirty = false;
    }
}