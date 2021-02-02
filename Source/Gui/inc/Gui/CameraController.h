#pragma once

#include <QtWidgets/qwidget.h>
#include <Math/Math.h>
namespace YAPT
{
	class Scene;
	class Camera;
	class CameraController 
	{
	public:
		CameraController(Scene* scene);
		~CameraController();

		void setVerticalRotationSpeed(float speed) { m_verticalRotationSpeed = speed; }
		float getVerticalRotationSpeed() const { return m_verticalRotationSpeed; }

		void setHorizontalRotationSpeed(float speed) { m_horizontalRotationSpeed = speed; }
		float getHorizontalRotationSpeed() const { return m_horizontalRotationSpeed; }

		void setMovementSpeed(float speed) { m_movementSpeed = speed; }
		float getMovementSpeed() const { return m_movementSpeed; }

		void update(float deltaTime);


		void mouseMoveEvent(QMouseEvent* e);
		void mousePressEvent(QMouseEvent* e);
		void mouseReleaseEvent(QMouseEvent* e);
		void keyPressEvent(QKeyEvent* e);
		void keyReleaseEvent(QKeyEvent* e);
	private:

		void rotateCamera(const glm::vec2& screenSpaceDelta);

		Camera& getCamera();

		

		Scene* m_scene;

		glm::vec2 m_pendingMouseMovement;
		glm::vec3 m_activeMovement;

		QPoint m_lastMousePosition;
		glm::vec2 m_rotAngles;

		float m_verticalRotationSpeed;
		float m_horizontalRotationSpeed;
		float m_movementSpeed;
	};




}
