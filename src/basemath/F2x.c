/* $Id$

Copyright (C) 2007  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#include "pari.h"
#include "paripriv.h"

/* Not so fast arithmetic with polynomials over F_2 */

/***********************************************************************/
/**                                                                   **/
/**                             F2x                                   **/
/**                                                                   **/
/***********************************************************************/
/* F2x objects are defined as follows:
   An F2x is a t_VECSMALL:
   x[0] = codeword
   x[1] = evalvarn(variable number)  (signe is not stored).
   x[2] = a_0...a_31 x[3] = a_32..a_63, etc.  on 32bit
   x[2] = a_0...a_63 x[3] = a_64..a_127, etc. on 64bit

   where the a_i are bits.

   signe(x) is not valid. Use degpol(x)>=0 instead.
   Note: F2x(0)=Flx(0 mod 2) ans F2x(1)=Flx(1 mod 2)
*/

GEN
polx_F2x(long sv) { return mkvecsmall2(sv, 2); }

INLINE long
F2x_degree_lg(GEN x, long l)
{
  if (l==2) return -1;
  else return ((l-3)<<TWOPOTBITS_IN_LONG)+BITS_IN_LONG-1-bfffo(x[l-1]);
}

long
F2x_degree(GEN x)
{
  return F2x_degree_lg(x, lg(x));
}

INLINE ulong
F2x_get_coeff(GEN x,long v)
{
   ulong u=(ulong)x[2+(v>>TWOPOTBITS_IN_LONG)];
   return (u>>(v&(BITS_IN_LONG-1)))&1UL;
}

INLINE void
F2x_set_coeff(GEN x,long v)
{
   ulong* u=(ulong*)&x[2+(v>>TWOPOTBITS_IN_LONG)];
   *u^=1UL<<(v&(BITS_IN_LONG-1UL));
}

GEN
F2x_to_ZX(GEN x)
{
  long l=3+F2x_degree(x);
  GEN z=cgetg(l,t_POL);
  long i,j,k;
  for(i=2,k=2;i<lg(x);i++)
    for(j=0; j<BITS_IN_LONG && k<l; j++,k++)
      gel(z,k)=(x[i]&(1UL<<j))?gen_1:gen_0;
  z[1]=evalsigne(l>=0)|x[1];
  return z;
}

GEN
F2x_to_Flx(GEN x)
{
  long l=3+F2x_degree(x);
  GEN z=cgetg(l,t_VECSMALL);
  long i,j,k;
  z[1]=x[1];
  for(i=2,k=2;i<lg(x);i++)
    for(j=0;j<BITS_IN_LONG && k<l; j++,k++)
      z[k]=(x[i]>>j)&1UL;
  return z;
}

GEN
ZX_to_F2x(GEN x)
{
  long l=(lgpol(x)+BITS_IN_LONG-1)>>TWOPOTBITS_IN_LONG;
  GEN z=cgetg(2+l,t_VECSMALL);
  long i,j,k;
  z[1]=((ulong)x[1])&VARNBITS;
  for(i=2, k=1,j=BITS_IN_LONG;i<lg(x);i++,j++)
  {
    if (j==BITS_IN_LONG)
    {
      j=0; k++; z[k]=0;
    }
    if (mpodd(gel(x,i)))
      z[k]|=1UL<<j;
  }
  return F2x_renormalize(z,2+l);
}

GEN
Flx_to_F2x(GEN x)
{
  long l=(lgpol(x)+BITS_IN_LONG-1)>>TWOPOTBITS_IN_LONG;
  GEN z=cgetg(2+l,t_VECSMALL);
  long i,j,k;
  z[1]=x[1];
  for(i=2, k=1,j=BITS_IN_LONG;i<lg(x);i++,j++)
  {
    if (j==BITS_IN_LONG)
    {
      j=0; k++; z[k]=0;
    }
    if (x[i])
      z[k]|=(1<<j);
  }
  return F2x_renormalize(z,2+l);
}

