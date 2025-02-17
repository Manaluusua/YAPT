#pragma once

#include <PyYAPT/PyYAPT.h>

namespace YAPT
{

	class CPyYAPT : public PyYAPT
	{
	public:
		CPyYAPT();
		virtual void executeFile(const char* filePath) override;
	protected:

		virtual ~CPyYAPT() final;
	};
}