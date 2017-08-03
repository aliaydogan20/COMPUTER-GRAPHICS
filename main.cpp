
#include <windows.h>
#include <GL/gl.h>    
#include <GL/glu.h>   
#include <GL/glut.h> 
#include <stdio.h>  
#include <iostream>   
#include <stdlib.h>  
#include <math.h>

using namespace std;

bool inbetween(float a, float x, float b){
	return (x >= fmin(a, b)) && (x <= fmax(a, b));
}

int iabs(int x){ if (x<0) return -x; else return x; }
int isign(float a){
	if (a<0.0) return -1; else if (a>0.0)return 1; else return 0;
}
int imin(int a, int b){ if (a <= b) return a; else return b; }
int imax(int a, int b){ if (a >= b) return a; else return b; }

static void OpenWindowPlease(int argc, char **argv);
static void GlInitProj(void);
static void GlutInitMenu(void);

/* my callback functions needed for GLUT event handling */
static void GlHandleMenu(int value);
static void GlDisplay(void);
static void GlDisplay2(void);
static void GlReshape(int width, int height);
static void GlMotion(int curx, int cury);
static void GlPassive(int curx, int cury);
static void GlButton(int button, int state, int x, int y);
static void GlIdle(void);
static void GlKeyboard(unsigned char key, int x, int y);

static void InitMyData(void);
void plotpix(int x, int y, double r, double g, double b);

/* functions needed for my drawing and geometry */
static void myDDA(int xstart, int ystart, int xend, int yend);
static void Bresenham(int xstart, int ystart, int xend, int yend);
/*   and a var: */
/*   var DDA function choice (will be one of the above procs)  */
void(*DDAorBresenham) (int xstart, int ystart, int xend, int yend);

/* my menu IDs for GLUT menu-handling functions */
/* can use any integer you like, as long as they are all different */

#define _MENU_EXIT         9999
#define _MENU_CLEAR           0
#define _MENU_DDA             1
#define _MENU_BRESENHAM       2
#define _MENU_NULL           -1

/* my constants for GLUT window defaults */
#define _WIND_X_SIZE       4000
#define _WIND_Y_SIZE       4000
#define _WIND_X_MINSIZE     20
#define _WIND_Y_MINSIZE     20
#define _WIND_X_POS         0
#define _WIND_Y_POS         0


/* my constants for stored information */
#define _RASTER_X_SIZE      100
#define _RASTER_Y_SIZE      100

/* the simulated 1 bit raster memory */
static int        gMyRasterMem[_RASTER_X_SIZE][_RASTER_Y_SIZE];

/* various coordinates in the 'big' simulated video raster memory */
static int  gMyPixelSizeX, gMyPixelSizeY;
static int  gMybigPassiveX, gMybigPassiveY;
static int  gMybigMotionX, gMybigMotionY;
static int  gMybigAX, gMybigAY;
static int  gMybigBX, gMybigBY;
static int  gMybigBprevX, gMybigBprevY;

/*  ====================================================================== */

int main(int argc, char **argv) {
	OpenWindowPlease(argc, argv);
	GlutInitMenu();          /* my GLUT menu initialization functions */
	GlInitProj();            /* initialize OpenGL projection */
	
	glutReshapeFunc(GlReshape); /* after reshaping GLUT window */
	glutMouseFunc(GlButton);   /* clicking/releasing button */
	glutMotionFunc(GlMotion);  /* moving mouse w/ button down */
	glutPassiveMotionFunc(GlPassive);  /* moving mouse w/ no buttons */
	glutIdleFunc(NULL);          /* or when nothing is going on */
	glutKeyboardFunc(GlKeyboard);      /* when a key is pressed */

	/* finally, initialize my globals */
	InitMyData();

	fprintf(stdout, "1.Use DDA algorithm \n");
	fprintf(stdout, "2.Use Bresenham algorithm\n");
	fprintf(stdout, "3.Exit\n");
	fprintf(stdout, "...");

	glutMainLoop();

	return(EXIT_SUCCESS);

} /* main() */

void GlButton(int button, int state, int curx, int cury) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			/* here I know the mouse's left button is down */
			gMybigAX = imin(curx / gMyPixelSizeX, (_RASTER_X_SIZE - 1));
			gMybigAY = imin(cury / gMyPixelSizeY, (_RASTER_Y_SIZE - 1));
			gMybigAX = imax(0, gMybigAX);
			gMybigAY = imax(0, gMybigAY);
			gMybigBX = gMybigAX;   gMybigBprevX = gMybigAX;
			gMybigBY = gMybigAY;   gMybigBprevY = gMybigAY;
		}
	}
	glutPostRedisplay(); /* make sure we Display according to changes */
} /* GlButton() */


