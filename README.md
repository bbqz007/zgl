# zgl
simplify the concepts of cpu memory and gpu memory on opengl apis.

simplify coding transfer between cpu memory and gpu memory.

# purpose
* GL2 - fixed pipeline rendering
  * VBO wrapped as `GpuBuffer` as input source.
* GL3 - gpgpu
  * `Texture` wrapped as `GpuImage` as input source or output sink, FBO device bind the two. 

# concept
* device
  * streaming input data port
  * streaming output data port
  * rendering filter 
* cpu memory
  * memory buffer in cpu host side
  * such as data array
    * `float []`
* gpu memory
  * memory buffer in gpu host side
  * such as
    * Array Buffer (VBO), wrapped as `GpuBuffer`.
    * Pixel Pack Buffer (PBO), readable or drawable to cpu.
    * Texture, wrapped as `GpuImage`.
  * interface
    * a couplue callings of `ensure()` and `leave()` for every usage
    * `alloc()` to allocate space in gpu memories
    * `copyTo()`, copy data to cpu host side memories 
    * `copy()`, copy data from cpu host side memories
    * specially, VBO has `mmap()` and `unmmap()`

# classes
* GL2
  * `GpuBuffer`
    * `GpuVertexArray`
    * `GpuElementArray`
    * `GpuPixelBufferReadable`
    * `GpuPixelBufferDrawable`
  * `GpuImage`
    * `GpuImage123D`
      * `GpuImage2D`
  * `GpuRenderDevice`
    * `GLFixedPipelineClient`, fixed output to screen render buffer.
      * `GLCpuClient`, with apis of ability of using cpu memory
  * `Lighting`
  * `Light`
  * `Material`
* GL3
  * `GpuBuffer`
    * `GpuVertexArray`
    * `GpuElementArray`
    * `GpuTexBuffer`
    * `GpuTexBufferHandle`
  * `GpuImage`
    * `GpuImage123D`
      * `GpuImageRect`
      * `GpuImage2D`
    * `GpuBufferImage`
  * `GpuRenderDevice`
  * `GpuFBODevice`

# examples
## GL2 use Cpu Buffer
```c++
float v[][3] = {...};  // Vertex
    float c[][3] = {...};  // Color
    float tc[][3] = {...}; // TexCoord
    float ix[][4] = {...}; // Element

    zhelper::GL2::GLCpuClient cpu;
            cpu.ensure();
            cpu.colorUseCpuBuffer(3, GL_FLOAT, c);
            cpu.vertexUseCpuBuffer(3, GL_FLOAT, v);

            cpu.connectColor().connectVertex();
            cpu.drawElements(GL_QUADS, sizeof(ix)/sizeof(GLint), ix[0]);
            cpu.disconnectColor().disconnectVertex();
```

## GL2 use Gpu Buffer (VBO)
```c++
     float v[][3] = {...};  // Vertex
     float c[][3] = {...};  // Color
     float tc[][3] = {...}; // TexCoord
     float ix[][4] = {...}; // Element
   
     zhelper::GL2::GpuVertexArray gpubuf;
             gpubuf.ensure();
             gpubuf.alloc(sizeof(v) + sizeof(c) + sizeof(tc), GL_STREAM_DRAW);
             gpubuf.copy(0, sizeof(v), v);
             gpubuf.copy(sizeof(v), sizeof(c), c);
             gpubuf.copy(sizeof(v) + sizeof(c), sizeof(tc), tc);
             gpubuf.vertex3fUseThisGpuBuffer(0, (GLvoid*)0);
             gpubuf.color3fUseThisGpuBuffer(0, (GLvoid*)sizeof(v));
             gpubuf.texCoordUseThisGpuBuffer(0, 2, GL_FLOAT, (GLvoid*)(sizeof(v) + sizeof(c)));
 
     zhelper::GL2::GLFixedPipelineClient cpu;17　　　　　　　 cpu.ensure();
             cpu.connectColor().connectVertex().connectTexCoord();
             cpu.drawElements(GL_QUADS, sizeof(ix)/sizeof(GLint), ix[begin]);
             cpu.disconnectColor().disconnectVertex().disconnectTexCoord();
```

## transfer data use PBO
```c++
                        zhelper::GL3::GpuBufferImage gpuMem1;
                        zhelper::GL3::GpuImage2D gpuMem2;
                        zhelper::GL3::GpuFBODevice<> dev;
                        zhelper::GL2::GpuPixelBufferReadable gpuPBORd;
                        zhelper::GL2::GpuPixelBufferDrawable gpuPBODw;
                         {
                                gpuPBODw.ensure();
                                gpuPBODw.alloc(texSize * texSize * sizeof(float), GL_STATIC_DRAW);
                                float* cpu_vaddr = gpuPBODw.mmap();
                                for (int i = 0; i < texSize * texSize; ++i)
                                     cpu_vaddr[i] = cpu_restore_space[i];
                                gpuPBODw.unmap();
                                gpuMem1.copyFromGpuPixelBufferDrawable(0, 0, texSize, h + 1, GL_RED, GL_FLOAT, 0);
                                gpuPBODw.leave();
                        }
                        {
                                gpuPBORd.ensure();
                                gpuPBORd.alloc(texSize * texSize * sizeof(float), GL_STATIC_DRAW);
                                gpuMem2.copyToGpuPixelBufferReadable(0, 0, texSize, h + 1, GL_RED, GL_FLOAT, 0);
                                float* cpu_vaddr = gpuPBORd.mmap();
                                for (int i = 0; i < texSize * texSize; ++i)
                                     cpu_restore_space[i] = cpu_vaddr[i];
                                gpuPBORd.unmap();
                                gpuPBORd.leave();
                        }
```

## GL3 gpgpu and transfer data use Texture
```c++
                        zhelper::GL3::GpuBufferImage gpuMem1;
                        zhelper::GL3::GpuImage2D gpuMem2;
                        zhelper::GL3::GpuFBODevice<> dev;
      // alloc input gpu buffer, and copy data from cpu host side
                            gpuMem1.ensure();
                            gpuMem1.setGP();
                            GLsizei h = shdayC.size() / (1 * texSize);
                            GLsizei odd = (shdayC.size() - 1 * texSize * h) / 1;
                            gpuMem1.alloc(GL_R32F, texSize * texSize * sizeof(float), (const GLvoid*)0);
                            gpuMem1.copyFromCpuMemory(texSize * h * sizeof(shdayC.front()), shdayC.data());
                            gpuMem1.copyFromCpuMemory(texSize * h * sizeof(shdayC.front()), odd * sizeof(shdayC.front()), shdayC.data() + texSize * h);
                            gpuMem1.copyToCpuMemory(0, shdayC.size() * sizeof(shdayC.front()), readbuf.data());
      // alloc output gpu buffer
                            gpuMem2.ensure(2);
                            gpuMem2.alloc(GL_R32F, texSize, texSize, 0, GL_RED, GL_FLOAT, 0);
                            dev.color1PinGpuImage2D(gpuMem2);
                            zhelper::GL3::GpuFBODevice<>::openDrawCurrentFBO(1);
     // bind shader program
                            auto& program = *shaderprogram2;
                            program.bind();
     // compute
                            glViewport(0, 0, texSize, texSize);
                            glDrawArrays(GL_QUADS, 4, 4);
     // copy gpu buffer data to cpu host side
                            dev.ensure();
                            zhelper::GL3::GpuFBODevice<>::openReadCurrentFBO(1);
                            gpuMem2.copyToCpuMemory(0, 0, texSize, h + 1, GL_RED, GL_FLOAT, readbuf.data() + 2*shdayC.size());
                            
```
