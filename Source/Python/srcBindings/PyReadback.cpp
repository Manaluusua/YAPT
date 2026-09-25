#include <PyReadback.h>
#include <Gfx/GfxBasicTypesUtility.h>

namespace YAPT
{
	/////////////////////// PyReadbackData ///////////////////////

	PyReadbackData::PyReadbackData(ReadbackObject* owner, const ReadbackData* data)
		:m_owner(owner),
		m_data(data)
	{
	}
	PyReadbackData::~PyReadbackData()
	{
		m_data = nullptr;
		m_owner = nullptr;
	}

	size_t PyReadbackData::getWidth() const
	{
		return m_data->widthOrSizeInBytes;
	}
	size_t PyReadbackData::getHeight() const
	{
		return m_data->height;
	}
	size_t PyReadbackData::getDepthOrSlices() const
	{
		return m_data->depthOrSlices;
	}

	size_t PyReadbackData::getTexelSizeInBytes() const
	{
		if (m_data->dimensions == ResourceDimension::BUFFER)
		{
			return 1;
		}
		return getFormatSizeInBytes(m_data->format);
	}

	size_t PyReadbackData::getRowPitchInBytes() const
	{
		if (m_data->dimensions == ResourceDimension::BUFFER)
		{
			return 0;
		}
		//assume tightly packed for now, should return row pitch later if needed
		return m_data->widthOrSizeInBytes * getTexelSizeInBytes();
	}

	size_t PyReadbackData::getSizeInBytes() const
	{
		if (m_data->dimensions == ResourceDimension::BUFFER)
		{
			return m_data->widthOrSizeInBytes;
		}

		const size_t height = m_data->height > 0 ? m_data->height : 1;
		size_t sizeInBytes = getRowPitchInBytes() * height;

		//a volume maps as a single subresource covering every depth slice, while an array maps one slice at a time
		if (m_data->dimensions == ResourceDimension::TEXTURE_3D)
		{
			sizeInBytes *= m_data->depthOrSlices > 0 ? m_data->depthOrSlices : 1;
		}
		return sizeInBytes;
	}

	ResourceFormat PyReadbackData::getFormat() const
	{
		return m_data->format;
	}
	ResourceDimension PyReadbackData::getDimensions() const
	{
		return m_data->dimensions;
	}

	pybind11::bytes PyReadbackData::getBytes() const
	{
		if (m_data->data == nullptr)
		{
			return pybind11::bytes();
		}
		return pybind11::bytes(reinterpret_cast<const char*>(m_data->data), getSizeInBytes());
	}

	/////////////////////// PyReadbackObject ///////////////////////

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

	std::shared_ptr<PyReadbackData> PyReadbackObject::getData()
	{
		if (m_readback.get() == nullptr)
		{
			return nullptr;
		}

		const ReadbackData* data = m_readback->getData();
		if (data == nullptr)
		{
			return nullptr;
		}
		return std::make_shared<PyReadbackData>(m_readback.get(), data);
	}

	BINDING_FUNC(PyReadbackObject, m)
	{
		pybind11::enum_<ReadbackTarget>(m, "ReadbackTarget")
			.value("FinalColor", ReadbackTarget::FinalColor);

		pybind11::enum_<ReadbackState>(m, "ReadbackState")
			.value("Created", ReadbackState::Created)
			.value("Pending", ReadbackState::Pending)
			.value("Ready", ReadbackState::Ready)
			.value("Failed", ReadbackState::Failed)
			.value("Freed", ReadbackState::Freed);

		pybind11::class_<PyReadbackData, std::shared_ptr<PyReadbackData>>(m, "ReadbackData")
			.def("getWidth", &PyReadbackData::getWidth)
			.def("getHeight", &PyReadbackData::getHeight)
			.def("getDepthOrSlices", &PyReadbackData::getDepthOrSlices)
			.def("getTexelSizeInBytes", &PyReadbackData::getTexelSizeInBytes)
			.def("getRowPitchInBytes", &PyReadbackData::getRowPitchInBytes)
			.def("getSizeInBytes", &PyReadbackData::getSizeInBytes)
			.def("getFormat", &PyReadbackData::getFormat)
			.def("getDimensions", &PyReadbackData::getDimensions)
			.def("getBytes", &PyReadbackData::getBytes);

		pybind11::class_<PyReadbackObject, std::shared_ptr<PyReadbackObject>>(m, "ReadbackObject")
			.def("getState", &PyReadbackObject::getState)
			.def("isReady", &PyReadbackObject::isReady)
			.def("release", &PyReadbackObject::release)
			.def("getData", &PyReadbackObject::getData);
	}
}
