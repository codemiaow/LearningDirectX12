#include "pch.h"
#include "DeviceResources.h"
#include "MainPage.h"
#include "MainPage.g.cpp"
#include <dxgidebug.h>

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Graphics::Display;

namespace winrt::BlankApp::implementation
{
 
	MainPage::MainPage(){
		InitializeComponent();

		Windows::UI::Core::CoreWindow window = Window::Current().CoreWindow();
		window.VisibilityChanged({ this, &MainPage::OnVisibilityChanged });

		DisplayInformation currentDisplayInformation = DisplayInformation::GetForCurrentView();

		currentDisplayInformation.DpiChanged({ this, &MainPage::OnDpiChanged });

		currentDisplayInformation.OrientationChanged({ this, &MainPage::OnOrientationChanged });

		DisplayInformation::DisplayContentsInvalidated({ this, &MainPage::OnDisplayContentsInvalidated });

		m_swapChainPanel().CompositionScaleChanged({ this, &MainPage::OnCompositionScaleChanged });

		m_swapChainPanel().SizeChanged(SizeChangedEventHandler(this, &MainPage::OnSwapChainPanelSizeChanged));

		// 注册我们的 SwapChainPanel 以获取独立的输入指针事件
		auto workItemHandler = Windows::System::Threading::WorkItemHandler([this](IAsyncAction)
			{
				// 对于指定的设备类型，无论它是在哪个线程上创建的，CoreIndependentInputSource 都将引发指针事件。
				m_coreInput = m_swapChainPanel().CreateCoreIndependentInputSource(
					Windows::UI::Core::CoreInputDeviceTypes::Mouse |
					Windows::UI::Core::CoreInputDeviceTypes::Touch |
					Windows::UI::Core::CoreInputDeviceTypes::Pen
				);

				// 指针事件的寄存器，将在后台线程上引发。
				m_coreInput.get().PointerPressed([this](IInspectable const& /* sender */, Windows::UI::Core::PointerEventArgs const& /* args */)
					{
						// 按下指针时开始跟踪指针移动。
						m_main->StartTracking();
					});
				m_coreInput.get().PointerMoved([this](IInspectable const& /* sender */, Windows::UI::Core::PointerEventArgs const&  args)
					{
						// 更新指针跟踪代码。
						if (m_main->IsTracking())
						{
							m_main->TrackingUpdate(args.CurrentPoint().Position().X);
						}
					});
				m_coreInput.get().PointerReleased([this](IInspectable const& /* sender */, Windows::UI::Core::PointerEventArgs	 const& /* args */)
					{
						m_main->StopTracking();
					});

				// 一旦发送输入消息，即开始处理它们。
				m_coreInput.get().Dispatcher().ProcessEvents(Windows::UI::Core::CoreProcessEventsOption::ProcessUntilQuit);
			});

		// 在高优先级的专用后台线程上运行任务。
		m_inputLoopWorker = Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, Windows::System::Threading::WorkItemPriority::High,
			Windows::System::Threading::WorkItemOptions::TimeSliced);

		m_main = std::unique_ptr<DirectX12XAMLCube::DirectX12UWPCubeMain>(new DirectX12XAMLCube::DirectX12UWPCubeMain());

	}

	MainPage::~MainPage() {
		// 析构时停止渲染和处理事件。
		m_main->StopRenderLoop();
		m_coreInput.get().Dispatcher().StopProcessEvents();
	}


    int32_t MainPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();	
    }


	void MainPage::OnDpiChanged(Windows::Graphics::Display::DisplayInformation sender, IInspectable const& args)
	{
		//critical_section::scoped_lock lock(m_main->GetCriticalSection());
		// 注意: 在此处检索到的 LogicalDpi 值可能与应用的有效 DPI 不匹配
		// 如果正在针对高分辨率设备对它进行缩放。在 DeviceResources 上设置 DPI 后，
		// 应始终使用 GetDpi 方法进行检索。
		// 有关详细信息，请参阅 DeviceResources.cpp。
		GetDeviceResources()->SetDpi(sender.LogicalDpi());
		m_main->OnWindowSizeChanged();
	}

	void MainPage::OnOrientationChanged(Windows::Graphics::Display::DisplayInformation sender, IInspectable const& args)
	{
		concurrency::critical_section::scoped_lock lock(m_main->GetCriticalSection());
		GetDeviceResources()->SetCurrentOrientation(sender.CurrentOrientation());
		m_main->OnWindowSizeChanged();
	}

	void MainPage::OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation sender, IInspectable const& args)
	{
		concurrency::critical_section::scoped_lock lock(m_main->GetCriticalSection());
		GetDeviceResources()->ValidateDevice();
	}



	// 在单击应用程序栏按钮时调用。
	void MainPage::AppBarButton_Click(IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs e)
	{
		// 使用应用程序栏(如果它适合你的应用程序)。设计应用程序栏，
		// 然后填充事件处理程序(与此类似)。
	}
	
	void MainPage::OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel sender, IInspectable const& args)
	{
		concurrency::critical_section::scoped_lock lock(m_main->GetCriticalSection());
		GetDeviceResources()->SetCompositionScale(sender.CompositionScaleX(), sender.CompositionScaleY());
		m_main->OnWindowSizeChanged();
	}

	void MainPage::OnSwapChainPanelSizeChanged(IInspectable const& sender, Windows::UI::Xaml::SizeChangedEventArgs e)
	{
		concurrency::critical_section::scoped_lock lock(m_main->GetCriticalSection());
		GetDeviceResources()->SetLogicalSize(e.NewSize());
		m_main->OnWindowSizeChanged();
	}

	std::shared_ptr<DX::DeviceResources> MainPage::GetDeviceResources()
	{
		if (m_deviceResources != nullptr && m_deviceResources->IsDeviceRemoved())
		{
			// All references to the existing D3D device must be released before a new device
			// can be created.

			m_deviceResources = nullptr;
			m_main->StopRenderLoop();
			m_main->OnDeviceRemoved();

#if defined(_DEBUG)
			winrt::com_ptr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(debugController), debugController.put_void())))
			{
				debugController->EnableDebugLayer();
			}
			
#endif
		}

		if (m_deviceResources == nullptr)
		{
			m_deviceResources = std::make_shared<DX::DeviceResources>();
			m_deviceResources->SetSwapChainPanel(m_swapChainPanel());
			m_main->CreateRenderers(m_deviceResources);
			m_main->StartRenderLoop(m_deviceResources);
		}
		return m_deviceResources;
	}

	void MainPage::OnVisibilityChanged(Windows::UI::Core::CoreWindow sender, Windows::UI::Core::VisibilityChangedEventArgs args)
	{
		m_windowVisible = args.Visible();
		if (m_windowVisible)
		{
			m_main->StartRenderLoop(GetDeviceResources());
		}
		else
		{
			m_main->StopRenderLoop();
		}
	}
	
}


