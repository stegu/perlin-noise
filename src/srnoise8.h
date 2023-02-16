// srnoise8.h

// Simplex Flow Noise written for 8-bit ARM CPUs, in "hardware-tuned C"
// Speed is a priority. Noise quality is of secondary concern.
// If you want pretty high-res pictures, this is not the code you seek.
// However, if you want to draw animated fire on a low resolution
// LED array with an Arduino, this code could be quite useful.

// This function is very similar to "armnoise8", but it adds a
// parameter "alpha" to animate the noise pattern. This happens
// at a small cost in speed, so if you don't want the animation,
// "armnoise8" is slightly faster.

// The input coordinates are 16-bit unsigned integers in 7.8 bits
// representation: 0 to 32767 represent 0.0 to 127.9961 in steps
// of 1/256, with the pattern making a seamless tiling wrap at 128.
// The output value is a signed 8-bit integer in 1.7s representation:
// -128 to 127 represents -1 to 0.9922 in steps of 1/128.
// The output value stays within the range -1 to 1 by a small margin.

// Author: Stefan Gustavson (stefan.gustavson@gmail.com)
// Version 2023-02-18
// Released under the terms of the MIT license.

signed char fakesin(unsigned char x);

signed char fakecos(unsigned char x);

signed char srnoise8(unsigned short x, unsigned short y, unsigned char alpha);
