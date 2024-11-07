#include "main.h"

#include "DXUTgui.h"
#include "SDKmisc.h"

HWND DXUTgetWindow();

GraphicResources * G;

SceneState scene_state;

BlurHandling blur_handling;

BlurParams blurParams;

std::unique_ptr<Keyboard> _keyboard;
std::unique_ptr<Mouse> _mouse;

CDXUTDialogResourceManager          g_DialogResourceManager;
CDXUTTextHelper*                    g_pTxtHelper = NULL;

#include <codecvt>
std::unique_ptr<SceneNode> loadSponza(ID3D11Device* device, ID3D11InputLayout** l, DirectX::IEffect *e);

inline float lerp(float x1, float x2, float t){
	return x1*(1.0 - t) + x2*t;
}

inline float nextFloat(float x1, float x2){
	return lerp(x1, x2, (float)std::rand() / (float)RAND_MAX);
}


HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* device, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	std::srand(unsigned(std::time(0)));

	HRESULT hr;

	ID3D11DeviceContext* context = DXUTGetD3D11DeviceContext();

	G = new GraphicResources();
	G->render_states = std::make_unique<CommonStates>(device);
	G->scene_constant_buffer = std::make_unique<ConstantBuffer<SceneState> >(device);

	_keyboard = std::make_unique<Keyboard>();
	_mouse = std::make_unique<Mouse>();
	HWND hwnd = DXUTgetWindow();
	_mouse->SetWindow(hwnd);

	g_DialogResourceManager.OnD3D11CreateDevice(device, context);
	g_pTxtHelper = new CDXUTTextHelper(device, context, &g_DialogResourceManager, 15);

	//effects
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"GS"] = { L"ModelVS.hlsl", L"MODEL3_GS", L"gs_5_0" };
		shaderDef[L"VS"] = { L"ModelVS.hlsl", L"MODEL3_VS", L"vs_5_0" };
		shaderDef[L"PS"] = { L"ModelPS.hlsl", L"MODEL3_PS", L"ps_5_0" };

		G->poligon_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"GS"] = { L"ModelVS.hlsl", L"MODEL4_GS", L"gs_5_0" };
		shaderDef[L"VS"] = { L"ModelVS.hlsl", L"MODEL3_VS", L"vs_5_0" };
		shaderDef[L"PS"] = { L"ModelPS.hlsl", L"MODEL3_PS", L"ps_5_0" };

		G->line_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"GS"] = { L"ModelVS.hlsl", L"QUAD3_GS", L"gs_5_0" };
		shaderDef[L"VS"] = { L"ModelVS.hlsl", L"QUAD3_VS", L"vs_5_0" };
		shaderDef[L"PS"] = { L"ModelPS.hlsl", L"QUAD3_PS", L"ps_5_0" };

		G->quad_effect = createHlslEffect(device, shaderDef);
	}
	/*
	{
	std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
	shaderDef[L"VS"] = { L"blur.hlsl", L"VS", L"vs_5_0" };
	shaderDef[L"GS"] = { L"blur.hlsl", L"GS", L"gs_5_0" };
	shaderDef[L"PS"] = { L"blur.hlsl", L"HB", L"ps_5_0" };

	G->blur_h_effect = createHlslEffect(device, shaderDef);
	}
	{
	std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
	shaderDef[L"VS"] = { L"blur.hlsl", L"VS", L"vs_5_0" };
	shaderDef[L"GS"] = { L"blur.hlsl", L"GS", L"gs_5_0" };
	shaderDef[L"PS"] = { L"blur.hlsl", L"VB", L"ps_5_0" };

	G->blur_v_effect = createHlslEffect(device, shaderDef);
	}
	*/
	//models
	{
	}
	{
		D3D11_RASTERIZER_DESC drd =
		{
			D3D11_FILL_SOLID,//D3D11_FILL_MODE FillMode;
			D3D11_CULL_NONE,//D3D11_CULL_MODE CullMode;
			FALSE,//BOOL FrontCounterClockwise;
			0,//INT DepthBias;
			0.0,//FLOAT DepthBiasClamp;
			0.0,//FLOAT SlopeScaledDepthBias;
			FALSE,//BOOL DepthClipEnable;
			FALSE,//BOOL ScissorEnable;
			FALSE,//BOOL MultisampleEnable;
			FALSE//BOOL AntialiasedLineEnable;   
		};
		device->CreateRasterizerState(&drd, G->rsPoligon.ReleaseAndGetAddressOf());
	}
	{
		D3D11_RASTERIZER_DESC drd =
		{
			D3D11_FILL_WIREFRAME,//D3D11_FILL_MODE FillMode;
			D3D11_CULL_NONE,//D3D11_CULL_MODE CullMode;
			FALSE,//BOOL FrontCounterClockwise;
			0,//INT DepthBias;
			0.0,//FLOAT DepthBiasClamp;
			0.0,//FLOAT SlopeScaledDepthBias;
			FALSE,//BOOL DepthClipEnable;
			FALSE,//BOOL ScissorEnable;
			FALSE,//BOOL MultisampleEnable;
			FALSE//BOOL AntialiasedLineEnable;   
		};
		device->CreateRasterizerState(&drd, G->rsLine.ReleaseAndGetAddressOf());
	}
	return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	delete g_pTxtHelper;

	g_DialogResourceManager.OnD3D11DestroyDevice();

	_mouse = 0;

	_keyboard = 0;

	delete G;
}
