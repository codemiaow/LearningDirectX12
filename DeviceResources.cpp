#include "pch.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include <ppltasks.h>
#include <inspectable.h>
#include <windows.ui.xaml.media.dxinterop.h>

using namespace winrt;
using namespace DirectX;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;

namespace DisplayMetrics
{
	// �߷ֱ�����ʾ������Ҫ���� GPU �͵�ص�Դ�����֡�
	// ���磬�����������ʱ���߷ֱ��ʵ绰���ܻ����̵��ʹ��ʱ��
	// ��Ϸ������ȫ����Ȱ� 60 ֡/����ٶȳ��֡�
	// ������ƽ̨�����ι����ȫ����ȳ��ֵľ���
	// Ӧ���������ǡ�
	static const bool SupportHighResolutions = false;

	// ���ڶ��塰�߷ֱ��ʡ���ʾ��Ĭ����ֵ���������ֵ
	// ������Χ������ SupportHighResolutions Ϊ false�������ųߴ�
	// 50%��
	static const float DpiThreshold = 192.0f;		// 200% ��׼������ʾ��
	static const float WidthThreshold = 1920.0f;	// 1080p ��
	static const float HeightThreshold = 1080.0f;	// 1080p �ߡ�
};

// ���ڼ�����Ļ��ת�ĳ�����
namespace ScreenRotation
{
	//0 �� Z ��ת
	static const XMFLOAT4X4 Rotation0(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	// 90 �� Z ��ת
	static const XMFLOAT4X4 Rotation90(
		0.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	//  180 �� Z ��ת
	static const XMFLOAT4X4 Rotation180(
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	//270 �� Z ��ת
	static const XMFLOAT4X4 Rotation270(
		0.0f, -1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
};


// DeviceResources �Ĺ��캯����
DX::DeviceResources::DeviceResources(DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat) :
	m_currentFrame(0),
	m_screenViewport(),
	m_rtvDescriptorSize(0),
	m_fenceEvent(0),
	m_backBufferFormat(backBufferFormat),
	m_depthBufferFormat(depthBufferFormat),
	m_fenceValues{},
	m_d3dRenderTargetSize(),
	m_outputSize(),
	m_logicalSize(),
	m_nativeOrientation(DisplayOrientations::None),
	m_currentOrientation(DisplayOrientations::None),
	m_dpi(-1.0f),
	m_effectiveDpi(-1.0f),
	m_deviceRemoved(false)
{
	CreateDeviceIndependentResources();
	CreateDeviceResources();
}

void DX::DeviceResources::CreateDeviceResources()
{

#if defined(_DEBUG)
	// �����Ŀ���ڵ������ɽ׶Σ���ͨ�� SDK �����õ��ԡ�
	{
		winrt::com_ptr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(debugController), debugController.put_void())))
		{
			debugController->EnableDebugLayer();
		}
	}
#endif
	DX::ThrowIfFailed(CreateDXGIFactory1(__uuidof(m_dxgiFactory), m_dxgiFactory.put_void()));

	winrt::com_ptr<IDXGIAdapter1> adapter;
	GetHardwareAdapter(m_dxgiFactory.get(), adapter.put());

	// ���� Direct3D 12 API �豸����
	HRESULT hr = D3D12CreateDevice(
		adapter.get(),					// Ӳ����������
		D3D_FEATURE_LEVEL_11_0,			// ��Ӧ�ÿ���֧�ֵ���͹��ܼ���
		__uuidof(m_d3dDevice), m_d3dDevice.put_void()		// ���ش����� Direct3D �豸��
	);

#if defined(_DEBUG)
	if (FAILED(hr))
	{
		// �����ʼ��ʧ�ܣ�����˵� WARP �豸��
		// �й� WARP ����ϸ��Ϣ�������: 
		// http://go.microsoft.com/fwlink/?LinkId=286690

		winrt::com_ptr<IDXGIAdapter> warpAdapter;
		m_dxgiFactory->EnumWarpAdapter(__uuidof(warpAdapter), warpAdapter.put_void());

		DX::ThrowIfFailed(
			D3D12CreateDevice(
				warpAdapter.get(),
				D3D_FEATURE_LEVEL_11_0,
				__uuidof(m_d3dDevice), m_d3dDevice.put_void()
			)
		);
	}
#endif
	DX::ThrowIfFailed(hr);


	// ����������С�
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	DX::ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&queueDesc, __uuidof(m_commandQueue), m_commandQueue.put_void()));
	NAME_D3D12_OBJECT(m_commandQueue);

	//Ϊ��ȾĿ����ͼ�����ģ����ͼ������������
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = c_frameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DX::ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, __uuidof(m_rtvHeap), m_rtvHeap.put_void()));
	NAME_D3D12_OBJECT(m_rtvHeap);

	m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, __uuidof(m_dsvHeap), m_dsvHeap.put_void()));
	NAME_D3D12_OBJECT(m_dsvHeap);

	for (UINT n = 0; n < c_frameCount; n++)
	{
		DX::ThrowIfFailed(
			m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(m_commandAllocators[n]), m_commandAllocators[n].put_void())
		);
	}

	// ����ͬ������
	DX::ThrowIfFailed(m_d3dDevice->CreateFence(m_fenceValues[m_currentFrame], D3D12_FENCE_FLAG_NONE, __uuidof(m_fence), m_fence.put_void()));
	m_fenceValues[m_currentFrame]++;

	m_fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);

}




