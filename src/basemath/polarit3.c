/* $Id$

Copyright (C) 2000-2004  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/***********************************************************************/
/**                                                                   **/
/**               ARITHMETIC OPERATIONS ON POLYNOMIALS                **/
/**                         (third part)                              **/
/**                                                                   **/
/***********************************************************************/
#include "pari.h"

#define lswap(x,y) {long _z=x; x=y; y=_z;}
#define pswap(x,y) {GEN *_z=x; x=y; y=_z;}
#define swap(x,y)  {GEN  _z=x; x=y; y=_z;}
#define swapspec(x,y, nx,ny) {swap(x,y); lswap(nx,ny);}
#define both_odd(x,y) ((x)&(y)&1)
extern GEN caractducos(GEN p, GEN x, int v);
extern double mylog2(GEN z);
extern GEN revpol(GEN x);

/*******************************************************************/
/*                                                                 */
/*                  KARATSUBA (for polynomials)                    */
/*                                                                 */
/*******************************************************************/

#if 1 /* for tunings */
long SQR_LIMIT = 6;
long MUL_LIMIT = 10;
long u_SQR_LIMIT = 6;
long u_MUL_LIMIT = 10;

void
setsqpol(long a) { SQR_LIMIT=a; u_SQR_LIMIT=a; }
void
setmulpol(long a) { MUL_LIMIT=a; u_MUL_LIMIT=a; }

GEN
u_specpol(GEN x, long nx)
{
  GEN z = cgetg(nx+2,t_POL);
  long i;
  for (i=0; i<nx; i++) z[i+2] = lstoi(x[i]);
  z[1] = evalsigne(1); return z;
}

GEN
specpol(GEN x, long nx)
{
  GEN z = cgetg(nx+2,t_POL);
  long i;
  for (i=0; i<nx; i++) z[i+2] = x[i];
  z[1] = evalsigne(1); return z;
}
#else
#  define SQR_LIMIT 6
#  define MUL_LIMIT 10
#endif

/* generic multiplication */

static GEN
addpol(GEN x, GEN y, long lx, long ly)
{
  long i,lz;
  GEN z;

  if (ly>lx) swapspec(x,y, lx,ly);
  lz = lx+2; z = cgetg(lz,t_POL) + 2;
  for (i=0; i<ly; i++) z[i]=ladd((GEN)x[i],(GEN)y[i]);
  for (   ; i<lx; i++) z[i]=x[i];
  z -= 2; z[1]=0; return normalizepol_i(z, lz);
}

static GEN
addpolcopy(GEN x, GEN y, long lx, long ly)
{
  long i,lz;
  GEN z;

  if (ly>lx) swapspec(x,y, lx,ly);
  lz = lx+2; z = cgetg(lz,t_POL) + 2;
  for (i=0; i<ly; i++) z[i]=ladd((GEN)x[i],(GEN)y[i]);
  for (   ; i<lx; i++) z[i]=lcopy((GEN)x[i]);
  z -= 2; z[1]=0; return normalizepol_i(z, lz);
}

INLINE GEN
mulpol_limb(GEN x, GEN y, char *ynonzero, long a, long b)
{
  GEN p1 = NULL;
  long i;
  pari_sp av = avma;
  for (i=a; i<b; i++)
    if (ynonzero[i])
    {
      GEN p2 = gmul((GEN)y[i],(GEN)x[-i]);
      p1 = p1 ? gadd(p1, p2): p2;
    }
  return p1 ? gerepileupto(av, p1): gzero;
}

/* assume nx >= ny > 0 */
static GEN
mulpol(GEN x, GEN y, long nx, long ny)
{
  long i,lz,nz;
  GEN z;
  char *p1;

  lz = nx+ny+1; nz = lz-2;
  z = cgetg(lz, t_POL) + 2; /* x:y:z [i] = term of degree i */
  p1 = gpmalloc(ny);
  for (i=0; i<ny; i++)
  {
    p1[i] = !isexactzero((GEN)y[i]);
    z[i] = (long)mulpol_limb(x+i,y,p1,0,i+1);
  }
  for (  ; i<nx; i++) z[i] = (long)mulpol_limb(x+i,y,p1,0,ny);
  for (  ; i<nz; i++) z[i] = (long)mulpol_limb(x+i,y,p1,i-nx+1,ny);
  free(p1); z -= 2; z[1]=0; return normalizepol_i(z, lz);
}

/* return (x * X^d) + y. Assume d > 0, x > 0 and y >= 0 */
GEN
addmulXn(GEN x, GEN y, long d)
{
  GEN xd,yd,zd = (GEN)avma;
  long a,lz,ny = lgpol(y), nx = lgpol(x);

  x += 2; y += 2; a = ny-d;
  if (a <= 0)
  {
    lz = (a>nx)? ny+2: nx+d+2;
    (void)new_chunk(lz); xd = x+nx; yd = y+ny;
    while (xd > x) *--zd = *--xd;
    x = zd + a;
    while (zd > x) *--zd = zero;
  }
  else
  {
    xd = new_chunk(d); yd = y+d;
    x = addpol(x,yd, nx,a);
    lz = (a>nx)? ny+2: lg(x)+d;
    x += 2; while (xd > x) *--zd = *--xd;
  }
  while (yd > y) *--zd = *--yd;
  *--zd = evalsigne(1);
  *--zd = evaltyp(t_POL) | evallg(lz); return zd;
}

GEN
addshiftpol(GEN x, GEN y, long d)
{
  long v = varn(x);
  if (!signe(x)) return y;
  x = addmulXn(x,y,d);
  setvarn(x,v); return x;
}

/* as above, producing a clean malloc */
static GEN
addmulXncopy(GEN x, GEN y, long d)
{
  GEN xd,yd,zd = (GEN)avma;
  long a,lz,ny = lgpol(y), nx = lgpol(x);

  x += 2; y += 2; a = ny-d;
  if (a <= 0)
  {
    lz = nx+d+2;
    (void)new_chunk(lz); xd = x+nx; yd = y+ny;
    while (xd > x) *--zd = lcopy((GEN)*--xd);
    x = zd + a;
    while (zd > x) *--zd = zero;
  }
  else
  {
    xd = new_chunk(d); yd = y+d;
    x = addpolcopy(x,yd, nx,a);
    lz = (a>nx)? ny+2: lg(x)+d;
    x += 2; while (xd > x) *--zd = *--xd;
  }
  while (yd > y) *--zd = lcopy((GEN)*--yd);
  *--zd = evalsigne(1);
  *--zd = evaltyp(t_POL) | evallg(lz); return zd;
}

/* shift polynomial in place. assume v free cells have been left before x */
static GEN
shiftpol_ip(GEN x, long v)
{
  long i, lx;
  GEN y;
  if (v <= 0 || !signe(x)) return x;
  lx = lg(x);
  x += 2; y = x + v;
  for (i = lx-3; i>=0; i--) y[i] = x[i];
  for (i = 0   ; i< v; i++) x[i] = zero;
  lx += v;
  *--x = evalsigne(1);
  *--x = evaltyp(t_POL) | evallg(lx); return x;
}

/* fast product (Karatsuba) of polynomials a,b. These are not real GENs, a+2,
 * b+2 were sent instead. na, nb = number of terms of a, b.
 * Only c, c0, c1, c2 are genuine GEN.
 */
static GEN
quickmul(GEN a, GEN b, long na, long nb)
{
  GEN a0,c,c0;
  long n0, n0a, i, v = 0;
  pari_sp av;

  while (na && isexactzero((GEN)a[0])) { a++; na--; v++; }
  while (nb && isexactzero((GEN)b[0])) { b++; nb--; v++; }
  if (na < nb) swapspec(a,b, na,nb);
  if (!nb) return zeropol(0);

  if (v) (void)cgetg(v,t_STR); /* v gerepile-safe cells for shiftpol_ip */
  if (nb < MUL_LIMIT)
    return shiftpol_ip(mulpol(a,b,na,nb), v);
  i=(na>>1); n0=na-i; na=i;
  av=avma; a0=a+n0; n0a=n0;
  while (n0a && isexactzero((GEN)a[n0a-1])) n0a--;

  if (nb > n0)
  {
    GEN b0,c1,c2;
    long n0b;

    nb -= n0; b0 = b+n0; n0b = n0;
    while (n0b && isexactzero((GEN)b[n0b-1])) n0b--;
    c = quickmul(a,b,n0a,n0b);
    c0 = quickmul(a0,b0, na,nb);

    c2 = addpol(a0,a, na,n0a);
    c1 = addpol(b0,b, nb,n0b);

    c1 = quickmul(c1+2,c2+2, lgpol(c1),lgpol(c2));
    c2 = gneg_i(gadd(c0,c));
    c0 = addmulXn(c0, gadd(c1,c2), n0);
  }
  else
  {
    c = quickmul(a,b,n0a,nb);
    c0 = quickmul(a0,b,na,nb);
  }
  c0 = addmulXncopy(c0,c,n0);
  return shiftpol_ip(gerepileupto(av,c0), v);
}

static GEN
sqrpol(GEN x, long nx)
{
  long i, j, l, lz, nz;
  pari_sp av;
  GEN p1,z;
  char *p2;

  if (!nx) return zeropol(0);
  lz = (nx << 1) + 1, nz = lz-2;
  z = cgetg(lz,t_POL) + 2;
  p2 = gpmalloc(nx);
  for (i=0; i<nx; i++)
  {
    p2[i] = !isexactzero((GEN)x[i]);
    p1=gzero; av=avma; l=(i+1)>>1;
    for (j=0; j<l; j++)
      if (p2[j] && p2[i-j])
        p1 = gadd(p1, gmul((GEN)x[j],(GEN)x[i-j]));
    p1 = gshift(p1,1);
    if ((i&1) == 0 && p2[i>>1])
      p1 = gadd(p1, gsqr((GEN)x[i>>1]));
    z[i] = lpileupto(av,p1);
  }
  for (  ; i<nz; i++)
  {
    p1=gzero; av=avma; l=(i+1)>>1;
    for (j=i-nx+1; j<l; j++)
      if (p2[j] && p2[i-j])
        p1 = gadd(p1, gmul((GEN)x[j],(GEN)x[i-j]));
    p1 = gshift(p1,1);
    if ((i&1) == 0 && p2[i>>1])
      p1 = gadd(p1, gsqr((GEN)x[i>>1]));
    z[i] = lpileupto(av,p1);
  }
  free(p2); z -= 2; z[1]=0; return normalizepol_i(z,lz);
}

static GEN
quicksqr(GEN a, long na)
{
  GEN a0,c,c0,c1;
  long n0, n0a, i, v = 0;
  pari_sp av;

  while (na && isexactzero((GEN)a[0])) { a++; na--; v += 2; }
  if (v) (void)new_chunk(v);
  if (na<SQR_LIMIT) return shiftpol_ip(sqrpol(a,na), v);
  i=(na>>1); n0=na-i; na=i;
  av=avma; a0=a+n0; n0a=n0;
  while (n0a && isexactzero((GEN)a[n0a-1])) n0a--;

  c = quicksqr(a,n0a);
  c0 = quicksqr(a0,na);
  c1 = gmul2n(quickmul(a0,a, na,n0a), 1);
  c0 = addmulXn(c0,c1, n0);
  c0 = addmulXncopy(c0,c,n0);
  return shiftpol_ip(gerepileupto(av,c0), v);
}

/*Renormalize (in place) polynomial with t_INT or t_POL coefficients.*/

GEN
ZX_renormalize(GEN x, long lx)
{
  long i;
  for (i = lx-1; i>1; i--)
    if (signe((GEN)x[i])) break;
  stackdummy(x + (i+1), lg(x) - (i+1));
  setlg(x, i+1); setsigne(x, i!=1); return x;
}

#define FpX_renormalize ZX_renormalize
#define FpXX_renormalize ZX_renormalize
#define FpXQX_renormalize ZX_renormalize

/************************************************************************
 **                                                                    ** 
 **                      Arithmetic in Z/pZ[X]                         **
 **                                                                    **
 ************************************************************************/

/* In practice, p is not assumed to be prime. */

/*********************************************************************
These functions suppose polynomials to be already reduced.
They are clean and memory efficient.
**********************************************************************/

GEN
FpX_center(GEN T,GEN mod)
{/*OK centermod exists, but is not so clean*/
  pari_sp av;
  long i, l=lg(T);
  GEN P,mod2;
  P=cgetg(l,t_POL);
  P[1]=T[1];
  av=avma;
  mod2=gclone(shifti(mod,-1));/*clone*/
  avma=av;
  for(i=2;i<l;i++)
    P[i]=cmpii((GEN)T[i],mod2)<0?licopy((GEN)T[i]):lsubii((GEN)T[i],mod);
  gunclone(mod2);/*unclone*/
  return P;
}

GEN
FpX_neg(GEN x,GEN p)
{
  long i,d=lg(x);
  GEN y;
  y=cgetg(d,t_POL); y[1]=x[1];
  for(i=2;i<d;i++)
    if (signe(x[i])) y[i]=lsubii(p,(GEN)x[i]);
    else y[i]=zero;
  return y;
}
/**********************************************************************
Unclean functions, do not garbage collect.
This is a feature: The malloc is corrupted only by the call to FpX_red
so garbage collecting so often is not desirable.
FpX_red can sometime be avoided by passing NULL for p.
In this case the function is usually clean (see below for detail)
Added to help not using POLMOD of INTMOD which are deadly slow.
gerepileupto of the result is legible.   Bill.
I don't like C++.  I am wrong.
**********************************************************************/
/*
 *If p is NULL no reduction is performed and the function is clean.
 * for FpX_add,FpX_mul,FpX_sqr,FpX_Fp_mul
 */
GEN
FpX_add(GEN x,GEN y,GEN p)
{
  long lx,ly,i;
  GEN z;
  lx = lg(x); ly = lg(y); if (lx < ly) swapspec(x,y, lx,ly);
  z = cgetg(lx,t_POL); z[1] = x[1];
  for (i=2; i<ly; i++) z[i]=laddii((GEN)x[i],(GEN)y[i]);
  for (   ; i<lx; i++) z[i]=licopy((GEN)x[i]);
  z = FpX_renormalize(z, lx);
  if (!lgpol(z)) { avma = (pari_sp)(z + lx); return zeropol(varn(x)); }
  if (p) z = FpX_red(z, p);
  return z;
}

GEN
FpX_sub(GEN x,GEN y,GEN p)
{
  long lx,ly,i,lz;
  GEN z;
  lx = lg(x); ly = lg(y);
  lz=max(lx,ly);
  z = cgetg(lz,t_POL);
  if (lx >= ly)
  {
    z[1] = x[1];
    for (i=2; i<ly; i++) z[i]=lsubii((GEN)x[i],(GEN)y[i]);
    for (   ; i<lx; i++) z[i]=licopy((GEN)x[i]);
    if (p) z = FpX_red(z, p);
    else   z = FpX_renormalize(z, lz);
  }
  else
  {
    z[1] = y[1];
    for (i=2; i<lx; i++) z[i]=lsubii((GEN)x[i],(GEN)y[i]);
    for (   ; i<ly; i++) z[i]=lnegi((GEN)y[i]);
    if (p) z = FpX_red(z, p);
    /*polynomial is always normalized*/
  }
  if (!lgpol(z)) { avma = (pari_sp)(z + lz); z = zeropol(varn(x)); }
  return z;
}
GEN
FpX_mul(GEN x,GEN y,GEN p)
{
  GEN z = quickmul(y+2, x+2, lgpol(y), lgpol(x));
  setvarn(z,varn(x));
  if (!p) return z;
  return FpX_red(z, p);
}
GEN
FpX_sqr(GEN x,GEN p)
{
  GEN z = quicksqr(x+2, lgpol(x));
  setvarn(z,varn(x));
  if (!p) return z;
  return FpX_red(z, p);
}

/* Inverse of x in Z/pZ[X]/(pol) or NULL if inverse doesn't exist
 * return lift(1 / (x mod (p,pol))) */
GEN
FpXQ_invsafe(GEN x, GEN T, GEN p)
{
  GEN z, U, V;

  if (typ(x) != t_POL) err(bugparier,"FpXQ_invsafe, not a polynomial");
  z = FpX_extgcd(x, T, p, &U, &V);
  if (degpol(z)) return NULL;
  z = mpinvmodsafe((GEN)z[2], p);
  if (!z) return NULL;
  return FpX_Fp_mul(U, z, p);
}

/* Product of y and x in Z/pZ[X]/(T)
 * return lift(lift(Mod(x*y*Mod(1,p),T*Mod(1,p)))) */
/* x and y must be poynomials in the same var as T.
 * t_INT are not allowed. Use Fq_mul instead.
 */
GEN
FpXQ_mul(GEN y,GEN x,GEN T,GEN p)
{
  GEN z;
  if (typ(x) == t_INT || typ(y) == t_INT)
    err(bugparier,"FpXQ_mul, not a polynomial");
  z = quickmul(y+2, x+2, lgpol(y), lgpol(x)); setvarn(z,varn(y));
  z = FpX_red(z, p); return FpX_rem(z,T, p);
}

/* Square of y in Z/pZ[X]/(pol)
 * return lift(lift(Mod(y^2*Mod(1,p),pol*Mod(1,p)))) */
GEN
FpXQ_sqr(GEN y,GEN pol,GEN p)
{
  GEN z = quicksqr(y+2,lgpol(y)); setvarn(z,varn(y));
  z = FpX_red(z, p); return FpX_rem(z,pol, p);
}
/*Modify y[2].
 *No reduction if p is NULL
 */
