#pragma once

#include "pch.h"
#include "DeviceResources.h"
#include "ShaderStructures.h"
#include "StepTimer.h"

using namespace winrt;

// ��ʾ��������ʵ����һ��������Ⱦ�ܵ���
class Sample3DSceneRenderer
{
public:
	Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
	~Sample3DSceneRenderer();
	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();
	void Update(DX::StepTimer const& timer);
	bool Render();
	void SaveState();

	void StartTracking();
	void TrackingUpdate(float positionX);
	void StopTracking();
	bool IsTracking() { return m_tracking; }

private:
	void LoadState();
	void Rotate(float radians);

private:
	// ������������С���붼�� 256 �ֽڵ���������
	static const UINT c_alignedConstantBufferSize = (sizeof(DirectX12XAMLCube::ModelViewProjectionConstantBuffer) + 255) & ~255;

	// ������豸��Դָ�롣
	std::shared_ptr<DX::DeviceResources> m_deviceResources;

	// ���弸�ε� Direct3D ��Դ��
	winrt::com_ptr<ID3D12GraphicsCommandList>	m_commandList;
	winrt::com_ptr<ID3D12RootSignature>			m_rootSignature;
	winrt::com_ptr<ID3D12PipelineState>			m_pipelineState;
	winrt::com_ptr<ID3D12DescriptorHeap>		m_cbvHeap;
	winrt::com_ptr<ID3D12Resource>				m_vertexBuffer;
	winrt::com_ptr<ID3D12Resource>				m_indexBuffer;
	winrt::com_ptr<ID3D12Resource>				m_constantBuffer;
	DirectX12XAMLCube::ModelViewProjectionConstantBuffer					m_constantBufferData;
	UINT8* m_mappedConstantBuffer;
	UINT												m_cbvDescriptorSize;
	D3D12_RECT											m_scissorRect;
	std::vector<byte>									m_vertexShader;
	std::vector<byte>									m_pixelShader;
	D3D12_VERTEX_BUFFER_VIEW							m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW								m_indexBufferView;

	// ������Ⱦѭ���ı�����
	bool	m_loadingComplete;
	float	m_radiansPerSecond;
	float	m_angle;
	bool	m_tracking;
};


