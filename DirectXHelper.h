#pragma once
#include "pch.h"
#include <ppltasks.h>
#include <string_view>


using namespace winrt;
using namespace Windows::ApplicationModel;
using namespace concurrency;

namespace DX 
{

	// �Ӷ������ļ���ִ���첽��ȡ�����ĺ�����
	inline concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename)
	{
		using namespace Windows::Storage;
		using namespace Concurrency;

		return concurrency::create_task([=]
			{
				auto folder = Windows::ApplicationModel::Package::Current().InstalledLocation();
				auto sampleFile{ folder.GetFileAsync(filename).get() };
				auto buffer{ Windows::Storage::FileIO::ReadBufferAsync(sampleFile).get() };
				std::vector<byte> returnBuffer;
				returnBuffer.resize(buffer.Length());
				Streams::DataReader::FromBuffer(buffer).ReadBytes(array_view<byte>(returnBuffer));
				return returnBuffer;
			});
	
	}

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// �ڴ��������öϵ㣬�Բ��� Win32 API ����
			winrt::throw_hresult(hr);
		}
	}

	// ��ʹ�����豸�޹ص�����(DIP)��ʾ�ĳ���ת��Ϊʹ���������ر�ʾ�ĳ��ȡ�
	inline float ConvertDipsToPixels(float dips, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(dips * dpi / dipsPerInch + 0.5f); // ���뵽��ӽ���������
	}

	// �������������԰������ԡ�
#if defined(_DEBUG)
	inline void SetName(ID3D12Object* pObject, LPCWSTR name)
	{
		pObject->SetName(name);
	}
#else
	inline void SetName(ID3D12Object*, LPCWSTR)
	{
	}
#endif
}

//�� ComPtr<T> �� helper ����������
// ����������ָ��Ϊ��������ơ�
#define NAME_D3D12_OBJECT(x) DX::SetName(x.get(), L#x)