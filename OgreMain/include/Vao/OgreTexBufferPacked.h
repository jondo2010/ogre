/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#ifndef _Ogre_TexBufferPacked_H_
#define _Ogre_TexBufferPacked_H_

#include "Vao/OgreBufferPacked.h"
#include "OgrePixelFormat.h"

namespace Ogre
{
    /** Represents Texture buffers (also known as tbuffers in D3D11)
     */
    class TexBufferPacked : public BufferPacked
    {
        PixelFormat mPixelFormat;

    public:
        TexBufferPacked( size_t internalBufferStart, size_t numElements, uint32 bytesPerElement,
                         BufferType bufferType, void *initialData, bool keepAsShadow,
                         VaoManager *vaoManager, BufferInterface *bufferInterface, PixelFormat pf ) :
            BufferPacked( internalBufferStart, numElements, bytesPerElement, bufferType,
                          initialData, keepAsShadow, vaoManager, bufferInterface ),
            mPixelFormat( pf )
        {
        }

        /** Binds the texture buffer to the given slot
        @param slot
            The slot to asign this constant buffer. In D3D11 it's called 'slot'.
            In GLSL it's called it's called 'binding'
        @param offset
            0-based offset. It is possible to bind a region of the buffer.
            Offset needs to be aligned. You can query the RS capabilities for
            the alignment, however 256 bytes is the maximum allowed alignment
            per the OpenGL specification, making it a safe bet to hardcode.
        @param sizeBytes
            Size in bytes to bind the tex buffer. When zero,
            binds from offset until the end of the buffer.
        */
        virtual void bindBuffer( uint16 slot, size_t offset=0, size_t sizeBytes=0 ) = 0;

        PixelFormat getPixelFormat(void) const           { return mPixelFormat; }

        //TODO
        virtual AsyncTicket* readRequest( size_t elementStart, size_t elementCount ) { return 0; }
        //TODO
        virtual void disposeTicket( AsyncTicket *ticket ) {}
    };
}

#endif
