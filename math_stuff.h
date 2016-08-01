#ifndef math_stuff_h
#define math_stuff_h

#include "types.h"

#define atLeast(x, y) max(x, y)
#define atMost(x, y) min(x, y)
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))



//
// Miscellaneous
//

f32
lerp(f32 a, f32 t, f32 b)
{
	return (a + t * (b - a));
}

int
hexagonNumber(int k)
{
	return (1 + 3 * k * (k - 1));
}

int
mod(int a, int b)
{
	int result = a % b;
	if (result < 0)
	{
		result += b;
	}
	return result;
}

//
// Vector
//
 
struct V2 {
    f32 x, y;
};
 
inline V2
v2(f32 x, f32 y)
{
    V2 result = {x, y};
    return result;
}
 
inline V2
operator * (f32 a, V2 b)
{
    V2 result;
     
    result.x = a * b.x;
    result.y = a * b.y;
     
    return result;
}
 
inline V2
operator * (V2 b, f32 a)
{
    return a * b;
}
 
inline V2
operator *= (V2 &b, f32 a)
{
    b = a * b;
    return b;
}
 
inline V2
operator / (V2 a, f32 b)
{
    return a * (1.0f / b);
}
 
inline V2
operator /= (V2 &a, f32 b)
{
    a = a / b;
    return a;
}
 
inline V2
operator + (V2 a, V2 b)
{
    V2 result;
     
    result.x = a.x + b.x;
    result.y = a.y + b.y;
     
    return result;
}
 
inline V2
operator += (V2 &b, V2 a)
{
    b = b + a;
    return b;
}
 
inline V2
operator - (V2 a)
{
    V2 result;
    result.x = - a.x;
    result.y = - a.y;
    return result;
}
 
inline V2
operator - (V2 a, V2 b)
{
    V2 result;
     
    result.x = a.x - b.x;
    result.y = a.y - b.y;
     
    return result;
}
 
inline V2
operator -= (V2 &b, V2 a)
{
    b = b - a;
    return b;
}
 
inline f32
inner(V2 a, V2 b)
{
    return a.x * b.x + a.y * b.y;
}
 
inline f32
outer(V2 a, V2 b)
{
    return a.x * b.y - a.y * b.x;
}
 
inline f32
square(f32 a)
{
    return a * a;
}

inline f64
square(f64 a)
{
    return a * a;
}
 
inline f32
square(V2 a)
{
    return inner(a, a);
}
 
 
// TODO(pontus): get rid of this epsilon
inline bool
operator == (V2 a, V2 b)
{
    return square(a - b) < 0.001f;
}
 
inline V2
normalize(V2 a)
{
    f32 sq = square(a);
    // TODO(pontus): should this be epsilon?
    if (sq > 0) {
        return a * (1.0f / sqrt(sq));
    }
    else
    {
        return a;
    }
}
 
inline bool
isNonZero(V2 a)
{
    return (a.x != 0) || (a.y != 0);
}
 
inline V2
normalFromVector(V2 a)
{
    // NOTE(pontus): on the right side of the vector
    return normalize( v2(a.y, -a.x) );
}

inline V2
v2FromAngle(f32 angle)
{
    return v2(cos(angle), sin(angle));
}
 
bool
ccw(V2 a, V2 b, V2 c)
{
    return outer(b - a, c - b) > 0;
}

V2
periodize(V2 a, f32 width, f32 height)
{
    a.x -= width * round(a.x / width);
    a.y -= height * round(a.y / height);
    return a;
}

void printV2(V2 v)
{
    printf("%.2f, %.2f\n", v.x, v.y);
}

//
// Colors
// 

struct Color4 {
    f32 r, g, b, a;
};

Color4
c4(f32 r, f32 g, f32 b, f32 a)
{
    Color4 color = {r, g, b, a};
    return color;
}

//
// Random Number Generation
//

f32
randomF32()
{
    return ((f32)rand() / RAND_MAX);
}

f32
randomBetween(f32 a, f32 b)
{
    return lerp(a, randomF32(), b);
}

f32
randomGaussian()
{

    // NOTE: this algorithm generates 2 random numbers at a time
    // we save the second one, so every other call is faster
    local_persist bool isPrepared;
    local_persist f32 preparedValue;

    if (isPrepared)
    {
        isPrepared = false;
		return preparedValue;
    }

    // Taken from http://www.design.caltech.edu/erik/Misc/Gaussian.html
    f32 x, y, w;
    do
    {
        x = 2 * randomF32() - 1;
        y = 2 * randomF32() - 1;
        w = x * x + y * y;
    } while (w >= 1);

    f32 factor = sqrt( (-2.0 * log(w)) / w);
	preparedValue = x * factor;
    return (y * factor);
}




#endif