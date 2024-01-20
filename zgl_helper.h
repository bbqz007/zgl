/**
MIT License

Copyright (c) 2022-2024 bbqz007 <https://github.com/bbqz007, http://www.cnblogs.com/bbqzsl>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __ZHELPER_GL2_H_
#define __ZHELPER_GL2_H_

//#define FEATURE_ZHELPER_GL2_USE_SOFTWARE
#ifdef FEATURE_ZHELPER_GL2_USE_SOFTWARE
#ifdef _WINDOWS
#if _WIN32 || __MINGW32__
#error "openglsw has __std apis and __cdecl apis, it is hell."
#endif // _WIN32 || __MINGW32__
#endif // _WINDOWS

#include <GL/gl.h>
#define GL_GLEXT_PROTOTYPES 1
#include <GL/glext.h>
#else
#include <GL/glew.h>
#endif  // FEATURE_ZHELPER_GL2_USE_SOFTWARE


/// Z#20220408 Design
///  categories on the docs at 'https://docs.gl'
/// 

/// this helper is not using any EXT, ARB, special vendor(NV, SGI, ATI ...) apis.

/// there are some concepts on my design
/// 1. texture and array buffer (VBO) are GPU Memories, data should be loaded from CPU Memories.
/// 2. VAO is the datasource, FBO is the datasink, and Shaders are the Filters.
/// 2.a VAO is like the Displaying Lists, but it bundles the Vertex Attributes rather than drawing.
/// X 2.b FBO is like the Displaying Lists, but it bundles the Textures.
/// 2.c VAO connect to Shaders via Vertex Attributes which is like to IPin between the ISouceFilter and the ShaderFilter
/// 3. Client Arrays are CPU Memories while VBOs are GPU Memories.
/// 4. glDrawArrays and glDrawElements have four modes:
/// 4.a not use shader program, Vertex Arrays use CPU Memories.
/// 4.b not use shader program, Vertex Arrays use GPU Memories.
/// 4.c use shader program, Vertex Attributes use CPU Memories.
/// 4.d use shader program, Vertex Attributes use GPU Memories.
/// 5. glEnableClientState makes the Vertex Arrays connect to GL server site.
///    while glEnableVertexAttribArray makes the Vertex Attributes connect to Shader.
/// 6. Vertex Arrays contains:
///     Vertex, Color, Normal, TexCoord, Indices(Elements), and etc.
/// 7. assume that, fixed-function pipeline has built-in shaders and they have fixed attributes (input ports).
/// 7.a those attributes have transparent layout locations.
/// 7.b so we use gl*Pointer with Named Attribute, to pin data to specified attribute, just like glVertexAttribPointer with location.
/// 7.c a attribtue like a IPin, we need to connect it to the pipeline,
///        using glEnableClientState with Named Attribute for fixed-function pipeline,
///        and using glEnableVertexAttribArray with location for programmable pipeline.
/// 7.d some time we do not want a input dataflow but just a constant value.
///        glColor, glVertex for fixed-function pipeline,
///        glVertexAttrib with location for programmable pipeline.

/// TODO: GL3
/// FBO and texture
/// overview FBO let you can draw content to texture. and until GL2 you can only draw texture to the sceen.
///  "http://www.songho.ca/opengl/gl_fbo.html"
/// 1. FBO is like a device, some like DC in win32. but it is a virtual device.
/// 1.a FBO has many (attachment) ports which the texture can connect to, the texture seems to be the backend storage to the FBO virtual device.
/// 1.a   by calling glFramebufferTexture2D
/// 1.b RenderBuffer can connect to FBO's ports.
/// 1.b   by calling glFramebufferRenderbuffer
/// 1.c FBO has READ/DRAW two ports for each (attachment) port. GL_FRAMEBUFFER include the two.
/// 2. we can specify which (attachment) ports to draw by call glDrawBuffers.
/// 3. there is a 'window-system-provided framebuffer', calling `glBindFramebuffer(GL_FRAMEBUFFER, 0);` to switch.
/// 3.a espacially it is DC in Win32.
/// 4. FBO is drawn by pipeline, and read by cpu calling glReadPixels or gpu calling glCopyTexSubImage2D.
///
/// texture
/// 1. you can not call glTex prefix functions to a texture target, until you bind it to a texture unit.
/// 1.a select a texture unit by calling glActiveTexture, always call glBindTexture next.
/// 2. the GPU Storage for a texture, is Image.
/// 2.a buffer texture, not use Image as storage, but the GPU Memory create by glGenBuffers. this texture can not call any gl*Image functions.
/// 2.b buffer textures are used as random accessable uniform.
/// 3. in GLSL, texture means sampler, and image means image. so after 3.2 core, glBindImageTexture tells the unit use image rather than texture.
///
/// texture.unit and texture.target
/// 1. any texture should be in a actived unit for (subsequent) uses.
/// 1.a glActiveTexture for any subsequent use of texture to FBO or GLSL.
/// 1.b GL2, if you want copy GL_FRONT to texture, you must call glActiveTexture.
/// 2. GL_TEXTURE_2D, etc. as factes of unit, you must select one.
/// 3. glClientActiveTexture is special for fixed-pipeline, if you want to glTexCoordPointer you should do this before this calling.
/// 3.a ie. glActiveTexture(GL_TEXTURE0) and glClientActiveTexture(GL_TEXTURE0), 
///         the subsequent glTexCoordPointer tells the fixed-pipeline GL_TEXTURE0 unit use TexCoord Array now i tell you.
/// 3.b difference to programable pipeline
///     which texcoord attribute array used for which sampler (must in one texture unit), that is your job in the glsl.
///
/// 3D:
/// 1. 1D = width
/// 2. 2D = width, height
/// 3. 3D = width, height, depth
///
/// FBO state:
/// 1. glReadBuffer and glDrawBuffer are neither read buffer nor draw buffer. they change the states on the read side of draw side.
/// 1.a glReadBuffer switch reading port of one specified attachment port of current FBO (device) on or off.
/// 1.b glDrawBuffer switch drawing port of one specified attachment port of current FBO (device) on or off.

/// pipeline:
///   http://spacesimulator.net/tutorials/OpenGL_Texture_Mapping_tutorial_3_3.html
///   https://math.hws.edu/graphicsbook/c6/s1.html
///  1. use shaders, means programable pipeline
///  2. otherwise, fixed pipeline.
///  X 3. before version 330 core, there is a fixed-function pipeline's shader, it has a special texture unit
///  X 3.a glEnable(GL_TEXTURE) to use special texture unit, no other glActiveTexture.
///  4. intel i7 2gen support 310(win) and 330(linux), and i7 3gen support 400. (https://en.wikipedia.org/wiki/List_of_Intel_graphics_processing_units)
///  5. GL2 can use fixed pipeline.
///  6. glVertexPxPointer is special for fixed pipeline, while glVertexAttribPointer is special for programable pipelines.
///  7. fixed pipeline has fixed attributes locations, and call glEnableClientState to enable the attribute via enum.
///  7.a programable pipelines have it's own attributes locations layouts, and call glEnableVertexAttribArray to enable the attribute via location.
///  8. fixed pipeline use fixed attributes function such as glVertexPointer, glColorPointer, etc. to pin attributes.
///  8.a programable pipelines treats all attributes as VertexArrays that the differences are only the locatin layouts, and use glVertexAttribPointer to pin attributes.

/// between cpu and gpu memory transfer:
///   https://github.com/sebbbi/perftest
/// 1. the alpha channel would be clamp to 1. you can use this channel for any other purpose than color vector.

/** GL_AMBIENT, GL_DIFFUSE, GL_SPECULAR, GL_POSITION, GL_SPOT_CUTOFF, GL_SPOT_DIRECTION, GL_SPOT_EXPONENT, GL_CONSTANT_ATTENUATION, GL_LINEAR_ATTENUATION, and GL_QUADRATIC_ATTENUATION*/
/// about light
/// 1. ambient, back light
/// 2. diffuse, sun light with position {x,y,z,0}, spot light with position {x,y,z,not 0}
/// 3. specular, high light
/// 4. diffuse as spot light:
/// 4.a cutoff, angle of spot light, 2 * angle, 180 espacially ignore directions.
/// 4.b directions
/// 4.c exponent, focus, range from 2 to 128, the bigger number the smaller focus.
/// 5. attenuation, intensity, normally more energy and more intensity. 
/// 5.a constant, k
/// 5.b linear, kl
/// 5.c quadratic, kq
/// 5.d 1/(k + kl*d + kq*d^2), d is distance
/// 6. glLight is a part of fixed functions.
/// 6.1 glLight only effects on fixed-function pipeline.

