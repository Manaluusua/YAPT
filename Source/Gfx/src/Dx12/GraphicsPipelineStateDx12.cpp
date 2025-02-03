#include <Gfx/Dx12/GraphicsPipelineStateDx12.h>
#include <Gfx/Dx12/ResourceManagerDx12.h>
#include <Gfx/Dx12/YaptToDx12Conversions.h>

namespace YAPT
{
	GraphicsPipelineStateDx12::GraphicsPipelineStateDx12(ResourceManagerDx12& resMngr, const GraphicsPipelineStateDesc& desc)
		:m_resMngr(resMngr),
		m_layout(desc.pipelineLayout)

	{
		//assign without incrementing the counter
		*(&m_signature) = createRootSignature(resMngr.getDevice(), m_layout.get(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		assert(m_signature.get() != nullptr);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsStateDesc;

		graphicsStateDesc.pRootSignature = m_signature.get();


		fillGraphicsPipelineStateShaderStages(desc.shaderStages, desc.numberOfShaderStages, graphicsStateDesc);
		fillStreamOutputDesc(graphicsStateDesc.StreamOutput);
		fillBlendDesc(*desc.blendStateDescription, desc.multisampleState->enableAlphaToCoverage, graphicsStateDesc.BlendState);
		fillRasterizerDesc(*desc.rasterizerStateDescription, desc.multisampleState->sampleCount != SampleCount::SAMPLE_COUNT_1, graphicsStateDesc.RasterizerState);
		fillDepthStencilDesc(*desc.depthStencilState, graphicsStateDesc.DepthStencilState);

		graphicsStateDesc.NumRenderTargets = desc.renderPass->renderTargetCount;
		for (size_t i = 0; i < 8; ++i)
		{
			if (i < graphicsStateDesc.NumRenderTargets)
			{
				graphicsStateDesc.RTVFormats[i] = desc.renderPass->rtvFormats[i];
			}
			else
			{
				graphicsStateDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
			}
			
		}
		graphicsStateDesc.DSVFormat = desc.renderPass->dsvFormat;

		graphicsStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		graphicsStateDesc.SampleMask = (UINT)desc.multisampleState->sampleMask;
		graphicsStateDesc.SampleDesc.Count = (UINT)std::pow(2.f, (float)desc.multisampleState->sampleCount);
		graphicsStateDesc.SampleDesc.Quality = (UINT)desc.multisampleState->minSampleShading;
		graphicsStateDesc.PrimitiveTopologyType = yaptToDx12PrimitiveTopologyType(desc.primitivetopology);
		graphicsStateDesc.NodeMask = 0;
		graphicsStateDesc.CachedPSO.CachedBlobSizeInBytes = 0;
		graphicsStateDesc.CachedPSO.pCachedBlob = nullptr;
		graphicsStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		//vp&scissors
		m_viewPorts.resize(desc.numberOfViewportsAndScissors);
		m_scissors.resize(desc.numberOfViewportsAndScissors);
		for (size_t i = 0; i < desc.numberOfViewportsAndScissors; ++i)
		{
			D3D12_VIEWPORT& destVp = m_viewPorts[i];
			D3D12_RECT& destScissors = m_scissors[i];

			const ViewPort& srcVp = desc.viewports[i];
			const ScissorRect& srcScissors = desc.scissors[i];

			destVp.Width = srcVp.width;
			destVp.Height = srcVp.height;
			destVp.TopLeftX = srcVp.x;
			destVp.TopLeftY = srcVp.y;
			destVp.MinDepth = srcVp.minDepth;
			destVp.MaxDepth = srcVp.maxDepth;

			destScissors.left = srcScissors.x;
			destScissors.right = srcScissors.x + srcScissors.width;
			destScissors.top = srcScissors.y;
			destScissors.bottom = srcScissors.y + srcScissors.height;
		}

		//input layout
		size_t totalNumberOfVertexAttributes = 0;
		size_t currentVertexAttributeElementDescIndex = 0;
		
		m_perVertexBufferStrides.resize(desc.numberOfVertexBufferLayouts);
		for (size_t i = 0; i < desc.numberOfVertexBufferLayouts; ++i)
		{
			totalNumberOfVertexAttributes += desc.vertexBufferLayouts[i].numberOfVertexAttributes;
			m_perVertexBufferStrides[i] = desc.vertexBufferLayouts[i].stride;
		}
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
		inputElementDescs.resize(totalNumberOfVertexAttributes);

		for (size_t vertexBufferIndex = 0; vertexBufferIndex < desc.numberOfVertexBufferLayouts; ++vertexBufferIndex)
		{
			const VertexBufferDefinition& vertexBufferDef = desc.vertexBufferLayouts[vertexBufferIndex];
			for (size_t i = 0; i < vertexBufferDef.numberOfVertexAttributes; ++i)
			{
				const VertexInputAttribute& attrib = vertexBufferDef.attributes[i];
				D3D12_INPUT_ELEMENT_DESC& elementDesc = inputElementDescs[currentVertexAttributeElementDescIndex];

				const char* semanticName = getStringFromAttributeSemanticName(attrib.shaderInputSlot.getType());
				assert(semanticName != nullptr);

				elementDesc.InputSlot = (UINT)vertexBufferIndex;
				elementDesc.SemanticName = semanticName;
				elementDesc.SemanticIndex = attrib.shaderInputSlot.getIndex();
				elementDesc.InstanceDataStepRate = vertexBufferDef.vertexInputRate;
				elementDesc.Format = yaptToDx12Format(attrib.format);
				elementDesc.AlignedByteOffset = attrib.perVertexOffset;
				elementDesc.InputSlotClass = vertexBufferDef.vertexInputRate == 0 ? D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA : D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;

				++currentVertexAttributeElementDescIndex;

			}
		}
		graphicsStateDesc.InputLayout.NumElements = (UINT)inputElementDescs.size();
		graphicsStateDesc.InputLayout.pInputElementDescs = inputElementDescs.data();


		ID3D12PipelineState* state;
		checkForDxError(resMngr.getDevice().CreateGraphicsPipelineState(&graphicsStateDesc, IID_PPV_ARGS(&state)));
		*(&m_pipelineState) = state;

		m_topology = yaptToDx12PrimitiveTopology(desc.primitivetopology, desc.tesselationPatchControlPointCount);
	}
	GraphicsPipelineStateDx12::~GraphicsPipelineStateDx12()
	{

	}

	void GraphicsPipelineStateDx12::bind(CommandBufferHandle cmdBuffer)
	{
		cmdBuffer->cmdList->SetPipelineState(m_pipelineState.get());
		cmdBuffer->cmdList->SetGraphicsRootSignature(m_signature.get());
		cmdBuffer->cmdList->IASetPrimitiveTopology(m_topology);
		cmdBuffer->cmdList->RSSetScissorRects((UINT)m_scissors.size() , m_scissors.data());
		cmdBuffer->cmdList->RSSetViewports((UINT)m_viewPorts.size(), m_viewPorts.data());
		cmdBuffer->boundLayout = m_layout.get();

		cmdBuffer->boundPSO = this;
		cmdBuffer->boundPsoType = CommandBufferHandleDx12::Graphics;
	}
}