GEN
FpX_Fp_add(GEN y,GEN x,GEN p)
{
  if (!signe(x)) return y;
  if (!signe(y))
    return scalarpol(x,varn(y));
  y[2]=laddii((GEN)y[2],x);
  if (p) y[2]=lmodii((GEN)y[2],p);
  if (!signe(y[2]) && degpol(y) == 0) return zeropol(varn(y));
  return y;
}
GEN
ZX_s_add(GEN y,long x)
{
  if (!x) return y;
  if (!signe(y))
    return scalarpol(stoi(x),varn(y));
  y[2] = laddis((GEN)y[2],x);
  if (!signe(y[2]) && degpol(y) == 0) return zeropol(varn(y));
  return y;
}
/* y is a polynomial in ZZ[X] and x an integer.
 * If p is NULL, no reduction is performed and return x*y
 *
 * else the result is lift(y*x*Mod(1,p))
 */
GEN
FpX_Fp_mul(GEN y,GEN x,GEN p)
{
  GEN z;
  int i;
  if (!signe(x))
    return zeropol(varn(y));
  z=cgetg(lg(y),t_POL);
  z[1]=y[1];
  for(i=2;i<lg(y);i++)
    z[i]=lmulii((GEN)y[i],x);
  if(!p) return z;
  return FpX_red(z,p);
}
/*****************************************************************
 *                 End of unclean functions.                     *
 *                                                               *
 *****************************************************************/

/* modular power */
ulong
powuumod(ulong x, ulong n0, ulong p)
{
  ulong y, z, n;
  if (n0 <= 2)
  { /* frequent special cases */
    if (n0 == 2) return muluumod(x,x,p);
    if (n0 == 1) return x;
    if (n0 == 0) return 1;
  }
  y = 1; z = x; n = n0;
  for(;;)
  {
    if (n&1) y = muluumod(y,z,p);
    n>>=1; if (!n) return y;
    z = muluumod(z,z,p);
  }
}

GEN
powgumod(GEN x, ulong n0, GEN p)
{
  GEN y, z;
  ulong n;
  if (n0 <= 2)
  { /* frequent special cases */
    if (n0 == 2) return resii(sqri(x),p);
    if (n0 == 1) return x;
    if (n0 == 0) return gun;
  }
  y = gun; z = x; n = n0;
  for(;;)
  {
    if (n&1) y = resii(mulii(y,z),p);
    n>>=1; if (!n) return y;
    z = resii(sqri(z),p);
  }
}

/*****************************************************************
 Clean and with no reduced hypothesis.  Beware that some operations
 will be much slower with big unreduced coefficient
*****************************************************************/
/* Inverse of x in Z[X] / (p,T)
 * return lift(lift(Mod(x*Mod(1,p), T*Mod(1,p))^-1)); */
GEN
FpXQ_inv(GEN x,GEN T,GEN p)
{
  pari_sp av;
  GEN U;

  if (!T) err(bugparier,"FpXQ_inv, T=NULL");
  av = avma;
  U = FpXQ_invsafe(x, T, p);
  if (!U) err(talker,"non invertible polynomial in FpXQ_inv");
  return gerepileupto(av, U);
}

GEN
FpXQ_div(GEN x,GEN y,GEN T,GEN p)
{
  pari_sp av = avma;
  return gerepileupto(av, FpXQ_mul(x,FpXQ_inv(y,T,p),T,p));
}

GEN
FpXV_FpV_innerprod(GEN V, GEN W, GEN p)
{
  pari_sp ltop=avma;
  long i;
  GEN z = FpX_Fp_mul((GEN)V[1],(GEN)W[1],NULL);
  for(i=2;i<lg(V);i++)
    z=FpX_add(z,FpX_Fp_mul((GEN)V[i],(GEN)W[i],NULL),NULL);
  return gerepileupto(ltop,FpX_red(z,p));
}

/* generates the list of powers of x of degree 0,1,2,...,l*/
GEN
FpXQ_powers(GEN x, long l, GEN T, GEN p)
{
  GEN V=cgetg(l+2,t_VEC);
  long i;
  V[1] = (long) scalarpol(gun,varn(T));
  if (l==0) return V;
  V[2] = lcopy(x);
  if (l==1) return V;
  V[3] = (long) FpXQ_sqr(x,T,p);
  for(i=4;i<l+2;i++)
    V[i] = (long) FpXQ_mul((GEN) V[i-1],x,T,p);
#if 0
  TODO: Karim proposes to use squaring:
  V[i] = (long) ((i&1)?FpXQ_sqr((GEN) V[(i+1)>>1],T,p)
                       :FpXQ_mul((GEN) V[i-1],x,T,p));
  Please profile it.
#endif
  return V;
}
#if 0
static long brent_kung_nbmul(long d, long n, long p)
{
  return p+n*((d+p-1)/p);
}
  TODO: This the the optimal parameter for the purpose of reducing
  multiplications, but profiling need to be done to ensure it is not slower 
  than the other option in practice
/*Return optimal parameter l for the evaluation of n polynomials of degree d*/
long brent_kung_optpow(long d, long n)
{
  double r;
  long f,c,pr;
  if (n>=d ) return d;
  pr=n*d;
  if (pr<=1) return 1;
  r=d/sqrt(pr);
  c=(long)ceil(d/ceil(r));
  f=(long)floor(d/floor(r));
  return (brent_kung_nbmul(d, n, c) <= brent_kung_nbmul(d, n, f))?c:f;
}
#endif 
/*Return optimal parameter l for the evaluation of n polynomials of degree d*/
long
brent_kung_optpow(long d, long n)
{
  long l, pr;
  if (n >= d) return d;
  pr = n*d; if (pr <= 1) return 1;
  l = (long) ((double)d / sqrt(pr));
  return (d+l-1) / l;
}

/*Close to FpXV_FpV_innerprod*/

static GEN
spec_compo_powers(GEN P, GEN V, long a, long n)
{
  long i;
  GEN z;
  z = scalarpol((GEN)P[2+a],varn(P));
  for(i=1;i<=n;i++)
    z=FpX_add(z,FpX_Fp_mul((GEN)V[i+1],(GEN)P[2+a+i],NULL),NULL);
  return z;
}
/*Try to implement algorithm in Brent & Kung (Fast algorithms for
 *manipulating formal power series, JACM 25:581-595, 1978)
 
 V must be as output by FpXQ_powers.
 For optimal performance, l (of FpXQ_powers) must be as output by
 brent_kung_optpow
 */

GEN
FpX_FpXQV_compo(GEN P, GEN V, GEN T, GEN p)
{
  pari_sp ltop=avma;
  long l=lg(V)-1;
  GEN z,u;
  long d=degpol(P),cnt=0;
  if (d==-1) return zeropol(varn(T));
  if (d<l)
  {
    z=spec_compo_powers(P,V,0,d);
    return gerepileupto(ltop,FpX_red(z,p));
  }
  if (l<=1)
    err(talker,"powers is only [] or [1] in FpX_FpXQV_compo");
  z=spec_compo_powers(P,V,d-l+1,l-1);
  d-=l;
  while(d>=l-1)
  {
    u=spec_compo_powers(P,V,d-l+2,l-2);
    z=FpX_add(u,FpXQ_mul(z,(GEN)V[l],T,p),NULL);
    d-=l-1;
    cnt++;
  }
  u=spec_compo_powers(P,V,0,d);
  z=FpX_add(u,FpXQ_mul(z,(GEN)V[d+2],T,p),NULL);
  cnt++;
  if (DEBUGLEVEL>=8) fprintferr("FpX_FpXQV_compo: %d FpXQ_mul [%d]\n",cnt,l-1);
  return gerepileupto(ltop,FpX_red(z,p));
}

/* T in Z[X] and  x in Z/pZ[X]/(pol)
 * return lift(lift(subst(T,variable(T),Mod(x*Mod(1,p),pol*Mod(1,p)))));
 */
GEN
FpX_FpXQ_compo(GEN T,GEN x,GEN pol,GEN p)
{
  pari_sp ltop=avma;
  GEN z;
  long d=degpol(T),rtd;
  if (!signe(T)) return zeropol(varn(T));
  rtd = (long) sqrt((double)d);
  z = FpX_FpXQV_compo(T,FpXQ_powers(x,rtd,pol,p),pol,p);
  return gerepileupto(ltop,z);
}

/* Evaluation in Fp
 * x in Z[X] and y in Z return x(y) mod p
 *
 * If p is very large (several longs) and x has small coefficients(<<p),
 * then Brent & Kung algorithm is faster. */
GEN
FpX_eval(GEN x,GEN y,GEN p)
{
  pari_sp av;
  GEN p1,r,res;
  long j, i=lg(x)-1;
  if (i<=2)
    return (i==2)? modii((GEN)x[2],p): gzero;
  res=cgetg(lgefint(p),t_INT);
  av=avma; p1=(GEN)x[i];
  /* specific attention to sparse polynomials (see poleval)*/
  /*You've guess it! It's a copy-paste(tm)*/
  for (i--; i>=2; i=j-1)
  {
    for (j=i; !signe((GEN)x[j]); j--)
      if (j==2)
      {
	if (i!=j) y = powgumod(y,i-j+1,p);
	p1=mulii(p1,y);
	goto fppoleval;/*sorry break(2) no implemented*/
      }
    r = (i==j)? y: powgumod(y,i-j+1,p);
    p1 = modii(addii(mulii(p1,r), (GEN)x[j]),p);
  }
 fppoleval:
  modiiz(p1,p,res);
  avma=av;
  return res;
}
/* Tz=Tx*Ty where Tx and Ty coprime
 * return lift(chinese(Mod(x*Mod(1,p),Tx*Mod(1,p)),Mod(y*Mod(1,p),Ty*Mod(1,p))))
 * if Tz is NULL it is computed
 * As we do not return it, and the caller will frequently need it,
 * it must compute it and pass it.
 */
GEN
FpX_chinese_coprime(GEN x,GEN y,GEN Tx,GEN Ty,GEN Tz,GEN p)
{
  pari_sp av = avma;
  GEN ax,p1;
  ax = FpX_mul(FpXQ_inv(Tx,Ty,p), Tx,p);
  p1=FpX_mul(ax, FpX_sub(y,x,p),p);
  p1 = FpX_add(x,p1,p);
  if (!Tz) Tz=FpX_mul(Tx,Ty,p);
  p1 = FpX_rem(p1,Tz,p);
  return gerepileupto(av,p1);
}

typedef struct {
  GEN pol, p;
} FpX_muldata;

static GEN
_sqr(void *data, GEN x)
{
  FpX_muldata *D = (FpX_muldata*)data;
  return FpXQ_sqr(x, D->pol, D->p);
}
static GEN
_mul(void *data, GEN x, GEN y)
{
  FpX_muldata *D = (FpX_muldata*)data;
  return FpXQ_mul(x,y, D->pol, D->p);
}

/* x,pol in Z[X], p in Z, n in Z, compute lift(x^n mod (p, pol)) */
GEN
FpXQ_pow(GEN x, GEN n, GEN pol, GEN p)
{
  FpX_muldata D;
  pari_sp av;
  GEN y;

  if (!signe(n)) return polun[ varn(x) ];
  if (is_pm1(n)) /* +/- 1 */
    return (signe(n) < 0)? FpXQ_inv(x,pol,p): gcopy(x);
  av = avma;
  if (!is_bigint(p))
  {
    ulong pp = p[2];
    pol = ZX_Flx(pol, pp);
    x   = ZX_Flx(x,   pp);
    y = Flx_ZX( Flxq_pow(x, n, pol, pp) );
  }
  else
  {
    D.pol = pol;
    D.p   = p;
    if (signe(n) < 0) x = FpXQ_inv(x,pol,p);
    y = leftright_pow(x, n, (void*)&D, &_sqr, &_mul);
  }
  return gerepileupto(av, y);
}

static GEN modulo;
static GEN _FpX_mul(GEN a,GEN b){return FpX_mul(a,b,modulo);}
GEN 
FpXV_prod(GEN V, GEN p)
{
  modulo = p; 
  return divide_conquer_prod(V, &_FpX_mul);
}

GEN
FpV_roots_to_pol(GEN V, GEN p, long v)
{
  pari_sp ltop=avma;
  long i;
  GEN g=cgetg(lg(V),t_VEC);
  for(i=1;i<lg(V);i++)
    g[i]=(long)deg1pol(gun,modii(negi((GEN)V[i]),p),v);
  return gerepileupto(ltop,FpXV_prod(g,p));
}

/*******************************************************************/
/*                                                                 */
/*                             FpXX                                */
/*                                                                 */
/*******************************************************************/
/*Polynomials whose coefficients are either polynomials or integers*/
GEN
FpXX_red(GEN z, GEN p)
{
  GEN res;
  int i;
  res = cgetg(lg(z),t_POL); res[1] = z[1];
  for(i=2;i<lg(res);i++)
    if (typ(z[i])==t_INT)
      res[i] = lmodii((GEN)z[i],p);
    else
    {
      pari_sp av=avma;
      res[i]=(long)FpX_red((GEN)z[i],p);
      if (lg(res[i])<=3)
      {
        if (lg(res[i])==2) {avma=av;res[i]=zero;}
        else res[i]=lpilecopy(av,gmael(res,i,2));
      }
    }
  return FpXX_renormalize(res,lg(res));
}

/*******************************************************************/
/*                                                                 */
/*                             (Fp[X]/(Q))[Y]                      */
/*                                                                 */
/*******************************************************************/
extern GEN to_Kronecker(GEN P, GEN Q);

/*Not malloc nor warn-clean.*/
GEN
FpXQX_from_Kronecker(GEN Z, GEN T, GEN p)
{
  long i,j,lx,l, N = (degpol(T)<<1) + 1;
  GEN x, t = cgetg(N,t_POL), z = FpX_red(Z, p);
  t[1] = T[1] & VARNBITS;
  l = lg(z); lx = (l-2) / (N-2);
  x = cgetg(lx+3,t_POL);
  for (i=2; i<lx+2; i++)
  {
    for (j=2; j<N; j++) t[j] = z[j];
    z += (N-2);
    x[i] = (long)FpX_rem(FpX_renormalize(t,N), T,p);
  }
  N = (l-2) % (N-2) + 2;
  for (j=2; j<N; j++) t[j] = z[j];
  x[i] = (long)FpX_rem(FpX_renormalize(t,N), T,p);
  return FpXQX_renormalize(x, i+1);
}

GEN
FpXQX_red(GEN z, GEN T, GEN p)
{
  GEN res;
  int i, l = lg(z);
  res = cgetg(l,t_POL); res[1] = z[1];
  for(i=2;i<l;i++)
    if (typ(z[i]) == t_INT)
      res[i] = lmodii((GEN)z[i],p);
    else if (T)
      res[i] = (long)FpX_rem((GEN)z[i],T,p);
    else
      res[i] = (long)FpX_red((GEN)z[i],p);
  return FpXQX_renormalize(res,lg(res));
}
GEN
FpXQX_mul(GEN x, GEN y, GEN T, GEN p)
{
  pari_sp ltop=avma;
  GEN z,kx,ky;
  long vx;
  if (!T) err(bugparier,"FpXQX_mul, T==NULL");
  vx = min(varn(x),varn(y));
  kx= to_Kronecker(x,T);
  ky= to_Kronecker(y,T);
  z = quickmul(ky+2, kx+2, lgpol(ky), lgpol(kx));
  z = FpXQX_from_Kronecker(z,T,p);
  setvarn(z,vx);/*quickmul and FpXQX_from_Kronecker are not varn-clean*/
  return gerepileupto(ltop,z);
}
GEN
FpXQX_sqr(GEN x, GEN T, GEN p)
{
  pari_sp ltop=avma;
  GEN z,kx;
  long vx=varn(x);
  kx= to_Kronecker(x,T);
  z = quicksqr(kx+2, lgpol(kx));
  z = FpXQX_from_Kronecker(z,T,p);
  setvarn(z,vx);/*quickmul and FpXQX_from_Kronecker are nor varn-clean*/
  return gerepileupto(ltop,z);
}

GEN
FpXQX_FpXQ_mul(GEN P, GEN U, GEN T, GEN p)
{
  int i, lP = lg(P);
  GEN res = cgetg(lP,t_POL);
  res[1] = P[1];
  for(i=2; i<lP; i++)
    res[i] = (long)Fq_mul(U,(GEN)P[i], T,p);
  return FpXQX_renormalize(res,lg(res));
}

/* a X^degpol, assume degpol >= 0 */
static GEN
monomial(GEN a, int degpol, int v)
{
  long i, lP = degpol+3;
  GEN P = cgetg(lP, t_POL);
  P[1] = evalsigne(1) | evalvarn(v);
  lP--; P[lP] = (long)a;
  for (i=2; i<lP; i++) P[i] = zero;
  return P;
}

/* return NULL if Euclidean remainder sequence fails (==> T reducible mod p)
 * last non-zero remainder otherwise */
GEN
FpXQX_safegcd(GEN P, GEN Q, GEN T, GEN p)
{
  pari_sp btop, ltop = avma, st_lim;
  long dg, vx = varn(P);
  GEN U, q;
  P = FpXX_red(P, p); btop = avma;
  Q = FpXX_red(Q, p);
  if (!signe(P)) return gerepileupto(ltop, Q);
  if (!signe(Q)) { avma = btop; return P; }
  T = FpX_red(T, p);

  btop = avma; st_lim = stack_lim(btop, 1);
  dg = lg(P)-lg(Q);
  if (dg < 0) { swap(P, Q); dg = -dg; }
  for(;;)
  {
    U = Fq_invsafe(leading_term(Q), T, p);
    if (!U) { avma = ltop; return NULL; }
    do /* set P := P % Q */
    {
      q = Fq_mul(U, gneg(leading_term(P)), T, p);
      P = gadd(P, FpXQX_mul(monomial(q, dg, vx), Q, T, p));
      P = FpXQX_red(P, T, p); /* wasteful, but negligible */
      dg = lg(P)-lg(Q);
    } while (dg >= 0);
    if (!signe(P)) break;

    if (low_stack(st_lim, stack_lim(btop, 1)))
    {
      GEN *bptr[2]; bptr[0]=&P; bptr[1]=&Q;
      if (DEBUGLEVEL>1) err(warnmem,"FpXQX_safegcd");
      gerepilemany(btop, bptr, 2);
    }
    swap(P, Q); dg = -dg;
  }
  Q = FpXQX_FpXQ_mul(Q, U, T, p); /* normalize GCD */
  return gerepileupto(ltop, Q);
}

