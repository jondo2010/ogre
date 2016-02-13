/*
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
#ifndef _OgreHlmsCompute_H_
#define _OgreHlmsCompute_H_

#include "OgreHlms.h"
#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class CompositorShadowNode;
    struct QueuedRenderable;

    /** \addtogroup Component
    *  @{
    */
    /** \addtogroup Material
    *  @{
    */

    /** HLMS implementation that handles compute shaders. It isn't registered in
        the usual way to the HlmsManager.
    */
    class _OgreExport HlmsCompute : public Hlms
    {
        struct ComputeJobEntry
        {
            HlmsComputeJob  *computeJob;
            String          name;
            ComputeJobEntry() : computeJob( 0 ) {}
            ComputeJobEntry( HlmsComputeJob *_computeJob, const String &_name ) :
                computeJob( _computeJob ), name( _name ) {}
        };

        typedef std::map<IdString, ComputeJobEntry> HlmsComputeJobMap;

        struct Hash
        {
            uint64 hashVal[2];

            bool operator < ( const Hash &_r ) const
            {
                if( hashVal[0] < _r.hashVal[0] ) return true;
                if( hashVal[0] > _r.hashVal[0] ) return false;

                if( hashVal[1] < _r.hashVal[1] ) return true;
                //if( hashVal[1] > _r.hashVal[1] ) return false;

                return false;
            }
        };

        struct ComputePsoCache
        {
            HlmsComputeJob *job;
            HlmsPropertyVec setProperties;
            HlmsComputePso  pso;

            ComputePsoCache() : job( 0 ) {}
            ComputePsoCache( HlmsComputeJob *_job, const HlmsPropertyVec &properties ) :
                job( _job ), setProperties( properties ) {}

            bool operator == ( const ComputePsoCache &_r ) const
            {
                //Exclude the PSO from the comparison!
                return setProperties == _r.setProperties && job == _r.job;
            }
        };

        typedef vector<ComputePsoCache>::type ComputePsoCacheVec;
        typedef map<Hash, GpuProgramPtr>::type CompiledShaderMap;

        AutoParamDataSource *mAutoParamDataSource;
        String const        *mComputeShaderTarget;

        /// Caches a compiled shader based on the hash of its source string
        /// We need this in case two HlmsComputeJobs use the same exact
        /// shader but with different buffers.
        CompiledShaderMap   mCompiledShaderCache;
        /// Caches a full PSO.
        ComputePsoCacheVec  mComputeShaderCache;

        HlmsComputeJobMap   mComputeJobs;

        void processPieces( const StringVector &pieceFiles );
        HlmsComputePso compileShader( HlmsComputeJob *job, uint32 finalHash );

        virtual HlmsDatablock* createDatablockImpl( IdString datablockName,
                                                    const HlmsMacroblock *macroblock,
                                                    const HlmsBlendblock *blendblock,
                                                    const HlmsParamVec &paramVec );

    public:
        HlmsCompute( AutoParamDataSource *autoParamDataSource );
        ~HlmsCompute();

        /** An HlmsComputeJob is very similar to an HlmsDatablock, except it
            contains a compute job instead. If multiple HlmsComputeJob end up
            having the same compute shader (i.e. the resulting source code is
            the same); they will share the same shader.
        @param datablockName
            Name to assign to the job, for lookup.
        @param refName
            User-friendly readable name of the job. Normally should match the datablockName
        @param sourceFilename
            Main file to use for compiling.
        @param includedPieceFiles
            Included files, to be parsed to defined pieces for the main file to use (can be empty).
        @return
            A new job.
        */
        HlmsComputeJob* createComputeJob( IdString datablockName, const String &refName,
                                          const String &sourceFilename,
                                          const StringVector &includedPieceFiles );

        /// Destroys all jobs created via @see createComputeJob
        void destroyAllComputeJobs(void);

        /// Destroys the shader cache from all jobs, causing us to reload shaders from file again
        virtual void clearShaderCache(void);

        /// Main function for dispatching a compute job.
        void dispatch( HlmsComputeJob *job );

        virtual void _changeRenderSystem( RenderSystem *newRs );
    };

    struct _OgreExport ComputeProperty
    {
        static const IdString ThreadsPerGroupX;
        static const IdString ThreadsPerGroupY;
        static const IdString ThreadsPerGroupZ;
        static const IdString NumThreadGroupsX;
        static const IdString NumThreadGroupsY;
        static const IdString NumThreadGroupsZ;

        static const IdString NumTextureSlots;
        static const IdString MaxTextureSlot;
        static const char *Texture;
    };

    /** @} */
    /** @} */

}

#include "OgreHeaderSuffix.h"

#endif