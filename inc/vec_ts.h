/*
    James William Fletcher (github.com/mrbid)
        September 2021

    Portable floating-point Vec3 lib with basic SSE support

    This is the THREAD-SAFE version.
*/

#ifndef VEC_H
#define VEC_H

#include <math.h>
#include <string.h>

#ifndef NOSSE
    #include <x86intrin.h>
#endif

#define PI 3.141592741f         // PI
#define x2PI 6.283185482f       // PI * 2
#define d2PI 1.570796371f       // PI / 2
#define DEGREE 57.29578018f     // 1 Radian as Degrees
#define RADIAN 0.01745329238f   // PI / 180 (1 Degree as Radians)
#define RAD2DEG DEGREE
#define DEG2RAD RADIAN

#define FLOAT_MAX 9223372036854775807.0f
#define INV_FLOAT_MAX 1.084202172e-19F

typedef struct{
    float x,y,z,w;
} vec;

static inline float rsqrtss(float f);
static inline float sqrtps(float f);
float randf(int *seed);  // uniform [0 to 1]
float randfc(int *seed); // uniform [-1 to 1]
float randfn(int *seed); // box-muller normal [bi-directional]
int vec_ftoi(float f); // float to integer quantise

// normalising the result is optional / at the callers responsibility
void vRuv(int *seed, vec* v);   // Random Unit Vector
void vRuvN(int *seed, vec* v);  // Normal Random Unit Vector
void vRuvBT(int *seed, vec* v); // Brian Tung Random Unit Vector (on surface of unit sphere)
void vRuvTA(int *seed, vec* v); // T.Davison Trial & Error (inside unit sphere)
void vRuvTD(int *seed, vec* v); // T.Davison Random Unit Vector Sphere

void  vCross(vec* r, const vec v1, const vec v2);
float vDot(const vec v1, const vec v2);
float vSum(const vec v);
void  vReflect(vec* r, const vec v, const vec n);

int  vEqualTol(const vec a, const vec b, const float tol);
int  vEqualInt(const vec a, const vec b);
void vMin(vec* r, const vec v1, const vec v2);
void vMax(vec* r, const vec v1, const vec v2);

void  vNorm(vec* v);
float vDist(const vec v1, const vec v2);
float vDistSq(const vec a, const vec b);
float vDistMh(const vec a, const vec b); // manhattan
float vDistLa(const vec a, const vec b); // longest axis
float vMod(const vec v); // modulus
float vMag(const vec v); // magnitude
void  vInv(vec* v); // invert
void  vCopy(vec* r, const vec v);
void  vDir(vec* r, const vec v1, const vec v2); // direction vector from v1 to v2

void vRotX(vec* v, const float radians);
void vRotY(vec* v, const float radians);
void vRotZ(vec* v, const float radians);

void vAdd(vec* r, const vec v1, const vec v2);
void vSub(vec* r, const vec v1, const vec v2);
void vDiv(vec* r, const vec numerator, const vec denominator);
void vMul(vec* r, const vec v1, const vec v2);

void vAddS(vec* r, const vec v1, const float v2);
void vSubS(vec* r, const vec v1, const float v2);
void vDivS(vec* r, const vec v1, const float v2);
void vMulS(vec* r, const vec v1, const float v2);

//

static inline float rsqrtss(float f)
{
#ifdef NOSSE
    return 1.f/sqrtf(f);
#else
    return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(f)));
#endif
}

static inline float sqrtps(float f)
{
#ifdef NOSSE
    return sqrtf(f);
#else
    return _mm_cvtss_f32(_mm_sqrt_ps(_mm_set_ss(f)));
#endif
}

