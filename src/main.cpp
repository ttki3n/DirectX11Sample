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

D3D11_VIEWPORT g_viewport = { 0 };

// specific for this sample
ID3D11InputLayout* g_d3dInputLayout = nullptr;
ID3D11Buffer* g_d3dVertexBuffer = nullptr;
ID3D11Buffer* g_d3dIndexBuffer = nullptr;

ID3D11VertexShader* g_d3dVertexShader = nullptr;
ID3D11PixelShader* g_d3dPixelShader = nullptr;

// shader resources
enum ConstantBuffers
{
	CB_APPLICATION,
	CB_FRAME,
	CB_OBJECT,
	CB_TOTAL
};

ID3D11Buffer* g_d3dConstantBuffers[CB_TOTAL];
XMMATRIX g_WorldMatrix, g_ViewMatrix, g_ProjectionMatrix;

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
DXGI_RATIONAL QueryRefreshRate(UINT screenWidth, UINT screenHeight, BOOL vsync);

template<class TShaderClass>
TShaderClass* LoadShader(const std::wstring& filename, const std::string& entrypoint, const std::string& profile);

template<class TShaderClass>
std::string GetLatestProfile();

template<class TShaderClass>
TShaderClass* CreateShader(ID3DBlob* pShaderBlob, ID3D11ClassLinkage* pClassLinkage);

bool LoadContent();
void UnloadContent();

void Update(float deltaTime);
void Render();
void CleanUp();



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
			
			Update(deltaTime);
			Render();
		}
	}
	
	return static_cast<int>(msg.wParam);
}


int InitDirectX(HINSTANCE hInstance, BOOL vSync);
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

	if (InitDirectX(hInstance, g_enableVSync) != 0)
	{
		MessageBox(nullptr, TEXT("Failed to initialize DirectX!"), TEXT("Error"), MB_OK);
		return -1;
	}

	if (!LoadContent())
	{
		MessageBox(nullptr, TEXT("Failed to load content."), TEXT("Error"), MB_OK);
		return -1;
	}
	
	int result = Run();

	UnloadContent();
	CleanUp();
	return result;
}

// Initialize the directX device and swapchain
int InitDirectX(HINSTANCE hInstance, BOOL vSync)
{
	// a window handle must have been created already
	assert(g_windowHandle != 0);

	RECT clientRect;
	GetClientRect(g_windowHandle, &clientRect);

	unsigned int clientWidth = clientRect.right - clientRect.left;
	unsigned int clientHeight = clientRect.bottom - clientRect.top;

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = clientWidth;
	swapChainDesc.BufferDesc.Height = clientHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate = QueryRefreshRate(clientWidth, clientHeight, vSync);
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = g_windowHandle;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Windowed = TRUE;

	UINT createDeviceFlags = 0;
#if _DEBUG
	//createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
		nullptr, createDeviceFlags, featureLevels, _countof(featureLevels),
		D3D11_SDK_VERSION, &swapChainDesc, &g_d3dSwapChain, &g_d3dDevice, &featureLevel, &g_d3dDeviceContext);

	// check if can't create with 11_1
	if (hr == E_INVALIDARG)
	{
		hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
			nullptr, createDeviceFlags, &featureLevels[1], _countof(featureLevels) - 1,
			D3D11_SDK_VERSION, &swapChainDesc, &g_d3dSwapChain, &g_d3dDevice, &featureLevel, &g_d3dDeviceContext);
	}

	if (FAILED(hr))
	{
		return -1;
	}

	// Initializa backbuffer of the swapchain and associate it to the render target view
	ID3D11Texture2D* backBuffer;
	hr = g_d3dSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	if (FAILED(hr))
	{
		return -1;
	}

	hr = g_d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &g_d3dRenderTargetView);
	if (FAILED(hr))
	{
		return -1;
	}

	COMSafeRelease(backBuffer);

	// Create the depth buffer
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
	ZeroMemory(&depthStencilBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));
	depthStencilBufferDesc.ArraySize = 1;
	depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilBufferDesc.CPUAccessFlags = 0;
	depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilBufferDesc.Width = clientWidth;
	depthStencilBufferDesc.Height = clientHeight;
	depthStencilBufferDesc.MipLevels = 1;
	depthStencilBufferDesc.SampleDesc.Count = 1;
	depthStencilBufferDesc.SampleDesc.Quality = 0;
	depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = g_d3dDevice->CreateTexture2D(&depthStencilBufferDesc, nullptr, &g_d3dDepthStencilBuffer);
	if (FAILED(hr))
	{
		return -1;
	}

	hr = g_d3dDevice->CreateDepthStencilView(g_d3dDepthStencilBuffer, nullptr, &g_d3dDepthStencilView);
	if (FAILED(hr))
	{
		return -1;
	}

	// setup depth stencil state
	D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc;
	ZeroMemory(&depthStencilStateDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	depthStencilStateDesc.DepthEnable = TRUE;
	depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilStateDesc.StencilEnable = FALSE;

	hr = g_d3dDevice->CreateDepthStencilState(&depthStencilStateDesc, &g_d3dDepthStencilState);

	// setup rasterizer state
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;

	hr = g_d3dDevice->CreateRasterizerState(&rasterizerDesc, &g_d3dRasterizeState);
	if (FAILED(hr))
	{
		return -1;
	}

	// Initialize the viewport
	g_viewport.Width = static_cast<float>(clientWidth);
	g_viewport.Height = static_cast<float>(clientHeight);
	g_viewport.TopLeftX = 0.0f;
	g_viewport.TopLeftY = 0.0f;
	g_viewport.MinDepth = 0.0f;
	g_viewport.MaxDepth = 1.0f;

	return 0;

}