void DX::DeviceResources::GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter)
{
	winrt::com_ptr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, adapter.put()); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// ��Ҫѡ�������������������������
			continue;
		}

		// ����������Ƿ�֧�� Direct3D 12������Ҫ����
		// ��Ϊʵ���豸��
		if (SUCCEEDED(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = adapter.detach();
}

// Mark the device as removed and disassociate the DXGI swap chain from the XAML SwapChainPanel.
//���豸���Ϊ��ɾ��������DXGI��������XAML��SwapChainPanel�ؼ����������
void DX::DeviceResources::RemoveDevice()
{
	m_deviceRemoved = true;

	// There's currently no good way to coordinate when the reference to the DXGI swap chain
	// is removed from the panel. DeviceResources needs to be destroyed and re-created as
	// part of the device recovery process but if the SwapChainPanel is still holding a
	// reference to the DXGI swap chain, a new D3D12 device cannot be created.
	//���������ɾ����DXGI������������ʱ��Ŀǰû�кõ�Э������ʽ�� 
	//DeviceResources��Ϊ�豸�ָ����̵�һ������Ҫ���ٺ����´�����
	//�������SwapChainPanel��Ȼ���ж�DXGI�����������ã����ܴ����µ�D3D12�豸��
	SetSwapChainOnPanelAsync(nullptr);
}

// �ȴ������ GPU ������ɡ�
void DX::DeviceResources::WaitForGpu()
{
	// �ڶ����а����ź����
	DX::ThrowIfFailed(m_commandQueue->Signal(m_fence.get(), m_fenceValues[m_currentFrame]));

	// �ȴ���ԽΧ����
	DX::ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_currentFrame], m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	// �Ե�ǰ֡����Χ��ֵ��
	m_fenceValues[m_currentFrame]++;
}


// ȷ��������Ŀ��ĳߴ缰���Ƿ���С��
void DX::DeviceResources::UpdateRenderTargetSize()
{
	m_effectiveDpi = m_dpi;

	// Ϊ���ӳ��߷ֱ����豸�ϵĵ��ʹ��ʱ�䣬����ֵ���С�ĳ�����Ŀ��
	// ������ GPU ����ʾ���ʱ���������
	if (!DisplayMetrics::SupportHighResolutions && m_dpi > DisplayMetrics::DpiThreshold)
	{
		float width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_dpi);
		float height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_dpi);

		// ���豸������ʾʱ���߶ȴ��ڿ�ȡ��뽫
		// �ϴ�ά��������ֵ�Ƚϣ�������Сά��
		// ��߶���ֵ���бȽϡ�
		if (max(width, height) > DisplayMetrics::WidthThreshold && min(width, height) > DisplayMetrics::HeightThreshold)
		{
			// Ϊ������Ӧ�ã����Ǹ�������Ч DPI���߼���С���䡣
			m_effectiveDpi /= 2.0f;
		}
	}

	// �����Ҫ�ĳ�����Ŀ���С(������Ϊ��λ)��
	m_outputSize.Width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_effectiveDpi);
	m_outputSize.Height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_effectiveDpi);

	// ��ֹ������СΪ��� DirectX ���ݡ�
	m_outputSize.Width = max(m_outputSize.Width, 1);
	m_outputSize.Height = max(m_outputSize.Height, 1);
}



