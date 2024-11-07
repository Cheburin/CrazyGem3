#include<windows.h>
#include<math.h>
#include "main.h"
struct Pixel
{
	float r;
	float g;
	float b;
	float a;
};

struct POINT3D {
	float X, Y, Z;
};

struct edge2 {
	edge2(POINT3D const *pVertices,
	int Top, int Bottom);
	inline int Step(void);
	float X, XStep; // fractional x and dX/dY
	int Y, Height; // current y and vert count
};

namespace PolygonRasterizer{
	DirectX::XMFLOAT4X4 M;
	DirectX::XMFLOAT4 Top;

	DirectX::XMFLOAT4 Depth;
	DirectX::XMFLOAT4 DeltaDepth;
}

edge2::edge2(POINT3D const *pVertices, int Top, int Bottom)
{
	Y = ceil(pVertices[Top].Y);
	int YEnd = ceil(pVertices[Bottom].Y);
	Height = YEnd - Y;
	float YPrestep = Y - pVertices[Top].Y;
	float RealHeight = pVertices[Bottom].Y - pVertices[Top].Y;
	float RealWidth = pVertices[Bottom].X - pVertices[Top].X;
	X = ((RealWidth * YPrestep) / RealHeight) + pVertices[Top].X;
	XStep = RealWidth / RealHeight;
}

inline int edge2::Step(void) {
	X += XStep; Y++; Height--;
	return Height;
}

void DrawScanLine2(Pixel* dest, edge2 *pLeft, edge2 *pRight)
{
	int XStart = ceil(pLeft->X);
	int Width = ceil(pRight->X) - XStart;

	if (Width > 0) {
		int X = XStart;
		while (Width--)
		{
			dest[1024 * pLeft->Y + X].r = dest[1024 * pLeft->Y + X].g = dest[1024 * pLeft->Y + X].b = 1.0f;

			SimpleMath::Matrix M(PolygonRasterizer::M);
			SimpleMath::Vector3 T(X - PolygonRasterizer::Top.x, pLeft->Y - PolygonRasterizer::Top.y, 0);

			SimpleMath::Vector3 dD(PolygonRasterizer::DeltaDepth.x, PolygonRasterizer::DeltaDepth.y, 0);
			SimpleMath::Vector3 D(PolygonRasterizer::Depth.x, 0, 0);

			SimpleMath::Vector3 bary = SimpleMath::Vector3::Transform(T, M);

			dest[1024 * pLeft->Y + X].a = D.x + bary.x*dD.x + bary.y*dD.y;

			X++;
		}
	}
}

void TextureMapTriangle2(Pixel* dest, POINT3D const *pVertices)
{
	int Top, Middle, Bottom;
	int MiddleCompare, BottomCompare;
	float Y0 = pVertices[0].Y;
	float Y1 = pVertices[1].Y;
	float Y2 = pVertices[2].Y;
	// sort vertices in y
	if (Y0 < Y1) {
		if (Y2 < Y0) {
			Top = 2; Middle = 0; Bottom = 1;
			MiddleCompare = 0; BottomCompare = 1;
		}
		else {
			Top = 0;
			if (Y1 < Y2) {
				Middle = 1; Bottom = 2;
				MiddleCompare = 1; BottomCompare = 2;
			}
			else {
				Middle = 2; Bottom = 1;
				MiddleCompare = 2; BottomCompare = 1;
			}
		}
	}
	else {
		if (Y2 < Y1) {
			Top = 2; Middle = 1; Bottom = 0;
			MiddleCompare = 1; BottomCompare = 0;
		}
		else {
			Top = 1;
			if (Y0 < Y2) {
				Middle = 0; Bottom = 2;
				MiddleCompare = 3; BottomCompare = 2;
			}
			else {
				Middle = 2; Bottom = 0;
				MiddleCompare = 2; BottomCompare = 3;
			}
		}
	}

	////////////////////////////////////////////////////
	{
		PolygonRasterizer::Top = SimpleMath::Vector4(pVertices[Top].X, pVertices[Top].Y, 0, 1);

		PolygonRasterizer::DeltaDepth = SimpleMath::Vector4(1 / pVertices[Bottom].Z - 1 / pVertices[Top].Z, 1 / pVertices[Middle].Z - 1 / pVertices[Top].Z, 0, 1);
		PolygonRasterizer::Depth = SimpleMath::Vector4(1 / pVertices[Top].Z, 0, 0, 1);

		SimpleMath::Vector3 V1 = SimpleMath::Vector3(pVertices[Bottom].X - pVertices[Top].X, pVertices[Bottom].Y - pVertices[Top].Y, 0);
		SimpleMath::Vector3 V2 = SimpleMath::Vector3(pVertices[Middle].X - pVertices[Top].X, pVertices[Middle].Y - pVertices[Top].Y, 0);
		SimpleMath::Matrix  M(V1, V2, SimpleMath::Vector3(0, 0, 1));
		PolygonRasterizer::M = M.Invert();
	}
	////////////////////////////////////////////////////

	edge2 TopToBottom(pVertices, Top, Bottom);
	edge2 TopToMiddle(pVertices, Top, Middle);
	edge2 MiddleToBottom(pVertices, Middle, Bottom);
	edge2 *pLeft, *pRight;
	int MiddleIsLeft;
	// the triangle is clockwise, so
	// if bottom > middle then middle is right
	if (BottomCompare > MiddleCompare) {
		MiddleIsLeft = 0;
		pLeft = &TopToBottom; pRight = &TopToMiddle;
	}
	else {
		MiddleIsLeft = 1;
		pLeft = &TopToMiddle; pRight = &TopToBottom;
	}
	int Height = TopToMiddle.Height;
	while (Height--) {
		DrawScanLine2(dest, pLeft, pRight);
		TopToMiddle.Step(); TopToBottom.Step();
	}
	Height = MiddleToBottom.Height;
	if (MiddleIsLeft) {
		pLeft = &MiddleToBottom; pRight = &TopToBottom;
	}
	else {
		pLeft = &TopToBottom; pRight = &MiddleToBottom;
	}
	while (Height--) {
		DrawScanLine2(dest, pLeft, pRight);
		MiddleToBottom.Step(); TopToBottom.Step();
	}
}