/// Coordinate system
/// 1. V[clip] = M[proj] * M[view] * M[model] * V[local]
/// 1.a V[local] = M[local] * V[local-ori]
/// 1.b proj: ortho or perspective
/// 1.c view: lookat
/// 1.d model: distance from worldlorigin, angle about world origin.
/// 1.e local: model vertex.
/// 2. normalized device coordinate (NDC) 
/// 2.a [0, 1]
/// 2.b texture
///      (0,0)...(1,0)
///      (0,1)...(1,1)
///      origin at left-top, x ->, and y down
///      suitable for 2D array.
/// 2.c render
///      (1,1)...(0,1)
///      (1,0)...(0,0)
///      origin at rtight-bottom, x <-, and y up
///
/// 2.d veiwport
///      (-1,-1)...( 1,-1)
///      |... ( 0, 0) ...|
///      (-1, 1)...( 1, 1)
///      origin at the center, x ->, and y down
/// 3. pixel coordinates 

/// PBO
/// Pixel Buffer Object
/// something like device memory mapped to cpu host's virtual memory. typically like mmap() maps the disk file to cpu virtual space.
/// cpu calling glTexSubImage or glGetTexImage does not access gpu memory, actually cpu host communicate with the gpu device.
/// cpu consume the gpu pipeline.
/// suppose there a pair of port between cpu and gpu, cpu read data from cpu memory to port, gpu accept data from port and write to gpu memory.
/// now map gpu memory to cpu virtual space. cpu write to the virtual space, DMA device take care of transfer. free the gpu.
/// that is the PBO.
/// gpu access PBOs on GPU memories. gpu does not to communicate to the far cpu.
/// cpu access the PBOs on GPU memories via DMA rather than GPU.
/// if your jobs are all sync in one only pipeline, there is no benifit.
/// you should simulate multiple pipelines.

/// data sink
/// OpenGL 2.x:
/// In OpenGL 2.x, shaders typically operate on the rendering buffer, which includes the front and back buffers.
/// Shaders in OpenGL 2.x are often used for tasks like vertex and fragment processing.
/// 
/// OpenGL 3.x:
/// OpenGL 3.x introduced Framebuffer Objects (FBOs), allowing rendering to textures rather than directly to the screen.
/// Shaders in OpenGL 3.x can write to textures through FBOs, and it is possible to modify textures in the fragment shader.
/// 
/// OpenGL 4.x: 
/// Compute shaders were introduced in OpenGL 4.x, providing a more flexible and general-purpose approach to parallel computing.
/// Compute shaders can output to textures through FBOs, and they can also modify textures directly in the shader.


/// TODO: 
///  GpuShaderSource, pin VertexAtrributeArrays to Shader, include Vertex, Color;
///    or pin VAO (need least GL3)
///  - VAO is like a memo, just marks down how you pin the vertex attribute arrays to a programable pipeline. you can easily do the same things again and again.
///  - QOpenGLShaderProgram wraps the source pin (attributes, uniforms) functions.
///  GpuShaderSink, pin FBO to Shader. (need least GL3)
///  - i made a GL3::GpuFBODevice class
///  - GpuRenderDevice class which is the default Framebuffer and it binds the FBO to 0. it has GL_FRONT and GL_BACK.



/// LIMIT:
/// 1. GLSL not support FP64. and the FP32 would be slow.
///     https://docs.unity3d.com/Manual/SL-DataTypesAndPrecision.html
/// 2. GPGPU needs least version 300 core

namespace zhelper
{
namespace GL2
{
    /// fixed-function pipeline by default
    /// attributes for fixed-function pipeline are called Client Arrays and Client Attribs
    /// gl*Pointer functions are attributes pinning for fixed-function pipeline
    /// gl*AttribPointer function are attributes pinning for programable pipeline
    template<GLenum _Ty>
    struct _Traits_GpuBuffer;
    template<>
    struct _Traits_GpuBuffer<GL_ARRAY_BUFFER>
    {
        static int queryCurrentBinding()
        {
            GLint vbo = 0;
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
            return vbo;
        }
    };
    template<>
    struct _Traits_GpuBuffer<GL_ELEMENT_ARRAY_BUFFER>
    {
        static int queryCurrentBinding()
        {
            GLint vbo = 0;
            glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &vbo);
            return vbo;
        }
    };
    
    template<GLenum _Ty, bool _AutoRelease = true, typename _Traits = _Traits_GpuBuffer<_Ty> >
    struct GpuBuffer
    {
        ~GpuBuffer()
        {
            if (vbo_ && _AutoRelease)
            {
                leave();
                if (_AutoRelease)
                    glDeleteBuffers(1, &vbo_);
            }
        }
        static int queryCurrentBinding()
        {
            return _Traits::queryCurrentBinding();
        }
        void ensure()
        {
            if (!vbo_)
                glGenBuffers(1, &vbo_);
            if (_Traits::queryCurrentBinding() != vbo_)
                glBindBuffer(_Ty, vbo_);
        }
        void leave()
        {
            if (_Traits::queryCurrentBinding() == vbo_)
                glBindBuffer(_Ty, 0);
        }
        void alloc(GLsizeiptr size, GLenum usage)
        {
            glBufferData(_Ty, size, 0, usage);
        }
        void alloc(GLsizeiptr size, const GLvoid* data, GLenum usage)
        {
            glBufferData(_Ty, size, data, usage);
        }
        void copy(GLintptr offset, GLsizeiptr size, const GLvoid* data)
        {
            glBufferSubData(_Ty, offset, size, data);
        }
        void copyTo(GLintptr offset, GLsizeiptr size, GLvoid* data)
        {
            glGetBufferSubData(_Ty, offset, size, data);
        }
        void* mmapReadOnly()
        {
            return glMapBuffer(_Ty, GL_READ_ONLY);
        }
        void* mmapWriteOnly()
        {
            return glMapBuffer(_Ty, GL_WRITE_ONLY);
        }
        void* mmapReadWrite()
        {
            return glMapBuffer(_Ty, GL_READ_WRITE);
        }
        bool unmap()
        {
            return glUnmapBuffer(_Ty);
        }
        
        GLuint vbo_ = 0;
    };
    