/*******************************************************************/
/*                                                                 */
/*                       (Fp[X]/T(X))[Y] / S(Y)                    */
/*                                                                 */
/*******************************************************************/

/*Preliminary implementation to speed up Fp_isom*/
typedef struct {
  GEN S, T, p;
} FpXYQQ_muldata;

/* reduce x in Fp[X, Y] in the algebra Fp[X, Y]/ (P(X),Q(Y)) */
static GEN
FpXYQQ_redswap(GEN x, GEN S, GEN T, GEN p)
{
  pari_sp ltop=avma;
  long n=degpol(S);
  long m=degpol(T);
  long v=varn(T),w=varn(S);
  GEN V = swap_polpol(x,n,w);
  setvarn(T,w);
  V = FpXQX_red(V,T,p);
  setvarn(T,v);
  V = swap_polpol(V,m,w);
  return gerepilecopy(ltop,V); 
}
static GEN
FpXYQQ_sqr(void *data, GEN x)
{
  FpXYQQ_muldata *D = (FpXYQQ_muldata*)data;
  return FpXYQQ_redswap(FpXQX_sqr(x, D->S, D->p),D->S,D->T,D->p);
  
}
static GEN
FpXYQQ_mul(void *data, GEN x, GEN y)
{
  FpXYQQ_muldata *D = (FpXYQQ_muldata*)data;
  return FpXYQQ_redswap(FpXQX_mul(x,y, D->S, D->p),D->S,D->T,D->p);
}

/* x in Z[X,Y], S in Z[X] over Fq = Z[Y]/(p,T); compute lift(x^n mod (S,T,p)) */
GEN
FpXYQQ_pow(GEN x, GEN n, GEN S, GEN T, GEN p)
{
  pari_sp av = avma;
  FpXYQQ_muldata D;
  GEN y;
  if (OK_ULONG(p))
  {
    ulong pp = p[2];
    x = ZXX_FlxX(x, pp, varn(T));
    S = ZX_Flx(S, pp);
    T = ZX_Flx(T, pp);
    y = FlxX_ZXX( FlxYqQ_pow(x, n, S, T, pp) );
  }
  else
  {
    D.S = S;
    D.T = T;
    D.p = p;
    y = leftright_pow(x, n, (void*)&D, &FpXYQQ_sqr, &FpXYQQ_mul);
  }
  return gerepileupto(av, y);
}

typedef struct {
  GEN T, p, S;
  long v;
} kronecker_muldata;

static GEN
FpXQYQ_red(void *data, GEN x)
{
  kronecker_muldata *D = (kronecker_muldata*)data;
  GEN t = FpXQX_from_Kronecker(x, D->T,D->p);
  setvarn(t, D->v);
  t = FpXQX_divrem(t, D->S,D->T,D->p, ONLY_REM);
  return to_Kronecker(t,D->T);
}
static GEN
FpXQYQ_mul(void *data, GEN x, GEN y) {
  return FpXQYQ_red(data, FpX_mul(x,y,NULL));
}
static GEN
FpXQYQ_sqr(void *data, GEN x) {
  return FpXQYQ_red(data, FpX_sqr(x,NULL));
}

/* x over Fq, return lift(x^n) mod S */
GEN
FpXQYQ_pow(GEN x, GEN n, GEN S, GEN T, GEN p)
{
  pari_sp ltop = avma;
  GEN y;
  kronecker_muldata D;
  if (OK_ULONG(p))
  {
    ulong pp = p[2];
    GEN z;
    long v = varn(T);
    T = ZX_Flx(T, pp);
    x = ZXX_FlxX(x, pp, v);
    S = ZXX_FlxX(S, pp, v);
    z = FlxqXQ_pow(x, n, S, T, pp);
    y = FlxX_ZXX(z);
  }
  else
  {
    long v = varn(x);
    D.S = S;
    D.T = T;
    D.p = p;
    D.v = v;
    y = leftright_pow(to_Kronecker(x,T), n, (void*)&D, &FpXQYQ_sqr, &FpXQYQ_mul);
    y = FpXQX_from_Kronecker(y, T,p);
    setvarn(y, v); 
  }
  return gerepileupto(ltop, y);
}
/*******************************************************************/
/*                                                                 */
/*                             Fq                                  */
/*                                                                 */
/*******************************************************************/

GEN
Fq_add(GEN x, GEN y, GEN T/*unused*/, GEN p)
{
  (void)T;
  switch((typ(x)==t_POL)|((typ(y)==t_POL)<<1))
  {
    case 0: return modii(addii(x,y),p);
    case 1: return FpX_Fp_add(x,y,p);
    case 2: return FpX_Fp_add(y,x,p);
    case 3: return FpX_add(x,y,p);
  }
  return NULL;
}

GEN
Fq_sub(GEN x, GEN y, GEN T/*unused*/, GEN p)
{
  (void)T;
  switch((typ(x)==t_POL)|((typ(y)==t_POL)<<1))
  {
    case 0: return modii(subii(x,y),p);
    case 1: return FpX_Fp_add(x,negi(y),p);
    case 2: return FpX_Fp_add(FpX_neg(y,p),x,p);
    case 3: return FpX_sub(x,y,p);
  }
  return NULL;
}

GEN
Fq_neg(GEN x, GEN T/*unused*/, GEN p)
{
  switch(typ(x)==t_POL)
  {
    case 0: return signe(x)?subii(p,x):gzero;
    case 1: return FpX_neg(x,p);
  }
  return NULL;
}

/* If T==NULL do not reduce*/
GEN
Fq_mul(GEN x, GEN y, GEN T, GEN p)
{
  switch((typ(x)==t_POL)|((typ(y)==t_POL)<<1))
  {
    case 0: return modii(mulii(x,y),p);
    case 1: return FpX_Fp_mul(x,y,p);
    case 2: return FpX_Fp_mul(y,x,p);
    case 3: if (T) return FpXQ_mul(x,y,T,p);
            else return FpX_mul(x,y,p);
  }
  return NULL;
}

GEN
Fq_neg_inv(GEN x, GEN T, GEN p)
{
  if (typ(x) == t_INT) return mpinvmod(negi(x),p);
  return FpXQ_inv(FpX_neg(x,p),T,p);
}

GEN
Fq_invsafe(GEN x, GEN pol, GEN p)
{
  if (typ(x) == t_INT) return mpinvmodsafe(x,p);
  return FpXQ_invsafe(x,pol,p);
}

GEN
Fq_inv(GEN x, GEN pol, GEN p)
{
  if (typ(x) == t_INT) return mpinvmod(x,p);
  return FpXQ_inv(x,pol,p);
}

GEN
Fq_pow(GEN x, GEN n, GEN pol, GEN p)
{
  if (typ(x) == t_INT) return powmodulo(x,n,p);
  return FpXQ_pow(x,n,pol,p);
}

GEN
Fq_red(GEN x, GEN T, GEN p)
{
  pari_sp ltop=avma;
  switch(typ(x)==t_POL)
  {
    case 0: return modii(x,p);
    case 1: return gerepileupto(ltop,FpX_rem(FpX_red(x,p),T,p));
  }
  return NULL;
}


/*******************************************************************/
/*                                                                 */
/*                             Fq[X]                               */
/*                                                                 */
/*******************************************************************/

GEN
FqX_mul(GEN x, GEN y, GEN T, GEN p)
{
  return T? FpXQX_mul(x, y, T, p): FpX_mul(x, y, p);
}
GEN
FqX_sqr(GEN x, GEN T, GEN p)
{
  return T? FpXQX_sqr(x, T, p): FpX_sqr(x, p);
}
GEN
FqX_div(GEN x, GEN y, GEN T, GEN p)
{
  return T? FpXQX_divrem(x,y,T,p,NULL): FpX_divrem(x,y,p,NULL);
}
GEN
FqX_rem(GEN x, GEN y, GEN T, GEN p)
{
  return T? FpXQX_divrem(x,y,T,p,ONLY_REM): FpX_divrem(x,y,p,ONLY_REM);
}
GEN
FqX_divrem(GEN x, GEN y, GEN T, GEN p, GEN *z)
{
  return T? FpXQX_divrem(x,y,T,p,z): FpX_divrem(x,y,p,z);
}

static GEN Tmodulo;
static GEN _FpXQX_mul(GEN a,GEN b){return FpXQX_mul(a,b,Tmodulo,modulo);}
GEN 
FpXQXV_prod(GEN V, GEN Tp, GEN p)
{
  modulo = p; Tmodulo = Tp;
  return divide_conquer_prod(V, &_FpXQX_mul);
}
GEN
FqV_roots_to_pol(GEN V, GEN Tp, GEN p, long v)
{
  pari_sp ltop = avma;
  long k;
  GEN W = cgetg(lg(V),t_VEC);
  for(k=1; k < lg(V); k++)
    W[k] = (long)deg1pol(gun,Fq_neg((GEN)V[k],Tp,p),v);
  return gerepileupto(ltop, FpXQXV_prod(W, Tp, p));
}

GEN
FqV_red(GEN z, GEN T, GEN p)
{
  GEN res;
  int i, l = lg(z);
  res=cgetg(l,typ(z));
  for(i=1;i<l;i++)
    if (typ(z[i]) == t_INT)
      res[i] = lmodii((GEN)z[i],p);
    else if (T)
      res[i] = (long)FpX_rem((GEN)z[i],T,p);
    else
      res[i] = (long)FpX_red((GEN)z[i],p);
  return res;
}

/*******************************************************************/
/*                                                                 */
/*                       n-th ROOT in Fq                           */
/*                                                                 */
/*******************************************************************/
/*NO clean malloc*/
static GEN fflgen(GEN l, long e, GEN r, GEN T ,GEN p, GEN *zeta)
{
  pari_sp av1 = avma;
  GEN z, m, m1;
  const long pp = is_bigint(p)? VERYBIGINT: itos(p);
  long x=varn(T),k,u,v,i;

  for (k=0; ; k++)
  {
    z = (degpol(T)==1)? polun[x]: polx[x];
    u = k/pp; v=2; /* FpX_Fp_add modify y */
    z = gaddgs(z, k%pp);
    while(u)
    {
      z = FpX_add(z, monomial(stoi(u%pp),v,x), NULL);
      u /= pp; v++;
    }
    if ( DEBUGLEVEL>=6 ) fprintferr("FF l-Gen:next %Z\n",z);
    m1 = m = FpXQ_pow(z,r,T,p);
    if (gcmp1(m)) { avma = av1; continue; }

    for (i=1; i<e; i++)
      if (gcmp1(m = FpXQ_pow(m,l,T,p))) break;
    if (i==e) break;
    avma = av1;
  }
  *zeta = m; return m1;
}
/* Solve x^l = a mod (p,T)
 * l must be prime
 * q = p^degpol(T)-1 = (l^e)*r, with e>=1 and pgcd(r,l)=1
 * m = y^(q/l)
 * y not an l-th power [ m != 1 ] */
GEN
ffsqrtlmod(GEN a, GEN l, GEN T ,GEN p , GEN q, long e, GEN r, GEN y, GEN m)
{
  pari_sp av = avma, lim;
  long i,k;
  GEN p1,p2,u1,u2,v,w,z;

  if (gcmp1(a)) return gcopy(a);

  (void)bezout(r,l,&u1,&u2); /* result is 1 */
  v = FpXQ_pow(a,u2,T,p);
  w = FpXQ_pow(a, modii(mulii(negi(u1),r),q), T,p);
  lim = stack_lim(av,1);
  while (!gcmp1(w))
  {
    k = 0;
    p1 = w;
    do { /* if p is not prime, loop will not end */
      z = p1;
      p1 = FpXQ_pow(p1,l,T,p);
      k++;
    } while (!gcmp1(p1));
    if (k==e) { avma=av; return NULL; }
    p2 = FpXQ_mul(z,m,T,p);
    for (i=1; !gcmp1(p2); i++) p2 = FpXQ_mul(p2,m,T,p);/*TODO: BS/GS instead*/
    p1= FpXQ_pow(y, modii(mulsi(i,gpowgs(l,e-k-1)), q), T,p);
    m = FpXQ_pow(m,stoi(i),T,p);
    e = k;
    v = FpXQ_mul(p1,v,T,p);
    y = FpXQ_pow(p1,l,T,p);
    w = FpXQ_mul(y,w,T,p);
    if (low_stack(lim, stack_lim(av,1)))
    {
      if(DEBUGMEM>1) err(warnmem,"ffsqrtlmod");
      gerepileall(av,4, &y,&v,&w,&m);
    }
  }
  return gerepilecopy(av, v);
}
/* Solve x^n = a mod p: n integer, a in Fp[X]/(T) [ p prime, T irred. mod p ]
 *
 * 1) if no solution, return NULL and (if zetan != NULL) set zetan to gzero.
 *
 * 2) If there is a solution, there are exactly  m=gcd(p-1,n) of them.
 * If zetan != NULL, it is set to a primitive mth root of unity so that the set
 * of solutions is {x*zetan^k;k=0 to m-1}
 *
 * If a = 0, return 0 and (if zetan != NULL) set zetan = gun */
GEN ffsqrtnmod(GEN a, GEN n, GEN T, GEN p, GEN *zetan)
{
  pari_sp ltop=avma, av1, lim;
  long i,j,e;
  GEN m,u1,u2;
  GEN q,r,zeta,y,l,z;

  if (typ(a) != t_POL || typ(n) != t_INT || typ(T) != t_POL || typ(p)!=t_INT)
    err(typeer,"ffsqrtnmod");
  if (!degpol(T)) err(constpoler,"ffsqrtnmod");
  if (!signe(n)) err(talker,"1/0 exponent in ffsqrtnmod");
  if (gcmp1(n)) {if (zetan) *zetan=gun;return gcopy(a);}
  if (gcmp0(a)) {if (zetan) *zetan=gun;return gzero;}

  q = addsi(-1, gpowgs(p,degpol(T)));
  m = bezout(n,q,&u1,&u2);
  if (!egalii(m,n)) a = FpXQ_pow(a, modii(u1,q), T,p);
  if (zetan) z = polun[varn(T)];
  lim = stack_lim(ltop,1);
  if (!gcmp1(m))
  {
    m = decomp(m); av1 = avma;
    for (i = lg(m[1])-1; i; i--)
    {
      l = gcoeff(m,i,1);
      j = itos(gcoeff(m,i,2));
      e = pvaluation(q,l,&r);
      if(DEBUGLEVEL>=6) (void)timer2();
      y = fflgen(l,e,r,T,p,&zeta);
      if(DEBUGLEVEL>=6) msgtimer("fflgen");
      if (zetan) z = FpXQ_mul(z, FpXQ_pow(y,gpowgs(l,e-j),T,p), T,p);
      for (; j; j--)
      {
	a = ffsqrtlmod(a,l,T,p,q,e,r,y,zeta);
	if (!a) {avma=ltop; return NULL;}
      }
      if (low_stack(lim, stack_lim(ltop,1)))
      { /* n can have lots of prime factors */
	if(DEBUGMEM>1) err(warnmem,"ffsqrtnmod");
        gerepileall(av1,zetan? 2: 1, &a,&z);
      }
    }
  }
  if (zetan)
  {
    *zetan = z;
    gerepileall(ltop,2,&a,zetan);
  }
  else
    a = gerepileupto(ltop, a);
  return a;
}
/*******************************************************************/
/*  Isomorphisms between finite fields                             */
/*                                                                 */
/*******************************************************************/
GEN
matrixpow(long n, long m, GEN y, GEN P,GEN l)
{
  return vecpol_to_mat(FpXQ_powers(y,m-1,P,l),n);
}
/* compute the reciprocical isomorphism of S mod T,p, i.e. V such that
   V(S)=X  mod T,p*/
GEN
Fp_inv_isom(GEN S,GEN T, GEN p)
{
  pari_sp ltop = avma;
  GEN     M, V;
  int     n, i;
  long    x;
  x = varn(T);
  n = degpol(T);
  M = matrixpow(n,n,S,T,p);
  V = cgetg(n + 1, t_COL);
  for (i = 1; i <= n; i++)
    V[i] = zero;
  V[2] = un;
  V = FpM_invimage(M,V,p);
  V = gtopolyrev(V, x);
  return gerepileupto(ltop, V);
}

/* Let M the matrix of the x^p Frobenius automorphism.
 * Compute x^(p^i) for i=0..r
 */
static GEN
polfrobenius(GEN M, GEN p, long r, long v)
{
  GEN V,W;
  long i;
  V = cgetg(r+2,t_VEC);
  V[1] = (long) polx[v];
  if (r == 0) return V;
  V[2] = (long) vec_to_pol((GEN)M[2],v);
  W = (GEN) M[2];
  for (i = 3; i <= r+1; ++i)
  {
    W = FpM_FpV_mul(M,W,p);
    V[i] = (long) vec_to_pol(W,v);
  }
  return V;
}

/* Let P a polynomial != 0 and M the matrix of the x^p Frobenius automorphism in
 * FFp[X]/T. Compute P(M)
 * V=polfrobenius(M, p, degpol(P), v)
 * not stack clean
 */

static GEN
matpolfrobenius(GEN V, GEN P, GEN T, GEN p)
{
  pari_sp btop;
  long i;
  long l=degpol(T);
  long v=varn(T);
  GEN M,W,Mi;
  GEN PV=gtovec(P);
  GEN *gptr[2];
  long lV=lg(V);
  PV=cgetg(degpol(P)+2,t_VEC);
  for(i=1;i<lg(PV);i++)
    PV[i]=P[1+i];
  M=cgetg(l+1,t_VEC);
  M[1]=(long)scalarpol(poleval(P,gun),v);
  M[2]=(long)FpXV_FpV_innerprod(V,PV,p);
  btop=avma;
  gptr[0]=&Mi;
  gptr[1]=&W;
  W=cgetg(lV,t_VEC);
  for(i=1;i<lV;i++)
     W[i]=V[i];
  for(i=3;i<=l;i++)
  {
    long j;
    pari_sp bbot;
    GEN W2=cgetg(lV,t_VEC);
    for(j=1;j<lV;j++)
      W2[j]=(long)FpXQ_mul((GEN)W[j],(GEN)V[j],T,p);
    bbot=avma;
    Mi=FpXV_FpV_innerprod(W2,PV,p);
    W=gcopy(W2);
    gerepilemanysp(btop,bbot,gptr,2);
    btop=(pari_sp)W;
    M[i]=(long)Mi;
  }
  return vecpol_to_mat(M,l);
}

