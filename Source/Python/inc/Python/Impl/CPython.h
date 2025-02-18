#pragma once

#include <Python/Python.h>

namespace YAPT
{

	class CPython : public Python
	{
	public:
		CPython();
		virtual void executeFile(const char* filePath) override;
	protected:

		virtual ~CPython() final;
	};
}