// ���˽�з�����DXGI������������XAML��SwapChainPanel�ؼ���
Windows::Foundation::IAsyncAction DX::DeviceResources::SetSwapChainOnPanelAsync(IDXGISwapChain3* swapChain)
{
	// ��UI�ı��ǣ���Ҫ����UI�̡߳�
	return m_swapChainPanel.get().Dispatcher().RunAsync(CoreDispatcherPriority::High, DispatchedHandler([=]()
		{
			// �õ�SwapChainPanel�ؼ��Ľӿڡ�Get backing native interface for SwapChainPanel.
			winrt::com_ptr<ISwapChainPanelNative> panelNative;
			DX::ThrowIfFailed(
				winrt::get_unknown(m_swapChainPanel.get())->QueryInterface(__uuidof(panelNative), panelNative.put_void())
			);

			DX::ThrowIfFailed(panelNative->SetSwapChain(swapChain));
		}));
}

// �� SizeChanged �¼����¼���������е��ô˷�����
void DX::DeviceResources::SetLogicalSize(Windows::Foundation::Size logicalSize)
{
	if (m_logicalSize != logicalSize)
	{
		m_logicalSize = logicalSize;
		CreateWindowSizeDependentResources();
	}
}



// �� DpiChanged �¼����¼���������е��ô˷�����
void DX::DeviceResources::SetDpi(float dpi)
{
	if (dpi != m_dpi)
	{
		m_dpi = dpi;
		CreateWindowSizeDependentResources();
	}
}

// �� OrientationChanged �¼����¼���������е��ô˷�����
void DX::DeviceResources::SetCurrentOrientation(DisplayOrientations currentOrientation)
{
	if (m_currentOrientation != currentOrientation)
	{
		m_currentOrientation = currentOrientation;
		CreateWindowSizeDependentResources();
	}
}

//�� CompositionScaleChanged �¼����¼���������е��ô˷���
void DX::DeviceResources::SetCompositionScale(float compositionScaleX, float compositionScaleY)
{
	if (m_compositionScaleX != compositionScaleX ||
		m_compositionScaleY != compositionScaleY)
	{
		m_compositionScaleX = compositionScaleX;
		m_compositionScaleY = compositionScaleY;
		CreateWindowSizeDependentResources();
	}
}


// �� DisplayContentsInvalidated �¼����¼���������е��ô˷�����
void DX::DeviceResources::ValidateDevice()
{
	// ���Ĭ�����������ģ�D3D �豸��������Ч����Ϊ���豸
	// �Ѵ�������豸���Ƴ���

	// ���ȣ���ȡ���豸����������Ĭ����������Ϣ��

	DXGI_ADAPTER_DESC previousDesc;
	{
		winrt::com_ptr<IDXGIAdapter1> previousDefaultAdapter;
		DX::ThrowIfFailed(m_dxgiFactory->EnumAdapters1(0, previousDefaultAdapter.put()));

		DX::ThrowIfFailed(previousDefaultAdapter->GetDesc(&previousDesc));
	}

	// ����������ȡ��ǰĬ������������Ϣ��

	DXGI_ADAPTER_DESC currentDesc;
	{
		winrt::com_ptr<IDXGIFactory4> currentDxgiFactory;
		DX::ThrowIfFailed(CreateDXGIFactory1(__uuidof(currentDxgiFactory), currentDxgiFactory.put_void()));

		winrt::com_ptr<IDXGIAdapter1> currentDefaultAdapter;
		DX::ThrowIfFailed(currentDxgiFactory->EnumAdapters1(0, currentDefaultAdapter.put()));

		DX::ThrowIfFailed(currentDefaultAdapter->GetDesc(&currentDesc));
	}

	// ��������� LUID ��ƥ�䣬���߸��豸�������ѱ��Ƴ���
	// ����봴���µ� D3D �豸��

	if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart ||
		previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart ||
		FAILED(m_d3dDevice->GetDeviceRemovedReason()))
	{
		RemoveDevice();
	}
}



// ����������������ʾ����Ļ�ϡ�
void DX::DeviceResources::Present()
{
	// ��һ������ָʾ DXGI ������ֱֹ�� VSync����ʹӦ�ó���
	// ����һ�� VSync ǰ�������ߡ��⽫ȷ�����ǲ����˷��κ�������Ⱦ
	// �Ӳ�������Ļ����ʾ��֡��
	HRESULT hr = m_swapChain->Present(1, 0);

	// If the device was removed either by a disconnection or a driver upgrade, we 
	// must recreate all device resources.
	//���ͨ���������������������豸�Ͽ������豸��ʧ����������´��������豸��Դ
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		RemoveDevice();
	}
	else
	{
		DX::ThrowIfFailed(hr);

		MoveToNextFrame();
	}
}


