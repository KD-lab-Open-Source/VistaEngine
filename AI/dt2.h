/*
This code is described in "Computational Geometry in C" (Second Edition),
Chapter 5.  It is not written to be comprehensible without the 
explanation in that book.

Input: 2n integer coordinates for the points.
Output: The Delaunay triangulation, in postscript with embedded comments.

Compile: gcc -o dt2 dt2.c (or simply: make)

Written by Joseph O'Rourke.
Last modified: July 1997
Questions to orourke@cs.smith.edu.
--------------------------------------------------------------------
This code is Copyright 1998 by Joseph O'Rourke.  It may be freely 
redistributed in its entirety provided that this copyright notice is 
not removed.
--------------------------------------------------------------------
*/

#include <stdio.h>
#include <math.h>

/*Define Boolean type */
//typedef	enum { FALSE, TRUE }	bool;

//#define FALSE false
//#define TRUE true

/* Define vertex indices. */
#define X   0
#define Y   1
#define Z   2


/* Define flags */
#define ONHULL   	TRUE
#define REMOVED  	TRUE
#define VISIBLE  	TRUE
#define PROCESSED	TRUE
#define SAFE            1000000         /* Range of safe coord values. */

/* Global variable definitions */
tVertex vertices = NULL;
tEdge edges    	 = NULL;
tFace faces    	 = NULL;
int vnum = 0;
bool debug = FALSE;
bool check = FALSE;

/* Function declarations */
tVertex MakeNullVertex( void );
void    SubVec( float a[3], float b[3], float c[3]);
void    DoubleTriangle( void );
void    ConstructHull( void );
bool	AddOne( tVertex p );
float   VolumeSign(tFace f, tVertex p);
float   Volumei( tFace f, tVertex p );
tFace	MakeConeFace( tEdge e, tVertex p );
void    MakeCcw( tFace f, tEdge e, tVertex p );
tEdge   MakeNullEdge( void );
tFace   MakeNullFace( void );
tFace   MakeFace( tVertex v0, tVertex v1, tVertex v2, tFace f );
void    CleanUp( void );
void    CleanEdges( void );
void    CleanFaces( void );
void    CleanVertices( void );
bool	Collinear( tVertex a, tVertex b, tVertex c );
int	    Normz( tFace f );
void    CheckEuler(int V, int E, int F );
void    Checks( void );
void	Consistency( void );
void	Convexity( void );
void    LowerFaces( void );

#include "macros.h"

//---------------------------------------------
void    LowerFaces( void )
{
   tFace f = faces;
   /*int   z;*/
   int   Flower = 0;   /* Total number of lower faces. */

   do {
     /*z = Normz( f );
     if ( z < 0 ) {*/
     if ( Normz( f ) < 0 ) {
        Flower++;
        f->lower = TRUE;
        /*printf("z=%10d; lower face indices: %d, %d, %d\n", z, */
        /*printf("lower face indices: %d, %d, %d\n",
           f->vertex[0]->vnum,
           f->vertex[1]->vnum,
           f->vertex[2]->vnum );*/
     }
     else f->lower = FALSE;
     f = f->next;
   } while ( f != faces );
   /*printf("A total of %d lower faces identified.\n", Flower);*/
}

/*---------------------------------------------------------------------
MakeNullVertex: Makes a vertex, nulls out fields.
---------------------------------------------------------------------*/
tVertex	MakeNullVertex( void )
{
   tVertex  v = NULL;
   
   DT_NEW( v, tsVertex );
   v->duplicate = NULL;
   v->onhull = !ONHULL;
   v->mark = !PROCESSED;
   DT_ADD( vertices, v );

   return v;
}

//---------------------------------------------
void DT2AddVertex(float x, float y, Circle * data)
{
   tVertex  v = NULL;
   v = MakeNullVertex();
   v->v[X] = x;
   v->v[Y] = y;
   v->v[Z] = x*x + y*y;
   v->data=data;
   v->vnum = vnum++;
}

