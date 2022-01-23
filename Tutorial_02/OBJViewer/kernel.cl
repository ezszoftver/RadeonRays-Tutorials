#ifndef KERNEL_CL
#define KERNEL_CL

typedef struct
{
    float m11;
    float m12;
    float m13;
    float m14;

    float m21;
    float m22;
    float m23;
    float m24;

    float m31;
    float m32;
    float m33;
    float m34;

    float m41;
    float m42;
    float m43;
    float m44;
}
Matrix4x4;

float2 ToFloat2(float x, float y)
{
    float2 ret;
    ret.x = x;
    ret.y = y;
    return ret;
}

float3 ToFloat3(float x, float y, float z)
{
    float3 ret;
    ret.x = x;
    ret.y = y;
    ret.z = z;
    return ret;
}

float4 ToFloat4(float x, float y, float z, float w)
{
    float4 ret;
    ret.x = x;
    ret.y = y;
    ret.z = z;
    ret.w = w;
    return ret;
}

float4 Mult_Matrix4x4Float3(Matrix4x4 T, float3 v, float w)
{
    float4 ret;

    float4 v1 = ToFloat4(v.x, v.y, v.z, w);

    ret.x = dot(ToFloat4(T.m11, T.m12, T.m13, T.m14), v1);
    ret.y = dot(ToFloat4(T.m21, T.m22, T.m23, T.m24), v1);
    ret.z = dot(ToFloat4(T.m31, T.m32, T.m33, T.m34), v1);
    ret.w = dot(ToFloat4(T.m41, T.m42, T.m43, T.m44), v1);

    return ret;
}

float4 Mult_Matrix4x4Float4(Matrix4x4 T, float4 v)
{
    float4 ret;

    ret.x = dot(ToFloat4(T.m11, T.m12, T.m13, T.m14), v);
    ret.y = dot(ToFloat4(T.m21, T.m22, T.m23, T.m24), v);
    ret.z = dot(ToFloat4(T.m31, T.m32, T.m33, T.m34), v);
    ret.w = dot(ToFloat4(T.m41, T.m42, T.m43, T.m44), v);

    return ret;
}

Matrix4x4 Mult_Matrix4x4Matrix4x4(Matrix4x4 T2, Matrix4x4 T1)
{
    Matrix4x4 ret;

    float4 T1row1 = ToFloat4(T1.m11, T1.m12, T1.m13, T1.m14);
    float4 T1row2 = ToFloat4(T1.m21, T1.m22, T1.m23, T1.m24);
    float4 T1row3 = ToFloat4(T1.m31, T1.m32, T1.m33, T1.m34);
    float4 T1row4 = ToFloat4(T1.m41, T1.m42, T1.m43, T1.m44);

    float4 T2column1 = ToFloat4(T2.m11, T2.m21, T2.m31, T2.m41);
    float4 T2column2 = ToFloat4(T2.m12, T2.m22, T2.m32, T2.m42);
    float4 T2column3 = ToFloat4(T2.m13, T2.m23, T2.m33, T2.m43);
    float4 T2column4 = ToFloat4(T2.m14, T2.m24, T2.m34, T2.m44);

    float m11 = dot(T1row1, T2column1);
    float m12 = dot(T1row1, T2column2);
    float m13 = dot(T1row1, T2column3);
    float m14 = dot(T1row1, T2column4);
                
    float m21 = dot(T1row2, T2column1);
    float m22 = dot(T1row2, T2column2);
    float m23 = dot(T1row2, T2column3);
    float m24 = dot(T1row2, T2column4);
                
    float m31 = dot(T1row3, T2column1);
    float m32 = dot(T1row3, T2column2);
    float m33 = dot(T1row3, T2column3);
    float m34 = dot(T1row3, T2column4);
                
    float m41 = dot(T1row4, T2column1);
    float m42 = dot(T1row4, T2column2);
    float m43 = dot(T1row4, T2column3);
    float m44 = dot(T1row4, T2column4);

    ret.m11 = m11;
    ret.m12 = m12;
    ret.m13 = m13;
    ret.m14 = m14;

    ret.m21 = m21;
    ret.m22 = m22;
    ret.m23 = m23;
    ret.m24 = m24;

    ret.m31 = m31;
    ret.m32 = m32;
    ret.m33 = m33;
    ret.m34 = m34;

    ret.m41 = m41;
    ret.m42 = m42;
    ret.m43 = m43;
    ret.m44 = m44;

    return ret;
}

typedef struct
{
    float x;
    float y;
    float z;
}
Vector3;

typedef struct
{
	float v3PositionX;
	float v3PositionY;
	float v3PositionZ;
    float v3NormalX;
	float v3NormalY;
	float v3NormalZ;
    float v2TextCoordX;
	float v2TextCoordY;
    float v3ColorX;
	float v3ColorY;
	float v3ColorZ;
}
Vertex;

