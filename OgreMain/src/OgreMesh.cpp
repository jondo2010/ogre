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
#include "OgreStableHeaders.h"
#include "OgreMesh.h"

#include "OgreSubMesh.h"
#include "OgreLogManager.h"
#include "OgreMeshSerializer.h"
#include "OgreSkeletonManager.h"
#include "OgreHardwareBufferManager.h"
#include "OgreStringConverter.h"
#include "OgreException.h"
#include "OgreMeshManager.h"
#include "OgreEdgeListBuilder.h"
#include "OgreAnimation.h"
#include "OgreAnimationState.h"
#include "OgreAnimationTrack.h"
#include "OgreBone.h"
#include "OgreOptimisedUtil.h"
#include "OgreTangentSpaceCalc.h"
#include "OgreLodStrategyManager.h"
#include "OgrePixelCountLodStrategy.h"

namespace Ogre {
    //-----------------------------------------------------------------------
    Mesh::Mesh(ResourceManager* creator, const String& name, ResourceHandle handle,
        const String& group, bool isManual, ManualResourceLoader* loader)
        : Resource(creator, name, handle, group, isManual, loader),
        mBoundRadius(0.0f),
        mBoneBoundingRadius(0.0f),
        mBoneAssignmentsOutOfDate(false),
        mLodStrategy(LodStrategyManager::getSingleton().getDefaultStrategy()),
        mHasManualLodLevel(false),
        mNumLods(1),
        mVertexBufferUsage(HardwareBuffer::HBU_STATIC_WRITE_ONLY),
        mIndexBufferUsage(HardwareBuffer::HBU_STATIC_WRITE_ONLY),
        mVertexBufferShadowBuffer(true),
        mIndexBufferShadowBuffer(true),
        mPreparedForShadowVolumes(false),
        mEdgeListsBuilt(false),
        mAutoBuildEdgeLists(true), // will be set to false by serializers of 1.30 and above
		mSharedVertexDataAnimationType(VAT_NONE),
		mSharedVertexDataAnimationIncludesNormals(false),
		mAnimationTypesDirty(true),
		mPosesIncludeNormals(false),
		sharedVertexData(0)
    {
		// Init first (manual) lod
		MeshLodUsage lod;
        lod.userValue = 0; // User value not used for base LOD level
		lod.value = getLodStrategy()->getBaseValue();
        lod.edgeData = NULL;
        lod.manualMesh.setNull();
		mMeshLodUsageList.push_back(lod);
    }
    //-----------------------------------------------------------------------
    Mesh::~Mesh()
    {
        // have to call this here reather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        unload();
    }
    //-----------------------------------------------------------------------
    SubMesh* Mesh::createSubMesh()
    {
        SubMesh* sub = OGRE_NEW SubMesh();
        sub->parent = this;

        mSubMeshList.push_back(sub);

		if (isLoaded())
			_dirtyState();

        return sub;
    }
    //-----------------------------------------------------------------------
    SubMesh* Mesh::createSubMesh(const String& name)
	{
		SubMesh *sub = createSubMesh();
		nameSubMesh(name, (ushort)mSubMeshList.size()-1);
		return sub ;
	}
    //-----------------------------------------------------------------------
	void Mesh::destroySubMesh(unsigned short index)
	{
        if (index >= mSubMeshList.size())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
						"Index out of bounds.",
						"Mesh::removeSubMesh");
        }
		SubMeshList::iterator i = mSubMeshList.begin();
		std::advance(i, index);
		mSubMeshList.erase(i);
		
		// Fix up any name/index entries
		for(SubMeshNameMap::iterator ni = mSubMeshNameMap.begin(); ni != mSubMeshNameMap.end();)
		{
			if (ni->second == index)
			{
				SubMeshNameMap::iterator eraseIt = ni++;
				mSubMeshNameMap.erase(eraseIt);
			}
			else
			{
				// reduce indexes following
				if (ni->second > index)
					ni->second = ni->second - 1;

				++ni;
			}
		}

		// fix edge list data by simply recreating all edge lists
		if( mEdgeListsBuilt)
		{
			this->freeEdgeList();
			this->buildEdgeList();
		}

		if (isLoaded())
			_dirtyState();
		
	}
    //-----------------------------------------------------------------------
    void Mesh::destroySubMesh(const String& name)
	{
		unsigned short index = _getSubMeshIndex(name);
		destroySubMesh(index);
	}
	//-----------------------------------------------------------------------
    unsigned short Mesh::getNumSubMeshes() const
    {
        return static_cast< unsigned short >( mSubMeshList.size() );
    }

    //---------------------------------------------------------------------
	void Mesh::nameSubMesh(const String& name, ushort index)
	{
		mSubMeshNameMap[name] = index ;
	}

	//---------------------------------------------------------------------
	void Mesh::unnameSubMesh(const String& name)
	{
		SubMeshNameMap::iterator i = mSubMeshNameMap.find(name);
		if (i != mSubMeshNameMap.end())
			mSubMeshNameMap.erase(i);
	}
    //-----------------------------------------------------------------------
    SubMesh* Mesh::getSubMesh(const String& name) const
	{
		ushort index = _getSubMeshIndex(name);
		return getSubMesh(index);
	}
    //-----------------------------------------------------------------------
    SubMesh* Mesh::getSubMesh(unsigned short index) const
    {
        if (index >= mSubMeshList.size())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                "Index out of bounds.",
                "Mesh::getSubMesh");
        }

        return mSubMeshList[index];
    }
	//-----------------------------------------------------------------------
	void Mesh::postLoadImpl(void)
	{
		// Prepare for shadow volumes?
		if (MeshManager::getSingleton().getPrepareAllMeshesForShadowVolumes())
		{
			if (mEdgeListsBuilt || mAutoBuildEdgeLists)
			{
				prepareForShadowVolume();
			}

			if (!mEdgeListsBuilt && mAutoBuildEdgeLists)
			{
				buildEdgeList();
			}
		}
#if !OGRE_NO_MESHLOD
        // The loading process accesses LOD usages directly, so
        // transformation of user values must occur after loading is complete.

        // Transform user LOD values (starting at index 1, no need to transform base value)
		for (MeshLodUsageList::iterator i = mMeshLodUsageList.begin() + 1; i != mMeshLodUsageList.end(); ++i)
            i->value = mLodStrategy->transformUserValue(i->userValue);
#endif
	}
	//-----------------------------------------------------------------------
    void Mesh::prepareImpl()
    {
        // Load from specified 'name'
        if (getCreator()->getVerbose())
            LogManager::getSingleton().logMessage("Mesh: Loading "+mName+".");

        mFreshFromDisk =
            ResourceGroupManager::getSingleton().openResource(
				mName, mGroup, true, this);
 
        // fully prebuffer into host RAM
        mFreshFromDisk = DataStreamPtr(OGRE_NEW MemoryDataStream(mName,mFreshFromDisk));
    }
    //-----------------------------------------------------------------------
    void Mesh::unprepareImpl()
    {
        mFreshFromDisk.setNull();
    }
    void Mesh::loadImpl()
    {
        MeshSerializer serializer;
        serializer.setListener(MeshManager::getSingleton().getListener());

        // If the only copy is local on the stack, it will be cleaned
        // up reliably in case of exceptions, etc
        DataStreamPtr data(mFreshFromDisk);
        mFreshFromDisk.setNull();

        if (data.isNull()) {
            OGRE_EXCEPT(Exception::ERR_INVALID_STATE,
                        "Data doesn't appear to have been prepared in " + mName,
                        "Mesh::loadImpl()");
        }

		serializer.importMesh(data, this);

        /* check all submeshes to see if their materials should be
           updated.  If the submesh has texture aliases that match those
           found in the current material then a new material is created using
           the textures from the submesh.
        */
        updateMaterialForAllSubMeshes();
    }

    //-----------------------------------------------------------------------
    void Mesh::unloadImpl()
    {
        // Teardown submeshes
        for (SubMeshList::iterator i = mSubMeshList.begin();
            i != mSubMeshList.end(); ++i)
        {
            OGRE_DELETE *i;
        }
        if (sharedVertexData)
        {
            OGRE_DELETE sharedVertexData;
            sharedVertexData = NULL;
        }
		// Clear SubMesh lists
		mSubMeshList.clear();
		mSubMeshNameMap.clear();
#if !OGRE_NO_MESHLOD
        // Removes all LOD data
        removeLodLevels();
#endif
        mPreparedForShadowVolumes = false;

		// remove all poses & animations
		removeAllAnimations();
		removeAllPoses();

        // Clear bone assignments
        mBoneAssignments.clear();
        mBoneAssignmentsOutOfDate = false;

        // Removes reference to skeleton
        setSkeletonName(StringUtil::BLANK);
    }

    //-----------------------------------------------------------------------
    MeshPtr Mesh::clone(const String& newName, const String& newGroup)
    {
        // This is a bit like a copy constructor, but with the additional aspect of registering the clone with
        //  the MeshManager

        // New Mesh is assumed to be manually defined rather than loaded since you're cloning it for a reason
        String theGroup;
        if (newGroup == StringUtil::BLANK)
        {
            theGroup = this->getGroup();
        }
        else
        {
            theGroup = newGroup;
        }
        MeshPtr newMesh = MeshManager::getSingleton().createManual(newName, theGroup);

        // Copy submeshes first
        vector<SubMesh*>::type::iterator subi;
        for (subi = mSubMeshList.begin(); subi != mSubMeshList.end(); ++subi)
        {
            (*subi)->clone("", newMesh.get());

        }

        // Copy shared geometry and index map, if any
        if (sharedVertexData)
        {
            newMesh->sharedVertexData = sharedVertexData->clone();
            newMesh->sharedBlendIndexToBoneIndexMap = sharedBlendIndexToBoneIndexMap;
        }

		// Copy submesh names
		newMesh->mSubMeshNameMap = mSubMeshNameMap ;
        // Copy any bone assignments
        newMesh->mBoneAssignments = mBoneAssignments;
        newMesh->mBoneAssignmentsOutOfDate = mBoneAssignmentsOutOfDate;
        // Copy bounds
        newMesh->mAABB = mAABB;
        newMesh->mBoundRadius = mBoundRadius;
        newMesh->mBoneBoundingRadius = mBoneBoundingRadius;
        newMesh->mAutoBuildEdgeLists = mAutoBuildEdgeLists;
		newMesh->mEdgeListsBuilt = mEdgeListsBuilt;

#if !OGRE_NO_MESHLOD
		newMesh->mHasManualLodLevel = mHasManualLodLevel;
        newMesh->mLodStrategy = mLodStrategy;
        newMesh->mNumLods = mNumLods;
        newMesh->mMeshLodUsageList = mMeshLodUsageList;
#endif
        // Unreference edge lists, otherwise we'll delete the same lot twice, build on demand
        MeshLodUsageList::iterator lodi, lodOldi;
		lodOldi = mMeshLodUsageList.begin();
		for (lodi = newMesh->mMeshLodUsageList.begin(); lodi != newMesh->mMeshLodUsageList.end(); ++lodi, ++lodOldi) {
            MeshLodUsage& newLod = *lodi;
			MeshLodUsage& lod = *lodOldi;
			newLod.manualName = lod.manualName;
			newLod.userValue = lod.userValue;
			newLod.value = lod.value;
			if (lod.edgeData) {
				newLod.edgeData = lod.edgeData->clone();
        }
        }
		newMesh->mVertexBufferUsage = mVertexBufferUsage;
		newMesh->mIndexBufferUsage = mIndexBufferUsage;
		newMesh->mVertexBufferShadowBuffer = mVertexBufferShadowBuffer;
		newMesh->mIndexBufferShadowBuffer = mIndexBufferShadowBuffer;

        newMesh->mSkeletonName = mSkeletonName;
        newMesh->mSkeleton = mSkeleton;

		// Keep prepared shadow volume info (buffers may already be prepared)
		newMesh->mPreparedForShadowVolumes = mPreparedForShadowVolumes;

		newMesh->mEdgeListsBuilt = mEdgeListsBuilt;
		
		// Clone vertex animation
		for (AnimationList::iterator i = mAnimationsList.begin();
			i != mAnimationsList.end(); ++i)
		{
			Animation *newAnim = i->second->clone(i->second->getName());
			newMesh->mAnimationsList[i->second->getName()] = newAnim;
		}
		// Clone pose list
		for (PoseList::iterator i = mPoseList.begin(); i != mPoseList.end(); ++i)
		{
			Pose* newPose = (*i)->clone();
			newMesh->mPoseList.push_back(newPose);
		}
		newMesh->mSharedVertexDataAnimationType = mSharedVertexDataAnimationType;
		newMesh->mAnimationTypesDirty = true;

        newMesh->load();
        newMesh->touch();

        return newMesh;
    }
    //-----------------------------------------------------------------------
    const AxisAlignedBox& Mesh::getBounds(void) const
    {
        return mAABB;
    }
    //-----------------------------------------------------------------------
    void Mesh::_setBounds(const AxisAlignedBox& bounds, bool pad)
    {
        mAABB = bounds;
		mBoundRadius = Math::boundingRadiusFromAABB(mAABB);

		if( mAABB.isFinite() )
		{
			Vector3 max = mAABB.getMaximum();
			Vector3 min = mAABB.getMinimum();

			if (pad)
			{
				// Pad out the AABB a little, helps with most bounds tests
				Vector3 scaler = (max - min) * MeshManager::getSingleton().getBoundsPaddingFactor();
				mAABB.setExtents(min  - scaler, max + scaler);
				// Pad out the sphere a little too
				mBoundRadius = mBoundRadius + (mBoundRadius * MeshManager::getSingleton().getBoundsPaddingFactor());
			}
		}
    }
    //-----------------------------------------------------------------------
    void Mesh::_setBoundingSphereRadius(Real radius)
    {
        mBoundRadius = radius;
    }
    //-----------------------------------------------------------------------
    void Mesh::_setBoneBoundingRadius(Real radius)
    {
        mBoneBoundingRadius = radius;
    }
    //-----------------------------------------------------------------------
	void Mesh::_updateBoundsFromVertexBuffers(bool pad)
	{
		bool extendOnly = false; // First time we need full AABB of the given submesh, but on the second call just extend that one.
		if (sharedVertexData){
			_calcBoundsFromVertexBuffer(sharedVertexData, mAABB, mBoundRadius, extendOnly);
			extendOnly = true;
		}
		for (size_t i = 0; i < mSubMeshList.size(); i++){
			if (mSubMeshList[i]->vertexData){
				_calcBoundsFromVertexBuffer(mSubMeshList[i]->vertexData, mAABB, mBoundRadius, extendOnly);
				extendOnly = true;
			}
		}
		if (pad)
		{
			Vector3 max = mAABB.getMaximum();
			Vector3 min = mAABB.getMinimum();
			// Pad out the AABB a little, helps with most bounds tests
			Vector3 scaler = (max - min) * MeshManager::getSingleton().getBoundsPaddingFactor();
			mAABB.setExtents(min - scaler, max + scaler);
			// Pad out the sphere a little too
			mBoundRadius = mBoundRadius + (mBoundRadius * MeshManager::getSingleton().getBoundsPaddingFactor());
		}
	}
	void Mesh::_calcBoundsFromVertexBuffer(VertexData* vertexData, AxisAlignedBox& outAABB, Real& outRadius, bool extendOnly /*= false*/)
	{
		if (vertexData->vertexCount == 0) {
			if (!extendOnly) {
				outAABB = AxisAlignedBox(Vector3::ZERO, Vector3::ZERO);
				outRadius = 0;
			}
			return;
		}
		const VertexElement* elemPos = vertexData->vertexDeclaration->findElementBySemantic(VES_POSITION);
		HardwareVertexBufferSharedPtr vbuf = vertexData->vertexBufferBinding->getBuffer(elemPos->getSource());
		
		unsigned char* vertex = static_cast<unsigned char*>(vbuf->lock(HardwareBuffer::HBL_READ_ONLY));

		if (!extendOnly){
			// init values
			outRadius = 0;
			float* pFloat;
			elemPos->baseVertexPointerToElement(vertex, &pFloat);
			Vector3 basePos(pFloat[0], pFloat[1], pFloat[2]);
			outAABB.setExtents(basePos, basePos);
		}
		int vSize = vbuf->getVertexSize();
		unsigned char* vEnd = vertex + vertexData->vertexCount * vSize;
		Real radiusSqr = outRadius * outRadius;
		// Loop through all vertices.
		for (; vertex < vEnd; vertex += vSize) {
			float* pFloat;
			elemPos->baseVertexPointerToElement(vertex, &pFloat);
			Vector3 pos(pFloat[0], pFloat[1], pFloat[2]);
			outAABB.getMinimum().makeFloor(pos);
			outAABB.getMaximum().makeCeil(pos);
			radiusSqr = std::max<Real>(radiusSqr, pos.squaredLength());
		}
		outRadius = std::sqrt(radiusSqr);
	}
    //-----------------------------------------------------------------------
    void Mesh::setSkeletonName(const String& skelName)
    {
		if (skelName != mSkeletonName)
		{
			mSkeletonName = skelName;

			if (skelName.empty())
			{
				// No skeleton
				mSkeleton.setNull();
			}
			else
			{
				// Load skeleton
				try {
					mSkeleton = SkeletonManager::getSingleton().load(skelName, mGroup).staticCast<Skeleton>();
				}
				catch (...)
				{
					mSkeleton.setNull();
					// Log this error
					String msg = "Unable to load skeleton ";
					msg += skelName + " for Mesh " + mName
						+ ". This Mesh will not be animated. "
						+ "You can ignore this message if you are using an offline tool.";
					LogManager::getSingleton().logMessage(msg);

				}


			}
			if (isLoaded())
				_dirtyState();
		}
    }
    //-----------------------------------------------------------------------
    bool Mesh::hasSkeleton(void) const
    {
        return !(mSkeletonName.empty());
    }
    //-----------------------------------------------------------------------
    const SkeletonPtr& Mesh::getSkeleton(void) const
    {
        return mSkeleton;
    }
    //-----------------------------------------------------------------------
    void Mesh::addBoneAssignment(const VertexBoneAssignment& vertBoneAssign)
    {
        mBoneAssignments.insert(
            VertexBoneAssignmentList::value_type(vertBoneAssign.vertexIndex, vertBoneAssign));
        mBoneAssignmentsOutOfDate = true;
    }
    //-----------------------------------------------------------------------
    void Mesh::clearBoneAssignments(void)
    {
        mBoneAssignments.clear();
        mBoneAssignmentsOutOfDate = true;
    }
    //-----------------------------------------------------------------------
    void Mesh::_initAnimationState(AnimationStateSet* animSet)
    {
		// Animation states for skeletal animation
		if (!mSkeleton.isNull())
		{
			// Delegate to Skeleton
			mSkeleton->_initAnimationState(animSet);

			// Take the opportunity to update the compiled bone assignments
            _updateCompiledBoneAssignments();

        }

		// Animation states for vertex animation
		for (AnimationList::iterator i = mAnimationsList.begin();
			i != mAnimationsList.end(); ++i)
		{
			// Only create a new animation state if it doesn't exist
			// We can have the same named animation in both skeletal and vertex
			// with a shared animation state affecting both, for combined effects
			// The animations should be the same length if this feature is used!
			if (!animSet->hasAnimationState(i->second->getName()))
			{
				animSet->createAnimationState(i->second->getName(), 0.0,
					i->second->getLength());
			}

		}

    }
	//---------------------------------------------------------------------
	void Mesh::_refreshAnimationState(AnimationStateSet* animSet)
	{
		if (!mSkeleton.isNull())
		{
			mSkeleton->_refreshAnimationState(animSet);
		}

		// Merge in any new vertex animations
		AnimationList::iterator i;
		for (i = mAnimationsList.begin(); i != mAnimationsList.end(); ++i)
		{
			Animation* anim = i->second;
			// Create animation at time index 0, default params mean this has weight 1 and is disabled
			const String& animName = anim->getName();
			if (!animSet->hasAnimationState(animName))
			{
				animSet->createAnimationState(animName, 0.0, anim->getLength());
			}
			else
			{
				// Update length incase changed
				AnimationState* animState = animSet->getAnimationState(animName);
				animState->setLength(anim->getLength());
				animState->setTimePosition(std::min(anim->getLength(), animState->getTimePosition()));
			}
		}

	}
    //-----------------------------------------------------------------------
    void Mesh::_updateCompiledBoneAssignments(void)
    {
        if (mBoneAssignmentsOutOfDate)
            _compileBoneAssignments();

        SubMeshList::iterator i;
        for (i = mSubMeshList.begin(); i != mSubMeshList.end(); ++i)
        {
            if ((*i)->mBoneAssignmentsOutOfDate)
            {
                (*i)->_compileBoneAssignments();
            }
        }
    }
    //-----------------------------------------------------------------------
    typedef multimap<Real, Mesh::VertexBoneAssignmentList::iterator>::type WeightIteratorMap;
    unsigned short Mesh::_rationaliseBoneAssignments(size_t vertexCount, Mesh::VertexBoneAssignmentList& assignments)
    {
        // Iterate through, finding the largest # bones per vertex
        unsigned short maxBones = 0;
		bool existsNonSkinnedVertices = false;
        VertexBoneAssignmentList::iterator i;

        for (size_t v = 0; v < vertexCount; ++v)
        {
            // Get number of entries for this vertex
            short currBones = static_cast<unsigned short>(assignments.count(v));
			if (currBones <= 0)
				existsNonSkinnedVertices = true;

            // Deal with max bones update
            // (note this will record maxBones even if they exceed limit)
            if (maxBones < currBones)
                maxBones = currBones;
            // does the number of bone assignments exceed limit?
            if (currBones > OGRE_MAX_BLEND_WEIGHTS)
            {
                // To many bone assignments on this vertex
                // Find the start & end (end is in iterator terms ie exclusive)
                std::pair<VertexBoneAssignmentList::iterator, VertexBoneAssignmentList::iterator> range;
                // map to sort by weight
                WeightIteratorMap weightToAssignmentMap;
                range = assignments.equal_range(v);
                // Add all the assignments to map
                for (i = range.first; i != range.second; ++i)
                {
                    // insert value weight->iterator
                    weightToAssignmentMap.insert(
                        WeightIteratorMap::value_type(i->second.weight, i));
                }
                // Reverse iterate over weight map, remove lowest n
                unsigned short numToRemove = currBones - OGRE_MAX_BLEND_WEIGHTS;
                WeightIteratorMap::iterator remIt = weightToAssignmentMap.begin();

                while (numToRemove--)
                {
                    // Erase this one
                    assignments.erase(remIt->second);
                    ++remIt;
                }
            } // if (currBones > OGRE_MAX_BLEND_WEIGHTS)

            // Make sure the weights are normalised
            // Do this irrespective of whether we had to remove assignments or not
            //   since it gives us a guarantee that weights are normalised
            //  We assume this, so it's a good idea since some modellers may not
            std::pair<VertexBoneAssignmentList::iterator, VertexBoneAssignmentList::iterator> normalise_range = assignments.equal_range(v);
            Real totalWeight = 0;
            // Find total first
            for (i = normalise_range.first; i != normalise_range.second; ++i)
            {
                totalWeight += i->second.weight;
            }
            // Now normalise if total weight is outside tolerance
            if (!Math::RealEqual(totalWeight, 1.0f))
            {
                for (i = normalise_range.first; i != normalise_range.second; ++i)
                {
                    i->second.weight = i->second.weight / totalWeight;
                }
            }

        }

		if (maxBones > OGRE_MAX_BLEND_WEIGHTS)
		{
            // Warn that we've reduced bone assignments
            LogManager::getSingleton().logMessage("WARNING: the mesh '" + mName + "' "
                "includes vertices with more than " +
                StringConverter::toString(OGRE_MAX_BLEND_WEIGHTS) + " bone assignments. "
                "The lowest weighted assignments beyond this limit have been removed, so "
                "your animation may look slightly different. To eliminate this, reduce "
                "the number of bone assignments per vertex on your mesh to " +
                StringConverter::toString(OGRE_MAX_BLEND_WEIGHTS) + ".", LML_CRITICAL);
            // we've adjusted them down to the max
            maxBones = OGRE_MAX_BLEND_WEIGHTS;

        }

		if (existsNonSkinnedVertices)
		{
            // Warn that we've non-skinned vertices
            LogManager::getSingleton().logMessage("WARNING: the mesh '" + mName + "' "
                "includes vertices without bone assignments. Those vertices will "
				"transform to wrong position when skeletal animation enabled. "
				"To eliminate this, assign at least one bone assignment per vertex "
				"on your mesh.", LML_CRITICAL);
		}

        return maxBones;
    }
    //-----------------------------------------------------------------------
    void  Mesh::_compileBoneAssignments(void)
    {
		if (sharedVertexData)
		{
			unsigned short maxBones = _rationaliseBoneAssignments(sharedVertexData->vertexCount, mBoneAssignments);

			if (maxBones != 0)
			{
				compileBoneAssignments(mBoneAssignments, maxBones, 
					sharedBlendIndexToBoneIndexMap, sharedVertexData);
			}
		}
        mBoneAssignmentsOutOfDate = false;
    }
    //---------------------------------------------------------------------
    void Mesh::buildIndexMap(const VertexBoneAssignmentList& boneAssignments,
        IndexMap& boneIndexToBlendIndexMap, IndexMap& blendIndexToBoneIndexMap)
    {
        if (boneAssignments.empty())
        {
            // Just in case
            boneIndexToBlendIndexMap.clear();
            blendIndexToBoneIndexMap.clear();
            return;
        }

        typedef set<unsigned short>::type BoneIndexSet;
        BoneIndexSet usedBoneIndices;

        // Collect actually used bones
        VertexBoneAssignmentList::const_iterator itVBA, itendVBA;
        itendVBA = boneAssignments.end();
        for (itVBA = boneAssignments.begin(); itVBA != itendVBA; ++itVBA)
        {
            usedBoneIndices.insert(itVBA->second.boneIndex);
        }

        // Allocate space for index map
        blendIndexToBoneIndexMap.resize(usedBoneIndices.size());
        boneIndexToBlendIndexMap.resize(*usedBoneIndices.rbegin() + 1);

        // Make index map between bone index and blend index
        BoneIndexSet::const_iterator itBoneIndex, itendBoneIndex;
        unsigned short blendIndex = 0;
        itendBoneIndex = usedBoneIndices.end();
        for (itBoneIndex = usedBoneIndices.begin(); itBoneIndex != itendBoneIndex; ++itBoneIndex, ++blendIndex)
        {
            boneIndexToBlendIndexMap[*itBoneIndex] = blendIndex;
            blendIndexToBoneIndexMap[blendIndex] = *itBoneIndex;
        }
    }
    //---------------------------------------------------------------------
    void Mesh::compileBoneAssignments(
        const VertexBoneAssignmentList& boneAssignments,
        unsigned short numBlendWeightsPerVertex,
        IndexMap& blendIndexToBoneIndexMap,
        VertexData* targetVertexData)
    {
        // Create or reuse blend weight / indexes buffer
        // Indices are always a UBYTE4 no matter how many weights per vertex
        // Weights are more specific though since they are Reals
        VertexDeclaration* decl = targetVertexData->vertexDeclaration;
        VertexBufferBinding* bind = targetVertexData->vertexBufferBinding;
        unsigned short bindIndex;

        // Build the index map brute-force. It's possible to store the index map
        // in .mesh, but maybe trivial.
        IndexMap boneIndexToBlendIndexMap;
        buildIndexMap(boneAssignments, boneIndexToBlendIndexMap, blendIndexToBoneIndexMap);

        const VertexElement* testElem =
            decl->findElementBySemantic(VES_BLEND_INDICES);
        if (testElem)
        {
            // Already have a buffer, unset it & delete elements
            bindIndex = testElem->getSource();
            // unset will cause deletion of buffer
            bind->unsetBinding(bindIndex);
            decl->removeElement(VES_BLEND_INDICES);
            decl->removeElement(VES_BLEND_WEIGHTS);
        }
        else
        {
            // Get new binding
            bindIndex = bind->getNextIndex();
        }

        HardwareVertexBufferSharedPtr vbuf =
            HardwareBufferManager::getSingleton().createVertexBuffer(
                sizeof(unsigned char)*4 + sizeof(float)*numBlendWeightsPerVertex,
                targetVertexData->vertexCount,
                HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                true // use shadow buffer
                );
        // bind new buffer
        bind->setBinding(bindIndex, vbuf);
        const VertexElement *pIdxElem, *pWeightElem;

        // add new vertex elements
        // Note, insert directly after all elements using the same source as
        // position to abide by pre-Dx9 format restrictions
        const VertexElement* firstElem = decl->getElement(0);
        if(firstElem->getSemantic() == VES_POSITION)
        {
            unsigned short insertPoint = 1;
            while (insertPoint < decl->getElementCount() &&
                decl->getElement(insertPoint)->getSource() == firstElem->getSource())
            {
                ++insertPoint;
            }
            const VertexElement& idxElem =
                decl->insertElement(insertPoint, bindIndex, 0, VET_UBYTE4, VES_BLEND_INDICES);
            const VertexElement& wtElem =
                decl->insertElement(insertPoint+1, bindIndex, sizeof(unsigned char)*4,
                VertexElement::multiplyTypeCount(VET_FLOAT1, numBlendWeightsPerVertex),
                VES_BLEND_WEIGHTS);
            pIdxElem = &idxElem;
            pWeightElem = &wtElem;
        }
        else
        {
            // Position is not the first semantic, therefore this declaration is
            // not pre-Dx9 compatible anyway, so just tack it on the end
            const VertexElement& idxElem =
                decl->addElement(bindIndex, 0, VET_UBYTE4, VES_BLEND_INDICES);
            const VertexElement& wtElem =
                decl->addElement(bindIndex, sizeof(unsigned char)*4,
                VertexElement::multiplyTypeCount(VET_FLOAT1, numBlendWeightsPerVertex),
                VES_BLEND_WEIGHTS);
            pIdxElem = &idxElem;
            pWeightElem = &wtElem;
        }

        // Assign data
        size_t v;
        VertexBoneAssignmentList::const_iterator i, iend;
        i = boneAssignments.begin();
		iend = boneAssignments.end();
        unsigned char *pBase = static_cast<unsigned char*>(
            vbuf->lock(HardwareBuffer::HBL_DISCARD));
        // Iterate by vertex
        float *pWeight;
        unsigned char *pIndex;
        for (v = 0; v < targetVertexData->vertexCount; ++v)
        {
            /// Convert to specific pointers
            pWeightElem->baseVertexPointerToElement(pBase, &pWeight);
            pIdxElem->baseVertexPointerToElement(pBase, &pIndex);
            for (unsigned short bone = 0; bone < numBlendWeightsPerVertex; ++bone)
            {
                // Do we still have data for this vertex?
                if (i != iend && i->second.vertexIndex == v)
                {
                    // If so, write weight
                    *pWeight++ = i->second.weight;
                    *pIndex++ = static_cast<unsigned char>(boneIndexToBlendIndexMap[i->second.boneIndex]);
                    ++i;
                }
                else
                {
                    // Ran out of assignments for this vertex, use weight 0 to indicate empty.
					// If no bones are defined (an error in itself) set bone 0 as the assigned bone. 
                    *pWeight++ = (bone == 0) ? 1.0f : 0.0f;
                    *pIndex++ = 0;
                }
            }
            pBase += vbuf->getVertexSize();
        }

        vbuf->unlock();

    }
    //---------------------------------------------------------------------
    Real distLineSegToPoint( const Vector3& line0, const Vector3& line1, const Vector3& pt )
    {
        Vector3 v01 = line1 - line0;
        Real tt = v01.dotProduct( pt - line0 ) / std::max( v01.dotProduct(v01), std::numeric_limits<Real>::epsilon() );
        tt = Math::Clamp( tt, Real(0.0f), Real(1.0f) );
        Vector3 onLine = line0 + tt * v01;
        return pt.distance( onLine );
    }
    //---------------------------------------------------------------------
    Real _computeBoneBoundingRadiusHelper( VertexData* vertexData,
        const Mesh::VertexBoneAssignmentList& boneAssignments,
        const vector<Vector3>::type& bonePositions,
        const vector< vector<ushort>::type >::type& boneChildren
        )
    {
        vector<Vector3>::type vertexPositions;
        {
            // extract vertex positions
            const VertexElement* posElem = vertexData->vertexDeclaration->findElementBySemantic(VES_POSITION);
            HardwareVertexBufferSharedPtr vbuf = vertexData->vertexBufferBinding->getBuffer(posElem->getSource());
            // if usage is write only,
            if ( !vbuf->hasShadowBuffer() && (vbuf->getUsage() & HardwareBuffer::HBU_WRITE_ONLY) )
            {
                // can't do it
                return Real(0.0f);
            }
            vertexPositions.resize( vertexData->vertexCount );
            unsigned char* vertex = static_cast<unsigned char*>(vbuf->lock(HardwareBuffer::HBL_READ_ONLY));
            float* pFloat;

            for(size_t i = 0; i < vertexData->vertexCount; ++i)
            {
                posElem->baseVertexPointerToElement(vertex, &pFloat);
                vertexPositions[ i ] = Vector3( pFloat[0], pFloat[1], pFloat[2] );
                vertex += vbuf->getVertexSize();
            }
            vbuf->unlock();
        }
        Real maxRadius = Real(0);
        Real minWeight = Real(0.01);
        // for each vertex-bone assignment,
        for (Mesh::VertexBoneAssignmentList::const_iterator i = boneAssignments.begin(); i != boneAssignments.end(); ++i)
        {
            // if weight is close to zero, ignore
            if (i->second.weight > minWeight)
            {
                // if we have a bounding box around all bone origins, we consider how far outside this box the
                // current vertex could ever get (assuming it is only attached to the given bone, and the bones all have unity scale)
                size_t iBone = i->second.boneIndex;
                const Vector3& v = vertexPositions[ i->second.vertexIndex ];
                Vector3 diff = v - bonePositions[ iBone ];
                Real dist = diff.length();  // max distance of vertex v outside of bounding box
                // if this bone has children, we can reduce the dist under the assumption that the children may rotate wrt their parent, but don't translate
                for (size_t iChild = 0; iChild < boneChildren[iBone].size(); ++iChild)
                {
                    // given this assumption, we know that the bounding box will enclose both the bone origin as well as the origin of the child bone,
                    // and therefore everything on a line segment between the bone origin and the child bone will be inside the bounding box as well
                    size_t iChildBone = boneChildren[ iBone ][ iChild ];
                    // compute distance from vertex to line segment between bones
                    float distChild = distLineSegToPoint( bonePositions[ iBone ], bonePositions[ iChildBone ], v );
                    dist = std::min( dist, distChild );
                }
                // scale the distance by the weight, this prevents the radius from being over-inflated because of a vertex that is lightly influenced by a faraway bone
                dist *= i->second.weight;
                maxRadius = std::max( maxRadius, dist );
            }
        }
        return maxRadius;
    }
    //---------------------------------------------------------------------
    void Mesh::_computeBoneBoundingRadius()
    {
        if (mBoneBoundingRadius == Real(0) && ! mSkeleton.isNull())
        {
            Real radius = Real(0);
            vector<Vector3>::type bonePositions;
            vector< vector<ushort>::type >::type boneChildren;  // for each bone, a list of children
            {
                // extract binding pose bone positions, and also indices for child bones
                size_t numBones = mSkeleton->getNumBones();
                mSkeleton->setBindingPose();
                mSkeleton->_updateTransforms();
                bonePositions.resize( numBones );
                boneChildren.resize( numBones );
                // for each bone,
                for (size_t iBone = 0; iBone < numBones; ++iBone)
                {
                    Bone* bone = mSkeleton->getBone( iBone );
                    bonePositions[ iBone ] = bone->_getDerivedPosition();
                    boneChildren[ iBone ].reserve( bone->numChildren() );
                    for (size_t iChild = 0; iChild < bone->numChildren(); ++iChild)
                    {
                        Bone* child = static_cast<Bone*>( bone->getChild( iChild ) );
                        boneChildren[ iBone ].push_back( child->getHandle() );
                    }
                }
            }
            if (sharedVertexData)
            {
                // check shared vertices
                radius = _computeBoneBoundingRadiusHelper(sharedVertexData, mBoneAssignments, bonePositions, boneChildren);
            }

            // check submesh vertices
            SubMeshList::const_iterator itor = mSubMeshList.begin();
            SubMeshList::const_iterator end  = mSubMeshList.end();

            while( itor != end )
            {
                SubMesh* submesh = *itor;
                if (!submesh->useSharedVertices && submesh->vertexData)
                {
                    Real r = _computeBoneBoundingRadiusHelper(submesh->vertexData, submesh->mBoneAssignments, bonePositions, boneChildren);
                    radius = std::max( radius, r );
                }
                ++itor;
            }
            if (radius > Real(0))
            {
                mBoneBoundingRadius = radius;
            }
            else
            {
                // fallback if we failed to find the vertices
                mBoneBoundingRadius = mBoundRadius;
            }
        }
    }
    //---------------------------------------------------------------------
    void Mesh::_notifySkeleton(SkeletonPtr& pSkel)
    {
        mSkeleton = pSkel;
        mSkeletonName = pSkel->getName();
    }
    //---------------------------------------------------------------------
    Mesh::BoneAssignmentIterator Mesh::getBoneAssignmentIterator(void)
    {
        return BoneAssignmentIterator(mBoneAssignments.begin(),
            mBoneAssignments.end());
    }
    //---------------------------------------------------------------------
    const String& Mesh::getSkeletonName(void) const
    {
        return mSkeletonName;
    }
    //---------------------------------------------------------------------
    ushort Mesh::getNumLodLevels(void) const
    {
        return mNumLods;
    }
    //---------------------------------------------------------------------
    const MeshLodUsage& Mesh::getLodLevel(ushort index) const
    {
#if !OGRE_NO_MESHLOD
        index = std::min(index, (ushort)(mMeshLodUsageList.size() - 1));
        if (this->_isManualLodLevel(index) && index > 0 && mMeshLodUsageList[index].manualMesh.isNull())
        {
            // Load the mesh now
			try {
				mMeshLodUsageList[index].manualMesh =
					MeshManager::getSingleton().load(
						mMeshLodUsageList[index].manualName,
						getGroup());
				// get the edge data, if required
				if (!mMeshLodUsageList[index].edgeData)
				{
					mMeshLodUsageList[index].edgeData =
						mMeshLodUsageList[index].manualMesh->getEdgeList(0);
				}
			}
			catch (Exception& )
			{
				LogManager::getSingleton().stream()
					<< "Error while loading manual LOD level "
					<< mMeshLodUsageList[index].manualName
					<< " - this LOD level will not be rendered. You can "
					<< "ignore this error in offline mesh tools.";
			}

        }
        return mMeshLodUsageList[index];
#else
		return mMeshLodUsageList[0];
#endif
    }
	//---------------------------------------------------------------------
	ushort Mesh::getLodIndex(Real value) const
	{
#if !OGRE_NO_MESHLOD
		// Get index from strategy
		return mLodStrategy->getIndex(value, mMeshLodUsageList);
#else
		return 0;
#endif
	}
    //---------------------------------------------------------------------
