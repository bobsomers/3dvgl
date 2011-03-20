#include <math.h>
#include <GL/glut.h>

#include "scene.h"

/*
   Create the geometry for the pulsar
*/
void PaulBourke::MakeGeometry(float rotateangle)
{
   int i,j,k;
   double cradius = 5.3;         /* Final radius of the cone */
   double clength = 30;            /* Cone length */
   double sradius = 10;            /* Final radius of sphere */
   double r1,r2;                  /* Min and Max radius of field lines */
   double x,y,z;
   XYZ p[4],n[4];
   COLOUR grey = {0.7,0.7,0.7};
   COLOUR white = {1.0,1.0,1.0};
   GLfloat specular[4] = {1.0,1.0,1.0,1.0};
   GLfloat shiny[1] = {5.0};
   //char cmd[64];

   glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,specular);
   glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,shiny);

   /* Top level rotation  - spin */
   glPushMatrix();
   glRotatef(rotateangle,0.0,1.0,0.0);

   /* Axis of rotation */
   /*if (showconstruct) {
      glColor3f(white.r,white.g,white.b);
      glBegin(GL_LINES);
      glVertex3f(0.0,-60.0,0.0);
      glVertex3f(0.0,60.0,0.0);
      glEnd();
   }*/

   /* Rotation about spin axis */
   glPushMatrix();
   glRotatef(45.0,0.0,0.0,1.0);

   /* Magnetic axis */
   /*if (showconstruct) {
      glColor3f(white.r,white.g,white.b);
      glBegin(GL_LINES);
      glVertex3f(0.0,-60.0,0.0);
      glVertex3f(0.0,60.0,0.0);
      glEnd();
   }*/

   /* Light in center */
   glColor3f(white.r,white.g,white.b);
   glutSolidSphere(5.0,16,8);

   /* Spherical center */
   for (i=0;i<360;i+=5) {
      for (j=-80;j<80;j+=5) {

         p[0].x = sradius * cos(j*DTOR) * cos(i*DTOR);
         p[0].y = sradius * sin(j*DTOR);
         p[0].z = sradius * cos(j*DTOR) * sin(i*DTOR);
         n[0] = p[0];

         p[1].x = sradius * cos((j+5)*DTOR) * cos(i*DTOR);
         p[1].y = sradius * sin((j+5)*DTOR);
         p[1].z = sradius * cos((j+5)*DTOR) * sin(i*DTOR);
         n[1] = p[1];

         p[2].x = sradius * cos((j+5)*DTOR) * cos((i+5)*DTOR);
         p[2].y = sradius * sin((j+5)*DTOR);
         p[2].z = sradius * cos((j+5)*DTOR) * sin((i+5)*DTOR);
         n[2] = p[2];

         p[3].x = sradius * cos(j*DTOR) * cos((i+5)*DTOR);
         p[3].y = sradius * sin(j*DTOR);
         p[3].z = sradius * cos(j*DTOR) * sin((i+5)*DTOR);
         n[3] = p[3];

         glBegin(GL_POLYGON);
         if (i % 20 == 0)
            glColor3f(1.0,0.0,0.0);
         else
            glColor3f(0.5,0.0,0.0);
         for (k=0;k<4;k++) {
            glNormal3f(n[k].x,n[k].y,n[k].z);
            glVertex3f(p[k].x,p[k].y,p[k].z);
         }
         glEnd();
      }
   }

   /* Draw the cones */
   for (j=-1;j<=1;j+=2) {
      for (i=0;i<360;i+=10) {
         
         p[0]   = origin;
         n[0]   = p[0];
         n[0].y = -1;

         p[1].x = cradius * cos(i*DTOR);
         p[1].y = j*clength;
         p[1].z = cradius * sin(i*DTOR);
         n[1]   = p[1];
         n[1].y = 0;

         p[2].x = cradius * cos((i+10)*DTOR);
         p[2].y = j*clength;
         p[2].z = cradius * sin((i+10)*DTOR);
         n[2]   = p[2];
         n[2].y = 0;

         glBegin(GL_POLYGON);
         if (i % 30 == 0)
            glColor3f(0.0,0.2,0.0);
         else
            glColor3f(0.0,0.5,0.0);
         for (k=0;k<3;k++) {
            glNormal3f(n[k].x,n[k].y,n[k].z);
            glVertex3f(p[k].x,p[k].y,p[k].z);
         }
         glEnd();
      }
   }

   /* Draw the field lines */
   r1 = 12;
   r2 = 16;
   for (j=0;j<360;j+=20) {
      glPushMatrix();
      glRotatef((double)j,0.0,1.0,0.0);
      glBegin(GL_LINE_STRIP);
      glColor3f(grey.r,grey.g,grey.b);
      for (i=-140;i<140;i++) {
         x = r1 + r1 * cos(i*DTOR);
         y = r2 * sin(i*DTOR);
         z = 0;
         glVertex3f(x,y,z);   
      }   
      glEnd();
      glPopMatrix();      
   }

   glPopMatrix(); /* Pulsar axis rotation */
   glPopMatrix(); /* Pulsar spin */
}

/*
   Set up the lighing environment
*/
void PaulBourke::MakeLighting()
{
   GLfloat fullambient[4] = {1.0,1.0,1.0,1.0};
   GLfloat position[4] = {0.0,0.0,0.0,0.0};
   GLfloat ambient[4]  = {0.2,0.2,0.2,1.0};
   GLfloat diffuse[4]  = {1.0,1.0,1.0,1.0};
   GLfloat specular[4] = {0.0,0.0,0.0,1.0};

   /* Turn off all the lights */
   glDisable(GL_LIGHT0);
   glDisable(GL_LIGHT1);
   glDisable(GL_LIGHT2);
   glDisable(GL_LIGHT3);
   glDisable(GL_LIGHT4);
   glDisable(GL_LIGHT5);
   glDisable(GL_LIGHT6);
   glDisable(GL_LIGHT7);
   glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_TRUE);
   glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);

   /* Turn on the appropriate lights */
   glLightModelfv(GL_LIGHT_MODEL_AMBIENT,fullambient);
   glLightfv(GL_LIGHT0,GL_POSITION,position);
   glLightfv(GL_LIGHT0,GL_AMBIENT,ambient);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse);
   glLightfv(GL_LIGHT0,GL_SPECULAR,specular);
   glEnable(GL_LIGHT0);

   /* Sort out the shading algorithm */
   glShadeModel(GL_SMOOTH);

   /* Turn lighting on */
   glEnable(GL_LIGHTING);
}