/*---------------------------------------------------------------------
SubVec:  Computes a - b and puts it into c.
---------------------------------------------------------------------*/
void    SubVec( float a[3], float b[3], float c[3])
{
   int  i;

   for( i=0; i < 2; i++ )
      c[i] = a[i] - b[i];

}

/*---------------------------------------------------------------------
 DoubleTriangle builds the initial double triangle.  It first finds 3 
 noncollinear points and makes two faces out of them, in opposite order.
 It then finds a fourth point that is not coplanar with that face.  The  
 vertices are stored in the face structure in counterclockwise order so 
 that the volume between the face and the point is negative. Lastly, the
 3 newfaces to the fourth point are constructed and the data structures
 are cleaned up. 
---------------------------------------------------------------------*/
void    DoubleTriangle( void )
{
   tVertex  v0, v1, v2, v3;
   tFace    f0, f1 = NULL;
//   tEdge    e0, e1, e2, s;
   float      vol;
	
	
   /* Find 3 non-Collinear points. */
   v0 = vertices;
   while ( Collinear( v0, v0->next, v0->next->next ) )
      if ( ( v0 = v0->next ) == vertices )
         printf("DoubleTriangle:  All points are Collinear!\n"), exit(0);
   v1 = v0->next;
   v2 = v1->next;
	
   /* Mark the vertices as processed. */
   v0->mark = PROCESSED;
   v1->mark = PROCESSED;
   v2->mark = PROCESSED;
   
   /* Create the two "twin" faces. */
   f0 = MakeFace( v0, v1, v2, f1 );
   f1 = MakeFace( v2, v1, v0, f0 );

   /* Link adjacent face fields. */
   f0->edge[0]->adjface[1] = f1;
   f0->edge[1]->adjface[1] = f1;
   f0->edge[2]->adjface[1] = f1;
   f1->edge[0]->adjface[1] = f0;
   f1->edge[1]->adjface[1] = f0;
   f1->edge[2]->adjface[1] = f0;
	
   /* Find a fourth, non-coplanar point to form tetrahedron. */
   v3 = v2->next;
   vol = VolumeSign( f0, v3 );
   while ( !vol )   {
      if ( ( v3 = v3->next ) == v0 ) 
         printf("DoubleTriangle:  All points are coplanar!\n"), exit(0);
      vol = VolumeSign( f0, v3 );
   }
	
   /* Insure that v3 will be the first added. */
   vertices = v3;
   if ( debug ) {
      fprintf(stderr, "DoubleTriangle: finished. Head repositioned at v3.\n");
   }

	
}

/*---------------------------------------------------------------------
ConstructHull adds the vertices to the hull one at a time.  The hull
vertices are those in the list marked as onhull.
---------------------------------------------------------------------*/
void	ConstructHull( void )
{
   tVertex  v, vnext;
//   int 	    vol;
   bool	    changed;	/* T if addition changes hull; not used. */

   v = vertices;
   do {
      vnext = v->next;
      if ( !v->mark ) {
         v->mark = PROCESSED;
	 changed = AddOne( v );
	 CleanUp();

	 if ( check ) {
	    fprintf(stderr,"ConstructHull: After Add of %d & Cleanup:\n", 
               v->vnum);
	    Checks();
	 }
     }
      v = vnext;
   } while ( v != vertices );
}

/*---------------------------------------------------------------------
AddOne is passed a vertex.  It first determines all faces visible from 
that point.  If none are visible then the point is marked as not 
onhull.  Next is a loop over edges.  If both faces adjacent to an edge
are visible, then the edge is marked for deletion.  If just one of the
adjacent faces is visible then a new face is constructed.
---------------------------------------------------------------------*/
bool 	AddOne( tVertex p )
{
   tFace  f; 
   tEdge  e;
   float	  vol;
   bool	  vis = FALSE;

   /* Mark faces visible from p. */
   f = faces;
   do {
      vol = VolumeSign( f, p );
      if (debug) fprintf(stderr, 
         "faddr: %6x   paddr: %6x   Vol = %d\n", f,p,vol);
      if ( vol < 0 ) {
	 f->visible = VISIBLE;  
	 vis = TRUE;                      
      }
      f = f->next;
   } while ( f != faces );

   /* If no faces are visible from p, then p is inside the hull. */
   if ( !vis ) {
      p->onhull = !ONHULL;  
      return FALSE; 
   }

   /* Mark edges in interior of visible region for deletion.
      Erect a newface based on each border edge. */
   e = edges;
   do {
      tEdge temp;
      temp = e->next;
      if ( e->adjface[0]->visible && e->adjface[1]->visible )
	 /* e interior: mark for deletion. */
	 e->del = REMOVED;
      else if ( e->adjface[0]->visible || e->adjface[1]->visible ) 
	 /* e border: make a new face. */
	 e->newface = MakeConeFace( e, p );
      e = temp;
   } while ( e != edges );
   return TRUE;
}

