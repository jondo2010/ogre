/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org/

  Copyright (c) 2000-2013 Torus Knot Software Ltd

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
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  -----------------------------------------------------------------------------
*/

#include "OgreGLSLAssembly.h"
#include "OgreGLSLShader.h"
#include "OgreGLSLMonolithicProgramManager.h"
#include "OgreGLSLSeparableProgramManager.h"
#include "OgreRoot.h"

namespace Ogre {

    GLuint GLSLAssembly::mVertexShaderCount = 0;
    GLuint GLSLAssembly::mFragmentShaderCount = 0;
    GLuint GLSLAssembly::mGeometryShaderCount = 0;
    GLuint GLSLAssembly::mHullShaderCount = 0;
    GLuint GLSLAssembly::mDomainShaderCount = 0;
    GLuint GLSLAssembly::mComputeShaderCount = 0;
    //-----------------------------------------------------------------------
    GLSLAssembly::GLSLAssembly(GLSLShader* parent) :
        GL3PlusShader(parent->getCreator(), parent->getName(), parent->getHandle(),
                      parent->getGroup(), false, 0), mGLSLShader(parent)
    {
        mType = parent->getType();
        mSyntaxCode = "glsl" + StringConverter::toString(Root::getSingleton().getRenderSystem()->getNativeShadingLanguageVersion());

        mLinked = 0;

        if (parent->getType() == GPT_VERTEX_PROGRAM)
        {
            mShaderID = ++mVertexShaderCount;
        }
        else if (parent->getType() == GPT_FRAGMENT_PROGRAM)
        {
            mShaderID = ++mFragmentShaderCount;
        }
        else if (parent->getType() == GPT_GEOMETRY_PROGRAM)
        {
            mShaderID = ++mGeometryShaderCount;
        }
        else if (parent->getType() == GPT_HULL_PROGRAM)
        {
            mShaderID = ++mHullShaderCount;
        }
        else if (parent->getType() == GPT_COMPUTE_PROGRAM)
        {
            mShaderID = ++mComputeShaderCount;
        }
        else
        {
            mShaderID = ++mDomainShaderCount;
        }

        // Transfer skeletal animation status from parent
        mSkeletalAnimation = mGLSLShader->isSkeletalAnimationIncluded();
        // There is nothing to load
        mLoadFromFile = false;
    }
    //-----------------------------------------------------------------------
    GLSLAssembly::~GLSLAssembly()
    {
        // Have to call this here rather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        unload();
    }
    //-----------------------------------------------------------------------
    void GLSLAssembly::loadImpl(void)
    {
        // Nothing to load.
    }

    //-----------------------------------------------------------------------
    void GLSLAssembly::unloadImpl(void)
    {
        // Nothing to unload.
    }

    //-----------------------------------------------------------------------
    void GLSLAssembly::loadFromSource(void)
    {
        // Nothing to load.
    }