/* Klavyeden girilecek olan argumanlar*/
void GlKeyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 27:                /* ESC key - default exit for IRIX (*not*   */
		/*           an OpenGL convention!)         */
	case '3':               /*           IMHO it's uuuuuugly. But then? */           
		exit(0);
	case '2':
		glutDisplayFunc(GlDisplay);  
		break;
	case '1':
		glutDisplayFunc(GlDisplay2);   
		break;
	}
} /* GlKeyboard() */


/* ------------------------------------------------------------------------ */
/* GlMotion() - GLUT callback, responds to the "active" mouse motion  */
/*                    In this function manipulate global vars only.         */
/*                    Do not attempt **any** heavy computational load or    */
/*                    graphical output.                                     */
/*                    Do graphical output **only** in GlDisplay().    */
/* ------------------------------------------------------------------------ */
void GlMotion(int curx, int cury) {

	gMybigMotionX = imin(curx / gMyPixelSizeX, (_RASTER_X_SIZE - 1));
	gMybigMotionY = imin(cury / gMyPixelSizeY, (_RASTER_Y_SIZE - 1));
	gMybigMotionX = imax(0, gMybigMotionX);
	gMybigMotionY = imax(0, gMybigMotionY);
	gMybigBX = gMybigMotionX;
	gMybigBY = gMybigMotionY;

	/* tell GLUT to remind GlDisplay() to redraw the window,
	otherwise no result nor feedback from change in globals may occur */
	glutPostRedisplay();

} /* GlMotion() */


void GlPassive(int curx, int cury) {
	gMybigPassiveX = imin(curx / gMyPixelSizeX, (_RASTER_X_SIZE - 1));
	gMybigPassiveY = imin(cury / gMyPixelSizeY, (_RASTER_Y_SIZE - 1));
	gMybigPassiveX = imax(0, gMybigPassiveX);
	gMybigPassiveY = imax(0, gMybigPassiveY);
	/* tell GLUT to remind GlDisplay() to redraw the window,
	otherwise no result nor feedback from change in globals may occur */
	glutPostRedisplay();
} /* GlPassive() */

/*   standard open window glut calls: */

void OpenWindowPlease(int argc, char **argv){
	/* GLUT initialization here */
	glutInit(&argc, argv);
	glutInitWindowPosition(_WIND_X_POS, _WIND_Y_POS);
	glutInitWindowSize(_WIND_X_SIZE, _WIND_Y_SIZE);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutCreateWindow("< HW01 >");
} 

void GlDisplay(void) {
	int        i, j;
	DDAorBresenham = myDDA; /*  default function  */

	glClearColor(0.8f, 0.96f, 0.95f, 4.0f);
	glClear(GL_COLOR_BUFFER_BIT);  /* clear  the color buffer! */

    glPointSize(12.0);
	glLineWidth(12.0);

	/* draw raster information: big pixels */
	for (i = 0; i < _RASTER_X_SIZE; i++) {
		for (j = 0; j < _RASTER_Y_SIZE; j++) {
			if (gMyRasterMem[i][j] == 0) /*   i=x, j=y  */
				glColor3f(0.0, 0.0, 0.0);
			else
				glColor3f(1.0, 1.0, 1.0);
			glRecti(((gMyPixelSizeX * i) + 1), ((gMyPixelSizeY * j) + 1),
				((gMyPixelSizeX * (i + 1)) - 1), ((gMyPixelSizeY * (j + 1)) - 1));
		} /* for j */
	} /* for i */

	/* yellow for the 'passive' mouse tracking rectangle */
	glColor3f(1.0f, 1.0f, 1.0f);
	glRecti((gMyPixelSizeX * gMybigPassiveX), (gMyPixelSizeY * gMybigPassiveY),
		(gMyPixelSizeX * (gMybigPassiveX + 1)), (gMyPixelSizeY * (gMybigPassiveY + 1)));

	/* magenta for the 'active' mouse tracking rectangle */
	glColor3f(1.0f, 0.0f, 1.0f);
	glRecti((gMyPixelSizeX * gMybigMotionX), (gMyPixelSizeY * gMybigMotionY),
		(gMyPixelSizeX * (gMybigMotionX + 1)), (gMyPixelSizeY * (gMybigMotionY + 1)));

	/* green for vertex A tracking rectangle */
	glColor3f(0.0f, 1.0f, 0.0f);
	glRecti((gMyPixelSizeX * gMybigAX), (gMyPixelSizeY * gMybigAY),
		(gMyPixelSizeX * (gMybigAX + 1)), (gMyPixelSizeY * (gMybigAY + 1)));

	/* blue for vertex B tracking rectangle */
	glColor3f(0.0f, 0.0f, 1.0f);
	glRecti((gMyPixelSizeX * gMybigBX), (gMyPixelSizeY * gMybigBY),
		(gMyPixelSizeX * (gMybigBX + 1)), (gMyPixelSizeY * (gMybigBY + 1)));

	/* 'drawing' of line with our own algorithm using big pixels */
	/*   first clear ( xor !!! )  previous line */

	DDAorBresenham(gMybigAX, gMybigAY, gMybigBprevX, gMybigBprevY);
	/*  and then draw new line !!! */
	DDAorBresenham(gMybigAX, gMybigAY, gMybigBX, gMybigBY);
	/* 'drawing' of big pixels done */

	/*    printf(".-^-._______ draw OpenGL line ________.-^-.\n"); */
	glColor3f(1.0f, 0.2f, 0.2f);
	glBegin(GL_LINES);
	glVertex2i((gMyPixelSizeX * gMybigAX) + (int)(gMyPixelSizeX / 2),
		(gMyPixelSizeY * gMybigAY) + (int)(gMyPixelSizeY / 2));
	glVertex2i((gMyPixelSizeX * gMybigBX) + (int)(gMyPixelSizeX / 2),
		(gMyPixelSizeY * gMybigBY) + (int)(gMyPixelSizeY / 2));
	glEnd();

	gMybigBprevX = gMybigBX;
	gMybigBprevY = gMybigBY;
	/* and I tell GLUT to swap buffers == perform the display */
	glutSwapBuffers();
} /* GlDisplay() */