GEN
F2x_add(GEN x, GEN y)
{
  long i,lz;
  GEN z;
  long lx=lg(x);
  long ly=lg(y);
  if (ly>lx) swapspec(x,y, lx,ly);
  lz = lx; z = cgetg(lz, t_VECSMALL); z[1]=x[1];
  for (i=2; i<ly; i++) z[i] = x[i]^y[i];
  for (   ; i<lx; i++) z[i] = x[i];
  return F2x_renormalize(z, lz);
}

static GEN
F2x_addspec(GEN x, GEN y, long lx, long ly)
{
  long i,lz;
  GEN z;

  if (ly>lx) swapspec(x,y, lx,ly);
  lz = lx+2; z = cgetg(lz, t_VECSMALL) + 2;
  for (i=0; i<ly; i++) z[i] = x[i]^y[i];
  for (   ; i<lx; i++) z[i] = x[i];
  z -= 2; return F2x_renormalize(z, lz);
}

GEN
F2x_1_add(GEN y)
{
  GEN z;
  long lz, i;
  if (!lgpol(y))
    return pol1_F2x(y[1]);
  lz=lg(y);
  z=cgetg(lz,t_VECSMALL);
  z[1] = y[1];
  z[2] = y[2]^1UL;
  for(i=3;i<lz;i++)
    z[i] = y[i];
  if (lz==3) z = F2x_renormalize(z,lz);
  return z;
}

static GEN
F2x_addshift(GEN x, GEN y, long d)
{
  GEN xd,yd,zd = (GEN)avma;
  long a,lz,ny = lgpol(y), nx = lgpol(x);
  long vs = x[1];

  x += 2; y += 2; a = ny-d;
  if (a <= 0)
  {
    lz = (a>nx)? ny+2: nx+d+2;
    (void)new_chunk(lz); xd = x+nx; yd = y+ny;
    while (xd > x) *--zd = *--xd;
    x = zd + a;
    while (zd > x) *--zd = 0;
  }
  else
  {
    xd = new_chunk(d); yd = y+d;
    x = F2x_addspec(x,yd,nx,a);
    lz = (a>nx)? ny+2: lg(x)+d;
    x += 2; while (xd > x) *--zd = *--xd;
  }
  while (yd > y) *--zd = *--yd;
  *--zd = vs;
  *--zd = evaltyp(t_VECSMALL) | evallg(lz); return zd;
}

/* shift polynomial + gerepile */
/* Do not set evalvarn*/
static GEN
F2x_shiftip(pari_sp av, GEN x, long v)
{
  long i, lx = lg(x), ly;
  GEN y;
  if (!v || lx==2) return gerepileuptoleaf(av, x);
  avma = av; ly = lx + v;
  x += lx; y = new_chunk(ly) + ly; /*cgetg could overwrite x!*/
  for (i = 2; i<lx; i++) *--y = *--x;
  for (i = 0; i< v; i++) *--y = 0;
  y -= 2; y[0] = evaltyp(t_VECSMALL) | evallg(ly);
  return y;
}

static GEN
F2x_mul1(ulong x, ulong y)
{
  ulong x1=(x&HIGHMASK)>>BITS_IN_HALFULONG;
  ulong x2=x&LOWMASK;
  ulong y1=(y&HIGHMASK)>>BITS_IN_HALFULONG;
  ulong y2=y&LOWMASK;
  ulong r1,r2,rr;
  GEN z;
  ulong i;
  rr=r1=r2=0UL;
  if (x2)
    for(i=0;i<BITS_IN_HALFULONG;i++)
      if (x2&(1UL<<i))
      {
        r2^=y2<<i;
        rr^=y1<<i;
      }
  if (x1)
    for(i=0;i<BITS_IN_HALFULONG;i++)
      if (x1&(1UL<<i))
      {
        r1^=y1<<i;
        rr^=y2<<i;
      }
  r2^=(rr&LOWMASK)<<BITS_IN_HALFULONG;
  r1^=(rr&HIGHMASK)>>BITS_IN_HALFULONG;
  z=cgetg((r1?4:3),t_VECSMALL);
  z[2]=r2;
  if (r1) z[3]=r1;
  return z;
}

