// noise1234
//
// Author: Stefan Gustavson, 2003-2005
// Contact: stegu@itn.liu.se
//
// This code was GPL licensed until February 2011.
// As the original author of this code, I hereby
// release it into the public domain.
// Please feel free to use it for whatever you want.
// Credit is appreciated where appropriate, and I also
// appreciate being told where this code finds any use,
// but you may do as you like.

/** \file
		\brief Declares the "noise1" through "noise4" functions for Perlin noise.
		\author Stefan Gustavson (stegu@itn.liu.se)
*/

/*
 * This is a backport to C of my improved noise class in C++.
 * It is highly reusable without source code modifications.
 *
 * Note:
 * Replacing the "float" type with "double" can actually make this run faster
 * on some platforms. A templatized version of Noise1234 could be useful.
 */

/** 1D, 2D, 3D and 4D float Perlin noise, SL "noise()"
 */
extern float noise1( float x );
extern float noise2( float x, float y );
extern float noise3( float x, float y, float z );
extern float noise4( float x, float y, float z, float w );

/** 1D, 2D, 3D and 4D float Perlin periodic noise, SL "pnoise()"
 */
extern float pnoise1( float x, int px );
extern float pnoise2( float x, float y, int px, int py );
extern float pnoise3( float x, float y, float z, int px, int py, int pz );
extern float pnoise4( float x, float y, float z, float w,
                              int px, int py, int pz, int pw );
