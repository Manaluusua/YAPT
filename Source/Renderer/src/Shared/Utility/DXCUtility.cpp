#include <Renderer/Shared/Utility/DXCUtility.h>
#include <filesystem>
#include <assert.h>
#include <comdef.h>
namespace YAPT
{
	//quick hacky includehandler to allow including relative to the original shader file
	class IncludeHandler : public IDxcIncludeHandler
	{
	public:
		IncludeHandler(LPCWSTR includingFile, IDxcIncludeHandler* actualHandler)
			:m_includingFile(includingFile),
			m_actualHandler(actualHandler),
			m_refCount(0)
		{

		}
		virtual HRESULT STDMETHODCALLTYPE LoadSource(
			_In_ LPCWSTR pFilename,                                   // Candidate filename.
			_COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
		)
		{
			const wchar_t* str;


			std::filesystem::path p(m_includingFile);
			p = p.remove_filename();
			p += pFilename;
			//first check if the file exists relative to the shader. if not, use default search (from workplace dir)
			if (std::filesystem::exists(p))
			{
				str = p.c_str();
			}
			else
			{
				str = pFilename;
			}


			return m_actualHandler->LoadSource(str, ppIncludeSource);

		};

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject)
		{
			return m_actualHandler->QueryInterface(riid, ppvObject);
		}

		virtual ULONG STDMETHODCALLTYPE AddRef(void)
		{
			return m_refCount++;
		}

		virtual ULONG STDMETHODCALLTYPE Release(void)
		{
			--m_refCount;
			if (m_refCount == 0)
			{
				delete this;
			}
			return 0;
		}


	private:
		ULONG m_refCount;
		LPCWSTR m_includingFile;
		IDxcIncludeHandler* m_actualHandler;
	};

	RCPtr<IDxcBlob> compileFromFile(LPCWSTR profile, LPCWSTR dbgName, LPCWSTR entryPoint, LPCWSTR filePath, const DxcDefine* defines, UINT32 defineCount, const LPCWSTR* extraParameters, size_t extraParamsCount)
	{
		RCPtr<IDxcLibrary> dxcLibrary;
		RCPtr<IDxcCompiler2> dxcCompiler;
		RCPtr<IDxcIncludeHandler> defaultIncludeHandler;
		RCPtr<IDxcBlobEncoding> srcBlob;
		{
			HRESULT res = DxcCreateInstance(CLSID_DxcLibrary, __uuidof(dxcLibrary), dxcLibrary.asVoid());
			assert(SUCCEEDED(res));
			res = DxcCreateInstance(CLSID_DxcCompiler, __uuidof(dxcCompiler), dxcCompiler.asVoid());
			assert(SUCCEEDED(res));
			res = dxcLibrary->CreateIncludeHandler(&defaultIncludeHandler);
			assert(SUCCEEDED(res));
		}
		
		

		RCPtr<IncludeHandler> inclHandler = new IncludeHandler(filePath, defaultIncludeHandler);

		{
			HRESULT res = dxcLibrary->CreateBlobFromFile(filePath, nullptr, &srcBlob);
			if (FAILED(res))
			{
				_com_error err(res);
				LPCTSTR errMsg = err.ErrorMessage();
				YAPT_LOG_ERROR("Failed to load shader file: %s", errMsg);

				return nullptr;
			}
		}

		RCPtr<IDxcOperationResult> compilerOpResult;

		//compiler arguments (currently none)
		std::vector<LPCWSTR> compilerOptions;
		compilerOptions.push_back(L"/enable_unbounded_descriptor_tables");
		compilerOptions.push_back(L"/res_may_alias");

		for (size_t i = 0; i < extraParamsCount; ++i)
		{
			compilerOptions.push_back(extraParameters[i]);
		}
#ifdef PRESERVE_DEBUG_INFO
		compilerOptions.push_back(L"/Zi");
#endif
		HRESULT result = dxcCompiler->Compile(
			srcBlob.get(), dbgName,
			entryPoint, profile,
			compilerOptions.data(), static_cast<UINT32>(compilerOptions.size()),
			defines, defineCount,
			inclHandler.get(),
			&compilerOpResult
		);

		if (FAILED(result))
		{
			YAPT_LOG_ERROR("Shader Compilation failed");
			return nullptr;
		}

		compilerOpResult->GetStatus(&result);

		if (FAILED(result))
		{
			RCPtr<IDxcBlobEncoding> error;
			HRESULT res = (compilerOpResult->GetErrorBuffer(&error));
			assert(SUCCEEDED(res));
			YAPT_LOG_FATAL_ERROR_W_LENGTH(reinterpret_cast<char*>(error->GetBufferPointer()), error->GetBufferSize());
			return nullptr;
		}
		else
		{
			RCPtr<IDxcBlob> compiledShader;
			HRESULT res = compilerOpResult->GetResult(&compiledShader);
			assert(SUCCEEDED(res));
			assert(compiledShader->GetBufferSize() > 0);
			return compiledShader;
		}
	}

}