GEN F2x_mul(GEN x, GEN y);

/* fast product (Karatsuba) of polynomials a,b. These are not real GENs, a+2,
 * b+2 were sent instead. na, nb = number of terms of a, b.
 * Only c, c0, c1, c2 are genuine GEN.
 */
static GEN
F2x_mulspec(GEN a, GEN b, long na, long nb)
{
  GEN a0,c,c0;
  long n0, n0a, i, v = 0;
  pari_sp av;

  while (na && !a[0]) { a++; na-=1; v++; }
  while (nb && !b[0]) { b++; nb-=1; v++; }
  if (na < nb) swapspec(a,b, na,nb);
  if (!nb) return zero_Flx(0);

  av = avma;
  if (na <=1) return F2x_shiftip(av,F2x_mul1(*a,*b),v);
  i=(na>>1); n0=na-i; na=i;
  a0=a+n0; n0a=n0;
  while (n0a && !a[n0a-1]) n0a--;

  if (nb > n0)
  {
    GEN b0,c1,c2;
    long n0b;

    nb -= n0; b0 = b+n0; n0b = n0;
    while (n0b && !b[n0b-1]) n0b--;
    c =  F2x_mulspec(a,b,n0a,n0b);
    c0 = F2x_mulspec(a0,b0,na,nb);

    c2 = F2x_addspec(a0,a,na,n0a);
    c1 = F2x_addspec(b0,b,nb,n0b);

    c1 = F2x_mul(c1,c2);
    c2 = F2x_add(c0,c);

    c2 = F2x_add(c1,c2);
    c0 = F2x_addshift(c0,c2,n0);
  }
  else
  {
    c  = F2x_mulspec(a,b,n0a,nb);
    c0 = F2x_mulspec(a0,b,na,nb);
  }
  c0 = F2x_addshift(c0,c,n0);
  return F2x_shiftip(av,c0, v);
}

GEN
F2x_mul(GEN x, GEN y)
{
  GEN z = F2x_mulspec(x+2,y+2, lgpol(x),lgpol(y));
  z[1] = x[1]; return z;
}

GEN
F2x_sqr(GEN x)
{
  const ulong sq[]={0,1,4,5,16,17,20,21,64,65,68,69,80,81,84,85};
  long i,ii,j,jj;
  long lx=lg(x), lz=2+((lx-2)<<1);
  GEN z;
  z = cgetg(lz, t_VECSMALL); z[1]=x[1];
  for (j=2,jj=2;j<lx;j++,jj++)
  {
    ulong x1=((ulong)x[j]&HIGHMASK)>>BITS_IN_HALFULONG;
    ulong x2=(ulong)x[j]&LOWMASK;
    z[jj]=0;
    if (x2)
      for(i=0,ii=0;i<BITS_IN_HALFULONG;i+=4,ii+=8)
        z[jj]|=sq[(x2>>i)&15UL]<<ii;
    z[++jj]=0;
    if (x1)
      for(i=0,ii=0;i<BITS_IN_HALFULONG;i+=4,ii+=8)
        z[jj]|=sq[(x1>>i)&15UL]<<ii;
  }
  return F2x_renormalize(z, lz);
}

INLINE void
F2x_addshiftip(GEN x, GEN y, ulong d)
{
  ulong dl=d>>TWOPOTBITS_IN_LONG;
  ulong db=d&(BITS_IN_LONG-1UL);
  long i;
  if (db)
  {
    ulong dc=BITS_IN_LONG-db;
    ulong r=0;
    for(i=2; i<lg(y); i++)
    {
      x[i+dl] ^= (((ulong)y[i])<<db)|r;
      r = ((ulong)y[i])>>dc;
    }
    if (r) x[i+dl] ^= r;
  }
  else
    for(i=2; i<lg(y); i++)
      x[i+dl] ^= y[i];
}

