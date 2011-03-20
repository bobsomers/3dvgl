#ifndef __STEREO_HELPER_H__
#define __STEREO_HELPER_H__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

#include "nvstusb.h"

namespace StereoHelper {

    /**
     * Simple 3-dimensional vector type.
     */
    struct Vec3 {
        Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
        Vec3(float xv, float yv, float zv) : x(xv), y(yv), z(zv) {}
        
        inline Vec3 operator +(const Vec3& rhs) const { return Vec3(x + rhs.x, y + rhs.y, z + rhs.z); }
        inline Vec3 operator -(const Vec3& rhs) const { return Vec3(x - rhs.x, y - rhs.y, z - rhs.z); }
        inline Vec3 operator *(float rhs) const { return Vec3(x * rhs, y * rhs, z * rhs); }
        inline Vec3 operator /(float rhs) const { return Vec3(x / rhs, y / rhs, z / rhs); }
        
        inline Vec3 Normalize() { return *this / sqrtf(x * x + y * y + z * z); }
        inline Vec3 Cross(const Vec3& rhs) {
            return Vec3(y * rhs.z - rhs.y * z,
                        z * rhs.x - rhs.z * x,
                        x * rhs.y - rhs.x * y);
        }
        
        float x;
        float y;
        float z;
    };

    /**
     * Specifies the different types of stereo cameras we know how to project.
     */
    enum CameraType {
        TOE_IN,
        PARALLEL_AXIS_ASYMMETRIC
    };

    /**
     * The camera from which stereo pairs are generated. Each element is as
     * follows:
     * 
     * type     The method for generating stereo pairs. The toe-in method uses
     *          uses two normal camera frusta with their eye positions separated
     *          by the interoccular distance, focused on the same look at point.
     *          It is generally less desirable than the other method, parallel
     *          axis asymmetric frusta. The parallel axis methods keeps both
     *          camera eye positions looking straight forward, but creates
     *          asymmetric fustra such that the images align at the focal
     *          distance of the lens.
     *
     * eye      The eye position of the camera (the actual left and right eye
     *          are computed to be half the interoccular distance to the left
     *          and right from this position.
     *
     * look     The look at point of the camera (or point of interest).
     *
     * up       The world up direction (usually positive y <0, 1, 0>).
     *
     * focal    The focal length of the camera, in world units. Objects that are
     *          this far away from the camera will appear with zero parallax (at
     *          screen depth). Objects closer will appear with negative parallax
     *          (out of the screen) and objects farther away will appear with
     *          positive parallax (into the screen).
     *
     * fov      The field of view of the camera. Just like OpenGL, this is
     *          vertical field of view in degrees.
     *
     * iod      The interoccular distance, or distance between the generated
     *          eyes. Generally for comfortable viewing you can set this to
     *          1/30th the focal length.
     *
     * near     Distance to the near plane of the camera. To prevent extreme
     *          negative parallax (think extraordinarily cheesy special effects
     *          that fly out of the screen towards you) you can generally set
     *          this to 1/5th the focal length.
     *
     * far      Distance to the far plane of the camera.
     */
    struct Camera {
        CameraType type;
        Vec3 eye;
        Vec3 look;
        Vec3 up;
        float focal;
        float fov;
        float iod;
        float near;
        float far;
    };

    /**
     * Ensures that the nvstusb refresh rate is in sync with what X11 thinks it
     * actually is.
     *
     * Pass in a pointer to the initialized nvstusb context.
     */
    void ConfigRefreshRate(nvstusb_context *ctx);

    /**
     * Computes the camera transform based on the camera type and the active eye
     * and places the entire matrix on the projection stack (this means that
     * your modelview stack should just be the identity matrix before any model
     * transforms.
     *
     * Pass in the camera (defined above), the aspect ratio of the window
     * (width / height), and the current eye (1 = left, 0 = right).
     * 
     * After calling this function the modelview stack will be the currently
     * selected matrix stack.
     */
    void ProjectCamera(const Camera& cam, float aspect, int eye);

// ============================================================================
//     IMPLEMENTATIONS ONLY BELOW THIS LINE
// ============================================================================

    void ConfigRefreshRate(nvstusb_context *ctx) {
        Display *display = XOpenDisplay(0);
        double display_num = DefaultScreen(display);
        XF86VidModeModeLine mode_line;
        int pixel_clk = 0;
        XF86VidModeGetModeLine(display, display_num, &pixel_clk, &mode_line);
        double frame_rate = (double) pixel_clk * 1000.0 / mode_line.htotal / mode_line.vtotal;
        printf("Detected refresh rate of %f Hz.\n", frame_rate);
        nvstusb_set_rate(ctx, frame_rate);
    }

    void ProjectCamera(const Camera& cam, float aspect, int eye) {
        // swap to the projection stack (we're putting the entire camera
        // transform on it
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        
        // compute the camera basis
        Vec3 dir = (cam.look - cam.eye).Normalize();
        Vec3 right = dir.Cross(cam.up).Normalize();
        
        // occular shift is based on which eye we're showing
        Vec3 shift;
        if (eye) { // left
            shift = right * (cam.iod / 2.0f) * -1.0f;
        } else { // right
            shift = right * (cam.iod / 2.0f);
        }
        
        // focus is the focal distance away along the camera view vector
        Vec3 focus = cam.eye + (dir * cam.focal);
        
        if (cam.type == TOE_IN) {
            // compute traditional perspective frusta
            gluPerspective(cam.fov, aspect, cam.near, cam.far);
            gluLookAt(cam.eye.x + shift.x, cam.eye.y + shift.y, cam.eye.z + shift.z,
                      focus.x, focus.y, focus.z,
                      cam.up.x, cam.up.y, cam.up.z);
        } else if (cam.type == PARALLEL_AXIS_ASYMMETRIC) {
            // compute the bounds of the asymmetric frustum
            float top = cam.near * tanf((cam.fov / 2.0f) * (M_PI / 180.0f));
            float bottom = -1.0f * top;
            float right = (eye) ? aspect * top - 0.5f * cam.iod * (cam.near / cam.focal) :
                                  aspect * top + 0.5f * cam.iod * (cam.near / cam.focal);
            float left = -1.0f * right;
            
            // create the frustum and do the projection
            glFrustum(left, right, bottom, top, cam.near, cam.far);
            
            // note that for the parallel axis camera, we shift both the camera
            // eye and the focus point (to keep the camera direction vector axis
            // parallel between the eyes)
            gluLookAt(cam.eye.x + shift.x, cam.eye.y + shift.y, cam.eye.z + shift.z,
                      focus.x + shift.x, focus.y + shift.y, focus.z + shift.z,
                      cam.up.x, cam.up.y, cam.up.z);
        } else {
            fprintf(stderr, "Unknown camera type in StereoHelper::ProjectCamera!\n");
            exit(EXIT_FAILURE);
        }
        
        // swap back to the modelview stack
        glMatrixMode(GL_MODELVIEW);
    }
    
}

#endif // __STEREO_HELPER_H__
