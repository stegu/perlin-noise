// Simplex Noise written for 8-bit ARM CPUs, in "hardware-tuned C"
// Speed is a priority. Noise quality is of secondary concern.
// If you want pretty high-res pictures, this is not the code you seek.
// However, if you want to draw animated fire on a low resolution
// LED array with an Arduino, this code could be quite useful.

// The input coordinates are 16-bit unsigned integers in 7.8 bits
// representation: 0 to 65535 represent 0.0 to 127.9961 in steps
// of 1/256, with the pattern making a seamless tiling wrap at 128.
// The output value is a signed 8-bit integer in 1.7s representation:
// -128 to 127 represents -1 to 0.9922 in steps of 1/128.
// The output value stays within the range -1 to 1 by a small margin.

// Author: Stefan Gustavson (stefan.gustavson@gmail.com)
// Version 2022-06-12
// Released under the terms of the MIT license.

signed char armnoise8(unsigned short x, unsigned short y);
