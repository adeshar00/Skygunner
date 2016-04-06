
// Includes
//{{{
#include "matrix.h"
//}}}


// Matrix Functions
//{{{

/*
   Note: These functions work "backwards": most of the below functions modify a matrix
    by multiplying it with the appropriate tranformation matrix on it's right.
   If a bunch of these functions are carried out in succession, the resulting matrix
    will work as if the transformation in the last function were carried out first and
    the first functions transformation last.
*/


void matrix_identity(matrix* mat)
{
	float* c = mat->cell;
	c[0]=1.0f; c[4]=0.0f; c[8] =0.0f; c[12]=0.0f;
	c[1]=0.0f; c[5]=1.0f; c[9] =0.0f; c[13]=0.0f;
	c[2]=0.0f; c[6]=0.0f; c[10]=1.0f; c[14]=0.0f;
	c[3]=0.0f; c[7]=0.0f; c[11]=0.0f; c[15]=1.0f;
}

void matrix_translate(matrix* mat, float x, float y, float z)
{
	float* c = mat->cell;
	c[12] = x*c[0] + y*c[4] + z*c[ 8] + c[12];
	c[13] = x*c[1] + y*c[5] + z*c[ 9] + c[13];
	c[14] = x*c[2] + y*c[6] + z*c[10] + c[14];
	c[15] = x*c[3] + y*c[7] + z*c[11] + c[15];
}

void matrix_scale(matrix* mat, float x, float y, float z)
{
	float* c = mat->cell;
	c[0]*= x;
	c[1]*= x;
	c[2]*= x;
	c[3]*= x;
	c[4]*= y;
	c[5]*= y;
	c[6]*= y;
	c[7]*= y;
	c[8]*= z;
	c[9]*= z;
	c[10]*= z;
	c[11]*= z;
}

void matrix_rotatex(matrix* mat, float cos, float sin)
{
	float* c = mat->cell;
	float t1, t2;

	t1 = cos*c[4] + sin*c[8];
	t2 = cos*c[8] - sin*c[4];
	c[4] = t1;
	c[8] = t2;
	t1 = cos*c[5] + sin*c[9];
	t2 = cos*c[9] - sin*c[5];
	c[5] = t1;
	c[9] = t2;
	t1 = cos*c[6] + sin*c[10];
	t2 = cos*c[10] - sin*c[6];
	c[6] = t1;
	c[10] = t2;
	t1 = cos*c[7] + sin*c[11];
	t2 = cos*c[11] - sin*c[7];
	c[7] = t1;
	c[11] = t2;
}

void matrix_rotatey(matrix* mat, float cos, float sin)
{
	float* c = mat->cell;
	float t1, t2;

	t1 = cos*c[8] + sin*c[0];
	t2 = cos*c[0] - sin*c[8];
	c[8] = t1;
	c[0] = t2;
	t1 = cos*c[9] + sin*c[1];
	t2 = cos*c[1] - sin*c[9];
	c[9] = t1;
	c[1] = t2;
	t1 = cos*c[10] + sin*c[2];
	t2 = cos*c[2] - sin*c[10];
	c[10] = t1;
	c[2] = t2;
	t1 = cos*c[11] + sin*c[3];
	t2 = cos*c[3] - sin*c[11];
	c[11] = t1;
	c[3] = t2;
}

void matrix_rotatez(matrix* mat, float cos, float sin)
{
	float* c = mat->cell;
	float t1, t2;

	t1 = cos*c[0] + sin*c[4];
	t2 = cos*c[4] - sin*c[0];
	c[0] = t1;
	c[4] = t2;
	t1 = cos*c[1] + sin*c[5];
	t2 = cos*c[5] - sin*c[1];
	c[1] = t1;
	c[5] = t2;
	t1 = cos*c[2] + sin*c[6];
	t2 = cos*c[6] - sin*c[2];
	c[2] = t1;
	c[6] = t2;
	t1 = cos*c[3] + sin*c[7];
	t2 = cos*c[7] - sin*c[3];
	c[3] = t1;
	c[7] = t2;
}

