#include<windows.h>
#include<math.h>

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

struct gradients {
	gradients(POINT3D const *pVertices);
	double aOneOverZ[3]; // 1/z for each vertex
	double dOneOverZdX, dOneOverZdY; // d(1/z)/dX, d(1/z)/dY
};

struct edge {
	edge(gradients const &Gradients,
	POINT3D const *pVertices,
	int Top, int Bottom);
	inline int Step(void);
	double X, XStep; // fractional x and dX/dY
	int Y, Height; // current y and vert count
	double OneOverZ, OneOverZStep; // 1/z and step
};

gradients::gradients(POINT3D const *pVertices)
{
	int Counter;
	double OneOverdX = 1 / (((pVertices[1].X - pVertices[2].X) *
		(pVertices[0].Y - pVertices[2].Y)) -
		((pVertices[0].X - pVertices[2].X) *
		(pVertices[1].Y - pVertices[2].Y)));
	double OneOverdY = -OneOverdX;
	for (Counter = 0; Counter < 3; Counter++) {
		double const OneOverZ = 1 / pVertices[Counter].Z;
		aOneOverZ[Counter] = OneOverZ;
	}
	dOneOverZdX = OneOverdX * (((aOneOverZ[1] - aOneOverZ[2]) *
		(pVertices[0].Y - pVertices[2].Y)) -
		((aOneOverZ[0] - aOneOverZ[2]) *
		(pVertices[1].Y - pVertices[2].Y)));
	dOneOverZdY = OneOverdY * (((aOneOverZ[1] - aOneOverZ[2]) *
		(pVertices[0].X - pVertices[2].X)) -
		((aOneOverZ[0] - aOneOverZ[2]) *
		(pVertices[1].X - pVertices[2].X)));
}

edge::edge(gradients const &Gradients,
	POINT3D const *pVertices, int Top, int Bottom)
{
	Y = ceil(pVertices[Top].Y);
	int YEnd = ceil(pVertices[Bottom].Y);
	Height = YEnd - Y;
	double YPrestep = Y - pVertices[Top].Y;
	double RealHeight = pVertices[Bottom].Y - pVertices[Top].Y;
	double RealWidth = pVertices[Bottom].X - pVertices[Top].X;
	X = ((RealWidth * YPrestep) / RealHeight) + pVertices[Top].X;
	XStep = RealWidth / RealHeight;
	double XPrestep = X - pVertices[Top].X;
	OneOverZ = Gradients.aOneOverZ[Top] +
		YPrestep * Gradients.dOneOverZdY +
		XPrestep * Gradients.dOneOverZdX;
	OneOverZStep = XStep *
		Gradients.dOneOverZdX + Gradients.dOneOverZdY;
}

inline int edge::Step(void) {
	X += XStep; Y++; Height--;
	OneOverZ += OneOverZStep;
	return Height;
}

void DrawScanLine(Pixel* dest, gradients const &Gradients, edge *pLeft, edge *pRight)
{
	int XStart = ceil(pLeft->X);
	double XPrestep = XStart - pLeft->X;
	int Width = ceil(pRight->X) - XStart;
	double OneOverZ = pLeft->OneOverZ +
		XPrestep * Gradients.dOneOverZdX;

	if (Width > 0) {
		int X = XStart;
		while (Width--)
		{
			dest[1024 * pLeft->Y + X].r = dest[1024 * pLeft->Y + X].g = dest[1024 * pLeft->Y + X].b = 1.0f;

			dest[1024 * pLeft->Y + X].a = OneOverZ;

			OneOverZ += Gradients.dOneOverZdX;

			X++;
		}
	}
}

void TextureMapTriangle(Pixel* dest, POINT3D const *pVertices)
{
	int Top, Middle, Bottom;
	int MiddleCompare, BottomCompare;
	double Y0 = pVertices[0].Y;
	double Y1 = pVertices[1].Y;
	double Y2 = pVertices[2].Y;
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

	gradients Gradients(pVertices);
	edge TopToBottom(Gradients, pVertices, Top, Bottom);
	edge TopToMiddle(Gradients, pVertices, Top, Middle);
	edge MiddleToBottom(Gradients, pVertices, Middle, Bottom);
	edge *pLeft, *pRight;
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
		DrawScanLine(dest, Gradients, pLeft, pRight);
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
		DrawScanLine(dest, Gradients, pLeft, pRight);
		MiddleToBottom.Step(); TopToBottom.Step();
	}
}