/*---------------------------------------------------------------------
VolumeSign returns the sign of the volume of the tetrahedron determined by f
and p.  VolumeSign is +1 iff p is on the negative side of f,
where the positive side is determined by the rh-rule.  So the volume 
is positive if the ccw normal to f points outside the tetrahedron.
The final fewer-multiplications form is due to Robert Fraczkiewicz.
---------------------------------------------------------------------*/
float  VolumeSign( tFace f, tVertex p )
{
   double  vol;
//   float   voli;
   double  ax, ay, az, bx, by, bz, cx, cy, cz, dx, dy, dz;
   double  bxdx, bydy, bzdz, cxdx, cydy, czdz;

   ax = f->vertex[0]->v[X];
   ay = f->vertex[0]->v[Y];
   az = f->vertex[0]->v[Z];
   bx = f->vertex[1]->v[X];
   by = f->vertex[1]->v[Y];
   bz = f->vertex[1]->v[Z];
   cx = f->vertex[2]->v[X];
   cy = f->vertex[2]->v[Y];
   cz = f->vertex[2]->v[Z];
   dx = p->v[X];
   dy = p->v[Y];
   dz = p->v[Z];
   
   bxdx=bx-dx;
   bydy=by-dy;
   bzdz=bz-dz;
   cxdx=cx-dx;
   cydy=cy-dy;
   czdz=cz-dz;
   vol =   (az-dz) * (bxdx*cydy - bydy*cxdx)
         + (ay-dy) * (bzdz*cxdx - bxdx*czdz)
	 + (ax-dx) * (bydy*czdz - bzdz*cydy);

//   if ( debug )
//      fprintf(stderr,"Face=%6x; Vertex=%d: vol(int) = %d, vol(double) = %lf\n",
//	      f,p->vnum,voli,vol);

   /* The volume should be an integer. */
   if      ( vol > 0.5 )   return  1;
   else if ( vol < -0.5 )   return -1;
   else                  return  0;
}
/*---------------------------------------------------------------------*/
float  Volumei( tFace f, tVertex p )
{
   float 	   vol;
   float 	   ax, ay, az, bx, by, bz, cx, cy, cz, dx, dy, dz;
   float	   bxdx, bydy, bzdz, cxdx, cydy, czdz;
//   double  vold;
//   int	   i;

   ax = f->vertex[0]->v[X];
   ay = f->vertex[0]->v[Y];
   az = f->vertex[0]->v[Z];
   bx = f->vertex[1]->v[X];
   by = f->vertex[1]->v[Y];
   bz = f->vertex[1]->v[Z];
   cx = f->vertex[2]->v[X];
   cy = f->vertex[2]->v[Y];
   cz = f->vertex[2]->v[Z];
   dx = p->v[X];
   dy = p->v[Y];
   dz = p->v[Z];
   
   bxdx=bx-dx;
   bydy=by-dy;
   bzdz=bz-dz;
   cxdx=cx-dx;
   cydy=cy-dy;
   czdz=cz-dz;
   vol =   (az-dz)*(bxdx*cydy-bydy*cxdx)
         + (ay-dy)*(bzdz*cxdx-bxdx*czdz)
	 + (ax-dx)*(bydy*czdz-bzdz*cydy);

   return vol;
}		