template<>
std::string GetLatestProfile<ID3D11VertexShader>()
{
	assert(g_d3dDevice);

	D3D_FEATURE_LEVEL featureLevel = g_d3dDevice->GetFeatureLevel();

	switch (featureLevel)
	{
	case D3D_FEATURE_LEVEL_11_1:
	case D3D_FEATURE_LEVEL_11_0:
		{
			return "vs_5_0";
		}
		break;
	case D3D_FEATURE_LEVEL_10_1:
		{
			return "vs_4_1";
		}
		break;
	case D3D_FEATURE_LEVEL_10_0:
		{
			return "vs_4_0";
		}
		break;
	case D3D_FEATURE_LEVEL_9_3:
		{
			return "vs_4_0_level_9_3";
		}
		break;
	case D3D_FEATURE_LEVEL_9_2:
	case D3D_FEATURE_LEVEL_9_1:
		{
			return "vs_4_0_level_9_1";
		}
		break;
	}

	return "";
}


template<>
std::string GetLatestProfile<ID3D11PixelShader>()
{
	assert(g_d3dDevice);

	D3D_FEATURE_LEVEL featureLevel = g_d3dDevice->GetFeatureLevel();

	switch (featureLevel)
	{
	case D3D_FEATURE_LEVEL_11_1:
	case D3D_FEATURE_LEVEL_11_0:
	{
		return "ps_5_0";
	}
	break;
	case D3D_FEATURE_LEVEL_10_1:
	{
		return "ps_4_1";
	}
	break;
	case D3D_FEATURE_LEVEL_10_0:
	{
		return "ps_4_0";
	}
	break;
	case D3D_FEATURE_LEVEL_9_3:
	{
		return "ps_4_0_level_9_3";
	}
	break;
	case D3D_FEATURE_LEVEL_9_2:
	case D3D_FEATURE_LEVEL_9_1:
	{
		return "ps_4_0_level_9_1";
	}
	break;
	}

	return "";
}

template<>
ID3D11VertexShader* CreateShader<ID3D11VertexShader>(ID3DBlob* pShaderBlob, ID3D11ClassLinkage* pClassLinkage)
{
	assert(g_d3dDevice);
	assert(pShaderBlob);
	ID3D11VertexShader* pVertexShader = nullptr;
	g_d3dDevice->CreateVertexShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), pClassLinkage, &pVertexShader);

	return pVertexShader;
}

template<>
ID3D11PixelShader* CreateShader<ID3D11PixelShader>(ID3DBlob* pShaderBlob, ID3D11ClassLinkage* pClassLinkage)
{
	assert(g_d3dDevice);
	assert(pShaderBlob);
	ID3D11PixelShader* pPixelShader = nullptr;
	g_d3dDevice->CreatePixelShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), pClassLinkage, &pPixelShader);

	return pPixelShader;
}

template<class TShaderClass>
TShaderClass* LoadShader(const std::wstring& fileName, const std::string& entryPoint, const std::string& _profile)
{
	ID3DBlob* pShaderBlob = nullptr;
	ID3DBlob* pErrorBlod = nullptr;
	TShaderClass* pShader = nullptr;

	std::string profile = _profile;
	if (profile == "lastest")
	{
		profile = GetLastestProfile<TShaderClass>();
	}

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
	flags |= D3DCOMPILE_DEBUG;
#endif

	HRESULT hr = D3DCompileFromFile(fileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), profile.c_str(),
		flags, 0, &pShaderBlob, &pErrorBlob);

	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			std::string msg = (char*)pErrorBlob->GetBufferPointer();
			OutputDebugStringA(msg.c_str());
			COMSafeRelease(pShaderBlob);
			COMSafeRelease(pErrorBlob);
		}
		return false;
	}

	pShader = CreateShader<TShaderClass>(pShaderBlob, nullptr);
	COMSafeRelease(pShaderBlob);
	COMSafeRelease(pErrorBlob);

	return pShader;
}