    struct GpuVertexArray : public GpuBuffer<GL_ARRAY_BUFFER>
    {
        /// apply arrays to fixed-function pipeline
        void vertexUseThisGpuBuffer(GLint size, GLenum type, GLsizei stride, const GLvoid* offset)
        {
            glVertexPointer(size, type, stride, offset);
        }
        void vertex3fUseThisGpuBuffer(GLsizei stride, const GLvoid* offset)
        {
            glVertexPointer(3, GL_FLOAT, stride, offset);
        }
        void vertex4fUseThisGpuBuffer(GLsizei stride, const GLvoid* offset)
        {
            glVertexPointer(4, GL_FLOAT, stride, offset);
        }
        void colorUseThisGpuBuffer(GLint size, GLenum type, GLsizei stride, const GLvoid* offset)
        {
            glColorPointer(size, type, stride, offset);
        }
        void color3fUseThisGpuBuffer(GLsizei stride, const GLvoid* offset)
        {
            glColorPointer(3, GL_FLOAT, stride, offset);
        }
        void color4fUseThisGpuBuffer(GLsizei stride, const GLvoid* offset)
        {
            glColorPointer(4, GL_FLOAT, stride, offset);
        }
        void normalUseThisGpuBuffer(GLenum type, GLsizei stride, const GLvoid* offset)
        {
            glNormalPointer(type, stride, offset);
        }
        void colorindexUseThisGpuBuffer(GLenum type, GLsizei stride, const GLvoid* offset)
        {
            glIndexPointer(type, stride, offset);
        }
        void texCoordUseThisGpuBuffer(GLint unit, GLint size, GLenum type, GLsizei stride, const GLvoid* offset)
        {
            glClientActiveTexture(GL_TEXTURE0 + unit);
            glTexCoordPointer(size, type, stride, offset);
        }
        void texCoordUseThisGpuBuffer(GLint unit, GLint size, GLenum type, const GLvoid* offset)
        {
            glClientActiveTexture(GL_TEXTURE0 + unit);
            glTexCoordPointer(size, type, 0, offset);
        }
        void texCoordUseThisGpuBufferAndCurrentUnit(GLint size, GLenum type, GLsizei stride, const GLvoid* offset)
        {
            glTexCoordPointer(size, type, stride, offset);
        }
        void texCoordUseThisGpuBufferAndCurrentUnit(GLint size, GLenum type, const GLvoid* offset)
        {
            glTexCoordPointer(size, type, 0, offset);
        }
        
        void drawArrays(GLenum mode, GLint first, GLsizei count)
        {
            glDrawArrays(mode, first, count);
        }
    };
    
    struct GpuElementArray : public GpuBuffer<GL_ELEMENT_ARRAY_BUFFER>
    {
        /// you just need to ensure before you want the fixed pipeline to draw elements using GL_ELEMENT_ARRAY_BUFFER.
        
        /// when you can the method below, i assume you have been ensure this GpuElementArray.
        void drawElements(GLenum mode, GLsizei count, const GLuint* indices)
        {
            glDrawElements(mode, count, GL_UNSIGNED_INT, indices);
        }
        void drawElements(GLenum mode, GLsizei count, const GLushort* indices)
        {
            glDrawElements(mode, count, GL_UNSIGNED_SHORT, indices);
        }
        void drawElements(GLenum mode, GLsizei count, const GLubyte* indices)
        {
            glDrawElements(mode, count, GL_UNSIGNED_BYTE, indices);
        }
    };
    
    /// 
    struct GpuRenderDevice
    {
        /// before GL3, there are no FBOs
        static void openReadOnFront()
        {
            glReadBuffer(GL_FRONT);
        }
        static void openReadOnBack()
        {
            glReadBuffer(GL_BACK);
        }
        static void openReadOnFrontAndBack()
        {
            glReadBuffer(GL_FRONT_AND_BACK);
        }
        static void openDrawOnFront()
        {
            glDrawBuffer(GL_FRONT);
        }
        static void openDrawOnBack()
        {
            glDrawBuffer(GL_BACK);
        }
        static void openDrawOnFrontAndBack()
        {
            glDrawBuffer(GL_FRONT_AND_BACK);
        }
        static void closeRead()
        {
            glReadBuffer(GL_NONE);
        }
        void clearColor4f(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
        {
            glClearColor(r, g, b, a);
        }
        void clear(GLbitfield mask)
        {
            glClear(mask);
        }
        void clearFrontAndBack()
        {
            glClear(GL_COLOR_BUFFER_BIT);
        }
        void clearDepth()
        {
            glClear(GL_DEPTH_BUFFER_BIT);
        }
        void clearStencil()
        {
            /// gray mask buffer
            glClear(GL_STENCIL_BUFFER_BIT);
        }
    };
    
    /// this class is a client of fixed-function pipeline
    struct GLFixedPipelineClient : public GpuRenderDevice
    {
        void ensure()
        {
            /// the program 0 is to not use any programable pipeline but the default fixed-function pipeline.
            glUseProgram(0);
        }
        GLFixedPipelineClient& connectColor()
        {
            glEnableClientState(GL_COLOR_ARRAY); return *this;
        }
        GLFixedPipelineClient& connectVertex()
        {
            glEnableClientState(GL_VERTEX_ARRAY); return *this;
        }
        GLFixedPipelineClient& connectTexCoord()
        {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY); return *this;
        }
        GLFixedPipelineClient& disconnectColor()
        {
            glDisableClientState(GL_COLOR_ARRAY); return *this;
        }
        GLFixedPipelineClient& disconnectVertex()
        {
            glDisableClientState(GL_VERTEX_ARRAY); return *this;
        }
        GLFixedPipelineClient& disconnectTexCoord()
        {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY); return *this;
        }
        
        void saveAttribArrays()
        {
            glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
        }
        void restoreAttribArrays()
        {
            glPopClientAttrib();
        }
        
        GLFixedPipelineClient& openTexture2D()
        {
            glEnable(GL_TEXTURE_2D); return *this;
        }
        GLFixedPipelineClient& openLighting()
        {
            glEnable(GL_LIGHTING); return *this;
        }
        GLFixedPipelineClient& openDepthTest()
        {
            glEnable(GL_DEPTH_TEST); return *this;
        }
        GLFixedPipelineClient& closeTexture2D()
        {
            glDisable(GL_TEXTURE_2D); return *this;
        }
        GLFixedPipelineClient& closeLighting()
        {
            glDisable(GL_LIGHTING); return *this;
        }
        GLFixedPipelineClient& closeDepthTest()
        {
            glDisable(GL_DEPTH_TEST); return *this;
        }
        