/* separate from F2x_divrem for maximal speed. */
GEN
F2x_rem(GEN x, GEN y)
{
  long dx,dy;
  long lx=lg(x);
  dy = F2x_degree(y); if (!dy) return zero_Flx(x[1]);
  dx = F2x_degree_lg(x,lx);
  x  = vecsmall_copy(x);
  while (dx>=dy)
  {
    F2x_addshiftip(x,y,dx-dy);
    while (lx>2 && x[lx-1]==0) lx--;
    dx=F2x_degree_lg(x,lx);
  }
  return F2x_renormalize(x, lx);
}

GEN
F2x_divrem(GEN x, GEN y, GEN *pr)
{
  GEN z;
  long dx,dy,dz,i;
  long vs=x[1];
  long lx=lg(x);

  if (pr == ONLY_REM) return F2x_rem(x, y);
  dy = F2x_degree(y);
  if (!dy)
  {
    z = vecsmall_copy(x);
    if (pr) *pr = zero_Flx(vs);
    return z;
  }
  dx = F2x_degree_lg(x,lx);
  dz = dx-dy;
  if (dz < 0)
  {
    z = zero_Flx(vs);
    if (pr) *pr = vecsmall_copy(x);
    return z;
  }
  z = cgetg(lg(x)-lg(y)+3, t_VECSMALL); z[1] = vs;
  x = vecsmall_copy(x);
  for (i=2; i<lg(z); i++) z[i]=0;
  while (dx>=dy)
  {
    F2x_set_coeff(z,dx-dy);
    F2x_addshiftip(x,y,dx-dy);
    if (x[lx-1]==0) lx--;
    dx = F2x_degree_lg(x,lx);
  }
  z = F2x_renormalize(z, lg(z));
  if (!pr) { cgiv(x); return z; }
  x = F2x_renormalize(x, lx);
  *pr = x; return z;
}

GEN
F2x_gcd(GEN a, GEN b)
{
  pari_sp av=avma;
  GEN c;
  if (lg(b) > lg(a)) swap(a, b);
  while (lgpol(b))
  {
    c = F2x_rem(a,b);
    a = b; b = c;
  }
  return gerepileuptoleaf(av,a);
}

GEN
F2x_extgcd(GEN a, GEN b, GEN *ptu, GEN *ptv)
{
  GEN q,z,u,v, x = a, y = b;

  u = zero_Flx(a[1]);
  v = pol1_F2x(a[1]); /* v = 1 */
  while (lgpol(y))
  {
    q = F2x_divrem(x,y,&z);
    x = y; y = z; /* (x,y) = (y, x - q y) */
    z = F2x_add(u, F2x_mul(q,v));
    u = v; v = z; /* (u,v) = (v, u - q v) */
  }
  z = F2x_add(x, F2x_mul(b,u));
  z = F2x_div(z,a);
  *ptu = z;
  *ptv = u; return x;
}

GEN
F2xq_mul(GEN x,GEN y,GEN pol)
{
  GEN z = F2x_mul(x,y);
  return F2x_rem(z,pol);
}

GEN
F2xq_sqr(GEN x,GEN pol)
{
  GEN z = F2x_sqr(x);
  return F2x_rem(z,pol);
}

GEN
F2xq_invsafe(GEN x, GEN T)
{
  GEN U, V;
  GEN z = F2x_extgcd(x, T, &U, &V);
  if (degpol(z)) return NULL;
  return U;
}

GEN
F2xq_inv(GEN x,GEN T)
{
  pari_sp av=avma;
  GEN U = F2xq_invsafe(x, T);
  if (!U) pari_err(talker,"non invertible polynomial in F2xq_inv");
  return gerepileuptoleaf(av, U);
}

GEN
F2xq_div(GEN x,GEN y,GEN T)
{
  pari_sp av = avma;
  return gerepileuptoleaf(av, F2xq_mul(x,F2xq_inv(y,T),T));
}

