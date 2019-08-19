#include "pch.h"
#include "DirectX12UWPCubeMain.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include <pix.h>

using namespace winrt;
using namespace DirectX12XAMLCube;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;


//����Ӧ�ó���ʱ���ز���ʼ��Ӧ�ó����ʲ���
DirectX12UWPCubeMain::DirectX12UWPCubeMain() :
	m_pointerLocationX(0.0f)
{
	// TODO: �����ҪĬ�ϵĿɱ�ʱ�䲽��ģʽ֮�������ģʽ������ļ�ʱ�����á�
	// ���磬���� 60 FPS �̶�ʱ�䲽�������߼��������:
	/*
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);
	*/
}


// ��������ʼ����������
void DirectX12UWPCubeMain::CreateRenderers(const std::shared_ptr<DX::DeviceResources>& deviceResources)
{
	//TODO: �����滻ΪӦ�ó������ݵĳ�ʼ����
	m_sceneRenderer = std::make_unique<Sample3DSceneRenderer>(deviceResources);

	OnWindowSizeChanged();
}

void DirectX12UWPCubeMain::StartRenderLoop(const std::shared_ptr<DX::DeviceResources>& deviceResources)
{
	// �����������ѭ���������У��������������̡߳�
	if (m_renderLoopWorker != nullptr && m_renderLoopWorker.Status() == AsyncStatus::Started)
	{
		return;
	}

	// ����һ�����ں�̨�߳������е�����
	auto workItemHandler =  WorkItemHandler([this, deviceResources](IAsyncAction action)
		{
			// ������µ�֡������ÿ���������ڳ���һ�Ρ�
			while (action.Status() == AsyncStatus::Started)
			{
				Concurrency::critical_section::scoped_lock lock(m_criticalSection);
				if (deviceResources->IsDeviceRemoved())
				{
					// �����豸����ʱ������һ�������̣߳��˳���Ⱦ�̡߳�
					break;
				}

				auto commandQueue = deviceResources->GetCommandQueue();
				PIXBeginEvent(commandQueue, 0, L"Update");
				{
					Update();
				}
				PIXEndEvent(commandQueue);

				PIXBeginEvent(commandQueue, 0, L"Render");
				{
					if (Render())
					{
						deviceResources->Present();
					}
				}
				PIXEndEvent(commandQueue);
			}
		});

	//�ڸ����ȼ���ר�ú�̨�߳�����������
	m_renderLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
}

void DirectX12UWPCubeMain::StopRenderLoop()
{
	m_renderLoopWorker.Cancel();
}

//  ÿ֡����һ��Ӧ�ó���״̬��
void DirectX12UWPCubeMain::Update()
{
	ProcessInput();

	//  ÿ֡����һ��Ӧ�ó���״̬��
	m_timer.Tick([&]()
		{
			// TODO: �����滻ΪӦ�ó������ݵĸ��º�����
			m_sceneRenderer->Update(m_timer);
		});
}

//�ڸ�����Ϸ״̬֮ǰ���������û�����
void DirectX12UWPCubeMain::ProcessInput()
{
	//  TODO: ��֡���봦���ڴ˴���ӡ�
	m_sceneRenderer->TrackingUpdate(m_pointerLocationX);
}

// ���ݵ�ǰӦ�ó���״̬���ֵ�ǰ֡��
// ���֡�ѳ��ֲ�����׼������ʾ���򷵻� true��
bool DirectX12UWPCubeMain::Render()
{
	// ���״θ���ǰ�������Գ����κ����ݡ�
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	// ���ֳ�������
	// TODO: �����滻ΪӦ�ó������ݵ���Ⱦ������
	return m_sceneRenderer->Render();
}

//�ڴ��ڴ�С����(���磬�豸�������)ʱ����Ӧ�ó���״̬
void DirectX12UWPCubeMain::OnWindowSizeChanged()
{
	//  TODO: �����滻ΪӦ�ó������ݵ����С��صĳ�ʼ����
	m_sceneRenderer->CreateWindowSizeDependentResources();
}

// ��Ӧ��֪ͨ��������
void DirectX12UWPCubeMain::OnSuspending()
{
	// TODO: ���������滻ΪӦ�õĹ����߼���

	// �������ڹ�����ܻ���ʱ��ֹ�����Ӧ�ã����
	// ��ñ���ʹӦ�ÿ������ж�λ�������������κ�״̬��

	m_sceneRenderer->SaveState();

	// ���Ӧ�ó���ʹ���������´�������Ƶ�ڴ���䣬
	// �뿼���ͷŸ��ڴ���ʹ���ɹ�����Ӧ�ó���ʹ�á�
}

// ��Ӧ��֪ͨ�������ٹ���
void DirectX12UWPCubeMain::OnResuming()
{
	// TODO: ���������滻ΪӦ�õĻָ��߼���
}

// ֪ͨ����������Ҫ�ͷ��豸��Դ��
void DirectX12UWPCubeMain::OnDeviceRemoved()
{
	// TODO: �����κ������Ӧ�ó���������״̬��Ȼ���ͷŲ���
	// ��Ч�ĳ�����������Դ��
	m_sceneRenderer->SaveState();
	m_sceneRenderer = nullptr;
}
