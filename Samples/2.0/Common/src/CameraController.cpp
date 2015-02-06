
#include "CameraController.h"

#include "GraphicsSystem.h"

#include "OgreCamera.h"
#include "OgreRenderWindow.h"

using namespace Demo;

namespace Demo
{
    CameraController::CameraController( GraphicsSystem *graphicsSystem, bool useSceneNode ) :
        mUseSceneNode( useSceneNode ),
        mCameraYaw( 0 ),
        mCameraPitch( 0 ),
        mGraphicsSystem( graphicsSystem )
    {
        memset( mWASD, 0, sizeof(mWASD) );
    }
    //-----------------------------------------------------------------------------------
    void CameraController::update( float timeSinceLast )
    {
        Ogre::Camera *camera = mGraphicsSystem->getCamera();

        if( mCameraYaw || mCameraPitch )
        {
            if( mUseSceneNode )
            {
                Ogre::Node *cameraNode = camera->getParentNode();
                cameraNode->yaw( Ogre::Radian( mCameraYaw ), Ogre::Node::TS_WORLD );
                cameraNode->pitch( Ogre::Radian( mCameraPitch ) );
            }
            else
            {
                camera->yaw( Ogre::Radian( mCameraYaw ) );
                camera->pitch( Ogre::Radian( mCameraPitch ) );
            }

            mCameraYaw   = 0.0f;
            mCameraPitch = 0.0f;
        }

        int camMovementZ = mWASD[2] - mWASD[0];
        int camMovementX = mWASD[3] - mWASD[1];

        if( camMovementZ || camMovementX )
        {
            Ogre::Vector3 camMovementDir( camMovementX, 0, camMovementZ );
            camMovementDir.normalise();
            camMovementDir *= timeSinceLast * 10.0f;

            if( mUseSceneNode )
            {
                Ogre::Node *cameraNode = camera->getParentNode();
                cameraNode->translate( camMovementDir, Ogre::Node::TS_LOCAL );
            }
            else
            {
                camera->moveRelative( camMovementDir );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    bool CameraController::keyPressed( const SDL_KeyboardEvent &arg )
    {
        if( arg.keysym.sym == SDLK_w )
            mWASD[0] = true;
        else if( arg.keysym.sym == SDLK_a )
            mWASD[1] = true;
        else if( arg.keysym.sym == SDLK_s )
            mWASD[2] = true;
        else if( arg.keysym.sym == SDLK_d )
            mWASD[3] = true;
        else
            return false;

        return true;
    }
    //-----------------------------------------------------------------------------------
    bool CameraController::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( arg.keysym.sym == SDLK_w )
            mWASD[0] = false;
        else if( arg.keysym.sym == SDLK_a )
            mWASD[1] = false;
        else if( arg.keysym.sym == SDLK_s )
            mWASD[2] = false;
        else if( arg.keysym.sym == SDLK_d )
            mWASD[3] = false;
        else
            return false;

        return true;
    }
    //-----------------------------------------------------------------------------------
    void CameraController::mouseMoved( const SDL_Event &arg )
    {
        float width  = mGraphicsSystem->getRenderWindow()->getWidth();
        float height = mGraphicsSystem->getRenderWindow()->getHeight();

        mCameraYaw   += -arg.motion.xrel / width;
        mCameraPitch += -arg.motion.yrel / height;
    }
}
