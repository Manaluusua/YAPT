#pragma once

#include <filesystem>


namespace YAPT
{
	class FileSystemPath
	{
	public:
		FileSystemPath(const char* path);
		~FileSystemPath();

		std::string getAbsolutePath();

	private:
		std::filesystem::path m_path;
	};

}

