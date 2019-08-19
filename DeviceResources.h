	#pragma once
#include "pch.h"

using namespace winrt;

namespace DX 
{
	static const UINT c_frameCount = 3;		// ʹ�����ػ��塣

	class DeviceResources
	{
	public:
		DeviceResources(DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT);
		void SetSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel panel);
		void SetLogicalSize(Windows::Foundation::Size logicalSize);
		void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
		void SetDpi(float dpi);
		void SetCompositionScale(float compositionScaleX, float compositionScaleY);
		void ValidateDevice();
		void Present();
		void WaitForGpu();

		// ������Ŀ��Ĵ�С��������Ϊ��λ��
		Windows::Foundation::Size   GetOutputSize() const { return m_outputSize; }

		// ������Ŀ��Ĵ�С���� dip Ϊ��λ��
		Windows::Foundation::Size   GetLogicalSize() const { return m_logicalSize; }

		float                       GetDpi() const { return m_effectiveDpi; }
		bool                        IsDeviceRemoved() const { return m_deviceRemoved; }

		// D3D ��������
		ID3D12Device* GetD3DDevice() const { return m_d3dDevice.get(); }
		IDXGISwapChain3* GetSwapChain() const { return m_swapChain.get(); }
		ID3D12Resource* GetRenderTarget() const { return m_renderTargets[m_currentFrame].get(); }
		ID3D12Resource* GetDepthStencil() const { return m_depthStencil.get(); }
		ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.get(); }
		ID3D12CommandAllocator* GetCommandAllocator() const { return m_commandAllocators[m_currentFrame].get(); }
		DXGI_FORMAT                 GetBackBufferFormat() const { return m_backBufferFormat; }
		DXGI_FORMAT                 GetDepthBufferFormat() const { return m_depthBufferFormat; }
		D3D12_VIEWPORT              GetScreenViewport() const { return m_screenViewport; }
		DirectX::XMFLOAT4X4         GetOrientationTransform3D() const { return m_orientationTransform3D; }
		UINT                        GetCurrentFrameIndex() const { return m_currentFrame; }

		CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_currentFrame, m_rtvDescriptorSize);
		}
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		}
	private:
		void CreateDeviceIndependentResources();
		void CreateDeviceResources();
		void CreateWindowSizeDependentResources();
		void UpdateRenderTargetSize();
		Windows::Foundation::IAsyncAction SetSwapChainOnPanelAsync(IDXGISwapChain3* swapChain);
		void RemoveDevice();
		void MoveToNextFrame();
		DXGI_MODE_ROTATION ComputeDisplayRotation();
		void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter);
		UINT											m_currentFrame;

		winrt::com_ptr<ID3D12Device> m_d3dDevice;
		winrt::com_ptr<IDXGIFactory4> m_dxgiFactory;
		winrt::com_ptr<IDXGISwapChain3> m_swapChain;
		winrt::com_ptr<ID3D12Resource>			m_renderTargets[c_frameCount];
		winrt::com_ptr<ID3D12Resource>			m_depthStencil;
		winrt::com_ptr<ID3D12DescriptorHeap>	m_rtvHeap;
		winrt::com_ptr<ID3D12DescriptorHeap>	m_dsvHeap;
		UINT											m_rtvDescriptorSize;
		winrt::com_ptr<ID3D12CommandQueue>		m_commandQueue;
		winrt::com_ptr<ID3D12CommandAllocator>	m_commandAllocators[c_frameCount];
		DXGI_FORMAT m_backBufferFormat;
		DXGI_FORMAT m_depthBufferFormat;
		D3D12_VIEWPORT									m_screenViewport;
		bool											m_deviceRemoved;

		// CPU/GPU ͬ����
		winrt::com_ptr<ID3D12Fence>				m_fence;
		UINT64											m_fenceValues[c_frameCount];
		HANDLE											m_fenceEvent;

		// �Դ��ڵĻ������á�
		winrt::agile_ref<winrt::Windows::UI::Xaml::Controls::SwapChainPanel> m_swapChainPanel;

		// ������豸���ԡ�
		Windows::Foundation::Size						m_d3dRenderTargetSize;
		Windows::Foundation::Size						m_outputSize;
		Windows::Foundation::Size						m_logicalSize;
		Windows::Graphics::Display::DisplayOrientations	m_nativeOrientation;
		Windows::Graphics::Display::DisplayOrientations	m_currentOrientation;
		float											m_dpi;
		float											m_compositionScaleX;
		float											m_compositionScaleY;

		// ���ǽ���Ӧ�ô��ص� DPI����������Ӧ���Ƿ�֧�ָ߷ֱ�����Ļ��
		float											m_effectiveDpi;

		// ������ʾ�����ת����
		DirectX::XMFLOAT4X4								m_orientationTransform3D;
	};

}


