#pragma once

#include "pch.h"

namespace DirectX12XAMLCube
{
	// �����򶥵���ɫ������ MVP ����ĳ�����������
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	// �����򶥵���ɫ������ÿ����������ݡ�
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};
}