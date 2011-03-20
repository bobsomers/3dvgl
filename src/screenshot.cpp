#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <GL/glut.h>

#include "screenshot.h"

void Screenshot::Init() {
    // byte alignment
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void Screenshot::Screenshot(int x, int y, int w, int h, const char *filename) {
    // read from the front buffer
    glReadBuffer(GL_FRONT);
    
    // grab the pixel data
    uint8_t buffer[w * h * (3 * sizeof(uint8_t))];
    glReadPixels(x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    
    // open the file for writing
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("Failed to open screenshot file for writing!");
        exit(EXIT_FAILURE);
    }
    
    // write 24-bit uncompressed targa header
    // thanks to Paul Bourke (http://local.wasp.uwa.edu.au/~pbourke/dataformats/tga/)
    putc(0, fp);
    putc(0, fp);
    putc(2, fp); // type is uncompressed RGB
    putc(0, fp);
    putc(0, fp);
    putc(0, fp);
    putc(0, fp);
    putc(0, fp);
    putc(0, fp); // x origin, low byte
    putc(0, fp); // x origin, high byte
    putc(0, fp); // y origin, low byte
    putc(0, fp); // y origin, high byte
    putc(w & 0xff, fp); // width, low byte
    putc((w & 0xff00) >> 8, fp); // width, high byte
    putc(h & 0xff, fp); // height, low byte
    putc((h & 0xff00) >> 8, fp); // height, high byte
    putc(24, fp); // 24-bit color depth
    putc(0, fp);
    
    // write the image data
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            // calculate the offset into the buffer
            int offset = (i + j * w) * (3 * sizeof(uint8_t));
            
            // copy out the bytes
            uint8_t r = buffer[offset + 0];
            uint8_t g = buffer[offset + 1];
            uint8_t b = buffer[offset + 2];
            
            // write out the bytes in BGR order
            putc(b, fp);
            putc(g, fp);
            putc(r, fp);
        }
    }
    
    fclose(fp);
}
