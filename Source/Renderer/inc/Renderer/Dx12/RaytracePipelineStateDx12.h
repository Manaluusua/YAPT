#pragma once
#include <Renderer/Shared/GfxTypes.h>

namespace YAPT
{

	class ResourceManagerDx12;
	class RaytracePipelineStateDx12
	{
	public:
		struct ShaderRecordInfo
		{
			void* shaderId;
		};


		RaytracePipelineStateDx12(ResourceManagerDx12& resMngr, const RaytracePipelineStateDesc& desc);
		~RaytracePipelineStateDx12();

		void bind(CommandBufferHandle cmdBuffer);

		const ShaderRecordInfo& getMissShaderRecordInfo(size_t index) const { return m_missShaderIdentifiers[index]; }
		const ShaderRecordInfo& getRayGenShaderRecordInfo(size_t index) const { return m_rayGenShaderIdentifiers[index]; }
		const ShaderRecordInfo& getHitGroupShaderRecordInfo(size_t index) const { return m_hitGroupShaderIdentifiers[index]; }

		size_t getMissShaderMaxLocalSignatureSizeInBytes() const { return m_missShaderLocalRootConstantsSizeInBytes; }
		size_t getRayGenhaderMaxLocalSignatureSizeInBytes() const { return m_rayGenShaderLocalRootConstantsSizeInBytes; }
		size_t getHitGroupMaxLocalSignatureSizeInBytes() const { return m_hitGroupShaderLocalRootConstantsSizeInBytes; }

		static size_t calculateAlignedShaderRecordSize(size_t localRootSigSizeInBytes);
		constexpr size_t getShaderIdentifierSize() { return D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; };

	private:

		

		void basicValidityCheck(const RaytracePipelineStateDesc& desc);

		void initializeRootSignatures(size_t rayHitConstantSizeInBytes, size_t missConstantSizeInBytes, size_t rayGenConstantSizeInBytes, PipelineLayoutDx12* globalLayout);
		void createPso(const RaytracePipelineStateDesc& desc);

		size_t m_missShaderLocalRootConstantsSizeInBytes;
		size_t m_rayGenShaderLocalRootConstantsSizeInBytes;
		size_t m_hitGroupShaderLocalRootConstantsSizeInBytes;

		RCPtr<ID3D12RootSignature> m_hitGroupLocalSignature;
		RCPtr<ID3D12RootSignature> m_rayGenLocalSignature;
		RCPtr<ID3D12RootSignature> m_missLocalSignature;

		std::vector<ShaderRecordInfo> m_hitGroupShaderIdentifiers;
		std::vector<ShaderRecordInfo> m_missShaderIdentifiers;
		std::vector<ShaderRecordInfo> m_rayGenShaderIdentifiers;

		RCPtr<ID3D12RootSignature> m_globalRootSignature;
		RCPtr<PipelineLayoutDx12> m_globalLayout;

		RCPtr<ID3D12StateObject> m_pipelineState;
		RCPtr<ID3D12StateObjectProperties> m_pipelineStateProps;
		

		ResourceManagerDx12& m_resMngr;
	};

}