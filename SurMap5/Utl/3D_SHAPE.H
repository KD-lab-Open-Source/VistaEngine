
#include "general.h"

// 3D Shape Interface

#define SHAPE_CONVERT			1
#define SHAPE_SCALE			    2
#define SHAPE_RELATIVE			 4
#define SHAPE_MOVE			    8
#define SHAPE_LOAD_PARAMS	 16
#define SHAPE_QUIT_IF_EXIST	     32	


int shape_control(int code);
void load_shape(char* name,int x,int y);
void release_shape(void);
//void view_shape();
void view_shape(int ViewX, int ViewY, unsigned short* GRAF_BUF, int GB_MAXX, int GB_MAXY);


extern int shape_size;
extern int shape_shift;
extern unsigned short* upper_buffer;
extern unsigned short* lower_buffer;
extern int shape_x,shape_y;
extern int shape_x_avr;
extern int shape_y_avr;
extern int shape_z_avr;

extern Model shape;
extern DBM A_shape;