        void saveStates()
        {
            glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT|GL_LIGHTING_BIT);
        }
        void restoreStates()
        {
            glPopAttrib();
        }
        
        void saveMatrix()
        {
            glPushMatrix();
        }
        void restoreMatrix()
        {
            glPopMatrix();
        }
        
        void drawArrays(GLenum mode, GLint first, GLsizei count)
        {
            glDrawArrays(mode, first, count);
        }
        void drawElements(GLenum mode, GLsizei count, const GLuint* indices)
        {
            glDrawElements(mode, count, GL_UNSIGNED_INT, indices);
        }
        void drawElements(GLenum mode, GLsizei count, const GLushort* indices)
        {
            glDrawElements(mode, count, GL_UNSIGNED_SHORT, indices);
        }
        void drawElements(GLenum mode, GLsizei count, const GLubyte* indices)
        {
            glDrawElements(mode, count, GL_UNSIGNED_BYTE, indices);
        }
    };
    
    struct GLCpuClient : public GLFixedPipelineClient
    {
        void ensure()
        {
            GLFixedPipelineClient::ensure();
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
        
        void vertexUseCpuBuffer(GLint size, GLenum type, GLsizei stride, const GLvoid* offset)
        {
            glVertexPointer(size, type, stride, offset);
        }
        void vertexUseCpuBuffer(GLint size, GLenum type, const GLvoid* offset)
        {
            glVertexPointer(size, type, 0, offset);
        }
        void colorUseCpuBuffer(GLint size, GLenum type, GLsizei stride, const GLvoid* offset)
        {
            glColorPointer(size, type, stride, offset);
        }
        void colorUseCpuBuffer(GLint size, GLenum type, const GLvoid* offset)
        {
            glColorPointer(size, type, 0, offset);
        }
        void texCoordUseCpuBuffer(GLint unit, GLint size, GLenum type, GLsizei stride, const GLvoid* offset)
        {
            glClientActiveTexture(GL_TEXTURE0 + unit);
            glTexCoordPointer(size, type, stride, offset);
        }
        void texCoordUseCpuBuffer(GLint unit, GLint size, GLenum type, const GLvoid* offset)
        {
            glClientActiveTexture(GL_TEXTURE0 + unit);
            glTexCoordPointer(size, type, 0, offset);
        }
        void texCoordUseCpuBufferAndCurrentUnit(GLint size, GLenum type, GLsizei stride, const GLvoid* offset)
        {
            glTexCoordPointer(size, type, stride, offset);
        }
        void texCoordUseCpuBufferAndCurrentUnit(GLint size, GLenum type, const GLvoid* offset)
        {
            glTexCoordPointer(size, type, 0, offset);
        }
        template<GLsizei _Stride>
        void colorUseCpuBufferStride(GLint size, GLenum type, const GLvoid* offset)
        {
            glColorPointer(size, type, _Stride, offset);
        }
        void normalUseCpuBuffer(GLenum type, GLsizei stride, const GLvoid* offset)
        {
            glNormalPointer(type, stride, offset);
        }
        void normalUseCpuBuffer(GLenum type, const GLvoid* pointer)
        {
            glNormalPointer(type, 0, pointer);
        }
        void colorindexUseCpuBuffer(GLenum type, GLsizei stride, const GLvoid* offset)
        {
            glIndexPointer(type, stride, offset);
        }
        void colorindexUseCpuBuffer(GLenum type, const GLvoid* offset)
        {
            glIndexPointer(type, 0, offset);
        }
    };
    
}; // NS GL2
}; // NS zhelper

namespace zhelper
{
namespace GL2
{
    /// these is a principle of usage
    ///  1. set the things before you enable/open it.
    ///  2. material can be called anywhere, and effects.
    
    struct Lighting
    {
        void open()
        {
            glEnable(GL_LIGHTING);
        }
        void close()
        {
            glDisable(GL_LIGHTING);
        }
        void backLightColorGlobal(GLfloat* v4f)
        {
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, v4f);
        }
        void highLightAngleOnViewer()
        {
            glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
        }
        void highLightAngleOnTargetObject()
        {
            glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
        }
        void lightBackFaceToo()
        {
            glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
        }
        void lightFrontFaceOnly()
        {
            glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
        }
        void highLightSingleColor()
        {
            glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
        }
        void highLightSecondaryColor()
        {
            /// secondary specular color
            glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
        }
    };
    
    template<GLenum _N>
    struct Light
    {
        /** GL_AMBIENT, GL_DIFFUSE, GL_SPECULAR, GL_POSITION, GL_SPOT_CUTOFF, GL_SPOT_DIRECTION, GL_SPOT_EXPONENT, GL_CONSTANT_ATTENUATION, GL_LINEAR_ATTENUATION, and GL_QUADRATIC_ATTENUATION*/
        void sunLightColor(GLfloat* v4f)
        {
            glLightfv(_N, GL_AMBIENT, v4f);
        }
        void backLightColor(GLfloat* v4f)
        {
            glLightfv(_N, GL_DIFFUSE, v4f);
        }
        void highLightColor(GLfloat* v4f)
        {
            glLightfv(_N, GL_SPECULAR, v4f);
        }
        void sunFrom(GLfloat* _v4f)
        {
            GLfloat v4f[4] = {_v4f[0], _v4f[1], _v4f[2], 0};
            glLightfv(_N, GL_POSITION, v4f);
        }
        void spotAt(GLfloat* v4f)
        {
            glLightfv(_N, GL_POSITION, v4f);
        }
        void spotDirect(GLfloat* v3f)
        {
            glLightfv(_N, GL_SPOT_DIRECTION, v3f);
        }
        void spotOverAll()
        {
            glLighti(_N, GL_SPOT_CUTOFF, 180);
        }
        void spotAngle(GLint angle)
        {
            glLighti(_N, GL_SPOT_CUTOFF, angle);
        }
        void spotFocus(GLint exp)
        {
            glLighti(_N, GL_SPOT_EXPONENT, exp);
        }
        void spotEnergy(GLfloat k, GLfloat linear, GLfloat quadratic)
        {
            glLightf(_N, GL_CONSTANT_ATTENUATION, k);
            glLightf(_N, GL_LINEAR_ATTENUATION, linear);
            glLightf(_N, GL_QUADRATIC_ATTENUATION, quadratic);
        }
        void open()
        {
            glEnable(_N);
        }
        void close()
        {
            glDisable(_N);
        }
    };
    
#define DECLARE_LIGHT(N)    typedef Light<GL_LIGHT##N> Light##N;
    DECLARE_LIGHT(0);
    DECLARE_LIGHT(1);
    DECLARE_LIGHT(2);
    DECLARE_LIGHT(3);
    DECLARE_LIGHT(4);
    DECLARE_LIGHT(5);
    DECLARE_LIGHT(6);
    DECLARE_LIGHT(7);
#undef DECLARE_LIGHT
    
    struct Material
    {
        void useColorMaterial()
        {
            /// cases
            /// 1. color array apply to material paramters for every vertex.
            /// 2. 
            glEnable(GL_COLOR_MATERIAL);
        }
        void useMaterial()
        {
            /// cases
            /// 1. material paramters is constant through the whole procedure.
            /// 2. manual change material paramters in immedate mode.
            glDisable(GL_COLOR_MATERIAL);
        }
        template<GLenum _Face>
        struct Face
        {
            /// Materail
            Face& backLightReflectPct(GLfloat* v4f)
            {
                glMaterialfv(_Face, GL_AMBIENT, v4f); return *this;
            }
            Face& sunLightReflectPct(GLfloat* v4f)
            {
                glMaterialfv(_Face, GL_DIFFUSE, v4f); return *this;
            }
            Face& highLightReflectPct(GLfloat* v4f)
            {
                glMaterialfv(_Face, GL_SPECULAR, v4f); return *this;
            }
            Face& highLightExp(GLint exp)
            {
                glMateriali(_Face, GL_SHININESS, exp); return *this;
            }
            Face& selfLightColor(GLfloat* v4f)
            {
                glMaterialfv(_Face, GL_EMISSION, v4f); return *this;
            }
            Face& withoutBackLight()
            {
                GLfloat no_mat[4] = {0};
                glMaterialfv(_Face, GL_AMBIENT, no_mat); return *this;
            }
            Face& withoutSunLight()
            {
                GLfloat no_mat[4] = {0};
                glMaterialfv(_Face, GL_DIFFUSE, no_mat); return *this;
            }
            Face& withoutHighLight()
            {
                GLfloat no_mat[4] = {0};
                glMaterialfv(_Face, GL_SPECULAR, no_mat); return *this;
            }
            Face& withoutSelfLight()
            {
                GLfloat no_mat[4] = {0};
                glMaterialfv(_Face, GL_EMISSION, no_mat); return *this;
            }
            /// ColorMaterial, lastest set replace the previous set.
            Face& applyColorToBackLight()
            {
                glColorMaterial(_Face, GL_AMBIENT); return *this;
            }
            Face& applyColorToSunLight()
            {
                glColorMaterial(_Face, GL_DIFFUSE); return *this;
            }
            Face& applyColorToSelfLight()
            {
                glColorMaterial(_Face, GL_EMISSION); return *this;
            }
            Face& applyColorToHighLight()
            {
                glColorMaterial(_Face, GL_SPECULAR); return *this;
            }
            Face& applyColorToBackAndSunLight()
            {
                glColorMaterial(_Face, GL_AMBIENT_AND_DIFFUSE); return *this;
            }
        };
        Face<GL_FRONT> front;
        Face<GL_BACK> back;
        Face<GL_FRONT_AND_BACK> both;
        
    };
}; // NS GL2
}; // NS zhelper