#if !OGRE_NO_MESHLOD
	void Mesh::updateManualLodLevel(ushort index, const String& meshName)
	{

		// Basic prerequisites
		assert(index != 0 && "Can't modify first LOD level (full detail)");
		assert(index < mMeshLodUsageList.size() && "Idndex out of bounds");
		// get lod
		MeshLodUsage* lod = &(mMeshLodUsageList[index]);

		lod->manualName = meshName;
		lod->manualMesh.setNull();
        if (lod->edgeData) OGRE_DELETE lod->edgeData;
        lod->edgeData = 0;
	}
    //---------------------------------------------------------------------
	void Mesh::_setLodInfo(unsigned short numLevels)
	{
        assert(!mEdgeListsBuilt && "Can't modify LOD after edge lists built");

		// Basic prerequisites
        assert(numLevels > 0 && "Must be at least one level (full detail level must exist)");

		mNumLods = numLevels;
		mMeshLodUsageList.resize(numLevels);
		// Resize submesh face data lists too
		for (SubMeshList::iterator i = mSubMeshList.begin(); i != mSubMeshList.end(); ++i)
		{
			(*i)->mLodFaceList.resize(numLevels - 1);
		}
	}
    //---------------------------------------------------------------------
	void Mesh::_setLodUsage(unsigned short level, MeshLodUsage& usage)
	{
        assert(!mEdgeListsBuilt && "Can't modify LOD after edge lists built");

		// Basic prerequisites
		assert(level != 0 && "Can't modify first LOD level (full detail)");
		assert(level < mMeshLodUsageList.size() && "Index out of bounds");

		mMeshLodUsageList[level] = usage;

		if(!mMeshLodUsageList[level].manualName.empty()){
			mHasManualLodLevel = true;
		}
	}
	//---------------------------------------------------------------------
	void Mesh::_setSubMeshLodFaceList(unsigned short subIdx, unsigned short level,
		IndexData* facedata)
	{
		assert(!mEdgeListsBuilt && "Can't modify LOD after edge lists built");

		// Basic prerequisites
		assert(mMeshLodUsageList[level].manualName.empty() && "Not using generated LODs!");
		assert(subIdx < mSubMeshList.size() && "Index out of bounds");
		assert(level != 0 && "Can't modify first LOD level (full detail)");
		assert(level-1 < (unsigned short)mSubMeshList[subIdx]->mLodFaceList.size() && "Index out of bounds");

		SubMesh* sm = mSubMeshList[subIdx];
		sm->mLodFaceList[level - 1] = facedata;
	}
