// Calculate and display the Mandelbrot set
// ECE4893/8893 final project, Fall 2011

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <GL/glut.h>
#include <GL/glext.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stack>
#include "complex.h"

using namespace std;

#define WINDOW_DIM 512
int nThreads=16;

// Min and max complex plane values
Complex  minC(-2.0, -1.2);
Complex  maxC( 1.0, 1.8);
int      maxIt = 2000;     // Max iterations for the set computations
int size = sizeof(int)*WINDOW_DIM*WINDOW_DIM;
int *result = (int*) malloc(size);      // stores computed result of MBSet data
Complex new_minC, new_maxC, temp_minC;  // stores new range for C
int valid=0,zoom_flag=0,mouse_flag=0;   
stack<float> arrayCR, arrayCI;          // stores history of range of C

pthread_barrier_t barrier;
pthread_mutex_t exitMutex;
pthread_cond_t exitCond;

// Define the RGB Class for coloring MBSet
class RGB
{
  public:
    RGB()
      : r(0), g(0), b(0) {} //black
    RGB(double r0, double g0, double b0)
      : r(r0), g(g0), b(b0) {}
  public:
    double r;
    double g;
    double b;
};

RGB* colors = 0; // Array of color values

void initColors()
{
  srand48(time(NULL)); //seed for different colors everytime
  colors = new RGB[maxIt + 1];
  for (int i = 0; i < maxIt; i++)
  {
    if (i < 6)
    { // white for small iterations
      colors[i] = RGB(1, 1, 1);
    }
    else
    {
      colors[i] = RGB(drand48(), drand48(), drand48());
    }
  }
  colors[maxIt] = RGB(); // black
}

void* calcMBdata(void* v)
{
	unsigned long myID=(unsigned long)v;
	int rowsPerThread=WINDOW_DIM/nThreads;
	int startRow = myID*rowsPerThread;
  Complex c;
	for (int i=startRow; i<startRow+rowsPerThread; i++)
	{
		for (int j=0; j<WINDOW_DIM; j++)
		{
			c.real=minC.real + i*(maxC.real - minC.real)/(WINDOW_DIM);
			c.imag=minC.imag + j*(maxC.imag - minC.imag)/(WINDOW_DIM);
			Complex Z=Complex(c);

			int k=0;
			while(k<maxIt && Z.Mag2()<4.0f)
			{
				Z=Z*Z+c;
				k++;
			}
			result[i+j*WINDOW_DIM]=k;
		}
	}
	pthread_barrier_wait(&barrier);
	if (!myID)		
	{
		pthread_mutex_lock(&exitMutex);
		pthread_cond_signal(&exitCond);	// thread 0 signals the condition variable
		pthread_mutex_unlock(&exitMutex);
	}
	return 0;
}

void createThreads()
{
  for (int i=0;i<nThreads;i++)
  {
  	pthread_t pt;
  	pthread_create(&pt, 0, calcMBdata, (void*)i);
  }
  pthread_cond_wait(&exitCond, &exitMutex);
}

//draw the MBSet based on the iterations calculated by calcMBdata
void drawMBSet(void)
{
  int count;
  glBegin(GL_POINTS);
  for (int i = 0; i < WINDOW_DIM; i++)
  {
    for (int j = 0; j < WINDOW_DIM; j++)
    {
    	count = i+j*WINDOW_DIM;
	    glColor3f(colors[result[count]].r,colors[result[count]].g,colors[result[count]].b);
	    glVertex2f(i,j);      
    }     
  }
  glEnd();
}

//draw the zoom box square
void selectionBox(void)
{
  mouse_flag=0;
  glLoadIdentity();
  gluOrtho2D(0, WINDOW_DIM, WINDOW_DIM, 0); 
  glColor3f(1.0, 0.0, 0.0);
  glLineWidth(3.0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(new_minC.real, new_minC.imag);
  glVertex2f(new_maxC.real, new_minC.imag);
  glVertex2f(new_maxC.real, new_maxC.imag);
  glVertex2f(new_minC.real, new_maxC.imag);
  glEnd();
}

void display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  glLoadIdentity();
  gluOrtho2D(0, WINDOW_DIM, WINDOW_DIM, 0);
  drawMBSet();
  if(mouse_flag==1)
    selectionBox();
  glutSwapBuffers();
}