namespace zhelper
{
namespace GL2
{
    // PBO
    // Z#20240118 docs
    // STREAM
    // The data store contents will be modified once and used at most a few times.
    // 
    // STATIC
    // The data store contents will be modified once and used many times.
    // 
    // DYNAMIC
    // The data store contents will be modified repeatedly and used many times.
    //
    // DRAW
    // The data store contents are modified by the application, and used as the source for GL drawing and image specification commands.
    // 
    // READ
    // The data store contents are modified by reading data from the GL, and used to return that data when queried by the application.
    // 
    // COPY
    // The data store contents are modified by reading data from the GL, and used as the source for GL drawing and image specification commands.
    
    /// Z#20240119 specially
    ///  GL_PIXEL_PACK_BUFFER, readable for cpu, cpu download PBO
    ///  GL_PIXEL_UNPACK_BUFFER, drawable for cpu, cpu upload PBO,
    template<>
    struct _Traits_GpuBuffer<GL_PIXEL_PACK_BUFFER>
    {
        static int queryCurrentBinding()
        {
            GLint vbo = 0;
            glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &vbo);
            return vbo;
        }
    };
    // Z#20240116
    // !!! careful
    /// after shader, should glFinish() or glMemoryBarrier()
    struct GpuPixelBufferReadable : public GpuBuffer<GL_PIXEL_PACK_BUFFER>
    {
        void alloc(GLsizeiptr size)
        {
            GpuBuffer::alloc(size, GL_STATIC_READ);
        }
        void allocDynamic(GLsizeiptr size)
        {
            GpuBuffer::alloc(size, GL_DYNAMIC_READ);
        }
        // alloc has been overloaded 
        void alloc(GLsizeiptr size, GLenum type)
        {
            GpuBuffer::alloc(size, type);
        }
        void* mmap()
        {
            return mmapReadOnly();
        }
    };
    struct GpuPixelBufferReadableSaver
    {
        GLint handle;
        ~GpuPixelBufferReadableSaver()
        {
            if (handle)
                glBindBuffer(GL_PIXEL_PACK_BUFFER, handle);
        }
        GpuPixelBufferReadableSaver() 
        {
            handle = GpuPixelBufferReadable::queryCurrentBinding();
            if (handle)
                glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        }
    };
    template<>
    struct _Traits_GpuBuffer<GL_PIXEL_UNPACK_BUFFER>
    {
        static int queryCurrentBinding()
        {
            GLint vbo = 0;
            glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &vbo);
            return vbo;
        }
    };
    struct GpuPixelBufferDrawable : public GpuBuffer<GL_PIXEL_UNPACK_BUFFER>
    {
        void alloc(GLsizeiptr size)
        {
            GpuBuffer::alloc(size, GL_STATIC_DRAW);
        }
        void allocDynamic(GLsizeiptr size)
        {
            GpuBuffer::alloc(size, GL_DYNAMIC_DRAW);
        }
        // alloc has been overloaded 
        void alloc(GLsizeiptr size, GLenum type)
        {
            GpuBuffer::alloc(size, type);
        }
        void allocStatic(GLsizeiptr size)
        {
            
        }
        void* mmap()
        {
            return mmapWriteOnly();
        }
    };
    
    // Z#20240116
    /// GpuPixelBuffer is a failure wrapper, 
    ///  a PBO should be either 
    ///    PACK (gpu wr to , cpu rd from) 
    ///    or UNPACK (cpu wr to, gpu rd from)
    /**
    struct GpuPixelBuffer : public GpuBuffer<GL_PIXEL_UNPACK_BUFFER>
    {
        void* mmap()
        {
            return mmapReadWrite();
        }
    };
    */
    
    template<GLenum _Ty>
    struct GpuImage
    {
        GLuint tex_ = 0;
        
        ~GpuImage()
        {
            if (tex_)
                glDeleteTextures(1, &tex_);
            tex_ = 0;
        }
        void ensure(GLuint n = 0)
        {
            if (!tex_)
                glGenTextures(1, &tex_);
            /// here, i assume the least version 330 core
            /// to use the built-in shader under pervious versions, glEnable(GL_TEXTURE);
            glActiveTexture(GL_TEXTURE0 + n);
            glBindTexture(_Ty, tex_);
        }
        bool available()
        {
            return glIsTexture(tex_);
        }
        /// Z#20240120 bug
        ///  should test download PBO binding
        ///  pbo = GpuPixelBufferReadable::queryCurrentBinding();
        ///  if(pbo) glBindBuffer()
        template<GLint _Lv = 0>
        void copyToCpuMemory(GLenum format, GLenum type, GLvoid* cpumem)
        {
            /// buffer texture can not use these gl*Image functions
            glGetTexImage(_Ty, _Lv, format, type, cpumem);
        }
        void copyToCpuMemory(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* cpumem)
        {
            /// depend to FBO
            /// you should GpuFBODevice::openReadCurrentFBO() first
            glReadPixels(x, y, width, height, format, type, cpumem);
        }
        /// Z#20220505 
        /// Z#20240116
        ///  careful, Tex -> PBO, with layout (R,G,B)[].
        template<GLint _Lv = 0>
        void copyToGpuPixelBufferReadable(GLenum format, GLenum type, GLvoid* offset = 0)
        {
            /// buffer texture can not use these gl*Image functions
            /// you should GpuPixelBufferReadable::ensure() first
            if (GpuPixelBufferReadable::queryCurrentBinding())
                glGetTexImage(_Ty, _Lv, format, type, offset);
        }
        void copyToGpuPixelBufferReadable(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* offset = 0)
        {
            /// depend to FBO and PBO (pack)
            /// you should GpuFBODevice::openReadCurrentFBO() first
            /// and GpuPixelBufferReadable::ensure() 
            if (GpuPixelBufferReadable::queryCurrentBinding())
                glReadPixels(x, y, width, height, format, type, offset);
        }
        template<GLint _Lv = 0>
        void alloc(GLint internalFormat, GLsizei width, GLsizei height, GLint border,
                   GLenum format, GLenum type, const GLvoid* data = 0);
    };
    
    template<GLenum _Ty>
    struct GpuImage123D : public GpuImage<_Ty>
    {
        void setMinFilterToNearest()
        {
            setMinFilter(GL_NEAREST);
        }
        void setMinFilterToLinear()
        {
            setMinFilter(GL_LINEAR);
        }
        void setMinFilter(GLint func)
        {
            glTexParameteri(_Ty, GL_TEXTURE_MIN_FILTER, func);
        }
        
        void setMagFilterToNearest()
        {
            setMagFilter(GL_NEAREST);
        }
        void setMagFilterToLinear()
        {
            setMagFilter(GL_LINEAR);
        }
        void setMagFilter(GLint func)
        {
            glTexParameteri(_Ty, GL_TEXTURE_MAG_FILTER, func);
        }
        
        void setMinLOD(GLint lod = -1000)
        {
            glTexParameteri(_Ty, GL_TEXTURE_MIN_LOD, lod);
        }
        void setMaxLOD(GLint lod = 1000)
        {
            glTexParameteri(_Ty, GL_TEXTURE_MAX_LOD, lod);
        }
        
        void setMaxLevel(GLint level = 1000)
        {
            glTexParameteri(_Ty, GL_TEXTURE_MAX_LOD, level);
        }
        
        void setBaseLevel(GLint level = 0)
        {
            glTexParameteri(_Ty, GL_TEXTURE_MAX_LEVEL, level);
        }
        
        void setWrapS(GLint S)
        {
            glTexParameteri(_Ty, GL_TEXTURE_WRAP_S, S);
        }
        
        void setWrapT(GLint T)
        {
            glTexParameteri(_Ty, GL_TEXTURE_WRAP_T, T);
        }
        
        void setWrapR(GLint R)
        {
            glTexParameteri(_Ty, GL_TEXTURE_WRAP_R, R);
        }
        
        void setGP()
        {
            setMinFilterToNearest();
            setMagFilterToNearest();
            setWrapS(GL_CLAMP);
            setWrapT(GL_CLAMP);
        }
    };
    
    /// the difference between GL_TEXTURE_2D and GL_TEXTURE_RECTANGLE
    ///  in the (fragment) shader, only GL_TEXTURE_RECTANGLE apply to gl_FragCoord.xy.
    ///  https://stackoverflow.com/questions/25157306/gl-texture-2d-vs-gl-texture-rectangle
    struct GpuImage2D : public GpuImage123D<GL_TEXTURE_2D>
    {
        template<GLint _Lv = 1>
        void alloc(GLint internalFormat, GLsizei width, GLsizei height)
        {
            /// Z#20220426 bug, GL4 at least
            glTexStorage2D(GL_TEXTURE_2D, _Lv, internalFormat, width, height);
        }
        template<GLint _Lv = 0>
        void alloc(GLint internalFormat, GLsizei width, GLsizei height, GLint border,
                   GLenum format, GLenum type, const GLvoid* data = 0)
        {
            glTexImage2D(GL_TEXTURE_2D, _Lv, internalFormat, width, height, border, format, type, data);
        }
        template<GLint _Lv = 0>
        void copyFromCpuMemory(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                   GLenum format, GLenum type, const GLvoid* data = 0)
        {
            glTexSubImage2D(GL_TEXTURE_2D, _Lv, xoffset, yoffset, width, height, format, type, data);
        }
        template<GLenum _Target = GL_TEXTURE_2D, GLint _Lv = 0>
        void copyFromRender(GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
        {
            glCopyTexSubImage2D(_Target, _Lv, xoffset, yoffset, x, y, width, height);
        }
        /// Z#20220505
        /// Z#20240116
        ///  carefully!!!
        ///  when single channel, the values of other channels in the Tex would be discard.
        template<GLint _Lv = 0>
        void copyFromGpuPixelBufferDrawable(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                   GLenum format, GLenum type, const GLvoid* data = 0)
        {
            if (GpuPixelBufferDrawable::queryCurrentBinding())
                glTexSubImage2D(GL_TEXTURE_2D, _Lv, xoffset, yoffset, width, height, format, type, data);
        }
        static const char* glslType()
        {
            return "sampler2D";
        }
    };
}; // NS GL2
}; // NS zhelper


