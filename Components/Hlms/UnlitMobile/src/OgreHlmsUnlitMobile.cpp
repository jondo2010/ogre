﻿/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

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

#include "OgreStableHeaders.h"

#include "OgreHlmsUnlitMobile.h"
#include "OgreHlmsUnlitMobileDatablock.h"

#include "OgreHighLevelGpuProgramManager.h"
#include "OgreHighLevelGpuProgram.h"

#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "OgreRenderTarget.h"
#include "Compositor/OgreCompositorShadowNode.h"

namespace Ogre
{
    const IdString UnlitMobileProp::PropertyTexMatrixCount      = IdString( "hlms_texture_matrix_count" );
    const IdString UnlitMobileProp::PropertyTexMatrixCount0     = IdString( "hlms_texture_matrix_count0" );
    const IdString UnlitMobileProp::PropertyTexMatrixCount1     = IdString( "hlms_texture_matrix_count1" );
    const IdString UnlitMobileProp::PropertyTexMatrixCount2     = IdString( "hlms_texture_matrix_count2" );
    const IdString UnlitMobileProp::PropertyTexMatrixCount3     = IdString( "hlms_texture_matrix_count3" );
    const IdString UnlitMobileProp::PropertyTexMatrixCount4     = IdString( "hlms_texture_matrix_count4" );
    const IdString UnlitMobileProp::PropertyTexMatrixCount5     = IdString( "hlms_texture_matrix_count5" );
    const IdString UnlitMobileProp::PropertyTexMatrixCount6     = IdString( "hlms_texture_matrix_count6" );
    const IdString UnlitMobileProp::PropertyTexMatrixCount7     = IdString( "hlms_texture_matrix_count7" );

    const IdString UnlitMobileProp::DiffuseMap                  = IdString( "diffuse_map" );