// ׼��������һ֡��
void DX::DeviceResources::MoveToNextFrame()
{
	// �ڶ����а����ź����
	const UINT64 currentFenceValue = m_fenceValues[m_currentFrame];
	HRESULT hr = m_commandQueue->Signal(m_fence.get(), currentFenceValue);

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		RemoveDevice();
		return;
	}

	DX::ThrowIfFailed(hr);

	//���֡������
	m_currentFrame = m_swapChain->GetCurrentBackBufferIndex();

	//  �����һ֡�Ƿ�׼����������
	if (m_fence->GetCompletedValue() < m_fenceValues[m_currentFrame])
	{
		DX::ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_currentFrame], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	// Ϊ��һ֡����Χ��ֵ��
	m_fenceValues[m_currentFrame] = currentFenceValue + 1;
}

// ���ò������� Direct3D �豸����Դ��
void DX::DeviceResources::CreateDeviceIndependentResources()
{
}


// ����(�����´���) XAML �ؼ�ʱ���ô˷�����
void DX::DeviceResources::SetSwapChainPanel(SwapChainPanel panel)
{
	DisplayInformation currentDisplayInformation = DisplayInformation::GetForCurrentView();

	m_swapChainPanel = panel;
	m_logicalSize = Windows::Foundation::Size(static_cast<float>(panel.ActualWidth()), static_cast<float>(panel.ActualHeight()));
	m_nativeOrientation = currentDisplayInformation.NativeOrientation();
	m_currentOrientation = currentDisplayInformation.CurrentOrientation();
	m_compositionScaleX = panel.CompositionScaleX();
	m_compositionScaleY = panel.CompositionScaleY();
	m_dpi = currentDisplayInformation.LogicalDpi();

	CreateWindowSizeDependentResources();
}