void matrix_multiply(matrix* result, matrix* left, matrix* right)
{
	float* c = result->cell;
	float* cl = left->cell;
	float* cr = right->cell;

	float cl00 = cl[0];
	float cl01 = cl[1];
	float cl02 = cl[2];
	float cl03 = cl[3];
	float cl04 = cl[4];
	float cl05 = cl[5];
	float cl06 = cl[6];
	float cl07 = cl[7];
	float cl08 = cl[8];
	float cl09 = cl[9];
	float cl10 = cl[10];
	float cl11 = cl[11];
	float cl12 = cl[12];
	float cl13 = cl[13];
	float cl14 = cl[14];
	float cl15 = cl[15];
	float cr00 = cr[0];
	float cr01 = cr[1];
	float cr02 = cr[2];
	float cr03 = cr[3];
	float cr04 = cr[4];
	float cr05 = cr[5];
	float cr06 = cr[6];
	float cr07 = cr[7];
	float cr08 = cr[8];
	float cr09 = cr[9];
	float cr10 = cr[10];
	float cr11 = cr[11];
	float cr12 = cr[12];
	float cr13 = cr[13];
	float cr14 = cr[14];
	float cr15 = cr[15];

	c[0]  = cl00*cr00 + cl04*cr01 + cl08*cr02 + cl12*cr03;
	c[1]  = cl01*cr00 + cl05*cr01 + cl09*cr02 + cl13*cr03;
	c[2]  = cl02*cr00 + cl06*cr01 + cl10*cr02 + cl14*cr03;
	c[3]  = cl03*cr00 + cl07*cr01 + cl11*cr02 + cl15*cr03;
	c[4]  = cl00*cr04 + cl04*cr05 + cl08*cr06 + cl12*cr07;
	c[5]  = cl01*cr04 + cl05*cr05 + cl09*cr06 + cl13*cr07;
	c[6]  = cl02*cr04 + cl06*cr05 + cl10*cr06 + cl14*cr07;
	c[7]  = cl03*cr04 + cl07*cr05 + cl11*cr06 + cl15*cr07;
	c[8]  = cl00*cr08 + cl04*cr09 + cl08*cr10 + cl12*cr11;
	c[9]  = cl01*cr08 + cl05*cr09 + cl09*cr10 + cl13*cr11;
	c[10] = cl02*cr08 + cl06*cr09 + cl10*cr10 + cl14*cr11;
	c[11] = cl03*cr08 + cl07*cr09 + cl11*cr10 + cl15*cr11;
	c[12] = cl00*cr12 + cl04*cr13 + cl08*cr14 + cl12*cr15;
	c[13] = cl01*cr12 + cl05*cr13 + cl09*cr14 + cl13*cr15;
	c[14] = cl02*cr12 + cl06*cr13 + cl10*cr14 + cl14*cr15;
	c[15] = cl03*cr12 + cl07*cr13 + cl11*cr14 + cl15*cr15;

}

//}}}

// Forward Matrix Functions
//{{{

/*
   I messed up initially and made functions that multiply in on the left side, derp.
   I figured I might as well keep em since I made em, in case I ever actually
    have a use for them at some point.
*/

void matrix_translatefw(matrix* mat, float x, float y, float z)
{
	float* c = mat->cell;
	c[12]+= x;
	c[13]+= y;
	c[14]+= z;
}

void matrix_scalefw(matrix* mat, float x, float y, float z)
{
	float* c = mat->cell;
	c[0]*= x;
	c[1]*= y;
	c[2]*= z;
	c[4]*= x;
	c[5]*= y;
	c[6]*= z;
	c[8]*= x;
	c[9]*= y;
	c[10]*= z;
	c[12]*= x;
	c[13]*= y;
	c[14]*= z;
}

void matrix_rotatexfw(matrix* mat, float cos, float sin)
{
	float* c = mat->cell;
	float t1, t2;

	t1 = cos*c[1] - sin*c[2];
	t2 = sin*c[1] + cos*c[2];
	c[1] = t1;
	c[2] = t2;
	t1 = cos*c[5] - sin*c[6];
	t2 = sin*c[5] + cos*c[6];
	c[5] = t1;
	c[6] = t2;
	t1 = cos*c[9] - sin*c[10];
	t2 = sin*c[9] + cos*c[10];
	c[9] = t1;
	c[10] = t2;
	t1 = cos*c[13] - sin*c[14];
	t2 = sin*c[13] + cos*c[14];
	c[13] = t1;
	c[14] = t2;
}

void matrix_rotateyfw(matrix* mat, float cos, float sin)
{
	float* c = mat->cell;
	float t1, t2;

	t1 = cos*c[2] - sin*c[0];
	t2 = sin*c[2] + cos*c[0];
	c[2] = t1;
	c[0] = t2;
	t1 = cos*c[6] - sin*c[4];
	t2 = sin*c[6] + cos*c[4];
	c[6] = t1;
	c[4] = t2;
	t1 = cos*c[10] - sin*c[8];
	t2 = sin*c[10] + cos*c[8];
	c[10] = t1;
	c[8] = t2;
	t1 = cos*c[14] - sin*c[12];
	t2 = sin*c[14] + cos*c[12];
	c[14] = t1;
	c[12] = t2;
}

void matrix_rotatezfw(matrix* mat, float cos, float sin)
{
	float* c = mat->cell;
	float t1, t2;

	t1 = cos*c[0] - sin*c[1];
	t2 = sin*c[0] + cos*c[1];
	c[0] = t1;
	c[1] = t2;
	t1 = cos*c[4] - sin*c[5];
	t2 = sin*c[4] + cos*c[5];
	c[4] = t1;
	c[5] = t2;
	t1 = cos*c[8] - sin*c[9];
	t2 = sin*c[8] + cos*c[9];
	c[8] = t1;
	c[9] = t2;
	t1 = cos*c[12] - sin*c[13];
	t2 = sin*c[12] + cos*c[13];
	c[12] = t1;
	c[13] = t2;
}

//}}}