    const IdString UnlitMobileProp::PropertyDiffuse             = IdString( "diffuse" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount0    = IdString( "diffuse_map_count0" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount1    = IdString( "diffuse_map_count1" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount2    = IdString( "diffuse_map_count2" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount3    = IdString( "diffuse_map_count3" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount4    = IdString( "diffuse_map_count4" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount5    = IdString( "diffuse_map_count5" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount6    = IdString( "diffuse_map_count6" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount7    = IdString( "diffuse_map_count7" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount8    = IdString( "diffuse_map_count8" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount9    = IdString( "diffuse_map_count9" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount10   = IdString( "diffuse_map_count10" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount11   = IdString( "diffuse_map_count11" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount12   = IdString( "diffuse_map_count12" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount13   = IdString( "diffuse_map_count13" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount14   = IdString( "diffuse_map_count14" );
    const IdString UnlitMobileProp::PropertyDiffuseMapCount15   = IdString( "diffuse_map_count15" );

    const IdString UnlitMobileProp::PropertyBlendModeIdx0       = IdString( "blend_mode_idx0" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx1       = IdString( "blend_mode_idx1" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx2       = IdString( "blend_mode_idx2" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx3       = IdString( "blend_mode_idx3" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx4       = IdString( "blend_mode_idx4" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx5       = IdString( "blend_mode_idx5" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx6       = IdString( "blend_mode_idx6" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx7       = IdString( "blend_mode_idx7" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx8       = IdString( "blend_mode_idx8" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx9       = IdString( "blend_mode_idx9" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx10      = IdString( "blend_mode_idx10" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx11      = IdString( "blend_mode_idx11" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx12      = IdString( "blend_mode_idx12" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx13      = IdString( "blend_mode_idx13" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx14      = IdString( "blend_mode_idx14" );
    const IdString UnlitMobileProp::PropertyBlendModeIdx15      = IdString( "blend_mode_idx15" );

    const IdString UnlitMobileProp::PropertyUvAtlas             = IdString( "uv_atlas" );
    const IdString UnlitMobileProp::PropertyUvAtlas0            = IdString( "uv_atlas0" );
    const IdString UnlitMobileProp::PropertyUvAtlas1            = IdString( "uv_atlas1" );
    const IdString UnlitMobileProp::PropertyUvAtlas2            = IdString( "uv_atlas2" );
    const IdString UnlitMobileProp::PropertyUvAtlas3            = IdString( "uv_atlas3" );
    const IdString UnlitMobileProp::PropertyUvAtlas4            = IdString( "uv_atlas4" );
    const IdString UnlitMobileProp::PropertyUvAtlas5            = IdString( "uv_atlas5" );
    const IdString UnlitMobileProp::PropertyUvAtlas6            = IdString( "uv_atlas6" );
    const IdString UnlitMobileProp::PropertyUvAtlas7            = IdString( "uv_atlas7" );
    const IdString UnlitMobileProp::PropertyUvAtlas8            = IdString( "uv_atlas8" );
    const IdString UnlitMobileProp::PropertyUvAtlas9            = IdString( "uv_atlas9" );
    const IdString UnlitMobileProp::PropertyUvAtlas10           = IdString( "uv_atlas10" );
    const IdString UnlitMobileProp::PropertyUvAtlas11           = IdString( "uv_atlas11" );
    const IdString UnlitMobileProp::PropertyUvAtlas12           = IdString( "uv_atlas12" );
    const IdString UnlitMobileProp::PropertyUvAtlas13           = IdString( "uv_atlas13" );
    const IdString UnlitMobileProp::PropertyUvAtlas14           = IdString( "uv_atlas14" );
    const IdString UnlitMobileProp::PropertyUvAtlas15           = IdString( "uv_atlas15" );

    extern const String c_blendModes[];

    const IdString *DiffuseMapCountPtrs[16][3] =
    {
        { &UnlitMobileProp::PropertyDiffuseMapCount0, &UnlitMobileProp::PropertyBlendModeIdx0,
          &UnlitMobileProp::PropertyUvAtlas0 },
        { &UnlitMobileProp::PropertyDiffuseMapCount1, &UnlitMobileProp::PropertyBlendModeIdx1,
          &UnlitMobileProp::PropertyUvAtlas1 },
        { &UnlitMobileProp::PropertyDiffuseMapCount2, &UnlitMobileProp::PropertyBlendModeIdx2,
          &UnlitMobileProp::PropertyUvAtlas2 },
        { &UnlitMobileProp::PropertyDiffuseMapCount3, &UnlitMobileProp::PropertyBlendModeIdx3,
          &UnlitMobileProp::PropertyUvAtlas3 },
        { &UnlitMobileProp::PropertyDiffuseMapCount4, &UnlitMobileProp::PropertyBlendModeIdx4,
          &UnlitMobileProp::PropertyUvAtlas4 },
        { &UnlitMobileProp::PropertyDiffuseMapCount5, &UnlitMobileProp::PropertyBlendModeIdx5,
          &UnlitMobileProp::PropertyUvAtlas5 },
        { &UnlitMobileProp::PropertyDiffuseMapCount6, &UnlitMobileProp::PropertyBlendModeIdx6,
          &UnlitMobileProp::PropertyUvAtlas6 },
        { &UnlitMobileProp::PropertyDiffuseMapCount7, &UnlitMobileProp::PropertyBlendModeIdx7,
          &UnlitMobileProp::PropertyUvAtlas7 },
        { &UnlitMobileProp::PropertyDiffuseMapCount8, &UnlitMobileProp::PropertyBlendModeIdx8,
          &UnlitMobileProp::PropertyUvAtlas8 },
        { &UnlitMobileProp::PropertyDiffuseMapCount9, &UnlitMobileProp::PropertyBlendModeIdx9,
          &UnlitMobileProp::PropertyUvAtlas9 },
        { &UnlitMobileProp::PropertyDiffuseMapCount10, &UnlitMobileProp::PropertyBlendModeIdx10,
          &UnlitMobileProp::PropertyUvAtlas10 },
        { &UnlitMobileProp::PropertyDiffuseMapCount11, &UnlitMobileProp::PropertyBlendModeIdx11,
          &UnlitMobileProp::PropertyUvAtlas11 },
        { &UnlitMobileProp::PropertyDiffuseMapCount12, &UnlitMobileProp::PropertyBlendModeIdx12,
          &UnlitMobileProp::PropertyUvAtlas12 },
        { &UnlitMobileProp::PropertyDiffuseMapCount13, &UnlitMobileProp::PropertyBlendModeIdx13,
          &UnlitMobileProp::PropertyUvAtlas13 },
        { &UnlitMobileProp::PropertyDiffuseMapCount14, &UnlitMobileProp::PropertyBlendModeIdx14,
          &UnlitMobileProp::PropertyUvAtlas14 },
        { &UnlitMobileProp::PropertyDiffuseMapCount15, &UnlitMobileProp::PropertyBlendModeIdx15,
          &UnlitMobileProp::PropertyUvAtlas15 },
    };


    const IdString *TexCoordAnimationMatrix[8] =
    {
        &UnlitMobileProp::PropertyTexMatrixCount0,
        &UnlitMobileProp::PropertyTexMatrixCount1,
        &UnlitMobileProp::PropertyTexMatrixCount2,
        &UnlitMobileProp::PropertyTexMatrixCount3,
        &UnlitMobileProp::PropertyTexMatrixCount4,
        &UnlitMobileProp::PropertyTexMatrixCount5,
        &UnlitMobileProp::PropertyTexMatrixCount6,
        &UnlitMobileProp::PropertyTexMatrixCount7
    };

    const String c_vsPerObjectUniforms[] =
    {
        "worldViewProj",
        "texture_matrix"
    };
    const String c_psPerObjectUniforms[] =
    {
        "constColour",
        "alpha_test_threshold",
        "atlasOffsets"
    };

    HlmsUnlitMobile::HlmsUnlitMobile( Archive *dataFolder ) : Hlms( HLMS_UNLIT, "unlit", dataFolder )
    {
    }
    //-----------------------------------------------------------------------------------
    HlmsUnlitMobile::~HlmsUnlitMobile()
    {
    }
    //-----------------------------------------------------------------------------------
    const HlmsCache* HlmsUnlitMobile::createShaderCacheEntry( uint32 renderableHash,
                                                              const HlmsCache &passCache,
                                                              uint32 finalHash,
                                                              const QueuedRenderable &queuedRenderable )
    {
        const HlmsCache *retVal = Hlms::createShaderCacheEntry( renderableHash, passCache, finalHash,
                                                                queuedRenderable );

        GpuNamedConstants *constantsDef;
        //Nasty const_cast, but the refactor required to remove this is 100x nastier.
        constantsDef = const_cast<GpuNamedConstants*>( &retVal->vertexShader->getConstantDefinitions() );
        for( size_t i=0; i<sizeof( c_vsPerObjectUniforms ) / sizeof( String ); ++i )
        {
            GpuConstantDefinitionMap::iterator it = constantsDef->map.find( c_vsPerObjectUniforms[i] );
            if( it != constantsDef->map.end() )
                it->second.variability = GPV_PER_OBJECT;
        }

        //Nasty const_cast, but the refactor required to remove this is 100x nastier.
        constantsDef = const_cast<GpuNamedConstants*>( &retVal->pixelShader->getConstantDefinitions() );
        for( size_t i=0; i<sizeof( c_psPerObjectUniforms ) / sizeof( String ); ++i )
        {
            GpuConstantDefinitionMap::iterator it = constantsDef->map.find( c_psPerObjectUniforms[i] );
            if( it != constantsDef->map.end() )
                it->second.variability = GPV_PER_OBJECT;
        }

        assert( dynamic_cast<const HlmsUnlitMobileDatablock*>( queuedRenderable.renderable->getDatablock() ) );
        const HlmsUnlitMobileDatablock *datablock = static_cast<const HlmsUnlitMobileDatablock*>(
                                                    queuedRenderable.renderable->getDatablock() );

        //Set samplers.
        GpuProgramParametersSharedPtr psParams = retVal->pixelShader->getDefaultParameters();

        if( datablock->mNumTextureUnits > 0 )
        {
            uint texUnit = 0;
            vector<int>::type diffuseTex;
            diffuseTex.reserve( datablock->mNumTextureUnits );
            for( texUnit=0; texUnit<datablock->mNumTextureUnits; ++texUnit )
            {
                assert( !datablock->mDiffuseTextures[texUnit].isNull() );
                diffuseTex.push_back( texUnit );
                ++texUnit;
            }

            psParams->setNamedConstant( "texDiffuseMap", &diffuseTex[0], diffuseTex.size(), 1 );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void HlmsUnlitMobile::calculateHashFor( Renderable *renderable, uint32 &outHash,
                                            uint32 &outCasterHash )
    {
        mSetProperties.clear();

        setProperty( HlmsBaseProp::Skeleton, 0 );

        v1::RenderOperation op;
        renderable->getRenderOperation( op );
        v1::VertexDeclaration *vertexDecl = op.vertexData->vertexDeclaration;
        const v1::VertexDeclaration::VertexElementList &elementList = vertexDecl->getElements();
        v1::VertexDeclaration::VertexElementList::const_iterator itor = elementList.begin();
        v1::VertexDeclaration::VertexElementList::const_iterator end  = elementList.end();

        uint numTexCoords = 0;
        while( itor != end )
        {
            const v1::VertexElement &vertexElem = *itor;
            switch( vertexElem.getSemantic() )
            {
            case VES_DIFFUSE:
                setProperty( HlmsBaseProp::Colour, 1 );
                break;
            case VES_TEXTURE_COORDINATES:
                numTexCoords = std::max<uint>( numTexCoords, vertexElem.getIndex() + 1 );
                setProperty( *HlmsBaseProp::UvCountPtrs[vertexElem.getIndex()],
                              v1::VertexElement::getTypeCount( vertexElem.getType() ) );
                break;
            default:
                break;
            }

            vertexElem.getType();
            ++itor;
        }

        setProperty( HlmsBaseProp::UvCount, numTexCoords );

        PiecesMap pieces[NumShaderTypes];

        assert( dynamic_cast<const HlmsUnlitMobileDatablock*>( renderable->getDatablock() ) );
        const HlmsUnlitMobileDatablock *datablock = static_cast<const HlmsUnlitMobileDatablock*>(
                                                                    renderable->getDatablock() );

        setProperty( UnlitMobileProp::PropertyDiffuse, datablock->mHasColour );
        setProperty( UnlitMobileProp::DiffuseMap, datablock->mNumTextureUnits );

        if( datablock->mIsAlphaTested )
        {
            setProperty( HlmsBaseProp::AlphaTest, 1 );

            switch( datablock->mShaderCreationData->alphaTestCmp )
            {
            case CMPF_LESS:             pieces[PixelShader]["alpha_test_cmp_func"] = "<"; break;
            case CMPF_LESS_EQUAL:       pieces[PixelShader]["alpha_test_cmp_func"] = "<="; break;
            case CMPF_EQUAL:            pieces[PixelShader]["alpha_test_cmp_func"] = "=="; break;
            case CMPF_GREATER:          pieces[PixelShader]["alpha_test_cmp_func"] = ">"; break;
            case CMPF_GREATER_EQUAL:    pieces[PixelShader]["alpha_test_cmp_func"] = ">="; break;
            case CMPF_NOT_EQUAL:        pieces[PixelShader]["alpha_test_cmp_func"] = "!="; break;
            default:
                break;
            }
        }

        setProperty( UnlitMobileProp::PropertyUvAtlas, datablock->mNumUvAtlas );
        for( size_t i=0; i<datablock->mNumTextureUnits; ++i )
        {
            const uint8 uvSet = datablock->mShaderCreationData->mUvSetForTexture[i];
            const uint8 blendModeIdx = datablock->mShaderCreationData->mBlendModes[i];

            if( uvSet >= numTexCoords )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             datablock->getName().getFriendlyText() +
                             ": diffuse_map's is trying to use more UV sets than the mesh "
                             "has ( " + StringConverter::toString(uvSet) + " vs " +
                             StringConverter::toString(numTexCoords) + " )",
                             "HlmsUnlitMobile::calculateHashFor" );
            }

            setProperty( *DiffuseMapCountPtrs[i][0], uvSet );
            pieces[PixelShader][*DiffuseMapCountPtrs[i][1]] = "@insertpiece( " +
                                                                c_blendModes[blendModeIdx] + ")";
            setProperty( *DiffuseMapCountPtrs[i][2],
                         datablock->mShaderCreationData->mTextureIsAtlas[i] );
        }

        setProperty( UnlitMobileProp::PropertyTexMatrixCount, datablock->mNumTextureMatrices );
        for( size_t i=0; i<datablock->mNumTextureMatrices; ++i )
            setProperty( *TexCoordAnimationMatrix[i], 1 );

        uint32 renderableHash = this->addRenderableCache( mSetProperties, pieces );

        outHash         = renderableHash;
        outCasterHash   = renderableHash;
    }
    //-----------------------------------------------------------------------------------
    HlmsCache HlmsUnlitMobile::preparePassHash( const CompositorShadowNode *shadowNode, bool casterPass,
                                                bool dualParaboloid, SceneManager *sceneManager )
    {
        HlmsCache retVal = Hlms::preparePassHash( shadowNode, casterPass, dualParaboloid, sceneManager );
        Camera *camera = sceneManager->getCameraInProgress();
        Matrix4 viewMatrix = camera->getViewMatrix(true);

        Matrix4 projectionMatrix = camera->getProjectionMatrixWithRSDepth();

        RenderTarget *renderTarget = sceneManager->getCurrentViewport()->getTarget();
        if( renderTarget->requiresTextureFlipping() )
        {
            projectionMatrix[1][0] = -projectionMatrix[1][0];
            projectionMatrix[1][1] = -projectionMatrix[1][1];
            projectionMatrix[1][2] = -projectionMatrix[1][2];
            projectionMatrix[1][3] = -projectionMatrix[1][3];
        }

        //mPreparedPass.viewProjMatrix[0]   = camera->getProjectionMatrix() * viewMatrix; //TODO
        mPreparedPass.viewProjMatrix[0]     = projectionMatrix * viewMatrix;
        mPreparedPass.viewProjMatrix[1]     = Matrix4::IDENTITY;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsUnlitMobile::fillBuffersFor( const HlmsCache *cache,
                                            const QueuedRenderable &queuedRenderable,
                                            bool casterPass, const HlmsCache *lastCache,
                                            uint32 lastTextureHash )
    {
        GpuProgramParametersSharedPtr vpParams = cache->vertexShader->getDefaultParameters();
        GpuProgramParametersSharedPtr psParams = cache->pixelShader->getDefaultParameters();
        float *vsUniformBuffer = vpParams->getFloatPointer( 0 );
#if _SECURE_SCL
        float *psUniformBuffer = 0;
        if( !psParams->getFloatConstantList().empty() )
            psUniformBuffer = psParams->getFloatPointer( 0 );
#else
        float *psUniformBuffer = psParams->getFloatPointer( 0 );
#endif

        assert( dynamic_cast<const HlmsUnlitMobileDatablock*>( queuedRenderable.renderable->getDatablock() ) );
        const HlmsUnlitMobileDatablock *datablock = static_cast<const HlmsUnlitMobileDatablock*>(
                                                queuedRenderable.renderable->getDatablock() );

        uint16 variabilityMask = cache != lastCache ? GPV_ALL : GPV_PER_OBJECT;

        //const Matrix4 &worldMat = queuedRenderable.movableObject->_getParentNodeFullTransform();
        Matrix4 worldMat;
        assert( queuedRenderable.renderable->getNumWorldTransforms() == 1 );
        queuedRenderable.renderable->getWorldTransforms( &worldMat );

        bool useIdentityProjection = queuedRenderable.renderable->getUseIdentityProjection();

        //---------------------------------------------------------------------------
        //                          ---- VERTEX SHADER ----
        //---------------------------------------------------------------------------
#if !OGRE_DOUBLE_PRECISION
        //mat4 worldViewProj
        Matrix4 tmp = mPreparedPass.viewProjMatrix[useIdentityProjection] * worldMat;
        memcpy( vsUniformBuffer, &tmp, sizeof(Matrix4) );
        vsUniformBuffer += 16;
#else
    #error Not Coded Yet! (cannot use memcpy on Matrix4)
#endif

        memcpy( vsUniformBuffer, datablock->mTextureMatrices,
                16 * datablock->mNumTextureMatrices * sizeof(float) );
        vsUniformBuffer += 16 * datablock->mNumTextureMatrices;

        //---------------------------------------------------------------------------
        //                          ---- PIXEL SHADER ----
        //---------------------------------------------------------------------------
        if( datablock->mHasColour )
        {
            memcpy( psUniformBuffer, &datablock->mR, 4 * sizeof(float) );
            psUniformBuffer += 4;
        }

        if( datablock->mIsAlphaTested )
            *psUniformBuffer++ = datablock->mAlphaTestThreshold;

        memcpy( psUniformBuffer, datablock->mUvAtlasParams,
                datablock->mNumUvAtlas * sizeof( HlmsUnlitMobileDatablock::UvAtlasParams ) );
        psUniformBuffer += datablock->mNumUvAtlas * (sizeof( HlmsUnlitMobileDatablock::UvAtlasParams ) /
                                                     sizeof(float) );

#if OGRE_DEBUG_MODE
        {
            IdString oldHash = datablock->mTextureHash;
            const_cast<HlmsUnlitMobileDatablock*>(datablock)->calculateHash();
            assert( oldHash == datablock->mTextureHash &&
                    "Forgot to call calculateHash after modifying a texture to datablock" );
        }
#endif

        if( datablock->mTextureHash != lastTextureHash )
        {
            //Rebind textures
            for( uint texUnit=0; texUnit<datablock->mNumTextureUnits; ++texUnit )
            {
                mRenderSystem->_setTexture( texUnit, true, datablock->mDiffuseTextures[texUnit].get() );
                ++texUnit;
            }

            mRenderSystem->_disableTextureUnitsFrom( datablock->mNumTextureUnits );
        }

        assert( vsUniformBuffer - vpParams->getFloatPointer( 0 ) == vpParams->getFloatConstantList().size() );

#if _SECURE_SCL
        if( !psParams->getFloatConstantList().empty() )
            assert( psUniformBuffer - psParams->getFloatPointer( 0 ) == psParams->getFloatConstantList().size() );
#else
        assert( psUniformBuffer - psParams->getFloatPointer( 0 ) == psParams->getFloatConstantList().size() );
#endif

        mRenderSystem->bindGpuProgramParameters( GPT_VERTEX_PROGRAM, vpParams, variabilityMask );
        mRenderSystem->bindGpuProgramParameters( GPT_FRAGMENT_PROGRAM, psParams, variabilityMask );

        return datablock->mTextureHash;
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsUnlitMobile::fillBuffersFor( const HlmsCache *cache,
                                            const QueuedRenderable &queuedRenderable,
                                            bool casterPass, const HlmsCache *lastCache,
                                            CommandBuffer *commandBuffer )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Trying to use fast-path on a mobile implementation. "
                     "Change the RenderQueue settings.",
                     "HlmsUnlitMobile::fillBuffersFor" );
    }
    //-----------------------------------------------------------------------------------
    HlmsDatablock* HlmsUnlitMobile::createDatablockImpl( IdString datablockName,
                                                         const HlmsMacroblock *macroblock,
                                                         const HlmsBlendblock *blendblock,
                                                         const HlmsParamVec &paramVec )
    {
        return OGRE_NEW HlmsUnlitMobileDatablock( datablockName, this, macroblock, blendblock, paramVec );
    }
}