void DX::DeviceResources::CreateWindowSizeDependentResources()
{

	// �ȵ���ǰ������ GPU ������ɡ�
	WaitForGpu();

	// ����ض�����һ���ڴ�С�����ݡ�
	for (UINT n = 0; n < c_frameCount; n++)
	{
		m_renderTargets[n] = nullptr;
		m_fenceValues[n] = m_fenceValues[m_currentFrame];
	}

	UpdateRenderTargetSize();

	// �������Ŀ�Ⱥ͸߶ȱ�����ڴ��ڵ�
	// ���򱾻��Ŀ�Ⱥ͸߶ȡ�������ڲ��ڱ���
	// ���������ʹ�ߴ練ת��
	DXGI_MODE_ROTATION displayRotation = ComputeDisplayRotation();

	bool swapDimensions = displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270;
	m_d3dRenderTargetSize.Width = swapDimensions ? m_outputSize.Height : m_outputSize.Width;
	m_d3dRenderTargetSize.Height = swapDimensions ? m_outputSize.Width : m_outputSize.Height;

	UINT backBufferWidth = lround(m_d3dRenderTargetSize.Width);
	UINT backBufferHeight = lround(m_d3dRenderTargetSize.Height);

	if (m_swapChain != nullptr)
	{
		// ����������Ѵ��ڣ���������С��
		HRESULT hr = m_swapChain->ResizeBuffers(c_frameCount, backBufferWidth, backBufferHeight, m_backBufferFormat, 0);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// ��������κ�ԭ���Ƴ����豸������Ҫ����һ���µ��豸�ͽ�������
			RemoveDevice();

			// �������ִ�д˷����������ٲ����´��� DeviceResources��
			return;
		}
		else
		{
			DX::ThrowIfFailed(hr);
		}
	}
	else
	{
		//����ʹ�������� Direct3D �豸��ͬ���������½�һ����
		DXGI_SCALING scaling = DisplayMetrics::SupportHighResolutions ? DXGI_SCALING_NONE : DXGI_SCALING_STRETCH;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

		swapChainDesc.Width = backBufferWidth;						// ƥ�䴰�ڵĴ�С��
		swapChainDesc.Height = backBufferHeight;
		swapChainDesc.Format = m_backBufferFormat;
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;							// �벻Ҫʹ�ö������
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = c_frameCount;					// ʹ�����ػ������̶ȵؼ�С�ӳ١�
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// ���� Windows ͨ��Ӧ�ö�����ʹ�� _FLIP_ SwapEffects
		swapChainDesc.Flags = 0;
		swapChainDesc.Scaling = scaling;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		winrt::com_ptr<IDXGISwapChain1> swapChain;
		DX::ThrowIfFailed(
			m_dxgiFactory->CreateSwapChainForComposition(
				m_commandQueue.get(),		//  ��������Ҫ�� DirectX 12 �е�������е����á�
				&swapChainDesc,
				nullptr,
				swapChain.put()
			)
		);
		swapChain.get()->QueryInterface(__uuidof(m_swapChain), m_swapChain.put_void());
		SetSwapChainOnPanelAsync(m_swapChain.get());
	}

	// Ϊ������������ȷ���򣬲�����
	// ת������Ⱦ����ת��������
	// ��ʽָ�� 3D ������Ա���������

	switch (displayRotation)
	{
	case DXGI_MODE_ROTATION_IDENTITY:
		m_orientationTransform3D = ScreenRotation::Rotation0;
		break;

	case DXGI_MODE_ROTATION_ROTATE90:
		m_orientationTransform3D = ScreenRotation::Rotation270;
		break;

	case DXGI_MODE_ROTATION_ROTATE180:
		m_orientationTransform3D = ScreenRotation::Rotation180;
		break;

	case DXGI_MODE_ROTATION_ROTATE270:
		m_orientationTransform3D = ScreenRotation::Rotation90;
		break;

	default:
		winrt::throw_hresult(hresult());
	}

	DX::ThrowIfFailed(
		m_swapChain->SetRotation(displayRotation)
	);

	// �ڽ����������÷�������
	DXGI_MATRIX_3X2_F inverseScale = {};
	float dpiScale = m_effectiveDpi / m_dpi;
	inverseScale._11 = 1.0f / (m_compositionScaleX * dpiScale);
	inverseScale._22 = 1.0f / (m_compositionScaleY * dpiScale);

	DX::ThrowIfFailed(
		m_swapChain->SetMatrixTransform(&inverseScale)
	);

	//������������̨�������ĳ���Ŀ����ͼ��
	{
		m_currentFrame = m_swapChain->GetCurrentBackBufferIndex();
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT n = 0; n < c_frameCount; n++)
		{
			DX::ThrowIfFailed(m_swapChain->GetBuffer(n, __uuidof(m_renderTargets[n]), m_renderTargets[n].put_void()));
			m_d3dDevice->CreateRenderTargetView(m_renderTargets[n].get(), nullptr, rtvDescriptor);
			rtvDescriptor.Offset(m_rtvDescriptorSize);

			WCHAR name[25];
			if (swprintf_s(name, L"m_renderTargets[%u]", n) > 0)
			{
				DX::SetName(m_renderTargets[n].get(), name);
			}
		}
	}

	// �������ģ����ͼ��
	{
		D3D12_HEAP_PROPERTIES depthHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		D3D12_RESOURCE_DESC depthResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_depthBufferFormat, backBufferWidth, backBufferHeight);
		depthResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		CD3DX12_CLEAR_VALUE depthOptimizedClearValue(m_depthBufferFormat, 1.0f, 0);

		ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
			&depthHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&depthResourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			__uuidof(m_depthStencil), m_depthStencil.put_void()
		));

		NAME_D3D12_OBJECT(m_depthStencil);

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = m_depthBufferFormat;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		m_d3dDevice->CreateDepthStencilView(m_depthStencil.get(), &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	//��������ȷ���������ڵ� 3D ��Ⱦ������
	m_screenViewport = { 0.0f, 0.0f, m_d3dRenderTargetSize.Width, m_d3dRenderTargetSize.Height, 0.0f, 1.0f };
}



// �˷���ȷ����������Ԫ��֮�����ת��ʽ: ��ʾ�豸�ı��������
// ��ǰ��ʾ����
DXGI_MODE_ROTATION DX::DeviceResources::ComputeDisplayRotation()
{
	DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

	// ע��: NativeOrientation ֻ��Ϊ "Landscape" �� "Portrait"����ʹ
	// DisplayOrientations ö�پ�������ֵ��
	switch (m_nativeOrientation)
	{
	case DisplayOrientations::Landscape:
		switch (m_currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;

		case DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;
		}
		break;

	case DisplayOrientations::Portrait:
		switch (m_currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;

		case DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;
		}
		break;
	}
	return rotation;
}