static GEN
_F2xq_sqr(void *data, GEN x)
{
  GEN pol = (GEN) data;
  return F2xq_sqr(x, pol);
}

static GEN
_F2xq_mul(void *data, GEN x, GEN y)
{
  GEN pol = (GEN) data;
  return F2xq_mul(x,y, pol);
}

GEN
F2xq_pow(GEN x, GEN n, GEN pol)
{
  pari_sp av=avma;
  GEN y;

  if (!signe(n)) return pol1_F2x(x[1]);
  if (is_pm1(n)) /* +/- 1 */
    return (signe(n) < 0)? F2xq_inv(x,pol): vecsmall_copy(x);

  if (signe(n) < 0) x = F2xq_inv(x,pol);
  y = leftright_pow(x, n, (void*)pol, &_F2xq_sqr, &_F2xq_mul);
  return gerepileupto(av, y);
}

static GEN
_F2xq_pow(void *data, GEN x, GEN n)
{
  GEN pol = (GEN) data;
  return F2xq_pow(x,n, pol);
}

GEN
random_F2x(long d, long vs)
{
  long i, l = 3 + (d>>TWOPOTBITS_IN_LONG);
  GEN y = cgetg(l,t_VECSMALL); y[1] = vs;
  for (i=2; i<l; i++) y[i] = pari_rand();
  y[l-1] &= (1UL<<(d&(BITS_IN_LONG-1UL)))-1UL;
  return F2x_renormalize(y,l);
}

static GEN
_F2xq_rand(void *data)
{
  pari_sp av=avma;
  GEN pol = (GEN) data;
  long d = F2x_degree(pol);
  GEN z;
  do
  {
    avma = av;
    z = random_F2x(d,pol[1]);
  } while (lgpol(z)==0);
  return z;
}

const static struct bb_group F2xq_star={_F2xq_mul,_F2xq_pow,_F2xq_rand,vecsmall_lexcmp,F2x_cmp1};

GEN
F2xq_order(GEN a, GEN ord, GEN T)
{
  return gen_eltorder(a,ord,(void*)T,&F2xq_star);
}

GEN
F2xq_log(GEN a, GEN g, GEN ord, GEN T)
{
  return gen_PH_log(a,g,ord,(void*)T,&F2xq_star,NULL);
}

GEN
F2xq_sqrtn(GEN a, GEN n, GEN T, GEN *zeta)
{
  if (!lgpol(a))
  {
    if (zeta)
      *zeta=pol1_F2x(T[1]);
    return zero_Flx(T[1]);
  }
  return gen_Shanks_sqrtn(a,n,addis(powuu(2,F2x_degree(T)),-1),zeta,(void*)T,&F2xq_star);
}

GEN
gener_F2xq(GEN T, GEN *po)
{
  long i, j, vT = T[1], f = F2x_degree(T);
  pari_sp av0 = avma, av;
  GEN g, L2, o, q;

  if (f == 1) {
    if (po) *po = trivfact();
    return pol1_F2x(vT);
  }
  q = subis(powuu(2,f), 1);
  o = factor_pn_1(gen_2,f);
  L2 = shallowcopy( gel(o, 1) );
  for (i = j = 1; i < lg(L2); i++)
  {
    if (equaliu(gel(L2,i),2)) continue;
    gel(L2,j++) = diviiexact(q, gel(L2,i));
  }
  setlg(L2, j);
  for (av = avma;; avma = av)
  {
    g = random_F2x(f, vT);
    if (F2x_degree(g) < 1) continue;
    for (i = 1; i < j; i++)
    {
      GEN a = F2xq_pow(g, gel(L2,i), T);
      if (!degpol(a) && (((ulong)a[2])&1UL) == 1UL) break;
    }
    if (i == j) break;
  }
  if (!po) g = gerepilecopy(av0, g);
  else {
    *po = o; gerepileall(av0, 2, &g, po);
  }
  return g;
}