/* Let M the matrix of the Frobenius automorphism of Fp[X]/(T).
 * Compute M^d
 * TODO: use left-right binary (tricky!)
 */
GEN
FpM_Frobenius_pow(GEN M, long d, GEN T, GEN p)
{
  pari_sp ltop=avma;
  long i,l=degpol(T);
  GEN R, W = (GEN) M[2];
  for (i = 2; i <= d; ++i)
    W = FpM_FpV_mul(M,W,p);
  R=matrixpow(l,l,vec_to_pol(W,varn(T)),T,p);
  return gerepilecopy(ltop,R);
}

/* Essentially we want to compute
 * FqM_ker(MA-polx[MAXVARN],U,l)
 * To avoid use of matrix in Fq we procede as follows:
 * We compute FpM_ker(U(MA),l) and then we recover
 * the eigen value by Galois action, see formula.
 */
static GEN
intersect_ker(GEN P, GEN MA, GEN U, GEN l)
{
  pari_sp ltop=avma;
  long vp=varn(P);
  long vu=varn(U), r=degpol(U);
  long i;
  GEN A, R, M, ib0, V;
  if (DEBUGLEVEL>=4) (void)timer2();
  V=polfrobenius(MA,l,r,vu);
  if (DEBUGLEVEL>=4) msgtimer("pol[frobenius]");
  M=matpolfrobenius(V,U,P,l);
  if (DEBUGLEVEL>=4) msgtimer("matrix cyclo");
  A=FpM_ker(M,l);
  if (DEBUGLEVEL>=4) msgtimer("kernel");
  A=gerepileupto(ltop,A);
  if (lg(A)!=r+1)
    err(talker,"ZZ_%Z[%Z]/(%Z) is not a field in Fp_intersect"
	,l,polx[vp],P);
  /*The formula is 
   * a_{r-1}=-\phi(a_0)/b_0
   * a_{i-1}=\phi(a_i)+b_ia_{r-1}  i=r-1 to 1
   * Where a_0=A[1] and b_i=U[i+2]
   */
  ib0=negi(mpinvmod((GEN)U[2],l));
  R=cgetg(r+1,t_MAT);
  R[1]=A[1];
  R[r]=(long)FpM_FpV_mul(MA,gmul((GEN)A[1],ib0),l);
  for(i=r-1;i>1;i--)
    R[i]=(long)FpV_red(gadd(FpM_FpV_mul(MA,(GEN)R[i+1],l),
	  gmul((GEN)U[i+2],(GEN)R[r])),l);
  R=gtrans_i(R);
  for(i=1;i<lg(R);i++)
    R[i]=(long)vec_to_pol((GEN)R[i],vu);
  A=gtopolyrev(R,vp);
  return gerepileupto(ltop,A);
}

/* n must divide both the degree of P and Q.  Compute SP and SQ such
  that the subfield of FF_l[X]/(P) generated by SP and the subfield of
  FF_l[X]/(Q) generated by SQ are isomorphic of degree n.  P and Q do
  not need to be of the same variable.  if MA (resp. MB) is not NULL,
  must be the matrix of the frobenius map in FF_l[X]/(P) (resp.
  FF_l[X]/(Q) ).  */
/* Note on the implementation choice:
 * We assume the prime p is very large
 * so we handle Frobenius as matrices.
 */
void
Fp_intersect(long n, GEN P, GEN Q, GEN l,GEN *SP, GEN *SQ, GEN MA, GEN MB)
{
  pari_sp lbot, ltop=avma;
  long vp,vq,np,nq,e,pg;
  GEN q;
  GEN A,B,Ap,Bp;
  GEN *gptr[2];
  vp=varn(P);vq=varn(Q);
  np=degpol(P);nq=degpol(Q);
  if (np<=0 || nq<=0 || n<=0 || np%n!=0 || nq%n!=0)
    err(talker,"bad degrees in Fp_intersect: %d,%d,%d",n,degpol(P),degpol(Q));
  e=pvaluation(stoi(n),l,&q);
  pg=itos(q);
  avma=ltop;
  if(!MA) MA=matrixpow(np,np,FpXQ_pow(polx[vp],l,P,l),P,l);
  if(!MB) MB=matrixpow(nq,nq,FpXQ_pow(polx[vq],l,Q,l),Q,l);
  A=Ap=zeropol(vp);
  B=Bp=zeropol(vq);
  if (pg > 1)
  {
    if (smodis(l,pg) == 1)
      /*We do not need to use relative extension in this setting, so
        we don't. (Well,now that we don't in the other case also, it is more
       dubious to treat cases apart...)*/
    {
      GEN L,An,Bn,ipg,z;
      z=FpX_roots(FpX_red(cyclo(pg,-1),l), l);
      if (lg(z)<2) err(talker,"%Z is not a prime in Fp_intersect",l);
      z=negi((GEN)z[1]);
      ipg=stoi(pg);
      if (DEBUGLEVEL>=4) (void)timer2();
      A=FpM_ker(gaddmat(z, MA),l);
      if (lg(A)!=2)
	err(talker,"ZZ_%Z[%Z]/(%Z) is not a field in Fp_intersect"
	    ,l,polx[vp],P);
      A=vec_to_pol((GEN)A[1],vp);
      B=FpM_ker(gaddmat(z, MB),l);
      if (lg(B)!=2)
	err(talker,"ZZ_%Z[%Z]/(%Z) is not a field in Fp_intersect"
	    ,l,polx[vq],Q);
      B=vec_to_pol((GEN)B[1],vq);
      if (DEBUGLEVEL>=4) msgtimer("FpM_ker");
      An=(GEN) FpXQ_pow(A,ipg,P,l)[2];
      Bn=(GEN) FpXQ_pow(B,ipg,Q,l)[2];
      if (!invmod(Bn,l,&z))
        err(talker,"Polynomials not irreducible in Fp_intersect");
      z=modii(mulii(An,z),l);
      L=mpsqrtnmod(z,ipg,l,NULL);
      if ( !L )
        err(talker,"Polynomials not irreducible in Fp_intersect");
      if (DEBUGLEVEL>=4) msgtimer("mpsqrtnmod");
      B=FpX_Fp_mul(B,L,l);
    }
    else
    {
      GEN L,An,Bn,ipg,U,z;
      U=lift(gmael(factmod(cyclo(pg,MAXVARN),l),1,1));
      ipg=stoi(pg);
      A=intersect_ker(P, MA, U, l); 
      B=intersect_ker(Q, MB, U, l);
      if (DEBUGLEVEL>=4) (void)timer2();
      An=(GEN) FpXYQQ_pow(A,stoi(pg),U,P,l)[2];
      Bn=(GEN) FpXYQQ_pow(B,stoi(pg),U,Q,l)[2];
      if (DEBUGLEVEL>=4) msgtimer("pows [P,Q]");
      z=FpXQ_inv(Bn,U,l);
      z=FpXQ_mul(An,z,U,l);
      L=ffsqrtnmod(z,ipg,U,l,NULL);
      if (DEBUGLEVEL>=4) msgtimer("ffsqrtn");
      if ( !L )
        err(talker,"Polynomials not irreducible in Fp_intersect");
      B=FpXQX_FpXQ_mul(B,L,U,l);
      B=gsubst(B,MAXVARN,gzero);
      A=gsubst(A,MAXVARN,gzero);
    }
  }
  if (e!=0)
  {
    GEN VP,VQ,moinsun,Ay,By,lmun;
    int i,j;
    moinsun=stoi(-1);
    lmun=addis(l,-1);
    MA=gaddmat(moinsun,MA);
    MB=gaddmat(moinsun,MB);
    Ay=polun[vp];
    By=polun[vq];
    VP=cgetg(np+1,t_COL);
    VP[1]=un;
    for(i=2;i<=np;i++) VP[i]=zero;
    if (np==nq) VQ=VP;/*save memory*/
    else
    {
      VQ=cgetg(nq+1,t_COL);
      VQ[1]=un;
      for(i=2;i<=nq;i++) VQ[i]=zero;
    }
    for(j=0;j<e;j++)
    {
      if (j)
      {
	Ay=FpXQ_mul(Ay,FpXQ_pow(Ap,lmun,P,l),P,l);
	for(i=1;i<lg(Ay)-1;i++) VP[i]=Ay[i+1];
	for(;i<=np;i++) VP[i]=zero;
      }
      Ap=FpM_invimage(MA,VP,l);
      Ap=vec_to_pol(Ap,vp);
      if (j)
      {
	By=FpXQ_mul(By,FpXQ_pow(Bp,lmun,Q,l),Q,l);
	for(i=1;i<lg(By)-1;i++) VQ[i]=By[i+1];
	for(;i<=nq;i++) VQ[i]=zero;
      }
      Bp=FpM_invimage(MB,VQ,l);
      Bp=vec_to_pol(Bp,vq);
      if (DEBUGLEVEL>=4) msgtimer("FpM_invimage");
    }
  }/*FpX_add is not clean, so we must do it *before* lbot=avma*/
  A=FpX_add(A,Ap,NULL);
  B=FpX_add(B,Bp,NULL);
  lbot=avma;
  *SP=FpX_red(A,l);
  *SQ=FpX_red(B,l);
  gptr[0]=SP;gptr[1]=SQ;
  gerepilemanysp(ltop,lbot,gptr,2);
}
/* Let l be a prime number, P, Q in ZZ[X].  P and Q are both
 * irreducible modulo l and degree(P) divides degree(Q).  Output a
 * monomorphism between FF_l[X]/(P) and FF_l[X]/(Q) as a polynomial R such
 * that Q | P(R) mod l.  If P and Q have the same degree, it is of course an
 * isomorphism.  */
GEN Fp_isom(GEN P,GEN Q,GEN l)
{
  pari_sp ltop=avma;
  GEN SP,SQ,R;
  Fp_intersect(degpol(P),P,Q,l,&SP,&SQ,NULL,NULL);
  R=Fp_inv_isom(SP,P,l);
  R=FpX_FpXQ_compo(R,SQ,Q,l);
  return gerepileupto(ltop,R);
}
GEN
Fp_factorgalois(GEN P,GEN l, long d, long w, GEN MP)
{
  pari_sp ltop=avma;
  GEN R,V,Tl,z,M;
  long n,k,m;
  long v;
  v=varn(P);
  n=degpol(P);
  m=n/d;
  if (DEBUGLEVEL>=4) (void)timer2();
  M=FpM_Frobenius_pow(MP,d,P,l);
  if (DEBUGLEVEL>=4) msgtimer("Fp_factorgalois: frobenius power");
  Tl=gcopy(P); setvarn(Tl,w);
  V=cgetg(m+1,t_VEC);
  V[1]=lpolx[w];
  z=pol_to_vec((GEN)V[1],n);
  for(k=2;k<=m;k++)
  {
    z=FpM_FpV_mul(M,z,l);
    V[k]=(long)vec_to_pol(z,w);
  }
  if (DEBUGLEVEL>=4) msgtimer("Fp_factorgalois: roots");
  R=FqV_roots_to_pol(V,Tl,l,v);
  if (DEBUGLEVEL>=4) msgtimer("Fp_factorgalois: pol");
  return gerepileupto(ltop,R);
}
/* P,Q irreducible over F_l. Factor P over FF_l[X] / Q  [factors are ordered as
 * a Frobenius cycle] */
GEN
Fp_factor_irred(GEN P,GEN l, GEN Q)
{
  pari_sp ltop=avma, av;
  GEN SP,SQ,MP,MQ,M,FP,FQ,E,V,IR,res;
  long np=degpol(P),nq=degpol(Q);
  long i,d=cgcd(np,nq);
  long vp=varn(P),vq=varn(Q);
  if (d==1)
  {	
    res=cgetg(2,t_COL);
    res[1]=lcopy(P);
    return res;
  }
  if (DEBUGLEVEL>=4) (void)timer2();
  FP=matrixpow(np,np,FpXQ_pow(polx[vp],l,P,l),P,l);
  FQ=matrixpow(nq,nq,FpXQ_pow(polx[vq],l,Q,l),Q,l);
  if (DEBUGLEVEL>=4) msgtimer("matrixpows");
  Fp_intersect(d,P,Q,l,&SP,&SQ,FP,FQ);
  av=avma;
  E=Fp_factorgalois(P,l,d,vq,FP);
  E=polpol_to_mat(E,np);
  MP = matrixpow(np,d,SP,P,l);
  IR = (GEN)FpM_indexrank(MP,l)[1];
  E = rowextract_p(E, IR);
  M = rowextract_p(MP,IR);
  M = FpM_inv(M,l);
  MQ = matrixpow(nq,d,SQ,Q,l);
  M = FpM_mul(MQ,M,l);
  M = FpM_mul(M,E,l);
  M = gerepileupto(av,M);
  V = cgetg(d+1,t_VEC);
  V[1]=(long)M;
  for(i=2;i<=d;i++)
    V[i]=(long)FpM_mul(FQ,(GEN)V[i-1],l);
  res=cgetg(d+1,t_COL);
  for(i=1;i<=d;i++)
    res[i]=(long)mat_to_polpol((GEN)V[i],vp,vq);
  return gerepilecopy(ltop,res);
}
GEN Fp_factor_rel0(GEN P,GEN l, GEN Q)
{
  pari_sp ltop=avma, tetpil;
  GEN V,ex,F,y,R;
  long n,nbfact=0,nmax=lgpol(P);
  long i;
  F=factmod0(P,l);
  n=lg((GEN)F[1]);
  V=cgetg(nmax,t_VEC);
  ex=cgetg(nmax,t_VECSMALL);
  for(i=1;i<n;i++)
  {
    int j,r;
    R=Fp_factor_irred(gmael(F,1,i),l,Q);
    r=lg(R);
    for (j=1;j<r;j++)
    {
      V[++nbfact]=R[j];
      ex[nbfact]=mael(F,2,i);
    }
  }
  setlg(V,nbfact+1);
  setlg(ex,nbfact+1);
  tetpil=avma; y=cgetg(3,t_VEC);
  y[1]=lcopy(V);
  y[2]=lcopy(ex);
  (void)sort_factor(y,cmp_pol);
  return gerepile(ltop,tetpil,y);
}
GEN Fp_factor_rel(GEN P, GEN l, GEN Q)
{
  pari_sp tetpil, av=avma;
  long nbfact;
  long j;
  GEN y,u,v;
  GEN z=Fp_factor_rel0(P,l,Q),t = (GEN) z[1],ex = (GEN) z[2];
  nbfact=lg(t);
  tetpil=avma; y=cgetg(3,t_MAT);
  u=cgetg(nbfact,t_COL); y[1]=(long)u;
  v=cgetg(nbfact,t_COL); y[2]=(long)v;
  for (j=1; j<nbfact; j++)
  {
    u[j] = lcopy((GEN)t[j]);
    v[j] = lstoi(ex[j]);
  }
  return gerepile(av,tetpil,y);
}

/*******************************************************************/
static GEN
to_intmod(GEN x, GEN p)
{
  GEN a = cgetg(3,t_INTMOD);
  a[1] = (long)p;
  a[2] = lmodii(x,p); return a;
}

/* z in Z[X], return z * Mod(1,p), normalized*/
GEN
FpX(GEN z, GEN p)
{
  long i,l = lg(z);
  GEN x = cgetg(l,t_POL);
  if (isonstack(p)) p = icopy(p);
  for (i=2; i<l; i++) x[i] = (long)to_intmod((GEN)z[i], p);
  x[1] = z[1]; return normalizepol_i(x,l);
}

/* z in Z^n, return z * Mod(1,p), normalized*/
GEN
FpV(GEN z, GEN p)
{
  long i,l = lg(z);
  GEN x = cgetg(l,typ(z));
  if (isonstack(p)) p = icopy(p);
  for (i=1; i<l; i++) x[i] = (long)to_intmod((GEN)z[i], p);
  return x;
}
/* z in Mat m,n(Z), return z * Mod(1,p), normalized*/
GEN
FpM(GEN z, GEN p)
{
  long i,j,l = lg(z), m = lg((GEN)z[1]);
  GEN x,y,zi;
  if (isonstack(p)) p = icopy(p);
  x = cgetg(l,t_MAT);
  for (i=1; i<l; i++)
  {
    x[i] = lgetg(m,t_COL);
    y = (GEN)x[i];
    zi= (GEN)z[i];
    for (j=1; j<m; j++) y[j] = (long)to_intmod((GEN)zi[j], p);
  }
  return x;
}
/* z in Z[X] or Z, return lift(z * Mod(1,p)), normalized*/
GEN
FpX_red(GEN z, GEN p)
{
  long i,l;
  GEN x;
  if (typ(z) == t_INT) return modii(z,p);
  l = lg(z); x = cgetg(l,t_POL);
  for (i=2; i<l; i++) x[i] = lmodii((GEN)z[i],p);
  x[1] = z[1]; return FpX_renormalize(x,l);
}

GEN
FpXV_red(GEN z, GEN p)
{
  long i,l = lg(z);
  GEN x = cgetg(l, t_VEC);
  for (i=1; i<l; i++) x[i] = (long)FpX_red((GEN)z[i], p);
  return x;
}

/* z in Z^n, return lift(z * Mod(1,p)) */
GEN
FpV_red(GEN z, GEN p)
{
  long i,l = lg(z);
  GEN x = cgetg(l,typ(z));
  for (i=1; i<l; i++) x[i] = lmodii((GEN)z[i],p);
  return x;
}

