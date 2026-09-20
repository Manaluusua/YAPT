#include <PyReadback.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS(PyReadbackObject)

	PyReadbackObject::PyReadbackObject(ReadbackObject* readback)
		:m_readback(readback)
	{
	}
	PyReadbackObject::~PyReadbackObject()
	{
		m_readback = nullptr;
	}

	ReadbackState PyReadbackObject::getState()
	{
		if (m_readback.get() == nullptr)
		{
			return ReadbackState::Freed;
		}
		return m_readback->getState();
	}

	bool PyReadbackObject::isReady()
	{
		return getState() == ReadbackState::Ready;
	}

	void PyReadbackObject::release()
	{
		m_readback = nullptr;
	}

	BINDING_FUNC(PyReadbackObject, m)
	{
		pybind11::enum_<ReadbackTarget>(m, "ReadbackTarget")
			.value("FinalColor", ReadbackTarget::FinalColor);

		pybind11::enum_<ReadbackState>(m, "ReadbackState")
			.value("Created", ReadbackState::Created)
			.value("Pending", ReadbackState::Pending)
			.value("Ready", ReadbackState::Ready)
			.value("Freed", ReadbackState::Freed);

		pybind11::class_<PyReadbackObject, std::shared_ptr<PyReadbackObject>>(m, "ReadbackObject")
			.def("getState", &PyReadbackObject::getState)
			.def("isReady", &PyReadbackObject::isReady)
			.def("release", &PyReadbackObject::release);
	}
}
