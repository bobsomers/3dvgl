#ifndef __SCREENSHOT_H__
#define __SCREENSHOT_H__

namespace Screenshot {

    // Sets OpenGL state such that we can take screenshots later.
    void Init();

    // Writes the framebuffer to a targa file with the specified filename.
    // Region is from (x, y) in the bottom left to (x + w, y + h) in the top
    // right.
    void Screenshot(int x, int y, int w, int h, const char *filename);

}

#endif // __SCREENSHOT_H__
