#pragma once
#include <PyBindingsCommon.h>
#include <Renderer/ReadbackHandle.h>
#include <memory>

namespace YAPT
{

	class PyReadbackData
	{
	public:

		PyReadbackData(ReadbackObject* owner, const ReadbackData* data);
		~PyReadbackData();

		//width in texels for textures, size in bytes for buffers
		size_t getWidth() const;
		size_t getHeight() const;
		size_t getDepthOrSlices() const;

		size_t getTexelSizeInBytes() const;
		size_t getRowPitchInBytes() const;
		size_t getSizeInBytes() const;

		ResourceFormat getFormat() const;
		ResourceDimension getDimensions() const;

		pybind11::bytes getBytes() const;
	private:
		ReadbackHandle m_owner;
		const ReadbackData* m_data;
	};

	class PyReadbackObject
	{
	public:
		DECLARE_BINDING_CLASS(PyReadbackObject);

		PyReadbackObject(ReadbackObject* readback);
		~PyReadbackObject();

		ReadbackState getState();
		bool isReady();
		void release();

		//returns None until the readback is ready
		std::shared_ptr<PyReadbackData> getData();

		const ReadbackHandle& getReadback() const { return m_readback; }
	private:
		ReadbackHandle m_readback;
	};
}
