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

#ifndef __ZHELPER_GLES2_H_
#define __ZHELPER_GLES2_H_
#define GL_GLES_PROTOTYPES 1
#define GL_GLEXT_PROTOTYPES 1
#define FEATURE_USE_ANGLE
#include <GLES3/gl3.h>


/// Z#20220419

/// history
/// 1. ES 1.0 has fixed-pipeline
/// 2. ES 2.0 (2007) remove the fixed-pipeline.

/// LIMIT:
/// 1. GLSL not support FP64. and the FP32 would be slow.
///     https://docs.unity3d.com/Manual/SL-DataTypesAndPrecision.html
/// 2. GPGPU needs least version 300 es, where can glReadPixels from Texture bound to current FBO

namespace zhelper
{
namespace GLES2
{
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
        GLuint vbo_ = 0;
    };
    
    struct GpuVertexArray : public GpuBuffer<GL_ARRAY_BUFFER>
    {
        
    };
    
    struct GpuElementArray : public GpuBuffer<GL_ELEMENT_ARRAY_BUFFER>
    {
        
    };
}; // NS GLES2
}; // NS zhelper


namespace zhelper
{
namespace GLES2
{
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
        void copyToCpuMemory(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* cpumem)
        {
            /// depend to FBO
            /// you should GpuFBODevice::openReadCurrentFBO() first
            glReadPixels(x, y, width, height, format, type, cpumem);
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
            setWrapS(GL_CLAMP_TO_EDGE);
            setWrapT(GL_CLAMP_TO_EDGE);
        }
    };
    
    /// the difference between GL_TEXTURE_2D and GL_TEXTURE_RECTANGLE
    ///  in the (fragment) shader, only GL_TEXTURE_RECTANGLE apply to gl_FragCoord.xy.
    ///  https://stackoverflow.com/questions/25157306/gl-texture-2d-vs-gl-texture-rectangle
    struct GpuImage2D : public GpuImage123D<GL_TEXTURE_2D>
    {
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
    };
    
    template<GLenum _Device = GL_FRAMEBUFFER>
    struct GpuFBODevice
    {
        
        static void openReadCurrentFBO(GLint i)
        {
            GLint port = (i == 0) ? GL_COLOR_ATTACHMENT0 : (GL_COLOR_ATTACHMENT1 + i -1);
            glReadBuffer(port);
        }
        static void openDrawCurrentFBO(GLint i = 0)
        {
            /// Z#20220420 bug
            ///  ES only allow draw attachmentN to bufferN.
            ///  otherwise, Ith value is not COLOR_ATTACHMENTi or GL_NONE
            /// Z#20220421 bug
            ///   ES 2.0 only support GL_COLOR_ATTACHMENT0, ANGLE GLESv2 seems only support GL_COLOR_ATTACHMENT0 too.
            ///   ES 3.0 only support GL_COLOR_ATTACHMENT0 for GL_DRAW_FRAMEBUFFER. 0-7 for GL_READ_FRAMEBUFFER.
            GLenum port[8] = {GL_NONE};
            port[i] = GL_COLOR_ATTACHMENT0 + i;
            glDrawBuffers(8, port);
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
        /// Z#20220422 bug
        ///   ES does not allow a texture are as an attachment of FBO and uniform of Shader.
        ///   DESKTOP GL OK, but ES not;
        
#define COLOR_N_PIN_TEX(_N_)  \
        void color##_N_##PinGpuImage2D(GpuImage2D& image, GLint level = 0)   \
        {   \
            glFramebufferTexture2D(_Device, GL_COLOR_ATTACHMENT##_N_, GL_TEXTURE_2D, image.tex_, level);    \
        }   \
        void color##_N_##UnpinGpuImage2D(GLint level = 0)   \
        {   \
            glFramebufferTexture2D(_Device, GL_COLOR_ATTACHMENT##_N_, GL_TEXTURE_2D, 0, level);    \
        } 
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

    /// https://arm-software.github.io/opengl-es-sdk-for-android/compute_intro.html
}; // NS GLES2
}; // NS zhelper

namespace zhelper
{
namespace GLES3
{
    struct GpuImage2D : public GLES2::GpuImage2D
    {
        template<GLint _Lv = 1>
        void alloc(GLint internalFormat, GLsizei width, GLsizei height)
        {
            /// ES 3.1 should use immutable texture for compute shader.
            glTexStorage2D(GL_TEXTURE_2D, _Lv, internalFormat, width, height);
        }
        template<GLint _Lv = 0>
        void alloc(GLint internalFormat, GLsizei width, GLsizei height, GLint border,
                   GLenum format, GLenum type, const GLvoid* data = 0)
        {
            GLES2::GpuImage2D::alloc(internalFormat, width, height, border, format, type, data);
        }
    };
    
    typedef GLES2::GpuRenderDevice GpuRenderDevice;
    
    template<GLenum _Device = GL_FRAMEBUFFER>
    struct GpuFBODevice : GLES2::GpuFBODevice<_Device>
    {
    };
    
}; // NS GLES3
}; // NS zhelper

#endif // __ZHELPER_GLES2_H_
