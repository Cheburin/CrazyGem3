#include "main.h"

#include "DXUTgui.h"
#include "SDKmisc.h"

extern GraphicResources * G;

extern SwapChainGraphicResources * SCG;

extern SceneState scene_state;

extern BlurHandling blur_handling;

extern CDXUTTextHelper*                    g_pTxtHelper;

ID3D11ShaderResourceView* null[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

inline void set_scene_constant_buffer(ID3D11DeviceContext* context){
	G->scene_constant_buffer->SetData(context, scene_state);
};

inline void set_blur_constant_buffer(ID3D11DeviceContext* context){
	//G->blur_constant_buffer->SetData(context, blur_handling);
};

void RenderText()
{
	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos(2, 0);
	g_pTxtHelper->SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	g_pTxtHelper->DrawTextLine(DXUTGetFrameStats(true && DXUTIsVsyncEnabled()));
	g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());

	g_pTxtHelper->End();
}

void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	Camera::OnFrameMove(fTime, fElapsedTime, pUserContext);
}

void renderSceneIntoGBuffer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
void postProccessGBuffer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context);
void postProccessBlur(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context, _In_opt_ std::function<void __cdecl()> setHState, _In_opt_ std::function<void __cdecl()> setVState);

void Draw_Single_Point(ID3D11DeviceContext* context, IEffect* effect, _In_opt_ std::function<void __cdecl()> setCustomState);

struct Pixel
{
	float r;
	float g;
	float b;
	float a;
};

void writePixel(D3D11_MAPPED_SUBRESOURCE & MappedSubResourceWrite, int x, int y)
{
	Pixel& pxl = ((Pixel*)(MappedSubResourceWrite.pData))[y * 1024 + x];
	pxl.r = pxl.g = pxl.b = 1.0f;
}

void clearPixels(void * data)
{
	for (int i = 0; i < 1024 * 1024; i++){
		Pixel& pxl = ((Pixel*)(data))[i];
		pxl.r = pxl.g = pxl.b = .0f;
		pxl.a = 1.0f;
	};
}

struct POINT3D {
	float X, Y, Z;
};

void TextureMapTriangle(Pixel* pixel, POINT3D const *pVertices);
void TextureMapTriangle2(Pixel* pixel, POINT3D const *pVertices);
void TextureMapLine(Pixel* dest, POINT3D const *pVertices);