/*---------------------------------------------------------------------
Volumed is the same as VolumeSign but computed with doubles.  For 
protection against overflow.
---------------------------------------------------------------------*/
double 	Volumed( tFace f, tVertex p )
{
   double  vol;
   double  ax, ay, az, bx, by, bz, cx, cy, cz, dx, dy, dz;
   double  bxdx, bydy, bzdz, cxdx, cydy, czdz;

   ax = f->vertex[0]->v[X];
   ay = f->vertex[0]->v[Y];
   az = f->vertex[0]->v[Z];
   bx = f->vertex[1]->v[X];
   by = f->vertex[1]->v[Y];
   bz = f->vertex[1]->v[Z];
   cx = f->vertex[2]->v[X];
   cy = f->vertex[2]->v[Y];
   cz = f->vertex[2]->v[Z];
   dx = p->v[X];
   dy = p->v[Y];
   dz = p->v[Z];
   
   bxdx=bx-dx;
   bydy=by-dy;
   bzdz=bz-dz;
   cxdx=cx-dx;
   cydy=cy-dy;
   czdz=cz-dz;
   vol = (az-dz)*(bxdx*cydy-bydy*cxdx)
         + (ay-dy)*(bzdz*cxdx-bxdx*czdz)
	 + (ax-dx)*(bydy*czdz-bzdz*cydy);

   return vol;
}

/*-------------------------------------------------------------------*/
void	PrintPoint( tVertex p )
{
   int	i;

   for ( i = 0; i < 3; i++ )
      printf("\t%d", p->v[i]);
   putchar('\n');
}

/*---------------------------------------------------------------------
MakeConeFace makes a new face and two new edges between the 
edge and the point that are passed to it. It returns a pointer to
the new face.
---------------------------------------------------------------------*/
tFace	MakeConeFace( tEdge e, tVertex p )
{
   tEdge  new_edge[2];
   tFace  new_face;
   int 	  i, j;

   /* Make two new edges (if don't already exist). */
   for ( i=0; i < 2; ++i ) 
      /* If the edge exists, copy it into new_edge. */
      if ( !( new_edge[i] = e->endpts[i]->duplicate) ) {
	 /* Otherwise (duplicate is NULL), MakeNullEdge. */
	 new_edge[i] = MakeNullEdge();
	 new_edge[i]->endpts[0] = e->endpts[i];
	 new_edge[i]->endpts[1] = p;
	 e->endpts[i]->duplicate = new_edge[i];
      }

   /* Make the new face. */
   new_face = MakeNullFace();   
   new_face->edge[0] = e;
   new_face->edge[1] = new_edge[0];
   new_face->edge[2] = new_edge[1];
   MakeCcw( new_face, e, p ); 
        
   /* Set the adjacent face pointers. */
   for ( i=0; i < 2; ++i )
      for ( j=0; j < 2; ++j )  
	 /* Only one NULL link should be set to new_face. */
	 if ( !new_edge[i]->adjface[j] ) {
	    new_edge[i]->adjface[j] = new_face;
	    break;
	 }
        
   return new_face;
}

/*---------------------------------------------------------------------
MakeCcw puts the vertices in the face structure in counterclock wise 
order.  We want to store the vertices in the same 
order as in the visible face.  The third vertex is always p.
---------------------------------------------------------------------*/
void	MakeCcw( tFace f, tEdge e, tVertex p )
{
   tFace  fv;   /* The visible face adjacent to e */
   int    i;    /* Index of e->endpoint[0] in fv. */
   tEdge  s;	/* Temporary, for swapping */
      
   if  ( e->adjface[0]->visible )      
        fv = e->adjface[0];
   else fv = e->adjface[1];
       
   /* Set vertex[0] & [1] of f to have the same orientation
      as do the corresponding vertices of fv. */ 
   for ( i=0; fv->vertex[i] != e->endpts[0]; ++i )
      ;
   /* Orient f the same as fv. */
   if ( fv->vertex[ (i+1) % 3 ] != e->endpts[1] ) {
      f->vertex[0] = e->endpts[1];  
      f->vertex[1] = e->endpts[0];    
   }
   else {                               
      f->vertex[0] = e->endpts[0];   
      f->vertex[1] = e->endpts[1];      
      SWAP( s, f->edge[1], f->edge[2] );
   }
   /* This swap is tricky. e is edge[0]. edge[1] is based on endpt[0],
      edge[2] on endpt[1].  So if e is oriented "forwards," we
      need to move edge[1] to follow [0], because it precedes. */
   
   f->vertex[2] = p;
}
 