// https://www.musicdsp.org/en/latest/Other/273-fast-float-random-numbers.html
// moc.liamg@seir.kinimod
float randf(int *seed)
{
    *seed *= 16807;
    return (float)(*seed & 0x7FFFFFFF) * 4.6566129e-010f;
}
float randfc(int *seed)
{
    *seed *= 16807;
    return ((float)(*seed)) * 4.6566129e-010f;
}
float randfn(int *seed)
{
    float u = randfc(seed);
    float v = randfc(seed);
    float r = u * u + v * v;
    while(r == 0.f || r > 1.f)
    {
        u = randfc(seed);
        v = randfc(seed);
        r = u * u + v * v;
    }
    return u * sqrtps(-2.f * logf(r) / r);
}

void vRuv(int *seed, vec* v)
{
    v->x = randfc(seed);
    v->y = randfc(seed);
    v->z = randfc(seed);
}

void vRuvN(int *seed, vec* v)
{
    v->x = randfn(seed);
    v->y = randfn(seed);
    v->z = randfn(seed);
}

void vRuvBT(int *seed, vec* v)
{
    // https://math.stackexchange.com/a/1586185
    // or should I have called this vRuvLR()
    // https://mathworld.wolfram.com/SpherePointPicking.html
    const float y = acosf(randfc(seed)) - d2PI;
    const float p = x2PI * randf(seed);
    v->x = cosf(y) * cosf(p);
    v->y = cosf(y) * sinf(p);
    v->z = sinf(y);
}

void vRuvTA(int *seed, vec* v)
{
    // T.P.Davison@tees.ac.uk
    while(1)
    {
        v->x = randfc(seed);
        v->y = randfc(seed);
        v->z = randfc(seed);
        const float len = vMag(*v);
        if(len <= 1.0f){return;}
    }
}

void vRuvTD(int *seed, vec* v)
{
    // T.P.Davison@tees.ac.uk
    v->x = sinf((randf(seed) * x2PI) - PI);
    v->y = cosf((randf(seed) * x2PI) - PI);
    v->z = randfc(seed);
}

void vCross(vec* r, const vec v1, const vec v2)
{
    r->x = (v1.y * v2.z) - (v2.y * v1.z);
    r->y = -((v1.x * v2.z) - (v2.x * v1.z));
    r->z = (v1.x * v2.y) - (v2.x * v1.y);
}

float vDot(const vec v1, const vec v2)
{
    return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
}

float vSum(const vec v)
{
    return v.x + v.y + v.z;
}

void vInv(vec* v)
{
    v->x = -v->x;
    v->y = -v->y;
    v->z = -v->z;
}

void vNorm(vec* v)
{
    const float len = rsqrtss(v->x*v->x + v->y*v->y + v->z*v->z);
    v->x *= len;
    v->y *= len;
    v->z *= len;
}

float vDist(const vec v1, const vec v2)
{
    const float xm = (v1.x - v2.x);
    const float ym = (v1.y - v2.y);
    const float zm = (v1.z - v2.z);
    return sqrtps(xm*xm + ym*ym + zm*zm);
}

float vDistSq(const vec a, const vec b)
{
    const float xm = (a.x - b.x);
    const float ym = (a.y - b.y);
    const float zm = (a.z - b.z);
    return xm*xm + ym*ym + zm*zm;
}

float vDistMh(const vec a, const vec b)
{
    return (a.x - b.x) + (a.y - b.y) + (a.z - b.z);
}

float vDistLa(const vec v1, const vec v2)
{
    const float xm = fabsf(v1.x - v2.x);
    const float ym = fabsf(v1.y - v2.y);
    const float zm = fabsf(v1.z - v2.z);

    float dist = xm;
    if(ym > dist)
        dist = ym;
    if(zm > dist)
        dist = zm;

    return dist;
}

void vReflect(vec* r, const vec v, const vec n)
{
    const float angle = vDot(v, n);
    r->x = v.x - (2.f * n.x) * angle;
    r->y = v.y - (2.f * n.y) * angle;
    r->z = v.z - (2.f * n.z) * angle;
}

