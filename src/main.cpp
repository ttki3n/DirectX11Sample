#include "pch.h"

using namespace DirectX; // for directXMath like vectors, matrices

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

// specific for this sample
ID3D11InputLayout* g_d3dInputLayout = nullptr;
ID3D11Buffer* g_d3dVertexBuffer = nullptr;
ID3D11Buffer* g_d3dIndexBuffer = nullptr;

ID3D11VertexShader* g_d3dVertexShader = nullptr;
ID3D11PixelShader* g_d3dPixelShader = nullptr;

//shader resources
enum ConstantBuffers
{
	CB_APPLICATION,
	CB_FRAME,
	CB_OBJECT,
	CB_TOTAL
};

ID3D11Buffer* g_d3dConstantBuffers[CB_TOTAL];
XMMATRIX g_WorldMatrix, g_ViewMatrix, g_ProjectionMatric;

// data of colored cube
struct VertexPosColor
{
	XMFLOAT3 pos;
	XMFLOAT3 color;
};

VertexPosColor g_vertices[8] =
{
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

WORD g_indices[36] = 
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

template<class ShaderClass>
ShaderClass* LoadShader(const std::wstring& filename, const std::string& entrypoint, const std::string& profile);

bool LoadContent();
void UnloadContent();

void Update(float deltaTime);
void Render();
void Cleanup();

int InitApplication(HINSTANCE hInstance, int cmdShow)
{
	WNDCLASSEX wndClass = { 0 };
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = &WndProc;
	wndClass.hInstance = hInstance;
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = g_windowClassName;

	if (!RegisterClassEx(&wndClass))
	{
		return -1;
	}

	RECT windowRect = { 0, 0, g_windowWidth, g_windowHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	g_windowHandle = CreateWindowA(g_windowClassName, g_windowName,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr, nullptr, hInstance, nullptr);

	if (!g_windowHandle)
	{
		return -1;
	}

	ShowWindow(g_windowHandle, cmdShow);
	UpdateWindow(g_windowHandle);

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT paintStruct;
	HDC	hDC;

	switch (msg)
	{
	case WM_PAINT:	
		hDC = BeginPaint(hWnd, &paintStruct);
		EndPaint(hWnd, &paintStruct);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);	
	}

	return 0;
}

int Run()
{
	MSG msg = { 0 };
	static DWORD previousTime = timeGetTime(); // miliseconds
	static const float targetFramerate = 30.0f;
	static const float maxTimeStep = 1.0f / targetFramerate;

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);			
			DispatchMessage(&msg);
		}
		else
		{
			DWORD currentTime = timeGetTime();
			float deltaTime = (currentTime - previousTime) / 1000.0f;
			previousTime = currentTime;

			//limit
			deltaTime = std::min<float>(deltaTime, maxTimeStep);
			
			//Update(deltaTime);
			//Render();
		}
	}
	
	return static_cast<int>(msg.wParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
	UNREFERENCED_PARAMETER(prevInstance);
	UNREFERENCED_PARAMETER(cmdLine);

	// check for DirectX Math 
	if (!XMVerifyCPUSupport())
	{
		MessageBox(nullptr, TEXT("Failed to verify DirectX Math library support!"), TEXT("Error"), MB_OK);
		return -1;
	}

	if (InitApplication(hInstance, cmdShow) != 0)
	{
		MessageBox(nullptr, TEXT("Failed to create application!"), TEXT("Error"), MB_OK);
		return -1;
	}

	int result = Run();
	return result;
}