namespace zhelper
{
namespace GL3
{
    struct GpuVertexArray : public GL2::GpuBuffer<GL_ARRAY_BUFFER>
    {
        /// GL3 programable pipeline
        void drawArrays(GLenum mode, GLint first, GLsizei count)
        {
            glDrawArrays(mode, first, count);
        }
    };
    
    typedef GL2::GpuElementArray GpuElementArray;
    
    template<GLenum _Ty>
    struct GpuImage
    {
        GLuint tex_ = 0;
        
        ~GpuImage()
        {
            if (tex_)
                glDeleteTextures(1, &tex_);
            tex_ = 0;
        }
        void ensure(GLuint n = 0)
        {
            if (!tex_)
                glGenTextures(1, &tex_);
            /// here, i assume the least version 330 core
            /// to use the built-in shader under pervious versions, glEnable(GL_TEXTURE);
            glActiveTexture(GL_TEXTURE0 + n);
            glBindTexture(_Ty, tex_);
        }
        bool available()
        {
            return glIsTexture(tex_);
        }
        template<GLint _Lv = 0>
        void copyToCpuMemory(GLenum format, GLenum type, GLvoid* cpumem)
        {
            /// buffer texture can not use these gl*Image functions
            glGetTexImage(_Ty, _Lv, format, type, cpumem);
        }
        void copyToCpuMemory(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* cpumem)
        {
            /// depend to FBO
            /// you should GpuFBODevice::openReadCurrentFBO() first
            glReadPixels(x, y, width, height, format, type, cpumem);
        }
        /// Z#20220505
        template<GLint _Lv = 0>
        void copyToGpuPixelBufferReadable(GLenum format, GLenum type, GLvoid* offset = NULL)
        {
            /// buffer texture can not use these gl*Image functions
            /// you should GpuPixelBufferReadable::ensure() first
            if (GL2::GpuPixelBufferReadable::queryCurrentBinding())
                glGetTexImage(_Ty, _Lv, format, type, offset);
        }
        void copyToGpuPixelBufferReadable(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* offset = NULL)
        {
            /// depend to FBO and PBO (pack)
            /// you should GpuFBODevice::openReadCurrentFBO() first
            /// and GpuPixelBufferReadable::ensure() 
            if (GL2::GpuPixelBufferReadable::queryCurrentBinding())
                glReadPixels(x, y, width, height, format, type, offset);
        }
        template<GLint _Lv = 0>
        void alloc(GLint internalFormat, GLsizei width, GLsizei height, GLint border,
                   GLenum format, GLenum type, const GLvoid* data = 0);
    };
    
    template<GLenum _Ty>
    struct GpuImage123D : public GpuImage<_Ty>
    {
        void setMinFilterToNearest()
        {
            setMinFilter(GL_NEAREST);
        }
        void setMinFilterToLinear()
        {
            setMinFilter(GL_LINEAR);
        }
        void setMinFilter(GLint func)
        {
            glTexParameteri(_Ty, GL_TEXTURE_MIN_FILTER, func);
        }
        
        void setMagFilterToNearest()
        {
            setMagFilter(GL_NEAREST);
        }
        void setMagFilterToLinear()
        {
            setMagFilter(GL_LINEAR);
        }
        void setMagFilter(GLint func)
        {
            glTexParameteri(_Ty, GL_TEXTURE_MAG_FILTER, func);
        }
        
        void setMinLOD(GLint lod = -1000)
        {
            glTexParameteri(_Ty, GL_TEXTURE_MIN_LOD, lod);
        }
        void setMaxLOD(GLint lod = 1000)
        {
            glTexParameteri(_Ty, GL_TEXTURE_MAX_LOD, lod);
        }
        
