//
// srnoise8.c
//
// 2-D simplex flow noise for low-performance 8-bit CPUs (Arduino etc)
//
// Copyright 2022 Stefan Gustavson
// (stefan.gustavson@gmail.com)
// Released under the terms of the MIT license:
// https://opensource.org/licenses/MIT
// Version: 2023-02-18
//
// This function is very similar to "armnoise8", but it adds a
// parameter "alpha" to animate the noise pattern. This happens
// at a small cost in speed, so if you don't want the animation,
// "armnoise8" is slightly faster.
//
// An implementation of 2-D "Simplex Noise" written for 8-bit
// CPUs, in "hardware-tuned C" made to run fast on an ATmega32.
// Intended use cases are low resolution LED array animations
// and procedural sound generation.
// The code should compile and run on any CPU architecture,
// but it was written specifically for the ARM 8-bit architecture,
// using 8-bit arithmetic wherever possible.
// A 16 MHz Arduino Uno or Arduino Micro can churn out around
// 35,000 noise values per second with this, which is enough to do a
// lot of low resolution graphics and at least some sound synthesis.
//
// Speed is a priority here, and the noise quality suffers.
// The 2-D pattern from this function is not as nice as you would
// want for high resolution graphics. It's not "random" enough on
// a larger scale, and quantization effects from 8-bit arithmetic
// are clearly visible if you render this noise as a 2-D image.
// It was made for low-res LED array graphics and sound synthesis.
//
// 8-bit integers use 8.0 or 0.8 bits unsigned representation
// or the "fractional value" 1.7 bits, signed or unsigned.
// The number of 16-bit integers have been kept to a minimum.
//

// Remove these macros for Arduino (they are already defined there)
#define highByte(w) ((w)>>8)
#define lowByte(w) ((w) & 0xFF)
#define int8_t signed char
#define uint8_t unsigned char
#define int16_t signed short
#define uint16_t unsigned short

// This is needed for abs() on some platforms (but not for Arduino)
#include <math.h>

//
// fakesincos.c
//
// Cheap fake sine/cosine functions, using just a few additions and
// a single multiplication. Useless for any precision work, but enough
// for a quick and dirty rotation of simplex noise gradients for
// "flow noise" in the style of Ken Perlin and Fabrice Neyret.
// The sine/cosine wave period is [0..255] (N.8u binary where N>=0),
// and the output range is -127 to 127 (1.7s representation).
//
signed char fakesin(unsigned char x) {
  signed char s = x & 0x7F;
  s = s - 64;
  s = abs(2 * s);
  s = (s*s)>>7; // Maps to the FMUL instruction in 8-bit ARM MCUs
  s = 127 - s;
  if (x & 0x80) return -s;
  else return s;
}

signed char fakecos(unsigned char x) {
  unsigned char y = (x + 64); // & 0xFF implied by the type
  return fakesin(y);
}