    //-----------------------------------------------------------------------
    void GLSLAssembly::bindShader(void)
    {
        if (Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
        {
            // Tell the Program Pipeline Manager what pipeline is to become active.
            switch (mType)
            {
            case GPT_VERTEX_PROGRAM:
                GLSLSeparableProgramManager::getSingleton().setActiveVertexShader(this);
                break;
            case GPT_FRAGMENT_PROGRAM:
                GLSLSeparableProgramManager::getSingleton().setActiveFragmentShader(this);
                break;
            case GPT_GEOMETRY_PROGRAM:
                GLSLSeparableProgramManager::getSingleton().setActiveGeometryShader(this);
                break;
            case GPT_HULL_PROGRAM:
                GLSLSeparableProgramManager::getSingleton().setActiveTessHullShader(this);
                break;
            case GPT_DOMAIN_PROGRAM:
                GLSLSeparableProgramManager::getSingleton().setActiveTessDomainShader(this);
                break;
            case GPT_COMPUTE_PROGRAM:
                GLSLSeparableProgramManager::getSingleton().setActiveComputeShader(this);
            default:
                break;
            }
        }
        else
        {
            // Tell the Link Program Manager what shader is to become active.
            switch (mType)
            {
            case GPT_VERTEX_PROGRAM:
                GLSLMonolithicProgramManager::getSingleton().setActiveVertexShader(this);
                break;
            case GPT_FRAGMENT_PROGRAM:
                GLSLMonolithicProgramManager::getSingleton().setActiveFragmentShader(this);
                break;
            case GPT_GEOMETRY_PROGRAM:
                GLSLMonolithicProgramManager::getSingleton().setActiveGeometryShader(this);
                break;
            case GPT_HULL_PROGRAM:
                GLSLMonolithicProgramManager::getSingleton().setActiveHullShader(this);
                break;
            case GPT_DOMAIN_PROGRAM:
                GLSLMonolithicProgramManager::getSingleton().setActiveDomainShader(this);
                break;
            case GPT_COMPUTE_PROGRAM:
                GLSLMonolithicProgramManager::getSingleton().setActiveComputeShader(this);
            default:
                break;
            }
        }



    }

    //-----------------------------------------------------------------------
    void GLSLAssembly::unbindShader(void)
    {
        if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
        {
            // Tell the Program Pipeline Manager what pipeline is to become inactive.
            if (mType == GPT_VERTEX_PROGRAM)
            {
                GLSLSeparableProgramManager::getSingleton().setActiveVertexShader(NULL);
            }
            else if (mType == GPT_GEOMETRY_PROGRAM)
            {
                GLSLSeparableProgramManager::getSingleton().setActiveGeometryShader(NULL);
            }
            else if (mType == GPT_HULL_PROGRAM)
            {
                GLSLSeparableProgramManager::getSingleton().setActiveTessHullShader(NULL);
            }
            else if (mType == GPT_DOMAIN_PROGRAM)
            {
                GLSLSeparableProgramManager::getSingleton().setActiveTessDomainShader(NULL);
            }
            else if (mType == GPT_COMPUTE_PROGRAM)
            {
                GLSLSeparableProgramManager::getSingleton().setActiveComputeShader(NULL);
            }
            else // It's a fragment shader
            {
                GLSLSeparableProgramManager::getSingleton().setActiveFragmentShader(NULL);
            }
        }
        else
        {
            // Tell the Link Program Manager what shader is to become inactive.
            if (mType == GPT_VERTEX_PROGRAM)
            {
                GLSLMonolithicProgramManager::getSingleton().setActiveVertexShader(NULL);
            }
            else if (mType == GPT_GEOMETRY_PROGRAM)
            {
                GLSLMonolithicProgramManager::getSingleton().setActiveGeometryShader(NULL);
            }
            else if (mType == GPT_HULL_PROGRAM)
            {
                GLSLMonolithicProgramManager::getSingleton().setActiveHullShader(NULL);
            }
            else if (mType == GPT_DOMAIN_PROGRAM)
            {
                GLSLMonolithicProgramManager::getSingleton().setActiveDomainShader(NULL);
            }
            else if (mType == GPT_COMPUTE_PROGRAM)
            {
                GLSLMonolithicProgramManager::getSingleton().setActiveComputeShader(NULL);
            }
            else // It's a fragment shader
            {
                GLSLMonolithicProgramManager::getSingleton().setActiveFragmentShader(NULL);
            }
        }
    }

    //-----------------------------------------------------------------------
    void GLSLAssembly::bindShaderSharedParameters(GpuProgramParametersSharedPtr params, uint16 mask)
    {
        // Link can throw exceptions, ignore them at this point.
        try
        {
            if (Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            {
                // Activate the program pipeline object.
                GLSLSeparableProgram* programPipeline = GLSLSeparableProgramManager::getSingleton().getActiveSeparableProgram();
                // Pass on parameters from params to program object uniforms.
                programPipeline->updateUniformBlocks(params, mask, mType);
                // programPipeline->updateShaderStorageBlock(params, mask, mType);
            }
            else
            {
                // Activate the link program object.
                GLSLMonolithicProgram* linkProgram = GLSLMonolithicProgramManager::getSingleton().getActiveMonolithicProgram();
                // Pass on parameters from params to program object uniforms.
                linkProgram->updateUniformBlocks(params, mask, mType);
            }
        }
        catch (Exception& e) {}
    }

    //-----------------------------------------------------------------------
    void GLSLAssembly::bindShaderParameters(GpuProgramParametersSharedPtr params, uint16 mask)
    {
        // Link can throw exceptions, ignore them at this point.
        try
        {
            if (Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            {
                // Activate the program pipeline object.
                GLSLSeparableProgram* programPipeline = GLSLSeparableProgramManager::getSingleton().getActiveSeparableProgram();
                // Pass on parameters from params to program object uniforms.
                programPipeline->updateUniforms(params, mask, mType);
                programPipeline->updateAtomicCounters(params, mask, mType);
            }
            else
            {
                // Activate the link program object.
                GLSLMonolithicProgram* linkProgram = GLSLMonolithicProgramManager::getSingleton().getActiveMonolithicProgram();
                // Pass on parameters from params to program object uniforms.
                linkProgram->updateUniforms(params, mask, mType);
                //TODO add atomic counter support
                //linkProgram->updateAtomicCounters(params, mask, mType);
            }
        }
        catch (Exception& e) {}
    }

    //-----------------------------------------------------------------------
    void GLSLAssembly::bindShaderPassIterationParameters(GpuProgramParametersSharedPtr params)
    {
        if (Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
        {
            // Activate the program pipeline object.
            GLSLSeparableProgram* programPipeline = GLSLSeparableProgramManager::getSingleton().getActiveSeparableProgram();
            // Pass on parameters from params to program object uniforms.
            programPipeline->updatePassIterationUniforms(params);
        }
        else
        {
            // Activate the link program object.
            GLSLMonolithicProgram* linkProgram = GLSLMonolithicProgramManager::getSingleton().getActiveMonolithicProgram();
            // Pass on parameters from params to program object uniforms.
            linkProgram->updatePassIterationUniforms(params);
        }
    }


    size_t GLSLAssembly::calculateSize(void) const
    {
        size_t memSize = 0;

        // Delegate names.
        memSize += sizeof(GLuint);
        memSize += sizeof(GLenum);
        memSize += GpuProgram::calculateSize();

        return memSize;
    }
}
