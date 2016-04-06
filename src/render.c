// NOTE: some scraps of code in this file came from
//  various "hello triangle" tutorials

// Includes & Window Vars
//{{{
#include <math.h>

#ifndef EMSCRIPTEN
	// Native
	#include <GL/glew.h> // should be first FLAG
	#include <GLES2/gl2.h>
	#include <SDL2/SDL.h>
#else
	// Web
	#include <stdlib.h>
	#include <SDL.h>
	#include <GLES2/gl2.h>
	#include <emscripten.h>
#endif

#define RENDER_C
#include "globals.h"
#include "matrix.h"
#include "thread.h"
#include "ui.h"
#include "render.h"
#include "intmath.h"
#include "debug.h"
//}}}


// Model Functions
//{{{


 // Rawmodel Free
//{{{
void render_rmfree(rawmodel* r)
{
	free(r->verts);
	free(r->indis);
	free(r);
}
//}}}

 // Generate Octohedron rawmodel
//{{{
rawmodel* render_rmgenocto()
{
	rawmodel* r;

	r = (rawmodel*)malloc(sizeof(rawmodel));

	r->vertslen = 6*3;
	r->verts = (GLfloat*)malloc(sizeof(GLfloat)*r->vertslen);
	r->verts[0]= 1.0f; r->verts[1]= 0.0f; r->verts[2]= 0.0f;
	r->verts[3]= 0.0f; r->verts[4]= 1.0f; r->verts[5]= 0.0f;
	r->verts[6]=-1.0f; r->verts[7]= 0.0f; r->verts[8]= 0.0f;
	r->verts[9]= 0.0f;r->verts[10]=-1.0f;r->verts[11]= 0.0f;
	r->verts[12]=0.0f;r->verts[13]= 0.0f;r->verts[14]= 1.0f;
	r->verts[15]=0.0f;r->verts[16]= 0.0f;r->verts[17]=-1.0f;

	r->indislen = 8*3;
	r->indis = (GLuint*)malloc(sizeof(GLuint)*r->indislen);
	r->indis[0] = 0;r->indis[1] = 1;r->indis[2] = 4;
	r->indis[3] = 1;r->indis[4] = 2;r->indis[5] = 4;
	r->indis[6] = 2;r->indis[7] = 3;r->indis[8] = 4;
	r->indis[9] = 0;r->indis[10]= 4;r->indis[11]= 3;
	r->indis[12]= 0;r->indis[13]= 5;r->indis[14]= 1;
	r->indis[15]= 1;r->indis[16]= 5;r->indis[17]= 2;
	r->indis[18]= 2;r->indis[19]= 5;r->indis[20]= 3;
	r->indis[21]= 0;r->indis[22]= 3;r->indis[23]= 5;

	return r;
}
//}}}

 // Genenerate Tetrahedron rawmodel
//{{{
rawmodel* render_rmgentetra()
{
	rawmodel* r;

	r = (rawmodel*)malloc(sizeof(rawmodel));

	// following coords: 0,0,1, sqrt(2)*2/3,0,-1/3, -sqrt(2)/3,+-sqrt(2/3),-1/3
	r->vertslen = 4*3;
	r->verts = (GLfloat*)malloc(sizeof(GLfloat)*r->vertslen);
	r->verts[0]=0.0f; r->verts[1]=0.0f; r->verts[2]=1.0f;
	r->verts[3]=0.94280904158f; r->verts[4]=0.0f; r->verts[5]=-0.33333333333f;
	r->verts[6]=-0.47140452079f; r->verts[7]=0.81649658092f; r->verts[8]=-0.33333333333f;
	r->verts[9]=-0.47140452079f; r->verts[10]=-0.81649658092f; r->verts[11]=-0.33333333333f;

	r->indislen = 4*3;
	r->indis = (GLuint*)malloc(sizeof(GLuint)*r->indislen);
	r->indis[0] = 0;r->indis[1] = 1;r->indis[2] = 2;
	r->indis[3] = 0;r->indis[4] = 3;r->indis[5] = 1;
	r->indis[6] = 0;r->indis[7] = 2;r->indis[8] = 3;
	r->indis[9] = 1;r->indis[10]= 3;r->indis[11]= 2;
	/*
	r->vertslen = 3*3;
	r->verts = (GLfloat*)malloc(sizeof(GLfloat)*r->vertslen);
	r->verts[0]=1.0f; r->verts[1]=0.0f; r->verts[2]=0.0f;
	r->verts[3]=-0.5f;r->verts[4]=0.86602540378f; r->verts[5]=0.0f;
	r->verts[6]=-0.5f;r->verts[7]=-0.86602540378f; r->verts[8]=0.0f;

	r->indislen = 3;
	r->indis = (GLuint*)malloc(sizeof(GLuint)*r->indislen);
	r->indis[0] = 0;r->indis[1] = 1;r->indis[2] = 2;
	*/

	return r;
}
//}}}

 // Transform rawmodel
//{{{
void render_rmtransform(rawmodel* r, matrix mat)
{
	// transforms rawmodel vertices via matrix

	int i;
	float x,y,z;
	int v = r->vertslen;

	for(i=0;i<v;i+=3)
	{
		x = r->verts[i];
		y = r->verts[i+1];
		z = r->verts[i+2];
		r->verts[i]   = mat.cell[0]*x + mat.cell[4]*y + mat.cell[8]*z  + mat.cell[12];
		r->verts[i+1] = mat.cell[1]*x + mat.cell[5]*y + mat.cell[9]*z  + mat.cell[13];
		r->verts[i+2] = mat.cell[2]*x + mat.cell[6]*y + mat.cell[10]*z + mat.cell[14];
	}

}
//}}}

 // Subdivide rawmodel