/*---------------------------------------------------------------------
MakeNullEdge creates a new cell and initializes all pointers to NULL
and sets all flags to off.  It returns a pointer to the empty cell.
---------------------------------------------------------------------*/
tEdge 	MakeNullEdge( void )
{
   tEdge  e = NULL;

   DT_NEW( e, tsEdge );
   e->adjface[0] = e->adjface[1] = e->newface = NULL;
   e->endpts[0] = e->endpts[1] = NULL;
   e->del = !REMOVED;
   DT_ADD( edges, e );
   return e;
}

/*--------------------------------------------------------------------
MakeNullFace creates a new face structure and initializes all of its
flags to NULL and sets all the flags to off.  It returns a pointer
to the empty cell.
---------------------------------------------------------------------*/
tFace 	MakeNullFace( void )
{
   tFace  f = NULL;
   int    i;

   DT_NEW( f, tsFace);
   for ( i=0; i < 3; ++i ) {
      f->edge[i] = NULL;
      f->vertex[i] = NULL;
	  f->dFace = NULL;
   }
   f->visible = !VISIBLE;
   DT_ADD( faces, f );
   return f;
}

/*---------------------------------------------------------------------
MakeFace creates a new face structure from three vertices (in ccw
order).  It returns a pointer to the face.
---------------------------------------------------------------------*/
tFace   MakeFace( tVertex v0, tVertex v1, tVertex v2, tFace fold )
{
   tFace  f;
   tEdge  e0, e1, e2;

   /* Create edges of the initial triangle. */
   if( !fold ) {
     e0 = MakeNullEdge();
     e1 = MakeNullEdge();
     e2 = MakeNullEdge();
   }
   else { /* Copy from fold, in reverse order. */
     e0 = fold->edge[2];
     e1 = fold->edge[1];
     e2 = fold->edge[0];
   }
   e0->endpts[0] = v0;              e0->endpts[1] = v1;
   e1->endpts[0] = v1;              e1->endpts[1] = v2;
   e2->endpts[0] = v2;              e2->endpts[1] = v0;
	
   /* Create face for triangle. */
   f = MakeNullFace();
   f->edge[0]   = e0;  f->edge[1]   = e1; f->edge[2]   = e2;
   f->vertex[0] = v0;  f->vertex[1] = v1; f->vertex[2] = v2;
	
   /* Link edges to face. */
   e0->adjface[0] = e1->adjface[0] = e2->adjface[0] = f;
	
   return f;
}

/*---------------------------------------------------------------------
CleanUp goes through each data structure list and clears all
flags and NULLs out some pointers.  The order of processing
(edges, faces, vertices) is important.
---------------------------------------------------------------------*/
void	CleanUp( void )
{
   CleanEdges();
   CleanFaces();
   CleanVertices();
}

void FreeDT2()
{
  tEdge e;
  tFace f;
  tVertex v;

  vnum = 0;

  e = edges;
  f = faces;

   while ( vertices ) { 
	  v = vertices;
      DT_DELETE( vertices, v);
   }

   while ( edges ) { 
	  e = edges;
      DT_DELETE( edges, e);
   }

   while ( faces ) { 
	  f = faces;
      DT_DELETE( faces, f);
   }

}


