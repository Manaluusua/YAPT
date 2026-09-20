#pragma once
#include <PyBindingsCommon.h>
#include <Renderer/ReadbackHandle.h>

namespace YAPT
{
	class PyReadbackObject
	{
	public:
		DECLARE_BINDING_CLASS(PyReadbackObject);

		PyReadbackObject(ReadbackObject* readback);
		~PyReadbackObject();

		ReadbackState getState();
		bool isReady();
		void release();

		const ReadbackHandle& getReadback() const { return m_readback; }
	private:
		ReadbackHandle m_readback;
	};
}