/* z in Mat m,n(Z), return lift(z * Mod(1,p)) */
GEN
FpM_red(GEN z, GEN p)
{
  long i, l = lg(z);
  GEN x = cgetg(l,t_MAT);
  for (i=1; i<l; i++) x[i] = (long)FpV_red((GEN)z[i], p);
  return x;
}

/* no garbage collection, divide by leading coeff, mod p */
GEN
FpX_normalize(GEN z, GEN p)
{
  GEN p1 = leading_term(z);
  if (lg(z) == 2 || gcmp1(p1)) return z;
  return FpX_Fp_mul(z, mpinvmod(p1,p), p);
}

GEN
FpXQX_normalize(GEN z, GEN T, GEN p)
{
  GEN p1 = leading_term(z);
  if (lg(z) == 2 || gcmp1(p1)) return z;
  if (!T) return FpX_normalize(z,p);
  return FpXQX_FpXQ_mul(z, Fq_inv(p1,T,p), T,p);
}

/* z in R[X,Y] representing an elt in R[X,Y] mod T(Y) in Kronecker form,
 * i.e subst(lift(z), x, y^(2deg(z)-1)). Recover the "real" z, with
 * normalized coefficients */
GEN
from_Kronecker(GEN z, GEN T)
{
  long i,j,lx,l = lg(z), N = (degpol(T)<<1) + 1;
  GEN a,x, t = cgetg(N,t_POL);
  t[1] = T[1] & VARNBITS;
  lx = (l-2) / (N-2); x = cgetg(lx+3,t_POL);
  if (isonstack(T)) T = gcopy(T);
  for (i=2; i<lx+2; i++)
  {
    a = cgetg(3,t_POLMOD); x[i] = (long)a;
    a[1] = (long)T;
    for (j=2; j<N; j++) t[j] = z[j];
    z += (N-2);
    a[2] = lres(normalizepol_i(t,N), T);
  }
  a = cgetg(3,t_POLMOD); x[i] = (long)a;
  a[1] = (long)T;
  N = (l-2) % (N-2) + 2;
  for (j=2; j<N; j++) t[j] = z[j];
  a[2] = lres(normalizepol_i(t,N), T);
  return normalizepol_i(x, i+1);
}

/*******************************************************************/
/*                                                                 */
/*                          MODULAR GCD                            */
/*                                                                 */
/*******************************************************************/
int
u_val(ulong n, ulong p)
{
  ulong dummy;
  return svaluation(n,p,&dummy);
}

/* assume p^k is SMALL */
int
u_pow(int p, int k)
{
  int i, pk;

  if (!k) return 1;
  if (p == 2) return 1<<k;
  pk = p; for (i=2; i<=k; i++) pk *= p;
  return pk;
}

#if 0
static ulong
umodratu(GEN a, ulong p)
{
  if (typ(a) == t_INT)
    return umodiu(a,p);
  else { /* assume a a t_FRAC */
    ulong num = umodiu((GEN)a[1],p);
    ulong den = umodiu((GEN)a[2],p);
    return (ulong)muluumod(num, invumod(den,p), p);
  }
}
#endif

/*FIXME: Unify the following 3 divrem routines. Treat the case x,y (lifted) in
 * R[X], y non constant. Given: (lifted) [inv(), mul()], (delayed) red() in R */

/* x and y in Z[X]. Possibly x in Z */
GEN
FpX_divrem(GEN x, GEN y, GEN p, GEN *pr)
{
  long vx, dx, dy, dz, i, j, sx, lrem;
  pari_sp av0, av, tetpil;
  GEN z,p1,rem,lead;

  if (!p) err(bugparier,"FpX_divrem, p==NULL");
  if (!signe(y)) err(gdiver);
  vx = varn(x);
  dy = degpol(y);
  dx = (typ(x)==t_INT)? 0: degpol(x);
  if (dx < dy)
  {
    if (pr)
    {
      av0 = avma; x = FpX_red(x, p);
      if (pr == ONLY_DIVIDES) { avma=av0; return signe(x)? NULL: gzero; }
      if (pr == ONLY_REM) return x;
      *pr = x;
    }
    return zeropol(vx);
  }
  lead = leading_term(y);
  if (!dy) /* y is constant */
  {
    if (pr && pr != ONLY_DIVIDES)
    {
      if (pr == ONLY_REM) return zeropol(vx);
      *pr = zeropol(vx);
    }
    if (gcmp1(lead)) return gcopy(x);
    av0 = avma; x = gmul(x, mpinvmod(lead,p)); tetpil = avma;
    return gerepile(av0,tetpil,FpX_red(x,p));
  }
  av0 = avma; dz = dx-dy;
  if (OK_ULONG(p))
  { /* assume ab != 0 mod p */
    ulong pp = (ulong)p[2];
    GEN a = ZX_Flx(x, pp);
    GEN b = ZX_Flx(y, pp);
    z = Flx_divrem(a,b,pp, pr);
    avma = av0; /* HACK: assume pr last on stack, then z */
    setlg(z, lg(z)); z = dummycopy(z);
    if (pr && pr != ONLY_DIVIDES && pr != ONLY_REM)
    {
      setlg(*pr, lg(*pr)); *pr = dummycopy(*pr);
      *pr = Flx_ZX_inplace(*pr);
    }
    return Flx_ZX_inplace(z);
  }
  lead = gcmp1(lead)? NULL: gclone(mpinvmod(lead,p));
  avma = av0;
  z=cgetg(dz+3,t_POL); z[1] = x[1];
  x += 2; y += 2; z += 2;

  p1 = (GEN)x[dx]; av = avma;
  z[dz] = lead? lpileupto(av, modii(mulii(p1,lead), p)): licopy(p1);
  for (i=dx-1; i>=dy; i--)
  {
    av=avma; p1=(GEN)x[i];
    for (j=i-dy+1; j<=i && j<=dz; j++)
      p1 = subii(p1, mulii((GEN)z[j],(GEN)y[i-j]));
    if (lead) p1 = mulii(p1,lead);
    tetpil=avma; z[i-dy] = lpile(av,tetpil,modii(p1, p));
  }
  if (!pr) { if (lead) gunclone(lead); return z-2; }

  rem = (GEN)avma; av = (pari_sp)new_chunk(dx+3);
  for (sx=0; ; i--)
  {
    p1 = (GEN)x[i];
    for (j=0; j<=i && j<=dz; j++)
      p1 = subii(p1, mulii((GEN)z[j],(GEN)y[i-j]));
    tetpil=avma; p1 = modii(p1,p); if (signe(p1)) { sx = 1; break; }
    if (!i) break;
    avma=av;
  }
  if (pr == ONLY_DIVIDES)
  {
    if (lead) gunclone(lead);
    if (sx) { avma=av0; return NULL; }
    avma = (pari_sp)rem; return z-2;
  }
  lrem=i+3; rem -= lrem;
  rem[0] = evaltyp(t_POL) | evallg(lrem);
  rem[1] = z[-1];
  p1 = gerepile((pari_sp)rem,tetpil,p1);
  rem += 2; rem[i]=(long)p1;
  for (i--; i>=0; i--)
  {
    av=avma; p1 = (GEN)x[i];
    for (j=0; j<=i && j<=dz; j++)
      p1 = subii(p1, mulii((GEN)z[j],(GEN)y[i-j]));
    tetpil=avma; rem[i]=lpile(av,tetpil, modii(p1,p));
  }
  rem -= 2;
  if (lead) gunclone(lead);
  if (!sx) (void)FpX_renormalize(rem, lrem);
  if (pr == ONLY_REM) return gerepileupto(av0,rem);
  *pr = rem; return z-2;
}

/* x and y in Z[Y][X]. Assume T irreducible mod p */
GEN
FpXQX_divrem(GEN x, GEN y, GEN T, GEN p, GEN *pr)
{
  long vx, dx, dy, dz, i, j, sx, lrem;
  pari_sp av0, av, tetpil;
  GEN z,p1,rem,lead;

  if (!p) return poldivrem(x,y,pr);
  if (!T) return FpX_divrem(x,y,p,pr);
  if (!signe(y)) err(gdiver);
  vx=varn(x); dy=degpol(y); dx=degpol(x);
  if (dx < dy)
  {
    if (pr)
    {
      av0 = avma; x = FpXQX_red(x, T, p);
      if (pr == ONLY_DIVIDES) { avma=av0; return signe(x)? NULL: gzero; }
      if (pr == ONLY_REM) return x;
      *pr = x;
    }
    return zeropol(vx);
  }
  lead = leading_term(y);
  if (!dy) /* y is constant */
  {
    if (pr && pr != ONLY_DIVIDES)
    {
      if (pr == ONLY_REM) return zeropol(vx);
      *pr = zeropol(vx);
    }
    if (gcmp1(lead)) return gcopy(x);
    av0 = avma; x = gmul(x, Fq_inv(lead,T,p)); tetpil = avma;
    return gerepile(av0,tetpil,FpXQX_red(x,T,p));
  }
  av0 = avma; dz = dx-dy;
#if 0 /* FIXME: to be done */
  if (OK_ULONG(p))
  { /* assume ab != 0 mod p */
  }
#endif
  lead = gcmp1(lead)? NULL: gclone(Fq_inv(lead,T,p));
  avma = av0;
  z = cgetg(dz+3,t_POL); z[1] = x[1];
  x += 2; y += 2; z += 2;

  p1 = (GEN)x[dx]; av = avma;
  z[dz] = lead? lpileupto(av, FpX_rem(gmul(p1,lead), T, p)): lcopy(p1);
  for (i=dx-1; i>=dy; i--)
  {
    av=avma; p1=(GEN)x[i];
    for (j=i-dy+1; j<=i && j<=dz; j++)
      p1 = gsub(p1, gmul((GEN)z[j],(GEN)y[i-j]));
    if (lead) p1 = gmul(FpX_rem(p1, T, p), lead);
    tetpil=avma; z[i-dy] = lpile(av,tetpil,FpX_rem(p1, T, p));
  }
  if (!pr) { if (lead) gunclone(lead); return z-2; }

  rem = (GEN)avma; av = (pari_sp)new_chunk(dx+3);
  for (sx=0; ; i--)
  {
    p1 = (GEN)x[i];
    for (j=0; j<=i && j<=dz; j++)
      p1 = gsub(p1, gmul((GEN)z[j],(GEN)y[i-j]));
    tetpil=avma; p1 = FpX_rem(p1, T, p); if (signe(p1)) { sx = 1; break; }
    if (!i) break;
    avma=av;
  }
  if (pr == ONLY_DIVIDES)
  {
    if (lead) gunclone(lead);
    if (sx) { avma=av0; return NULL; }
    avma = (pari_sp)rem; return z-2;
  }
  lrem=i+3; rem -= lrem;
  rem[0] = evaltyp(t_POL) | evallg(lrem);
  rem[1] = z[-1];
  p1 = gerepile((pari_sp)rem,tetpil,p1);
  rem += 2; rem[i]=(long)p1;
  for (i--; i>=0; i--)
  {
    av=avma; p1 = (GEN)x[i];
    for (j=0; j<=i && j<=dz; j++)
      p1 = gsub(p1, gmul((GEN)z[j],(GEN)y[i-j]));
    tetpil=avma; rem[i]=lpile(av,tetpil, FpX_rem(p1, T, p));
  }
  rem -= 2;
  if (lead) gunclone(lead);
  if (!sx) (void)FpXQX_renormalize(rem, lrem);
  if (pr == ONLY_REM) return gerepileupto(av0,rem);
  *pr = rem; return z-2;
}

/* R = any base ring */
GEN
RXQX_red(GEN P, GEN T)
{
  long i, l = lg(P);
  GEN Q = cgetg(l, t_POL);
  Q[1] = P[1];
  for (i=2; i<l; i++) Q[i] = lres((GEN)P[i], T);
  return Q;
}

/* R = any base ring */
GEN
RXQV_red(GEN P, GEN T)
{
  long i, l = lg(P);
  GEN Q = cgetg(l, typ(P));
  for (i=1; i<l; i++) Q[i] = lres((GEN)P[i], T);
  return Q;
}

/* x and y in (R[Y]/T)[X]  (lifted), T in R[Y]. y preferably monic */
GEN
RXQX_divrem(GEN x, GEN y, GEN T, GEN *pr)
{
  long vx, dx, dy, dz, i, j, sx, lrem;
  pari_sp av0, av, tetpil;
  GEN z,p1,rem,lead;

  if (!signe(y)) err(gdiver);
  vx = varn(x);
  dx = degpol(x);
  dy = degpol(y);
  if (dx < dy)
  {
    if (pr)
    {
      av0 = avma; x = RXQX_red(x, T);
      if (pr == ONLY_DIVIDES) { avma=av0; return signe(x)? NULL: gzero; }
      if (pr == ONLY_REM) return x;
      *pr = x;
    }
    return zeropol(vx);
  }
  lead = leading_term(y);
  if (!dy) /* y is constant */
  {
    if (pr && pr != ONLY_DIVIDES)
    {
      if (pr == ONLY_REM) return zeropol(vx);
      *pr = zeropol(vx);
    }
    if (gcmp1(lead)) return gcopy(x);
    av0 = avma; x = gmul(x, ginvmod(lead,T)); tetpil = avma;
    return gerepile(av0,tetpil,RXQX_red(x,T));
  }
  av0 = avma; dz = dx-dy;
  lead = gcmp1(lead)? NULL: gclone(ginvmod(lead,T));
  avma = av0;
  z = cgetg(dz+3,t_POL); z[1] = x[1];
  x += 2; y += 2; z += 2;

  p1 = (GEN)x[dx]; av = avma;
  z[dz] = lead? lpileupto(av, gres(gmul(p1,lead), T)): lcopy(p1);
  for (i=dx-1; i>=dy; i--)
  {
    av=avma; p1=(GEN)x[i];
    for (j=i-dy+1; j<=i && j<=dz; j++)
      p1 = gsub(p1, gmul((GEN)z[j],(GEN)y[i-j]));
    if (lead) p1 = gmul(gres(p1, T), lead);
    tetpil=avma; z[i-dy] = lpile(av,tetpil, gres(p1, T));
  }
  if (!pr) { if (lead) gunclone(lead); return z-2; }

  rem = (GEN)avma; av = (pari_sp)new_chunk(dx+3);
  for (sx=0; ; i--)
  {
    p1 = (GEN)x[i];
    for (j=0; j<=i && j<=dz; j++)
      p1 = gsub(p1, gmul((GEN)z[j],(GEN)y[i-j]));
    tetpil=avma; p1 = gres(p1, T); if (signe(p1)) { sx = 1; break; }
    if (!i) break;
    avma=av;
  }
  if (pr == ONLY_DIVIDES)
  {
    if (lead) gunclone(lead);
    if (sx) { avma=av0; return NULL; }
    avma = (pari_sp)rem; return z-2;
  }
  lrem=i+3; rem -= lrem;
  rem[0] = evaltyp(t_POL) | evallg(lrem);
  rem[1] = z[-1];
  p1 = gerepile((pari_sp)rem,tetpil,p1);
  rem += 2; rem[i]=(long)p1;
  for (i--; i>=0; i--)
  {
    av=avma; p1 = (GEN)x[i];
    for (j=0; j<=i && j<=dz; j++)
      p1 = gsub(p1, gmul((GEN)z[j],(GEN)y[i-j]));
    tetpil=avma; rem[i]=lpile(av,tetpil, gres(p1, T));
  }
  rem -= 2;
  if (lead) gunclone(lead);
  if (!sx) (void)normalizepol_i(rem, lrem);
  if (pr == ONLY_REM) return gerepileupto(av0,rem);
  *pr = rem; return z-2;
}

/* x and y in Z[X], return lift(gcd(x mod p, y mod p)) */
GEN
FpX_gcd(GEN x, GEN y, GEN p)
{
  GEN a,b,c;
  pari_sp av0, av;

  if (OK_ULONG(p))
  {
    ulong pp=p[2];
    av = avma;
    (void)new_chunk((lg(x) + lg(y)) << 2); /* scratch space */
    a = ZX_Flx(x, pp);
    b = ZX_Flx(y, pp);
    a = Flx_gcd_i(a,b, pp);
    avma = av; return Flx_ZX(a);
  }
  av0=avma;
  a = FpX_red(x, p); av = avma;
  b = FpX_red(y, p);
  while (signe(b))
  {
    av = avma; c = FpX_rem(a,b,p); a=b; b=c;
  }
  avma = av; return gerepileupto(av0, a);
}

/*Return 1 if gcd can be computed
 * else return a factor of p*/

GEN
FpX_gcd_check(GEN x, GEN y, GEN p)
{
  GEN a,b,c;
  pari_sp av=avma;

  a = FpX_red(x, p); 
  b = FpX_red(y, p);
  while (signe(b))
  {
    GEN lead = leading_term(b);
    GEN g = mppgcd(lead,p);
    if (!is_pm1(g)) return gerepileupto(av,g);
    c = FpX_rem(a,b,p); a=b; b=c;
  }
  avma = av; return gun;
}

/* x and y in Z[X], return lift(gcd(x mod p, y mod p)). Set u and v st
 * ux + vy = gcd (mod p) */
