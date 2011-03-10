#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>

#include <GL/freeglut.h>

typedef struct timespec timespec;

void bomb(int err, const char *msg);
timespec nanotime_diff(timespec start, timespec end);
void init_emitter();
void cleanup_emitter();
void shutter();
void camera();
void draw_axes();
void display();
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);
int main(int argc, char *argv[]);

#endif // __MAIN_H__