/*---------------------------------------------------------------------
CleanEdges runs through the edge list and cleans up the structure.
If there is a newface then it will put that face in place of the 
visible face and NULL out newface. It also deletes so marked edges.
---------------------------------------------------------------------*/
void	CleanEdges( void )
{
   tEdge  e;	/* Primary index into edge list. */
   tEdge  t;	/* Temporary edge pointer. */
		
   /* Integrate the newface's into the data structure. */
   /* Check every edge. */
   e = edges;
   do {
      if ( e->newface ) { 
	 if ( e->adjface[0]->visible )
	    e->adjface[0] = e->newface; 
	 else	e->adjface[1] = e->newface;
	 e->newface = NULL;
      }
      e = e->next;
   } while ( e != edges );

   /* Delete any edges marked for deletion. */
   while ( edges && edges->del ) { 
      e = edges;
      DT_DELETE( edges, e );
   }
   e = edges->next;
   do {
      if ( e->del ) {
	 t = e;
	 e = e->next;
	 DT_DELETE( edges, t );
      }
      else e = e->next;
   } while ( e != edges );
}

/*---------------------------------------------------------------------
CleanFaces runs through the face list and dels any face marked visible.
---------------------------------------------------------------------*/
void	CleanFaces( void )
{
   tFace  f;	/* Primary pointer into face list. */
   tFace  t;	/* Temporary pointer, for deleting. */
	

   while ( faces && faces->visible ) { 
      f = faces;
      DT_DELETE( faces, f );
   }
   f = faces->next;
   do {
      if ( f->visible ) {
	 t = f;
	 f = f->next;
	 DT_DELETE( faces, t );
      }
      else f = f->next;
   } while ( f != faces );
}

/*---------------------------------------------------------------------
CleanVertices runs through the vertex list and dels the 
vertices that are marked as processed but are not incident to any 
undeld edges. 
---------------------------------------------------------------------*/
void	CleanVertices( void )
{
   tEdge    e;
   tVertex  v, t;
	
   /* Mark all vertices incident to some undeld edge as on the hull. */
   e = edges;
   do {
      e->endpts[0]->onhull = e->endpts[1]->onhull = ONHULL;
      e = e->next;
   } while (e != edges);
	
   /* Delete all vertices that have been processed but
      are not on the hull. */
   while ( vertices && vertices->mark && !vertices->onhull ) { 
      v = vertices;
      DT_DELETE( vertices, v );
   }
   v = vertices->next;
   do {
      if ( v->mark && !v->onhull ) {    
	 t = v; 
	 v = v->next;
	 DT_DELETE( vertices, t )
      }
      else v = v->next;
   } while ( v != vertices );
	
   /* Reset flags. */
   v = vertices;
   do {
      v->duplicate = NULL; 
      v->onhull = !ONHULL; 
      v = v->next;
   } while ( v != vertices );
}

/*---------------------------------------------------------------------
Collinear checks to see if the three points given are collinear,
by checking to see if each element of the cross product is zero.
---------------------------------------------------------------------*/
bool	Collinear( tVertex a, tVertex b, tVertex c )
{
   return 
         ( c->v[Z] - a->v[Z] ) * ( b->v[Y] - a->v[Y] ) -
         ( b->v[Z] - a->v[Z] ) * ( c->v[Y] - a->v[Y] ) == 0
      && ( b->v[Z] - a->v[Z] ) * ( c->v[X] - a->v[X] ) -
         ( b->v[X] - a->v[X] ) * ( c->v[Z] - a->v[Z] ) == 0
      && ( b->v[X] - a->v[X] ) * ( c->v[Y] - a->v[Y] ) -
         ( b->v[Y] - a->v[Y] ) * ( c->v[X] - a->v[X] ) == 0  ;
}

/*---------------------------------------------------------------------
Computes the z-coordinate of the vector normal to face f.
---------------------------------------------------------------------*/
int	Normz( tFace f )
{
   tVertex a, b, c;
   /*double ba0, ca1, ba1, ca0,z;*/

   a = f->vertex[0];
   b = f->vertex[1];
   c = f->vertex[2];

/*
   ba0 = ( b->v[X] - a->v[X] );
   ca1 = ( c->v[Y] - a->v[Y] );
   ba1 = ( b->v[Y] - a->v[Y] );
   ca0 = ( c->v[X] - a->v[X] );

   z = ba0 * ca1 - ba1 * ca0; 
   printf("Normz = %lf=%g\n", z,z);
   if      ( z > 0.0 )  return  1;
   else if ( z < 0.0 )  return -1;
   else                 return  0;
*/
   return 
      ( b->v[X] - a->v[X] ) * ( c->v[Y] - a->v[Y] ) -
      ( b->v[Y] - a->v[Y] ) * ( c->v[X] - a->v[X] );
}

