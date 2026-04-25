#include <PyBindingsCommon.h>
#include <Common/RCObjectPtr.h>
#include <vector>

struct RegisterPythonClassEntry
{
	RegisterPythonClassFunc registerFunc;
	int selfType;
	int baseType;
};

static std::vector<RegisterPythonClassEntry> s_registerPythonClassFuncs;


void addPythonRegisterFunc(RegisterPythonClassFunc f, int typeID, int baseTypeID)
{
	s_registerPythonClassFuncs.push_back({ f, typeID, baseTypeID });
}

int getNextRegisterTypeID()
{
	static int id = 0;
	return id++;
}

int getRankForEntryIndex(int index, int* parentsList, int* ranks)
{
	if (ranks[index] != -1)
	{
		return ranks[index];
	}

	if (parentsList[index] == -1)
	{
		return 0;
	}

	int parentRank = getRankForEntryIndex(parentsList[index], parentsList, ranks);
	return parentRank + 1;
}

PYBIND11_MODULE(py_yapt, m)
{
    m.doc() = "YAPT Python bindings module"; 

	//a naive sort to initialise bases before their inherited registrars
	std::vector<int> typeToBase;
	std::vector<int> typeToRegisterFuncs;
	std::vector<int> typeToRank;
	typeToBase.resize(s_registerPythonClassFuncs.size(), -1);
	typeToRegisterFuncs.resize(s_registerPythonClassFuncs.size());
	typeToRank.resize(s_registerPythonClassFuncs.size(), -1);

	for(size_t i = 0; i < s_registerPythonClassFuncs.size(); ++i)
	{
		const RegisterPythonClassEntry& entry = s_registerPythonClassFuncs[i];
		typeToBase[entry.selfType] = entry.baseType;
		typeToRegisterFuncs[entry.selfType] = (int)i;
	}

	for (size_t i = 0; i < typeToRank.size(); ++i)
	{
		typeToRank[i] = getRankForEntryIndex((int)i, typeToBase.data(), typeToRank.data());
	}

	std::vector<std::pair<int, int>> sortArray;
	sortArray.reserve(typeToRank.size());
	for (size_t i = 0; i < typeToRank.size(); ++i)
	{
		sortArray.emplace_back(typeToRank[i], (int)i);
	}
	std::sort(sortArray.begin(), sortArray.end(), 
		[](const std::pair<int, int>& a, const std::pair<int, int>& b)
		{
			return a.first < b.first;
		}
	);

	for (size_t i = 0; i < sortArray.size(); ++i)
	{
		int typeIndex = sortArray[i].second;
		int registerFuncIndex = typeToRegisterFuncs[typeIndex];
		s_registerPythonClassFuncs[registerFuncIndex].registerFunc(m);
	}
}