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
#include "radeon_rays_cl.h"
#include "CLW.h"

#include <GL/glew.h>
#include <GL/GLUT.h>
#include <cassert>
#include <iostream>
#include <memory>
#include "shader_manager.h"
#include "tiny_obj_loader.h"
#include "FreeImage.h"

using namespace RadeonRays;
using namespace tinyobj;

namespace {
    std::vector<shape_t> g_objshapes;
    std::vector<material_t> g_objmaterials;
    GLuint g_vertex_buffer, g_index_buffer;
    GLuint g_texture = 0;
    int g_window_width = 640;
    int g_window_height = 480;
    std::unique_ptr<ShaderManager> g_shader_manager;
    
    IntersectionApi* g_api;

    //CL data
    CLWContext g_context;
    CLWProgram g_program;
    CLWBuffer<float> g_positions;
    CLWBuffer<float> g_normals;
	CLWBuffer<float> g_texcoords;
    CLWBuffer<int> g_indices;
    CLWBuffer<float> g_colors;
    CLWBuffer<int> g_indent;
	CLWImage2D image;

    struct Camera
    {
        // Camera coordinate frame
		float3 pos;
        float3 at;
        float3 up;
        
        // far Z
        float zfar;
    };

	clock_t elapsedTime, currentTime, dt, sec = 0;
	int fps = 0;
}



CLWImage2D texture;

void InitData()
{
    //Load
    std::string basepath = "./"; 
    std::string filename = basepath + "House.obj";
    std::string res = LoadObj(g_objshapes, g_objmaterials, filename.c_str(), basepath.c_str());
    if (res != "")
    {
        throw std::runtime_error(res);
    }

    // Load data to CL
    std::vector<float> verts;
    std::vector<float> normals;
	std::vector<float> texcoords;
    std::vector<int> inds;
    std::vector<float> colors;
    std::vector<int> indents;
    int indent = 0;

    for (int id = 0; id < g_objshapes.size(); ++id)
    {
        const mesh_t& mesh = g_objshapes[id].mesh;
        verts.insert(verts.end(), mesh.positions.begin(), mesh.positions.end());
        normals.insert(normals.end(), mesh.normals.begin(), mesh.normals.end());
		texcoords.insert(texcoords.end(), mesh.texcoords.begin(), mesh.texcoords.end());
        inds.insert(inds.end(), mesh.indices.begin(), mesh.indices.end());
        for (int mat_id : mesh.material_ids)
        {
            const material_t& mat = g_objmaterials[mat_id];
            colors.push_back(mat.diffuse[0]);
            colors.push_back(mat.diffuse[1]);
            colors.push_back(mat.diffuse[2]);
        }
        
        indents.push_back(indent);
        indent += mesh.indices.size();
    }
    g_positions = CLWBuffer<float>::Create(g_context, CL_MEM_READ_ONLY, verts.size(), verts.data());
    g_normals = CLWBuffer<float>::Create(g_context, CL_MEM_READ_ONLY, normals.size(), normals.data());
	g_texcoords = CLWBuffer<float>::Create(g_context, CL_MEM_READ_ONLY, texcoords.size(), texcoords.data());
    g_indices = CLWBuffer<int>::Create(g_context, CL_MEM_READ_ONLY, inds.size(), inds.data());
    g_colors = CLWBuffer<float>::Create(g_context, CL_MEM_READ_ONLY, colors.size(), colors.data());
    g_indent = CLWBuffer<int>::Create(g_context, CL_MEM_READ_ONLY, indents.size(), indents.data());

	texture = CLWImage2D::CreateFromFile(g_context, "house_tex.bmp");
}

float fullTime = 0.0f;
Camera cam;

Buffer* GeneratePrimaryRays()
{
	float rotateSpeed = 0.5f;
	fullTime += ((float)dt) / 1000.0f;

    //prepare camera buf
	cam.pos = { (sin(fullTime * rotateSpeed) * 10.0f), 3.f, (cos(fullTime * rotateSpeed) * 10.0f) };
    cam.at = {0.f, 1.f, 0.f };
    cam.up = { 0.f, 1.f, 0.f };
    cam.zfar = { 100.0f };
    CLWBuffer<Camera> camera_buf = CLWBuffer<Camera>::Create(g_context, CL_MEM_READ_ONLY, 1, &cam);

    //run kernel
    CLWBuffer<ray> ray_buf = CLWBuffer<ray>::Create(g_context, CL_MEM_READ_WRITE, g_window_width*g_window_height);
    CLWKernel kernel = g_program.GetKernel("GeneratePerspectiveRays");
    kernel.SetArg(0, ray_buf);
    kernel.SetArg(1, camera_buf);
    kernel.SetArg(2, g_window_width);
    kernel.SetArg(3, g_window_height);

    // Run generation kernel
    size_t gs[] = { static_cast<size_t>((g_window_width + 7) / 8 * 8), static_cast<size_t>((g_window_height + 7) / 8 * 8) };
    size_t ls[] = { 8, 8 };
    g_context.Launch2D(0, gs, ls, kernel);
    g_context.Flush(0);

    return CreateFromOpenClBuffer(g_api, ray_buf);
}