signed char srnoise8(unsigned short x, unsigned short y, unsigned char alpha) {

  // x and y are inherently 8.8u modulo-256, but are treated
  // algoritmically as 7.8u modulo-128 for better efficiency
  x = x & 0x7FFF; // Hopefully the compiler understands that
  y = y & 0x7FFF; // it doesn't need to touch the low byte here.

  // Skew input coords to staggered grid
  unsigned short u = (x) + (y>>1); // u is 8.8u
  unsigned short v = y; // v is just an alias of y, 7.8u

  // Split to integer and fractional parts
  unsigned char u0 = highByte(u); // u0 is 8.0u
  unsigned char v0 = highByte(v); // v0 is 8.0u (or 7.0u, MSB = 0)

  unsigned char uf0 = lowByte(u); // uf0 is 0.8u
  unsigned char vf0 = lowByte(v); // vf0 is 0.8u
    
  // Determine the second vertex for the simplex
  unsigned char u1;
  unsigned char v1;
  if(uf0 > vf0) {
    u1 = u0 + 1; // u1 is 8.0u
    v1 = v0;     // v1 is 8.0u - see below
  } else {
    u1 = u0;
    v1 = v0 + 1; // This requires 8.0u for v1 (but only just)
  }

  // Third vertex is always (+1, +1)
  unsigned char u2 = u0 + 1; // u2 is 8.0u
  unsigned char v2 = v0 + 1; // v2 is 8.0u
	    
  // Transform ui,vi back to x,y coords before wrap
  signed char x0 = (u0<<1) - v0; // x0 is 7.1s
  // x0 varies in steps of 0.5 from -0.5 to 126.5
  unsigned char y0 = v0; // y0 is alias of v0, 8.0u (or 7.0u)
    
  signed short x1 = (u1<<1) - v1; // x1 is "special" 8.1s:
  // x1 varies in steps of 0.5 from -0.5 to 127.5,
  // hence 8.1s is required, but only just. With a further reduced
  // x,y domain of 6.8u, x1 would be 7.1s and fit in a signed char.
  unsigned char y1 = v1; // y1 is alias of v1, 8.0u

  unsigned char x2 = (u2<<1) - v2; // x2 is 7.1u, will never be negative
  unsigned char y2 = v2; // y2 is alias of v2, 8.0u

  // Compute vectors in x,y coords from vertices
  signed char xf0 = lowByte(x>>1) - (x0<<6); // x is 8.8u, x0 is 7.1u, xf0 is 1.7s
  signed char yf0 = lowByte(y>>1) - (y0<<7); // yf0 is 1.7s
  signed char xf1 = lowByte(x>>1) - (x1<<6); // xf1 is 1.7s
  signed char yf1 = lowByte(y>>1) - (y1<<7); // yf1 is 1.7s
  signed char xf2 = lowByte(x>>1) - (x2<<6); // xf2 is 1.7s, always xf0 - 0.5, room for optimization
  signed char yf2 = lowByte(y>>1) - (y2<<7); // yf2 is 1.7s, always yf0 - 1
    
  // Generate vertex hashes from ui, vi
  unsigned char hash0; // hash0 is 8.0u
  hash0 = (13*u0 + 7)*u0;
  hash0 = hash0 + v0;
  hash0 = (15*hash0 + 11)*hash0;

  // TODO: When u1=u0, we could reuse the u hash from hash0
  unsigned char hash1;
  hash1 = (13*u1 + 7)*u1;
  hash1 = hash1 + v1;
  hash1 = (15*hash1 + 11)*hash1;

  // TODO: When u1=u0+1, we could reuse the u hash from hash1
  unsigned char hash2; // hash2 is 8.0u
  hash2 = (13*u2 + 7)*u2;
  hash2 = hash2 + v2;
  hash2 = (15*hash2 + 11)*hash2;
	
  // Pick gradients from a small set of 8 directions    
  signed char g0x; // All these are 1.7s
  signed char g0y;
  signed char g1x;
  signed char g1y;
  signed char g2x;
  signed char g2y;

  // using +/-0.9921875 (+/-127) instead of
  // +0.9921875/-1.0 (+127/-128) for symmetry
  if (hash0 & 0x01) {
    g0x = 54; g0y = 108;
  } else {
    g0x = 108; g0y = 54;
  };
  if(hash0 & 0x02) {
    g0x = -g0x;
  };
  if (hash0 & 0x04) {
    g0y = -g0y;
  };

  if (hash1 & 0x01) {
    g1x = 54; g1y = 108;
  } else {
    g1x = 108; g1y = 54;
  };
  if(hash1 & 0x02) {
    g1x = -g1x;
  };
  if (hash1 & 0x04) {
    g1y = -g1y;
  };

  if (hash2 & 0x01) {
    g2x = 54; g2y = 108;
  } else {
    g2x = 108; g2y = 54;
  };
  if(hash2 & 0x02) {
    g2x = -g2x;
  };
  if (hash2 & 0x04) {
    g2y = -g2y;
  };

  signed char Ca, Sa;  // 1.7s
  signed char g0x_t, g0y_t, g1x_t, g1y_t, g2x_t, g2y_t; 
  if(alpha != 0) { // Rotate the gradients
    Sa = fakesin(alpha);
    Ca = fakecos(alpha);
    g0x_t = g0x; // all 1.7s, temp storage to prevent trampling on input values
    g0y_t = g0y;
    g1x_t = g1x;
    g1y_t = g1y;
    g2x_t = g2x;
    g2y_t = g2y;
    g0x = ((Ca*g0x_t)>>7) - ((Sa*g0y_t)>>7); // These twelve multiplications constitute a
    g0y = ((Sa*g0x_t)>>7) + ((Ca*g0y_t)>>7); // considerable amount of work for a weak CPU.
    g1x = ((Ca*g1x_t)>>7) - ((Sa*g1y_t)>>7); // Don't use alpha != 0 unless you need it.
    g1y = ((Sa*g1x_t)>>7) + ((Ca*g1y_t)>>7);
    g2x = ((Ca*g2x_t)>>7) - ((Sa*g2y_t)>>7);
    g2y = ((Sa*g2x_t)>>7) + ((Ca*g2y_t)>>7);
  }

  // Compute ramps (g dot u) from vertices
  signed char g0 = ((g0x*xf0)>>7) + ((g0y*yf0)>>7); // g0 is 1.7s
  signed char g1 = ((g1x*xf1)>>7) + ((g1y*yf1)>>7); // g1 is 1.7s
  signed char g2 = ((g2x*xf2)>>7) + ((g2y*yf2)>>7); // g2 is 1.7s
  // Note that g0/g1/g2 will overflow 1.7s at some corners, but
  // the overflow happens only in regions where m0/m1/m2 = 0 and
  // the incorrect, sign-flipped value is multiplied by zero.

  // Compute radial falloff from vertices
  unsigned char r0 = ((xf0*xf0)>>7) + ((yf0*yf0)>>7); // r0 is 1.7u
  unsigned char m0;
  if(r0 > 102) {
    m0 = 0;
  } else {
    m0 = 255 - (r0<<1) - (r0>>1);
    // m0 is 0.8u, "(r0<<1)+(r0>>1)" is 1.25*r0 in 0.8u
    m0 = (m0*m0)>>8; // 8-bit by 8-bit to 8-bit 0.8u mult
    m0 = (m0*m0)>>8;
  }

  unsigned char r1 = ((xf1*xf1)>>7) + ((yf1*yf1)>>7); // r1 is 1.7u
  unsigned char m1;
  if(r1 > 102) {
    m1 = 0;
  } else {
    m1 = 255 - (r1<<1) - (r1>>1); // m1 is 0.8u
    m1 = (m1*m1)>>8;
    m1 = (m1*m1)>>8;
  }

  unsigned char r2 = ((xf2*xf2)>>7) + ((yf2*yf2)>>7); // r2 is 0.8u
  unsigned char m2;
  if(r2 > 102) {
    m2 = 0;
  } else {
    m2 = 255 - (r2<<1) - (r2>>1); // m2 is 0.8u
    m2 = (m2*m2)>>8;
    m2 = (m2*m2)>>8;
  }

  // Multiply ramps with falloffs
  signed char n0 = (g0 * m0)>>6; // g0 is 1.7s, m0 is 0.8u, n0 is 1.7s, scale by 4
  signed char n1 = (g1 * m1)>>6; // mult scaled by 4
  signed char n2 = (g2 * m2)>>6; // mult scaled by 4
  // Factors gi, mi span their ranges and can't be scaled individually,
  // but all of the products (ni) are always < 0.25 and can be shifted
  // left 2 steps for two additional bits of precision.
  // Multiplications in ATmega32 are 8-by-8-to-16 bits, but selecting
  // the "best" bits requires a few extra operations.

  // Sum up noise contributions from all vertices
  signed char n;
  n = (136*(n0 + n1 + n2))>>7; // Scale to better cover the range [-128,127]

  // We're done. Return the result in 1.7s format.
  return n;
}
