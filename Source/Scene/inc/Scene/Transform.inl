
namespace YAPT
{
    

    inline Transform::Transform(TransformDirtyCallback dirtyCallback, void* usrData)
        :m_parent(nullptr),
        m_childIndex(CHILD_INDEX_INVALID),
        m_translation(0, 0, 0),
        m_scale(1, 1, 1),
        m_orientation(1.f, 0.f, 0.f, 0.f),
        m_dirty(true)
    {
        setDirtyCallback(dirtyCallback, usrData);
    }
    inline Transform::~Transform()
    {

    }

    inline void Transform::setDirtyCallback(TransformDirtyCallback dirtyCallback, void* usrData)
    {
        m_dirtyCallback = dirtyCallback;
        m_usrData = usrData;
    }

    inline void Transform::setDirty()
    {
        if (!m_dirty)
        {
            if (m_dirtyCallback)
            {
                m_dirtyCallback(m_usrData);
            }
            for (size_t i = 0; i < m_children.size(); ++i)
            {
                m_children[i]->setDirty();
            }
        }
        m_dirty = true;
    }


    inline const mat4& Transform::getMatrixWS() const
    {
        if (m_dirty)
        {
            calculateMatrixOS(m_worldMatrix);

            if (m_parent != nullptr)
            {
                m_worldMatrix = m_parent->getMatrixWS() * m_worldMatrix;
            }

            m_dirty = false;
        }
        return m_worldMatrix;
    }

    inline void Transform::calculateMatrixOS(mat4& matOut) const
    {
        matOut = mat4(1.f);
        matOut[0][0] = m_scale.x;
        matOut[1][1] = m_scale.y;
        matOut[2][2] = m_scale.z;

        mat4 rotationMat = glm::mat4_cast(m_orientation);

        matOut = rotationMat * matOut;
        matOut[3][0] = m_translation.x;
        matOut[3][1] = m_translation.y;
        matOut[3][2] = m_translation.z;
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
        return mat3(getMatrixWS()) * s_worldRight;
    }
    inline vec3 Transform::up()
    {
        return mat3(getMatrixWS()) * s_worldUp;
    }
    inline vec3 Transform::forward()
    {
        return mat3(getMatrixWS()) * s_worldForward;
    }

    inline void Transform::lookAt(const vec3& position, const vec3& at, const vec3& up)
    {
        mat4 m = glm::inverse(glm::lookAt(position, at, up));
        m_translation = position;
        m_orientation = glm::quat_cast(m);
        m_scale = vec3(1.0f, 1.0f, 1.0f);

        setDirty();
    }

    inline void Transform::setParent(Transform* parent)
    {
        if (m_parent != nullptr)
        {
            m_parent->removeChild(m_childIndex);
        }

        m_parent = parent;
        m_parent->addChild(this);

        setDirty();
    }

    inline void Transform::setChildIndex(size_t index)
    {
        m_childIndex = index;
    }
    inline size_t Transform::addChild(Transform* c)
    {
        size_t ind = m_children.size();
        m_children.push_back(c);
        return ind;
    }
    inline void Transform::removeChild(size_t index)
    {
        if (m_children.size() > 1)
        {
            Transform* s = m_children.back();
            std::swap(m_children[index], s);
            s->setChildIndex(index);
        }
        m_children.pop_back();
        
    }
}