/*---------------------------------------------------------------------
Consistency runs through the edge list and checks that all
adjacent faces have their endpoints in opposite order.  This verifies
that the vertices are in counterclockwise order.
---------------------------------------------------------------------*/
void	Consistency( void )
{
   register tEdge  e;
   register int    i, j;

   e = edges;

   do {
      /* find index of endpoint[0] in adjacent face[0] */
      for ( i = 0; e->adjface[0]->vertex[i] != e->endpts[0]; ++i )
	 ;
   
      /* find index of endpoint[0] in adjacent face[1] */
      for ( j = 0; e->adjface[1]->vertex[j] != e->endpts[0]; ++j )
	 ;

      /* check if the endpoints occur in opposite order */
      if ( !( e->adjface[0]->vertex[ (i+1) % 3 ] ==
	      e->adjface[1]->vertex[ (j+2) % 3 ] ||
	      e->adjface[0]->vertex[ (i+2) % 3 ] ==
	      e->adjface[1]->vertex[ (j+1) % 3 ] )  )
	 break;
      e = e->next;

   } while ( e != edges );

   if ( e != edges )
      fprintf( stderr, "Checks: edges are NOT consistent.\n");
   else
      fprintf( stderr, "Checks: edges consistent.\n");

}

/*---------------------------------------------------------------------
Convexity checks that the volume between every face and every
point is negative.  This shows that each point is inside every face
and therefore the hull is convex.
---------------------------------------------------------------------*/
void	Convexity( void )
{
   register tFace    f;
   register tVertex  v;
   int               vol;

   f = faces;
   
   do {
      v = vertices;
      do {
	 if ( v->mark ) {
	    vol = VolumeSign( f, v );
	    if ( vol < 0 )
	       break;
	 }
	 v = v->next;
      } while ( v != vertices );

      f = f->next;

   } while ( f != faces );

   if ( f != faces )
      fprintf( stderr, "Checks: NOT convex.\n");
   else if ( check ) 
      fprintf( stderr, "Checks: convex.\n");
}

/*---------------------------------------------------------------------
CheckEuler checks Euler's relation, as well as its implications when
all faces are known to be triangles.  Only prints positive information
when debug is true, but always prints negative information.
---------------------------------------------------------------------*/
void	CheckEuler( int V, int E, int F )
{
   if ( check )
      fprintf( stderr, "Checks: V, E, F = %d %d %d:\t", V, E, F);

   if ( (V - E + F) != 2 )
      fprintf( stderr, "Checks: V-E+F != 2\n");
   else if ( check )
      fprintf( stderr, "V-E+F = 2\t");


   if ( F != (2 * V - 4) )
      fprintf( stderr, "Checks: F=%d != 2V-4=%d; V=%d\n",
	      F, 2*V-4, V);
   else if ( check ) 
      fprintf( stderr, "F = 2V-4\t");
   
   if ( (2 * E) != (3 * F) )
      fprintf( stderr, "Checks: 2E=%d != 3F=%d; E=%d, F=%d\n",
	      2*E, 3*F, E, F );
   else if ( check ) 
      fprintf( stderr, "2E = 3F\n");
}

/*-------------------------------------------------------------------*/
void	Checks( void )
{
   tVertex  v;
   tEdge    e;
   tFace    f;
   int 	   V = 0, E = 0 , F = 0;

   Consistency();
   Convexity();
   if ( v = vertices )
      do {
         if (v->mark) V++;
	 v = v->next;
      } while ( v != vertices );
   if ( e = edges )
      do {
         E++;
	 e = e->next;
      } while ( e != edges );
   if ( f = faces )
      do {
         F++;
	 f  = f ->next;
      } while ( f  != faces );
   CheckEuler( V, E, F );
}
