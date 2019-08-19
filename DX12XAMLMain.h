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
		// TODO: �滻Ϊ���Լ������ݳ�������
		std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;

		// ��Ⱦѭ����ʱ����
		DX::StepTimer m_timer;
	};

}