void GlDisplay2(void) {
	
	int        i, j;
	
	DDAorBresenham = Bresenham; /*  default function  */

	glClearColor(255.0,255.0,255,220);
	glClear(GL_COLOR_BUFFER_BIT); 
	glPointSize(10.0);
	glLineWidth(10.0);

	
	
	/* draw raster information: big pixels */
	for (i = 0; i < _RASTER_X_SIZE; i++) {
		for (j = 0; j < _RASTER_Y_SIZE; j++) {
			if (gMyRasterMem[i][j] == 0) /*   i=x, j=y  */
				glColor3f(0.0, 0.0, 0.0);
			else
				glColor3f(1.0, 1.0, 1.0);
			glRecti(((gMyPixelSizeX * i) + 1), ((gMyPixelSizeY * j) + 1),
				((gMyPixelSizeX * (i + 1)) - 1), ((gMyPixelSizeY * (j + 1)) - 1));
		} /* for j */
	} /* for i */

	glColor3f(0.5f, 1.8f, 3.0f);
	glRecti((gMyPixelSizeX * gMybigPassiveX), (gMyPixelSizeY * gMybigPassiveY),
		(gMyPixelSizeX * (gMybigPassiveX + 1)), (gMyPixelSizeY * (gMybigPassiveY + 1)));

	
	glColor3f(1.0f, 0.4f, 2.0f);
	glRecti((gMyPixelSizeX * gMybigMotionX), (gMyPixelSizeY * gMybigMotionY),
		(gMyPixelSizeX * (gMybigMotionX + 1)), (gMyPixelSizeY * (gMybigMotionY + 1)));

	/* green for vertex A tracking rectangle */
	glColor3f(0.0f, 1.0f, 0.0f);
	glRecti((gMyPixelSizeX * gMybigAX), (gMyPixelSizeY * gMybigAY),
		(gMyPixelSizeX * (gMybigAX + 1)), (gMyPixelSizeY * (gMybigAY + 1)));

	/* blue for vertex B tracking rectangle */
	glColor3f(0.0f, 0.0f, 1.0f);
	glRecti((gMyPixelSizeX * gMybigBX), (gMyPixelSizeY * gMybigBY),
		(gMyPixelSizeX * (gMybigBX + 1)), (gMyPixelSizeY * (gMybigBY + 1)));

	/* 'drawing' of line with our own algorithm using big pixels */
	/*   first clear ( xor !!! )  previous line */

	DDAorBresenham(gMybigAX, gMybigAY, gMybigBprevX, gMybigBprevY);
	/*  and then draw new line !!! */
	DDAorBresenham(gMybigAX, gMybigAY, gMybigBX, gMybigBY);
	/* 'drawing' of big pixels done */

	/*    printf(".-^-._______ draw OpenGL line ________.-^-.\n"); */
	glColor3f(1.0f, 0.2f, 0.2f);
	glBegin(GL_LINES);
	glVertex2i((gMyPixelSizeX * gMybigAX) + (int)(gMyPixelSizeX / 2),
		(gMyPixelSizeY * gMybigAY) + (int)(gMyPixelSizeY / 2));
	glVertex2i((gMyPixelSizeX * gMybigBX) + (int)(gMyPixelSizeX / 2),
		(gMyPixelSizeY * gMybigBY) + (int)(gMyPixelSizeY / 2));
	glEnd();

	gMybigBprevX = gMybigBX;
	gMybigBprevY = gMybigBY;
	/* and I tell GLUT to swap buffers == perform the display */
	glutSwapBuffers();
}




