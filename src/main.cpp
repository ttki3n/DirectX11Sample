#include "pch.h"

LONG g_windowWidth = 1280;
LONG g_windowHeight = 720;
LPCSTR g_windowClassName = "D3D11WindowClass";
LPCSTR g_windowName = "D3D11 Sample";
HWND g_windowHandle = 0;

bool g_enableVSync = true;

ID3D11Device*			g_d3dDevice = nullptr;
ID3D11DeviceContext*	g_d3dDeviceContext = nullptr;
IDXGISwapChain*			g_d3dSwapChain = nullptr;

ID3D11RenderTargetView* g_d3dRenderTargetView = nullptr;
ID3D11DepthStencilView* g_d3dDepthStencilView = nullptr;
ID3D11Texture2D*		g_d3dDepthStencilBuffer = nullptr; //a texture to associate to the depth stencil view

ID3D11DepthStencilState*	g_d3dDepthStencilState = nullptr;
ID3D11RasterizerState*		g_d3dRasterizeState = nullptr;

D3D11_VIEWPORT g_viewPort = { 0 };

int main(int argc, char** argv)
{
	return 0;
}