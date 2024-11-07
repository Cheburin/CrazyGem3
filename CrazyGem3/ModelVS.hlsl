cbuffer cbMain : register( b0 )
{
	matrix    g_mWorld;                         // World matrix
	matrix    g_mView;                          // View matrix
	matrix    g_mProjection;                    // Projection matrix
	matrix    g_mWorldViewProjection;           // WVP matrix
	matrix    g_mWorldView;                     // WV matrix
	matrix    g_mInvView;                       // Inverse of view matrix

	matrix    g_mObject1;                // VP matrix
	matrix    g_mObject1WorldView;                       // Inverse of view matrix
	matrix    g_mObject1WorldViewProjection;                       // Inverse of view matrix

	matrix    g_mObject2;                // VP matrix
	matrix    g_mObject2WorldView;                       // Inverse of view matrix
	matrix    g_mObject2WorldViewProjection;                       // Inverse of view matrix

	float4    g_vFrustumNearFar;              // Screen resolution
	float4    g_vFrustumParams;              // Screen resolution
	float4    g_viewLightPos;                   //
};

cbuffer cbRay : register( b1 )
{
	float4    g_ray_s;
	float4    g_ray_v;
};

struct PosNormalTex2d
{
//    float3 pos : SV_Position;
//    float3 normal   : NORMAL;
//    float2 tex      : TEXCOORD0;
    float3 pos : SV_Position;
    float3 normal   : NORMAL;
	float4 tangent  : TANGENT;
	float4 color    : COLOR0;
    float2 tex      : TEXCOORD0;
};

struct PosNormalTangentTex2d
{
    float3 pos : SV_Position;
    float3 normal   : NORMAL;
	float4 tangent  : TANGENT;
	float4 color    : COLOR0;
    float2 tex      : TEXCOORD0;
};

struct ClipPosNormalTangentTex2d
{
    float4 pos : SV_POSITION;
    float3 normal   : NORMAL;
	float4 tangent  : TANGENT;
	float4 color    : COLOR0;
    float2 tex      : TEXCOORD0;
};

struct ClipPosColor
{
    float4 color          : TEXCOORD1;
    float4 clip_pos       : SV_POSITION; // Output position
};

///////////////////////////////////////////////////////////////////////////////////////////////////

struct GROUND_VS_OUTPUT
{
    float2 tex            : TEXCOORD1;
    float4 clip_pos       : SV_POSITION; // Output position
};

GROUND_VS_OUTPUT GROUND_VS( in PosNormalTex2d i )
{
    GROUND_VS_OUTPUT output;

    output.tex = i.tex;

    output.clip_pos = mul( float4( i.pos, 1.0 ), g_mWorldViewProjection );
    
    return output;
}; 
///////////////////////////////////////////////////////////////////////////////////////////////////

struct SKY_VS_OUTPUT
{
	float3 tex            : TEXCOORD1;
    float4 clip_pos       : SV_POSITION; // Output position
};

SKY_VS_OUTPUT SKY_VS( in PosNormalTex2d i )
{
    SKY_VS_OUTPUT output;

    output.clip_pos = mul( float4( i.pos, 1.0 ), g_mWorldViewProjection ).xyww;
    
	output.tex = i.pos;

    return output;
}; 

/////////////////////////////////////////////////////////////////////////////////////////////////////

struct BOX_VS_OUTPUT
{
    float4 clip_pos       : SV_POSITION; // Output position
};

BOX_VS_OUTPUT BOX_VS(uint VertexID : SV_VERTEXID)
{
	BOX_VS_OUTPUT output;

	output.clip_pos = float4(0,0,0,1);

    return output;	
}

static const float4 cubeVerts[8] = 
{
	float4(0, 0, 0, 1),// LB  0
	float4(0, 1, 0, 1), // LT  1
	float4(1, 0, 0, 1), // RB  2
	float4(1, 1, 0, 1),  // RT  3
	float4(0, 0, 1, 1), // LB  4
	float4(0, 1, 1, 1),  // LT  5
	float4(1, 0, 1, 1),  // RB  6
	float4(1, 1, 1, 1)    // RT  7
};

static const int cubeIndices[24] =
{
	0, 1, 2, 3, // front
	7, 6, 3, 2, // right
	7, 5, 6, 4, // back
	4, 0, 6, 2, // bottom
	1, 0, 5, 4, // left
	3, 1, 7, 5  // top
};