int vEqualTol(const vec a, const vec b, const float tol)
{
    if( a.x >= b.x - tol && a.x <= b.x + tol &&
        a.y >= b.y - tol && a.y <= b.y + tol &&
        a.z >= b.z - tol && a.z <= b.z + tol )
        return 1;
    else
        return 0;
}

void vMin(vec* r, const vec v1, const vec v2)
{
    if(v1.x < v2.x && v1.y < v2.y && v1.z < v2.z)
    {
        r->x = v1.x;
        r->y = v1.y;
        r->z = v1.z;
    }

    r->x = v2.x;
    r->y = v2.y;
    r->z = v2.z;
}

void vMax(vec* r, const vec v1, const vec v2)
{
    if(v1.x > v2.x && v1.y > v2.y && v1.z > v2.z)
    {
        r->x = v1.x;
        r->y = v1.y;
        r->z = v1.z;
    }

    r->x = v2.x;
    r->y = v2.y;
    r->z = v2.z;
}

int vec_ftoi(float f)
{
    if(f < 0.f)
        f -= 0.5f;
    else
        f += 0.5f;
    return (int)f;
}

int vEqualInt(const vec a, const vec b)
{
    if(vec_ftoi(a.x) == vec_ftoi(b.x) && vec_ftoi(a.y) == vec_ftoi(b.y) && vec_ftoi(a.z) == vec_ftoi(b.z))
        return 1;
    else
        return 0;
}

float vMod(const vec v)
{
    return sqrtps(v.x*v.x + v.y*v.y + v.z*v.z);
}

float vMag(const vec v)
{
    return v.x*v.x + v.y*v.y + v.z*v.z;
}

void vCopy(vec* r, const vec v)
{
    memcpy(r, &v, sizeof(vec));
}

void vDir(vec* r, const vec v1, const vec v2)
{
    vSub(r, v2, v1);
    vNorm(r);
}

void vRotX(vec* v, const float radians)
{
    v->y = v->y * cosf(radians) + v->z * sinf(radians);
    v->z = v->y * sinf(radians) - v->z * cosf(radians);
}

void vRotY(vec* v, const float radians)
{
    v->x = v->z * sinf(radians) - v->x * cosf(radians);
    v->z = v->z * cosf(radians) + v->x * sinf(radians);
}

void vRotZ(vec* v, const float radians)
{
    v->x = v->x * cosf(radians) + v->y * sinf(radians);
    v->y = v->x * sinf(radians) - v->y * cosf(radians);
}

void vAdd(vec* r, const vec v1, const vec v2)
{
    r->x = v1.x + v2.x;
    r->y = v1.y + v2.y;
    r->z = v1.z + v2.z;
}

void vSub(vec* r, const vec v1, const vec v2)
{
    r->x = v1.x - v2.x;
    r->y = v1.y - v2.y;
    r->z = v1.z - v2.z;
}

void vDiv(vec* r, const vec numerator, const vec denominator)
{
    r->x = numerator.x / denominator.x;
    r->y = numerator.y / denominator.y;
    r->z = numerator.z / denominator.z;
}

void vMul(vec* r, const vec v1, const vec v2)
{
    r->x = v1.x * v2.x;
    r->y = v1.y * v2.y;
    r->z = v1.z * v2.z;
}

void vAddS(vec* r, const vec v1, const float v2)
{
    r->x = v1.x + v2;
    r->y = v1.y + v2;
    r->z = v1.z + v2;
}

void vSubS(vec* r, const vec v1, const float v2)
{
    r->x = v1.x - v2;
    r->y = v1.y - v2;
    r->z = v1.z - v2;
}

void vDivS(vec* r, const vec v1, const float v2)
{
    r->x = v1.x / v2;
    r->y = v1.y / v2;
    r->z = v1.z / v2;
}

void vMulS(vec* r, const vec v1, const float v2)
{
    r->x = v1.x * v2;
    r->y = v1.y * v2;
    r->z = v1.z * v2;
}

#endif
