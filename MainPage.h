#pragma once
#include "MainPage.g.h"
#include "DeviceResources.h"
#include "DirectX12UWPCubeMain.h"

namespace winrt::BlankApp::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
		MainPage();
		virtual ~MainPage();

        int32_t MyProperty();

        void MyProperty(int32_t value);


		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		std::unique_ptr<DirectX12XAMLCube::DirectX12UWPCubeMain> m_main;

		//  m_deviceResources 的专用访问器，防范设备已删除错误。
		std::shared_ptr<DX::DeviceResources> GetDeviceResources();
		// XAML 低级渲染事件处理程序。
		// 窗口事件处理程序。
		void OnVisibilityChanged(Windows::UI::Core::CoreWindow sender, Windows::UI::Core::VisibilityChangedEventArgs args);
		// DisplayInformation 事件处理程序。
		void OnDpiChanged(Windows::Graphics::Display::DisplayInformation sender, IInspectable const& args);
		void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation sender, IInspectable const& args);
		void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation sender, IInspectable const& args);

		// 其他事件处理程序。
		void AppBarButton_Click(IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs e);
		void OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel sender, IInspectable const& args);
		void OnSwapChainPanelSizeChanged(IInspectable const& sender, Windows::UI::Xaml::SizeChangedEventArgs e);

		// 在后台工作线程上跟踪我们的独立输入。
		Windows::Foundation::IAsyncAction m_inputLoopWorker;
		winrt::agile_ref<Windows::UI::Core::CoreIndependentInputSource> m_coreInput;


		bool m_windowVisible;
	};
}

namespace winrt::BlankApp::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
