
SamplerState linearSampler : register( s0 );

Texture2D  texDiff : register( t0 );

Texture2D  texNormal : register( t1 );

TextureCube  texCube : register( t2 );

struct Targets
{
    float4 color: SV_Target0;
};

///////////////////////////////////////////////////////////////////////////////////////////////
Targets GROUND_PS(
    float2 tex            : TEXCOORD1,
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   Targets output;

   output.color = float4(0.5,0.5,0.5,1);

   return output;
};
///////////////////////////////////////////////////////////////////////////////////////////////
Targets SKY_PS(
    float3 tex            : TEXCOORD1,
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   Targets output;

   output.color = texCube.Sample(linearSampler, tex);

   return output;
};
///////////////////////////////////////////////////////////////////////////////////////////////

Targets BOX_PS(
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   Targets output;

   output.color = float4(0,0,1,0.5);

   return output;
};

///////////////////////////////////////////////////////////////////////////////////////////////
void lighting_calculations(
        float3 AmbiColor,
        float3 LightColor, 
        float3 LightPos, 
        float3 VertexPos, 
        float3 VertexNormal,

		out float3 DiffuseContrib
        )
{
    float3 Ln = normalize(LightPos - VertexPos);    
    float3 Nn = VertexNormal;
    float3 Vn = normalize( - VertexPos);
    float3 Hn = normalize(Vn + Ln);
    float4 litV = lit(dot(Ln,Nn),dot(Hn,Nn),0);
    
    DiffuseContrib = litV.y * LightColor + AmbiColor;
}
///////////////////////////////////////////////////////////////////////////////////////////////
float3x3 GetTBN(float3 t, float3 b, float3 n)
{
    float3 _t = normalize(t);
    float3 _b = normalize(b);
    float3 _n = normalize(n);    

    float3x3 tbn = { 
        _t.x, _t.y, _t.z,
	    _b.x, _b.y, _b.z,
	    _n.x, _n.y, _n.z,
    }; 

    return tbn;
}
float3 UnpackNormal(float3 n)
{
    return n * 2.0 - float3(1.0, 1.0, 1.0);
}
Targets FIGURE_PS(
    float4 clip_pos       : SV_POSITION,
	float3 t       : TEXCOORD0,
	float3 b       : TEXCOORD1,
	float3 n       : TEXCOORD2,
	float2 uv      : TEXCOORD3,

	float3 lightPos: TEXCOORD4,
	float3 fragPos:  TEXCOORD5,
   	float3 fragPosRelToLight : TEXCOORD6,

    float4 renderDepthPass   : TEXCOORD7
):SV_TARGET
{ 
   Targets output;

   float sampleDepth = .0;
   float projectDepth = .0;
   if(renderDepthPass.x == 0.)
   {
       sampleDepth = texCube.Sample(linearSampler, fragPosRelToLight).r;
       float maxFragPosRelToLightCoord = max( abs( fragPosRelToLight.x ), max( abs( fragPosRelToLight.y ), abs( fragPosRelToLight.z ) ) );
       // the math is: -1.0f / maxCoord * (Zn * Zf / (Zf - Zn) + Zf / (Zf - Zn) (should match the shadow projection matrix)
       projectDepth =  -(1.0f / maxFragPosRelToLightCoord) * (1000.0 * 0.1 / (1000.0 - 0.1)) + (1000.0 / (1000.0 - 0.1)) - 0.00001;
   }

   float3 DiffuseContrib;
   if(projectDepth > sampleDepth)
   {
       DiffuseContrib = float3(0.,0.,0.);
   }
   else
   {   
       float3 fragNormal = UnpackNormal(texNormal.Sample(linearSampler, uv).xyz);
       fragNormal = normalize(fragNormal);
       fragNormal = mul(fragNormal, GetTBN(t,b,n));
       fragNormal = normalize(fragNormal);

       float3 AmbiColor = float3(0.,0.,0.);
       float3 LightColor = float3(1.,1.,1.);

       lighting_calculations(AmbiColor, LightColor, lightPos, fragPos, fragNormal, DiffuseContrib);
   }

   float3 MaterialColor = float3(1.0,1.0,1.0);
   output.color = float4(DiffuseContrib*MaterialColor,1);

   return output;
};
///////////////////////////////////////////////////////////////////////////////////////////////
Targets BENCH_PS(
    float4 clip_pos       : SV_POSITION,
	float3 t       : TEXCOORD0,
	float3 b       : TEXCOORD1,
	float3 n       : TEXCOORD2,
	float2 uv      : TEXCOORD3,

	float3 lightPos: TEXCOORD4,
	float3 fragPos: TEXCOORD5,
   	float3 fragPosRelToLight : TEXCOORD6,

    float4 renderDepthPass   : TEXCOORD7
):SV_TARGET
{
   Targets output;

   float sampleDepth = .0;
   float projectDepth = .0;
   if(renderDepthPass.x == 0.)
   {
       sampleDepth = texCube.Sample(linearSampler, fragPosRelToLight).r;
       float maxFragPosRelToLightCoord = max( abs( fragPosRelToLight.x ), max( abs( fragPosRelToLight.y ), abs( fragPosRelToLight.z ) ) );
       // the math is: -1.0f / maxCoord * (Zn * Zf / (Zf - Zn) + Zf / (Zf - Zn) (should match the shadow projection matrix)
       projectDepth =  -(1.0f / maxFragPosRelToLightCoord) * (1000.0 * 0.1 / (1000.0 - 0.1)) + (1000.0 / (1000.0 - 0.1)) - 0.00001;
   }

   float3 DiffuseContrib;
   if(projectDepth > sampleDepth)
   {
       DiffuseContrib = float3(0.,0.,0.);
   }
   else
   {
       float3 fragNormal = normalize(n);

       float3 AmbiColor = float3(0.,0.,0.);
       float3 LightColor = float3(1.,1.,1.);

       lighting_calculations(AmbiColor, LightColor, lightPos, fragPos, fragNormal, DiffuseContrib);
   }

   float3 MaterialColor = float3(1.0,1.0,1.0);
   output.color = float4(DiffuseContrib*MaterialColor,1);

   return output;
}
///////////////////////////////////////////////////////////////////////////////////////////////

Targets RAY_PS(
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   Targets output;

   output.color = float4(1,0,0,1);

   return output;
};


Targets RENDER_TBN_PS(
    float4 color          : TEXCOORD1,
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   Targets output;

   output.color = color;

   return output;
};

///////////////////////////////////////////////////////////////////////////////////////////////
Targets MODEL3_PS(
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   Targets output;

   output.color = float4(0.5,0.5,0.5,1);

   return output;
};

///////////////////////////////////////////////////////////////////////////////////////////////
Targets QUAD3_PS(
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   Targets output;

   output.color = texDiff.Sample(linearSampler, float2(1.0/1024.0*clip_pos.xy));

   return output;
};