//{{{ //FLAGTE free mallocs you stupid fuck!!!!!!!!!!!!!!!!
void render_rmsubdivide(rawmodel* r)
{

	// Structs
	//{{{
	typedef struct edge
	{
		int i0;
		int i1;
		int in;
		struct edge* next;
	} edge;
	typedef struct tri
	{
		int i0;
		int i1;
		int i2;
		edge* e[3];
		struct tri* next;
	} tri;
	//}}}

	// Variables
	GLfloat* verts = r->verts;
	int vertslen = r->vertslen;
	GLuint* indis = r->indis;
	int indislen = r->indislen;
	GLfloat* newverts;
	int newvertslen;
	GLuint* newindis;
	int newindislen;

	edge* elist = 0;
	edge** eltail = &elist;
	int edgecount;
	tri* tlist = 0;
	tri** tltail = &tlist;

	// Generate tri and edge lists
	{
		// temp vars
		int i, j;
		int ind[3];	// indices for a triangle, 0 1 2
		int is, ie;	// indices for an edge, start end (start always lower of the two)
		edge* cedge;
		tri* ctri;

		edgecount = 0;

		for(i=0;i<indislen;i+=3)
		{
			*tltail = ctri = (tri*)malloc(sizeof(tri));
			tltail = &(ctri->next);
			ctri->next = 0;
			ctri->i0 = ind[0] = indis[i];
			ctri->i1 = ind[1] = indis[i+1];
			ctri->i2 = ind[2] = indis[i+2];

			for(j=0;j<3;j++)
			{
				is = ind[j];
				ie = ind[(j+1)%3];
				if(is>ie){is^=ie;ie^=is;is^=ie;}

				// Check if edge is already in list
				cedge = elist;
				while(cedge)
				{
					if(cedge->i0==is)
					{
						if(cedge->i1==ie)
						{
							goto edgeexists;
						}
					}
					cedge = cedge->next;
				}
				// Add to list (skipped if already there)
				*eltail = cedge = (edge*)malloc(sizeof(edge));
				eltail = &(cedge->next);
				cedge->next = 0;
				cedge->i0 = is;
				cedge->i1 = ie;
				edgecount++;

				edgeexists:
				ctri->e[j] = cedge;
			}
		}

	}

	// Create new vertarray
	{
		int i;
		GLfloat v0;
		GLfloat v1;
		int i0;
		int i1;
		edge* cedge;

		newvertslen = vertslen + 3*edgecount;
		newverts = (GLfloat*)malloc(sizeof(GLfloat)*newvertslen);

		for(i=0;i<vertslen;i++)
		{
			newverts[i] = verts[i];
		}
		cedge = elist;
		while(cedge)
		{
			i0 = 3*(cedge->i0);
			i1 = 3*(cedge->i1);
			v0 = verts[i0];
			v1 = verts[i1];
			newverts[i] = (v0+v1)*0.5f;
			v0 = verts[i0+1];
			v1 = verts[i1+1];
			newverts[i+1] = (v0+v1)*0.5f;
			v0 = verts[i0+2];
			v1 = verts[i1+2];
			newverts[i+2] = (v0+v1)*0.5f;

			cedge->in = i/3;

			i+=3;
			cedge = cedge->next;
		}

	}

	// Generate new tri/indices list
	{
		int i;
		int i0,i1,i2,i3,i4,i5;
		tri* ctri = tlist;

		newindislen = indislen*4;
		newindis = (GLuint*)malloc(sizeof(GLuint)*newindislen);

		i=0;
		while(ctri)
		{
			i0 = ctri->i0;
			i1 = ctri->i1;
			i2 = ctri->i2;
			i3 = ctri->e[0]->in;
			i4 = ctri->e[1]->in;
			i5 = ctri->e[2]->in;

			newindis[0+i] =i0;newindis[1+i] =i3;newindis[2+i] =i5;
			newindis[3+i] =i1;newindis[4+i] =i4;newindis[5+i] =i3;
			newindis[6+i] =i2;newindis[7+i] =i5;newindis[8+i] =i4;
			newindis[9+i] =i3;newindis[10+i]=i4;newindis[11+i]=i5;
			i+=12;

			ctri = ctri->next;
		}
	}

	// cleanup, free old verts and indis
	r->verts = newverts;
	r->indis = newindis;
	r->indislen = newindislen;
	r->vertslen = newvertslen;
	free(verts);
	free(indis);

	// Print Tris and Edges
	/*
	{
		edge* cedge = elist;
		tri* ctri = tlist;
		int i;

		i=0;
		printf("Edges:\n");
		while(cedge)
		{
			printf("e%d:  %d, %d\n",i++,cedge->i0,cedge->i1);
			cedge = cedge->next;
		}
		i=0;
		printf("Tris:\n");
		while(ctri)
		{
			printf("t%d: %d, %d, %d, - %d %d, %d %d, %d %d\n",i++,
					ctri->i0,ctri->i1,ctri->i2,
					ctri->e[0]->i0,ctri->e[0]->i1,
					ctri->e[1]->i0,ctri->e[1]->i1,
					ctri->e[2]->i0,ctri->e[2]->i1);
			ctri = ctri->next;
		}
	}
	*/

}
//}}}

 // Normalize rawmodel
//{{{
void render_rmnormalize(rawmodel* r)
{
	int i;
	GLfloat vx,vy,vz,t;
	GLfloat* verts = r->verts;
	int v = r->vertslen;

	for(i=0;i<v;i+=3)
	{
		vx = verts[i];
		vy = verts[i+1];
		vz = verts[i+2];
		t = sqrt(vx*vx + vy*vy + vz*vz);
		t = (t>0)? 1/t : 0;
		verts[i]  = vx*t;
		verts[i+1]= vy*t;
		verts[i+2]= vz*t;
	}
}
//}}}

 // Print rawmodel
//{{{
void render_rmprint(rawmodel* r)
{
	int i;
	int v = r->vertslen;
	int s = r->indislen;

	debug_message("model print:\n");
	debug_message("vertslen: %d\nindislen: %d\nverts:\n", v, s);
	for(i=0;i<v;i++)
	{
		debug_message("%f,", r->verts[i]);
		if((i%3)==2)
			debug_message("\n");
	}
	debug_message("indis:\n");
	for(i=0;i<s;i++)
	{
		debug_message("%d,", r->indis[i]);
		if((i%3)==2)
			debug_message("\n");
	}
	debug_message("model print complete.\n");
}
//}}}


 // Bake rawmodel into model
//{{{
model render_rmbake(rawmodel* r)
{
	// Sends rawmodel data to GPU and store indexes for GPU buffers in model struct
	//  and frees the rawmodel

	model m;

	m.IBOlen = r->indislen;

	// Generate buffers
	glGenBuffers(1, &(m.VBO)); // Create new pointer to (yet unmade) buffer
	glBindBuffer(GL_ARRAY_BUFFER, m.VBO); // Establish buffer type for pointer
	glBufferData(GL_ARRAY_BUFFER,
			r->vertslen * sizeof(GLfloat),
			r->verts,
			GL_STATIC_DRAW); // Create buffer
	glGenBuffers(1, &(m.IBO));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			m.IBOlen * sizeof(GLuint),
			r->indis,
			GL_STATIC_DRAW);

	render_rmfree(r);

	return m;
}
//}}}

 // Draw model
