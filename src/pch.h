#pragma once
// system
#include <Windows.h>

// directX
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

// STL
#include <iostream>
#include <string>


// link
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")


// safely release COM object
template<typename T>
inline void COMSafeRelease(T& t)
{
	if (t != NULL)
	{
		t->Release();
		t = 0;
	}
}

