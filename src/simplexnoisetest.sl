/*
 * simplexnoisetest.sl : simple shader to test the simplex noise DSO
 *
 * Lighting has absolutely no influence on this shader.
 * The colors are always fully bright.
 */

surface simplexnoisetest() {

 color white = color( 1.0, 1.0, 1.0 );
 color black  = color( 0.0, 0.0, 0.0 );

 float weight = simplexnoise(8.0*P);

 Ci = mix(black, white, weight);
 Oi = Os;
}
