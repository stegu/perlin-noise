//
// armnoise8.c
//
// 2-D simplex noise for low-performance 8-bit CPUs (Arduino etc)
//
// Copyright 2022 Stefan Gustavson (stefan.gustavson@gmail.com)
// Released under the terms of the MIT license:
// https://opensource.org/licenses/MIT
// Version: 2022-06-08
//
// An implementation of 2-D "Simplex Noise" written for 8-bit
// CPUs, in "hardware-tuned C" made to run fast on an ATmega32.
// Intended use cases are low resolution LED array animations
// and procedural sound generation.
// A 16 MHz Arduino Uno or Arduino Micro can't churn out more than
// around 25,000 noise values per second, but you can do quite a
// lot with that.
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
// The code should compile and run on any CPU architecture
// with the inclusion of the macros below.

// Disable these on GCC-AVR, where they are already defined
#define highByte(w) ((w)>>8)
#define lowByte(w) ((w) & 0xFF)

signed char armnoise8(unsigned short x, unsigned short y) {

    // x and y are inherently 8.8u modulo-256, but are treated
	// algoritmically as 7.8u modulo-128 to remove the need for
	// a couple of 16-bit integers in the code.
	x = x & 0x7FFF; // Not sure if the compiler understands that
	y = y & 0x7FFF; // it doesn't need to touch the low byte here...

    // Skew input coords to staggered grid
    unsigned short u = (x) + (y>>1); // u: 8.8u
    unsigned short v = y; // v is alias of y: 7.8u

    // Split to integer and fractional parts
    unsigned char u0 = highByte(u); // u0: 8.0u
    unsigned char v0 = highByte(v); // v0: 8.0u (or 7.0u, MSB = 0)

    unsigned char uf0 = lowByte(u); // uf0: 0.8u
    unsigned char vf0 = lowByte(v); // vf0: 0.8u
    
    // Determine the second vertex for the simplex
	unsigned char u1;
	unsigned char v1;
    if(uf0 > vf0) {
        u1 = u0 + 1; // u1: 8.0u
        v1 = v0;     // v1: 8.0u - see below
    } else {
        u1 = u0;
        v1 = v0 + 1; // This requires 8.0u for v1 (but only just)
    }

    // Third vertex is always (+1, +1)
    unsigned char u2 = u0 + 1; // u2: 8.0u
    unsigned char v2 = v0 + 1; // v2: 8.0u
	    
    // Transform ui,vi back to x,y coords before wrap
    signed char x0 = (u0<<1) - v0; // x0: 7.1s
	// x0 varies in steps of 0.5 from -0.5 to 126.5
    unsigned char y0 = v0; // y0 alias of v0: 8.0u (or 7.0u)
    
    signed short x1 = (u1<<1) - v1; // x1 is "special": 8.1s
	// x1 varies in steps of 0.5 from -0.5 to 127.5,
	// hence 8.1s is required, but only just. With a further reduced
	// x,y domain of 6.8u, x1 would be 7.1s and fit in a signed char.
	// (It's the only remaining 16-bit variable, though. Not a big deal.)
    unsigned char y1 = v1; // y1 is alias of v1: 8.0u

    unsigned char x2 = (u2<<1) - v2; // x2: 7.1u, will never be negative
    unsigned char y2 = v2; // y2 is alias of v2: 8.0u

    // Compute vectors in x,y coords from vertices
    signed char xf0 = lowByte(x>>1) - (x0<<6); // x: 8.8u, x0: 7.1u, xf0: 1.7s
    signed char yf0 = lowByte(y>>1) - (y0<<7); // yf0: 1.7s
    signed char xf1 = lowByte(x>>1) - (x1<<6); // xf1: 1.7s
    signed char yf1 = lowByte(y>>1) - (y1<<7); // yf1: 1.7s
    signed char xf2 = lowByte(x>>1) - (x2<<6); // xf2: 1.7s
	// xf2 is always xf0 - 0.5, room for optimization
    signed char yf2 = lowByte(y>>1) - (y2<<7); // yf2: 1.7s
	// always yf0 - 1, room for optimization
    
    // Generate vertex hashes from ui, vi
	unsigned char hash0; // hash0: 8.0u
	hash0 = (13*u0 + 7)*u0;
	hash0 = hash0 + v0;
	hash0 = (15*hash0 + 11)*hash0;

	// TODO: When u1=u0, we could reuse the u hash from hash0
	unsigned char hash1; // hash1: 8.0u
	hash1 = (13*u1 + 7)*u1;
	hash1 = hash1 + v1;
	hash1 = (15*hash1 + 11)*hash1;

	// TODO: When u1=u0+1, we could reuse the u hash from hash1
    unsigned char hash2; // hash2: 8.0u
	hash2 = (13*u2 + 7)*u2;
	hash2 = hash2 + v2;
	hash2 = (15*hash2 + 11)*hash2;
	
    // Pick gradients from a small set of 8 directions    
	signed char g0x; // All these: 1.7s
	signed char g0y;
	signed char g1x;
	signed char g1y;
	signed char g2x;
	signed char g2y;

    // using +/-0.9921875 (+/-127) instead of
	// +0.9921875/-1.0 (+127/-128) for symmetry
    if (hash0 & 0x01) {
        g0x = 64; g0y = 127;
    } else {
        g0x = 127; g0y = 64;
    };
    if(hash0 & 0x02) {
        g0x = -g0x;
    };
    if (hash0 & 0x04) {
        g0y = -g0y;
    };

    if (hash1 & 0x01) {
        g1x = 64; g1y = 127;
    } else {
        g1x = 127; g1y = 64;
    };
    if(hash1 & 0x02) {
        g1x = -g1x;
    };
    if (hash1 & 0x04) {
        g1y = -g1y;
    };

    if (hash2 & 0x01) {
        g2x = 64; g2y = 127;
    } else {
        g2x = 127; g2y = 64;
    };
    if(hash2 & 0x02) {
        g2x = -g2x;
    };
    if (hash2 & 0x04) {
        g2y = -g2y;
    };

    // Compute ramps (g dot u) from vertices
    signed char g0 = ((g0x*xf0)>>7) + ((g0y*yf0)>>7); // g0: 1.7s
    signed char g1 = ((g1x*xf1)>>7) + ((g1y*yf1)>>7); // g1: 1.7s
    signed char g2 = ((g2x*xf2)>>7) + ((g2y*yf2)>>7); // g2: 1.7s
    // Note that g0/g1/g2 will overflow 1.7s at some corners, but
	// the overflow happens only in regions where m0/m1/m2 = 0 and
	// the incorrect, sign-flipped value is multiplied by zero.

    // Compute radial falloff from vertices
	unsigned char r0 = ((xf0*xf0)>>7) + ((yf0*yf0)>>7); // r0: 1.7u
    unsigned char m0;
	if(r0 > 102) {
		m0 = 0;
	} else {
		m0 = 255 - (r0<<1) - (r0>>1);
		// m0: 0.8u, "(r0<<1)+(r0>>1)" is 1.25*r0 in 0.8u
        m0 = (m0*m0)>>8; // 8-bit by 8-bit to 8-bit 0.8u mult
        m0 = (m0*m0)>>8;
	}
	
	unsigned char r1 = ((xf1*xf1)>>7) + ((yf1*yf1)>>7); // r1: 1.7u
    unsigned char m1;
	if(r1 > 102) {
		m1 = 0;
	} else {
		m1 = 255 - (r1<<1) - (r1>>1); // m1: 0.8u
        m1 = (m1*m1)>>8;
        m1 = (m1*m1)>>8;
	}

	unsigned char r2 = ((xf2*xf2)>>7) + ((yf2*yf2)>>7); // r2: 0.8u
    unsigned char m2;
	if(r2 > 102) {
		m2 = 0;
	} else {
		m2 = 255 - (r2<<1) - (r2>>1); // m2: 0.8u
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
    // Multiplications in ATmega32 are 8-by-8-to-16 bits, but selecting
    // the "best" bits requires a few extra operations.

    // Sum up noise contributions from all vertices
    signed char n; // n: 1.7s
    // Optional step: scale n ever so slightly to cover the range [-1,1].
    // Multiply by 1.015625 in 1.7u (1.0000010), 130 in integer notation
	// n is 1.7s, factor 130 is 1.7u - can use "mixed" FMULSU instruction
	n = (130 * (n0 + n1 + n2))>>7;
	// Alternative expression for the scaling:
	//n = n0 + n1 + n2;
	//n += (n>>6); // Equivalent to a mult by 1.000001
	// (The shift-add is not faster if a 2-cycle HW multiplier is present)

	// We're done. Return the result in 1.7s format.
	return n;
}
