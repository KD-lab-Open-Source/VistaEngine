/*====================================================================
    macros.h
 
 	macros used to access data structures and perform quick tests.

  ====================================================================*/

#include <stdio.h>
#include <stdlib.h>

#ifndef MY_MACROS_H
#define MY_MACROS_H
/* general-purpose macros */
#define SWAP(t,x,y)	{ t = x; x = y; y = t; }

typedef struct
{
	double x,y,z; //double used to prevent buffer overflows
}Node;


char *malloc();

#define DT_NEW(p,type)	if ((p=(type *) calloc (1, sizeof(type))) == NULL) {\
				printf ("Out of Memory!\n"); \
			}

#define FREE(p)		if (p) { free ((char *) p); p = NULL; }


#define DT_ADD( head, p )  if ( head )  { \
				p->next = head; \
				p->prev = head->prev; \
				head->prev = p; \
				p->prev->next = p; \
			} \
			else { \
				head = p; \
				head->next = head->prev = p; \
			}

#define DT_DELETE( head, p ) if ( head )  { \
				if ( head == head->next ) \
					head = NULL;  \
				else if ( p == head ) \
					head = head->next; \
				p->next->prev = p->prev;  \
				p->prev->next = p->next;  \
				FREE( p ); \
			} 

#endif //MY_MACROS_H
