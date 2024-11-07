#include<windows.h>
#include<math.h>
#include <algorithm>
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

void TextureMapLine(Pixel* dest, POINT3D const *pVertices){
	Pixel* dst;

	float x0 = pVertices[0].X;
	float y0 = pVertices[0].Y;
	float z0 = 1.0 / pVertices[0].Z;
	float x1 = pVertices[1].X;
	float y1 = pVertices[1].Y;
	float z1 = 1.0 / pVertices[1].Z;

	float A = y1 - y0;
	float B = x0 - x1;

	float Xoff = 0.5 - (x0 - floor(x0));
	float Yoff = 1.0 - (y0 - floor(y0));

	int x = 0.5 + int(x0);
	int y = 0.5 + int(y0);
	double z;

	float D = A*Xoff + B*Yoff;
    
	//double A1 = (x1 - x0) / pow((x1 - x0) + (y1 - y0), 2);
	//double B1 = (y1 - y0) / pow((x1 - x0) + (y1 - y0), 2);
	//double C1 = (-(x0)*(x1 - x0) + -(y0)*(y1 - y0)) / pow((x1 - x0) + (y1 - y0), 2);

	for (; x < x1; x++) {
		if (D>0)
		{
			D = D + A + B;
			y++;
			//z = (A1 + B1) * (z1 - z0) + (z0 + C1 * (z1 - z0));
		}
		else
		{
			D = D + A;
			//z = A1 * (z1 - z0) + (z0 + C1 * (z1 - z0));
		}

		double PraX = x + 0.5 - x0 - 0.0;
		double PraY = y + 0.5 - y0 - 0.0;
		double PbaX = x1 - x0;
		double PbaY = y1 - y0;
		double t = (PraX*PbaX + PraY*PbaY) / (PbaX*PbaX + PbaY*PbaY);

		z = (1.0 - t)*z0 + t*z1;

		if (y < 1024 && x < 1024)
		{
			dst = &(dest[1024 * y + x]);
			dst->r = dst->g = dst->b = 1.0f;
			dst->a = z;
		}
	}
}