void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context,
	double fTime, float fElapsedTime, void* pUserContext)
{
	HRESULT hr;

	static ID3D11ShaderResourceView* nullptrSRV[] = {0,0,0, 0,0,0, 0,0,0, 0};

	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = scene_state.vFrustumParams.x;
	vp.Height = scene_state.vFrustumParams.y;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	context->RSSetViewports(1, &vp);

	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	context->ClearRenderTargetView(DXUTGetD3D11RenderTargetView(), ClearColor);
	context->ClearDepthStencilView(SCG->depthStencilV.Get(), D3D10_CLEAR_DEPTH, 1.0, 0);

	context->OMSetRenderTargets(1, renderTargetViewToArray(DXUTGetD3D11RenderTargetView()), SCG->depthStencilV.Get());

	Draw_Single_Point(context, G->poligon_effect.get(), [=]{
		context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
		context->RSSetState(G->rsPoligon.Get());
	});

	Draw_Single_Point(context, G->line_effect.get(), [=]{
		context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
		context->RSSetState(G->rsLine.Get());
	});

	context->OMSetRenderTargets(1, renderTargetViewToArray(DXUTGetD3D11RenderTargetView()), DXUTGetD3D11DepthStencilView());

	context->CopyResource(SCG->depthStencilTStaging.Get(), SCG->depthStencilT.Get());

	{
		D3D11_MAPPED_SUBRESOURCE MappedSubResourceRead;
		hr = context->Map(SCG->depthStencilTStaging.Get(), 0, D3D11_MAP_READ, 0, &MappedSubResourceRead);

		D3D11_MAPPED_SUBRESOURCE MappedSubResourceWrite;
		hr = context->Map(SCG->colorTStaging.Get(), 0, D3D11_MAP_WRITE, 0, &MappedSubResourceWrite);

		clearPixels(MappedSubResourceWrite.pData);

		/* //horizontal line
		float d1 = 1.0 / 40;
		float d2 = 1.0 / 30;

		int lineLength = (1024 - 200) - 200; //ha include last point
		float InvDepthPerLength = (d2 - d1) / lineLength;

		float _d = ((float*)(MappedSubResourceRead.pData))[199 * 1024 + 200];
		float d = d1 + InvDepthPerLength*0.5;
		for (int i = 1; i < lineLength; i++)
		{
			d = d + InvDepthPerLength;
			_d = ((float*)(MappedSubResourceRead.pData))[199 * 1024 + 200 + i];
			if (abs(d - _d) > 0.000001)
				break;
		}
		*/

		/*
		float d1 = 1.0 / 40;
		float d2 = 1.0 / 30;

		int lineWLength = (1024 - 200) - 200;
		int lineHLength = (400) - 200;

		float pLen = pow(lineHLength*lineHLength + lineWLength*lineWLength, 0.5);
		float pX = (lineHLength / pLen), pY = -(lineWLength / pLen);

		float piDiv2 = acos(0);
		float DH = 0.5f / 1.0;// cos(piDiv2 - acos(pX));

		float dHdW = (lineHLength / (float)lineWLength);
		float dZdW = ((d2 - d1) / (float)lineWLength);

		float top = 200;// .5;// . + pY*0.5;// +dHdW*0.5;
		float left = 200;// .5;// +pX*0.5;// +0.5;

		writePixel(MappedSubResourceWrite, left, top);

		bool w = false;
		for (int i = 0; i < lineWLength; i++)
		{
			w = false;
			left = left + 1.0;
			top += dHdW;

			writePixel(MappedSubResourceWrite, left, top);

			float T1 = top - DH;
			//if ( abs(T1 - (int(T1) + 0.5))<0.1 || int(T1 + 0.5) == int(T1))
			{
				w = true;
				writePixel(MappedSubResourceWrite, left, int(T1) + 0);
			}

			float T2 = top + DH;

			//if ( abs(T2 - (int(T2) + 0.5))<0.1 || int(T2 - 0.5) == int(T2))
			{
				w = true;
				writePixel(MappedSubResourceWrite, left, int(T2) + 0);
			}
			
			//if (!w){
			//	break;
			//}
		}
		*/

		struct POINT3D points[3];

		points[0].X = 200 - 0.5;//200;
		points[0].Y = 200 - 0.5;//200; 
		points[0].Z = 40;
		
		points[1].X = 824 - 0.5;//824;
		points[1].Y = 400 - 0.5;//400;
		points[1].Z = 30;
		
		points[2].X = 50 - 0.5;//50;
		points[2].Y = 400 - 0.5;//400;
		points[2].Z = 20;

		float dy = 1.0 / 30 - 1.0 / 40;
		float dx = 1.0 / 20 - 1.0 / 30;
		float sample = 1.0 / 40 + dy * 0.5 + dx*0.5;

		TextureMapTriangle(((Pixel*)(MappedSubResourceWrite.pData)), points);

		points[0].X = 200 + 0.0;//200;
		points[0].Y = 500 + 0.0;//500; 
		points[0].Z = 40;

		points[1].X = 824 + 0.0;//824;
		points[1].Y = 700 + 0.0;//700;
		points[1].Z = 30;

		TextureMapLine(((Pixel*)(MappedSubResourceWrite.pData)), points);

		///////
		for (int i = 0; i < 401; i++)
		{
			for (int k = 0; k < 1024; k++)
			{
				float depth = ((float*)(MappedSubResourceRead.pData))[i * 1024 + k];
				Pixel p = ((Pixel*)(MappedSubResourceWrite.pData))[i * 1024 + k];
				if (abs(depth - p.a)>0.00000001)
					break;
			}
		}
		int f = 0;
		float __depth = ((float*)(MappedSubResourceRead.pData))[699 * 1024 + 823];
		for (int i = 401; i < 1024; i++)
		{
			for (int k = 200; k < 1024; k++)
			{
				float depth = ((float*)(MappedSubResourceRead.pData))[i * 1024 + k];
				Pixel p = ((Pixel*)(MappedSubResourceWrite.pData))[i * 1024 + k];
				//if ((p.r != .0f && depth == 1.0) || (p.r == .0f && depth != 1.0))
				//if (depth != 1.0)
				//{
					if (abs(depth - p.a)>0.00001)
						break;
				//}
			}
		}
		///////
		//f = 10;

		context->Unmap(SCG->colorTStaging.Get(), 0);

		context->Unmap(SCG->depthStencilTStaging.Get(), 0);
	}

	context->CopyResource(SCG->colorT.Get(), SCG->colorTStaging.Get());

	if (1==1){
		context->ClearRenderTargetView(DXUTGetD3D11RenderTargetView(), ClearColor);
		context->ClearDepthStencilView(DXUTGetD3D11DepthStencilView(), D3D10_CLEAR_DEPTH, 1.0, 0);

		context->OMSetRenderTargets(1, renderTargetViewToArray(DXUTGetD3D11RenderTargetView()), DXUTGetD3D11DepthStencilView());

		Draw_Single_Point(context, G->quad_effect.get(), [=]{
			context->PSSetSamplers(0, 1, samplerStateToArray(G->render_states->PointWrap()));

			context->PSSetShaderResources(0, 1, shaderResourceViewToArray(SCG->colorSRV.Get()));
		
			context->OMSetDepthStencilState(G->render_states->DepthNone(), 0);
			context->RSSetState(G->rsPoligon.Get());
		});
	}

	context->PSSetShaderResources(0, 1, nullptrSRV);

	RenderText();
}