        void setMaxLevel(GLint level = 1000)
        {
            glTexParameteri(_Ty, GL_TEXTURE_MAX_LOD, level);
        }
        
        void setBaseLevel(GLint level = 0)
        {
            glTexParameteri(_Ty, GL_TEXTURE_MAX_LEVEL, level);
        }
        
        void setWrapS(GLint S)
        {
            glTexParameteri(_Ty, GL_TEXTURE_WRAP_S, S);
        }
        
        void setWrapT(GLint T)
        {
            glTexParameteri(_Ty, GL_TEXTURE_WRAP_T, T);
        }
        
        void setWrapR(GLint R)
        {
            glTexParameteri(_Ty, GL_TEXTURE_WRAP_R, R);
        }
        
        void setGP()
        {
            setMinFilterToNearest();
            setMagFilterToNearest();
            setWrapS(GL_CLAMP);
            setWrapT(GL_CLAMP);
        }
    };
    
    /// the difference between GL_TEXTURE_2D and GL_TEXTURE_RECTANGLE
    ///  in the (fragment) shader, only GL_TEXTURE_RECTANGLE apply to gl_FragCoord.xy.
    ///  https://stackoverflow.com/questions/25157306/gl-texture-2d-vs-gl-texture-rectangle
    struct GpuImage2D : public GpuImage123D<GL_TEXTURE_2D>
    {
        template<GLint _Lv = 1>
        void alloc(GLint internalFormat, GLsizei width, GLsizei height)
        {
            /// Z#20220426 bug, GL4.2 at least
            glTexStorage2D(GL_TEXTURE_2D, _Lv, internalFormat, width, height);
        }
        template<GLint _Lv = 0>
        void alloc(GLint internalFormat, GLsizei width, GLsizei height, GLint border,
                   GLenum format, GLenum type, const GLvoid* data = 0)
        {
            glTexImage2D(GL_TEXTURE_2D, _Lv, internalFormat, width, height, border, format, type, data);
        }
        template<GLint _Lv = 0>
        void copyFromCpuMemory(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                   GLenum format, GLenum type, const GLvoid* data = 0)
        {
            glTexSubImage2D(GL_TEXTURE_2D, _Lv, xoffset, yoffset, width, height, format, type, data);
        }
        template<GLenum _Target = GL_TEXTURE_2D, GLint _Lv = 0>
        void copyFromCurrentFBO(GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
        {
            glCopyTexSubImage2D(_Target, _Lv, xoffset, yoffset, x, y, width, height);
        }
        /// Z#20220505
        /// Z#20240116
        ///  carefully!!!
        ///  when single channel, the values of other channels in the Tex would be discard.
        template<GLint _Lv = 0>
        void copyFromGpuPixelBufferDrawable(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                   GLenum format, GLenum type, const GLvoid* data = 0)
        {
            if (GL2::GpuPixelBufferDrawable::queryCurrentBinding())
                glTexSubImage2D(GL_TEXTURE_2D, _Lv, xoffset, yoffset, width, height, format, type, data);
        }
    };
    
    struct GpuImageRect : public GpuImage123D<GL_TEXTURE_RECTANGLE>
    {
        template<GLint _Lv = 0>
        void alloc(GLint internalFormat, GLsizei width, GLsizei height, GLint border,
                   GLenum format, GLenum type, const GLvoid* data = 0)
        {
            (GL_TEXTURE_RECTANGLE, _Lv, internalFormat, width, height, border, format, type, data);
        }
        template<GLint _Lv = 0>
        void copyFromCpuMemory(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, 
                   GLenum format, GLenum type, const GLvoid* data = 0)
        {
            glTexSubImage2D(GL_TEXTURE_RECTANGLE, _Lv, xoffset, yoffset, width, height, format, type, data);
        }
        template<GLenum _Target = GL_TEXTURE_2D, GLint _Lv = 0>
        void copyFromCurrentFBO(GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
        {
            glCopyTexSubImage2D(_Target, _Lv, xoffset, yoffset, x, y, width, height);
        }
        
        static const char* glslType()
        {
            return "sampler2DRect";
        }
    };
    
    struct _Traits_GpuTexBuffer
    {
        static int queryCurrentBinding()
        {
            GLint vbo = 0;
            glGetIntegerv(GL_TEXTURE_BUFFER_BINDING, &vbo);
            return vbo;
        }
    };
    
    struct GpuTexBuffer : public GL2::GpuBuffer<GL_TEXTURE_BUFFER, true, _Traits_GpuTexBuffer>
    {
        
    };
    
    struct GpuTexBufferHandle : public GL2::GpuBuffer<GL_TEXTURE_BUFFER, false, _Traits_GpuTexBuffer>
    {
        
    };
    
    /// buffer texture can not attach to FBO
    /// buffer texture is 1D array.
    /// Z#20240118 doc
    ///  PBO and GL_TEXTURE_BUFFER are buffers can bind to a sort of TEXURE with the same name GL_TEXTURE_BUFFER
    /// carefully
    ///  buffer of GL_TEXTURE_BUFFER
    ///  texture of GL_TEXTURE_BUFFER
    struct GpuBufferImage : public GpuImage<GL_TEXTURE_BUFFER>
    {
        GpuTexBuffer self_buf_;
        void ensure(GLuint n = 0)
        {
            if (self_buf_.vbo_)
                self_buf_.ensure();
            GpuImage::ensure(n);
        }
        /// Z#20240118 add pbo
        void attach(GLint internalFormat, GL2::GpuPixelBufferReadable& pbo)
        {
            alloc(internalFormat, pbo.vbo_);
        }
        void attach(GLint internalFormat, GL2::GpuPixelBufferDrawable& pbo)
        {
            alloc(internalFormat, pbo.vbo_);
        }
        void alloc(GLint internalFormat, GLuint buffer)
        {
            /// the buffer is ensured by user
            /// Z#20240118
            ///  attenuation!!!
            ///  the buffer can be GL_TEXTRUE_BUFFER or PBO
            glTexBuffer(GL_TEXTURE_BUFFER, internalFormat, buffer);
        }
        void alloc(GLint internalFormat, GLsizeiptr bytes, GLenum usage = GL_STATIC_READ)
        {
            self_buf_.ensure();
            self_buf_.alloc(bytes, usage);
            alloc(internalFormat, self_buf_.vbo_); 
            // self_buf_.leave();
        }
        void alloc(GLint internalFormat, GLsizeiptr bytes, const GLvoid* data, GLenum usage = GL_STATIC_READ)
        {
            self_buf_.ensure();
            self_buf_.alloc(bytes, data, usage);
            alloc(internalFormat, self_buf_.vbo_);
            // self_buf_.leave();
        }
        void copyFromCpuMemory(GLintptr offset, GLsizeiptr bytes, const GLvoid* data)
        {
            GpuTexBufferHandle handle;
            handle.vbo_ = bufferId();
            handle.copy(offset, bytes, data);
        }
        void copyFromCpuMemory(GLsizeiptr bytes, const GLvoid* data)
        {
            GpuTexBufferHandle handle;
            handle.vbo_ = bufferId();
            handle.copy(0, bytes, data);
        }
        void copyToCpuMemory(GLintptr offset, GLsizeiptr bytes, GLvoid* data)
        {
            GpuTexBufferHandle handle;
            handle.vbo_ = bufferId();
            handle.copyTo(offset, bytes, data);
        }
        void copyToCpuMemory(GLsizeiptr bytes, GLvoid* data)
        {
            GpuTexBufferHandle handle;
            handle.vbo_ = bufferId();
            handle.copyTo(0, bytes, data);
        }
        void copyToCpuMemory(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* cpumem)
        {
            /// oveloaded the base class method
            /// buffer texture can not attach to FBO
            
        }
        GLint maxSize()
        {
            GLint size;
            glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &size);
            return size;
        }
        GLint bufferId()
        {
            /// Z#20220413 bug GL_TEXTURE_BINDING_BUFFER
            ///   GL_TEXTURE_BINDING_BUFFER get the texid or bufid ?
            /// 1. get the current texid which glBindTexture to GL_TEXTURE_BUFFER 
            /// 2. get the current bufid which glBindBuffer to GL_TEXTURE_BUFFER
            GLint bufid;
            glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, &bufid);
            return bufid;
        }
        