//{{{
void render_drawmodel(model m, matrix mat, float red, float green, float blue)
// INPUT CHANGED FOR DEMO
{
	// FLAG take program as argument?
	//  make "model program" struct for all model rendering programs?

	glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
	glVertexAttribPointer(objvertpointer, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.IBO);

	glUniformMatrix4fv(objmatrixpointer, 1, 0, mat.cell);

#ifndef EMSCRIPTEN // FLAG put in debug option??? do wireframes with model struct too
	/*
	glUniform3f(objcolorpointer, 0.0f, 1.0f, 0.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawElements(GL_TRIANGLES, m.IBOlen, GL_UNSIGNED_INT, 0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	*/
#endif
	glUniform3f(objcolorpointer, red, green, blue);//DEMO
	glDrawElements(GL_TRIANGLES, m.IBOlen, GL_UNSIGNED_INT, 0);
}
//}}}

 // Free model
//{{{
void render_freemodelbuffers(model m)
{
	glDeleteBuffers(1,&(m.VBO));
	glDeleteBuffers(1,&(m.IBO));
}
//}}}

 // Model init
//{{{
void render_modelinit()
{
	/*
	   Note: This function runs in the main thread, not the UI thread, during init
	    phase when assets are being loaded.
	   */

	modellistcount = RE_MODELCOUNT;
	modellist = (model*)malloc(sizeof(model)*modellistcount);
	
	// Vulcan bullet
	{
		rawmodel* rtetra = render_rmgenocto();
		render_rmsubdivide(rtetra);
		render_rmnormalize(rtetra);
		render_rmsubdivide(rtetra);
		render_rmnormalize(rtetra);
		matrix mat;
		matrix_identity(&mat);
		matrix_scale(&mat,0.25f,0.25f,0.25f);
		render_rmtransform(rtetra, mat);
		modellist[RE_MBULLET] = render_rmbake(rtetra);
	}

	// Charge gun thing
	{
		rawmodel* rtetra = render_rmgenocto();
		render_rmsubdivide(rtetra);
		render_rmnormalize(rtetra);
		render_rmsubdivide(rtetra);
		render_rmnormalize(rtetra);
		render_rmsubdivide(rtetra);
		render_rmnormalize(rtetra);
		modellist[RE_MCHARGE] = render_rmbake(rtetra);
	}

	// Rifleman
	{
		rawmodel* rm = render_rmgenocto();
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		matrix mat;
		matrix_identity(&mat);
		matrix_translate(&mat,0,0,1.0f);
		matrix_scale(&mat,0.5f,0.5f,1.0f);
		render_rmtransform(rm,mat);
		modellist[RE_MRIFLEMAN] = render_rmbake(rm);
	}
	// Tank
	{
		rawmodel* rm = render_rmgenocto();
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		matrix mat;
		matrix_identity(&mat);
		matrix_translate(&mat,0.0f,0.0f,1.25f);
		matrix_scale(&mat,2.5f,1.5f,1.25f);
		render_rmtransform(rm,mat);
		modellist[RE_MTANK] = render_rmbake(rm);
	}
	// Jeep
	{
		rawmodel* rm = render_rmgenocto();
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		matrix mat;
		matrix_identity(&mat);
		matrix_translate(&mat,-1.0f,0.0f,1.0f);
		matrix_scale(&mat,2.0f,1.0f,1.0f);
		render_rmtransform(rm,mat);
		modellist[RE_MJEEP] = render_rmbake(rm);
	}
	// Missle
	{
		rawmodel* rm = render_rmgenocto();
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		matrix mat;
		matrix_identity(&mat);
		matrix_translate(&mat,0.5f,0.0f,0.0f);
		matrix_scale(&mat,0.75f,0.25f,0.25f);
		render_rmtransform(rm,mat);
		modellist[RE_MAAMISSLE] = render_rmbake(rm);
	}
	// Monolith
	{
		rawmodel* rm = render_rmgenocto();
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		matrix mat;
		matrix_identity(&mat);
		matrix_translate(&mat,0.f,0.0f,3.0f);
		matrix_scale(&mat,3.0f,2.0f,10.0);
		render_rmtransform(rm,mat);
		modellist[RE_MMONOLITH] = render_rmbake(rm);
	}

	// Temp sphere for gas
	{
		//xx
		rawmodel* rm = render_rmgenocto();
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		render_rmsubdivide(rm);
		render_rmnormalize(rm);
		modellist[RE_MTEMPGAS] = render_rmbake(rm);
	}

	// crosshair tick FLAGTE make seperate model list for HUD elements?
	rawmodel* t = (rawmodel*)malloc(sizeof(rawmodel));
	t->verts = (GLfloat*)malloc(sizeof(GLfloat)*36);
	t->indis = (GLuint*)malloc(sizeof(GLuint)*12);
	t->vertslen = 36;
	t->indislen = 12;
	int i = 0;
	float w = .02f;
	float h = .07f;
	float s = .035f;
	t->verts[i++]= w;t->verts[i++]=h+s;t->verts[i++]=0;
	t->verts[i++]=-w;t->verts[i++]=h+s;t->verts[i++]=0;
	t->verts[i++]= 0;t->verts[i++]=0+s;t->verts[i++]=0;
	t->verts[i++]=-w;t->verts[i++]=-h-s;t->verts[i++]=0;
	t->verts[i++]= w;t->verts[i++]=-h-s;t->verts[i++]=0;
	t->verts[i++]= 0;t->verts[i++]= 0-s;t->verts[i++]=0;
	t->verts[i++]= h+s;t->verts[i++]=-w;t->verts[i++]=0;
	t->verts[i++]= h+s;t->verts[i++]= w;t->verts[i++]=0;
	t->verts[i++]= 0+s;t->verts[i++]= 0;t->verts[i++]=0;
	t->verts[i++]=-h-s;t->verts[i++]= w;t->verts[i++]=0;
	t->verts[i++]=-h-s;t->verts[i++]=-w;t->verts[i++]=0;
	t->verts[i++]= 0-s;t->verts[i++]= 0;t->verts[i++]=0;
	i=0;
	t->indis[i]=i;i++;
	t->indis[i]=i;i++;
	t->indis[i]=i;i++;
	t->indis[i]=i;i++;
	t->indis[i]=i;i++;
	t->indis[i]=i;i++;
	t->indis[i]=i;i++;
	t->indis[i]=i;i++;
	t->indis[i]=i;i++;
	t->indis[i]=i;i++;
	t->indis[i]=i;i++;
	t->indis[i]=i;i++;
	modellist[RE_MWOOPS] = render_rmbake(t);

	// Generate Test rectangle for HUD
	{
		GLfloat verts[20]; // 5 floats per vertex
		GLuint indis[6];

		/* FLAGTE delete
		verts[0] = 1.0f; verts[1] = 1.0f; verts[2] = 0.0f;
		verts[3] =-1.0f; verts[4] = 1.0f; verts[5] = 0.0f;
		verts[6] =-1.0f; verts[7] =-1.0f; verts[8] = 0.0f;
		verts[9] = 1.0f; verts[10]=-1.0f; verts[11]= 0.0f;
		*/

		verts[0] = 1.0f;verts[1] = 1.0f;verts[2] =0.0f;verts[3] =1.0f;verts[4] =1.0f;
		verts[5] =-1.0f;verts[6] = 1.0f;verts[7] =0.0f;verts[8] =0.0f;verts[9] =1.0f;
		verts[10]=-1.0f;verts[11]=-1.0f;verts[12]=0.0f;verts[13]=0.0f;verts[14]=0.0f;
		verts[15]= 1.0f;verts[16]=-1.0f;verts[17]=0.0f;verts[18]=1.0f;verts[19]=0.0f;

		indis[0] = 0; indis[1] = 1; indis[2] = 2;
		indis[3] = 2; indis[4] = 3; indis[5] = 0;

		// Generate buffers
		glGenBuffers(1, &(testrectVBO)); // Create new pointer to (yet unmade) buffer
		glBindBuffer(GL_ARRAY_BUFFER, testrectVBO); // Establish buffer type for pointer
		glBufferData(GL_ARRAY_BUFFER,
				20 * sizeof(GLfloat),
				verts,
				GL_STATIC_DRAW); // Create buffer
		glGenBuffers(1, &(testrectIBO));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, testrectIBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				6 * sizeof(GLuint),
				indis,
				GL_STATIC_DRAW);

	}

	// Generate test texture for HUD
	{
		// Reference:
		//  http://www.opengl-tutorial.org/beginners-tutorials/tutorial-5-a-textured-cube/
		int w = 64;
		int h = 64;
		unsigned char data[4*w*h];
		{
			int i,j;
			for(j=0;j<h;j++)
			{
				for(i=0;i<w;i++)
				{
					int dx = w/2 - i;
					int dy = h/2 - j;
					data[4*(i+w*j)+0] = 255-255*(dx*dx+dy*dy)/(w*w/4);
					data[4*(i+w*j)+1] = i*255/w;
					data[4*(i+w*j)+2] = j*255/h;
					data[4*(i+w*j)+3] = 255-255*(dx*dx+dy*dy)/(w*w/4);
				}
			}
		}

		/*
		data[0] = 255; data[1] =   0; data[2] =   0; data[3] = 255;
		data[4] =   0; data[5] = 255; data[6] =   0; data[7] = 200;
		data[8] =   0; data[9] =   0; data[10]= 255; data[11]= 255;
		data[12]= 255; data[13]= 100; data[14]=   0; data[15]=  55;
		*/

		glGenTextures(1,&testtextureID);
		glBindTexture(GL_TEXTURE_2D, testtextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

}
//}}}


//}}}

// Render Functions
//{{{

void render_generateprojectionmatrix(matrix* mat)
{
	float* c = mat->cell;

	float near;		// distance of inner clipping plane
	float far;		// distance of outter clipping plane
	float tangent;		// tangent of view angle
	float widthratio;	// ratio of width to proper square screen
	float heightratio;	// ratio of height to proper square screen

	// "tangent" gives how wide a square screen would be if the screen
	//  were 1 unit of distance from the eye.
	// "widthratio" and "heightratio" give how wide or high the screen
	//  is in relation to the above described square screen.
	// Either widthratio or heighratio will be equal to 1, depending
	//  on the dimensions of the screen.
	
	near = ui_clipdistancenear;
	far = ui_clipdistancefar;
	tangent = (float)g_viewangletangent/((float)UPM);
	if(g_windowwidth>g_windowheight)
	{
		widthratio = ((float)g_windowwidth)/g_windowheight;
		heightratio = 1.0f;
	}
	else
	{
		heightratio = ((float)g_windowheight)/g_windowwidth;
		widthratio = 1.0f;
	}

	c[0]  = 1.0f/tangent/widthratio;
	c[1]  = 0.0f;
	c[2]  = 0.0f;
	c[3]  = 0.0f;
	c[4]  = 0.0f;
	c[5]  = 1.0f/tangent/heightratio;
	c[6]  = 0.0f;
	c[7]  = 0.0f;
	c[8]  = 0.0f;
	c[9]  = 0.0f;
	c[10] = (far+near)/(near-far);
	c[11] = -1.0f;
	c[12] = 0.0f;
	c[13] = 0.0f;
	c[14] = 2.0f*far*near/(near-far);
	c[15] = 0.0f;
}
void render_printmatrix(matrix* mat) // FLAG use this? erase?
{

	debug_message("Matrix:\n");
	int i,j;
	for(i=0;i<4;i++)
	{
		for(j=0;j<4;j++)
		{
			debug_message(" %2.3f",mat->cell[i+4*j]);
		}
		debug_message("\n");
	}
}

void render_resize(int w, int h)
{
	g_windowwidth = w;
	g_windowheight = h;
	//glViewport(0,0,w,h);
}

void printprogramlog(GLuint program)
{
	if(!glIsProgram)
	{
		debug_errormessage("Invalid Program Passed.\n");
		return;
	}
	int maxlen;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxlen);
	char* infolog = (char*)malloc(sizeof(char)*maxlen);
	glGetProgramInfoLog(program, maxlen, 0, infolog);
	debug_errormessage(infolog);
	free(infolog);
}

