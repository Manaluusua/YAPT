#include <Renderer/Dx12/RaytracePipelineStateDx12.h>
#include <Renderer/Dx12/ResourceManagerDx12.h>
#include <Renderer/Dx12/PipelineStateUtilityDx12.h>
#include <Renderer/Dx12/Dx12MiscUtils.h>


#define SHADERTABLE_CONSTANT_SPACE 0
#define SHADERTABLE_CONSTANT_BINDING_INDEX 0

#ifdef ENABLE_DEBUG_UTILITIES_DX12
	#define DEBUG_PRINT_STATEOBJECTSDESC
#endif

//taken from D3D12 raytrace examples
#ifdef DEBUG_PRINT_STATEOBJECTSDESC
#include <sstream>
#include <iomanip>

inline void PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* desc)
{
	std::wstringstream wstr;
	wstr << L"\n";
	wstr << L"--------------------------------------------------------------------\n";
	wstr << L"| D3D12 State Object 0x" << static_cast<const void*>(desc) << L": ";
	if (desc->Type == D3D12_STATE_OBJECT_TYPE_COLLECTION) wstr << L"Collection\n";
	if (desc->Type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE) wstr << L"Raytracing Pipeline\n";

	auto ExportTree = [](UINT depth, UINT numExports, const D3D12_EXPORT_DESC* exports)
	{
		std::wostringstream woss;
		for (UINT i = 0; i < numExports; i++)
		{
			woss << L"|";
			if (depth > 0)
			{
				for (UINT j = 0; j < 2 * depth - 1; j++) woss << L" ";
			}
			woss << L" [" << i << L"]: ";
			if (exports[i].ExportToRename) woss << exports[i].ExportToRename << L" --> ";
			woss << exports[i].Name << L"\n";
		}
		return woss.str();
	};

	for (UINT i = 0; i < desc->NumSubobjects; i++)
	{
		wstr << L"| [" << i << L"]: ";
		switch (desc->pSubobjects[i].Type)
		{
		case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
			wstr << L"Global Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
			break;
		case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
			wstr << L"Local Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
			break;
		case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
			wstr << L"Node Mask: 0x" << std::hex << std::setfill(L'0') << std::setw(8) << *static_cast<const UINT*>(desc->pSubobjects[i].pDesc) << std::setw(0) << std::dec << L"\n";
			break;
		case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY:
		{
			wstr << L"DXIL Library 0x";
			auto lib = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[i].pDesc);
			wstr << lib->DXILLibrary.pShaderBytecode << L", " << lib->DXILLibrary.BytecodeLength << L" bytes\n";
			wstr << ExportTree(1, lib->NumExports, lib->pExports);
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION:
		{
			wstr << L"Existing Library 0x";
			auto collection = static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(desc->pSubobjects[i].pDesc);
			wstr << collection->pExistingCollection << L"\n";
			wstr << ExportTree(1, collection->NumExports, collection->pExports);
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
		{
			wstr << L"Subobject to Exports Association (Subobject [";
			auto association = static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
			UINT index = static_cast<UINT>(association->pSubobjectToAssociate - desc->pSubobjects);
			wstr << index << L"])\n";
			for (UINT j = 0; j < association->NumExports; j++)
			{
				wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
			}
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
		{
			wstr << L"DXIL Subobjects to Exports Association (";
			auto association = static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
			wstr << association->SubobjectToAssociate << L")\n";
			for (UINT j = 0; j < association->NumExports; j++)
			{
				wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
			}
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG:
		{
			wstr << L"Raytracing Shader Config\n";
			auto config = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(desc->pSubobjects[i].pDesc);
			wstr << L"|  [0]: Max Payload Size: " << config->MaxPayloadSizeInBytes << L" bytes\n";
			wstr << L"|  [1]: Max Attribute Size: " << config->MaxAttributeSizeInBytes << L" bytes\n";
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG:
		{
			wstr << L"Raytracing Pipeline Config\n";
			auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(desc->pSubobjects[i].pDesc);
			wstr << L"|  [0]: Max Recursion Depth: " << config->MaxTraceRecursionDepth << L"\n";
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP:
		{
			wstr << L"Hit Group (";
			auto hitGroup = static_cast<const D3D12_HIT_GROUP_DESC*>(desc->pSubobjects[i].pDesc);
			wstr << (hitGroup->HitGroupExport ? hitGroup->HitGroupExport : L"[none]") << L")\n";
			wstr << L"|  [0]: Any Hit Import: " << (hitGroup->AnyHitShaderImport ? hitGroup->AnyHitShaderImport : L"[none]") << L"\n";
			wstr << L"|  [1]: Closest Hit Import: " << (hitGroup->ClosestHitShaderImport ? hitGroup->ClosestHitShaderImport : L"[none]") << L"\n";
			wstr << L"|  [2]: Intersection Import: " << (hitGroup->IntersectionShaderImport ? hitGroup->IntersectionShaderImport : L"[none]") << L"\n";
			break;
		}
		}
		wstr << L"|--------------------------------------------------------------------\n";
	}
	wstr << L"\n";
	OutputDebugStringW(wstr.str().c_str());
}
#endif

namespace YAPT
{
	RaytracePipelineStateDx12::RaytracePipelineStateDx12(ResourceManagerDx12& resMngr, const RaytracePipelineStateDesc& desc)
		:m_resMngr(resMngr),
		m_missShaderLocalRootConstantsSizeInBytes(0),
		m_rayGenShaderLocalRootConstantsSizeInBytes(0),
		m_hitGroupShaderLocalRootConstantsSizeInBytes(0)
	{
		basicValidityCheck(desc);

		initializeRootSignatures(desc.hitGroupShaderTableConstantsSizeInBytes, desc.missShaderTableConstantsSizeInBytes, desc.rayGenShaderTableConstantsSizeInBytes, desc.layout);
		createPso(desc);
	}
	RaytracePipelineStateDx12::~RaytracePipelineStateDx12()
	{
	}

	void RaytracePipelineStateDx12::createPso(const RaytracePipelineStateDesc& desc)
	{
		
		struct ShaderLibraryWithExports
		{
			D3D12_DXIL_LIBRARY_DESC desc;
			D3D12_EXPORT_DESC exportDesc;
			std::wstring exportName;
		};

		struct HitGroupDef
		{
			D3D12_HIT_GROUP_DESC desc;
			std::wstring name;
		};

		struct SubObjectAssociation
		{
			std::vector<const WCHAR*> exportNamesArray;
			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION associationSruct;
		};

		std::vector<ShaderLibraryWithExports> shaderLibraryDescs;
		shaderLibraryDescs.resize(desc.numberOfShaders);

		std::vector<HitGroupDef> hitGroups;
		hitGroups.resize(desc.numberOfHitGroupDescription);

		std::vector<D3D12_RAYTRACING_SHADER_CONFIG> shaderConfigs;
		shaderConfigs.resize(desc.numberOfConfigs);

		std::vector<SubObjectAssociation> subObjectAssociations;

		size_t numberOfLocalRootsignatures = 0;

		size_t hitGroupLocalSignatureOffset = numberOfLocalRootsignatures;
		if (m_hitGroupLocalSignature != nullptr) ++numberOfLocalRootsignatures;
		
		size_t rayGenLocalSignatureOffset = numberOfLocalRootsignatures;
		if (m_rayGenLocalSignature != nullptr) ++numberOfLocalRootsignatures;

		size_t missLocalSignatureOffset = numberOfLocalRootsignatures;
		if (m_missLocalSignature != nullptr) ++numberOfLocalRootsignatures;
		



		D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;

		//library descs
		for (size_t i = 0; i < shaderLibraryDescs.size(); ++i)
		{
			const ShaderStageCreateInfo& shdInfo = desc.shaders[i];
			ShaderLibraryWithExports& libraryDesc = shaderLibraryDescs[i];

			stringToWString(shdInfo.entryPoint, libraryDesc.exportName);
			libraryDesc.exportDesc.Name = libraryDesc.exportName.c_str();
			libraryDesc.exportDesc.ExportToRename = nullptr;
			libraryDesc.exportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

			//for now, only one export per shader blob
			libraryDesc.desc.NumExports = 1;
			libraryDesc.desc.pExports = &libraryDesc.exportDesc;
			libraryDesc.desc.DXILLibrary.pShaderBytecode = shdInfo.shaderModule->shaderBlob->GetBufferPointer();
			libraryDesc.desc.DXILLibrary.BytecodeLength = shdInfo.shaderModule->shaderBlob->GetBufferSize();
		}

		//hit groups
		for (size_t i = 0; i < hitGroups.size(); ++i)
		{
			const RayHitGroupDescription& hitGroupDesc = desc.hitGroupDescriptions[i];
			HitGroupDef& hitGroupDef = hitGroups[i];

			stringToWString(hitGroupDesc.hitGroupName, hitGroupDef.name);
			hitGroupDef.desc.HitGroupExport = hitGroupDef.name.c_str();

			if (hitGroupDesc.closestHitShaderIndex != YAPT_NULL_INDEX)
			{
				hitGroupDef.desc.ClosestHitShaderImport = shaderLibraryDescs[hitGroupDesc.closestHitShaderIndex].exportName.c_str();
			}
			else
			{
				hitGroupDef.desc.ClosestHitShaderImport = nullptr;
			}

			if (hitGroupDesc.anyHitShaderIndex != YAPT_NULL_INDEX)
			{
				hitGroupDef.desc.AnyHitShaderImport = shaderLibraryDescs[hitGroupDesc.anyHitShaderIndex].exportName.c_str();
			}
			else
			{
				hitGroupDef.desc.AnyHitShaderImport = nullptr;
			}

			if (hitGroupDesc.intersectionShaderIndex != YAPT_NULL_INDEX)
			{
				hitGroupDef.desc.IntersectionShaderImport = shaderLibraryDescs[hitGroupDesc.intersectionShaderIndex].exportName.c_str();
			}
			else
			{
				hitGroupDef.desc.IntersectionShaderImport = nullptr;
			}

			hitGroupDef.desc.Type = hitGroupDesc.hitGroupType == HitGroupType::TRIANGLE ? D3D12_HIT_GROUP_TYPE_TRIANGLES : D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;

		}

		//shader configs
		for (size_t i = 0; i < shaderConfigs.size(); ++i)
		{
			shaderConfigs[i].MaxAttributeSizeInBytes = (UINT)desc.configs[i].maxAttributeSizeInBytes;
			shaderConfigs[i].MaxPayloadSizeInBytes = (UINT)desc.configs[i].maxPayloadSizeInBytes;
		}

		//pipeline config
		pipelineConfig.MaxTraceRecursionDepth = min((UINT)desc.maxTraceRecursionDepth, (UINT)D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH);

		//calculate number of associations
		size_t numberOfAssociations = 0;
		numberOfAssociations += desc.numberOfConfigs;
		numberOfAssociations += numberOfLocalRootsignatures;

		subObjectAssociations.resize(numberOfAssociations);
		size_t configAssociationOffset = 0;
		size_t rootSigAssociationOffset = desc.numberOfConfigs;

		//go through all the rt shader entries and fill associations (shader entry point to root sig and config)
		for (size_t i = 0; i < desc.numberOfHitGroupDescription; ++i)
		{
			const RayHitGroupDescription& hitGroupDesc = desc.hitGroupDescriptions[i];
			const HitGroupDef& hitGroupDef = hitGroups[i];
			size_t associationEntryIndexConfig = configAssociationOffset + hitGroupDesc.configIndex;

			assert(associationEntryIndexConfig < subObjectAssociations.size());

			subObjectAssociations[associationEntryIndexConfig].exportNamesArray.push_back(hitGroupDef.name.c_str());
			if (m_hitGroupLocalSignature != nullptr)
			{
				subObjectAssociations[rootSigAssociationOffset + hitGroupLocalSignatureOffset].exportNamesArray.push_back(hitGroupDef.name.c_str());
			}
			
		}

		for (size_t i = 0; i < desc.numberOfRayGenerationDescription; ++i)
		{
			const RayGenerationDescription& rayGenDesc = desc.rayGenerationDescriptions[i];
			const ShaderLibraryWithExports& shaderLibraryEntry = shaderLibraryDescs[rayGenDesc.shaderIndex];

			size_t associationEntryIndexConfig = configAssociationOffset + rayGenDesc.configIndex;

			assert(associationEntryIndexConfig < subObjectAssociations.size());

			subObjectAssociations[associationEntryIndexConfig].exportNamesArray.push_back(shaderLibraryEntry.exportName.c_str());
			if (m_rayGenLocalSignature != nullptr)
			{
				subObjectAssociations[rootSigAssociationOffset + rayGenLocalSignatureOffset].exportNamesArray.push_back(shaderLibraryEntry.exportName.c_str());
			}
			
		}

		for (size_t i = 0; i < desc.numberOfRayMissDescription; ++i)
		{
			const RayMissDescription& missDesc = desc.rayMissDescriptions[i];
			const ShaderLibraryWithExports& shaderLibraryEntry = shaderLibraryDescs[missDesc.shaderIndex];

			size_t associationEntryIndexConfig = configAssociationOffset + missDesc.configIndex;
			
			assert(associationEntryIndexConfig < subObjectAssociations.size());

			subObjectAssociations[associationEntryIndexConfig].exportNamesArray.push_back(shaderLibraryEntry.exportName.c_str());
			if (m_missLocalSignature != nullptr)
			{
				subObjectAssociations[rootSigAssociationOffset + missLocalSignatureOffset].exportNamesArray.push_back(shaderLibraryEntry.exportName.c_str());
			}
		}

		for (size_t i = 0; i < subObjectAssociations.size(); ++i)
		{
			subObjectAssociations[i].associationSruct.NumExports = (UINT)subObjectAssociations[i].exportNamesArray.size();
			subObjectAssociations[i].associationSruct.pExports = subObjectAssociations[i].exportNamesArray.data();
		}

		//fill the subobjects
		size_t totalNumberOfSubObjects = 0;
		totalNumberOfSubObjects += shaderLibraryDescs.size();
		totalNumberOfSubObjects += hitGroups.size();

		size_t shaderConfigSubobjectsOffset = totalNumberOfSubObjects; //remember the offset to properly setup subobject association
		totalNumberOfSubObjects += shaderConfigs.size();
		totalNumberOfSubObjects += 1; //pipeline config

		size_t localRootSignatureSubobjectsOffset = totalNumberOfSubObjects; //remember the offset to properly setup subobject association
		totalNumberOfSubObjects += numberOfLocalRootsignatures;
		totalNumberOfSubObjects += 1; //global root sig
	
		totalNumberOfSubObjects += numberOfAssociations;

		std::vector<D3D12_STATE_SUBOBJECT> subObjects;
		subObjects.resize(totalNumberOfSubObjects);
		size_t currentSubObjectIndex = 0;

		for (size_t i = 0; i < shaderLibraryDescs.size(); ++i)
		{
			D3D12_STATE_SUBOBJECT& subObj = subObjects[currentSubObjectIndex++];
			subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
			subObj.pDesc = &shaderLibraryDescs[i].desc;
		}

		for (size_t i = 0; i < hitGroups.size(); ++i)
		{
			D3D12_STATE_SUBOBJECT& subObj = subObjects[currentSubObjectIndex++];
			subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
			subObj.pDesc = &hitGroups[i].desc;
		}

		for (size_t i = 0; i < shaderConfigs.size(); ++i)
		{
			D3D12_STATE_SUBOBJECT& subObj = subObjects[currentSubObjectIndex++];
			subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
			subObj.pDesc = &shaderConfigs[i];
		}

		{
			D3D12_STATE_SUBOBJECT& subObj = subObjects[currentSubObjectIndex++];
			subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
			subObj.pDesc = &pipelineConfig;
		}
		
		if(m_hitGroupLocalSignature != nullptr)
		{
			D3D12_STATE_SUBOBJECT& subObj = subObjects[currentSubObjectIndex++];
			subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
			subObj.pDesc = &m_hitGroupLocalSignature;
		}

		if(m_rayGenLocalSignature != nullptr)
		{
			D3D12_STATE_SUBOBJECT& subObj = subObjects[currentSubObjectIndex++];
			subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
			subObj.pDesc = &m_rayGenLocalSignature;
		}
		if(m_missLocalSignature)
		{
			D3D12_STATE_SUBOBJECT& subObj = subObjects[currentSubObjectIndex++];
			subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
			subObj.pDesc = &m_missLocalSignature;
		}

		if (m_globalRootSignature)
		{
			D3D12_STATE_SUBOBJECT& subObj = subObjects[currentSubObjectIndex++];
			subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
			subObj.pDesc = &m_globalRootSignature;
		}

		for (size_t i = 0; i < desc.numberOfConfigs; ++i)
		{
			D3D12_STATE_SUBOBJECT& subObj = subObjects[currentSubObjectIndex++];
			SubObjectAssociation& assoc = subObjectAssociations[i + configAssociationOffset];
			assoc.associationSruct.pSubobjectToAssociate = &subObjects[shaderConfigSubobjectsOffset + i];

			subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			subObj.pDesc = &assoc.associationSruct;
		}

		for (size_t i = 0; i < numberOfLocalRootsignatures; ++i)
		{
			D3D12_STATE_SUBOBJECT& subObj = subObjects[currentSubObjectIndex++];
			SubObjectAssociation& assoc = subObjectAssociations[i + rootSigAssociationOffset];
			assoc.associationSruct.pSubobjectToAssociate = &subObjects[localRootSignatureSubobjectsOffset];

			subObj.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			subObj.pDesc = &assoc.associationSruct;
		}

		assert(currentSubObjectIndex == totalNumberOfSubObjects);

		//create the pso
		D3D12_STATE_OBJECT_DESC finalDx12SubobjectDesc;
		finalDx12SubobjectDesc.NumSubobjects = (UINT)subObjects.size();
		finalDx12SubobjectDesc.pSubobjects = subObjects.data();
		finalDx12SubobjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

#ifdef DEBUG_PRINT_STATEOBJECTSDESC
		PrintStateObjectDesc(&finalDx12SubobjectDesc);
#endif

		ID3D12StateObject* state;
		checkForDxError(m_resMngr.getDevice().CreateStateObject(&finalDx12SubobjectDesc, IID_PPV_ARGS(&state)));
		*(&m_pipelineState) = state;

		ID3D12StateObjectProperties* props;
		checkForDxError(state->QueryInterface(IID_PPV_ARGS(&props)));
		*(&m_pipelineStateProps) = props;

		//cache shader identifiers based on the index in the description for given type
		m_hitGroupShaderIdentifiers.resize(hitGroups.size());
		for (size_t i = 0; i < m_hitGroupShaderIdentifiers.size(); ++i)
		{
			void* shaderId = m_pipelineStateProps->GetShaderIdentifier(hitGroups[i].name.c_str());
			assert(shaderId != nullptr);
			m_hitGroupShaderIdentifiers[i].shaderId = shaderId;

		}

		m_missShaderIdentifiers.resize(desc.numberOfRayMissDescription);
		for (size_t i = 0; i < m_missShaderIdentifiers.size(); ++i)
		{
			size_t shdIndex = desc.rayMissDescriptions[i].shaderIndex;
			void* shaderId = m_pipelineStateProps->GetShaderIdentifier(shaderLibraryDescs[shdIndex].exportName.c_str());
			assert(shaderId != nullptr);
			m_missShaderIdentifiers[i].shaderId = shaderId;
			
		}

		m_rayGenShaderIdentifiers.resize(desc.numberOfRayGenerationDescription);
		for (size_t i = 0; i < m_rayGenShaderIdentifiers.size(); ++i)
		{
			size_t shdIndex = desc.rayGenerationDescriptions[i].shaderIndex;
			void* shaderId = m_pipelineStateProps->GetShaderIdentifier(shaderLibraryDescs[shdIndex].exportName.c_str());
			assert(shaderId != nullptr);
			m_rayGenShaderIdentifiers[i].shaderId = shaderId;
		}


		m_missShaderLocalRootConstantsSizeInBytes = desc.missShaderTableConstantsSizeInBytes;
		m_rayGenShaderLocalRootConstantsSizeInBytes = desc.rayGenShaderTableConstantsSizeInBytes;
		m_hitGroupShaderLocalRootConstantsSizeInBytes = desc.hitGroupShaderTableConstantsSizeInBytes;
		

	}

	void RaytracePipelineStateDx12::initializeRootSignatures(size_t rayHitConstantSizeInBytes, size_t missConstantSizeInBytes, size_t rayGenConstantSizeInBytes,  PipelineLayoutDx12* globalLayout)
	{
		if(rayHitConstantSizeInBytes > 0)
		{
			D3D12_ROOT_PARAMETER rootParam;

			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParam.Constants.Num32BitValues = (UINT)((rayHitConstantSizeInBytes + 3) / 4);
			rootParam.Constants.RegisterSpace = SHADERTABLE_CONSTANT_SPACE;
			rootParam.Constants.ShaderRegister = SHADERTABLE_CONSTANT_BINDING_INDEX;

			*(&m_hitGroupLocalSignature) = createRootSignature(m_resMngr.getDevice(), &rootParam, 1, nullptr, 0, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
		}
		
		if (missConstantSizeInBytes > 0)
		{
			D3D12_ROOT_PARAMETER rootParam;

			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParam.Constants.Num32BitValues = (UINT)((missConstantSizeInBytes + 3) / 4);
			rootParam.Constants.RegisterSpace = SHADERTABLE_CONSTANT_SPACE;
			rootParam.Constants.ShaderRegister = SHADERTABLE_CONSTANT_BINDING_INDEX;

			*(&m_missLocalSignature) = createRootSignature(m_resMngr.getDevice(), &rootParam,  1, nullptr, 0, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
		}

		if(rayGenConstantSizeInBytes > 0)
		{
			D3D12_ROOT_PARAMETER rootParam;

			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParam.Constants.Num32BitValues = (UINT)((rayGenConstantSizeInBytes + 3)/4);
			rootParam.Constants.RegisterSpace = SHADERTABLE_CONSTANT_SPACE;
			rootParam.Constants.ShaderRegister = SHADERTABLE_CONSTANT_BINDING_INDEX;

			*(&m_rayGenLocalSignature) = createRootSignature(m_resMngr.getDevice(), &rootParam, 1, nullptr, 0, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

		}

		m_globalLayout = globalLayout;
		*(&m_globalRootSignature) = createRootSignature(m_resMngr.getDevice(), m_globalLayout.get(), D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

	}

	void RaytracePipelineStateDx12::basicValidityCheck(const RaytracePipelineStateDesc& desc)
	{
		assert(desc.maxTraceRecursionDepth <= D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH);
		for (size_t i = 0; i < desc.numberOfConfigs; ++i)
		{
			assert(desc.configs[i].maxAttributeSizeInBytes < D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES);
		}
	}


	void RaytracePipelineStateDx12::bind(CommandBufferHandle cmdBuffer)
	{
		cmdBuffer->cmdList->SetPipelineState1(m_pipelineState.get());
		cmdBuffer->cmdList->SetComputeRootSignature(m_globalRootSignature.get());
		cmdBuffer->boundLayout = m_globalLayout.get();
		cmdBuffer->boundPSO = this;
		cmdBuffer->boundPsoType = CommandBufferHandleDx12::Raytrace;
	}

	size_t RaytracePipelineStateDx12::calculateAlignedShaderRecordSize(size_t localRootSigSizeInBytes)
	{
		return align(localRootSigSizeInBytes + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	}
}