void init(void)
{
  glShadeModel(GL_SMOOTH);
}

void timer(int)
{
  glutPostRedisplay();
  glutTimerFunc(1, timer, 0);
}

void reshape(int w, int h)
{ // Your OpenGL window reshape code here
  GLfloat aspect = (GLfloat) w / (GLfloat) h;
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (w <= h)
    glOrtho(-1.25, 1.25, -1.25 * aspect, 1.25 * aspect, -2.0, 2.0);
  else
    glOrtho(-1.25 * aspect, 1.25 * aspect, -1.25, 1.25, -2.0, 2.0);
  glMatrixMode(GL_MODELVIEW);
  glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{ // Your mouse click processing here
  // state == 0 means pressed, state != 0 means released
  // Note that the x and y coordinates passed in are in
  // PIXELS, with y = 0 at the top.
  zoom_flag=1;
  if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
  {
    new_minC.real = x;
    new_minC.imag = y;    
    temp_minC.imag=new_minC.imag;
    temp_minC.real=new_minC.real;
    arrayCR.push(minC.real);
    arrayCI.push(minC.imag);
    arrayCR.push(maxC.real); 
    arrayCI.push(maxC.imag);
  }
  if(button == GLUT_LEFT_BUTTON && state == GLUT_UP)
  {
    new_maxC.real = x;
    new_maxC.imag = y;
    valid = 1;
  }
  if(valid==1)
  {
    new_minC.real=temp_minC.real;
    new_minC.imag=temp_minC.imag;
    int xdel,ydel;
    xdel=new_maxC.real-new_minC.real;
    ydel=new_maxC.imag-new_minC.imag;

    if((new_minC.real<new_maxC.real)&&(new_minC.imag<new_maxC.imag))
    {   
      if(xdel<0)
        xdel=-xdel;
      if(ydel<0)
        ydel=-ydel;
      if(xdel>ydel)
        new_maxC.real=new_minC.real+(new_maxC.imag-new_minC.imag);
      if(xdel<ydel)
        new_maxC.imag=new_minC.imag+(new_maxC.real-new_minC.real);    
    }
    //up left
    else if((new_minC.real>new_maxC.real)&&(new_minC.imag>new_maxC.imag))
    { 
      if(xdel<0)
        xdel=-xdel;
      if(ydel<0)
        ydel=-ydel;
      if(xdel>ydel)
        new_maxC.real=new_minC.real+(new_maxC.imag-new_minC.imag);
      if(xdel<ydel)
        new_maxC.imag=new_minC.imag+(new_maxC.real-new_minC.real);
    } 
    //up right 
    else if((new_minC.real<new_maxC.real)&&(new_minC.imag>new_maxC.imag)) 
    { 
      if(xdel<0)
        xdel=-xdel;
      if(ydel<0)
        ydel=-ydel;
      if(xdel>ydel)
        new_maxC.real=new_minC.real-(new_maxC.imag-new_minC.imag);
      if(xdel<ydel)
        new_maxC.imag=new_minC.imag-(new_maxC.real-new_minC.real);       
    } 
    //down left 
    else if((new_minC.real>new_maxC.real)&&(new_minC.imag<new_maxC.imag)) 
    {        
      if(xdel<0)
        xdel=-xdel;
      if(ydel<0)
        ydel=-ydel;
      if(xdel>ydel)
        new_maxC.real=new_minC.real-(new_maxC.imag-new_minC.imag);
      if(xdel<ydel)
        new_maxC.imag=new_minC.imag-(new_maxC.real-new_minC.real);    
    }
    
    double x_delta=((maxC.real-minC.real)/WINDOW_DIM);
    double y_delta=((maxC.imag-minC.imag)/WINDOW_DIM);   
    double new_minCRe=minC.real +(new_minC.real*x_delta);
    double new_minCIm=minC.imag +(new_maxC.imag*y_delta);
    double new_maxCRe=minC.real +(new_maxC.real*x_delta);
    double new_maxCIm=minC.imag +(new_minC.imag*y_delta);
    minC.real = new_minCRe;
    minC.imag = new_minCIm;
    maxC.real = new_maxCRe;
    maxC.imag = new_maxCIm;

    // avoids inversion of range of C
    if(maxC.real < minC.real)
    {
      swap(maxC.real,minC.real);
    }   
    if(maxC.imag < minC.imag)
    {
      swap(maxC.imag,minC.imag);
    }      
    valid=0;
    selectionBox();
    glutSwapBuffers();
    createThreads();
    display();
  }  
}

void motion(int x , int y)
{ // Your mouse motion here, x and y coordinates are as above
  new_maxC.real = x;
  new_maxC.imag = y;
  int xdel,ydel;
  new_minC.real=temp_minC.real;
  new_minC.imag=temp_minC.imag;
  //bottom right
  xdel=new_maxC.real-new_minC.real;
  ydel=new_maxC.imag-new_minC.imag;
  if((new_minC.real<new_maxC.real)&&(new_minC.imag<new_maxC.imag))
  {   
    if(xdel<0)
      xdel=-xdel;
    if(ydel<0)
      ydel=-ydel;
    if(xdel>ydel)
      new_maxC.real=new_minC.real+(new_maxC.imag-new_minC.imag);
    if(xdel<ydel)
      new_maxC.imag=new_minC.imag+(new_maxC.real-new_minC.real);    
  }
  //top left
  else if((new_minC.real>new_maxC.real)&&(new_minC.imag>new_maxC.imag))
  { 
    if(xdel<0)
      xdel=-xdel;
    if(ydel<0)
      ydel=-ydel;
    if(xdel>ydel)
      new_maxC.real=new_minC.real+(new_maxC.imag-new_minC.imag);
    if(xdel<ydel)
      new_maxC.imag=new_minC.imag+(new_maxC.real-new_minC.real);
  } 
  //top right 
  else if((new_minC.real<new_maxC.real)&&(new_minC.imag>new_maxC.imag)) 
  { 
    if(xdel<0)
      xdel=-xdel;
    if(ydel<0)
      ydel=-ydel;
    if(xdel>ydel)
      new_maxC.real=new_minC.real-(new_maxC.imag-new_minC.imag);
    if(xdel<ydel)
      new_maxC.imag=new_minC.imag-(new_maxC.real-new_minC.real);       
  } 
  //bottom left 
  else if((new_minC.real>new_maxC.real)&&(new_minC.imag<new_maxC.imag)) 
  {        
    if(xdel<0)
      xdel=-xdel;
    if(ydel<0)
      ydel=-ydel;
    if(xdel>ydel)
      new_maxC.real=new_minC.real-(new_maxC.imag-new_minC.imag);
    if(xdel<ydel)
      new_maxC.imag=new_minC.imag-(new_maxC.real-new_minC.real);    
  }
  mouse_flag=1;
  glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{ // Your keyboard processing here
  if(key == 'b')
  {
    if( (!arrayCI.empty()) && (!arrayCR.empty()) )
    {
      maxC.imag = arrayCI.top();
      arrayCI.pop();
      maxC.real = arrayCR.top();
      arrayCR.pop();
      minC.imag = arrayCI.top();
      arrayCI.pop();
      minC.real = arrayCR.top();
      arrayCR.pop();
    }
    createThreads();
    display();
  }
}

int main(int argc, char** argv)
{
  // Initialize OpenGL, but only on the "master" thread or process.
  // See the assignment writeup to determine which is "master" 
  // and which is slave.
  pthread_barrier_init(&barrier, NULL, nThreads);
  initColors();
  createThreads();
  glutInit(&argc, argv);
  glutInitWindowSize(WINDOW_DIM,WINDOW_DIM);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowPosition(100,100);
  glutCreateWindow ("Mandelbrotset");
  glClearColor(1.0,1.0,1.0,0);
  init();
  glMatrixMode(GL_MODELVIEW);
  glutDisplayFunc(display);
  glutMotionFunc(motion);
  glutMouseFunc(mouse);
  glutKeyboardFunc(keyboard);
  // glutReshapeFunc(reshape);
  glutTimerFunc(1, timer, 0);
  glutMainLoop();
  return 0;
}