void printshaderlog(GLuint shader)
{
	if(!glIsShader(shader))
	{
		debug_errormessage("Invalid Shader Passed.\n");
		return;
	}

	int maxlen;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxlen);
	char* infolog = (char*)malloc(sizeof(char)*maxlen);
	glGetShaderInfoLog(shader, maxlen, 0, infolog);
	debug_errormessage(infolog);
	free(infolog);

}

//}}}

// Render Gamephase Functions
//{{{

 // Init
//{{{
void render_init()
{
	// NOTE FLAG this is suuuuper temporary, just works atm, organize better
	//  and put into various functions and shit where necessary

	// Init Video
	if(SDL_Init(SDL_INIT_VIDEO))
	{
		debug_errormessage("Unable to initialize SDL: %s\n",SDL_GetError());
		exit(1);
	}


	// Create Window, and setup OpenGL
#ifndef EMSCRIPTEN
	{
		// Set window dimensions
		SDL_DisplayMode dm;
		if(SDL_GetDesktopDisplayMode(0,&dm))
		{
			debug_errormessage("SDL_GetDesktopDisplayMode failed: %s",SDL_GetError());
			exit(1);
		}
		g_windowwidth = dm.w;
		g_windowheight = dm.h;
		g_windowsquare = (dm.w>dm.h)?dm.h:dm.w;

		// temp code for resizable
		int w = 600;
		int h = 400;
		g_windowwidth = w;
		g_windowheight = h;
		g_windowsquare = (w>h)?h:w;
		//FLAGTE clean this up and consider globals.c
	}
	window = SDL_CreateWindow(
			"Test",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			g_windowwidth,
			g_windowheight,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
			//SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP
			//SDL_WINDOW_OPENGL
			);
	glcontext = SDL_GL_CreateContext(window);
	if(!glcontext)
	{
		debug_errormessage("GL Context creation failed.\n");
		exit(1);
	}
	SDL_GL_MakeCurrent(window, glcontext);

	// Set to use OpenGL 3.1
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// Init Glew
	{
		glewExperimental = GL_TRUE;
		GLenum g = glewInit();
		if(g)
		{
			debug_errormessage("GLEW init failure: %s\n", glewGetErrorString(g));
			exit(1);
		}
	}

	// Set swap interval
	//  (Try for late swap tearing first, if no-go then use synchronized updates)
	if(SDL_GL_SetSwapInterval(-1)==-1) SDL_GL_SetSwapInterval(1);
#else
	if(!(screen = SDL_SetVideoMode(g_windowwidth,g_windowheight,16,SDL_OPENGL | SDL_RESIZABLE)))
	{
		debug_errormessage("SDL SetVideoMode failure: ");
		exit(1);
	}
#endif
	
	// Hide Cursor
	SDL_ShowCursor(SDL_DISABLE);


	// Generate Programs
	{

		// Environment Program
		{//{{{

			GLint check;

			envprogram = glCreateProgram();

			GLuint v = glCreateShader(GL_VERTEX_SHADER);
			const GLchar* vs[1];
			vs[0] = "#version 100\n\
				precision highp float;\n\
				\n\
				uniform mat4 testmatrix;\n\
				attribute vec3 vert;\n\
				varying vec2 xy;\n\
				void main()\n\
				{\n\
					xy = vec2(vert[0],vert[1]);\n\
					gl_Position = testmatrix*vec4(vert[0], vert[1], vert[2], 1.0);\n\
				}";
			glShaderSource(v, 1, vs, 0);
			glCompileShader(v);
			check = GL_FALSE;
			glGetShaderiv(v, GL_COMPILE_STATUS, &check);
			if(check!=GL_TRUE)
			{
				debug_errormessage("Vertex Shader compilation failure: ");
				printshaderlog(v);
				exit(1);
			}
			glAttachShader(envprogram, v);

			GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
			const GLchar* fs[1];
			fs[0] = "#version 100\n\
				precision highp float;\n\
				\n\
				varying vec2 xy;\n\
				void main()\n\
				{\n\
					float c = (floor(mod(xy[0],2.0))+floor(mod(xy[1],2.0)))/2.0;\n\
					vec3 c1 = vec3(1.0, 0.7, 0.0);\n\
					vec3 c2 = vec3(0.6, 0.4, 0.1);\n\
					c1 = mix(c1, c2, c);\n\
					gl_FragColor = vec4(c1, 1.0);\n\
				}";
			/* floating point suffix unsupported????
			fs[0] = "#version 100\n\
				precision highp float;\n\
				\n\
				varying vec2 xy;\n\
				void main()\n\
				{\n\
					float c = (floor(mod(xy[0],2.0f))+floor(mod(xy[1],2.0f)))/2.0f;\n\
					vec3 c1 = vec3(1.0f, 0.8f, 0.0f);\n\
					vec3 c2 = vec3(0.5f, 0.5f, 0.0f);\n\
					c1 = mix(c1, c2, c);\n\
					gl_FragColor = vec4(c1, 1.0);\n\
				}";
				*/
			glShaderSource(f, 1, fs, 0);
			glCompileShader(f);
			check = GL_FALSE;
			glGetShaderiv(f, GL_COMPILE_STATUS, &check);
			if(check!=GL_TRUE)
			{
				debug_errormessage("Fragment Shader compilation failure: ");
				printshaderlog(f);
				exit(1);
			}
			glAttachShader(envprogram, f);

			glLinkProgram(envprogram);
			check = GL_TRUE;
			glGetProgramiv(envprogram, GL_LINK_STATUS, &check);
			if(check!=GL_TRUE)
			{
				debug_errormessage("Program Link failure: ");
				printprogramlog(envprogram);
				exit(1);
			}

			// Set pointer to "vert" location in shader
			envvertpointer = glGetAttribLocation(envprogram, "vert");
			if(envvertpointer==-1)
			{
				debug_errormessage("Invalid variable name.\n");
				exit(1);
			}
			envmatrixpointer = glGetUniformLocation(envprogram, "testmatrix");
			if(envmatrixpointer==-1)
			{
				debug_errormessage("Invalid variable name.\n");
				exit(1);
			}

		}//}}}

		// Object Program
		{//{{{

			GLint check;

			objprogram = glCreateProgram();

			GLuint v = glCreateShader(GL_VERTEX_SHADER);
			const GLchar* vs[1];
			vs[0] = "#version 100\n\
				precision highp float;\n\
				\n\
				uniform mat4 testmatrix;\n\
				attribute vec3 vert;\n\
				varying float z;\n\
				void main()\n\
				{\n\
					z = (vert[2]+0.5)/3.0;\n\
					gl_Position = testmatrix*vec4(vert[0], vert[1], vert[2], 1.0);\n\
				}";
			glShaderSource(v, 1, vs, 0);
			glCompileShader(v);
			check = GL_FALSE;
			glGetShaderiv(v, GL_COMPILE_STATUS, &check);
			if(check!=GL_TRUE)
			{
				debug_errormessage("Vertex Shader compilation failure: ");
				printshaderlog(v);
				exit(1);
			}
			glAttachShader(objprogram, v);

			GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
			const GLchar* fs[1];
			fs[0] = "#version 100\n\
				precision highp float;\n\
				\n\
				uniform vec3 color;\n\
				varying float z;\n\
				void main()\n\
				{\n\
					gl_FragColor = vec4(z*color, 1.0);\n\
				}";
			glShaderSource(f, 1, fs, 0);
			glCompileShader(f);
			check = GL_FALSE;
			glGetShaderiv(f, GL_COMPILE_STATUS, &check);
			if(check!=GL_TRUE)
			{
				debug_errormessage("Fragment Shader compilation failure: ");
				printshaderlog(f);
				exit(1);
			}
			glAttachShader(objprogram, f);

			glLinkProgram(objprogram);
			check = GL_TRUE;
			glGetProgramiv(objprogram, GL_LINK_STATUS, &check);
			if(check!=GL_TRUE)
			{
				debug_errormessage("Program Link failure: ");
				printprogramlog(objprogram);
				exit(1);
			}

			// Set pointer to "vert" location in shader
			objvertpointer = glGetAttribLocation(objprogram, "vert");
			if(objvertpointer==-1)
			{
				debug_errormessage("Invalid variable name.\n");
				exit(1);
			}
			objmatrixpointer = glGetUniformLocation(objprogram, "testmatrix");
			if(objmatrixpointer==-1)
			{
				debug_errormessage("Invalid variable name.\n");
				exit(1);
			}
			objcolorpointer = glGetUniformLocation(objprogram, "color");
			if(objmatrixpointer==-1)
			{
				debug_errormessage("Invalid variable name.\n");
				exit(1);
			}
		}//}}}

		// HUD (Text & Icon) Program
		{//{{{

			GLint check;

			hudprogram = glCreateProgram();

			GLuint v = glCreateShader(GL_VERTEX_SHADER);
			const GLchar* vs[1];
			vs[0] = "#version 100\n\
				precision highp float;\n\
				\n\
				uniform mat4 testmatrix;\n\
				attribute vec3 vert;\n\
				attribute vec2 uv;\n\
				varying vec2 ftex;\n\
				void main()\n\
				{\n\
					gl_Position = testmatrix*vec4(vert, 1.0);\n\
					ftex = uv;\n\
				}";
			glShaderSource(v, 1, vs, 0);
			glCompileShader(v);
			check = GL_FALSE;
			glGetShaderiv(v, GL_COMPILE_STATUS, &check);
			if(check!=GL_TRUE)
			{
				debug_errormessage("Vertex Shader compilation failure: ");
				printshaderlog(v);
				exit(1);
			}
			glAttachShader(hudprogram, v);

			GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
			const GLchar* fs[1];
			fs[0] = "#version 100\n\
				precision highp float;\n\
				\n\
				uniform sampler2D texturesampler;\n\
				varying vec2 ftex;\n\
				void main()\n\
				{\n\
					gl_FragColor = texture2D(texturesampler,ftex);\n\
				}";
			glShaderSource(f, 1, fs, 0);
			glCompileShader(f);
			check = GL_FALSE;
			glGetShaderiv(f, GL_COMPILE_STATUS, &check);
			if(check!=GL_TRUE)
			{
				debug_errormessage("Fragment Shader compilation failure: ");
				printshaderlog(f);
				exit(1);
			}
			glAttachShader(hudprogram, f);

			glLinkProgram(hudprogram);
			check = GL_TRUE;
			glGetProgramiv(hudprogram, GL_LINK_STATUS, &check);
			if(check!=GL_TRUE)
			{
				debug_errormessage("Program Link failure: ");
				printprogramlog(hudprogram);
				exit(1);
			}

			// Set pointer to "vert" location in shader
			hudvertpointer = glGetAttribLocation(hudprogram, "vert");
			if(hudvertpointer==-1)
			{
				debug_errormessage("Invalid variable name.\n");
				exit(1);
			}
			huduvpointer = glGetAttribLocation(hudprogram, "uv");
			if(huduvpointer==-1)
			{
				debug_errormessage("Invalid variable name uv.\n");
				exit(1);
			}
			hudmatrixpointer = glGetUniformLocation(hudprogram, "testmatrix");
			if(hudmatrixpointer==-1)
			{
				debug_errormessage("Invalid variable name.\n");
				exit(1);
			}
			hudtexturepointer = glGetUniformLocation(hudprogram, "texturesampler");
			if(hudtexturepointer==-1)
			{
				debug_errormessage("Invalid variable name.\n");
				exit(1);
			}
		}//}}}

	}


	// Create VBO and IBO
	{
		// Data to pass to buffers
		//GLfloat cpuverts[] = {1.0,-1.0,0.0, -1.0,-1.0,0.0, 0.0,1.0,0.0};
		GLfloat cpuverts[] = {-1000.0,-1000.0,0.0, 1000.0,-1000.0,0.0, 0.0,1000.0,0.0};
		GLint cpuvertcount = 3*3;
		GLuint cpuindis[] = {0,1,2};
		icount = 3;

		// Generate buffers
		glGenBuffers(1, &VBO); // Create new pointer to (yet unmade) buffer
		glBindBuffer(GL_ARRAY_BUFFER, VBO); // Establish buffer type for pointer
		glBufferData(GL_ARRAY_BUFFER,
				cpuvertcount * sizeof(GLfloat),
				cpuverts,
				GL_STATIC_DRAW); // Create buffer
		glGenBuffers(1, &IBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				icount * sizeof(GLuint),
				cpuindis,
				GL_STATIC_DRAW);
	}



}
//}}}

 // De-init
