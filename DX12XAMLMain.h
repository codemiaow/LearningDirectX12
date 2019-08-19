#pragma once
#include "Sample3DSceneRenderer.h"
#include "DeviceResources.h"

namespace DX12XAML {

	class DX12XAMLMain
	{
	public:
		DX12XAMLMain();
		void CreateRenderers(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void Update();
		bool Render();

		void OnWindowSizeChanged();
		void OnSuspending();
		void OnResuming();
		void OnDeviceRemoved();

	private:
		// TODO: 替换为你自己的内容呈现器。
		std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;

		// 渲染循环计时器。
		DX::StepTimer m_timer;
	};

}