/*TODO: Document the typ() of *ptu and *ptv*/
GEN
FpX_extgcd(GEN x, GEN y, GEN p, GEN *ptu, GEN *ptv)
{
  GEN a,b,q,r,u,v,d,d1,v1;
  pari_sp ltop=avma, lbot;
  GEN *gptr[3]; 
  if (OK_ULONG(p))
  {
    ulong pp=p[2];
    a = ZX_Flx(x, pp);
    b = ZX_Flx(y, pp);
    d = Flx_extgcd(a,b, pp, &u,&v);
    lbot=avma;
    d=Flx_ZX(d);
    u=Flx_ZX(u);
    v=Flx_ZX(v);
  }
  else
  {
    a = FpX_red(x, p);
    b = FpX_red(y, p);
    d = a; d1 = b; v = gzero; v1 = gun;
    while (signe(d1))
    {
      q = FpX_divrem(d,d1,p, &r);
      v = gadd(v, gneg_i(gmul(q,v1)));
      v = FpX_red(v,p);
      u=v; v=v1; v1=u;
      u=r; d=d1; d1=u;
    }
    u = gadd(d, gneg_i(gmul(b,v)));
    u = FpX_red(u, p);
    lbot = avma;
    u = FpX_div(u,a,p);
    d = gcopy(d);
    v = gcopy(v);
  }
  gptr[0] = &d; gptr[1] = &u; gptr[2] = &v;
  gerepilemanysp(ltop,lbot,gptr,3);
  *ptu = u; *ptv = v; return d;
}

/* x and y in Z[Y][X], return lift(gcd(x mod T,p, y mod T,p)). Set u and v st
 * ux + vy = gcd (mod T,p) */
GEN
FpXQX_extgcd(GEN x, GEN y, GEN T, GEN p, GEN *ptu, GEN *ptv)
{
  GEN a,b,q,r,u,v,d,d1,v1;
  pari_sp ltop=avma, lbot;

  a = FpXQX_red(x, T, p);
  b = FpXQX_red(y, T, p);
  d = a; d1 = b; v = gzero; v1 = gun;
  while (signe(d1))
  {
    q = FpXQX_divrem(d,d1,T,p, &r);
    v = gadd(v, gneg_i(gmul(q,v1)));
    v = FpXQX_red(v,T,p);
    u=v; v=v1; v1=u;
    u=r; d=d1; d1=u;
  }
  u = gadd(d, gneg_i(gmul(b,v)));
  u = FpXQX_red(u,T, p);
  lbot = avma;
  u = FpXQX_divrem(u,a,T,p,NULL);
  d = gcopy(d);
  v = gcopy(v);
  {
    GEN *gptr[3]; gptr[0] = &d; gptr[1] = &u; gptr[2] = &v;
    gerepilemanysp(ltop,lbot,gptr,3);
  }
  *ptu = u; *ptv = v; return d;
}

/*x must be reduced*/
GEN
FpXQ_charpoly(GEN x, GEN T, GEN p)
{
  pari_sp ltop=avma;
  long v=varn(T);
  GEN R;
  T=gcopy(T); setvarn(T,MAXVARN);
  x=gcopy(x); setvarn(x,MAXVARN);
  R=FpY_FpXY_resultant(T,deg1pol(gun,FpX_neg(x,p),v),p);
  return gerepileupto(ltop,R);
}

GEN 
FpXQ_minpoly(GEN x, GEN T, GEN p)
{
  pari_sp ltop=avma;
  GEN R=FpXQ_charpoly(x, T, p);
  GEN G=FpX_gcd(R,derivpol(R),p);
  G=FpX_normalize(G,p);
  G=FpX_div(R,G,p);
  return gerepileupto(ltop,G);
}

/* return z = a mod q, b mod p (p,q) = 1. qinv = 1/q mod p */
static GEN
u_chinese_coprime(GEN a, ulong b, GEN q, ulong p, ulong qinv, GEN pq)
{
  ulong d, amod = umodiu(a, p);
  pari_sp av = avma;
  GEN ax;

  if (b == amod) return NULL;
  d = (b > amod)? b - amod: p - (amod - b); /* (b - a) mod p */
  (void)new_chunk(lgefint(pq)<<1); /* HACK */
  ax = mului(muluumod(d,qinv,p), q); /* d mod p, 0 mod q */
  avma = av; return addii(a, ax); /* in ]-q, pq[ assuming a in -]-q,q[ */
}

/* centerlift(u mod p) */
long
u_center(ulong u, ulong p, ulong ps2)
{
  return (long) (u > ps2)? u - p: u;
}

GEN
ZX_init_CRT(GEN Hp, ulong p, long v)
{
  long i, l = lg(Hp), lim = (long)(p>>1);
  GEN H = cgetg(l, t_POL);
  H[1] = evalsigne(1) | evalvarn(v);
  for (i=2; i<l; i++)
    H[i] = lstoi(u_center(Hp[i], p, lim));
  return H;
}

/* assume lg(Hp) > 1 */
GEN 
ZM_init_CRT(GEN Hp, ulong p)
{
  long i,j, m = lg(Hp[1]), l = lg(Hp), lim = (long)(p>>1);
  GEN c,cp,H = cgetg(l, t_MAT);
  for (j=1; j<l; j++)
  {
    cp = (GEN)Hp[j];
    c = cgetg(m, t_COL);
    H[j] = (long)c;
    for (i=1; i<l; i++) c[i] = lstoi(u_center(cp[i],p, lim));
  }   
  return H;
}

int
Z_incremental_CRT(GEN *H, ulong Hp, GEN q, GEN qp, ulong p)
{
  GEN h, lim = shifti(qp,-1);
  ulong qinv = invumod(umodiu(q,p), p);
  int stable = 1;
  h = u_chinese_coprime(*H,Hp,q,p,qinv,qp);
  if (h)
  {
    if (cmpii(h,lim) > 0) h = subii(h,qp);
    *H = h; stable = 0;
  }
  return stable;
}

int
ZX_incremental_CRT(GEN *ptH, GEN Hp, GEN q, GEN qp, ulong p)
{
  GEN H = *ptH, h, lim = shifti(qp,-1);
  ulong qinv = invumod(umodiu(q,p), p);
  long i, l = lg(H), lp = lg(Hp);
  int stable = 1;

  if (l < lp)
  { /* degree increases */
    GEN x = cgetg(lp, t_POL);
    for (i=1; i<l; i++) x[i] = H[i];
    for (   ; i<lp; i++)
    {
      h = stoi(Hp[i]);
      if (cmpii(h,lim) > 0) h = subii(h,qp);
      x[i] = (long)h;
    }
    *ptH = H = x;
    stable = 0; lp = l;
  }
  for (i=2; i<lp; i++)
  {
    h = u_chinese_coprime((GEN)H[i],Hp[i],q,p,qinv,qp);
    if (h)
    {
      if (cmpii(h,lim) > 0) h = subii(h,qp);
      H[i] = (long)h; stable = 0;
    }
  }
  for (   ; i<l; i++)
  {
    h = u_chinese_coprime((GEN)H[i],   0,q,p,qinv,qp);
    if (h)
    {
      if (cmpii(h,lim) > 0) h = subii(h,qp);
      H[i] = (long)h; stable = 0;
    }
  }
  return stable;
}

int
ZM_incremental_CRT(GEN H, GEN Hp, GEN q, GEN qp, ulong p)
{
  GEN h, lim = shifti(qp,-1);
  ulong qinv = invumod(umodiu(q,p), p);
  long i,j, l = lg(H), m = lg(H[1]);
  int stable = 1;
  for (j=1; j<l; j++)
    for (i=1; i<m; i++)
    {
      h = u_chinese_coprime(gcoeff(H,i,j), coeff(Hp,i,j),q,p,qinv,qp);
      if (h)
      {
        if (cmpii(h,lim) > 0) h = subii(h,qp);
        coeff(H,i,j) = (long)h; stable = 0;
      }
    }
  return stable;
}

/* returns a polynomial in variable v, whose coeffs correspond to the digits
 * of m (in base p) */
GEN
stopoly(ulong m, ulong p, long v)
{
  GEN y = new_chunk(BITS_IN_LONG + 2);
  long l = 2;
  do { ulong q = m/p; y[l++] = lutoi(m - q*p); m=q; } while (m);
  y[1] = evalsigne(1) | evalvarn(v); 
  y[0] = evaltyp(t_POL) | evallg(l); return y;
}

GEN
stopoly_gen(GEN m, GEN p, long v)
{
  GEN *y = (GEN*)new_chunk(bit_accuracy(lgefint(m))+2);
  long l = 2;
  do { m = dvmdii(m,p,&(y[l])); l++; } while (signe(m));
  y[1] = (GEN)(evalsigne(1) | evalvarn(v));
  y[0] = (GEN)(evaltyp(t_POL) | evallg(l)); return (GEN)y;
}

static GEN
muliimod(GEN x, GEN y, GEN p)
{
  return modii(mulii(x,y), p);
}

/* Res(A,B) = Res(B,R) * lc(B)^(a-r) * (-1)^(ab), with R=A%B, a=deg(A) ...*/
GEN
FpX_resultant(GEN a, GEN b, GEN p)
{
  long da,db,dc;
  pari_sp av, lim;
  GEN c,lb, res = gun;

  if (!signe(a) || !signe(b)) return gzero;
  da = degpol(a);
  db = degpol(b);
  if (db > da)
  {
    swapspec(a,b, da,db);
    if (both_odd(da,db)) res = subii(p, res);
  }
  if (!da) return gun; /* = res * a[2] ^ db, since 0 <= db <= da = 0 */
  av = avma; lim = stack_lim(av,2);
  while (db)
  {
    lb = (GEN)b[db+2];
    c = FpX_rem(a,b, p);
    a = b; b = c; dc = degpol(c);
    if (dc < 0) { avma = av; return 0; }

    if (both_odd(da,db)) res = subii(p, res);
    if (!gcmp1(lb)) res = muliimod(res, powgumod(lb, da - dc, p), p);
    if (low_stack(lim,stack_lim(av,2)))
    {
      if (DEBUGMEM>1) err(warnmem,"FpX_resultant (da = %ld)",da);
      gerepileall(av,3, &a,&b,&res);
    }
    da = db; /* = degpol(a) */
    db = dc; /* = degpol(b) */
  }
  res = muliimod(res, powgumod((GEN)b[2], da, p), p);
  return gerepileuptoint(av, res);
}

/* assuming the PRS finishes on a degree 1 polynomial C0 + C1X, with "generic"
 * degree sequence as given by dglist, set *Ci and return resultant(a,b) */
ulong
u_FpX_resultant_all(GEN a, GEN b, long *C0, long *C1, GEN dglist, ulong p)
{
  long da,db,dc,cnt,ind;
  ulong lb, cx = 1, res = 1UL;
  pari_sp av;
  GEN c;

  if (C0) { *C0 = 1; *C1 = 0; }
  if (lgpol(a)==0 || lgpol(b)==0) return 0;
  da = degpol(a);
  db = degpol(b);
  if (db > da)
  {
    swapspec(a,b, da,db);
    if (both_odd(da,db)) res = p-res;
  }
  /* = res * a[2] ^ db, since 0 <= db <= da = 0 */
  if (!da) return 1;
  cnt = ind = 0; av = avma;
  while (db)
  {
    lb = b[db+2];
    c = Flx_rem(a,b, p);
    a = b; b = c; dc = degpol(c);
    if (dc < 0) { avma = av; return 0; }

    ind++;
    if (C0)
    { /* check that Euclidean remainder sequence doesn't degenerate */
      if (dc != dglist[ind]) { avma = av; return 0; }
      /* update resultant */
      if (both_odd(da,db)) res = p-res;
      if (lb != 1)
      {
        ulong t = powuumod(lb, da - dc, p);
        res = muluumod(res, t, p);
        if (dc) cx = muluumod(cx, t, p);
      }
    }
    else
    {
      if (dc > dglist[ind]) dglist[ind] = dc;
    }
    if (++cnt == 4) { cnt = 0; avma = av; }
    da = db; /* = degpol(a) */
    db = dc; /* = degpol(b) */
  }
  if (!C0)
  {
    if (ind+1 > lg(dglist)) setlg(dglist,ind+1);
    return 0;
  }

  if (da == 1) /* last non-constant polynomial has degree 1 */
  {
    *C0 = muluumod(cx, a[2], p);
    *C1 = muluumod(cx, a[3], p);
    lb = b[2];
  } else lb = powuumod(b[2], da, p);
  avma = av; return muluumod(res, lb, p);
}

/* u P(X) + v P(-X) */
static GEN
pol_comp(GEN P, GEN u, GEN v)
{
  long i, l = lg(P);
  GEN y = cgetg(l, t_POL);
  for (i=2; i<l; i++)
  {
    GEN t = (GEN)P[i];
    y[i] = gcmp0(t)? zero:
                     (i&1)? lmul(t, gsub(u,v)) /*  odd degree */
                          : lmul(t, gadd(u,v));/* even degree */
  }
  y[1] = P[1]; return normalizepol_i(y,l);
}

GEN
polint_triv(GEN xa, GEN ya)
{
  GEN P = NULL, Q = roots_to_pol(xa,0);
  long i, n = lg(xa);
  pari_sp av = avma, lim = stack_lim(av, 2);
  for (i=1; i<n; i++)
  {
    GEN T,dP;
    if (gcmp0((GEN)ya[i])) continue;
    T = gdeuc(Q, gsub(polx[0], (GEN)xa[i]));
    if (i < n-1 && absi_equal((GEN)xa[i], (GEN)xa[i+1]))
    { /* x_i = -x_{i+1} */
      T = gdiv(T, poleval(T, (GEN)xa[i]));
      dP = pol_comp(T, (GEN)ya[i], (GEN)ya[i+1]);
      i++;
    }
    else
    {
      dP = gmul((GEN)ya[i], T);
      dP = gdiv(dP, poleval(T,(GEN)xa[i]));
    }
    P = P? gadd(P, dP): dP;
    if (low_stack(lim,stack_lim(av,2)))
    {
      if (DEBUGMEM>1) err(warnmem,"polint_triv2 (i = %ld)",i);
      P = gerepileupto(av, P);
    }
  }
  return P? P: zeropol(0);
}

static GEN
FpX_div_by_X_x(GEN a, GEN x, GEN p)
{
  long l = lg(a), i;
  GEN z = cgetg(l-1, t_POL), a0, z0;
  z[1] = evalsigne(1) | evalvarn(0);
  a0 = a + l-1;
  z0 = z + l-2; *z0 = *a0--;
  for (i=l-3; i>1; i--) /* z[i] = (a[i+1] + x*z[i+1]) % p */
  {
    GEN t = addii((GEN)*a0--, muliimod(x, (GEN)*z0--, p));
    *z0 = (long)t;
  }
  return z;
}

GEN
FpV_polint(GEN xa, GEN ya, GEN p)
{
  GEN inv,T,dP, P = NULL, Q = FpV_roots_to_pol(xa, p, 0);
  long i, n = lg(xa);
  pari_sp av, lim;
  av = avma; lim = stack_lim(av,2);
  for (i=1; i<n; i++)
  {
    if (!signe(ya[i])) continue;
    T = FpX_div_by_X_x(Q, (GEN)xa[i], p);
    inv = mpinvmod(FpX_eval(T,(GEN)xa[i], p), p);
    if (i < n-1 && egalii(addii((GEN)xa[i], (GEN)xa[i+1]), p))
    {
      dP = pol_comp(T, muliimod((GEN)ya[i],  inv,p),
                       muliimod((GEN)ya[i+1],inv,p));
      i++; /* x_i = -x_{i+1} */
    }
    else
      dP = FpX_Fp_mul(T, muliimod((GEN)ya[i],inv,p), p);
    P = P? FpX_add(P, dP, p): dP;
    if (low_stack(lim, stack_lim(av,2)))
    {
      if (DEBUGMEM>1) err(warnmem,"FpV_polint");
      if (!P) avma = av; else P = gerepileupto(av, P);
    }
  }
  return P? P: zeropol(0);
}

static void
u_FpV_polint_all(GEN xa, GEN ya, GEN C0, GEN C1, ulong p)
{
  GEN T,Q = Flv_roots_to_pol(xa, p, 0);
  GEN dP  = NULL,  P = NULL;
  GEN dP0 = NULL, P0= NULL;
  GEN dP1 = NULL, P1= NULL;
  long i, n = lg(xa);
  ulong inv;
  for (i=1; i<n; i++)
  {
    T = Flx_div_by_X_x(Q, xa[i], p, NULL);
    inv = invumod(Flx_eval(T,xa[i], p), p);

    if (ya[i])
    {
      dP = Flx_Fl_mul(T, muluumod(ya[i],inv,p), p);
      P = P ? Flx_add(P , dP , p): dP;
    }
    if (C0[i])
    {
      dP0= Flx_Fl_mul(T, muluumod(C0[i],inv,p), p);
      P0= P0? Flx_add(P0, dP0, p): dP0;
    }
    if (C1[i])
    {
      dP1= Flx_Fl_mul(T, muluumod(C1[i],inv,p), p);
      P1= P1? Flx_add(P1, dP1, p): dP1;
    }
  }
  ya[1] = (long) (P ? P : Flx_zero(0));
  C0[1] = (long) (P0? P0: Flx_zero(0));
  C1[1] = (long) (P1? P1: Flx_zero(0));
}

/* b a vector of polynomials representing B in Fp[X][Y], evaluate at X = x,
 * Return 0 in case of degree drop. */
static GEN
u_vec_FpX_eval(GEN b, ulong x, ulong p)
{
  GEN z;
  long i, lb = lg(b);
  ulong leadz = Flx_eval(leading_term(b), x, p);
  long vs=mael(b,2,1);
  if (!leadz) return Flx_zero(vs);

  z = cgetg(lb, t_VECSMALL); z[1] = vs;
  for (i=2; i<lb-1; i++) z[i] = Flx_eval((GEN)b[i], x, p);
  z[i] = leadz; return z;
}

/* as above, but don't care about degree drop */
static GEN
u_vec_FpX_eval_gen(GEN b, ulong x, ulong p, int *drop)
{
  GEN z;
  long i, lb = lg(b);
  z = cgetg(lb,t_VECSMALL); z[1]=mael(b,2,1);

  for (i=2; i<lb; i++) z[i] = Flx_eval((GEN)b[i], x, p);
  z = Flx_renormalize(z, lb);
  *drop = lb - lg(z);
  return z;
}