//{{{
void render_deinit()
{
	glDeleteProgram(envprogram);
	glDeleteProgram(objprogram);
	glDeleteProgram(hudprogram);
	render_freemodelbuffers(tetratest);

#ifndef EMSCRIPTEN
	SDL_DestroyWindow(window);
#endif
	//SDL_Quit();
}
//}}}

 // Test
//{{{
void render_test(int color)
{

	glClearColor( ((float)((color>>8)&0xF))/15.0, ((float)((color>>4)&0xF))/15.0, ((float)((color>>0)&0xF))/15.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

#ifndef EMSCRIPTEN
	SDL_GL_SwapWindow(window);
#else
	SDL_GL_SwapBuffers();
#endif

}
//}}}

 // Shop
//{{{
void render_shop()
{

	float mat[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

	glClearColor( 1.0, 0.5, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(objprogram);

	glEnableVertexAttribArray(objvertpointer); // Use verices for draw calls
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(objvertpointer, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), 0);

	glUniformMatrix4fv(objmatrixpointer, 1, 0, mat);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glDrawElements(GL_TRIANGLES, icount, GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(objvertpointer);

	glUseProgram(0);

#ifndef EMSCRIPTEN
	SDL_GL_SwapWindow(window);
#else
	SDL_GL_SwapBuffers();
#endif

}
//}}}

 // Battle
//{{{
void render_battle(battlerenderbuffer* brb, float time, intv mousex, intv mousey, effectdata* e)
{

	//// Variables
	matrix cammatrix;
	matrix rotmatrix;
	matrix projmatrix;

	debug_benchstart("Render");

	// Generate projection matrix
	render_generateprojectionmatrix(&projmatrix);

	// Set matrix to projection and camera (NOT translated by camera offset)
	{
		int theta, phi;
		int c,s;
		float fc,fs;
		matrix_identity(&rotmatrix);
		theta = brb->camtheta;
		phi = brb->camphi;
		ANGTOCS(-PI2-phi,c,s)
		fc = (float)c/((float)UPM); // FLAGTE replace with intmath matrix operations
		fs = (float)s/((float)UPM);
		matrix_rotatex(&rotmatrix,fc,fs);
		ANGTOCS(PI2-theta,c,s)
		fc = (float)c/((float)UPM);
		fs = (float)s/((float)UPM);
		matrix_rotatez(&rotmatrix,fc,fs);
		//matrix_translate(&cammatrix,-brb->camx,-brb->camy,-brb->camz); FLAGTE delete
		matrix_multiply(&cammatrix,&projmatrix,&rotmatrix);
	}

	// Clear Buffers
	glClearColor( 0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	debug_benchstart("Update");

	// Enable Depth testing and backface culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Draw Environment
	//{{{
	{
		matrix Tmatrix = cammatrix; // FLAGTE delete once env shader rewritten
		matrix_translate(&Tmatrix,-brb->camx,-brb->camy,-brb->camz);

		glUseProgram(envprogram);

		glEnableVertexAttribArray(envvertpointer); // pointer in env prog to incoming verts
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glVertexAttribPointer(envvertpointer, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

		glUniformMatrix4fv(envmatrixpointer, 1, 0, Tmatrix.cell);

		glDrawElements(GL_TRIANGLES, icount, GL_UNSIGNED_INT, 0);

		glDisableVertexAttribArray(envvertpointer);
	}
	//}}}

	// Draw Objects
	//{{{
	{
		glUseProgram(objprogram);

		glEnableVertexAttribArray(objvertpointer); // pointer in env prog to incoming verts

		int i;
		int oc = brb->renderobjectcount;
		int m;
		int c, s;
		float fc, fs;
		matrix mat;
		for(i=0;i<oc;i++)
		{
			m = brb->romodelnum[i];
			if(m<0||m>=modellistcount)
			{
				//FLAGTE make model 0 a big purple cube, and m=0;
			}


			mat = cammatrix;
			matrix_translate(&mat,
					((float)brb->ropx[i])/UPM,
					((float)brb->ropy[i])/UPM,
					((float)brb->ropz[i])/UPM);
			ANGTOCS(brb->rotheta[i],c,s)
			fc = (float)c/((float)UPM);
			fs = (float)s/((float)UPM);
			matrix_rotatez(&mat,fc,fs);
			ANGTOCS(-brb->rophi[i],c,s)
			fc = (float)c/((float)UPM);
			fs = (float)s/((float)UPM);
			matrix_rotatey(&mat,fc,fs);
			/*
			// Interpolate matrices FLAGTE
			smat = brb->rosmatrix[i];
			// Rotation
			mat.cell[0]=smat.cell[0];mat.cell[4]=smat.cell[3];mat.cell[8]=smat.cell[6];
			mat.cell[1]=smat.cell[1];mat.cell[5]=smat.cell[4];mat.cell[9]=smat.cell[7];
			mat.cell[2]=smat.cell[2];mat.cell[6]=smat.cell[5];mat.cell[10]=smat.cell[8];
			// Translation
			mat.cell[12]=smat.cell[9];
			mat.cell[13]=smat.cell[10];
			mat.cell[14]=smat.cell[11];
			// Bottom Row
			mat.cell[3] = 0.0f;
			mat.cell[7] = 0.0f;
			mat.cell[11] = 0.0f;
			mat.cell[15] = 1.0f;
			*/

			//matrix_multiply(&mat, &cammatrix, &mat);
			switch(m)
			{
				case RE_MRIFLEMAN:
					render_drawmodel(modellist[m], mat, 0.0,1.0,0.0);
					break;
				case RE_MJEEP:
					render_drawmodel(modellist[m], mat, 0.5,0.8,0.0);
					break;
				case RE_MTANK:
					render_drawmodel(modellist[m], mat, 0.2,0.3,0.1);
					break;
				case RE_MBULLET:
					render_drawmodel(modellist[m], mat, 1.0,0.9,0.7);
					break;
				case RE_MTEMPGAS:
					render_drawmodel(modellist[m], mat, 1.0,1.0,1.0);
					break;
				default:
					render_drawmodel(modellist[m], mat, 1.0,1.0,1.0);
					break;
			}
		}

		glDisableVertexAttribArray(objvertpointer);
		
	}
	//}}}

	// Draw Effects
	//{{{
	{

		// FLAGTE NOTE
		// Eventually replace this with call to gas shader, and have model structs
		//  specific to use with that shader

		/*
		if(ec>0){ // DELETE when removing messages
		debug_message("Effects: ");
		for(i=0;i<ec;i++)
		{
			debug_message("%d:%d ", e->chunktype[i], e->chunklife[i]);
		}
		debug_message("\n");}
		*/

		glUseProgram(objprogram);

		glEnableVertexAttribArray(objvertpointer); // pointer in env prog to incoming verts
		
		int i;
		int ec = e->chunkcount;

		for(i=0;i<ec;i++)
		{
			int m = RE_MTEMPGAS;

			float size = 2;
			float r = 0;
			float g = 1;
			float b = 1;
			if(e->chunktype[i] == 1)
			{
				size = ((float)e->chunklife[i])/(TPS/4);
				size = 2.0*size - 1.0;
				size = 4.0*(1.0 - size*size);
				r = 1.0;
				g = 0.0;
				b = 0.0;
				e->chunkpz[i] += 8*UPM/TPS;
			}
			if(e->chunktype[i] == 2)
			{
				size = ((float)e->chunklife[i])/(TPS/4);
				size = 1.0*size - 1.0;
				size = 0.5*(1.0 - size*size);
				r = 1.0;
				g = 1.0;
				b = 0.0;
				e->chunkpz[i] += 10*UPM/TPS;
			}
			if(e->chunktype[i] == 3)
			{
				//if(e->chunklife[i] > TPS/6)
					//e->chunklife[i] = TPS/6;
				size = ((float)e->chunklife[i])/(TPS/4);
				size = 2.0*size - 1.0;
				size = 7.0*(1.0 - size*size);
				r = 1.0;
				g = 0.4;
				b = 0.0;
			}
			if(e->chunktype[i] == 4)
			{
				size = ((float)e->chunklife[i])/(TPS/4);
				size = 1.0*size - 1.0;
				size = 0.5*(1.0 - size*size);
				r = 0.9;
				g = 0.3;
				b = 0.0;
				e->chunkpz[i] += 5*UPM/TPS;
			}
			if(e->chunktype[i] == 5)
			{
				size = ((float)e->chunklife[i])/(TPS/4);
				size = 2.0*size - 1.0;
				size = 2.5*(1.0 - size*size);
				r = 1.0;
				g = 0.0;
				b = 0.0;
				e->chunkpz[i] += 8*UPM/TPS;
			}
			matrix mat = cammatrix;
			matrix_translate(&mat,-brb->camx,-brb->camy,-brb->camz);
			// FLAG cam is a float, could lead to jigging depending on distance
			matrix_translate(&mat,
					((float)e->chunkpx[i])/UPM,
					((float)e->chunkpy[i])/UPM,
					((float)e->chunkpz[i])/UPM);
			matrix_scale(&mat,size,size,size);

			render_drawmodel(modellist[m], mat,r,g,b);
		//xx
		}

		glDisableVertexAttribArray(objvertpointer);
	}
	//}}}

	// Draw HUD
	//{{{
	{
		glUseProgram(objprogram);

		glEnableVertexAttribArray(objvertpointer); // pointer in env prog to incoming verts

		int m;
		matrix mat;

		// replace with modified proj matrix
		float wr = ((float)g_windowsquare)/g_windowwidth;
		float hr = ((float)g_windowsquare)/g_windowheight;

		m = RE_MWOOPS;

		matrix_identity(&mat);
		matrix_translate(&mat,(float)mousex/UPM*wr,(float)mousey/UPM*hr,-0.9f);
		matrix_scale(&mat, wr,hr,1.0f);
		//matrix_multiply(&mat, &cammatrix, &mat);
		render_drawmodel(modellist[m], mat,1.0,0.5,0.0);

		// Render cannon direction
		if(1) // change to a check if render relative direction option enabled
		{
			intv evel = brb->hudevel;
			intv aimx = mousex;
			intv aimy = mousey;
			intv aimz = -UPM;
			NORMALIZE(aimx,aimy,aimz);
			intv utvx = (brb->camlx-brb->camx)*UPM*TPS;
			intv utvy = (brb->camly-brb->camy)*UPM*TPS;
			intv utvz = (brb->camlz-brb->camz)*UPM*TPS;
			intv tvx = UMV((int)(UPM*rotmatrix.cell[0]),utvx)
				+UMV((int)(UPM*rotmatrix.cell[4]),utvy)
				+UMV((int)(UPM*rotmatrix.cell[8]),utvz);
			intv tvy = UMV((int)(UPM*rotmatrix.cell[1]),utvx)
				+UMV((int)(UPM*rotmatrix.cell[5]),utvy)
				+UMV((int)(UPM*rotmatrix.cell[9]),utvz);
			intv tvz = UMV((int)(UPM*rotmatrix.cell[2]),utvx)
				+UMV((int)(UPM*rotmatrix.cell[6]),utvy)
				+UMV((int)(UPM*rotmatrix.cell[10]),utvz);
			intv adv = UMV(aimx,tvx)+UMV(aimy,tvy)+UMV(aimz,tvz);
			intv px = tvx - UMV(aimx,adv);
			intv py = tvy - UMV(aimy,adv);
			intv pz = tvz - UMV(aimz,adv);
			intv ada = VMV(evel,evel) - VMV(px,px)+VMV(py,py)+VMV(pz,pz);
			if(ada>0 && evel>0)
			{
				intv ine = VDV(UPM,evel);
				intv maga = SQR(ada);
				intv ax = UMV(aimx,maga);
				intv ay = UMV(aimy,maga);
				intv az = UMV(aimz,maga);
				aimx = VMV(ine,ax) + VMV(ine,px);
				aimy = VMV(ine,ay) + VMV(ine,py);
				aimz = VMV(ine,az) + VMV(ine,pz);
				if(aimz<0)
				{
					aimz = VDV(-UPM,aimz);
					aimx = VMV(aimx,aimz);
					aimy = VMV(aimy,aimz);
				}
			}
			matrix_identity(&mat);
			matrix_translate(&mat,(float)aimx/UPM*wr,(float)aimy/UPM*hr,-0.9f);
			matrix_scale(&mat, 0.6*wr,0.6*hr,1.0f);
			matrix_rotatez(&mat, 0.707, 0.707);
			render_drawmodel(modellist[m], mat, 1.0,0.0,0.0);
		}

		glDisableVertexAttribArray(objvertpointer);

		// Test texture thingy
		{

			matrix hmat;
			matrix_identity(&hmat);
			matrix_translate(&hmat, 0.7f, 0.7f, 0.0f);
			matrix_scale(&hmat, 0.2f, 0.2f, 1.0f);

			glUseProgram(hudprogram);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glEnableVertexAttribArray(hudvertpointer);
			glEnableVertexAttribArray(huduvpointer);


			glBindBuffer(GL_ARRAY_BUFFER, testrectVBO);
			glVertexAttribPointer(hudvertpointer, 3, GL_FLOAT, GL_FALSE,
							5*sizeof(GLfloat), 0);
			glVertexAttribPointer(huduvpointer, 2, GL_FLOAT, GL_FALSE,
						5*sizeof(GLfloat), (void*)(3*sizeof(float)));

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, testrectIBO);

			glUniformMatrix4fv(hudmatrixpointer, 1, 0, hmat.cell);
			glUniform1i(hudtexturepointer, 0);

			//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


			glDisable(GL_BLEND);

			glDisableVertexAttribArray(hudvertpointer);
		}
	}
	//}}}

	glUseProgram(0);

	debug_benchstop("Update","");

#ifndef EMSCRIPTEN
	SDL_GL_SwapWindow(window);
#else
	SDL_GL_SwapBuffers();
#endif

	debug_benchstop("Render","");

}
//}}}


//}}}