/* ------------------------------------------------------------------------ */
/* GlReshape() - GLUT callback function if window has been reshaped   */
/* ------------------------------------------------------------------------ */
void GlReshape(int width, int height) {

	/* this time, force window at least 200x200 ... where? */
	GlInitProj();                         /* reset OpenGL projection mode */

	gMyPixelSizeX = glutGet(GLUT_WINDOW_WIDTH) / _RASTER_X_SIZE;
	gMyPixelSizeY = glutGet(GLUT_WINDOW_HEIGHT) / _RASTER_Y_SIZE;

	glutPostRedisplay();   /* now tell GLUT it's time to update the window! */

} /* GlReshape() */


/* ------------------------------------------------------------------------ */
/* GlutInitMenu() - Initialize GLUT Menus */
/* ------------------------------------------------------------------------ */
void GlutInitMenu(void) {
	/* I create a GLUT menu to appear when the right mouse button is clicked */
	/*  EXIT 9999   CLEAR 1   SIMPLE 2   DDA1 3    DDA2 4   BRESENHAM 5 NULL -1 */
	glutCreateMenu(GlHandleMenu);                /* this is just a handle */
	glutAddMenuEntry("Clear Window Contents", _MENU_CLEAR);
	glutAddMenuEntry("DDA ", _MENU_DDA);
	glutAddMenuEntry("Bresenham ", _MENU_BRESENHAM);
	glutAddMenuEntry("NULL ", _MENU_NULL);
	glutAddMenuEntry("Exit", _MENU_EXIT);         /* GlHandleMenu() */
	glutAttachMenu(GLUT_RIGHT_BUTTON);  /* I attach it to the right button. */
} /* GlutInitMenu() */

/* ------------------------------------------------------------------------ */
/* GlInitProj() - (re)initialize OpenGL projection... absolutely CRUCIAL! */
/* ------------------------------------------------------------------------ */
void GlInitProj(void) {
	int        width_viewport, hight_viewport;   /*  to store the size of the window */
	int ww = glutGet(GLUT_WINDOW_WIDTH);
	int hh = glutGet(GLUT_WINDOW_HEIGHT);
	/*  get the current window size from GLUT */
	width_viewport = imax(ww, _WIND_X_MINSIZE);
	hight_viewport = imax(hh, _WIND_Y_MINSIZE);
	/* this time, force window at least 200x200 */
	glutReshapeWindow(width_viewport, hight_viewport);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	/* use the GL projection matrix: we want to initialize that one */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	/* this also takes care of the GLUT vs. OpenGL coordinate mismatch: */
	gluOrtho2D(0.0, width_viewport, hight_viewport, 0.0);
	/* call glViewport() to clip the display to the new window size */
	glViewport(0, 0, width_viewport, hight_viewport);
} /* GlInitProj() */

/* ------------------------------------------------------------------------ */
/* GlHandleMenu() - Handle GLUT Menus.  Do not do heavy computation here! */
/* ------------------------------------------------------------------------ */

void GlHandleMenu(int value) {
	switch (value) {
	case _MENU_CLEAR: /*DO NOT CLEAR DISPLAY HERE: post a message to "display" */
		GlInitProj();  InitMyData();      /* reset OpenGL projection mode */
		glutPostRedisplay();
		break;
	case _MENU_DDA:    DDAorBresenham = myDDA; break;
	case _MENU_BRESENHAM: DDAorBresenham =Bresenham; break;
	case _MENU_EXIT:
		exit(0);
		break;
	default:;       /*  empty  */
	} /* switch () */
	glutPostRedisplay(); /* make sure we Display according to changes */
} /* GlHandleMenu() */


