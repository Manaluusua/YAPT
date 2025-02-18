#pragma once
#include <PyBindingsCommon.h>

namespace YAPT
{

	class PyMesh
	{
	public:
		static void registerToPythonModule(pybind11::module& m);

		PyMesh();
		~PyMesh();

	private:
		
	};
}