        void setGP()
        {
            /// do nothing
        }
        
        static const char* glslType()
        {
            return "samplerBuffer";
        }
    };
    
    /// this is the general default framebuffer which is called window-system-provided framebuffer.
    /// http://www.songho.ca/opengl/gl_fbo.html#:~:text=By%20default%2C%20OpenGL%20uses%20the,window%2Dsystem%2Dprovided%20framebuffer.
    /// https://www.khronos.org/opengl/wiki/Framebuffer
    struct GpuRenderDevice
    {
        void ensure()
        {
            /// Z#20220421 bug
            ///  0 is not the default framebuffer, it just unbind anything.
            ///  1 would be the default framebuffer. 
            ///  but i have no idea about NV or ATI drivers.
            /// thanks to https://stackoverflow.com/questions/11617013/why-would-glbindframebuffergl-framebuffer-0-result-in-blank-screen-in-cocos2d
            glBindFramebuffer(GL_FRAMEBUFFER, 1);
        }
        static void openReadOnFront()
        {
            glReadBuffer(GL_FRONT);
        }
        static void openReadOnFrontAndBack()
        {
            glReadBuffer(GL_FRONT_AND_BACK);
        }
        static void openDrawOnFront()
        {
            glDrawBuffer(GL_FRONT);
        }
        static void openDrawOnFrontAndBack()
        {
            glDrawBuffer(GL_FRONT_AND_BACK);
        }
    };
    
    template<GLenum _Device = GL_FRAMEBUFFER>
    struct GpuFBODevice
    {
        static void openReadCurrentFBO(GLint i)
        {
            glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
        }
        static void openDrawCurrentFBO(GLint i)
        {
            GLenum port = GL_COLOR_ATTACHMENT0 + i;
            glDrawBuffers(1, &port);
        }
        GLuint fbo_ = 0;
        ~GpuFBODevice()
        {
            leave();
            if (!fbo_)
                glDeleteFramebuffers(1, &fbo_);
            fbo_ = 0;
        }
        void ensure()
        {
            if (!fbo_)
                glGenFramebuffers(1, &fbo_);
            glBindFramebuffer(_Device, fbo_);
        }
        void leave()
        {
            auto fleave = [this](GLenum _Dev, GLenum _Query){
                GLuint fbo = 0;
                glGetIntegerv(_Query, (GLint*)&fbo);
                if (fbo == fbo_)
                {
                    glBindFramebuffer(_Dev, 0);
                }
            };
            if (GL_FRAMEBUFFER == _Device || GL_READ_FRAMEBUFFER == _Device)
            {
                fleave(GL_READ_FRAMEBUFFER, GL_READ_FRAMEBUFFER_BINDING);
            }
            if (GL_FRAMEBUFFER == _Device || GL_DRAW_FRAMEBUFFER == _Device)
            {
                fleave(GL_DRAW_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER_BINDING);
            }
        }
#define COLOR_N_PIN_TEX(_N_)  \
        void color##_N_##PinTexture(GLuint texture, GLint level = 0)  \
        {   \
            glFramebufferTexture(_Device, GL_COLOR_ATTACHMENT##_N_, texture, level);    \
        }   \
        template<GLenum _Ty>  \
        void color##_N_##PinGpuImage(GpuImage<_Ty>& image, GLint level = 0)   \
        {   \
            glFramebufferTexture(_Device, GL_COLOR_ATTACHMENT##_N_, image.tex_, level); \
        }   \
        void color##_N_##PinGpuImage2D(GpuImage2D& image, GLint level = 0)   \
        {   \
            glFramebufferTexture2D(_Device, GL_COLOR_ATTACHMENT##_N_, GL_TEXTURE_2D, image.tex_, level);    \
        }   \
        void color##_N_##PinGpuImage(GpuBufferImage&, GLint level = 0) = delete;
        COLOR_N_PIN_TEX(0);
        COLOR_N_PIN_TEX(1);
        COLOR_N_PIN_TEX(2);
        COLOR_N_PIN_TEX(3);
        COLOR_N_PIN_TEX(4);
        COLOR_N_PIN_TEX(5);
        COLOR_N_PIN_TEX(6);
        COLOR_N_PIN_TEX(7);
        COLOR_N_PIN_TEX(8);
        COLOR_N_PIN_TEX(9);
        COLOR_N_PIN_TEX(10);
        COLOR_N_PIN_TEX(11);
        COLOR_N_PIN_TEX(12);
        COLOR_N_PIN_TEX(13);
        COLOR_N_PIN_TEX(14);
        COLOR_N_PIN_TEX(15);
#undef COLOR_N_PIN_TEX
    };

}; // NS GL3
}; // NS zhelper

#ifndef HAS_NO_COMPUTE_SHADER
/// typically, opengl < 4.2
namespace zhelper
{
namespace GL4
{
    /// Z#20240118
    ///   GL4::GpuImage2D special for computing shader
    ///   GL3::GpuImage2D special for vertex-fragment shader
    struct GpuImage2D : public GL3::GpuImage123D<GL_TEXTURE_2D>
    {
        template<GLint _Lv = 1>
        void alloc(GLint internalFormat, GLsizei width, GLsizei height)
        {
            /// Z#20220426 bug, GL4.2 at least
            glTexStorage2D(GL_TEXTURE_2D, _Lv, internalFormat, width, height);
        }
        /// can not be glTexImage2D, if usage for computing shader
        template<GLint _Lv = 0>
        void alloc(GLint internalFormat, GLsizei width, GLsizei height, GLint border,
                   GLenum format, GLenum type, const GLvoid* data = 0) = delete;
        template<GLint _Lv = 0>
        void copyFromCpuMemory(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                   GLenum format, GLenum type, const GLvoid* data = 0)
        {
            glTexSubImage2D(GL_TEXTURE_2D, _Lv, xoffset, yoffset, width, height, format, type, data);
        }
        template<GLenum _Target = GL_TEXTURE_2D, GLint _Lv = 0>
        void copyFromCurrentFBO(GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
        {
            glCopyTexSubImage2D(_Target, _Lv, xoffset, yoffset, x, y, width, height);
        }
        /// Z#20220505
        /// Z#20240116
        ///  carefully!!!
        ///  when single channel, the values of other channels in the Tex would be discard.
        template<GLint _Lv = 0>
        void copyFromGpuPixelBufferDrawable(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                   GLenum format, GLenum type, const GLvoid* data = 0)
        {
            if (GL2::GpuPixelBufferDrawable::queryCurrentBinding())
                glTexSubImage2D(GL_TEXTURE_2D, _Lv, xoffset, yoffset, width, height, format, type, data);
        }
    };
}; // NS GL3
}; // NS zhelper
#endif // HAS_NO_COMPUTE_SHADER

#endif // __ZHELPER_GL2_H_