bool LoadContent()
{
	assert(g_d3dDevice);
	// create & initialize vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));

	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.ByteWidth = sizeof(VertexPosColor)* _countof(g_vertices);
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA resourceData;
	ZeroMemory(&resourceData, sizeof(D3D11_SUBRESOURCE_DATA));

	resourceData.pSysMem = g_vertices;
	
	HRESULT hr = g_d3dDevice->CreateBuffer(&vertexBufferDesc, &resourceData, &g_d3dVertexBuffer);
	if (FAILED(hr))
	{
		return false;
	}

	// create & initialize index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));

	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.ByteWidth = sizeof(WORD) * _countof(g_indices);
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	resourceData.pSysMem = g_indices;

	hr = g_d3dDevice->CreateBuffer(&indexBufferDesc, &resourceData, &g_d3dIndexBuffer);
	if (FAILED(hr))
	{
		return false;
	}

	// create constant buffer for the variable in vertex buffer
	D3D11_BUFFER_DESC constantBufferDesc;
	ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));

	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.ByteWidth = sizeof(XMMATRIX);
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	hr = g_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &g_d3dConstantBuffers[CB_APPLICATION]);
	if (FAILED(hr))
	{
		return false;
	}

	hr = g_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &g_d3dConstantBuffers[CB_FRAME]);
	if (FAILED(hr))
	{
		return false;
	}

	hr = g_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &g_d3dConstantBuffers[CB_OBJECT]);
	if (FAILED(hr))
	{
		return false;
	}

	// load the compiled shaders
	ID3DBlob* vertexShaderBlob;
	LPCWSTR compiledVertexShaderObject = L"SimpleVertexShader.cso";

	hr = D3DReadFileToBlob(compiledVertexShaderObject, &vertexShaderBlob);
	if (FAILED(hr))
	{
		return false;
	}

	hr = g_d3dDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &g_d3dVertexShader);
	if (FAILED(hr))
	{
		return false;
	}


	// create the input layout for the vertex shader
	D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosColor, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosColor, color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = g_d3dDevice->CreateInputLayout(vertexLayoutDesc, _countof(vertexLayoutDesc), vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(), &g_d3dInputLayout);
	if (FAILED(hr))
	{
		return false;
	}
	COMSafeRelease(vertexShaderBlob);

	// load the precompiled pixel shader
	ID3DBlob* pixelShaderBlob;
	LPCWSTR compiledPixelShaderObject = L"SimplePixelShader.cso";

	hr = D3DReadFileToBlob(compiledPixelShaderObject, &pixelShaderBlob);
	if (FAILED(hr))
	{
		return false;
	}

	hr = g_d3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &g_d3dPixelShader);
	if (FAILED(hr))
	{
		return false;
	}
	COMSafeRelease(pixelShaderBlob);

	// Setup projection matrix
	RECT clientRect;
	GetClientRect(g_windowHandle, &clientRect);

	// compute the exact client dimensions.
	float clientWidth = static_cast<float>(clientRect.right - clientRect.left);
	float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

	g_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), clientWidth / clientHeight, 0.1f, 100.f);

	g_d3dDeviceContext->UpdateSubresource(g_d3dConstantBuffers[CB_APPLICATION], 0, nullptr, &g_ProjectionMatrix, 0, 0);

	return true;
}

void Update(float deltaTime)
{
	XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	g_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
	g_d3dDeviceContext->UpdateSubresource(g_d3dConstantBuffers[CB_FRAME], 0, nullptr, &g_ViewMatrix, 0, 0);

	static float angle = 0.0f;
	angle += 1.0f * deltaTime;
	XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);

	g_WorldMatrix = XMMatrixRotationAxis(rotationAxis, angle);
	g_d3dDeviceContext->UpdateSubresource(g_d3dConstantBuffers[CB_OBJECT], 0, nullptr, &g_WorldMatrix, 0, 0);
}

void Clear(const FLOAT clearColor[4], FLOAT clearDepth, UINT8 clearStencil)
{
	g_d3dDeviceContext->ClearRenderTargetView(g_d3dRenderTargetView, clearColor);
	g_d3dDeviceContext->ClearDepthStencilView(g_d3dDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDepth, clearStencil);
}

void Present(bool vSync)
{
	if (vSync)
	{
		g_d3dSwapChain->Present(1, 0);
	}
	else
	{
		g_d3dSwapChain->Present(0, 0);
	}
}