Buffer* Shading(const CLWBuffer<Intersection> &isect, const float3& light)
{
    //pass data to buffers
    CLWBuffer<unsigned char> out_buff = CLWBuffer<unsigned char>::Create(g_context, CL_MEM_READ_ONLY, 4*g_window_width*g_window_height);
    cl_float4 light_cl = { light.x,
                            light.y,
                            light.z,
                            light.w };
    //run kernel
    CLWBuffer<ray> ray_buf = CLWBuffer<ray>::Create(g_context, CL_MEM_READ_WRITE, g_window_width*g_window_height);
    CLWKernel kernel = g_program.GetKernel("Shading");
    kernel.SetArg(0, g_positions);
    kernel.SetArg(1, g_normals);
    kernel.SetArg(2, g_indices);
    kernel.SetArg(3, g_colors);
    kernel.SetArg(4, g_indent);
    kernel.SetArg(5, isect);
    //kernel.SetArg(6, occluds);
    kernel.SetArg(6, light_cl);
    kernel.SetArg(7, g_window_width);
    kernel.SetArg(8, g_window_height);
    kernel.SetArg(9, out_buff);
	kernel.SetArg(10, texture);
	kernel.SetArg(11, g_texcoords);

    // Run generation kernel
    size_t gs[] = { static_cast<size_t>((g_window_width + 7) / 8 * 8), static_cast<size_t>((g_window_height + 7) / 8 * 8) };
    size_t ls[] = { 8, 8 };
    g_context.Launch2D(0, gs, ls, kernel);
    g_context.Flush(0);

    return CreateFromOpenClBuffer(g_api, out_buff);
}

void DrawScene()
{
	// FPS
	fps++;
	elapsedTime = currentTime;
	currentTime = clock();

	dt = currentTime - elapsedTime;

	fps++;
	sec += dt;
	if (sec >= 1000)
	{
		char title[100];
		sprintf(title, "fps: %d\n", fps);
		glutSetWindowTitle((const char*)title);

		fps = 0;
		sec = 0;
	}

	const int k_raypack_size = g_window_height * g_window_width;
	// Prepare rays. One for each texture pixel.
	Buffer* ray_buffer = GeneratePrimaryRays();
	// Intersection data
	CLWBuffer<Intersection> isect_buffer_cl = CLWBuffer<Intersection>::Create(g_context, CL_MEM_READ_WRITE, g_window_width*g_window_height);
	Buffer* isect_buffer = CreateFromOpenClBuffer(g_api, isect_buffer_cl);

	// Intersection
	g_api->QueryIntersection(ray_buffer, k_raypack_size, isect_buffer, nullptr, nullptr);

	// Point light position
	float3 light = { cam.pos };

	// Shading
	Buffer* tex_buf = Shading(isect_buffer_cl, light);

	// Get image data
	std::vector<unsigned char> tex_data(k_raypack_size * 4);
	unsigned char* pixels = nullptr;
	Event* e = nullptr;
	g_api->MapBuffer(tex_buf, kMapRead, 0, 4 * k_raypack_size * sizeof(unsigned char), (void**)&pixels, &e);
	e->Wait();
	memcpy(tex_data.data(), pixels, 4 * k_raypack_size * sizeof(unsigned char));

	
	// bind gl texture
	glBindTexture(GL_TEXTURE_2D, g_texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_window_width, g_window_height, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
	glBindTexture(GL_TEXTURE_2D, NULL);



	g_api->DeleteBuffer(ray_buffer);
	g_api->DeleteBuffer(isect_buffer);
	g_api->DeleteBuffer(tex_buf);
	g_api->DeleteEvent(e);

    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, g_window_width, g_window_height);

    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_index_buffer);

    // shader data
    GLuint program = g_shader_manager->GetProgram("simple");
    glUseProgram(program);
    GLuint texloc = glGetUniformLocation(program, "g_Texture");
    assert(texloc >= 0);

    glUniform1i(texloc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_texture);

    GLuint position_attr = glGetAttribLocation(program, "inPosition");
    GLuint texcoord_attr = glGetAttribLocation(program, "inTexcoord");
    glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
    glVertexAttribPointer(texcoord_attr, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(position_attr);
    glEnableVertexAttribArray(texcoord_attr);

    // draw rectanle
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

    glDisableVertexAttribArray(texcoord_attr);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);

    //glFinish();
    glutSwapBuffers();
}

