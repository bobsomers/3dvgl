#ifndef __SCENE_H__
#define __SCENE_H__

// Sample "pulsar" scene from Paul Bourke's website. Only minor modifications
// have been made.
// http://paulbourke.net/miscellaneous/stereographics/stereorender/

// There isn't anything special about it. The only thing you need to keep in
// mind when using your own scenes is that most people have the best 3D
// experiences when everything goes "into" the screen, which means all your
// geometry should be *futher* away than the focal length of the camera to push
// it into positive parallax. Any HUD or UI should probably be rendered
// separately at screen depth by wiping the projection stack and using a simple
// 2D projection to keep them the same in both eyes.

// For the purposes of this demo, I manually adjusted the focal length of the
// camera to greatly exaggerate the 3D effect, pushing the pulsar way out in
// front of the screen. Please recognize that this is just for the obvious
// purposes of this demo, and it practice you *should not do that*. It hurts and
// makes people cry. Please don't make people cry.

namespace PaulBourke {

    typedef struct {
       double x,y,z;
    } XYZ;

    typedef struct {
       double r,g,b;
    } COLOUR;

    const float DTOR = 0.0174532925;
    const XYZ origin = {0.0,0.0,0.0};

    void MakeGeometry(float rotateangle);
    void MakeLighting();
}

#endif // __SCENE_H__