static GEN
vec_FpX_eval_gen(GEN b, GEN x, GEN p, int *drop)
{
  GEN z;
  long i, lb = lg(b);
  z = cgetg(lb, t_POL);
  z[1] = b[1];
  for (i=2; i<lb; i++)
    z[i] = (long)FpX_eval((GEN)b[i], x, p);
  z = FpX_renormalize(z, lb);
  *drop = lb - lg(z);
  return z;
}

/* Interpolate at roots of 1 and use Hadamard bound for univariate resultant:
 *   bound = N_2(A)^degpol B N_2(B)^degpol(A),  where
 *     N_2(A) = sqrt(sum (N_1(Ai))^2)
 * Return e such that Res(A, B) < 2^e */
ulong
ZY_ZXY_ResBound(GEN A, GEN B, GEN dB)
{
  pari_sp av = avma;
  GEN a = gzero, b = gzero;
  long i , lA = lg(A), lB = lg(B);
  double loga, logb;
  for (i=2; i<lA; i++) a = addii(a, sqri((GEN)A[i]));
  for (i=2; i<lB; i++)
  {
    GEN t = (GEN)B[i];
    if (typ(t) == t_POL) t = gnorml1(t, 0);
    b = addii(b, sqri(t));
  }
  loga = mylog2(a);
  logb = mylog2(b); if (dB) logb -= 2 * mylog2(dB);
  i = (long)((degpol(B) * loga + degpol(A) * logb) / 2);
  avma = av; return (i <= 0)? 1: 1 + (ulong)i;
}

/* return Res(a(Y), b(n,Y)) over Fp. la = leading_term(a) [for efficiency] */
static ulong
u_FpX_eval_resultant(GEN a, GEN b, ulong n, ulong p, ulong la)
{
  int drop;
  GEN ev = u_vec_FpX_eval_gen(b, n, p, &drop);
  ulong r = Flx_resultant(a, ev, p);
  if (drop && la != 1) r = muluumod(r, powuumod(la, drop,p),p);
  return r;
}
static GEN
FpX_eval_resultant(GEN a, GEN b, GEN n, GEN p, GEN la)
{
  int drop;
  GEN ev = vec_FpX_eval_gen(b, n, p, &drop);
  GEN r = FpX_resultant(a, ev, p);
  if (drop && !gcmp1(la)) r = muliimod(r, powgumod(la, drop,p),p);
  return r;
}

/* assume dres := deg(Res_Y(a,b), X) <= deg(a,Y) * deg(b,X) < p */
static GEN
Fly_Flxy_resultant_polint(GEN a, GEN b, ulong p, ulong dres)
{
  ulong i, n, la = (ulong)leading_term(a);
  GEN  x = cgetg(dres+2, t_VECSMALL);
  GEN  y = cgetg(dres+2, t_VECSMALL);
 /* Evaluate at dres+ 1 points: 0 (if dres even) and +/- n, so that P_n(X) =
  * P_{-n}(-X), where P_i is Lagrange polynomial: P_i(j) = delta_{i,j} */
  for (i=0,n = 1; i < dres; n++)
  {
    x[++i] = n;   y[i] = u_FpX_eval_resultant(a,b, x[i], p,la);
    x[++i] = p-n; y[i] = u_FpX_eval_resultant(a,b, x[i], p,la);
  }
  if (i == dres)
  {
    x[++i] = 0;   y[i] = u_FpX_eval_resultant(a,b, x[i], p,la);
  }
  return Flv_polint(x,y, p, evalvarn(varn(b)));
}

/* x^n mod p */
ulong
powusmod(ulong x, long n, ulong p)
{
  if (n < 0)
    return powuumod(invumod(x, p), (ulong)(-n), p);
  else
    return powuumod(x, (ulong)n, p);
}

static GEN
u_FpXX_pseudorem(GEN x, GEN y, ulong p)
{
  long vx = varn(x), dx, dy, dz, i, lx, dp;
  pari_sp av = avma, av2, lim;

  if (!signe(y)) err(gdiver);
  (void)new_chunk(2);
  dx=degpol(x); x = revpol(x);
  dy=degpol(y); y = revpol(y); dz=dx-dy; dp = dz+1;
  av2 = avma; lim = stack_lim(av2,1);
  for (;;)
  {
    x[0] = (long)Flx_neg((GEN)x[0], p); dp--;
    for (i=1; i<=dy; i++)
      x[i] = (long)Flx_add( Flx_mul((GEN)y[0], (GEN)x[i], p),
                              Flx_mul((GEN)x[0], (GEN)y[i], p), p );
    for (   ; i<=dx; i++)
      x[i] = (long)Flx_mul((GEN)y[0], (GEN)x[i], p);
    do { x++; dx--; } while (dx >= 0 && lg((GEN)x[0])==2);
    if (dx < dy) break;
    if (low_stack(lim,stack_lim(av2,1)))
    {
      if(DEBUGMEM>1) err(warnmem,"pseudorem dx = %ld >= %ld",dx,dy);
      gerepilemanycoeffs(av2,x,dx+1);
    }
  }
  if (dx < 0) return Flx_zero(0);
  lx = dx+3; x -= 2;
  x[0]=evaltyp(t_POL) | evallg(lx);
  x[1]=evalsigne(1) | evalvarn(vx);
  x = revpol(x) - 2;
  if (dp)
  { /* multiply by y[0]^dp   [beware dummy vars from FpY_FpXY_resultant] */
    GEN t = Flx_pow((GEN)y[0], dp, p);
    for (i=2; i<lx; i++)
      x[i] = (long)Flx_mul((GEN)x[i], t, p);
  }
  return gerepilecopy(av, x);
}

static GEN
u_FpX_divexact(GEN x, GEN y, ulong p)
{
  long i, l;
  GEN z;
  if (degpol(y) == 0)
  {
    ulong t = (ulong)y[2];
    if (t == 1) return x;
    t = invumod(t, p);
    l = lg(x); z = cgetg(l, t_POL); z[1] = x[1];
    for (i=2; i<l; i++) z[i] = (long)Flx_Fl_mul((GEN)x[i],t,p);
  }
  else
  {
    l = lg(x); z = cgetg(l, t_POL); z[1] = x[1];
    for (i=2; i<l; i++) z[i] = (long)Flx_div((GEN)x[i],y,p);
  }
  return z;
}

static GEN
Flyx_subres(GEN u, GEN v, ulong p)
{
  pari_sp av = avma, av2, lim;
  long degq,dx,dy,du,dv,dr,signh;
  GEN z,g,h,r,p1;

  dx=degpol(u); dy=degpol(v); signh=1;
  if (dx < dy)
  {
    swap(u,v); lswap(dx,dy);
    if (both_odd(dx, dy)) signh = -signh;
  }
  if (dy < 0) return gzero;
  if (dy==0) return gerepileupto(av, Flx_pow((GEN)v[2],dx,p));

  g = h = Fl_Flx(1,0); av2 = avma; lim = stack_lim(av2,1);
  for(;;)
  {
    r = u_FpXX_pseudorem(u,v,p); dr = lg(r);
    if (dr == 2) { avma = av; return gzero; }
    du = degpol(u); dv = degpol(v); degq = du-dv;
    u = v; p1 = g; g = leading_term(u);
    switch(degq)
    {
      case 0: break;
      case 1:
        p1 = Flx_mul(h,p1, p); h = g; break;
      default:
        p1 = Flx_mul(Flx_pow(h,degq,p), p1, p);
        h = Flx_div(Flx_pow(g,degq,p), Flx_pow(h,degq-1,p), p);
    }
    if (both_odd(du,dv)) signh = -signh;
    v = u_FpX_divexact(r, p1, p);
    if (dr==3) break;
    if (low_stack(lim,stack_lim(av2,1)))
    {
      if(DEBUGMEM>1) err(warnmem,"subresall, dr = %ld",dr);
      gerepileall(av2,4, &u, &v, &g, &h);
    }
  }
  z = (GEN)v[2];
  if (dv > 1) z = Flx_div(Flx_pow(z,dv,p), Flx_pow(h,dv-1,p), p);
  if (signh < 0) z = Flx_neg(z,p);
  return gerepileupto(av, z);
}

/* return a t_POL (in variable v) whose coeffs are the coeffs of b,
 * in variable v. This is an incorrect PARI object if initially varn(b) << v.
 * We could return a vector of coeffs, but it is convenient to have degpol()
 * and friends available. Even in that case, it will behave nicely with all
 * functions treating a polynomial as a vector of coeffs (eg poleval). 
 * FOR INTERNAL USE! */
GEN
swap_vars(GEN b0, long v)
{
  long i, n = poldegree(b0, v);
  GEN b = cgetg(n+3, t_POL), x = b + 2;
  b[1] = evalsigne(1) | evalvarn(v);
  for (i=0; i<=n; i++) x[i] = (long)polcoeff_i(b0, i, v);
  return b;
}

/* assume varn(b) << varn(a) */
GEN
FpY_FpXY_resultant(GEN a, GEN b0, GEN p)
{
  long i,n,dres, vX = varn(b0), vY = varn(a);
  GEN la,x,y,b = swap_vars(b0, vY);
 
  dres = degpol(a)*degpol(b0);
  if (OK_ULONG(p))
  {
    ulong pp = (ulong)p[2];
    b = ZXX_FlxX(b, pp, vX);
    if ((ulong)dres >= pp)
    {
      a = ZXX_FlxX(a, pp, vX);
      x = Flyx_subres(a, b, pp);
    }
    else
    {
      a = ZX_Flx(a, pp);
      x = Fly_Flxy_resultant_polint(a, b, pp, (ulong)dres);
      setvarn(x, vX);
    }
    return Flx_ZX(x);
  }
 
  la = leading_term(a);
  x = cgetg(dres+2, t_VEC);
  y = cgetg(dres+2, t_VEC);
 /* Evaluate at dres+ 1 points: 0 (if dres even) and +/- n, so that P_n(X) =
  * P_{-n}(-X), where P_i is Lagrange polynomial: P_i(j) = delta_{i,j} */
  for (i=0,n = 1; i < dres; n++)
  {
    x[++i] = lstoi(n);    y[i] = (long)FpX_eval_resultant(a,b, (GEN)x[i], p,la);
    x[++i] = lsubis(p,n); y[i] = (long)FpX_eval_resultant(a,b, (GEN)x[i], p,la);
  }
  if (i == dres)
  {
    x[++i] = zero;        y[i] = (long)FpX_eval_resultant(a,b, (GEN)x[i], p,la);
  }
  x = FpV_polint(x,y, p);
  setvarn(x, vX); return x;
}

/* check that theta(maxprime) - theta(27448) >= 2^bound */
/* NB: theta(27449) ~ 27225.387, theta(x) > 0.98 x for x>7481
 * (Schoenfeld, 1976 for x > 1155901 + direct calculations) */
static void
check_theta(ulong bound) {
  maxprime_check( (ulong)ceil((bound * LOG2 + 27225.388) / 0.98) );
}
/* 27449 = prime(3000) */
byteptr
init_modular(ulong *p) { *p = 27449; return diffptr + 3000; }

/* 0, 1, -1, 2, -2, ... */
#define next_lambda(a) (a>0 ? -a : 1-a)

/* Assume A in Z[Y], B in Q[Y][X], and Res_Y(A, B) in Z[X].
 * If lambda = NULL, return Res_Y(A,B).
 * Otherwise, find a small lambda (start from *lambda, use the sequence above)
 * such that R(X) = Res_Y(A, B(X + lambda Y)) is squarefree, reset *lambda to
 * the chosen value and return R
 *
 * If LERS is non-NULL, set it to the Last non-constant polynomial in the
 * Euclidean Remainder Sequence */
GEN
ZY_ZXY_resultant_all(GEN A, GEN B0, long *lambda, GEN *LERS)
{
  int checksqfree = lambda? 1: 0, delvar = 0, stable;
  ulong bound, p, dp;
  pari_sp av = avma, av2 = 0, lim;
  long i,n, lb, degA = degpol(A), dres = degA*degpol(B0);
  long vX = varn(B0), vY = varn(A); /* assume vX << vY */
  GEN x, y, dglist, dB, B, q, a, b, ev, H, H0, H1, Hp, H0p, H1p, C0, C1, L;
  byteptr d = init_modular(&p);

  dglist = Hp = H0p = H1p = C0 = C1 = NULL; /* gcc -Wall */
  if (LERS)
  {
    if (!lambda) err(talker,"ZY_ZXY_resultant_all: LERS needs lambda");
    C0 = cgetg(dres+2, t_VECSMALL);
    C1 = cgetg(dres+2, t_VECSMALL);
    dglist = cgetg(dres+1, t_VECSMALL);
  }
  x = cgetg(dres+2, t_VECSMALL);
  y = cgetg(dres+2, t_VECSMALL);
  if (vY == MAXVARN)
  {
    vY = fetch_var(); delvar = 1;
    B0 = gsubst(B0, MAXVARN, polx[vY]);
    A = dummycopy(A); setvarn(A, vY);
  }
  L = polx[MAXVARN];
  B0 = Q_remove_denom(B0, &dB);
  lim = stack_lim(av,2);

INIT:
  if (av2) { avma = av2; *lambda = next_lambda(*lambda); } 
  if (lambda)
  {
    L = gadd(polx[MAXVARN], gmulsg(*lambda, polx[vY]));
    if (DEBUGLEVEL>4) fprintferr("Trying lambda = %ld\n",*lambda);
  }
  B = poleval(B0, L); av2 = avma;

  if (degA <= 3)
  { /* sub-resultant faster for small degrees */
    if (LERS)
    {
      H = subresall(A,B,&q);
      if (typ(q) != t_POL || degpol(q)!=1 || !ZX_is_squarefree(H)) goto INIT;
      H0 = (GEN)q[2]; if (typ(H0) == t_POL) setvarn(H0,vX);
      H1 = (GEN)q[3]; if (typ(H1) == t_POL) setvarn(H1,vX);
    }
    else
    {
      H = subres(A,B);
      if (checksqfree && !ZX_is_squarefree(H)) goto INIT;
    }
    goto END;
  }

  /* make sure p large enough */
  while (p < (ulong)(dres<<1)) NEXT_PRIME_VIADIFF(p,d);

  H = H0 = H1 = NULL;
  lb = lg(B); 
  bound = ZY_ZXY_ResBound(A, B, dB);
  if (DEBUGLEVEL>4) fprintferr("bound for resultant coeffs: 2^%ld\n",bound);
  check_theta(bound);

  dp = 1;
  for(;;)
  {
    NEXT_PRIME_VIADIFF_CHECK(p,d);
    if (dB) { dp = smodis(dB, p); if (!dp) continue; }

    a = ZX_Flx(A, p);
    b = ZXX_FlxX(B, p, varn(A));
    if (LERS)
    {
      if (!b[lb-1] || degpol(a) < degA) continue; /* p | lc(A)lc(B) */
      if (checksqfree)
      { /* find degree list for generic Euclidean Remainder Sequence */
        int goal = min(degpol(a), degpol(b)); /* longest possible */
        for (n=1; n <= goal; n++) dglist[n] = 0;
        setlg(dglist, 1);
        for (n=0; n <= dres; n++)
        {
          ev = u_vec_FpX_eval(b, n, p);
          (void)u_FpX_resultant_all(a, ev, NULL, NULL, dglist, p);
          if (lg(dglist)-1 == goal) break;
        }
        /* last pol in ERS has degree > 1 ? */
        goal = lg(dglist)-1;
        if (degpol(B) == 1) { if (!goal) goto INIT; }
        else
        {
          if (goal <= 1) goto INIT;
          if (dglist[goal] != 0 || dglist[goal-1] != 1) goto INIT;
        }
        if (DEBUGLEVEL>4)
          fprintferr("Degree list for ERS (trials: %ld) = %Z\n",n+1,dglist);
      }

      for (i=0,n = 0; i <= dres; n++)
      {
        ev = u_vec_FpX_eval(b, n, p);
        x[++i] = n; y[i] = u_FpX_resultant_all(a, ev, C0+i, C1+i, dglist, p);
        if (!C1[i]) i--; /* C1(i) = 0. No way to recover C0(i) */
      }
      u_FpV_polint_all(x,y,C0,C1, p);
      Hp = (GEN)y[1];
      H0p= (GEN)C0[1];
      H1p= (GEN)C1[1];
    }
    else
      Hp = Fly_Flxy_resultant_polint(a, b, p, (ulong)dres);
    if (!H && degpol(Hp) != dres) continue;
    if (dp != 1) Hp = Flx_Fl_mul(Hp, powusmod(dp,-degA,p), p);
    if (checksqfree) {
      if (!Flx_is_squarefree(Hp, p)) goto INIT;
      if (DEBUGLEVEL>4) fprintferr("Final lambda = %ld\n",*lambda);
      checksqfree = 0;
    }

    if (!H)
    { /* initialize */
      q = utoi(p); stable = 0;
      H = ZX_init_CRT(Hp, p,vX);
      if (LERS) {
        H0= ZX_init_CRT(H0p, p,vX);
        H1= ZX_init_CRT(H1p, p,vX);
      }
    }
    else
    {
      GEN qp = muliu(q,p);
      stable = ZX_incremental_CRT(&H, Hp, q,qp, p);
      if (LERS) {
        stable &= ZX_incremental_CRT(&H0,H0p, q,qp, p);
        stable &= ZX_incremental_CRT(&H1,H1p, q,qp, p);
      }
      q = qp;
    }
    /* could make it probabilistic for H ? [e.g if stable twice, etc]
     * Probabilistic anyway for H0, H1 */
    if (DEBUGLEVEL>5)
      msgtimer("resultant mod %ld (bound 2^%ld, stable=%ld)", p,expi(q),stable);
    if (stable && (ulong)expi(q) >= bound) break; /* DONE */
    if (low_stack(lim, stack_lim(av,2)))
    {
      GEN *gptr[4]; gptr[0] = &H; gptr[1] = &q; gptr[2] = &H0; gptr[3] = &H1;
      if (DEBUGMEM>1) err(warnmem,"ZY_ZXY_resultant");
      gerepilemany(av2,gptr,LERS? 4: 2); 
    }
  }
END:
  setvarn(H, vX); if (delvar) (void)delete_var();
  if (LERS)
  {
    GEN z = cgetg(3, t_VEC);
    z[1] = (long)H0;
    z[2] = (long)H1; *LERS = z;
    gerepileall(av, 2, &H, LERS);
    return H;
  }
  return gerepilecopy(av, H);
}

