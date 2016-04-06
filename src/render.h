
// Structs
//{{{
#ifdef RENDER_C

typedef struct rawmodel
{
		GLfloat* verts;
		int vertslen;	// number of floats in "verts" array (number of verts * 3)
		GLuint* indis;
		int indislen;   // number of uints in "indis" array (number of tris * 3)
} rawmodel;

typedef struct model
{
	GLuint VBO;	// pointer to vertex buffer on gpu
	GLuint IBO;	// pointer to index buffer on gpu
	GLsizei IBOlen;	// size of IBO (number of tris * 3)
} model;

#ifdef DEBUG // FLAG
typedef struct rawwireframe
{
	GLuint* indis;
	int indislen;
	rawmodel* base;
} rawwireframe;

typedef struct wireframe
{
	GLuint IBO;	// pointer to index buffer on gpu
	GLsizei IBOlen;	// size of IBO (number of tris * 3)
	model* base;
} wireframe;
#endif

#endif
//}}}

// Definitions
//{{{
#if defined(RENDER_C) || defined(BATTLE_C)
enum
{
RE_MBULLET,
RE_MCHARGE,
RE_MWOOPS,
RE_MRIFLEMAN,
RE_MTANK,
RE_MJEEP,
RE_MAAMISSLE,
RE_MMONOLITH,
RE_MTEMPGAS,
RE_MODELCOUNT};	// Leave as last enum for accurate modelcount
#endif
//}}}

// Variables
//{{{

 // "Public" Global Vars
#ifdef BATTLE_C
#define VISIBLE
#else
#define VISIBLE extern
#endif

#if defined(RENDER_C) || defined(INPUT_C)
//VISIBLE matrix projmatrix;
#endif

#undef VISIBLE

// "Private" Global Vars
#ifdef RENDER_C

#ifndef EMSCRIPTEN
	// Native
	SDL_Window* window;
	SDL_GLContext glcontext;
#else
	// Web
	SDL_Surface* screen;
#endif

GLuint envprogram;
GLint envvertpointer;
GLint envmatrixpointer;
GLuint objprogram;
GLint objvertpointer;
GLint objmatrixpointer;
GLint objcolorpointer;
GLuint hudprogram;
GLint hudvertpointer;
GLint hudmatrixpointer;
GLint huduvpointer;
GLint hudtexturepointer;

SDL_Event event;

// FLAG for env tri, replace with model
GLuint VBO;
GLuint IBO;
GLsizei icount;

model tetratest;


model* modellist;
int modellistcount;

// FLAGTE eventually make a struct for these?
// Currently populated in render_modelinit and used in battle HUD render
GLuint testrectVBO;
GLuint testrectIBO;
GLuint testtextureID;

#endif
//}}}

// Prototypes
//{{{

#if defined(UI_C) || defined(RENDER_C)

void render_init();
void render_deinit();
void render_test(int);
void render_shop();
void render_battle(battlerenderbuffer*,float,intv,intv,effectdata*);
void render_modelinit();

#endif

#ifdef INPUT_C
void render_resize(int,int);
#endif

//}}}

