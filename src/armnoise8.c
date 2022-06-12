//
// armnoise8.c
//
// 2-D simplex noise for low-performance 8-bit CPUs (Arduino etc)
//
// Copyright 2022 Stefan Gustavson
// (stefan.gustavson@gmail.com)
// Released under the terms of the MIT license:
// https://opensource.org/licenses/MIT
// Version: 2022-06-12
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

// The input coordinates are 16-bit unsigned integers in 7.8 bits
// representation: 0 to 32767 represent 0.0 to 127.9961 in steps
// of 1/256, with the pattern making a seamless tiling wrap at 128.
// The output value is a signed 8-bit integer in 1.7s representation:
// -128 to 127 represents -1 to 0.9922 in steps of 1/128.
// The output value stays within the range -1 to 1 by a small margin.
// If you want unsigned 0-255 output, add 128 to the output.
//
int8_t armnoise8(uint16_t x, uint16_t y)
{
  // Wrap x,y to 0..127 (7.8u)
  x = x & 0x7FFF;
  y = y & 0x7FFF;

  /*
   * Skew input (x,y) coords to staggered (u,v) grid
   */
  uint16_t u; // u: 8.8u
  // Effectively: u = x + fract(y/2)
  u = x + lowByte(y >> 1);
  // uint16_t v = y; // Don't copy, just use y in-place

  /*
   * Split u, v to integer and fractional parts
   */
  uint8_t u0;
  uint8_t v0;
  uint8_t uf0;
  uint8_t vf0;
  // We could use these in-place, but copying can release one half of a
  // 16-bit paired register early, and more free registers is always good.
  u0 = highByte(u);
  uf0 = lowByte(u);
  v0 = highByte(y); // v = y
  vf0 = lowByte(y);

  /*
   * Transform u0 back to x coords
   */
  signed char x0; // x0: 7.1s
  x0 = (u0 << 1) - (v0 & 0x01);
  // unsigned char y0 = v0; // Don't copy, just use v0

  /*
   * Determine the second vertex for the simplex
   */
  unsigned char u1; // u1: 8.0u
  unsigned char v1; // v1: 8.0u
  signed char x1; // x1: 7.1s
  if(uf0 > vf0) {
    u1 = u0 + 1;
    v1 = v0;
    x1 = x0 + 2; // "+1" in 7.1s
  } else {
    u1 = u0;
    v1 = v0 + 1;
    x1 = x0 - 1; // "-0.5" in 7.1s
  }
  // y1 = v1; // Don't copy, just use v1

  /*
   * The third vertex is always (+1, +1) in u,v
   */
  unsigned char u2;
  unsigned char v2;
  unsigned char x2;
  u2 = u0 + 1; // u2: 8.0u
  v2 = v0 + 1; // v2: 8.0u
  x2 = x0 + 1; // x2: 7.1u (+1 means +0.5)
  // y2 = v2;  // Don't copy, use v2

  /*
   * Generate hashes from x, y vertex coordinates
   */
  unsigned char h0; // h0: 8.0u
  unsigned char h1; // h1: 8.0u
  unsigned char h2; // h2: 8.0u
  // NOTE: Re-use of h(x0) when x1=x0 could save a few operations,
  // but requires a conditional. A marginal speedup, if any.
  h0 = (13*x0 + 3)*x0;
  h0 = h0 + v0;
  h0 = (17*h0 + 7)*h0;
  h1 = (13*x1 + 3)*x1;
  h1 = h1 + v1;
  h1 = (17*h1 + 7)*h1;
  h2 = (13*x2 + 3)*x2;
  h2 = h2 + v2;
  h2 = (17*h2 + 7)*h2;

  /*
   * Compute vector in x,y coords from the first vertex
   */
  signed char xf0; // xf0: 1.7s
  signed char yf0; // yf0: 1.7s
  signed char xf1; // xf1: 1.7s
  signed char yf1; // yf1: 1.7s
  signed char xf2; // xf2: 1.7s
  signed char yf2; // yf2: 1.7s
  xf0 = lowByte(x>>1) - (x0<<6); // x0 is 7.1s
  yf0 = (vf0 >> 1); // conversion vf0: 0.8u -> yf0: 1.7s
  xf1 = lowByte(x>>1) - (x1<<6); // x1 is 7.1s
  // NOTE: yf1 could possibly be computed faster together with y1 above
  yf1 = lowByte(y>>1) - (v1<<7); // v1 is 8.0f, hence <<7
  xf2 = xf0 - 64;  // -0.5 in 1.7s
  yf2 = yf0 - 128; // -1 in 1.7s

  // Pick gradients from a small set of 8 directions    
  signed char g0x; // g0x: 1.7s
  signed char g0y; // g0y: 1.7s
  signed char g1x; // g1x: 1.7s
  signed char g1y; // g1y: 1.7s
  signed char g2x; // g2x: 1.7s
  signed char g2y; // g2y: 1.7s

  // Using +/-0.9921875 (+/-127) instead of
  // +0.9921875/-1.0 (+127/-128) for symmetry.
  // NOTE: These can be tweaked to scale the return value "for free".
  if(h0 & 0x02) {
    g0x = 64; g0y = 127;
  } else {
    g0x = 127; g0y = 64;
  };
  if(h0 & 0x08) {
    g0x = -g0x;
  };
  if(h0 & 0x04) {
    g0y = -g0y;
  };

  if(h1 & 0x02) {
    g1x = 64; g1y = 127;
  } else {
    g1x = 127; g1y = 64;
  };
  if(h1 & 0x08) {
    g1x = -g1x;
  };
  if(h1 & 0x04) {
    g1y = -g1y;
  };

  if(h2 & 0x02) {
    g2x = 64; g2y = 127;
  } else {
    g2x = 127; g2y = 64;
  };
  if(h2 & 0x08) {
    g2x = -g2x;
  };
  if(h2 & 0x04) {
    g2y = -g2y;
  };

  // Compute ramps (g dot u) from vertices
  signed char g0; // g0: 1.7s
  signed char g1; // g1: 1.7s
  signed char g2; // g2: 1.7s
  g0 = ((g0x*xf0)>>7) + ((g0y*yf0)>>7);
  g1 = ((g1x*xf1)>>7) + ((g1y*yf1)>>7);
  g2 = ((g2x*xf2)>>7) + ((g2y*yf2)>>7);
  // Note that g0/g1/g2 will overflow 1.7s at some corners, but
  // the overflow happens only in regions where m0/m1/m2 = 0 and
  // the incorrect, sign-flipped value is multiplied by zero.

  // Compute radial falloff from vertices
  unsigned char r0; // r0: 1.7u
  unsigned char m0; // m0: 0.8u
  r0 = ((xf0*xf0)>>7) + ((yf0*yf0)>>7);
  if(r0 > 102) {
    m0 = 0;
  } else {
    m0 = 255 - (r0<<1) - (r0>>1);
    // "(r0<<1)+(r0>>1)" is 1.25*r0 in 0.8u
    m0 = (m0*m0)>>8; // 8-bit by 8-bit to 8-bit 0.8u mult
    m0 = (m0*m0)>>8;
  }
  
  unsigned char r1; // r1: 1.7u
  unsigned char m1; // m1: 0.8u
  r1 = ((xf1*xf1)>>7) + ((yf1*yf1)>>7);
  if(r1 > 102) {
    m1 = 0;
  } else {
    m1 = 255 - (r1<<1) - (r1>>1);
    m1 = (m1*m1)>>8;
    m1 = (m1*m1)>>8;
  }

  unsigned char r2; // r2: 1.7u
  unsigned char m2; // m2: 0.8u
  r2 = ((xf2*xf2)>>7) + ((yf2*yf2)>>7);
  if(r2 > 102) {
    m2 = 0;
  } else {
    m2 = 255 - (r2<<1) - (r2>>1);
    m2 = (m2*m2)>>8;
    m2 = (m2*m2)>>8;
  }

  // Multiply ramps with falloffs
  signed char n0 = (g0 * m0)>>6; // g0: 1.7s, m0: 0.8u, n0: 1.7s, scale by 4
  signed char n1 = (g1 * m1)>>6; // mult scaled by 4
  signed char n2 = (g2 * m2)>>6; // mult scaled by 4
  // Factors gi, mi span their ranges and can't be scaled individually,
  // but all of the products (ni) are always < 0.25 and can be shifted
  // left 2 steps for two additional bits of precision.

  // Sum up noise contributions from all vertices
  signed char n; // n: 1.7s
  // Final tweak: scale n ever so slightly to cover the range [-1,1].
  // Multiply by 1.0625 in 1.7u (1.0001000), 136 in integer notation
  // n is 1.7s, factor 136 is 1.7u - can use "mixed" FMULSU instruction
  n = (136 * (n0 + n1 + n2)) >> 7;
  // NOTE: The classic trick n += (n>>4) is slower with a two-cycle HW multiply
  // and a lack of multi-step bit-shifts in the ATmega instruction set.

  // We're done. Return the result in 1.7s format.
  return n;
}
