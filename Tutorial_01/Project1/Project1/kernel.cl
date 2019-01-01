/**********************************************************************
 Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ********************************************************************/
#ifndef KERNEL_CL
#define KERNEL_CL

#define EPSILON 0.001f

typedef struct _Ray
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

typedef struct _Camera
    {
        // Camera coordinate frame
		float3 pos;
        float3 at;
        float3 up;
        
        float zfar;
    } Camera;

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

float4 ConvertFromBarycentric(__global const float* vec, 
                            __global const int* ind, 
                            int prim_id, 
                            __global const float4* uvwt)
{
    float4 a = (float4)(vec[ind[prim_id * 3] * 3],
                        vec[ind[prim_id * 3] * 3 + 1],
                        vec[ind[prim_id * 3] * 3 + 2], 0.f);

    float4 b = (float4)(vec[ind[prim_id * 3 + 1] * 3],
                        vec[ind[prim_id * 3 + 1] * 3 + 1],
                        vec[ind[prim_id * 3 + 1] * 3 + 2], 0.f);

    float4 c = (float4)(vec[ind[prim_id * 3 + 2] * 3],
                        vec[ind[prim_id * 3 + 2] * 3 + 1],
                        vec[ind[prim_id * 3 + 2] * 3 + 2], 0.f);
    return a * (1 - uvwt->x - uvwt->y) + b * uvwt->x + c * uvwt->y;
}

float3 scale(float3 point, float scale)
{
	float3 ret;
	ret.x = point.x * scale;
	ret.y = point.y * scale;
	ret.z = point.z * scale;
	return ret;
}

__kernel void GeneratePerspectiveRays(__global Ray* rays,
                                    __global const Camera* cam,
                                    int width,
                                    int height)
{
    int2 globalid;
    globalid.x  = get_global_id(0);
    globalid.y  = get_global_id(1);

    // Check borders
    if (globalid.x < width && globalid.y < height)
    {
		float stepPerPixel = 2.0f / (float)height;
		
		float3 dir = normalize(cam->at - cam->pos);
		float3 up = cam->up;
		float3 right = cross(dir, up);
		
		int movePixelX = globalid.x - (width / 2);
		int movePixelY = globalid.y - (height / 2);
		
		float3 moveRight = scale(right, stepPerPixel * movePixelX);
		float3 moveUp = scale(up, stepPerPixel * movePixelY);

		float3 dir2 = dir + moveRight + moveUp;

        int k = globalid.y * width + globalid.x;
        rays[k].o.xyz = cam->pos;
		rays[k].o.w = cam->zfar;
		rays[k].d.xyz = dir2;
		rays[k].d.w = 0;
		rays[k].extra.x = 0xFFFFFFFF;
        rays[k].extra.y = 0xFFFFFFFF;
    }
}

__kernel void GenerateShadowRays(__global Ray* rays,
                            //scene
                            __global float* positions,
                            __global float* normals,
                            __global int* ids,
                            __global float* colors,
                            __global int* indents,
                            //intersection
                            __global Intersection* isect,
                            //light pos
                            float4 light,
                            //window size
                            int width,
                            int height)
{
    int2 globalid;
    globalid.x  = get_global_id(0);
    globalid.y  = get_global_id(1);

    // Check borders
    if (globalid.x < width && globalid.y < height)
    {
        int k = globalid.y * width + globalid.x;
        int shape_id = isect[k].shapeid;
        int prim_id = isect[k].primid;

        // Need shadow rays only for intersections
        if (shape_id == -1 || prim_id == -1)
        {
           return;
        }
        
        // Calculate position and normal of the intersection point
        int ind = indents[shape_id];
        float4 pos = ConvertFromBarycentric(positions + ind*3, ids + ind, prim_id, &isect[k].uvwt);
        float4 norm = ConvertFromBarycentric(normals + ind*3, ids + ind, prim_id, &isect[k].uvwt);
        norm = normalize(norm);

        float4 dir = light - pos;
        rays[k].d = normalize(dir);
        rays[k].o = pos + norm * EPSILON;
        rays[k].o.w = length(dir);

        rays[k].extra.x = 0xFFFFFFFF;
        rays[k].extra.y = 0xFFFFFFFF;
   }
}

__kernel void Shading(//scene
                __global float* positions,
                __global float* normals,
                __global int* ids,
                __global float* colors,
                __global int* indents,
                //intersection
                __global Intersection* isect,
                //light pos
                float4 light,
                int width,
                int height,
                __global unsigned char* out)
{
    int2 globalid;
    globalid.x  = get_global_id(0);
    globalid.y  = get_global_id(1);

    // Check borders
    if (globalid.x < width && globalid.y < height)
    {
        int k = globalid.y * width + globalid.x;
        int shape_id = isect[k].shapeid;
        int prim_id = isect[k].primid;

        {
            // Calculate position and normal of the intersection point
            int ind = indents[shape_id];

            float4 pos = ConvertFromBarycentric(positions + ind*3, ids + ind, prim_id, &isect[k].uvwt);
            float4 norm = ConvertFromBarycentric(normals + ind*3, ids + ind, prim_id, &isect[k].uvwt);
            norm = normalize(norm);

            //triangle diffuse color
            int color_id = ind + prim_id*3;
            float4 diff_col = (float4)( colors[color_id],
                                        colors[color_id + 1],
                                        colors[color_id + 2], 1.f);

            // Calculate lighting
            float4 col = (float4)( 0.f, 0.f, 0.f, 0.f );
            float4 light_dir = normalize(light - pos);
            float dot_prod = dot(norm, light_dir);
            if (dot_prod > 0)
                col += dot_prod * diff_col;

            out[k * 4] = col.x * 255;
            out[k * 4 + 1] = col.y * 255;
            out[k * 4 + 2] = col.z * 255;
            out[k * 4 + 3] = 255;
        }
    }
}


#endif //KERNEL_CL
