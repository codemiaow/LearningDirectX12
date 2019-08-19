#pragma once
#include "pch.h"
#include <ppltasks.h>
#include <string_view>


using namespace winrt;
using namespace Windows::ApplicationModel;
using namespace concurrency;

namespace DX 
{

	// 从二进制文件中执行异步读取操作的函数。
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
			// 在此行中设置断点，以捕获 Win32 API 错误。
			winrt::throw_hresult(hr);
		}
	}

	// 将使用与设备无关的像素(DIP)表示的长度转换为使用物理像素表示的长度。
	inline float ConvertDipsToPixels(float dips, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(dips * dpi / dipsPerInch + 0.5f); // 舍入到最接近的整数。
	}

	// 向对象分配名称以帮助调试。
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

//对 ComPtr<T> 的 helper 函数命名。
// 将变量名称指定为对象的名称。
#define NAME_D3D12_OBJECT(x) DX::SetName(x.get(), L#x)