[maxvertexcount(36)]
void BOX_GS(point BOX_VS_OUTPUT pnt[1], uint primID : SV_PrimitiveID,  inout TriangleStream<BOX_VS_OUTPUT> triStream )
{
	BOX_VS_OUTPUT v[8];
	[unroll]
	for (int j = 0; j < 8; j++)
	{
		v[j].clip_pos = mul(cubeVerts[j], g_mWorldViewProjection);
	}
	
	[unroll]
	for (int i = 0; i < 6; i++)
	{
		triStream.Append(v[cubeIndices[i * 4 + 0]]);
		triStream.Append(v[cubeIndices[i * 4 + 1]]);
		triStream.Append(v[cubeIndices[i * 4 + 2]]);
		triStream.Append(v[cubeIndices[i * 4 + 3]]);
		triStream.RestartStrip();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

struct  FIGURE_VS_OUTPUT
{
    float4 clip_pos       : SV_POSITION;
	float3 t       		  : TEXCOORD0;
	float3 b       		  : TEXCOORD1;
	float3 n       		  : TEXCOORD2;
	float2 uv      	   	  : TEXCOORD3;

	float3 lightPos       : TEXCOORD4;
	float3 vertexPos      : TEXCOORD5;

	float3 vertexPosRelToLight : TEXCOORD6;
	float4 renderDepthPass     : TEXCOORD7;
};

FIGURE_VS_OUTPUT FIGURE_VS( in PosNormalTangentTex2d i )
{
    FIGURE_VS_OUTPUT output;

    float3 lightPos = float3( 0, 10, 20 );

    output.clip_pos = mul( float4( i.pos, 1.0 ), g_mWorldViewProjection );

	output.vertexPos = mul( float4( i.pos, 1.0 ), g_mWorldView ).xyz;

	output.vertexPosRelToLight = mul( float4( i.pos, 1.0 ), g_mWorld ).xyz - lightPos;

	output.lightPos = mul( float4( lightPos, 1.0 ), g_mView ).xyz;
    
	output.n = normalize(mul(float4(i.normal.xyz,  0), g_mWorldView).xyz);
	output.t = normalize(mul(float4(i.tangent.xyz, 0), g_mWorldView).xyz);
	output.b = normalize(cross(output.n, output.t))*i.tangent.w;
	
	output.uv = i.tex;

    output.renderDepthPass = float4(0,0,0,0);

    return output;
}; 
///////////////////////////////////////////////////////////////////////////////////////////////////

struct RAY_VS_OUTPUT
{
    float4 clip_pos       : SV_POSITION; // Output position
};

RAY_VS_OUTPUT RAY_VS(uint VertexID : SV_VERTEXID)
{
	RAY_VS_OUTPUT output;

	output.clip_pos = float4(0,0,0,1);

    return output;	
}

RAY_VS_OUTPUT CREATE_RAY_VS_OUTPUT(float4 arg){
	RAY_VS_OUTPUT output;

	output.clip_pos = arg;

	return output;
}

[maxvertexcount(3)]
void RAY_GS(point RAY_VS_OUTPUT pnt[1], uint primID : SV_PrimitiveID,  inout TriangleStream<RAY_VS_OUTPUT> triStream )
{
	float4 p = g_ray_s + 1000.0*g_ray_v;
	p = mul(p, g_mWorldView);
	triStream.Append(CREATE_RAY_VS_OUTPUT(mul(g_ray_s, g_mWorldViewProjection)));
	triStream.Append(CREATE_RAY_VS_OUTPUT(mul(float4(p.xyz,1), g_mProjection)));
	triStream.Append(CREATE_RAY_VS_OUTPUT(mul(g_ray_s, g_mWorldViewProjection)));
	triStream.RestartStrip();
}
///////////////////////////////////////////////////////////////////////////////////////////////////

struct  SPHERE_VS_OUTPUT
{
    float4 clip_pos       : SV_POSITION; // Output position
};

SPHERE_VS_OUTPUT SPHERE_VS( in PosNormalTex2d i )
{
    SPHERE_VS_OUTPUT output;

    output.clip_pos = mul( float4( i.pos, 1.0 ), g_mWorldViewProjection );
    
    return output;
}; 
///////////////////////////////////////////////////////////////////////////////////////////////////
///Render TBN
ClipPosNormalTangentTex2d RENDER_TBN_VS( in PosNormalTangentTex2d i )
{
    ClipPosNormalTangentTex2d output;

	output.pos = float4(i.pos,1);
	output.normal = i.normal;
	output.tangent = i.tangent;

	return output;
}

void axis(in float4 color, in float4 position, in float4 direction, inout LineStream<ClipPosColor> SpriteStream){
    ClipPosColor output;

    output.color = color;
    output.clip_pos = mul(mul( position, g_mView ), g_mProjection );
    SpriteStream.Append( output );

    output.color = color;
    output.clip_pos = mul(mul(position + .5 * direction, g_mView ), g_mProjection );
    SpriteStream.Append( output );

    SpriteStream.RestartStrip();
}

[maxvertexcount(2)]
void RENDER_TBN_GS(point ClipPosNormalTangentTex2d input[1], uint primID : SV_PrimitiveID, inout LineStream<ClipPosColor> SpriteStream){
  float3 n = normalize(mul(input[0].normal.xyz,  (float3x3)g_mWorld));
  float3 t = normalize(mul(input[0].tangent.xyz, (float3x3)g_mWorld));

  float3 b = normalize(cross(n, t))*input[0].tangent.w;	

  //axis(float4(1,0,0,1), mul(input[0].pos, g_mWorld), float4(t, 0), SpriteStream);
  //axis(float4(0,1,0,1), mul(input[0].pos, g_mWorld), float4(b, 0), SpriteStream);
  axis(float4(0,0,1,1), mul(input[0].pos, g_mWorld), float4(n, 0), SpriteStream);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct MODEL3_VS_OUTPUT
{
    float4 clip_pos       : SV_POSITION; // Output position
};

MODEL3_VS_OUTPUT MODEL3_VS(uint VertexID : SV_VERTEXID)
{
	MODEL3_VS_OUTPUT output;

	output.clip_pos = float4(0,0,0,1);

    return output;	
}

[maxvertexcount(3)]
void MODEL3_GS(point MODEL3_VS_OUTPUT pnt[1], uint primID : SV_PrimitiveID,  inout TriangleStream<MODEL3_VS_OUTPUT> Stream )
{
	MODEL3_VS_OUTPUT output;
    /*
	float d1 = 40;
	float d2 = 30;

	float top = -1.0*(200.0/1024)*2 + 1; 
	float bottom = -1.0*(400.0/1024)*2 + 1; 
	float left = (200.0/1024)*2 - 1; 
	float right = ((1024.0 - 200.0)/1024)*2 - 1; 

	output.clip_pos = float4(left*d1,top*d1,1,d1);
	Stream.Append(output);

	output.clip_pos = float4(right*d2,bottom*d2,1,d2);
	Stream.Append(output);
	*/

    float width = 1024;
	float d1 = 40;
	float d2 = 30;
	float d3 = 20;

	float left =        (200.0/width)*2 - 1; 
	float top =    -1.0*(200.0/width)*2 + 1; 

	float _left =       (50.0/width)*2 - 1; 
	float right =       (824.0/width)*2 - 1; 
	float bottom = -1.0*(400.0/width)*2 + 1; 


	output.clip_pos = float4(left*d1,top*d1,1,d1);
	Stream.Append(output);

	output.clip_pos = float4(right*d2,bottom*d2,1,d2);
	Stream.Append(output);

	output.clip_pos = float4(_left*d3,bottom*d3,1,d3);
	Stream.Append(output);

	Stream.RestartStrip();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
[maxvertexcount(2)]
void MODEL4_GS(point MODEL3_VS_OUTPUT pnt[1], uint primID : SV_PrimitiveID,  inout LineStream<MODEL3_VS_OUTPUT> Stream )
{
	MODEL3_VS_OUTPUT output;
	float width = 1024;

	float d1 = 40;
	float d2 = 30;

	float left1 =        (200.0/width)*2 - 1; 
	float top1 =    -1.0*(500.0/width)*2 + 1; 

	float left2 =        (824.0/width)*2 - 1; 
	float top2 =    -1.0*(700.0/width)*2 + 1; 

	output.clip_pos = float4(left1*d1,top1*d1,1,d1);
	Stream.Append(output);

	output.clip_pos = float4(left2*d2,top2*d2,1,d2);
	Stream.Append(output);

	Stream.RestartStrip();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct QUAD_VS_OUTPUT
{
    float4 clip_pos       : SV_POSITION; // Output position
};

QUAD_VS_OUTPUT QUAD3_VS(uint VertexID : SV_VERTEXID)
{
	QUAD_VS_OUTPUT output;

	output.clip_pos = float4(0,0,0,1);

    return output;	
}

[maxvertexcount(4)]
void QUAD3_GS(point QUAD_VS_OUTPUT pnt[1], uint primID : SV_PrimitiveID,  inout TriangleStream<QUAD_VS_OUTPUT> Stream )
{
	QUAD_VS_OUTPUT output;
    
	output.clip_pos = float4(1,1,0,1);
	Stream.Append(output);

	output.clip_pos = float4(1,-1,0,1);
	Stream.Append(output);

	output.clip_pos = float4(-1,1,0,1);
	Stream.Append(output);

	output.clip_pos = float4(-1,-1,0,1);
	Stream.Append(output);

	Stream.RestartStrip();
}