void InitGl()
{
    g_shader_manager.reset(new ShaderManager());

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glCullFace(GL_NONE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glGenBuffers(1, &g_vertex_buffer);
    glGenBuffers(1, &g_index_buffer);

    // create Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_index_buffer);

    float quad_vdata[] =
    {
        -1, -1, 0.5, 0, 0,
        1, -1, 0.5, 1, 0,
        1, 1, 0.5, 1, 1,
        -1, 1, 0.5, 0, 1
    };

    GLshort quad_idata[] =
    {
        0, 1, 3,
        3, 1, 2
    };

    // fill data
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vdata), quad_vdata, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_idata), quad_idata, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // texture
    glGenTextures(1, &g_texture);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_window_width, g_window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void InitCl()
{
    std::vector<CLWPlatform> platforms;
    CLWPlatform::CreateAllPlatforms(platforms);

    if (platforms.size() == 0)
    {
        throw std::runtime_error("No OpenCL platforms installed.");
    }

    for (int i = 0; i < platforms.size(); ++i)
    {
        for (int d = 0; d < (int)platforms[i].GetDeviceCount(); ++d)
        {
            if (platforms[i].GetDevice(d).GetType() != CL_DEVICE_TYPE_GPU)
                continue;

            g_context = CLWContext::Create(platforms[i].GetDevice(d));
            break;
        }

        if (g_context)
            break;
    }
    const char* kBuildopts(" -cl-mad-enable -cl-fast-relaxed-math -cl-std=CL1.2 -I . ");


	g_program = CLWProgram::CreateFromFile("kernel.cl", kBuildopts, g_context);
}

void Resize(int width, int height) 
{
	if (width < 1) { width = 1; }
	if (height < 1) { height = 1; }

	g_window_width = width;
	g_window_height = height;

	// texture
	if (0 != g_texture) 
	{
		glDeleteTextures(1, &g_texture);
		g_texture = 0;
	}

	glGenTextures(1, &g_texture);
	glBindTexture(GL_TEXTURE_2D, g_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_window_width, g_window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);
}

int main(int argc, char* argv[])
{
    // GLUT Window Initialization:
    glutInit(&argc, (char**)argv);
    glutInitWindowSize(g_window_width, g_window_height);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutCreateWindow("");
#ifndef __APPLE__
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cout << "GLEW initialization failed\n";
        return -1;
    }
#endif
    // Prepare rectangle for drawing texture
    // rendered using intersection results
    InitGl();

    InitCl();

    // Load CornellBox model
    InitData();

    // Create api using already exist opencl context
    cl_device_id id = g_context.GetDevice(0).GetID();
    cl_command_queue queue = g_context.GetCommandQueue(0);

    // Create intersection API
    g_api = RadeonRays::CreateFromOpenClContext(g_context, id, queue);
    
    // Adding meshes to tracing scene
    for (int id = 0; id < g_objshapes.size(); ++id)
    {
        shape_t& objshape = g_objshapes[id];
        float* vertdata = objshape.mesh.positions.data();
        int nvert = objshape.mesh.positions.size();
        int* indices = objshape.mesh.indices.data();
        int nfaces = objshape.mesh.indices.size() / 3;
        Shape* shape = g_api->CreateMesh(vertdata, nvert, 3 * sizeof(float), indices, 0, nullptr, nfaces);

        assert(shape != nullptr);
        g_api->AttachShape(shape);
        shape->SetId(id);
    }
    // Ñommit scene changes
    g_api->Commit();

    
	elapsedTime = currentTime = clock();
    

    // Start the main loop
	glutReshapeFunc(Resize);
	glutIdleFunc(DrawScene);
    glutDisplayFunc(DrawScene);
    glutMainLoop();

    // Cleanup
    IntersectionApi::Delete(g_api); g_api = nullptr;

    return 0;
}