#endif
	//---------------------------------------------------------------------
	bool Mesh::_isManualLodLevel( unsigned short level ) const
	{
#if !OGRE_NO_MESHLOD
		return !mMeshLodUsageList[level].manualName.empty();
#else
		return false;
#endif
	}
    //---------------------------------------------------------------------
	ushort Mesh::_getSubMeshIndex(const String& name) const
	{
		SubMeshNameMap::const_iterator i = mSubMeshNameMap.find(name) ;
		if (i == mSubMeshNameMap.end())
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "No SubMesh named " + name + " found.",
                "Mesh::_getSubMeshIndex");

		return i->second;
	}
    //--------------------------------------------------------------------
    void Mesh::removeLodLevels(void)
    {
#if !OGRE_NO_MESHLOD
        // Remove data from SubMeshes
        SubMeshList::iterator isub, isubend;
        isubend = mSubMeshList.end();
        for (isub = mSubMeshList.begin(); isub != isubend; ++isub)
        {
            (*isub)->removeLodLevels();
        }

        freeEdgeList();

        // Reinitialise
        mNumLods = 1;
		mMeshLodUsageList.resize(1);
		mMeshLodUsageList[0].edgeData = NULL;
		// TODO: Shouldn't we rebuild edge lists after freeing them?
#endif
    }

    //---------------------------------------------------------------------
    Real Mesh::getBoundingSphereRadius(void) const
    {
        return mBoundRadius;
    }
    //---------------------------------------------------------------------
    Real Mesh::getBoneBoundingRadius(void) const
    {
        return mBoneBoundingRadius;
    }
    //---------------------------------------------------------------------
	void Mesh::setVertexBufferPolicy(HardwareBuffer::Usage vbUsage, bool shadowBuffer)
	{
		mVertexBufferUsage = vbUsage;
		mVertexBufferShadowBuffer = shadowBuffer;
	}
    //---------------------------------------------------------------------
	void Mesh::setIndexBufferPolicy(HardwareBuffer::Usage vbUsage, bool shadowBuffer)
	{
		mIndexBufferUsage = vbUsage;
		mIndexBufferShadowBuffer = shadowBuffer;
	}
	//---------------------------------------------------------------------
	void Mesh::mergeAdjacentTexcoords( unsigned short finalTexCoordSet,
										unsigned short texCoordSetToDestroy )
	{
		if( sharedVertexData )
			mergeAdjacentTexcoords( finalTexCoordSet, texCoordSetToDestroy, sharedVertexData );

		SubMeshList::const_iterator itor = mSubMeshList.begin();
		SubMeshList::const_iterator end  = mSubMeshList.end();

		while( itor != end )
		{
			if( !(*itor)->useSharedVertices )
				mergeAdjacentTexcoords( finalTexCoordSet, texCoordSetToDestroy, (*itor)->vertexData );
			++itor;
		}
	}
	//---------------------------------------------------------------------
	void Mesh::mergeAdjacentTexcoords( unsigned short finalTexCoordSet,
										unsigned short texCoordSetToDestroy,
										VertexData *vertexData )
	{
		VertexDeclaration *vDecl	= vertexData->vertexDeclaration;

	    const VertexElement *uv0 = vDecl->findElementBySemantic( VES_TEXTURE_COORDINATES,
																	finalTexCoordSet );
		const VertexElement *uv1 = vDecl->findElementBySemantic( VES_TEXTURE_COORDINATES,
																	texCoordSetToDestroy );

		if( uv0 && uv1 )
		{
			//Check that both base types are compatible (mix floats w/ shorts) and there's enough space
			VertexElementType baseType0 = VertexElement::getBaseType( uv0->getType() );
			VertexElementType baseType1 = VertexElement::getBaseType( uv1->getType() );

			unsigned short totalTypeCount = VertexElement::getTypeCount( uv0->getType() ) +
											VertexElement::getTypeCount( uv1->getType() );
			if( baseType0 == baseType1 && totalTypeCount <= 4 )
			{
				const VertexDeclaration::VertexElementList &veList = vDecl->getElements();
				VertexDeclaration::VertexElementList::const_iterator uv0Itor = std::find( veList.begin(),
																					veList.end(), *uv0 );
				unsigned short elem_idx		= std::distance( veList.begin(), uv0Itor );
				VertexElementType newType	= VertexElement::multiplyTypeCount( baseType0,
																				totalTypeCount );

				if( ( uv0->getOffset() + uv0->getSize() == uv1->getOffset() ||
					  uv1->getOffset() + uv1->getSize() == uv0->getOffset() ) &&
					uv0->getSource() == uv1->getSource() )
				{
					//Special case where they adjacent, just change the declaration & we're done.
					size_t newOffset		= std::min( uv0->getOffset(), uv1->getOffset() );
					unsigned short newIdx	= std::min( uv0->getIndex(), uv1->getIndex() );

					vDecl->modifyElement( elem_idx, uv0->getSource(), newOffset, newType,
											VES_TEXTURE_COORDINATES, newIdx );
					vDecl->removeElement( VES_TEXTURE_COORDINATES, texCoordSetToDestroy );
					uv1 = 0;
				}

				vDecl->closeGapsInSource();
			}
		}
	}
    //---------------------------------------------------------------------
    void Mesh::organiseTangentsBuffer(VertexData *vertexData,
        VertexElementSemantic targetSemantic, unsigned short index, 
		unsigned short sourceTexCoordSet)
    {
	    VertexDeclaration *vDecl = vertexData->vertexDeclaration ;
	    VertexBufferBinding *vBind = vertexData->vertexBufferBinding ;

	    const VertexElement *tangentsElem = vDecl->findElementBySemantic(targetSemantic, index);
	    bool needsToBeCreated = false;

	    if (!tangentsElem)
        { // no tex coords with index 1
			    needsToBeCreated = true ;
	    }
        else if (tangentsElem->getType() != VET_FLOAT3)
        {
            //  buffer exists, but not 3D
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                "Target semantic set already exists but is not 3D, therefore "
				"cannot contain tangents. Pick an alternative destination semantic. ",
                "Mesh::organiseTangentsBuffer");
	    }

	    HardwareVertexBufferSharedPtr newBuffer;
	    if (needsToBeCreated)
        {
            // To be most efficient with our vertex streams,
            // tack the new tangents onto the same buffer as the
            // source texture coord set
            const VertexElement* prevTexCoordElem =
                vertexData->vertexDeclaration->findElementBySemantic(
                    VES_TEXTURE_COORDINATES, sourceTexCoordSet);
            if (!prevTexCoordElem)
            {
                OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
                    "Cannot locate the first texture coordinate element to "
					"which to append the new tangents.", 
					"Mesh::orgagniseTangentsBuffer");
            }
            // Find the buffer associated with  this element
            HardwareVertexBufferSharedPtr origBuffer =
                vertexData->vertexBufferBinding->getBuffer(
                    prevTexCoordElem->getSource());
            // Now create a new buffer, which includes the previous contents
            // plus extra space for the 3D coords
		    newBuffer = HardwareBufferManager::getSingleton().createVertexBuffer(
                origBuffer->getVertexSize() + 3*sizeof(float),
                vertexData->vertexCount,
			    origBuffer->getUsage(),
			    origBuffer->hasShadowBuffer() );
            // Add the new element
		    vDecl->addElement(
                prevTexCoordElem->getSource(),
                origBuffer->getVertexSize(),
                VET_FLOAT3,
                targetSemantic,
                index);
            // Now copy the original data across
            unsigned char* pSrc = static_cast<unsigned char*>(
                origBuffer->lock(HardwareBuffer::HBL_READ_ONLY));
            unsigned char* pDest = static_cast<unsigned char*>(
                newBuffer->lock(HardwareBuffer::HBL_DISCARD));
            size_t vertSize = origBuffer->getVertexSize();
            for (size_t v = 0; v < vertexData->vertexCount; ++v)
            {
                // Copy original vertex data
                memcpy(pDest, pSrc, vertSize);
                pSrc += vertSize;
                pDest += vertSize;
                // Set the new part to 0 since we'll accumulate in this
                memset(pDest, 0, sizeof(float)*3);
                pDest += sizeof(float)*3;
            }
            origBuffer->unlock();
            newBuffer->unlock();

            // Rebind the new buffer
            vBind->setBinding(prevTexCoordElem->getSource(), newBuffer);
	    }
    }
    //---------------------------------------------------------------------
    void Mesh::buildTangentVectors(VertexElementSemantic targetSemantic, 
		unsigned short sourceTexCoordSet, unsigned short index, 
		bool splitMirrored, bool splitRotated, bool storeParityInW)
    {

		TangentSpaceCalc tangentsCalc;
		tangentsCalc.setSplitMirrored(splitMirrored);
		tangentsCalc.setSplitRotated(splitRotated);
		tangentsCalc.setStoreParityInW(storeParityInW);

		// shared geometry first
		if (sharedVertexData)
		{
			tangentsCalc.setVertexData(sharedVertexData);
			bool found = false;
			for (SubMeshList::iterator i = mSubMeshList.begin(); i != mSubMeshList.end(); ++i)
			{
				SubMesh* sm = *i;
				if (sm->useSharedVertices)
				{
					tangentsCalc.addIndexData(sm->indexData);
					found = true;
				}
			}
			if (found)
			{
				TangentSpaceCalc::Result res = 
					tangentsCalc.build(targetSemantic, sourceTexCoordSet, index);

				// If any vertex splitting happened, we have to give them bone assignments
				if (getSkeletonName() != StringUtil::BLANK)
				{
					for (TangentSpaceCalc::IndexRemapList::iterator r = res.indexesRemapped.begin(); 
						r != res.indexesRemapped.end(); ++r)
					{
						TangentSpaceCalc::IndexRemap& remap = *r;
						// Copy all bone assignments from the split vertex
						VertexBoneAssignmentList::iterator vbstart = mBoneAssignments.lower_bound(remap.splitVertex.first);
						VertexBoneAssignmentList::iterator vbend = mBoneAssignments.upper_bound(remap.splitVertex.first);
						for (VertexBoneAssignmentList::iterator vba = vbstart; vba != vbend; ++vba)
						{
							VertexBoneAssignment newAsgn = vba->second;
							newAsgn.vertexIndex = static_cast<unsigned int>(remap.splitVertex.second);
							// multimap insert doesn't invalidate iterators
							addBoneAssignment(newAsgn);
						}
						
					}
				}

				// Update poses (some vertices might have been duplicated)
				// we will just check which vertices have been split and copy
				// the offset for the original vertex to the corresponding new vertex
				PoseIterator pose_it = getPoseIterator();

				while( pose_it.hasMoreElements() )
				{
					Pose* current_pose = pose_it.getNext();
					const Pose::VertexOffsetMap& offset_map = current_pose->getVertexOffsets();

					for( TangentSpaceCalc::VertexSplits::iterator it = res.vertexSplits.begin();
						it != res.vertexSplits.end(); ++it )
					{
						TangentSpaceCalc::VertexSplit& split = *it;

						Pose::VertexOffsetMap::const_iterator found_offset = offset_map.find( split.first );

						// copy the offset
						if( found_offset != offset_map.end() )
						{
							current_pose->addVertex( split.second, found_offset->second );
						}
					}
				}
			}
		}

		// Dedicated geometry
		for (SubMeshList::iterator i = mSubMeshList.begin(); i != mSubMeshList.end(); ++i)
		{
			SubMesh* sm = *i;
			if (!sm->useSharedVertices)
			{
				tangentsCalc.clear();
				tangentsCalc.setVertexData(sm->vertexData);
                tangentsCalc.addIndexData(sm->indexData, sm->operationType);
				TangentSpaceCalc::Result res = 
					tangentsCalc.build(targetSemantic, sourceTexCoordSet, index);

				// If any vertex splitting happened, we have to give them bone assignments
				if (getSkeletonName() != StringUtil::BLANK)
				{
					for (TangentSpaceCalc::IndexRemapList::iterator r = res.indexesRemapped.begin(); 
						r != res.indexesRemapped.end(); ++r)
					{
						TangentSpaceCalc::IndexRemap& remap = *r;
						// Copy all bone assignments from the split vertex
						VertexBoneAssignmentList::const_iterator vbstart = 
							sm->getBoneAssignments().lower_bound(remap.splitVertex.first);
						VertexBoneAssignmentList::const_iterator vbend = 
							sm->getBoneAssignments().upper_bound(remap.splitVertex.first);
						for (VertexBoneAssignmentList::const_iterator vba = vbstart; vba != vbend; ++vba)
						{
							VertexBoneAssignment newAsgn = vba->second;
							newAsgn.vertexIndex = static_cast<unsigned int>(remap.splitVertex.second);
							// multimap insert doesn't invalidate iterators
							sm->addBoneAssignment(newAsgn);
						}

					}

				}
			}
		}

    }
    //---------------------------------------------------------------------
    bool Mesh::suggestTangentVectorBuildParams(VertexElementSemantic targetSemantic,
		unsigned short& outSourceCoordSet, unsigned short& outIndex)
    {
        // Go through all the vertex data and locate source and dest (must agree)
        bool sharedGeometryDone = false;
        bool foundExisting = false;
        bool firstOne = true;
        SubMeshList::iterator i, iend;
        iend = mSubMeshList.end();
        for (i = mSubMeshList.begin(); i != iend; ++i)
        {
            SubMesh* sm = *i;
            VertexData* vertexData;

            if (sm->useSharedVertices)
            {
                if (sharedGeometryDone)
                    continue;
                vertexData = sharedVertexData;
                sharedGeometryDone = true;
            }
            else
            {
                vertexData = sm->vertexData;
            }

            const VertexElement *sourceElem = 0;
            unsigned short targetIndex = 0;
            for (targetIndex = 0; targetIndex < OGRE_MAX_TEXTURE_COORD_SETS; ++targetIndex)
            {
                const VertexElement* testElem =
                    vertexData->vertexDeclaration->findElementBySemantic(
                        VES_TEXTURE_COORDINATES, targetIndex);
                if (!testElem)
                    break; // finish if we've run out, t will be the target

                if (!sourceElem)
                {
                    // We're still looking for the source texture coords
                    if (testElem->getType() == VET_FLOAT2)
                    {
                        // Ok, we found it
                        sourceElem = testElem;
                    }
                }
                
				if(!foundExisting && targetSemantic == VES_TEXTURE_COORDINATES)
                {
                    // We're looking for the destination
                    // Check to see if we've found a possible
                    if (testElem->getType() == VET_FLOAT3)
                    {
                        // This is a 3D set, might be tangents
                        foundExisting = true;
                    }

                }

            }

			if (!foundExisting && targetSemantic != VES_TEXTURE_COORDINATES)
			{
				targetIndex = 0;
				// Look for existing semantic
				const VertexElement* testElem =
					vertexData->vertexDeclaration->findElementBySemantic(
					targetSemantic, targetIndex);
				if (testElem)
				{
					foundExisting = true;
				}

			}

            // After iterating, we should have a source and a possible destination (t)
            if (!sourceElem)
            {
                OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
                    "Cannot locate an appropriate 2D texture coordinate set for "
                    "all the vertex data in this mesh to create tangents from. ",
                    "Mesh::suggestTangentVectorBuildParams");
            }
            // Check that we agree with previous decisions, if this is not the
            // first one, and if we're not just using the existing one
            if (!firstOne && !foundExisting)
            {
                if (sourceElem->getIndex() != outSourceCoordSet)
                {
                    OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        "Multiple sets of vertex data in this mesh disagree on "
                        "the appropriate index to use for the source texture coordinates. "
                        "This ambiguity must be rectified before tangents can be generated.",
                        "Mesh::suggestTangentVectorBuildParams");
                }
                if (targetIndex != outIndex)
                {
                    OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        "Multiple sets of vertex data in this mesh disagree on "
                        "the appropriate index to use for the target texture coordinates. "
                        "This ambiguity must be rectified before tangents can be generated.",
                        "Mesh::suggestTangentVectorBuildParams");
                }
            }

            // Otherwise, save this result
            outSourceCoordSet = sourceElem->getIndex();
            outIndex = targetIndex;

            firstOne = false;

       }

        return foundExisting;

    }
    //---------------------------------------------------------------------
    void Mesh::buildEdgeList(void)
    {
        if (mEdgeListsBuilt)
            return;
#if !OGRE_NO_MESHLOD
        // Loop over LODs
        for (unsigned short lodIndex = 0; lodIndex < (unsigned short)mMeshLodUsageList.size(); ++lodIndex)
        {
            // use getLodLevel to enforce loading of manual mesh lods
            MeshLodUsage& usage = const_cast<MeshLodUsage&>(getLodLevel(lodIndex));

            if (!usage.manualName.empty() && lodIndex != 0)
            {
                // Delegate edge building to manual mesh
                // It should have already built it's own edge list while loading
				if (!usage.manualMesh.isNull())
				{
					usage.edgeData = usage.manualMesh->getEdgeList(0);
				}
            }
            else
            {
                // Build
                EdgeListBuilder eb;
                size_t vertexSetCount = 0;
                bool atLeastOneIndexSet = false;

                if (sharedVertexData)
                {
                    eb.addVertexData(sharedVertexData);
                    vertexSetCount++;
                }

                // Prepare the builder using the submesh information
                SubMeshList::iterator i, iend;
                iend = mSubMeshList.end();
                for (i = mSubMeshList.begin(); i != iend; ++i)
                {
                    SubMesh* s = *i;
					if (s->operationType != RenderOperation::OT_TRIANGLE_FAN && 
						s->operationType != RenderOperation::OT_TRIANGLE_LIST && 
						s->operationType != RenderOperation::OT_TRIANGLE_STRIP)
					{
                        continue;
					}
                    if (s->useSharedVertices)
                    {
                        // Use shared vertex data, index as set 0
                        if (lodIndex == 0)
                        {
                            eb.addIndexData(s->indexData, 0, s->operationType);
                        }
                        else
                        {
                            eb.addIndexData(s->mLodFaceList[lodIndex-1], 0,
                                s->operationType);
                        }
                    }
                    else if(s->isBuildEdgesEnabled())
                    {
                        // own vertex data, add it and reference it directly
                        eb.addVertexData(s->vertexData);
                        if (lodIndex == 0)
                        {
                            // Base index data
                            eb.addIndexData(s->indexData, vertexSetCount++,
                                s->operationType);
                        }
                        else
                        {
                            // LOD index data
                            eb.addIndexData(s->mLodFaceList[lodIndex-1],
                                vertexSetCount++, s->operationType);
                        }

                    }
					atLeastOneIndexSet = true;
                }

                if (atLeastOneIndexSet)
				{
					usage.edgeData = eb.build();

                #if OGRE_DEBUG_MODE
                    // Override default log
                    Log* log = LogManager::getSingleton().createLog(
                        mName + "_lod" + StringConverter::toString(lodIndex) +
                        "_prepshadow.log", false, false);
                    usage.edgeData->log(log);
					// clean up log & close file handle
					LogManager::getSingleton().destroyLog(log);
                #endif
				}
				else
				{
					// create empty edge data
					usage.edgeData = OGRE_NEW EdgeData();
				}
            }
        }
#else
		// Build
		EdgeListBuilder eb;
		size_t vertexSetCount = 0;
		if (sharedVertexData)
		{
			eb.addVertexData(sharedVertexData);
			vertexSetCount++;
		}

		// Prepare the builder using the submesh information
		SubMeshList::iterator i, iend;
		iend = mSubMeshList.end();
		for (i = mSubMeshList.begin(); i != iend; ++i)
		{
			SubMesh* s = *i;
			if (s->operationType != RenderOperation::OT_TRIANGLE_FAN && 
				s->operationType != RenderOperation::OT_TRIANGLE_LIST && 
				s->operationType != RenderOperation::OT_TRIANGLE_STRIP)
			{
				continue;
			}
			if (s->useSharedVertices)
			{
				eb.addIndexData(s->indexData, 0, s->operationType);
			}
			else if(s->isBuildEdgesEnabled())
			{
				// own vertex data, add it and reference it directly
				eb.addVertexData(s->vertexData);
				// Base index data
				eb.addIndexData(s->indexData, vertexSetCount++,
					s->operationType);
			}
		}

			mMeshLodUsageList[0].edgeData = eb.build();

#if OGRE_DEBUG_MODE
			// Override default log
			Log* log = LogManager::getSingleton().createLog(
				mName + "_lod0"+
				"_prepshadow.log", false, false);
			mMeshLodUsageList[0].edgeData->log(log);
			// clean up log & close file handle
			LogManager::getSingleton().destroyLog(log);
#endif
#endif
        mEdgeListsBuilt = true;
    }
    //---------------------------------------------------------------------
    void Mesh::freeEdgeList(void)
    {
        if (!mEdgeListsBuilt)
            return;
#if !OGRE_NO_MESHLOD
        // Loop over LODs
        MeshLodUsageList::iterator i, iend;
        iend = mMeshLodUsageList.end();
        unsigned short index = 0;
        for (i = mMeshLodUsageList.begin(); i != iend; ++i, ++index)
        {
            MeshLodUsage& usage = *i;

            if (usage.manualName.empty() || index == 0)
            {
                // Only delete if we own this data
                // Manual LODs > 0 own their own
                OGRE_DELETE usage.edgeData;
            }
            usage.edgeData = NULL;
        }
#else
		OGRE_DELETE mMeshLodUsageList[0].edgeData;
		mMeshLodUsageList[0].edgeData = NULL;
#endif
        mEdgeListsBuilt = false;
    }
    //---------------------------------------------------------------------
    void Mesh::prepareForShadowVolume(void)
    {
        if (mPreparedForShadowVolumes)
            return;

        if (sharedVertexData)
        {
            sharedVertexData->prepareForShadowVolume();
        }
        SubMeshList::iterator i, iend;
        iend = mSubMeshList.end();
        for (i = mSubMeshList.begin(); i != iend; ++i)
        {
            SubMesh* s = *i;
            if (!s->useSharedVertices && 
				(s->operationType == RenderOperation::OT_TRIANGLE_FAN || 
				s->operationType == RenderOperation::OT_TRIANGLE_LIST ||
				s->operationType == RenderOperation::OT_TRIANGLE_STRIP))
            {
                s->vertexData->prepareForShadowVolume();
            }
        }
        mPreparedForShadowVolumes = true;
    }
    //---------------------------------------------------------------------
    EdgeData* Mesh::getEdgeList(unsigned short lodIndex)
    {
        // Build edge list on demand
        if (!mEdgeListsBuilt && mAutoBuildEdgeLists)
        {
            buildEdgeList();
        }
#if !OGRE_NO_MESHLOD
		return getLodLevel(lodIndex).edgeData;
#else
		assert(lodIndex == 0);
		return mMeshLodUsageList[0].edgeData;
#endif
    }
    //---------------------------------------------------------------------
    const EdgeData* Mesh::getEdgeList(unsigned short lodIndex) const
    {
#if !OGRE_NO_MESHLOD
		return getLodLevel(lodIndex).edgeData;
#else
		assert(lodIndex == 0);
		return mMeshLodUsageList[0].edgeData;
#endif
    }
    //---------------------------------------------------------------------
    void Mesh::prepareMatricesForVertexBlend(const Matrix4** blendMatrices,
        const Matrix4* boneMatrices, const IndexMap& indexMap)
    {
        assert(indexMap.size() <= 256);
        IndexMap::const_iterator it, itend;
        itend = indexMap.end();
        for (it = indexMap.begin(); it != itend; ++it)
        {
            *blendMatrices++ = boneMatrices + *it;
        }
    }
    //---------------------------------------------------------------------
    void Mesh::softwareVertexBlend(const VertexData* sourceVertexData,
        const VertexData* targetVertexData,
        const Matrix4* const* blendMatrices, size_t numMatrices,
        bool blendNormals)
    {
        float *pSrcPos = 0;
        float *pSrcNorm = 0;
        float *pDestPos = 0;
        float *pDestNorm = 0;
        float *pBlendWeight = 0;
        unsigned char* pBlendIdx = 0;
        size_t srcPosStride = 0;
        size_t srcNormStride = 0;
        size_t destPosStride = 0;
        size_t destNormStride = 0;
        size_t blendWeightStride = 0;
        size_t blendIdxStride = 0;


        // Get elements for source
        const VertexElement* srcElemPos =
            sourceVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION);
        const VertexElement* srcElemNorm =
            sourceVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL);
        const VertexElement* srcElemBlendIndices =
            sourceVertexData->vertexDeclaration->findElementBySemantic(VES_BLEND_INDICES);
        const VertexElement* srcElemBlendWeights =
            sourceVertexData->vertexDeclaration->findElementBySemantic(VES_BLEND_WEIGHTS);
        assert (srcElemPos && srcElemBlendIndices && srcElemBlendWeights &&
            "You must supply at least positions, blend indices and blend weights");
        // Get elements for target
        const VertexElement* destElemPos =
            targetVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION);
        const VertexElement* destElemNorm =
            targetVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL);

        // Do we have normals and want to blend them?
        bool includeNormals = blendNormals && (srcElemNorm != NULL) && (destElemNorm != NULL);


        // Get buffers for source
        HardwareVertexBufferSharedPtr srcPosBuf = sourceVertexData->vertexBufferBinding->getBuffer(srcElemPos->getSource());
		HardwareVertexBufferSharedPtr srcIdxBuf = sourceVertexData->vertexBufferBinding->getBuffer(srcElemBlendIndices->getSource());
		HardwareVertexBufferSharedPtr srcWeightBuf = sourceVertexData->vertexBufferBinding->getBuffer(srcElemBlendWeights->getSource());
		HardwareVertexBufferSharedPtr srcNormBuf;

        srcPosStride = srcPosBuf->getVertexSize();
        
        blendIdxStride = srcIdxBuf->getVertexSize();
        
        blendWeightStride = srcWeightBuf->getVertexSize();
        if (includeNormals)
        {
            srcNormBuf = sourceVertexData->vertexBufferBinding->getBuffer(srcElemNorm->getSource());
            srcNormStride = srcNormBuf->getVertexSize();
        }
        // Get buffers for target
        HardwareVertexBufferSharedPtr destPosBuf = targetVertexData->vertexBufferBinding->getBuffer(destElemPos->getSource());
		HardwareVertexBufferSharedPtr destNormBuf;
        destPosStride = destPosBuf->getVertexSize();
        if (includeNormals)
        {
            destNormBuf = targetVertexData->vertexBufferBinding->getBuffer(destElemNorm->getSource());
            destNormStride = destNormBuf->getVertexSize();
        }

        void* pBuffer;

        // Lock source buffers for reading
        pBuffer = srcPosBuf->lock(HardwareBuffer::HBL_READ_ONLY);
        srcElemPos->baseVertexPointerToElement(pBuffer, &pSrcPos);
        if (includeNormals)
        {
            if (srcNormBuf != srcPosBuf)
            {
                // Different buffer
                pBuffer = srcNormBuf->lock(HardwareBuffer::HBL_READ_ONLY);
            }
            srcElemNorm->baseVertexPointerToElement(pBuffer, &pSrcNorm);
        }

        // Indices must be 4 bytes
        assert(srcElemBlendIndices->getType() == VET_UBYTE4 &&
               "Blend indices must be VET_UBYTE4");
        pBuffer = srcIdxBuf->lock(HardwareBuffer::HBL_READ_ONLY);
        srcElemBlendIndices->baseVertexPointerToElement(pBuffer, &pBlendIdx);
        if (srcWeightBuf != srcIdxBuf)
        {
            // Lock buffer
            pBuffer = srcWeightBuf->lock(HardwareBuffer::HBL_READ_ONLY);
        }
        srcElemBlendWeights->baseVertexPointerToElement(pBuffer, &pBlendWeight);
        unsigned short numWeightsPerVertex =
            VertexElement::getTypeCount(srcElemBlendWeights->getType());


        // Lock destination buffers for writing
        pBuffer = destPosBuf->lock(
            (destNormBuf != destPosBuf && destPosBuf->getVertexSize() == destElemPos->getSize()) ||
            (destNormBuf == destPosBuf && destPosBuf->getVertexSize() == destElemPos->getSize() + destElemNorm->getSize()) ?
            HardwareBuffer::HBL_DISCARD : HardwareBuffer::HBL_NORMAL);
        destElemPos->baseVertexPointerToElement(pBuffer, &pDestPos);
        if (includeNormals)
        {
            if (destNormBuf != destPosBuf)
            {
                pBuffer = destNormBuf->lock(
                    destNormBuf->getVertexSize() == destElemNorm->getSize() ?
                    HardwareBuffer::HBL_DISCARD : HardwareBuffer::HBL_NORMAL);
            }
            destElemNorm->baseVertexPointerToElement(pBuffer, &pDestNorm);
        }

        OptimisedUtil::getImplementation()->softwareVertexSkinning(
            pSrcPos, pDestPos,
            pSrcNorm, pDestNorm,
            pBlendWeight, pBlendIdx,
            blendMatrices,
            srcPosStride, destPosStride,
            srcNormStride, destNormStride,
            blendWeightStride, blendIdxStride,
            numWeightsPerVertex,
            targetVertexData->vertexCount);

        // Unlock source buffers
        srcPosBuf->unlock();
        srcIdxBuf->unlock();
        if (srcWeightBuf != srcIdxBuf)
        {
            srcWeightBuf->unlock();
        }
        if (includeNormals && srcNormBuf != srcPosBuf)
        {
            srcNormBuf->unlock();
        }
        // Unlock destination buffers
        destPosBuf->unlock();
        if (includeNormals && destNormBuf != destPosBuf)
        {
            destNormBuf->unlock();
        }

    }
	//---------------------------------------------------------------------
	void Mesh::softwareVertexMorph(Real t,
		const HardwareVertexBufferSharedPtr& b1,
		const HardwareVertexBufferSharedPtr& b2,
		VertexData* targetVertexData)
	{
		float* pb1 = static_cast<float*>(b1->lock(HardwareBuffer::HBL_READ_ONLY));
		float* pb2;
		if (b1.get() != b2.get())
		{
			pb2 = static_cast<float*>(b2->lock(HardwareBuffer::HBL_READ_ONLY));
		}
		else
		{
			// Same buffer - track with only one entry or time index exactly matching
			// one keyframe
			// For simplicity of main code, interpolate still but with same val
			pb2 = pb1;
		}

		const VertexElement* posElem =
			targetVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION);
		assert(posElem);
		const VertexElement* normElem =
			targetVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL);
		
		bool morphNormals = false;
		if (normElem && normElem->getSource() == posElem->getSource() &&
			b1->getVertexSize() == 24 && b2->getVertexSize() == 24)
			morphNormals = true;
		
		HardwareVertexBufferSharedPtr destBuf =
			targetVertexData->vertexBufferBinding->getBuffer(
				posElem->getSource());
		assert((posElem->getSize() == destBuf->getVertexSize()
				|| (morphNormals && posElem->getSize() + normElem->getSize() == destBuf->getVertexSize())) &&
			"Positions (or positions & normals) must be in a buffer on their own for morphing");
		float* pdst = static_cast<float*>(
			destBuf->lock(HardwareBuffer::HBL_DISCARD));

        OptimisedUtil::getImplementation()->softwareVertexMorph(
            t, pb1, pb2, pdst,
			b1->getVertexSize(), b2->getVertexSize(), destBuf->getVertexSize(),
            targetVertexData->vertexCount,
			morphNormals);

		destBuf->unlock();
		b1->unlock();
		if (b1.get() != b2.get())
			b2->unlock();
	}
	//---------------------------------------------------------------------
	void Mesh::softwareVertexPoseBlend(Real weight,
		const map<size_t, Vector3>::type& vertexOffsetMap,
		const map<size_t, Vector3>::type& normalsMap,
		VertexData* targetVertexData)
	{
		// Do nothing if no weight
		if (weight == 0.0f)
			return;

		const VertexElement* posElem =
			targetVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION);
		const VertexElement* normElem =
			targetVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL);
		assert(posElem);
		// Support normals if they're in the same buffer as positions and pose includes them
		bool normals = normElem && !normalsMap.empty() && posElem->getSource() == normElem->getSource();
		HardwareVertexBufferSharedPtr destBuf =
			targetVertexData->vertexBufferBinding->getBuffer(
			posElem->getSource());

		size_t elemsPerVertex = destBuf->getVertexSize()/sizeof(float);

		// Have to lock in normal mode since this is incremental
		float* pBase = static_cast<float*>(
			destBuf->lock(HardwareBuffer::HBL_NORMAL));
				
		// Iterate over affected vertices
		for (map<size_t, Vector3>::type::const_iterator i = vertexOffsetMap.begin();
			i != vertexOffsetMap.end(); ++i)
		{
			// Adjust pointer
			float *pdst = pBase + i->first*elemsPerVertex;

			*pdst = *pdst + (i->second.x * weight);
			++pdst;
			*pdst = *pdst + (i->second.y * weight);
			++pdst;
			*pdst = *pdst + (i->second.z * weight);
			++pdst;
			
		}
		
		if (normals)
		{
			float* pNormBase;
			normElem->baseVertexPointerToElement((void*)pBase, &pNormBase);
			for (map<size_t, Vector3>::type::const_iterator i = normalsMap.begin();
				i != normalsMap.end(); ++i)
			{
				// Adjust pointer
				float *pdst = pNormBase + i->first*elemsPerVertex;

				*pdst = *pdst + (i->second.x * weight);
				++pdst;
				*pdst = *pdst + (i->second.y * weight);
				++pdst;
				*pdst = *pdst + (i->second.z * weight);
				++pdst;				
				
			}
		}
		destBuf->unlock();
	}
    //---------------------------------------------------------------------
	size_t Mesh::calculateSize(void) const
	{
		// calculate GPU size
		size_t ret = 0;
		unsigned short i;
		// Shared vertices
		if (sharedVertexData)
		{
			for (i = 0;
				i < sharedVertexData->vertexBufferBinding->getBufferCount();
				++i)
			{
				ret += sharedVertexData->vertexBufferBinding
					->getBuffer(i)->getSizeInBytes();
			}
		}

		SubMeshList::const_iterator si;
		for (si = mSubMeshList.begin(); si != mSubMeshList.end(); ++si)
		{
			// Dedicated vertices
			if (!(*si)->useSharedVertices)
			{
				for (i = 0;
					i < (*si)->vertexData->vertexBufferBinding->getBufferCount();
					++i)
				{
					ret += (*si)->vertexData->vertexBufferBinding
						->getBuffer(i)->getSizeInBytes();
				}
			}
			if (!(*si)->indexData->indexBuffer.isNull())
			{
				// Index data
				ret += (*si)->indexData->indexBuffer->getSizeInBytes();
			}

		}
		return ret;
	}
	//-----------------------------------------------------------------------------
	bool Mesh::hasVertexAnimation(void) const
	{
		return !mAnimationsList.empty();
	}
	//---------------------------------------------------------------------
	VertexAnimationType Mesh::getSharedVertexDataAnimationType(void) const
	{
		if (mAnimationTypesDirty)
		{
			_determineAnimationTypes();
		}

		return mSharedVertexDataAnimationType;
	}
	//---------------------------------------------------------------------
	void Mesh::_determineAnimationTypes(void) const
	{
		// Don't check flag here; since detail checks on track changes are not
		// done, allow caller to force if they need to

		// Initialise all types to nothing
		mSharedVertexDataAnimationType = VAT_NONE;
		mSharedVertexDataAnimationIncludesNormals = false;
		for (SubMeshList::const_iterator i = mSubMeshList.begin();
			i != mSubMeshList.end(); ++i)
		{
			(*i)->mVertexAnimationType = VAT_NONE;
			(*i)->mVertexAnimationIncludesNormals = false;
		}
		
		mPosesIncludeNormals = false;
		for (PoseList::const_iterator i = mPoseList.begin(); i != mPoseList.end(); ++i)
		{
			if (i == mPoseList.begin())
				mPosesIncludeNormals = (*i)->getIncludesNormals();
			else if (mPosesIncludeNormals != (*i)->getIncludesNormals())
				// only support normals if consistently included
				mPosesIncludeNormals = mPosesIncludeNormals && (*i)->getIncludesNormals();
		}

		// Scan all animations and determine the type of animation tracks
		// relating to each vertex data
		for(AnimationList::const_iterator ai = mAnimationsList.begin();
			ai != mAnimationsList.end(); ++ai)
		{
			Animation* anim = ai->second;
			Animation::VertexTrackIterator vit = anim->getVertexTrackIterator();
			while (vit.hasMoreElements())
			{
				VertexAnimationTrack* track = vit.getNext();
				ushort handle = track->getHandle();
				if (handle == 0)
				{
					// shared data
					if (mSharedVertexDataAnimationType != VAT_NONE &&
						mSharedVertexDataAnimationType != track->getAnimationType())
					{
						// Mixing of morph and pose animation on same data is not allowed
						OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
							"Animation tracks for shared vertex data on mesh "
							+ mName + " try to mix vertex animation types, which is "
							"not allowed.",
							"Mesh::_determineAnimationTypes");
					}
					mSharedVertexDataAnimationType = track->getAnimationType();
					if (track->getAnimationType() == VAT_MORPH)
						mSharedVertexDataAnimationIncludesNormals = track->getVertexAnimationIncludesNormals();
					else 
						mSharedVertexDataAnimationIncludesNormals = mPosesIncludeNormals;

				}
				else
				{
					// submesh index (-1)
					SubMesh* sm = getSubMesh(handle-1);
					if (sm->mVertexAnimationType != VAT_NONE &&
						sm->mVertexAnimationType != track->getAnimationType())
					{
						// Mixing of morph and pose animation on same data is not allowed
						OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
							"Animation tracks for dedicated vertex data "
							+ StringConverter::toString(handle-1) + " on mesh "
							+ mName + " try to mix vertex animation types, which is "
							"not allowed.",
							"Mesh::_determineAnimationTypes");
					}
					sm->mVertexAnimationType = track->getAnimationType();
					if (track->getAnimationType() == VAT_MORPH)
						sm->mVertexAnimationIncludesNormals = track->getVertexAnimationIncludesNormals();
					else 
						sm->mVertexAnimationIncludesNormals = mPosesIncludeNormals;

				}
			}
		}

		mAnimationTypesDirty = false;
	}
	//---------------------------------------------------------------------
	Animation* Mesh::createAnimation(const String& name, Real length)
	{
		// Check name not used
		if (mAnimationsList.find(name) != mAnimationsList.end())
		{
			OGRE_EXCEPT(
				Exception::ERR_DUPLICATE_ITEM,
				"An animation with the name " + name + " already exists",
				"Mesh::createAnimation");
		}

		Animation* ret = OGRE_NEW Animation(name, length);
		ret->_notifyContainer(this);

		// Add to list
		mAnimationsList[name] = ret;

		// Mark animation types dirty
		mAnimationTypesDirty = true;

		return ret;

	}
	//---------------------------------------------------------------------
	Animation* Mesh::getAnimation(const String& name) const
	{
		Animation* ret = _getAnimationImpl(name);
		if (!ret)
		{
			OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
				"No animation entry found named " + name,
				"Mesh::getAnimation");
		}

		return ret;
	}
	//---------------------------------------------------------------------
	Animation* Mesh::getAnimation(unsigned short index) const
	{
		// If you hit this assert, then the index is out of bounds.
		assert( index < mAnimationsList.size() );

		AnimationList::const_iterator i = mAnimationsList.begin();

		std::advance(i, index);

		return i->second;

	}
	//---------------------------------------------------------------------
	unsigned short Mesh::getNumAnimations(void) const
	{
		return static_cast<unsigned short>(mAnimationsList.size());
	}
	//---------------------------------------------------------------------
	bool Mesh::hasAnimation(const String& name) const
	{
		return _getAnimationImpl(name) != 0;
	}
	//---------------------------------------------------------------------
	Animation* Mesh::_getAnimationImpl(const String& name) const
	{
		Animation* ret = 0;
		AnimationList::const_iterator i = mAnimationsList.find(name);

		if (i != mAnimationsList.end())
		{
			ret = i->second;
		}

		return ret;

	}
	//---------------------------------------------------------------------
	void Mesh::removeAnimation(const String& name)
	{
		AnimationList::iterator i = mAnimationsList.find(name);

		if (i == mAnimationsList.end())
		{
			OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "No animation entry found named " + name,
				"Mesh::getAnimation");
		}

		OGRE_DELETE i->second;

		mAnimationsList.erase(i);

		mAnimationTypesDirty = true;
	}
	//---------------------------------------------------------------------
	void Mesh::removeAllAnimations(void)
	{
		AnimationList::iterator i = mAnimationsList.begin();
		for (; i != mAnimationsList.end(); ++i)
		{
			OGRE_DELETE i->second;
		}
		mAnimationsList.clear();
		mAnimationTypesDirty = true;
	}
	//---------------------------------------------------------------------
	VertexData* Mesh::getVertexDataByTrackHandle(unsigned short handle)
	{
		if (handle == 0)
		{
			return sharedVertexData;
		}
		else
		{
			return getSubMesh(handle-1)->vertexData;
		}
	}
	//---------------------------------------------------------------------
	Pose* Mesh::createPose(ushort target, const String& name)
	{
		Pose* retPose = OGRE_NEW Pose(target, name);
		mPoseList.push_back(retPose);
		return retPose;
	}
	//---------------------------------------------------------------------
	Pose* Mesh::getPose(ushort index)
	{
		if (index >= getPoseCount())
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
				"Index out of bounds",
				"Mesh::getPose");
		}

		return mPoseList[index];

	}
	//---------------------------------------------------------------------
	Pose* Mesh::getPose(const String& name)
	{
		for (PoseList::iterator i = mPoseList.begin(); i != mPoseList.end(); ++i)
		{
			if ((*i)->getName() == name)
				return *i;
		}
		StringUtil::StrStreamType str;
		str << "No pose called " << name << " found in Mesh " << mName;
		OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
			str.str(),
			"Mesh::getPose");

	}
	//---------------------------------------------------------------------
	void Mesh::removePose(ushort index)
	{
		if (index >= getPoseCount())
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
				"Index out of bounds",
				"Mesh::removePose");
		}
		PoseList::iterator i = mPoseList.begin();
		std::advance(i, index);
		OGRE_DELETE *i;
		mPoseList.erase(i);

	}
	//---------------------------------------------------------------------
	void Mesh::removePose(const String& name)
	{
		for (PoseList::iterator i = mPoseList.begin(); i != mPoseList.end(); ++i)
		{
			if ((*i)->getName() == name)
			{
				OGRE_DELETE *i;
				mPoseList.erase(i);
				return;
			}
		}
		StringUtil::StrStreamType str;
		str << "No pose called " << name << " found in Mesh " << mName;
		OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
			str.str(),
			"Mesh::removePose");
	}
	//---------------------------------------------------------------------
	void Mesh::removeAllPoses(void)
	{
		for (PoseList::iterator i = mPoseList.begin(); i != mPoseList.end(); ++i)
		{
			OGRE_DELETE *i;
		}
		mPoseList.clear();
	}
	//---------------------------------------------------------------------
	Mesh::PoseIterator Mesh::getPoseIterator(void)
	{
		return PoseIterator(mPoseList.begin(), mPoseList.end());
	}
	//---------------------------------------------------------------------
	Mesh::ConstPoseIterator Mesh::getPoseIterator(void) const
	{
		return ConstPoseIterator(mPoseList.begin(), mPoseList.end());
	}
	//-----------------------------------------------------------------------------
	const PoseList& Mesh::getPoseList(void) const
	{
		return mPoseList;
	}
	//---------------------------------------------------------------------
	void Mesh::updateMaterialForAllSubMeshes(void)
	{
        // iterate through each sub mesh and request the submesh to update its material
        vector<SubMesh*>::type::iterator subi;
        for (subi = mSubMeshList.begin(); subi != mSubMeshList.end(); ++subi)
        {
            (*subi)->updateMaterialUsingTextureAliases();
        }

    }
	//---------------------------------------------------------------------
    const LodStrategy *Mesh::getLodStrategy() const
    {
		return mLodStrategy;
    }
#if !OGRE_NO_MESHLOD
    //---------------------------------------------------------------------
    void Mesh::setLodStrategy(LodStrategy *lodStrategy)
    {
        mLodStrategy = lodStrategy;

        assert(mMeshLodUsageList.size());
        mMeshLodUsageList[0].value = mLodStrategy->getBaseValue();

        // Re-transform user LOD values (starting at index 1, no need to transform base value)
		for (MeshLodUsageList::iterator i = mMeshLodUsageList.begin()+1; i != mMeshLodUsageList.end(); ++i)
            i->value = mLodStrategy->transformUserValue(i->userValue);

    }
#endif

}

