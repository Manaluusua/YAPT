#include <Renderer/Dx12/ShaderUtilityDx12.h>
#include <Renderer/RendererCommonTypesUtility.h>
#include <d3d12shader.h>
#include <vector>
#include <Common/Logger.h>
#include <Common/CommonUtilities.h>
#include <Renderer/Shared/GfxTypes.h>
#include <filesystem>
#include <assert.h>
#include <comdef.h>
#include <unordered_map>

#define PRESERVE_DEBUG_INFO


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




	LPCWSTR shaderTypeToProfilePrefix(ShaderModuleType shdType)
	{
		switch (shdType)
		{
		case ShaderModuleType::VERTEX_MODULE:
			return L"vs_";
		case ShaderModuleType::FRAGMENT_MODULE:
			return L"ps_";
		case ShaderModuleType::HULL_MODULE:
			return L"hs_";
		case ShaderModuleType::DOMAIN_MODULE:
			return L"ds_";
		case ShaderModuleType::GEOMETRY_MODULE:
			return L"gs_";
		case ShaderModuleType::COMPUTE_MODULE:
			return L"cs_";
		case ShaderModuleType::LIBRARY_MODULE:
			return L"lib_";
		default:
			return L"lib_";
		}
	}

	void shaderTypeToProfile(ShaderModuleType shdType, std::wstring& profileOut)
	{
		profileOut = shaderTypeToProfilePrefix(shdType);
		profileOut += L"6_3";
	}

	void dx12ShaderInputTypeToResourceTypeAndAccessFlags(D3D12_SHADER_INPUT_BIND_DESC resourceDesc, DescriptorType& typeOut, AccessFlags& accessFlagsOut)
	{
		switch (resourceDesc.Type)
		{
		case D3D_SIT_CBUFFER:
		case D3D_SIT_TBUFFER:
			typeOut = DescriptorType::UNIFORM_BUFFER;
			accessFlagsOut = ACCESS_FLAGS_READ;
			break;
		case D3D_SIT_TEXTURE:
			if (resourceDesc.Dimension == D3D_SRV_DIMENSION_BUFFER)
			{
				typeOut = DescriptorType::UNIFORM_TEXEL_BUFFER;
				accessFlagsOut = ACCESS_FLAGS_READ;
			}
			else
			{
				typeOut = DescriptorType::TEXTURE;
				accessFlagsOut = ACCESS_FLAGS_READ;
			}
			break;
		case D3D_SIT_SAMPLER:
			typeOut = DescriptorType::SAMPLER;
			accessFlagsOut = ACCESS_FLAGS_READ;
			break;
		case D3D_SIT_STRUCTURED:
		case D3D_SIT_BYTEADDRESS:
			typeOut = DescriptorType::STORAGE_BUFFER;
			accessFlagsOut = ACCESS_FLAGS_READ;
			break;
		case D3D_SIT_UAV_RWTYPED:
			if (resourceDesc.Dimension == D3D_SRV_DIMENSION_BUFFER)
			{
				typeOut = DescriptorType::STORAGE_TEXEL_BUFFER;
				accessFlagsOut = ACCESS_FLAGS_READ_WRITE;
			}
			else
			{
				typeOut = DescriptorType::STORAGE_TEXTURE;
				accessFlagsOut = ACCESS_FLAGS_READ_WRITE;
			}
			break;
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:

			if (resourceDesc.Dimension == D3D_SRV_DIMENSION_BUFFER)
			{
				typeOut = DescriptorType::STORAGE_BUFFER;
				accessFlagsOut = ACCESS_FLAGS_READ_WRITE;
			}
			else
			{
				typeOut = DescriptorType::STORAGE_TEXTURE;
				accessFlagsOut = ACCESS_FLAGS_READ_WRITE;
			}
			break;

		case 12: //seems that acceleration structure is type 12
			assert(resourceDesc.Dimension == D3D_SRV_DIMENSION_UNKNOWN);
			typeOut = DescriptorType::ACCELERATION_STRUCTURE;
			accessFlagsOut = ACCESS_FLAGS_READ;
		}
		

	}
	 
	AttributeSemantic toAttributeSemantic(LPCSTR name, UINT index)
	{

		AttributeSemantic semantic(getAttributeSemanticNameFromString(name),index);
		return semantic;
	}


	void readReflectionDataInternal(ID3D12ShaderReflection* refl, ShaderReflectionDataDx12& reflOut)
	{	
		D3D12_SHADER_DESC shaderDesc;
		refl->GetDesc(&shaderDesc);
		 
		UINT boundResourcesCount = shaderDesc.BoundResources;
		 
		reflOut.sortedBindings.resize(boundResourcesCount);

		for (UINT boundResourceIndex = 0; boundResourceIndex < boundResourcesCount; ++boundResourceIndex)
		{
			D3D12_SHADER_INPUT_BIND_DESC resourceDesc;
			refl->GetResourceBindingDesc(boundResourceIndex, &resourceDesc);

			ShaderReflectionDataBindingDx12& binding = reflOut.sortedBindings[boundResourceIndex];

			binding.name = resourceDesc.Name;
			binding.bindPoint = resourceDesc.BindPoint;
			binding.bindCount = resourceDesc.BindCount;
			binding.spaceIndex = resourceDesc.Space;
			dx12ShaderInputTypeToResourceTypeAndAccessFlags(resourceDesc, binding.type, binding.accessFlags);
		}
		
		if (shaderDesc.InputParameters > 0)
		{
			reflOut.sortedInputData.resize(shaderDesc.InputParameters);

			for (UINT inputIndex = 0; inputIndex < shaderDesc.InputParameters; ++inputIndex)
			{
				D3D12_SIGNATURE_PARAMETER_DESC inputDesc;
				refl->GetInputParameterDesc(inputIndex, &inputDesc);

				ShaderReflectionDataVertexInputEntryDx12& inputData = reflOut.sortedInputData[inputIndex];

				inputData.semantic = toAttributeSemantic(inputDesc.SemanticName, inputDesc.SemanticIndex);
				inputData.index = inputDesc.Register;
			}
		}

		//sort
		std::sort(reflOut.sortedBindings.data(), reflOut.sortedBindings.data() + reflOut.sortedBindings.size(),
			[](const ShaderReflectionDataBindingDx12& a, const ShaderReflectionDataBindingDx12& b)
			{
				if (a.spaceIndex == b.spaceIndex)
				{
					return a.bindPoint < b.bindPoint;
				}
				else
				{
					return a.spaceIndex < b.spaceIndex;
				}
			}
		);

		std::sort(reflOut.sortedInputData.data(), reflOut.sortedInputData.data() + reflOut.sortedInputData.size(),
			[](const ShaderReflectionDataVertexInputEntryDx12& a, const ShaderReflectionDataVertexInputEntryDx12& b)
			{
				return a.index < b.index;
			}
		);

	}

	void readReflectionDataInternal(ID3D12LibraryReflection* refl, const char* entryPointName, ShaderReflectionDataDx12& reflOut)
	{
		D3D12_LIBRARY_DESC desc;
		refl->GetDesc(&desc);

		//for now just always lod the first
		assert(desc.FunctionCount == 1);

		for (size_t i = 0; i < min(desc.FunctionCount, 1u); ++i)
		{
			ID3D12FunctionReflection* funcRefl;
			funcRefl = refl->GetFunctionByIndex((INT)i);

			D3D12_FUNCTION_DESC funcDesc;
			funcRefl->GetDesc(&funcDesc);

			/*if (strcmp(funcDesc.Name, entryPointName) != 0)
			{
				continue;
			}*/

			size_t boundResourcesCount = funcDesc.BoundResources;
			reflOut.sortedBindings.reserve(boundResourcesCount);
			for (UINT boundResourceIndex = 0; boundResourceIndex < boundResourcesCount; ++boundResourceIndex)
			{
				D3D12_SHADER_INPUT_BIND_DESC resourceDesc;
				funcRefl->GetResourceBindingDesc(boundResourceIndex, &resourceDesc);

				if (resourceDesc.Space < HIGHEST_UNIQUE_DESCSET_INDEX)
				{
					ShaderReflectionDataBindingDx12 binding;
					binding.name = resourceDesc.Name;
					binding.bindPoint = resourceDesc.BindPoint;
					binding.bindCount = resourceDesc.BindCount;
					binding.spaceIndex = resourceDesc.Space;
					dx12ShaderInputTypeToResourceTypeAndAccessFlags(resourceDesc, binding.type, binding.accessFlags);
					reflOut.sortedBindings.push_back(binding);
				}
			}

			
			std::sort(reflOut.sortedBindings.data(), reflOut.sortedBindings.data() + reflOut.sortedBindings.size(),
				[](const ShaderReflectionDataBindingDx12& a, const ShaderReflectionDataBindingDx12& b)
				{
					if (a.spaceIndex == b.spaceIndex)
					{
						return a.bindPoint < b.bindPoint;
					}
					else
					{
						return a.spaceIndex < b.spaceIndex;
					}
				}
			);

		}

	}
	
	template<typename T>
	bool tryFetchReflectionDataFromContainer(IDxcContainerReflection* container, RCPtr<T>& out)
	{
		UINT32 numberOfParts;
		checkForDxError(container->GetPartCount(&numberOfParts));
		for (UINT32 partIndex = 0; partIndex < numberOfParts; ++partIndex)
		{
			//TODO: need dxc compiler source to directly get the DFCC_DXIL part(s). For now be lazy and try all the shoes if they fit!
			RCPtr<T> shaderReflection;
			if (container->GetPartReflection(partIndex, __uuidof(T), shaderReflection.asVoid()) == S_OK)
			{
				out = shaderReflection;
				return true;
			}

		}
		return false;
	}
	
	void readReflectionData(IDxcBlob* blob, const char* entryPointName, ShaderReflectionDataDx12& reflOut)
	{
		RCPtr<IDxcContainerReflection> reflContainer;
		checkForDxError(DxcCreateInstance(CLSID_DxcContainerReflection, __uuidof(reflContainer), reflContainer.asVoid()));
		checkForDxError(reflContainer->Load(blob));
		

		RCPtr<ID3D12ShaderReflection> shaderRefl;
		RCPtr<ID3D12LibraryReflection> libraryRefl;
		if (tryFetchReflectionDataFromContainer(reflContainer.get(), shaderRefl))
		{
			readReflectionDataInternal(shaderRefl.get(), reflOut);
		}
		else if(tryFetchReflectionDataFromContainer(reflContainer.get(), libraryRefl))
		{
			readReflectionDataInternal(libraryRefl.get(), entryPointName, reflOut);
		}

		
	}

	RCPtr<IDxcBlob> compileFromFile(LPCWSTR profile, LPCWSTR dbgName, LPCWSTR entryPoint, LPCWSTR filePath, const DxcDefine* defines, UINT32 defineCount)
	{
		RCPtr<IDxcLibrary> dxcLibrary;
		RCPtr<IDxcCompiler2> dxcCompiler;
		RCPtr<IDxcIncludeHandler> defaultIncludeHandler;
		RCPtr<IDxcBlobEncoding> srcBlob;
		 
		checkForDxError(DxcCreateInstance(CLSID_DxcLibrary, __uuidof(dxcLibrary), dxcLibrary.asVoid()));
		checkForDxError(DxcCreateInstance(CLSID_DxcCompiler, __uuidof(dxcCompiler), dxcCompiler.asVoid()));
		checkForDxError(dxcLibrary->CreateIncludeHandler(&defaultIncludeHandler));

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
			checkForDxError(compilerOpResult->GetErrorBuffer(&error));
			YAPT_LOG_FATAL_ERROR_W_LENGTH(reinterpret_cast<char*>(error->GetBufferPointer()), error->GetBufferSize());
			return nullptr;
		} 
		else
		{
			RCPtr<IDxcBlob> compiledShader;
			checkForDxError(compilerOpResult->GetResult(&compiledShader));
			assert(compiledShader->GetBufferSize() > 0);
			return compiledShader;
		}
	}
}