typedef struct
{
    /// xyz - origin, w - max range
    float4 o;
    /// xyz - direction, w - time
    float4 d;
    /// x - ray mask, y - activity flag
    int2 extra;
    /// Padding
    float2 padding;
} Ray;

typedef struct _Intersection
{
    // id of a shape
    int shapeid;
    // Primitive index
    int primid;
    // Padding elements
    int padding0;
    int padding1;
        
    // uv - hit barycentrics, w - ray distance
    float4 uvwt;
} Intersection;

__kernel void VertexShader(__global Vertex *inVertex, __global Vertex *outVertex, __global Ray* rays, int nNumVertices, int nWidth, int nHeight, Matrix4x4 mWorld, Vector3 v3CameraPos, Vector3 v3CameraAt, Vector3 v3DirLight)
{
	int pixelx = get_global_id(0);
    	int pixely = get_global_id(1);
    	if (pixelx >= nWidth || pixely >= nHeight)
    	{
        	return;
    	}
    	int id = (nWidth * pixely) + pixelx;
	//int id = get_global_id(0);
	
	if (id >= nNumVertices)
	{
		return;
	}
	
	float3 inPos = ToFloat3(inVertex[id].v3PositionX, inVertex[id].v3PositionY, inVertex[id].v3PositionZ);
	float3 pos = Mult_Matrix4x4Float3(mWorld, inPos, 1.0f).xyz;
	
	float3 inNormal = ToFloat3(inVertex[id].v3NormalX, inVertex[id].v3NormalY, inVertex[id].v3NormalZ);
	float3 normal = normalize( Mult_Matrix4x4Float3(mWorld, inNormal, 0.0f).xyz );
	
	outVertex[id].v3PositionX  = pos.x;
	outVertex[id].v3PositionY  = pos.y;
	outVertex[id].v3PositionZ  = pos.z;
	outVertex[id].v3NormalX    = normal.x;
	outVertex[id].v3NormalY    = normal.y;
	outVertex[id].v3NormalZ    = normal.z;
	outVertex[id].v2TextCoordX = inVertex[id].v2TextCoordX;
	outVertex[id].v2TextCoordY = inVertex[id].v2TextCoordY;
	
	float3 cameraDir = normalize(ToFloat3(v3CameraAt.x, v3CameraAt.y, v3CameraAt.z) - ToFloat3(v3CameraPos.x, v3CameraPos.y, v3CameraPos.z));
	
	/* drop behind */
	if (dot(pos - ToFloat3(v3CameraPos.x, v3CameraPos.y, v3CameraPos.z), cameraDir) < 0.0f)
	{
		return;
	}
	
	/* cull face */
	if (dot(normal, -cameraDir) < 0.0f)
	{
		return;
	}
	
	/* diffuse lighting */
	float3 toLight = - normalize(ToFloat3(v3DirLight.x, v3DirLight.y, v3DirLight.z));
	float diffuseIntensity = max(dot(toLight, normal), 0.3f);
	outVertex[id].v3ColorX = diffuseIntensity;
	outVertex[id].v3ColorY = diffuseIntensity;
	outVertex[id].v3ColorZ = diffuseIntensity;
	
	/* shadow, if near */
	if ( length(pos - ToFloat3(v3CameraPos.x, v3CameraPos.y, v3CameraPos.z)) < 1000.0f )
	{
		rays[id].o.xyz = pos + (toLight * 0.001f);
		rays[id].d.xyz = toLight;
		rays[id].o.w = 1000.0f;
		rays[id].extra.x = 0xFFFFFFFF;
		rays[id].extra.y = 0xFFFFFFFF;
	}
}

__kernel void ShadowShader(__global Vertex *outVertex, __global Intersection *intersection, int nNumVertices, int nWidth, int nHeight)
{
	int pixelx = get_global_id(0);
    	int pixely = get_global_id(1);
    	if (pixelx >= nWidth || pixely >= nHeight)
    	{
        	return;
    	}
    	int id = (nWidth * pixely) + pixelx;
	//int id = get_global_id(0);
	
	if (id >= nNumVertices)
	{
		return;
	}
	
	int shape_id = intersection[id].shapeid;
    	int prim_id = intersection[id].primid;
	float dist = intersection[id].uvwt.w;

    	if (shape_id != -1 && prim_id != -1)
	{
		outVertex[id].v3ColorX = min(0.3f + (0.03f * dist), outVertex[id].v3ColorX);
		outVertex[id].v3ColorY = min(0.3f + (0.03f * dist), outVertex[id].v3ColorY);
		outVertex[id].v3ColorZ = min(0.3f + (0.03f * dist), outVertex[id].v3ColorZ);
	}
}

#endif //KERNEL_CL
