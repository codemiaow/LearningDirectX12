#pragma once
#include "pch.h"
#include "StepTimer.h"
#include "DeviceResources.h"
#include "Sample3DSceneRenderer.h"
#include <concrt.h>


// ����Ļ�ϳ��� Direct3D ���ݡ����ڹ���Ӧ�ó�����Դ������Ӧ�ó���״̬�ͳ���֡�ķ�����
namespace DirectX12XAMLCube
{
	class DirectX12UWPCubeMain
	{
	public:
		DirectX12UWPCubeMain();
		void CreateRenderers(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void StartTracking() { m_sceneRenderer->StartTracking(); }
		void TrackingUpdate(float positionX) { m_pointerLocationX = positionX; }
		void StopTracking() { m_sceneRenderer->StopTracking(); }
		bool IsTracking() { return m_sceneRenderer->IsTracking(); }
		void StartRenderLoop(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void StopRenderLoop();
		Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

		void OnWindowSizeChanged();
		void OnSuspending();
		void OnResuming();
		void OnDeviceRemoved();

	private:
		void ProcessInput();
		void Update();
		bool Render();

		// ������豸��Դָ�롣
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// TODO: �滻Ϊ���Լ������ݳ�������
		std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;

		Windows::Foundation::IAsyncAction m_renderLoopWorker;
		Concurrency::critical_section m_criticalSection;

		// ��Ⱦѭ����ʱ����
		DX::StepTimer m_timer;

		// ���ٵ�ǰ����ָ��λ��
		float m_pointerLocationX;
	};
}