void Render()
{
	assert(g_d3dDevice);
	assert(g_d3dDeviceContext);

	Clear(Colors::CornflowerBlue, 1.0f, 0);

	// setup input assembler stage
	const UINT vertexStride = sizeof(VertexPosColor);
	const UINT offset = 0;

	g_d3dDeviceContext->IASetVertexBuffers(0, 1, &g_d3dVertexBuffer, &vertexStride, &offset);
	g_d3dDeviceContext->IASetInputLayout(g_d3dInputLayout);
	g_d3dDeviceContext->IASetIndexBuffer(g_d3dIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	g_d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// setup vertex shader stage
	g_d3dDeviceContext->VSSetShader(g_d3dVertexShader, nullptr, 0);
	g_d3dDeviceContext->VSSetConstantBuffers(0, 3, g_d3dConstantBuffers);

	// setup rasterizer stage
	// After the vertex shader stage but before the pixel shader stage comes the rasterizer stage. 
	// The rasterizer stage is responsible for interpolating the various vertex attributes output from the vertex shader and invoking the pixel shader program for each screen pixel which is affected by the rendered geometry.
	g_d3dDeviceContext->RSSetState(g_d3dRasterizeState);
	g_d3dDeviceContext->RSSetViewports(1, &g_viewport);

	// setup the pixel shader stage
	g_d3dDeviceContext->PSSetShader(g_d3dPixelShader, nullptr, 0);

	// setup output merger stage
	g_d3dDeviceContext->OMSetRenderTargets(1, &g_d3dRenderTargetView, g_d3dDepthStencilView);
	g_d3dDeviceContext->OMSetDepthStencilState(g_d3dDepthStencilState, 1);

	g_d3dDeviceContext->DrawIndexed(_countof(g_indices), 0, 0);

	Present(g_enableVSync);
}

// cleanup contents in LoadContent()
void UnloadContent()
{
	COMSafeRelease(g_d3dConstantBuffers[CB_APPLICATION]);
	COMSafeRelease(g_d3dConstantBuffers[CB_FRAME]);
	COMSafeRelease(g_d3dConstantBuffers[CB_OBJECT]);
	COMSafeRelease(g_d3dIndexBuffer);
	COMSafeRelease(g_d3dVertexBuffer);
	COMSafeRelease(g_d3dInputLayout);
	COMSafeRelease(g_d3dVertexShader);
	COMSafeRelease(g_d3dPixelShader);
}

// cleanup contents in InitDirectX()
void CleanUp()
{
	COMSafeRelease(g_d3dDepthStencilView);
	COMSafeRelease(g_d3dRenderTargetView);
	COMSafeRelease(g_d3dDepthStencilBuffer);
	COMSafeRelease(g_d3dDepthStencilState);
	COMSafeRelease(g_d3dRasterizeState);
	COMSafeRelease(g_d3dSwapChain);
	COMSafeRelease(g_d3dDeviceContext);
	COMSafeRelease(g_d3dDevice);
}











// This function was inspired by:
// http://www.rastertek.com/dx11tut03.html
DXGI_RATIONAL QueryRefreshRate(UINT screenWidth, UINT screenHeight, BOOL vsync)
{
	DXGI_RATIONAL refreshRate = { 0, 1 };
	if (vsync)
	{
		IDXGIFactory* factory;
		IDXGIAdapter* adapter;
		IDXGIOutput* adapterOutput;
		DXGI_MODE_DESC* displayModeList;

		// Create a DirectX graphics interface factory.
		HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Could not create DXGIFactory instance."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to create DXGIFactory.");
		}

		hr = factory->EnumAdapters(0, &adapter);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to enumerate adapters."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to enumerate adapters.");
		}

		hr = adapter->EnumOutputs(0, &adapterOutput);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to enumerate adapter outputs."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to enumerate adapter outputs.");
		}

		UINT numDisplayModes;
		hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, nullptr);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to query display mode list."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to query display mode list.");
		}

		displayModeList = new DXGI_MODE_DESC[numDisplayModes];
		assert(displayModeList);

		hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, displayModeList);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to query display mode list."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to query display mode list.");
		}

		// Now store the refresh rate of the monitor that matches the width and height of the requested screen.
		for (UINT i = 0; i < numDisplayModes; ++i)
		{
			if (displayModeList[i].Width == screenWidth && displayModeList[i].Height == screenHeight)
			{
				refreshRate = displayModeList[i].RefreshRate;
			}
		}

		delete[] displayModeList;
		COMSafeRelease(adapterOutput);
		COMSafeRelease(adapter);
		COMSafeRelease(factory);
	}

	return refreshRate;
}
