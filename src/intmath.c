
// About
//{{{
/*
	Author:  Andrew Desharnais
	Version: 0.0.01

   A library for the use of integer data types to represent fixed-point rational numbers.
   
   See intmath.h About section for more information.

*/
//}}}


// Includes
//{{{

#include <math.h>
#include <stdlib.h> // for rand
#include <time.h> // for seeding srand

#define INTMATH_C
#include "intmath.h"

//}}}


// Functions
//{{{

void intmath_init()
{

	// Populate Trig lookup tables
	{
#define PI 3.14159265359f
		float ha,la;
		int i;
		for(i=0;i<_TTABLESIZE;i++)
		{
			ha = (((float)i/(float)_TTABLESIZE))*2.0f*PI;
			_THCOS[i] = cos(ha)*UPM;
			_THSIN[i] = sin(ha)*UPM;
			la = (((float)i/(float)_TTABLESIZE))*2.0f*PI/((float)_TTABLESIZE);
			_TLCOS[i] = cos(la)*UPM;
			_TLSIN[i] = sin(la)*UPM;
		}
		// Make sure 1's are where they should be
		_THCOS[_TTABLESIZE/2] = -UPM;
		_THSIN[_TTABLESIZE/4] = UPM;
		_THSIN[_TTABLESIZE*3/4] = -UPM;
		/*
		// Print table
		for(i=0;i<_TTABLESIZE;i++)
		{
			printf("%3d %8x %8x %8x %8x \n",i,_THCOS[i],_THSIN[i],_TLCOS[i],_TLSIN[i]);
		}
		*/
#undef PI
	}

	// Seed random number generator FLAG remove when get rid of rand() call below
	srand(time(NULL));

}

// Temporary functions, just uses math library and floats and applies to integers
int isqrt(int x)
{
	float fx = ((float)x)/UPM;
	return (int)(sqrt(fx)*UPM);
}

int vdv(int v1, int v2)
{
	float f1 = ((float)v1)/UPM;
	float f2 = ((float)v2)/UPM;
	return f1/f2*UPM;
}

int irand()
{
	// Returns int in range of 0 <= x < UPM
	// unseeded, maybe good
	return rand()&(UPM-1);
}

int iatan2(intv y, intv x)
{
#define PI 3.14159265359f
	float fy = ((float)y)/UPM;
	float fx = ((float)x)/UPM;
	return (inta)(atan2(fy,fx)/PI*PI2*2);
#undef PI
}

/*

int rec(int v)
{
	float fv = ((float)v)/UPM;
	return 1.0f/f1*UPM;
}
*/

void normalize(int* x, int* y, int* z)
{
	// make a v square macro instaed of using VMV
	float fx = ((float)*x)/UPM;
	float fy = ((float)*y)/UPM;
	float fz = ((float)*z)/UPM;
	float rmag = 1.0f/sqrt(fx*fx+fy*fy+fz*fz);
	fx*= rmag;
	fy*= rmag;
	fz*= rmag;
	*x = fx*UPM;
	*y = fy*UPM;
	*z = fz*UPM;
}

// Functions representing macros in intmath.h

/*

void angtocs(int a, int* c, int* s)
{
	int hi; // high index
	int li; // low index
	int bc;
	int bs;
	int blc;
	int bls;
	int ap = a + _TNUDGE;
	hi = ((ap&_THMASK)>>_THRS)&_TIMASK;
	li = ((ap&_TLMASK)>>_TLRS)&_TIMASK;
	bc = _THCOS[hi];
	bs = _THSIN[hi];
	blc = _TLCOS[li];
	bls = _TLSIN[li];
	*c =  umu(blc,bc) - umu(bls,bs);
	*s =  umu(bls,bc) + umu(blc,bs);
}

int vmv(int v1, int v2)
{
	int v1w = ((v1&(-UPM))>>_FB);
	int v2w = ((v2&(-UPM))>>_FB);
	int v1f = (v1&(UPM-1));
	int v2f = (v2&(UPM-1));
	return ((v1w*v2w)<<_FB)+v1w*v2f+v2w*v1f+((v1f*v2f)>>_FB);
}
*/

//}}}