/* ------------------------------------------------------------------------ */
/* InitMyData() - Initialize my array, etc */
/* ------------------------------------------------------------------------ */
void InitMyData(void) {
	int i, j;
	
	DDAorBresenham = myDDA; /*  default function  */

	for (i = 0; i<_RASTER_X_SIZE; i++) {
		for (j = 0; j<_RASTER_Y_SIZE; j++) {
			gMyRasterMem[i][j] = 0; /*  i=x, j=y  */
		}
	}

	gMyPixelSizeX = glutGet(GLUT_WINDOW_WIDTH) / _RASTER_X_SIZE;
	gMyPixelSizeY = glutGet(GLUT_WINDOW_HEIGHT) / _RASTER_Y_SIZE;
	gMybigPassiveX = 0; gMybigPassiveY = 0;
	gMybigMotionX = 0; gMybigMotionY = 0;
	gMybigAX = 0; gMybigAY = 0;
	gMybigBX = 0; gMybigBY = 0;
	gMybigBprevX = 0; gMybigBprevY = 0;

} /* InitMyData() */

void plotpix(int x, int y, double r, double g, double b) {

	gMyRasterMem[x][y] = 0x000000FF & (gMyRasterMem[x][y] ^ 1);

	glColor3f(r, g, b);
	glRecti(((gMyPixelSizeX * x) + 1), ((gMyPixelSizeY * y) + 1),
		((gMyPixelSizeX * (x + 1)) - 1), ((gMyPixelSizeY * (y + 1)) - 1));
} /*  plotpix  */

void sca(int * x, int * y){ int  z; z = *x; *x = *y; *y = z; }

/*  ======================================================================== */
/*  DRAW LINE ALGORITHMS: */

static void myDDA(int xstart, int ystart, int xend, int yend){
	float a, b, x, y, dx, dy;
	dy = yend - ystart;   dx = xend - xstart;
	if (dx == 0 && dy == 0) {
		plotpix((int)xstart, (int)ystart, 1.0, 1.0, 1.0);
		return;
	}
	if (fabs(dx) > fabs(dy)){
		if (xstart>xend) {
			sca(&xstart, &xend); sca(&ystart, &yend);
			dy = yend - ystart;   dx = xend - xstart;
		}
		a = dy / dx;
		b = ystart - a * xstart; /*  y = a*x + b; -> b=y-a*x; ->.. */
		x = xstart;
		do{
			y = a*x + b;
			plotpix((int)x, (int)y, 1.0, 1.0, 1.0);
			x = x + 1;
		} while (x <= xend);
	}
	else {
		if (ystart>yend) {
			sca(&xstart, &xend); sca(&ystart, &yend);
			dy = yend - ystart;   dx = xend - xstart;
		}
		a = dx / dy;
		b = xstart - a * ystart;  /*  x = a*y +b;  b = x - a*y; ..  */
		y = ystart;   /* x2=a*y2+b;  x1=a*y1+b; b=x1-a*y1;  x2-x1=a*(y2-y1); a=dx/dy */
		do{
			x = a*y + b;
			plotpix((int)x, (int)y, 1.0, 1.0, 1.0);
			y = y + 1;
		} while (y <= yend);
	}
} /*  Simple */





void Bresenham(int xstart, int ystart, int xend, int yend) {
	int i, tmp, x, y, dx, dy, Signdx, Signdy, Two_dx, Two_dy, Swap, error;
	int uStartX, uStartY;
	if (abs(xstart - xend)<2){ /*  vertical: */
		y = ystart; if (ystart<yend) dy = 1; else dy = -1;
		do {
			plotpix(xstart, y, 1.0f, 1.0f, 1.0f);
			y = y + dy;
		} while (abs(yend - y)>1);
		return;
	}
	if (xstart > xend) {
		sca(&xstart, &xend); sca(&ystart, &yend);
	}

	x = xstart;
	y = ystart;

	dx = xend - xstart;
	dy = yend - ystart;
	Swap = 0;
	Signdx = isign(dx);
	Signdy = isign(dy);
	dx = abs(dx);
	dy = abs(dy);
	if (dy>dx) { tmp = dx; dx = dy; dy = tmp; Swap = 1; }
	Two_dx = 2 * dx;
	Two_dy = 2 * dy;
	error = Two_dy - dx;

	for (i = 1; i <= dx; i++) {
		plotpix(x, y, 1.0f, 1.0f, 1.0f);

		if (error > 0) {
			if (Swap) x = x + Signdx;  else y = y + Signdy;
			error = error - Two_dx;
		}
		if (Swap) y = y + Signdy; else  x = x + Signdx;
		error = error + Two_dy;
	}

	/*  gMyRasterMem[x][y] = 0x000000FF & (gMyRasterMem[x][y] ^ 1); */
	plotpix(x, y, 1.0f, 1.0f, 1.0f);

} /* */


/* ------------------------------------------------------------------------ */
/* GlIdle() - callback for me to do something when GLUT does nothing */
/* ------------------------------------------------------------------------ */
void GlIdle(void) {
} /* GlIdle() */
