#include <Gui/CameraController.h>
#include <Scene/Camera.h>
#include <Scene/Scene.h>
#include <qevent.h>
#include <qkeysequence.h>

namespace YAPT
{
	CameraController::CameraController(Scene* scene)
		:m_scene(scene),
		m_rotAngles(0,0),
		m_verticalRotationSpeed(1.f),
		m_horizontalRotationSpeed(1.f),
		m_movementSpeed(30.0),
		m_activeMovement(0.f, 0.f, 0.f),
		m_pendingMouseMovement(0.f, 0.f)
	{

	}
	CameraController::~CameraController()
	{

	}
	void CameraController::mouseMoveEvent(QMouseEvent* e)
	{
		QPoint p = e->pos();
		QPoint delta = (p - m_lastMousePosition);
		rotateCamera(glm::vec2(delta.x(), delta.y()));

		m_lastMousePosition = p;
	}
	void CameraController::mousePressEvent(QMouseEvent* e)
	{
		m_lastMousePosition = e->pos();
	}
	void CameraController::mouseReleaseEvent(QMouseEvent* e)
	{
		
	}

	void CameraController::keyPressEvent(QKeyEvent* e)
	{
		switch (e->key())
		{
		case Qt::Key::Key_W:
			m_activeMovement.z = 1;
			break;
		case Qt::Key::Key_S:
			m_activeMovement.z = -1;
			break;

		case Qt::Key::Key_A:
			m_activeMovement.x = -1;
			break;
		case Qt::Key::Key_D:
			m_activeMovement.x = 1;
			break;

		case Qt::Key::Key_Q:
			m_activeMovement.y = 1;
			break;
		case Qt::Key::Key_E:
			m_activeMovement.y = -1;
			break;

		default:
			//noop
			break;
		}

		
	}
	void CameraController::keyReleaseEvent(QKeyEvent* e)
	{
		switch (e->key())
		{
		case Qt::Key::Key_W:
			m_activeMovement.z = 0;
			break;
		case Qt::Key::Key_S:
			m_activeMovement.z = 0;
			break;

		case Qt::Key::Key_A:
			m_activeMovement.x = 0;
			break;
		case Qt::Key::Key_D:
			m_activeMovement.x = 0;
			break;

		case Qt::Key::Key_Q:
			m_activeMovement.y = 0;
			break;
		case Qt::Key::Key_E:
			m_activeMovement.y = 0;
			break;

		default:
			//noop
			break;
		}
	}

	void CameraController::update(float deltaTime)
	{
		Camera& cam = getCamera();
		if (m_pendingMouseMovement != glm::vec2(0, 0))
		{
			m_rotAngles.x += m_pendingMouseMovement.x * m_horizontalRotationSpeed * deltaTime;
			m_rotAngles.y += m_pendingMouseMovement.y * m_verticalRotationSpeed * deltaTime;

			quat rotHor = glm::angleAxis(m_rotAngles.x, s_worldUp);
			quat rotVer = glm::angleAxis(m_rotAngles.y, s_worldRight);

			cam.getTransform().setOrientation(rotHor * rotVer);

			m_pendingMouseMovement = glm::vec2(0, 0);
		}

		if (m_activeMovement != glm::vec3(0, 0, 0))
		{
			Camera& cam = getCamera();

			glm::vec3 relativeMov = m_activeMovement.x * cam.getTransform().right() +
				m_activeMovement.y * cam.getTransform().up() +
				m_activeMovement.z * cam.getTransform().forward();


			cam.getTransform().addTranslation(relativeMov * m_movementSpeed * deltaTime);
		}
	}

	Camera& CameraController::getCamera()
	{
		return *m_scene->getMainCamera();
	}

	void CameraController::rotateCamera(const glm::vec2& screenSpaceDelta)
	{
		m_pendingMouseMovement = screenSpaceDelta;

		

	}

}