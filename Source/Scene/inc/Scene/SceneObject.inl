namespace YAPT
{
	inline const Transform& SceneObject::getTransform() const
	{
		return m_transform;
	}
	inline Transform& SceneObject::getTransform()
	{
		return m_transform;
	}
}