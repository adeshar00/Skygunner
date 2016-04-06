#ifndef INTMATH_H
#define INTMATH_H

// About
//{{{
/*
	Author:  Andrew Desharnais
	Version: 0.0.01


   A library for the use of integer data types to represent fixed-point rational numbers.

   Note: makes heavy use of macros.  All macro names are in all caps (e.x. UMU(a,b)).
    Macros with three-letter names represent a simple replacement, while names with over
    three leters represent an entire codeblock, and usually treat their arguments
    as pointers (though take the variables themselves as arguments, instead of pointers to
    those variables, for the sake of speed)

   This code should work for any int byte size, as long as the int has enough bytes to
    accomodate the values specified in the "Independent Definitions" section.

   Macros and definitions prefixed with an underscore represent values/macros that
    are "private" to this library.  Those without the underscore represent
    values/functions intended to be used externally.


   Three pseudo-types of ints:
 	Arbitrary vector coordinate (intv)
   	Unit vector coordinate (intu)
	Angle (inta)

   "intv" and "intu" types represent fixed point rational numbers.  Both of them can represent
    fractional values with an accuracy of 1/UPM (the UPM definition, defined below,
    represents the number of int units that make up 1 meter)

   Functions/macros operating on "intv" types produce defined behavior for any number between
    -(2^(sizeof(int)*8-_FB-1)) and 2^(sizeof(int)*8-_FB-1)-1 (inclusive). If a "intv" type is
    set to represent a value outside of this range it will overflow.  "intv" types are meant to
    represent the coordinates of a vector, or a scalar (1 dimensional vector).

   Functions/macros operating on "intu" types only produce defined behavior when the value
    of that "intu" falls between -sqrt(2) and sqrt(2) (exclusive)*.  They are meant to represent
    the coordinates of a unit vector, or any scalar which is meant to reduce a value.
   Operations involving "intu" types generally have less overhead than functions involving
    "intv" types.
   *the UMU macro may overflow if the result is >= 2, if ints are 4 bytes

   "inta" types use all int bits: 2^(sizeof(int)*8) = one full rotation (2*PI radians).
   Overflow from adding or subtracting "inta" types naturally mimicks behavior of an
    angle, and subtracting one angle from another is guaranteed to yield a "inta" type
    within a range of PI to -PI radians.
   Sin and Cos are done via lookup tables, gives a 1/(2^(2*_TBITS)) approximation
   Lookup table size = 4*(2^_TBITS)(sizeof(int))
   "inta" types can be multiplied by scalars via the VMV or UMV macros (treat the inta like
    an intv when doing so).



   */
//}}}


// Typedefs
//{{{
typedef int intv;
typedef int intu;
typedef int inta;
//}}}

// Independent Definitions
//{{{

// Fractional Bits: how many bits used to represent a fraction (binary decimal places)
//  Note - should not exceed ((sizeof(int)*8/2)-1)
#define _FB 15

// Bits for lookup tables in trig operations (see comments below)
//  Note - should not exceed (sizeof(int)*8/2))
#define _TBITS 7

//}}}

// Dependent Definitions
//{{{


// Units Per Meter
#define UPM (1<<_FB)

// Negative bit set to 1 (0x10...00)
#define _NEGBIT (1<<((sizeof(int)*8)-1))

// PI over 2   (An A type int (angle) that is 1/2 PI Radians in value)
#define PI2 ((_NEGBIT>>1)^_NEGBIT)

// Trig related definitions (used in ANGTOCS)
#define _TTABLESIZE (1<<_TBITS)
#define _THMASK (_NEGBIT>>(_TBITS-1)) // Mask for higher digits of angle
#define _TLMASK ((_NEGBIT>>(2*_TBITS-1))^_THMASK) // Mask for lower digits of angle
#define _THRS ((sizeof(int)*8)-_TBITS)  // High digit Right Shift (to generate index for lookup)
#define _TLRS ((sizeof(int)*8)-2*_TBITS)  // Low digit Right Shift
#define _TIMASK ((1<<_TBITS)-1) // index mask (to use after rightshifted)
#define _TNUDGE (1<<(_TLRS-1))


//}}}

// Global Variables
//{{{

// Lookup table for ANGTOCS
#ifdef INTMATH_C
#define VISIBLE
#else
#define VISIBLE extern
#endif
VISIBLE int _THCOS[_TTABLESIZE];
VISIBLE int _THSIN[_TTABLESIZE];
VISIBLE int _TLCOS[_TTABLESIZE];
VISIBLE int _TLSIN[_TTABLESIZE];
#undef VISIBLE

//}}}

// Function Prototypes
//{{{

void intmath_init();

// FLAG temp, remove when replacements finished
int isqrt();
void normalize(intv*,intv*,intv*);
int vdv(intv,intv);
int irand();
int iatan2(intv,intv);
//}}}


// U V Macros
//{{{

// intu * intu
#define UMU(u1,u2) (((u1)*(u2))>>_FB)

// intu * intv
#define UMV(u,v) (((u)*(((v)&(-UPM))>>_FB))+(((u)*((v)&(UPM-1)))>>_FB))

// intv * intv
#define VMV(v1,v2) ((((((v1)&(-UPM))>>_FB)*(((v2)&(-UPM))>>_FB))<<_FB)+(((v1)&(-UPM))>>_FB)*((v2)&(UPM-1))+(((v2)&(-UPM))>>_FB)*((v1)&(UPM-1))+((((v1)&(UPM-1))*((v2)&(UPM-1)))>>_FB))

// intv / intv FLAG replace with something more efficient
#define VDV(v1,v2) (vdv((v1),(v2)))

//}}}

// Trigonometry Macros
//{{{

// ANGle TO Cosine and Sine (ANGTOCS)
//  Takes an angle (a) and sets the c and s variables to the angles cosine and sine
//  Macro is an expansion of the commented out angtocs function in intmath.c
#define ANGTOCS(a,c,s)\
{\
c=UMU((_TLCOS[(((((a)+_TNUDGE)&_TLMASK)>>_TLRS)&_TIMASK)]),(_THCOS[(((((a)+_TNUDGE)&_THMASK)>>_THRS)&_TIMASK)]))-UMU((_TLSIN[(((((a)+_TNUDGE)&_TLMASK)>>_TLRS)&_TIMASK)]),(_THSIN[(((((a)+_TNUDGE)&_THMASK)>>_THRS)&_TIMASK)]));\
s=UMU((_TLSIN[(((((a)+_TNUDGE)&_TLMASK)>>_TLRS)&_TIMASK)]),(_THCOS[(((((a)+_TNUDGE)&_THMASK)>>_THRS)&_TIMASK)]))+UMU((_TLCOS[(((((a)+_TNUDGE)&_TLMASK)>>_TLRS)&_TIMASK)]),(_THSIN[(((((a)+_TNUDGE)&_THMASK)>>_THRS)&_TIMASK)]));\
}

// Takes y and x values and produces angle
#define ATAN2(y,x,a) a=iatan2((y),(x))

//}}}

// Square Root and Vector Magnitude Macros
//{{{

// Temporary, will eventually replace with something actually efficient FLAG
#define SQR(x) (isqrt(x))

#define NORMALIZE(x,y,z) {normalize((&x),(&y),(&z));}

//}}}

#endif