GEN
ZY_ZXY_resultant(GEN A, GEN B, long *lambda)
{
  return ZY_ZXY_resultant_all(A, B, lambda, NULL);
}

/* If lambda = NULL, return caract(Mod(B, A)), A,B in Z[X].
 * Otherwise find a small lambda such that caract (Mod(B + lambda X, A)) is
 * squarefree */
GEN
ZX_caract_sqf(GEN A, GEN B, long *lambda, long v)
{
  pari_sp av = avma;
  GEN B0, R, a;
  long dB;
  int delvar;

  if (v < 0) v = 0;
  switch (typ(B))
  {
    case t_POL: dB = degpol(B); if (dB > 0) break;
      B = dB? (GEN)B[2]: gzero; /* fall through */
    default:
      if (lambda) { B = scalarpol(B,varn(A)); dB = 0; break;}
      return gerepileupto(av, gpowgs(gsub(polx[v], B), degpol(A)));
  }
  delvar = 0;
  if (varn(A) == 0)
  {
    long v0 = fetch_var(); delvar = 1;
    A = dummycopy(A); setvarn(A,v0);
    B = dummycopy(B); setvarn(B,v0);
  }
  B0 = cgetg(4, t_POL);
  B0[1] = evalsigne(1);
  B0[2] = (long)gneg_i(B);
  B0[3] = un;
  R = ZY_ZXY_resultant(A, B0, lambda);
  if (delvar) (void)delete_var();
  setvarn(R, v); a = leading_term(A);
  if (!gcmp1(a)) R = gdiv(R, gpowgs(a, dB));
  return gerepileupto(av, R);
}


/* B may be in Q[X], but assume A and result are integral */
GEN
ZX_caract(GEN A, GEN B, long v)
{
  return (degpol(A) < 16) ? caractducos(A,B,v): ZX_caract_sqf(A,B, NULL, v);
}

static GEN
trivial_case(GEN A, GEN B)
{
  long d;
  if (typ(A) == t_INT) return gpowgs(A, degpol(B));
  d = degpol(A);
  if (d == 0) return trivial_case((GEN)A[2],B);
  if (d < 0) return gzero;
  return NULL;
}

/* Res(A, B/dB), assuming the A,B in Z[X] and result is integer */
GEN
ZX_resultant_all(GEN A, GEN B, GEN dB, ulong bound)
{
  ulong Hp, dp, p;
  pari_sp av = avma, av2, lim;
  long degA;
  int stable;
  GEN q, a, b, H;
  byteptr d;

  if ((H = trivial_case(A,B)) || (H = trivial_case(B,A))) return H;
  q = H = NULL;
  av2 = avma; lim = stack_lim(av,2);
  degA = degpol(A);
  if (!bound)
  {
    bound = ZY_ZXY_ResBound(A, B, dB);
    if (bound > 50000)
    {
      long prec = MEDDEFAULTPREC;
      for(;; prec = (prec-1)<<1)
      {
        GEN run = realun(prec);
        GEN R = subres(gmul(A, run), gmul(B, run));
        bound = gexpo(R) + 1;
        if (!gcmp0(R) || bound <= 0) break;
      }
      if (dB) bound -= (long)(mylog2(dB)*degA);
    }
  }
  if (DEBUGLEVEL>4) fprintferr("bound for resultant: 2^%ld\n",bound);
  d = init_modular(&p);
  check_theta(bound);

  dp = 1; /* denominator mod p */
  for(;;)
  {
    NEXT_PRIME_VIADIFF_CHECK(p,d);
    if (dB) { dp = smodis(dB, p); if (!dp) continue; }

    a = ZX_Flx(A, p);
    b = ZX_Flx(B, p);
    Hp= Flx_resultant(a, b, p);
    if (dp != 1) Hp = muluumod(Hp, powusmod(dp, -degA, p), p);
    if (!H)
    {
      stable = 0; q = utoi(p);
      H = stoi(u_center(Hp, p, p>>1));
    }
    else /* could make it probabilistic ??? [e.g if stable twice, etc] */
    {
      GEN qp = muliu(q,p);
      stable = Z_incremental_CRT(&H, Hp, q,qp, p);
      q = qp;
    }
    if (DEBUGLEVEL>5)
      msgtimer("resultant mod %ld (bound 2^%ld, stable = %d)",p,expi(q),stable);
    if (stable && (ulong)expi(q) >= bound) break; /* DONE */
    if (low_stack(lim, stack_lim(av,2)))
    {
      GEN *gptr[2]; gptr[0] = &H; gptr[1] = &q;
      if (DEBUGMEM>1) err(warnmem,"ZX_resultant");
      gerepilemany(av2,gptr, 2);
    }
  }
  return gerepileuptoint(av, icopy(H));
}

GEN
ZX_resultant(GEN A, GEN B) { return ZX_resultant_all(A,B,NULL,0); }

GEN
ZX_QX_resultant(GEN A, GEN B)
{
  GEN c, d, n, R;
  pari_sp av = avma;
  B = Q_primitive_part(B, &c);
  if (!c) return ZX_resultant(A,B);
  n = numer(c);
  d = denom(c); if (is_pm1(d)) d = NULL;
  R = ZX_resultant_all(A, B, d, 0);
  if (!is_pm1(n)) R = mulii(R, gpowgs(n, degpol(A)));
  return gerepileuptoint(av, R);
}

/* assume x has integral coefficients */
GEN
ZX_disc_all(GEN x, ulong bound)
{
  pari_sp av = avma;
  GEN l, d = ZX_resultant_all(x, derivpol(x), NULL, bound);
  l = leading_term(x); if (!gcmp1(l)) d = diviiexact(d,l);
  if (degpol(x) & 2) d = negi(d);
  return gerepileuptoint(av,d);
}
GEN ZX_disc(GEN x) { return ZX_disc_all(x,0); }

int
ZX_is_squarefree(GEN x)
{
  pari_sp av = avma;
  GEN d = modulargcd(x,derivpol(x));
  int r = (lg(d) == 3); avma = av; return r;
}

static GEN
_gcd(GEN a, GEN b)
{
  if (!a) a = gun;
  if (!b) b = gun;
  return ggcd(a,b);
}

/* A0 and B0 in Q[X] */
GEN
modulargcd(GEN A0, GEN B0)
{
  GEN a,b,Hp,D,A,B,q,qp,H,g;
  long m,n;
  ulong p;
  pari_sp av2, av = avma, avlim = stack_lim(av, 1);
  byteptr d;

  if ((typ(A0) | typ(B0)) !=t_POL) err(notpoler,"modulargcd");
  if (!signe(A0)) return gcopy(B0);
  if (!signe(B0)) return gcopy(A0);
  A = primitive_part(A0, &a); check_pol_int(A, "modulargcd");
  B = primitive_part(B0, &b); check_pol_int(B, "modulargcd");
  D = _gcd(a,b);
  if (varn(A) != varn(B)) err(talker,"different variables in modulargcd");
 
  /* A, B in Z[X] */
  g = mppgcd(leading_term(A), leading_term(B)); /* multiple of lead(gcd) */
  if (degpol(A) < degpol(B)) swap(A, B);
  n = 1 + degpol(B); /* > degree(gcd) */

  av2 = avma; H = NULL;
  d = init_modular(&p);
  for(;;)
  {
    NEXT_PRIME_VIADIFF_CHECK(p,d);
    if (!umodiu(g,p)) continue;

    a = ZX_Flx(A, p);
    b = ZX_Flx(B, p); Hp = Flx_gcd_i(a,b, p);
    m = degpol(Hp);
    if (m == 0) { H = polun[varn(A0)]; break; } /* coprime. DONE */
    if (m > n) continue; /* p | Res(A/G, B/G). Discard */

    if (is_pm1(g)) /* make sure lead(H) = g mod p */
      Hp = Flx_normalize(Hp, p);
    else
    {
      ulong t = muluumod(umodiu(g, p), invumod(Hp[m+2],p), p);
      Hp = Flx_Fl_mul(Hp, t, p);
    }
    if (m < n)
    { /* First time or degree drop [all previous p were as above; restart]. */
      H = ZX_init_CRT(Hp,p,varn(A0));
      q = utoi(p); n = m; continue;
    }

    qp = muliu(q,p);
    if (ZX_incremental_CRT(&H, Hp, q, qp, p))
    { /* H stable: check divisibility */
      if (!is_pm1(g)) H = primpart(H);
      if (gcmp0(pseudorem(A,H)) && gcmp0(pseudorem(B,H))) break; /* DONE */

      if (DEBUGLEVEL) fprintferr("modulargcd: trial division failed");
    }
    q = qp;
    if (low_stack(avlim, stack_lim(av,1)))
    {
      GEN *gptr[2]; gptr[0]=&H; gptr[1]=&q;
      if (DEBUGMEM>1) err(warnmem,"modulargcd");
      gerepilemany(av2,gptr,2);
    }
  }
  return gerepileupto(av, gmul(D,H));
}

/* lift(1 / Mod(A,B)). B0 a t_POL, A0 a scalar or a t_POL. Rational coeffs */
GEN
QX_invmod(GEN A0, GEN B0)
{
  GEN a,b,D,A,B,q,qp,Up,Vp,U,V,res;
  long stable;
  ulong p;
  pari_sp av2, av = avma, avlim = stack_lim(av, 1);
  byteptr d;

  if (typ(B0) != t_POL) err(notpoler,"QX_invmod");
  if (typ(A0) != t_POL)
  {
    if (is_scalar_t(typ(A0))) return ginv(A0);
    err(notpoler,"QX_invmod");
  }
  if (degpol(A0) < 15) return ginvmod(A0,B0);
  A = primitive_part(A0, &D);
  B = primpart(B0);
  /* A, B in Z[X] */
  av2 = avma; U = NULL;
  d = init_modular(&p);
  for(;;)
  {
    NEXT_PRIME_VIADIFF_CHECK(p,d);
    a = ZX_Flx(A, p);
    b = ZX_Flx(B, p);
    /* if p | Res(A/G, B/G), discard */
    if (!Flx_extresultant(b,a,p, &Vp,&Up)) continue;

    if (!U)
    { /* First time */
      U = ZX_init_CRT(Up,p,varn(A0));
      V = ZX_init_CRT(Vp,p,varn(A0));
      q = utoi(p); continue;
    }
    if (DEBUGLEVEL>5) msgtimer("QX_invmod: mod %ld (bound 2^%ld)", p,expi(q));
    qp = muliu(q,p);
    stable  = ZX_incremental_CRT(&U, Up, q,qp, p);
    stable &= ZX_incremental_CRT(&V, Vp, q,qp, p);
    if (stable)
    { /* all stable: check divisibility */
      res = gadd(gmul(A,U), gmul(B,V));
      if (degpol(res) == 0) break; /* DONE */
      if (DEBUGLEVEL) fprintferr("QX_invmod: char 0 check failed");
    }
    q = qp;
    if (low_stack(avlim, stack_lim(av,1)))
    {
      GEN *gptr[3]; gptr[0]=&q; gptr[1]=&U; gptr[2]=&V;
      if (DEBUGMEM>1) err(warnmem,"QX_invmod");
      gerepilemany(av2,gptr,3);
    }
  }
  D = D? gmul(D,res): res;
  return gerepileupto(av, gdiv(U,D));
}

/* irreducible (unitary) polynomial of degree n over Fp */
GEN
ffinit_rand(GEN p,long n)
{
  pari_sp av = avma;
  GEN pol;

  for(;; avma = av)
  {
    pol = gadd(gpowgs(polx[0],n), FpX_rand(n-1,0, p));
    if (FpX_is_irred(pol, p)) break;
  }
  return pol;
}

GEN
FpX_direct_compositum(GEN A, GEN B, GEN p)
{
  GEN C,a,b,x;
  a = dummycopy(A); setvarn(a, MAXVARN);
  b = dummycopy(B); setvarn(b, MAXVARN);
  x = gadd(polx[0], polx[MAXVARN]);
  C = FpY_FpXY_resultant(a, poleval(b,x),p);
  return C;
}

GEN
FpX_compositum(GEN A, GEN B, GEN p)
{
  GEN C, a,b;
  long k;

  a = dummycopy(A); setvarn(a, MAXVARN);
  b = dummycopy(B); setvarn(b, MAXVARN);
  for (k = 1;; k = next_lambda(k))
  {
    GEN x = gadd(polx[0], gmulsg(k, polx[MAXVARN]));
    C = FpY_FpXY_resultant(a, poleval(b,x),p);
    if (FpX_is_squarefree(C, p)) break;
  }
  return C;
}

/* return an extension of degree 2^l of F_2, assume l > 0 
 * using Adleman-Lenstra Algorithm.
 * Not stack clean. */
static GEN
f2init(long l)
{
  long i;
  GEN q, T, S;

  if (l == 1) return cyclo(3, MAXVARN);

  S = coefs_to_pol(4, gun,gun,gzero,gzero); /* #(#^2 + #) */
  setvarn(S, MAXVARN);
  q = coefs_to_pol(3, gun,gun, S); /* X^2 + X + #(#^2+#) */

  /* x^4+x+1, irred over F_2, minimal polynomial of a root of q */
  T = coefs_to_pol(5, gun,gzero,gzero,gun,gun);
  for (i=2; i<l; i++)
  { /* q = X^2 + X + a(#) irred. over K = F2[#] / (T(#))
     * ==> X^2 + X + a(#) b irred. over K for any root b of q
     * ==> X^2 + X + (b^2+b)b */
    setvarn(T, MAXVARN);
    T = FpY_FpXY_resultant(T, q, gdeux);
    /* T = minimal polynomial of b over F2 */
  }
  return T;
}

/* return an extension of degree p^l of F_p, assume l > 0 
 * using Adleman-Lenstra Algorithm, see below.
 * Not stack clean. */
GEN
ffinit_Artin_Shreier(GEN ip, long l)
{
  long i;
  long p=itos(ip);
  GEN x,xp,yp,y2pm1;
  GEN P, Q;
  xp=monomial(gun,p,0);
  P = FpX_sub(xp,deg1pol(gun,gun,0),NULL);
  if (l == 1) return P;
  x=polx[0];
  yp=monomial(gun,p,MAXVARN);
  y2pm1=monomial(gun,2*p-1,MAXVARN);
  Q = gsub(FpX_sub(xp, x, NULL), FpX_sub(y2pm1, yp, NULL));
  for (i = 2; i <= l; ++i)
  {
    setvarn(P,MAXVARN);
    P = FpY_FpXY_resultant(P, Q, ip);
  }
  return P;
}


/*Check if subcyclo(n,l,0) is irreducible modulo p*/
static long
fpinit_check(GEN p, long n, long l)
{
  pari_sp ltop=avma;
  long q,o;
  if (!isprime(stoi(n))) {avma=ltop; return 0;}
  q = smodis(p,n);
  if (!q) {avma=ltop; return 0;}
  o = itos(order(gmodulss(q,n)));
  avma = ltop;
  return ( cgcd((n-1)/o,l) == 1 );
}

/* let k=2 if p%4==1, and k=4 else and assume k*p does not divide l.
 * Return an irreducible polynomial of degree l over F_p.
 * This a variant of an algorithm of Adleman and Lenstra
 * "Finding irreducible polynomials over finite fields",
 * ACM, 1986 (5)  350--355
 * Not stack clean.
 */
static GEN
fpinit(GEN p, long l)
{
  ulong n = 1+l, k = 1;
  while (!fpinit_check(p,n,l)) { n += l; k++; }
  if (DEBUGLEVEL>=4)
    fprintferr("FFInit: using subcyclo(%ld, %ld)\n",n,l);
  return FpX_red(subcyclo(n,l,0),p);
}

GEN
ffinit_fact(GEN p, long n)
{
  pari_sp ltop=avma;
  GEN F;	  /* vecsmall */
  GEN P;	  /* pol */
  long i;
  F = decomp_primary_small(n);
  /* If n is even, then F[1] is 2^bfffo(n)*/
  if (!(n&1) && egalii(p, gdeux))
    P = f2init(vals(n));
  else
    P = fpinit(p, F[1]);
  for (i = 2; i < lg(F); ++i)
    P = FpX_direct_compositum(fpinit(p, F[i]), P, p);
  return gerepileupto(ltop,FpX(P,p));
}

GEN
ffinit_nofact(GEN p, long n)
{
  pari_sp av = avma;
  GEN P,Q=NULL;
  if (lgefint(p)==3)
  {
    ulong lp=p[2], q;
    long v=svaluation(n,lp,&q);
    if (v>0)
    {
      if (lp==2)
        Q=f2init(v);
      else
        Q=fpinit(p,n/q);
      n=q;
    }
  }
  if (n==1)
    P=Q;
  else
  {
    P = fpinit(p, n);
    if (Q) P = FpX_direct_compositum(P, Q, p);
  }
  return gerepileupto(av, FpX(P,p));
}

GEN
ffinit(GEN p, long n, long v)
{
  pari_sp ltop=avma;
  GEN P;
  if (n <= 0) err(talker,"non positive degree in ffinit");
  if (typ(p) != t_INT) err(typeer, "ffinit");
  if (v < 0) v = 0;
  if (n == 1) return FpX(polx[v],p);
  /*If we are in a easy case just use cyclo*/
  if (fpinit_check(p, n + 1, n))
    return gerepileupto(ltop,FpX(cyclo(n + 1, v),p));
  if ((ulong)lgefint(p)-2 < BITS_IN_LONG-bfffo(n))
    P=ffinit_fact(p,n);
  else
    P=ffinit_nofact(p,n);
  setvarn(P, v);
  return P;
}
