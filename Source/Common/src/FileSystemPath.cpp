#include <Common/FileSystemPath.h>

namespace YAPT
{
	

	FileSystemPath::FileSystemPath(const char* path)
	{
		m_path =  std::filesystem::current_path();
		m_path += path;
		
	}

	FileSystemPath::~FileSystemPath()
	{

	}

	std::string FileSystemPath::getAbsolutePath()
	{
		return m_path.generic_string();
	}

}