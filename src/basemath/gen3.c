/* $Id$

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/********************************************************************/
/**                                                                **/
/**                      GENERIC OPERATIONS                        **/
/**                         (third part)                           **/
/**                                                                **/
/********************************************************************/
#include "pari.h"

extern GEN ishiftr_spec(GEN x, long lx, long n);
extern GEN ptolift(GEN x, GEN Y);

/********************************************************************/
/**                                                                **/
/**                 PRINCIPAL VARIABLE NUMBER                      **/
/**                                                                **/
/********************************************************************/
int
gvar(GEN x)
{
  long i, v, w;
  switch(typ(x))
  {
    case t_POL: case t_SER: return varn(x);
    case t_POLMOD: return varn(x[1]);
    case t_RFRAC:
      v = gvar((GEN)x[1]);
      w = gvar((GEN)x[2]); return min(v,w);
    case t_VEC: case t_COL: case t_MAT:
      v = BIGINT;
      for (i=1; i < lg(x); i++) { w=gvar((GEN)x[i]); if (w<v) v=w; }
      return v;
  }
  return BIGINT;
}

/* assume x POLMOD or RFRAC */
static int
_var2(GEN x)
{
  long v = gvar2((GEN)x[1]);
  long w = gvar2((GEN)x[2]); if (w<v) v=w;
  return v;
}

int
gvar2(GEN x)
{
  long i, v, w;
  switch(typ(x))
  {
    case t_POLMOD: case t_RFRAC:
      return _var2(x);

    case t_POL: case t_SER:
      v = BIGINT;
      for (i=2; i < lg(x); i++) { w=gvar((GEN)x[i]); if (w<v) v=w; }
      return v;

    case t_VEC: case t_COL: case t_MAT:
      v = BIGINT;
      for (i=1; i < lg(x); i++) { w=gvar2((GEN)x[i]); if (w<v) v=w; }
      return v;
  }
  return BIGINT;
}

int
gvar9(GEN x)
{
  return (typ(x) == t_POLMOD)? _var2(x): gvar(x);
}

GEN
gpolvar(GEN x)
{
  if (typ(x)==t_PADIC) x = (GEN)x[2];
  else
  {
    long v=gvar(x);
    if (v==BIGINT) err(typeer,"polvar");
    x = polx[v];
  }
  return gcopy(x);
}

/*******************************************************************/
/*                                                                 */
/*                    PRECISION OF SCALAR OBJECTS                  */
/*                                                                 */
/*******************************************************************/

long
precision(GEN x)
{
  long tx=typ(x),k,l;

  if (tx==t_REAL)
  {
    k=2-(expo(x)>>TWOPOTBITS_IN_LONG);
    l=lg(x); if (l>k) k=l;
    return k;
  }
  if (tx==t_COMPLEX)
  {
    k=precision((GEN)x[1]);
    l=precision((GEN)x[2]); if (l && (!k || l<k)) k=l;
    return k;
  }
  return 0;
}

long
gprecision(GEN x)
{
  long tx=typ(x),lx=lg(x),i,k,l;

  if (is_scalar_t(tx)) return precision(x);
  switch(tx)
  {
    case t_POL: case t_VEC: case t_COL: case t_MAT:
      k=VERYBIGINT;
      for (i=lontyp[tx]; i<lx; i++)
      {
        l = gprecision((GEN)x[i]);
	if (l && l<k) k = l;
      }
      return (k==VERYBIGINT)? 0: k;

    case t_RFRAC:
    {
      k=gprecision((GEN)x[1]);
      l=gprecision((GEN)x[2]); if (l && (!k || l<k)) k=l;
      return k;
    }
    case t_QFR:
      return gprecision((GEN)x[4]);
  }
  return 0;
}

GEN
ggprecision(GEN x)
{
  long a=gprecision(x);
  return stoi(a ? (long) ((a-2)*pariK): VERYBIGINT);
}

GEN
precision0(GEN x, long n)
{
  if (n) return gprec(x,n);
  return ggprecision(x);
}

/* attention: precision p-adique absolue */
long
padicprec(GEN x, GEN p)
{
  long i,s,t,lx=lg(x),tx=typ(x);

  switch(tx)
  {
    case t_INT: case t_FRAC:
      return VERYBIGINT;

    case t_INTMOD:
      return ggval((GEN)x[1],p);

    case t_PADIC:
      if (!gegal((GEN)x[2],p))
	err(talker,"not the same prime in padicprec");
      return precp(x)+valp(x);

    case t_POL:
    case t_COMPLEX: case t_QUAD: case t_POLMOD: case t_SER: case t_RFRAC:
    case t_VEC: case t_COL: case t_MAT:
      for (s=VERYBIGINT, i=lontyp[tx]; i<lx; i++)
      {
        t = padicprec((GEN)x[i],p); if (t<s) s = t;
      }
      return s;
  }
  err(typeer,"padicprec");
  return 0; /* not reached */
}

#define DEGREE0 -VERYBIGINT
/* Degree of x (scalar, t_POL, t_RFRAC) wrt variable v if v >= 0,
 * wrt to main variable if v < 0.
 */
long
poldegree(GEN x, long v)
{
  long tx = typ(x), lx,w,i,d;

  if (is_scalar_t(tx)) return gcmp0(x)? DEGREE0: 0;
  switch(tx)
  {
    case t_POL:
      if (!signe(x)) return DEGREE0;
      w = varn(x);
      if (v < 0 || v == w) return degpol(x);
      if (v < w) return 0;
      lx = lg(x); d = DEGREE0;
      for (i=2; i<lx; i++)
      {
        long e = poldegree((GEN)x[i], v);
        if (e > d) d = e;
      }
      return d;

    case t_RFRAC:
      if (gcmp0((GEN)x[1])) return DEGREE0;
      return poldegree((GEN)x[1],v) - poldegree((GEN)x[2],v);
  }
  err(typeer,"degree");
  return 0; /* not reached  */
}

long
degree(GEN x)
{
  return poldegree(x,-1);
}

/* si v<0, par rapport a la variable principale, sinon par rapport a v.
 * On suppose que x est un polynome ou une serie.
 */
GEN
pollead(GEN x, long v)
{
  long l, tx = typ(x), w;
  pari_sp av, tetpil;
  GEN xinit;

  if (is_scalar_t(tx)) return gcopy(x);
  w = varn(x);
  switch(tx)
  {
    case t_POL:
      if (v < 0 || v == w)
      {
	l=lg(x);
	return (l==2)? gzero: gcopy((GEN)x[l-1]);
      }
      if (v < w) return gcopy(x);
      av = avma; xinit = x;
      x = gsubst(gsubst(x,w,polx[MAXVARN]),v,polx[0]);
      if (gvar(x)) { avma = av; return gcopy(xinit); }
      l = lg(x); if (l == 2) { avma = av; return gzero; }
      tetpil = avma; x = gsubst((GEN)x[l-1],MAXVARN,polx[w]);
      return gerepile(av,tetpil,x);

    case t_SER:
      if (v < 0 || v == w) return (!signe(x))? gzero: gcopy((GEN)x[2]);
      if (v < w) return gcopy(x);
      av = avma; xinit = x;
      x = gsubst(gsubst(x,w,polx[MAXVARN]),v,polx[0]);
      if (gvar(x)) { avma = av; return gcopy(xinit);}
      if (!signe(x)) { avma = av; return gzero;}
      tetpil = avma; x = gsubst((GEN)x[2],MAXVARN,polx[w]);
      return gerepile(av,tetpil,x);
  }
  err(typeer,"pollead");
  return NULL; /* not reached */
}

/* returns 1 if there's a real component in the structure, 0 otherwise */
int
isinexactreal(GEN x)
{
  long tx=typ(x),lx,i;

  if (is_scalar_t(tx))
  {
    switch(tx)
    {
      case t_REAL:
        return 1;

      case t_COMPLEX: case t_QUAD:
        return (typ(x[1])==t_REAL || typ(x[2])==t_REAL);
    }
    return 0;
  }
  switch(tx)
  {
    case t_QFR: case t_QFI:
      return 0;

    case t_RFRAC:
      return isinexactreal((GEN)x[1]) || isinexactreal((GEN)x[2]);
  }
  if (is_noncalc_t(tx)) return 0;
  lx = lg(x);
  for (i=lontyp[tx]; i<lx; i++)
    if (isinexactreal((GEN)x[i])) return 1;
  return 0;
}

int
isexactzeroscalar(GEN g)
{
  switch (typ(g))
  {
    case t_INT:
      return !signe(g);
    case t_REAL: case t_PADIC: case t_SER:
      return 0;
    case t_INTMOD: case t_POLMOD:
      return isexactzeroscalar((GEN)g[2]);
    case t_FRAC: case t_RFRAC:
      return isexactzeroscalar((GEN)g[1]);
    case t_COMPLEX:
      return isexactzeroscalar((GEN)g[1]) && isexactzeroscalar((GEN)g[2]);
    case t_QUAD:
      return isexactzeroscalar((GEN)g[2]) && isexactzeroscalar((GEN)g[3]);
    case t_POL: return lg(g) == 2;
  }
  return 0;
}

int
isexactzero(GEN g)
{
  long i;
  switch (typ(g))
  {
    case t_INT:
      return !signe(g);
    case t_REAL: case t_PADIC: case t_SER:
      return 0;
    case t_INTMOD: case t_POLMOD:
      return isexactzero((GEN)g[2]);
    case t_FRAC: case t_RFRAC:
      return isexactzero((GEN)g[1]);
    case t_COMPLEX:
      return isexactzero((GEN)g[1]) && isexactzero((GEN)g[2]);
    case t_QUAD:
      return isexactzero((GEN)g[2]) && isexactzero((GEN)g[3]);

    case t_POL: return lg(g) == 2;
    case t_VEC: case t_COL: case t_MAT:
      for (i=lg(g)-1; i; i--)
	if (!isexactzero((GEN)g[i])) return 0;
      return 1;
  }
  return 0;
}

int
iscomplex(GEN x)
{
  switch(typ(x))
  {
    case t_INT: case t_REAL: case t_FRAC:
      return 0;
    case t_COMPLEX:
      return !gcmp0((GEN)x[2]);
    case t_QUAD:
      return signe(mael(x,1,2)) > 0;
  }
  err(typeer,"iscomplex");
  return 0; /* not reached */
}

int
ismonome(GEN x)
{
  long i;
  if (typ(x)!=t_POL || !signe(x)) return 0;
  for (i=lg(x)-2; i>1; i--)
    if (!isexactzero((GEN)x[i])) return 0;
  return 1;
}

/*******************************************************************/
/*                                                                 */
/*                    GENERIC REMAINDER                            */
/*                                                                 */
/*******************************************************************/
/* euclidean quotient for scalars of admissible types */
static GEN
_quot(GEN x, GEN y)
{
  GEN q = gdiv(x,y), f = gfloor(q);
  if (gsigne(y) < 0 && !gegal(f,q)) f = gadd(f,gun);
  return f;
}
static GEN
quot(GEN x, GEN y)
{
  pari_sp av = avma;
  return gerepileupto(av, _quot(x, y));
}

GEN
gmod(GEN x, GEN y)
{
  pari_sp av, tetpil;
  long i,lx,ty, tx = typ(x);
  GEN z,p1;

  if (is_matvec_t(tx))
  {
    lx=lg(x); z=cgetg(lx,tx);
    for (i=1; i<lx; i++) z[i] = lmod((GEN)x[i],y);
    return z;
  }
  ty = typ(y);
  switch(ty)
  {
    case t_INT:
      switch(tx)
      {
	case t_INT:
	  return modii(x,y);

	case t_INTMOD: z=cgetg(3,tx);
          z[1]=(long)gcdii((GEN)x[1],y);
	  z[2]=lmodii((GEN)x[2],(GEN)z[1]); return z;

	case t_FRAC:
	  av=avma;
	  p1=mulii((GEN)x[1],mpinvmod((GEN)x[2],y));
	  tetpil=avma; return gerepile(av,tetpil,modii(p1,y));

	case t_QUAD: z=cgetg(4,tx);
          copyifstack(x[1],z[1]);
	  z[2]=lmod((GEN)x[2],y);
          z[3]=lmod((GEN)x[3],y); return z;

	case t_PADIC: return ptolift(x, y);
	case t_POLMOD: case t_POL:
	  return gzero;
      /* case t_REAL could be defined as below, but conlicting semantic
       * with lift(x * Mod(1,y)). */

	default: err(operf,"%",x,y);
      }

    case t_REAL: case t_FRAC:
      switch(tx)
      {
	case t_INT: case t_REAL: case t_FRAC:
	  av = avma;
          return gerepileupto(av, gsub(x, gmul(_quot(x,y),y)));

	case t_POLMOD: case t_POL:
	  return gzero;

	default: err(operf,"%",x,y);
      }

    case t_POL:
      if (is_scalar_t(tx))
      {
        if (tx!=t_POLMOD || varncmp(varn(x[1]), varn(y)) > 0)
          return degpol(y)? gcopy(x): gzero;
	if (varn(x[1])!=varn(y)) return gzero;
        z=cgetg(3,t_POLMOD);
        z[1]=lgcd((GEN)x[1],y);
        z[2]=lres((GEN)x[2],(GEN)z[1]); return z;
      }
      switch(tx)
      {
	case t_POL:
	  return grem(x,y);

	case t_RFRAC:
	  av=avma;
	  p1=gmul((GEN)x[1],ginvmod((GEN)x[2],y)); tetpil=avma;
          return gerepile(av,tetpil,grem(p1,y));

        case t_SER:
          if (ismonome(y) && varn(x) == varn(y))
          {
            long d = degpol(y);
            if (lg(x)-2 + valp(x) < d) err(operi,"%",x,y);
            av = avma; 
            return gerepileupto(av, gmod(gtrunc(x), y));
          }
	default: err(operf,"%",x,y);
      }
  }
  err(operf,"%",x,y);
  return NULL; /* not reached */
}

GEN
gmodulsg(long x, GEN y)
{
  GEN z;

  switch(typ(y))
  {
    case t_INT: z=cgetg(3,t_INTMOD);
      z[1]=labsi(y); z[2]=lmodsi(x,y); return z;

    case t_POL: z=cgetg(3,t_POLMOD);
      z[1]=lcopy(y); z[2]=lstoi(x); return z;
  }
  err(operf,"%",stoi(x),y); return NULL; /* not reached */
}

GEN
gmodulss(long x, long y)
{
  GEN z=cgetg(3,t_INTMOD);

  y=labs(y); z[1]=lstoi(y); z[2]=lstoi(x % y); return z;
}

static GEN 
specialmod(GEN x, GEN y)
{
  GEN z = gmod(x,y);
  if (varncmp(gvar(z), varn(y)) < 0) z = gzero;
  return z;
}

GEN
gmodulo(GEN x,GEN y)
{
  long tx=typ(x),l,i;
  GEN z;

  if (is_matvec_t(tx))
  {
    l=lg(x); z=cgetg(l,tx);
    for (i=1; i<l; i++) z[i] = lmodulo((GEN)x[i],y);
    return z;
  }
  switch(typ(y))
  {
    case t_INT:
      if (tx!=t_INT && tx != t_FRAC && tx!=t_PADIC) break;
      z=cgetg(3,t_INTMOD);
      if (!signe(y)) err(talker,"zero modulus in gmodulo");
      y = gclone(y); setsigne(y,1);
      z[1]=(long)y;
      z[2]=lmod(x,y); return z;

    case t_POL: z=cgetg(3,t_POLMOD);
      z[1] = lclone(y);
      if (is_scalar_t(tx)) { z[2]=lcopy(x); return z; }
      if (tx!=t_POL && tx!=t_RFRAC && tx!=t_SER) break;
      z[2]=(long)specialmod(x,y); return z;
  }
  err(operf,"%",x,y); return NULL; /* not reached */
}

GEN
gmodulcp(GEN x,GEN y)
{
  long tx=typ(x),l,i;
  GEN z;

  if (is_matvec_t(tx))
  {
    l=lg(x); z=cgetg(l,tx);
    for (i=1; i<l; i++) z[i] = lmodulcp((GEN)x[i],y);
    return z;
  }
  switch(typ(y))
  {
    case t_INT:
      if (tx!=t_INT && tx != t_FRAC && tx!=t_PADIC) break;
      z=cgetg(3,t_INTMOD);
      z[1] = labsi(y);
      z[2] = lmod(x,y); return z;

    case t_POL: z=cgetg(3,t_POLMOD);
      z[1] = lcopy(y);
      if (is_scalar_t(tx))
      {
        z[2] = (lg(y) > 3)? lcopy(x): lmod(x,y);
        return z;
      }
      if (tx!=t_POL && tx != t_RFRAC && tx!=t_SER) break;
      z[2]=(long)specialmod(x,y); return z;
  }
  err(operf,"%",x,y); return NULL; /* not reached */
}

GEN
Mod0(GEN x,GEN y,long flag)
{
  switch(flag)
  {
    case 0: return gmodulcp(x,y);
    case 1: return gmodulo(x,y);
    default: err(flagerr,"Mod");
  }
  return NULL; /* not reached */
}

/*******************************************************************/
/*                                                                 */
/*                 GENERIC EUCLIDEAN DIVISION                      */
/*                                                                 */
/*******************************************************************/

GEN
gdivent(GEN x, GEN y)
{
  long tx = typ(x);

  if (is_matvec_t(tx))
  {
    long i, lx = lg(x);
    GEN z = cgetg(lx,tx);
    for (i=1; i<lx; i++) z[i] = (long)gdivent((GEN)x[i],y);
    return z;
  }
  switch(typ(y))
  {
    case t_INT:
      switch(tx)
      { /* equal to, but more efficient than, quot(x,y) */
        case t_INT: return truedvmdii(x,y,NULL);
        case t_REAL: case t_FRAC: return quot(x,y);
        case t_POL: return gdiv(x,y);
      }
      break;
    case t_REAL: case t_FRAC: return quot(x,y);
    case t_POL:
      if (is_scalar_t(tx))
      {
        if (tx == t_POLMOD) break;
        return degpol(y)? gzero: gdiv(x,y);
      }
      if (tx == t_POL) return gdeuc(x,y);
  }
  err(operf,"\\",x,y);
  return NULL; /* not reached */
}

/* with remainder */
static GEN
quotrem(GEN x, GEN y, GEN *r)
{
  pari_sp av;
  GEN q = quot(x,y);
  av = avma;
  *r = gerepileupto(av, gsub(x, gmul(q,y)));
  return q;
}

GEN
gdiventres(GEN x, GEN y)
{
  long tx = typ(x);
  GEN z,q,r;

  if (is_matvec_t(tx))
  {
    long i, lx = lg(x);
    z = cgetg(lx,tx);
    for (i=1; i<lx; i++) z[i] = (long)gdiventres((GEN)x[i],y);
    return z;
  }
  z = cgetg(3,t_COL);
  switch(typ(y))
  {
    case t_INT:
      switch(tx)
      { /* equal to, but more efficient than next case */
        case t_INT:
          z[1] = (long)truedvmdii(x,y,(GEN*)(z+2));
          return z;
        case t_REAL: case t_FRAC:
          q = quotrem(x,y,&r);
          z[1] = (long)q;
          z[2] = (long)r; return z;
        case t_POL:
          z[1] = ldiv(x,y);
          z[2] = zero; return z;
      }
      break;
    case t_REAL: case t_FRAC:
          q = quotrem(x,y,&r);
          z[1] = (long)q;
          z[2] = (long)r; return z;
    case t_POL:
      if (is_scalar_t(tx))
      {
        if (tx == t_POLMOD) break;
        if (degpol(y))
        {
          q = gzero;
          r = gcopy(x);
        }
        else
        {
          q = gdiv(x,y);
          r = gzero;
        }
        z[1] = (long)q;
        z[2] = (long)r; return z;
      }
      if (tx == t_POL)
      {
        z[1] = (long)poldivrem(x,y,(GEN *)(z+2));
        return z;
      }
  }
  err(operf,"\\",x,y);
  return NULL; /* not reached */
}

extern GEN swap_vars(GEN b0, long v);

GEN
divrem(GEN x, GEN y, long v)
{
  pari_sp av = avma;
  long vx,vy;
  GEN z,q,r;
  if (v < 0 || typ(y) != t_POL || typ(x) != t_POL) return gdiventres(x,y);
  vx = varn(x); if (vx != v) x = swap_vars(x,v);
  vy = varn(y); if (vy != v) y = swap_vars(y,v);
  q = poldivrem(x,y, &r);
  if (v && (vx != v || vy != v))
  {
    q = poleval(q, polx[v]);
    r = poleval(r, polx[v]);
  }
  z = cgetg(3,t_COL);
  z[1] = (long)q;
  z[2] = (long)r; return gerepilecopy(av, z);
}

static int
is_scal(long t) { return t==t_INT || t==t_FRAC; }

GEN
diviiround(GEN x, GEN y)
{
  pari_sp av1, av = avma;
  GEN q,r;
  int fl;

  q = dvmdii(x,y,&r); /* q = x/y rounded towards 0, sgn(r)=sgn(x) */
  if (r==gzero) return q;
  av1 = avma;
  fl = absi_cmp(shifti(r,1),y);
  avma = av1; cgiv(r);
  if (fl >= 0) /* If 2*|r| >= |y| */
  {
    long sz = signe(x)*signe(y);
    if (fl || sz > 0) q = gerepileuptoint(av, addis(q,sz));
  }
  return q;
}

/* If x and y are not both scalars, same as gdivent.
 * Otherwise, compute the quotient x/y, rounded to the nearest integer
 * (towards +oo in case of tie). */
GEN
gdivround(GEN x, GEN y)
{
  pari_sp av1, av;
  long tx=typ(x),ty=typ(y);
  GEN q,r;
  int fl;

  if (tx==t_INT && ty==t_INT) return diviiround(x,y);
  av = avma;
  if (is_scal(tx) && is_scal(ty))
  { /* same as diviiround but less efficient */
    q = quotrem(x,y,&r);
    av1 = avma;
    fl = gcmp(gmul2n(gabs(r,0),1), gabs(y,0));
    avma = av1; cgiv(r);
    if (fl >= 0) /* If 2*|r| >= |y| */
    {
      long sz = gsigne(y);
      if (fl || sz > 0) q = gerepileupto(av, gaddgs(q, sz));
    }
    return q;
  }
  if (is_matvec_t(tx))
  {
    long i,lx = lg(x);
    GEN z = cgetg(lx,tx);
    for (i=1; i<lx; i++) z[i] = (long)gdivround((GEN)x[i],y);
    return z;
  }
  return gdivent(x,y);
}

GEN
gdivmod(GEN x, GEN y, GEN *pr)
{
  long ty,tx=typ(x);

  if (tx==t_INT)
  {
    ty=typ(y);
    if (ty==t_INT) return dvmdii(x,y,pr);
    if (ty==t_POL) { *pr=gcopy(x); return gzero; }
    err(typeer,"gdivmod");
  }
  if (tx!=t_POL) err(typeer,"gdivmod");
  return poldivrem(x,y,pr);
}

/*******************************************************************/
/*                                                                 */
/*                       SHIFT D'UN GEN                            */
/*                                                                 */
/*******************************************************************/

/* Shift tronque si n<0 (multiplication tronquee par 2^n)  */

GEN
gshift(GEN x, long n)
{
  long i, lx, tx = typ(x);
  GEN y;

  switch(tx)
  {
    case t_INT:
      return shifti(x,n);
    case t_REAL:
      return shiftr(x,n);

    case t_VEC: case t_COL: case t_MAT:
      lx = lg(x); y = cgetg(lx,tx);
      for (i=1; i<lx; i++) y[i] = lshift((GEN)x[i],n);
      return y;
  }
  return gmul2n(x,n);
}

GEN
real2n(long n, long prec) { GEN z = realun(prec); setexpo(z, n); return z; }

/*******************************************************************/
/*                                                                 */
/*                      INVERSE D'UN GEN                           */
/*                                                                 */
/*******************************************************************/
extern GEN fix_rfrac_if_pol(GEN x, GEN y);
extern GEN quad_polmod_norm(GEN x, GEN y);
extern GEN quad_polmod_conj(GEN x, GEN y);

GEN
mpinv(GEN b)
{
  long i, l1, l = lg(b), e = expo(b), s = signe(b);
  GEN x = cgetr(l), a = mpcopy(b);
  double t;

  a[1] = evalexpo(0) | evalsigne(1);
  for (i = 3; i < l; i++) x[i] = 0;
  t = (((double)HIGHBIT) * HIGHBIT) / (double)(ulong)a[2];
  if (((ulong)t) & HIGHBIT)
    x[1] = evalexpo(0) | evalsigne(1);
  else {
    t *= 2;
    x[1] = evalexpo(-1) | evalsigne(1);
  }
  x[2] = (long)(ulong)t;
  l1 = 1; l -= 2;
  while (l1 < l)
  {
    l1 <<= 1; if (l1 > l) l1 = l;
    setlg(a, l1 + 2);
    setlg(x, l1 + 2);
    /* TODO: mulrr(a,x) should be a half product (the higher half is known).
     * mulrr(x, ) already is */
    affrr(addrr(x, mulrr(x, subsr(1, mulrr(a,x)))), x);
    avma = (pari_sp)a;
  }
  x[1] = evalexpo(expo(x)-e) | evalsigne(s);
  avma = (pari_sp)x; return x;
}

GEN
ginv(GEN x)
{
  long s;
  pari_sp av, tetpil;
  GEN X, z, y, p1, p2;

  switch(typ(x))
  {
    case t_INT:
      if (is_pm1(x)) return icopy(x);
      s = signe(x); if (!s) err(gdiver);
      z = cgetg(3,t_FRAC);
      z[1] = s<0? lnegi(gun): un;
      z[2] = labsi(x); return z;

    case t_REAL:
      return divsr(1,x);

    case t_INTMOD: z=cgetg(3,t_INTMOD);
      icopyifstack(x[1],z[1]);
      z[2]=lmpinvmod((GEN)x[2],(GEN)x[1]); return z;

    case t_FRAC:
      s = signe(x[1]); if (!s) err(gdiver);
      if (is_pm1(x[1]))
        return s>0? icopy((GEN)x[2]): negi((GEN)x[2]);
      z = cgetg(3,t_FRAC);
      z[1] = licopy((GEN)x[2]);
      z[2] = licopy((GEN)x[1]);
      if (s < 0)
      {
	setsigne(z[1],-signe(z[1]));
	setsigne(z[2],1);
      }
      return z;

    case t_COMPLEX: case t_QUAD:
      av=avma; p1=gnorm(x); p2=gconj(x); tetpil=avma;
      return gerepile(av,tetpil,gdiv(p2,p1));

    case t_PADIC: z = cgetg(5,t_PADIC);
      if (!signe(x[4])) err(gdiver);
      z[1] = evalprecp(precp(x)) | evalvalp(-valp(x));
      icopyifstack(x[2], z[2]);
      z[3] = licopy((GEN)x[3]);
      z[4] = lmpinvmod((GEN)x[4],(GEN)z[3]); return z;

    case t_POLMOD: z = cgetg(3,t_POLMOD);
      X = (GEN)x[1];
      copyifstack(X, z[1]);
      if (degpol(X) == 2) { /* optimized for speed */
        av = avma;
        z[2] = lpileupto(av, gdiv(quad_polmod_conj((GEN)x[2], X),
                                  quad_polmod_norm((GEN)x[2], X)) );
      }
      else z[2] = linvmod((GEN)x[2], X);
      return z;

    case t_POL: case t_SER:
      return gdiv(gun,x);

    case t_RFRAC:
      if (gcmp0((GEN) x[1])) err(gdiver);
      p1 = fix_rfrac_if_pol((GEN)x[2],(GEN)x[1]);
      if (p1) return p1;
      z=cgetg(3,t_RFRAC);
      z[1]=lcopy((GEN)x[2]);
      z[2]=lcopy((GEN)x[1]); return z;

    case t_QFR:
    {
      long k,l;
      l=signe(x[2]); setsigne(x[2],-l);
      k=signe(x[4]); setsigne(x[4],-k); z=redreal(x);
      setsigne(x[2],l); setsigne(x[4],k); return z;
    }
    case t_QFI:
      y=gcopy(x);
      if (!egalii((GEN)x[1],(GEN)x[2]) && !egalii((GEN)x[1],(GEN)x[3]))
	setsigne(y[2],-signe(y[2]));
      return y;
    case t_MAT:
      return (lg(x)==1)? cgetg(1,t_MAT): invmat(x);
    case t_VECSMALL:
    {
      long i,lx = lg(x);
      y = cgetg(lx,t_VECSMALL);
      for (i=1; i<lx; i++)
      {
        long xi=x[i];
	  if (xi<1 || xi>=lx) err(talker,"incorrect permtuation to inverse");
        y[xi] = i;
      }
      return y;
    }
  }
  err(typeer,"inverse");
  return NULL; /* not reached */
}

/*******************************************************************/
/*                                                                 */
/*           SUBSTITUTION DANS UN POLYNOME OU UNE SERIE            */
/*                                                                 */
/*******************************************************************/

/* Convert t_SER --> t_POL / t_RFRAC */
static GEN
gconvsp(GEN x, long e)
{
  long v = varn(x), i;
  GEN y;

  if (gcmp0(x)) return zeropol(v);
  y = dummycopy(x);
  i = lg(x)-1; while (i > 1 && gcmp0((GEN)y[i])) i--;
  y[0] = evaltyp(t_POL) | evallg(i+1);
  y[1] &= ~VALPBITS;
  return e? gmul(y,  gpowgs(polx[v], e)): y;
}

/*
   subst_poly(pol, from, to) =
   { local(t='subst_poly_t, M);

     \\ if fraction
     M = numerator(from) - t * denominator(from);
     \\ else
     M = from - t;
     subst(pol % M, t, to)
   }
 */
GEN
gsubst_expr(GEN pol, GEN from, GEN to)
{
  pari_sp av = avma;
  long v = fetch_var();		/* XXX Need fetch_var_low_priority() */
  GEN tmp;

  from = simplify_i(from);
  switch (typ(from)) {
  case t_RFRAC: /* M= numerator(from) - t * denominator(from) */
    tmp = gsub((GEN)from[1], gmul(polx[v], (GEN)from[2]));
    break;
  default:
    tmp = gsub(from, polx[v]);	/* M = from - t */
  }

  if (v <= gvar(from)) err(talker, "subst: unexpected variable precedence");
  tmp = gmul(pol, gmodulcp(gun, tmp));
  if (typ(tmp) == t_POLMOD)
    tmp = (GEN)tmp[2];			/* optimize lift */
  else					/* Vector? */
    tmp = lift0(tmp, gvar(from));
  tmp = gsubst(tmp, v, to);
  (void)delete_var();
  return gerepilecopy(av, tmp);
}

GEN
gsubstpol(GEN x, GEN T, GEN y)
{
  pari_sp av;
  long d, v;
  GEN deflated;

  if (typ(T) != t_POL || !ismonome(T) || !gcmp1(leading_term(T)))
    return gsubst_expr(x,T,y);
  d = degpol(T); v = varn(T);
  if (d == 1) return gsubst(x, v, y);
  av = avma;
  CATCH(cant_deflate) {
    avma = av;
    return gsubst_expr(x,T,y);      
  } TRY {
    deflated = gdeflate(x, v, d);
  } ENDCATCH
  return gerepilecopy(av, gsubst(deflated, v, y));
}

GEN
gsubst(GEN x, long v, GEN y)
{
  long tx = typ(x), ty = typ(y), lx = lg(x), ly = lg(y);
  long l, vx, vy, e, ex, ey, i, j, k, jb;
  pari_sp tetpil, av, limite;
  GEN t,p1,p2,z;

  if (ty==t_MAT)
  {
    if (ly==1) return cgetg(1,t_MAT);
    if (ly != lg(y[1]))
      err(talker,"forbidden substitution by a non square matrix");
  } else if (is_graphicvec_t(ty))
    err(talker,"forbidden substitution by a vector");

  if (is_scalar_t(tx))
  {
    if (tx!=t_POLMOD || v <= varn(x[1]))
    {
      if (ty==t_MAT) return gscalmat(x,ly-1);
      return gcopy(x);
    }
    av=avma;
    p1=gsubst((GEN)x[1],v,y); vx=varn(p1);
    p2=gsubst((GEN)x[2],v,y); vy=gvar(p2);
    if (typ(p1)!=t_POL)
      err(talker,"forbidden substitution in a scalar type");
    if (varncmp(vy, vx) >= 0)
    {
      tetpil=avma;
      return gerepile(av,tetpil,gmodulcp(p2,p1));
    }
    lx=lg(p2); tetpil=avma; z=cgetg(lx,t_POL); z[1]=p2[1];
    for (i=2; i<lx; i++) z[i]=lmodulcp((GEN)p2[i],p1);
    return gerepile(av,tetpil, normalizepol_i(z,lx));
  }

  switch(tx)
  {
    case t_POL:
      if (lx==2)
        return (ty==t_MAT)? gscalmat(gzero,ly-1): gzero;

      vx = varn(x);
      if (varncmp(vx, v) < 0)
      {
        if (varncmp(gvar(y), vx) > 0)
        { /* easy special case */
          z = cgetg(lx, t_POL); z[1] = x[1];
          for (i=2; i<lx; i++) z[i] = lsubst((GEN)x[i],v,y);
          return normalizepol_i(z,lx);
        }
        /* general case */
	av=avma; p1=polx[vx]; z= gsubst((GEN)x[lx-1],v,y);
	for (i=lx-1; i>2; i--) z=gadd(gmul(z,p1),gsubst((GEN)x[i-1],v,y));
	return gerepileupto(av,z);
      }
      /* v <= vx */
      if (ty!=t_MAT)
        return varncmp(vx,v) > 0 ? gcopy(x): poleval(x,y);

      if (varncmp(vx, v) > 0) return gscalmat(x,ly-1);
      if (lx==3) return gscalmat((GEN)x[2],ly-1);
      av=avma; z=(GEN)x[lx-1];
      for (i=lx-1; i>2; i--) z=gaddmat((GEN)x[i-1],gmul(z,y));
      return gerepileupto(av,z);

    case t_SER:
      vx = varn(x);
      if (varncmp(vx, v) > 0) return (ty==t_MAT)? gscalmat(x,ly-1): gcopy(x);
      ex = valp(x);
      if (varncmp(vx, v) < 0)
      {
        if (!signe(x)) return gcopy(x);
        /* FIXME: improve this */
        av = avma; p1 = gconvsp(x, 0);
        z = tayl(gsubst(p1,v,y), vx, lx-2);
        if (ex) z = gmul(z, gpowgs(polx[vx],ex));
        return gerepileupto(av, z);
      }
      switch(ty) /* here vx == v */
      {
        case t_SER:
	  ey = valp(y);
          vy = varn(y);
	  if (ey < 1) return zeroser(vy, ey*(ex+lx-2));
          if (!signe(x)) return zeroser(vy, ey*ex);
	  l = (lx-2)*ey+2;
	  if (ex) { if (l>ly) l = ly; }
	  else
	  {
	    if (gcmp0(y)) l = ey+2;
	    else if (l>ly) l = ey+ly;
	  }
	  if (vy != vx)
	  {
	    av = avma; z = zeroser(vy,0);
	    for (i=lx-1; i>=2; i--) z = gadd((GEN)x[i], gmul(y,z));
	    if (ex) z = gmul(z, gpowgs(y,ex));
	    return gerepileupto(av,z);
	  }

	  av = avma; limite=stack_lim(av,1);
          t = cgetg(ly,t_SER);
          z = scalarser((GEN)x[2],varn(y),l-2);
	  for (i=2; i<ly; i++) t[i] = y[i];
	  for (i=3,jb=ey; jb<=l-2; i++,jb+=ey)
	  {
	    for (j=jb+2; j<l; j++)
	      z[j] = ladd((GEN)z[j], gmul((GEN)x[i],(GEN)t[j-jb]));
	    for (j=l-1-jb-ey; j>1; j--)
	    {
	      p1 = gzero;
	      for (k=2; k<j; k++)
		p1 = gadd(p1, gmul((GEN)t[j-k+2],(GEN)y[k]));
	      t[j]=ladd(p1, gmul((GEN)t[2],(GEN)y[j]));
	    }
            if (low_stack(limite, stack_lim(av,1)))
	    {
	      if(DEBUGMEM>1) err(warnmem,"gsubst");
	      gerepileall(av,2, &z,&t);
	    }
	  }
	  if (!ex) return gerepilecopy(av,z);

          if (l < ly) { setlg(y,l); p1 = gpowgs(y,ex); setlg(y,ly); }
          else p1 = gpowgs(y,ex);
          tetpil=avma; return gerepile(av,tetpil,gmul(z,p1));

        case t_POL: case t_RFRAC:
          if (isexactzero(y)) return scalarser((GEN)x[2],v,lx-2);
          vy=gvar(y); e=gval(y,vy);
          if (e<=0)
            err(talker,"non positive valuation in a series substitution");
	  av = avma; p1 = gconvsp(x, ex); 
          z = tayl(gsubst(p1,v,y), vy, e*(lx-2+ex));
	  return gerepileupto(av, z);

        default:
          err(talker,"non polynomial or series type substituted in a series");
      }
      break;

    case t_RFRAC: av=avma;
      p1=gsubst((GEN)x[1],v,y);
      p2=gsubst((GEN)x[2],v,y); tetpil=avma;
      return gerepile(av,tetpil,gdiv(p1,p2));

    case t_VEC: case t_COL: case t_MAT: z=cgetg(lx,tx);
      for (i=1; i<lx; i++) z[i]=lsubst((GEN)x[i],v,y);
      return z;
  }
  return gcopy(x);
}

/*******************************************************************/
/*                                                                 */
/*                SERIE RECIPROQUE D'UNE SERIE                     */
/*                                                                 */
/*******************************************************************/

GEN
recip(GEN x)
{
  long v=varn(x), lx = lg(x);
  pari_sp tetpil, av=avma;
  GEN p1,p2,a,y,u;

  if (typ(x)!=t_SER) err(talker,"not a series in serreverse");
  if (valp(x)!=1 || lx < 3)
    err(talker,"valuation not equal to 1 in serreverse");

  a=(GEN)x[2];
  if (gcmp1(a))
  {
    long i, j, k, mi;
    pari_sp lim=stack_lim(av, 2);

    mi = lx-1; while (mi>=3 && gcmp0((GEN)x[mi])) mi--;
    u = cgetg(lx,t_SER);
    y = cgetg(lx,t_SER);
    u[1] = y[1] = evalsigne(1) | evalvalp(1) | evalvarn(v);
    u[2] = y[2] = un;
    if (lx > 3)
    {
      u[3] = lmulsg(-2,(GEN)x[3]);
      y[3] = lneg((GEN)x[3]);
    }
    for (i=3; i<lx-1; )
    {
      pari_sp av2;
      for (j=3; j<i+1; j++)
      {
        av2 = avma; p1 = (GEN)x[j];
        for (k = max(3,j+2-mi); k < j; k++)
          p1 = gadd(p1, gmul((GEN)u[k],(GEN)x[j-k+2]));
        p1 = gneg(p1);
        u[j] = lpileupto(av2, gadd((GEN)u[j], p1));
      }
      av2 = avma;
      p1 = gmulsg(i,(GEN)x[i+1]);
      for (k = 2; k < min(i,mi); k++)
      {
        p2 = gmul((GEN)x[k+1],(GEN)u[i-k+2]);
        p1 = gadd(p1, gmulsg(k,p2));
      }
      i++;
      u[i] = lpileupto(av2, gneg(p1));
      y[i] = ldivgs((GEN)u[i], i-1);
      if (low_stack(lim, stack_lim(av,2)))
      {
	if(DEBUGMEM>1) err(warnmem,"recip");
	for(k=i+1; k<lx; k++) u[k]=y[k]=zero; /* dummy */
	gerepileall(av,2, &u,&y);
      }
    }
    return gerepilecopy(av,y);
  }
  y = gdiv(x,a); y[2] = un; y = recip(y);
  a = gdiv(polx[v],a); tetpil = avma;
  return gerepile(av,tetpil, gsubst(y,v,a));
}

/*******************************************************************/
/*                                                                 */
/*                    DERIVATION ET INTEGRATION                    */
/*                                                                 */
/*******************************************************************/
GEN
derivpol(GEN x)
{
  long i,lx = lg(x)-1;
  GEN y;

  if (lx<3) return zeropol(varn(x));
  y = cgetg(lx,t_POL);
  for (i=2; i<lx ; i++) y[i] = lmulsg(i-1,(GEN)x[i+1]);
  y[1] = x[1]; return normalizepol_i(y,i);
}

GEN
derivser(GEN x)
{
  long i,j,vx = varn(x), e = valp(x), lx = lg(x);
  GEN y;
  if (gcmp0(x)) return zeroser(vx,e? e-1: 0);
  if (e)
  {
    y=cgetg(lx,t_SER); y[1] = evalsigne(1) | evalvalp(e-1) | evalvarn(vx);
    for (i=2; i<lx; i++) y[i]=lmulsg(i+e-2,(GEN)x[i]);
    return y;
  }
  i=3; while (i<lx && gcmp0((GEN)x[i])) i++;
  if (i==lx) return zeroser(vx,lx-3);
  lx--; if (lx<3) lx=3;
  lx = lx-i+3; y=cgetg(lx,t_SER);
  y[1]=evalsigne(1) | evalvalp(i-3) | evalvarn(vx);
  for (j=2; j<lx; j++) y[j]=lmulsg(j+i-4,(GEN)x[i+j-2]);
  return y;
}

GEN
deriv(GEN x, long v)
{
  long lx, vx, tx, e, i, j;
  pari_sp av, tetpil;
  GEN y,p1,p2;

  tx=typ(x); if (is_const_t(tx)) return gzero;
  if (v < 0) v = gvar(x);
  switch(tx)
  {
    case t_POLMOD:
      if (v<=varn(x[1])) return gzero;
      y=cgetg(3,t_POLMOD); copyifstack(x[1],y[1]);
      y[2]=lderiv((GEN)x[2],v); return y;

    case t_POL:
      vx=varn(x); if (varncmp(vx, v) > 0) return gzero;
      if (varncmp(vx, v) < 0)
      {
        lx = lg(x); y = cgetg(lx,t_POL);
        for (i=2; i<lx; i++) y[i] = lderiv((GEN)x[i],v);
        y[1] = evalvarn(vx);
        return normalizepol_i(y,i);
      }
      return derivpol(x);

    case t_SER:
      vx=varn(x); if (varncmp(vx, v) > 0) return gzero;
      if (varncmp(vx, v) < 0)
      {
        if (!signe(x)) return gcopy(x);
        lx=lg(x); e=valp(x);
	av=avma;
	for (i=2; i<lx; i++)
        {
          if (!gcmp0(deriv((GEN)x[i],v))) break;
          avma=av;
        }
	if (i==lx) return zeroser(vx, e+lx-2);
	y = cgetg(lx-i+2,t_SER);
        y[1] = evalsigne(1) | evalvalp(e+i-2) | evalvarn(vx);
	for (j=2; i<lx; j++,i++) y[j] = lderiv((GEN)x[i],v);
        return y;
      }
      return derivser(x);

    case t_RFRAC: av=avma; y=cgetg(3,t_RFRAC);
      y[2]=lsqr((GEN)x[2]); av=avma;
      p1=gmul((GEN)x[2],deriv((GEN)x[1],v));
      p2=gmul(gneg_i((GEN)x[1]),deriv((GEN)x[2],v));
      tetpil=avma; y[1]=lpile(av,tetpil, gadd(p1,p2)); return y;

    case t_VEC: case t_COL: case t_MAT: lx=lg(x); y=cgetg(lx,tx);
      for (i=1; i<lx; i++) y[i]=lderiv((GEN)x[i],v);
      return y;
  }
  err(typeer,"deriv");
  return NULL; /* not reached */
}

/*******************************************************************/
/*                                                                 */
/*                    INTEGRATION FORMELLE                         */
/*                                                                 */
/*******************************************************************/

static GEN
triv_integ(GEN x, long v, long tx, long lx)
{
  GEN y=cgetg(lx,tx);
  long i;

  y[1]=x[1];
  for (i=2; i<lx; i++) y[i]=linteg((GEN)x[i],v);
  return y;
}

GEN
integ(GEN x, long v)
{
  long lx, tx, e, i, vx, n;
  pari_sp av=avma, tetpil;
  GEN y,p1;

  tx = typ(x);
  if (v < 0) v = gvar(x);
  if (is_scalar_t(tx))
  {
    if (tx == t_POLMOD && v>varn(x[1]))
    {
      y=cgetg(3,t_POLMOD); copyifstack(x[1],y[1]);
      y[2]=linteg((GEN)x[2],v); return y;
    }
    if (gcmp0(x)) return gzero;

    y = cgetg(4,t_POL);
    y[1] = evalsigne(1) | evalvarn(v);
    y[2] = zero;
    y[3] = lcopy(x); return y;
  }

  switch(tx)
  {
    case t_POL:
      vx=varn(x); lx=lg(x);
      if (lx==2) return zeropol(min(v,vx));
      if (varncmp(vx, v) > 0)
      {
        y=cgetg(4,t_POL);
	y[1] = x[1];
        y[2] = zero;
        y[3] = lcopy(x); return y;
      }
      if (varncmp(vx, v) < 0) return triv_integ(x,v,tx,lx);
      y = cgetg(lx+1,tx); y[1] = x[1]; y[2] = zero;
      for (i=3; i<=lx; i++) y[i] = ldivgs((GEN)x[i-1],i-2);
      return y;

    case t_SER:
      lx=lg(x); e=valp(x); vx=varn(x);
      if (!signe(x))
      {
        if (vx == v) e++; else if (varncmp(vx, v) < 0) v = vx;
        return zeroser(v,e);
      }
      if (varncmp(vx, v) > 0)
      {
        y = cgetg(4,t_POL);
        y[1] = evalvarn(v) | evalsigne(1);
        y[2] = zero;
        y[3] = lcopy(x); return y;
      }
      if (varncmp(vx, v) < 0) return triv_integ(x,v,tx,lx);
      y = cgetg(lx,tx);
      for (i=2; i<lx; i++)
      {
	long j = i+e-1;
        if (!j)
	{
	  if (gcmp0((GEN)x[i])) { y[i] = zero; continue; }
          err(talker, "a log appears in intformal");
	}
	else y[i] = ldivgs((GEN)x[i],j);
      }
      y[1] = x[1]+1; return y;

    case t_RFRAC:
      vx = gvar(x);
      if (varncmp(vx, v) > 0)
      {
	y=cgetg(4,t_POL);
	y[1] = signe(x[1])? evalvarn(v) | evalsigne(1)
	                  : evalvarn(v);
        y[2]=zero; y[3]=lcopy(x); return y;
      }
      if (varncmp(vx, v) < 0)
      {
	p1=cgetg(v+2,t_VEC);
	for (i=0; i<vx; i++)   p1[i+1] = lpolx[i];
	for (i=vx+1; i<v; i++) p1[i+1] = lpolx[i];
        p1[v+1] = lpolx[vx]; p1[vx+1] = lpolx[v];
	y=integ(changevar(x,p1),vx); tetpil=avma;
	return gerepile(av,tetpil,changevar(y,p1));
      }

      tx = typ(x[1]); i = is_scalar_t(tx)? 0: degpol(x[1]);
      tx = typ(x[2]); n = is_scalar_t(tx)? 0: degpol(x[2]);
      n = i+n + 2;
      y = gdiv(gtrunc(gmul((GEN)x[2], integ(tayl(x,v,n),v))), (GEN)x[2]);
      if (!gegal(deriv(y,v),x)) err(talker,"a log/atan appears in intformal");
      if (typ(y)==t_RFRAC && lg(y[1]) == lg(y[2]))
      {
        GEN p2;
	tx=typ(y[1]); p1=is_scalar_t(tx)? (GEN)y[1]: leading_term((GEN)y[1]);
	tx=typ(y[2]); p2=is_scalar_t(tx)? (GEN)y[2]: leading_term((GEN)y[2]);
	y=gsub(y, gdiv(p1,p2));
      }
      return gerepileupto(av,y);

    case t_VEC: case t_COL: case t_MAT:
      lx=lg(x); y=cgetg(lx,tx);
      for (i=1; i<lg(x); i++) y[i]=linteg((GEN)x[i],v);
      return y;
  }
  err(typeer,"integ");
  return NULL; /* not reached */
}

/*******************************************************************/
/*                                                                 */
/*                    PARTIES ENTIERES                             */
/*                                                                 */
/*******************************************************************/

GEN
gfloor(GEN x)
{
  GEN y;
  long i,lx, tx = typ(x);

  switch(tx)
  {
    case t_INT:
    case t_POL: return gcopy(x);
    case t_REAL: return floorr(x);
    case t_FRAC: return truedvmdii((GEN)x[1],(GEN)x[2],NULL);
    case t_RFRAC: return gdeuc((GEN)x[1],(GEN)x[2]);
    case t_VEC: case t_COL: case t_MAT:
      lx = lg(x); y = cgetg(lx,tx);
      for (i=1; i<lx; i++) y[i] = lfloor((GEN)x[i]);
      return y;
  }
  err(typeer,"gfloor");
  return NULL; /* not reached */
}

GEN
gfrac(GEN x)
{
  pari_sp av = avma, tetpil;
  GEN p1 = gneg_i(gfloor(x));
  tetpil = avma; return gerepile(av,tetpil,gadd(x,p1));
}

/* assume x t_REAL */
GEN
ceilr(GEN x) {
  pari_sp av = avma;
  GEN y = floorr(x);
  if (cmpri(x, y)) return gerepileuptoint(av, addsi(1,y));
  return y;
}

GEN
gceil(GEN x)
{
  GEN y, p1;
  long i, lx, tx = typ(x);
  pari_sp av;

  switch(tx)
  {
    case t_INT: case t_POL: return gcopy(x);
    case t_REAL: return ceilr(x);
    case t_FRAC:
      av = avma; y = dvmdii((GEN)x[1],(GEN)x[2],&p1);
      if (p1 != gzero && gsigne(x) > 0)
      {
        cgiv(p1);
        return gerepileuptoint(av, addsi(1,y));
      }
      return y;

    case t_RFRAC:
      return gdeuc((GEN)x[1],(GEN)x[2]);

    case t_VEC: case t_COL: case t_MAT:
      lx = lg(x); y = cgetg(lx,tx);
      for (i=1; i<lx; i++) y[i] = lceil((GEN)x[i]);
      return y;
  }
  err(typeer,"gceil");
  return NULL; /* not reached */
}

GEN
round0(GEN x, GEN *pte)
{
  if (pte) { long e; x = grndtoi(x,&e); *pte = stoi(e); }
  return ground(x);
}

/* assume x a t_REAL */
GEN
roundr(GEN x)
{
  long ex, s = signe(x);
  pari_sp av;
  GEN t;
  if (!s || (ex=expo(x)) < -1) return gzero;
  if (ex < 0) return s>0? gun: negi(gun);
  av = avma;
  t = real2n(-1, 3 + (ex>>TWOPOTBITS_IN_LONG)); /* = 0.5 */
  return gerepileuptoint(av, floorr( addrr(x,t) ));
}

GEN
ground(GEN x)
{
  GEN y;
  long i, lx, tx=typ(x);
  pari_sp av;

  switch(tx)
  {
    case t_INT: case t_INTMOD: case t_QUAD: return gcopy(x);
    case t_REAL: return roundr(x);
    case t_FRAC: return diviiround((GEN)x[1], (GEN)x[2]);
    case t_POLMOD: y=cgetg(3,t_POLMOD);
      copyifstack(x[1],y[1]);
      y[2]=lround((GEN)x[2]); return y;

    case t_COMPLEX:
      av = avma; y = cgetg(3, t_COMPLEX);
      y[2] = lround((GEN)x[2]);
      if (!signe(y[2])) { avma = av; return ground((GEN)x[1]); }
      y[1] = lround((GEN)x[1]); return y;

    case t_POL: case t_SER: case t_RFRAC: case t_VEC: case t_COL: case t_MAT:
      y = init_gen_op(x, tx, &lx, &i);
      for (; i<lx; i++) y[i] = lround((GEN)x[i]);
      if (tx==t_POL) return normalizepol_i(y, lx);
      if (tx==t_SER) return normalize(y);
      return y;
  }
  err(typeer,"ground");
  return NULL; /* not reached */
}

/* e = number of error bits on integral part */
GEN
grndtoi(GEN x, long *e)
{
  GEN y, p1;
  long i, tx=typ(x), lx, ex, e1;
  pari_sp av;

  *e = -(long)HIGHEXPOBIT;
  switch(tx)
  {
    case t_INT: case t_INTMOD: case t_QUAD: case t_FRAC:
      return ground(x);

    case t_REAL:
      av = avma; p1 = gadd(ghalf,x); ex = expo(p1);
      if (ex < 0)
      {
	if (signe(p1)>=0) { *e = expo(x); avma = av; return gzero; }
        *e = expo(addsr(1,x)); avma = av; return negi(gun);
      }
      lx= lg(x); e1 = ex - bit_accuracy(lx) + 1;
      y = ishiftr_spec(p1, lx, e1);
      if (signe(x)<0) y=addsi(-1,y);
      y = gerepileuptoint(av,y);

      if (e1 <= 0) { av = avma; e1 = expo(subri(x,y)); avma = av; }
      *e = e1; return y;

    case t_POLMOD: y = cgetg(3,t_POLMOD);
      copyifstack(x[1], y[1]);
      y[2] = lrndtoi((GEN)x[2], e); return y;

    case t_COMPLEX:
      av = avma; y = cgetg(3, t_COMPLEX);
      y[2] = lrndtoi((GEN)x[2], e);
      if (!signe(y[2]))
      {
        avma = av;
        y = grndtoi((GEN)x[1], &e1);
      }
      else
        y[1] = lrndtoi((GEN)x[1], &e1);
      if (e1 > *e) *e = e1;
      return y;

    case t_POL: case t_SER: case t_RFRAC: case t_VEC: case t_COL: case t_MAT:
      y = init_gen_op(x, tx, &lx, &i);
      for (; i<lx; i++)
      {
        y[i] = lrndtoi((GEN)x[i],&e1);
        if (e1 > *e) *e = e1;
      }
      return y;
  }
  err(typeer,"grndtoi");
  return NULL; /* not reached */
}

/* floor(x * 2^s) */
GEN
gfloor2n(GEN x, long s)
{
  GEN z;
  switch(typ(x))
  {
    case t_INT:
      return shifti(x, s);
    case t_REAL:
      return ishiftr(x, s);
    case t_COMPLEX:
      z = cgetg(3, t_COMPLEX);
      z[1] = (long)gfloor2n((GEN)x[1], s);
      z[2] = (long)gfloor2n((GEN)x[2], s);
      return z;
    default: err(typeer,"gfloor2n");
      return NULL; /* not reached */
  }
}

/* e = number of error bits on integral part */
GEN
gcvtoi(GEN x, long *e)
{
  long tx = typ(x), lx, i, ex, e1;
  GEN y;

  if (tx == t_REAL)
  {
    ex = expo(x); if (ex < 0) { *e = ex; return gzero; }
    lx = lg(x); e1 = ex - bit_accuracy(lx) + 1;
    y = ishiftr_spec(x, lx, e1);
    if (e1 <= 0) { pari_sp av = avma; e1 = expo(subri(x,y)); avma = av; }
    *e = e1; return y;
  }
  *e = -(long)HIGHEXPOBIT;
  if (is_matvec_t(tx))
  {
    lx = lg(x); y = cgetg(lx,tx);
    for (i=1; i<lx; i++)
    {
      y[i] = lcvtoi((GEN)x[i],&e1);
      if (e1 > *e) *e = e1;
    }
    return y;
  }
  return gtrunc(x);
}

/* smallest integer greater than any incarnations of the real x
 * [avoid mpfloor() and "precision loss in truncation"] */
GEN
ceil_safe(GEN x)
{
  pari_sp av = avma;
  long e;
  GEN y = gcvtoi(x,&e);
  if (e < 0) e = 0;
  y = addii(y, shifti(gun,e));
  return gerepileuptoint(av, y);
}

static GEN
ser2rfrac(GEN x)
{
  pari_sp av = avma;
  return gerepilecopy(av, gconvsp(x, valp(x)));
}

GEN
gtrunc(GEN x)
{
  long tx=typ(x), i, v;
  pari_sp av;
  GEN y;

  switch(tx)
  {
    case t_INT: case t_POL:
      return gcopy(x);

    case t_REAL:
      return mptrunc(x);

    case t_FRAC:
      return divii((GEN)x[1],(GEN)x[2]);

    case t_PADIC:
      if (!signe(x[4])) return gzero;
      v = valp(x);
      if (!v) return gcopy((GEN)x[4]);
      if (v>0)
      { /* here p^v is an integer */
        av = avma; y = gpowgs((GEN)x[2],v);
        return gerepileuptoint(av, mulii(y,(GEN)x[4]));
      }
      y=cgetg(3,t_FRAC);
      y[1] = licopy((GEN)x[4]);
      y[2] = lpowgs((GEN)x[2],-v);
      return y;

    case t_RFRAC:
      return gdeuc((GEN)x[1],(GEN)x[2]);

    case t_SER:
      return ser2rfrac(x);

    case t_VEC: case t_COL: case t_MAT:
    {
      long lx = lg(x); y = cgetg(lx,tx);
      for (i=1; i<lx; i++) y[i]=ltrunc((GEN)x[i]);
      return y;
    }
  }
  err(typeer,"gtrunc");
  return NULL; /* not reached */
}

GEN
trunc0(GEN x, GEN *pte)
{
  if (pte) { long e; x = gcvtoi(x,&e); *pte = stoi(e); }
  return gtrunc(x);
}
/*******************************************************************/
/*                                                                 */
/*                             ZERO                                */
/*                                                                 */
/*******************************************************************/
/* O(p^e) */
GEN
zeropadic(GEN p, long e)
{
  GEN y = cgetg(5,t_PADIC);
  y[4] = zero;
  y[3] = un;
  copyifstack(p,y[2]);
  y[1] = evalvalp(e) | evalprecp(0);
  return y;
}
/* O(polx[v]^e) */
GEN
zeroser(long v, long e)
{
  GEN x = cgetg(2, t_SER);
  x[1] = evalvalp(e) | evalvarn(v); return x;
}
/* 0 * polx[v] */
GEN
zeropol(long v)
{
  GEN x = cgetg(2,t_POL);
  x[1] = evalvarn(v); return x;
}
/* vector(n) */
GEN
zerocol(long n)
{
  GEN y = cgetg(n+1,t_COL);
  long i; for (i=1; i<=n; i++) y[i]=zero;
  return y;
}
/* vectorv(n) */
GEN
zerovec(long n)
{
  GEN y = cgetg(n+1,t_VEC);
  long i; for (i=1; i<=n; i++) y[i]=zero;
  return y;
}
/* matrix(m, n) */
GEN
zeromat(long m, long n)
{
  GEN y = cgetg(n+1,t_MAT);
  GEN v = zerocol(m);
  long i; for (i=1; i<=n; i++) y[i]=(long)v;
  return y;
}
/*******************************************************************/
/*                                                                 */
/*                  CONVERSIONS -->  INT, POL & SER                */
/*                                                                 */
/*******************************************************************/

/* return a_n B^n + ... + a_0, where B = 2^32. Assume n even if BIL=64 and
 * the a_i are 32bits integers */
GEN
coefs_to_int(long n, ...)
{
  va_list ap;
  pari_sp av = avma;
  GEN x, y;
  long i;
  int iszero = 1;
  va_start(ap,n);
#ifdef LONG_IS_64BIT
  n >>= 1;
#endif
  x = cgetg(n+2, t_INT); 
  x[1] = evallgefint(n+2) | evalsigne(1);
  y = int_MSW(x);
  for (i=0; i <n; i++)
  {
#ifdef LONG_IS_64BIT
    ulong a = va_arg(ap, long);
    ulong b = va_arg(ap, long);
    *y = (a << 32) | b;
#else
    *y = va_arg(ap, long);
#endif
    if (*y) iszero = 0;
    y = int_precW(y);
  }
  va_end(ap);
  if (iszero) { avma = av; return gzero; }
  return x;
}

/* 2^32 a + b */
GEN
u2toi(ulong a, ulong b)
{
  GEN x;
  if (!a && !b) return gzero;
#ifdef LONG_IS_64BIT
  x = cgetg(3, t_INT);
  x[1] = evallgefint(3)|evalsigne(1);
  x[2] = (long) ((a << 32) | b);
#else
  x = cgetg(4, t_INT);
  x[1] = evallgefint(4)|evalsigne(1);
  *(int_MSW(x)) = (long)a;
  *(int_LSW(x)) = (long)b;
#endif
  return x;
}

/* return a_(n-1) x^(n-1) + ... + a_0 */
GEN
coefs_to_pol(long n, ...)
{
  va_list ap;
  GEN x, y;
  long i;
  va_start(ap,n);
  x = cgetg(n+2, t_POL); y = x + 2;
  x[1] = evalvarn(0);
  for (i=n-1; i >= 0; i--) y[i] = (long) va_arg(ap, GEN);
  va_end(ap); return normalizepol(x);
}

/* return [a_1, ..., a_n] */
GEN
coefs_to_vec(long n, ...)
{
  va_list ap;
  GEN x;
  long i;
  va_start(ap,n);
  x = cgetg(n+1, t_VEC);
  for (i=1; i <= n; i++) x[i] = (long) va_arg(ap, GEN);
  va_end(ap); return x;
}

GEN
coefs_to_col(long n, ...)
{
  va_list ap;
  GEN x;
  long i;
  va_start(ap,n);
  x = cgetg(n+1, t_COL);
  for (i=1; i <= n; i++) x[i] = (long) va_arg(ap, GEN);
  va_end(ap); return x;
}

GEN
scalarpol(GEN x, long v)
{
  GEN y=cgetg(3,t_POL);
  y[1] = gcmp0(x)? evalvarn(v)
                 : evalvarn(v) | evalsigne(1);
  y[2]=lcopy(x); return y;
}

/* deg1pol(a,b,x)=a*x+b*/
GEN
deg1pol(GEN x1, GEN x0,long v)
{
  GEN x = cgetg(4,t_POL);
  x[1] = evalsigne(1) | evalvarn(v);
  x[2] = lcopy(x0);
  x[3] = lcopy(x1); return normalizepol_i(x,4);
}

/* same, no copy */
GEN
deg1pol_i(GEN x1, GEN x0,long v)
{
  GEN x = cgetg(4,t_POL);
  x[1] = evalsigne(1) | evalvarn(v);
  x[2] = (long)x0;
  x[3] = (long)x1; return normalizepol_i(x,4);
}

static GEN
gtopoly0(GEN x, long v, int reverse)
{
  long tx=typ(x),lx,i,j;
  GEN y;

  if (v<0) v = 0;
  if (isexactzero(x)) return zeropol(v);
  if (is_scalar_t(tx)) return scalarpol(x,v);

  switch(tx)
  {
    case t_POL:
      y=gcopy(x); break;
    case t_SER:
      y = ser2rfrac(x);
      if (typ(y) != t_POL)
        err(talker,"t_SER with negative valuation in gtopoly");
      break;
    case t_RFRAC:
      y=gdeuc((GEN)x[1],(GEN)x[2]); break;
    case t_QFR: case t_QFI: case t_VEC: case t_COL: case t_MAT:
      lx=lg(x);
      if (reverse)
      {
	while (lx-- && isexactzero((GEN)x[lx]));
	i = lx+2; y=cgetg(i,t_POL);
	y[1] = gcmp0(x)? 0: evalsigne(1);
	for (j=2; j<i; j++) y[j]=lcopy((GEN)x[j-1]);
      }
      else
      {
	i=1; j=lx; while (lx-- && isexactzero((GEN)x[i++]));
	i=lx+2; y=cgetg(i,t_POL);
	y[1] = gcmp0(x)? 0: evalsigne(1);
	lx = j-1;
	for (j=2; j<i; j++) y[j]=lcopy((GEN)x[lx--]);
      }
      break;
    default: err(typeer,"gtopoly");
      return NULL; /* not reached */
  }
  setvarn(y,v); return y;
}

GEN
gtopolyrev(GEN x, long v) { return gtopoly0(x,v,1); }

GEN
gtopoly(GEN x, long v) { return gtopoly0(x,v,0); }

GEN
scalarser(GEN x, long v, long prec)
{
  long i, l = prec+2;
  GEN y = cgetg(l, t_SER);
  y[1] = evalsigne(1) | evalvalp(0) | evalvarn(v);
  y[2] = lcopy(x); for (i=3; i<l; i++) y[i] = zero;
  return y;
}

static GEN _gtoser(GEN x, long v, long prec);

/* assume x a t_[SER|POL], apply gtoser to all coeffs */
static GEN
coefstoser(GEN x, long v, long prec)
{
  long i, tx = typ(x), lx = lg(x);
  GEN y = cgetg(lx, tx);
  y[1] = x[1];
  for (i=2; i<lx; i++) y[i] = (long)_gtoser((GEN)x[i], v, prec);
  return y;
}

/* assume x a scalar or t_POL. Not stack-clean */
GEN
poltoser(GEN x, long v, long prec)
{
  long tx = typ(x), vx = varn(x), lx, i, j, l;
  GEN y;

  if (gcmp0(x)) return zeroser(v, prec);
  if (is_scalar_t(tx) || varncmp(vx, v) > 0) return scalarser(x, v, prec);
  if (varncmp(vx, v) < 0) return coefstoser(x, v, prec);

  lx = lg(x); i = 2; while (i<lx && gcmp0((GEN)x[i])) i++;
  l = lx-i; if (precdl > (ulong)l) l = (long)precdl;
  l += 2;
  y = cgetg(l,t_SER);
  y[1] = evalsigne(1) | evalvalp(i-2) | evalvarn(v);
  for (j=2; j<=lx-i+1; j++) y[j] = x[j+i-2];
  for (   ; j < l;     j++) y[j] = zero;
  return y;
}

/* x a t_RFRAC[N]. Not stack-clean */
GEN
rfractoser(GEN x, long v, long prec)
{
  return gdiv(poltoser((GEN)x[1], v, prec), 
              poltoser((GEN)x[2], v, prec));
}

GEN
_toser(GEN x)
{
  switch(typ(x))
  {
    case t_SER: return x;
    case t_POL: return poltoser(x, varn(x), precdl);
    case t_RFRAC: return rfractoser(x, gvar(x), precdl);
  }
  return NULL;
}

static GEN
_gtoser(GEN x, long v, long prec)
{
  long tx=typ(x), lx, i, j, l;
  pari_sp av;
  GEN y;

  if (v < 0) v = 0;
  if (tx == t_SER)
  {
    long vx = varn(x);
    if      (varncmp(vx, v) < 0) y = coefstoser(x, v, prec);
    else if (varncmp(vx, v) > 0) y = scalarser(x, v, prec);
    else y = gcopy(x);
    return y;
  }
  if (is_scalar_t(tx)) return scalarser(x,v,prec);
  switch(tx)
  {
    case t_POL:
      if (isexactzero(x)) return zeroser(v,prec);
      y = poltoser(x, v, prec); l = lg(y);
      for (i=2; i<l; i++)
        if (y[i] != zero) y[i] = lcopy((GEN)y[i]);
      break;

    case t_RFRAC:
      if (isexactzero(x)) return zeroser(v,prec);
      av = avma;
      return gerepileupto(av, rfractoser(x, v, prec));

    case t_QFR: case t_QFI: case t_VEC: case t_COL:
      if (isexactzero(x)) return zeroser(v, lg(x)-1);
      lx = lg(x); i=1; while (i<lx && isexactzero((GEN)x[i])) i++;
      y = cgetg(lx-i+2,t_SER);
      y[1] = evalsigne(1) | evalvalp(i-1) | evalvarn(v);
      for (j=2; j<=lx-i+1; j++) y[j]=lcopy((GEN)x[j+i-2]);
      break;

    default: err(typeer,"gtoser");
      return NULL; /* not reached */
  }
  return y;
}

GEN
gtoser(GEN x, long v) { return _gtoser(x,v,precdl); }

/* assume typ(x) = t_STR */
static GEN
str_to_vecsmall(GEN x)
{
  char *s = GSTR(x);
  long i, l = strlen(s);
  GEN y = cgetg(l+1, t_VECSMALL);
  s--;
  for (i=1; i<=l; i++) y[i] = (long)s[i];
  return y;
}

GEN
gtovec(GEN x)
{
  long tx,lx,i;
  GEN y;

  if (!x) return cgetg(1,t_VEC);
  tx = typ(x);
  if (is_scalar_t(tx) || tx == t_RFRAC)
  {
    y=cgetg(2,t_VEC); y[1]=lcopy(x);
    return y;
  }
  if (tx == t_STR)
  {
    char t[2] = {0,0};
    y = str_to_vecsmall(x);
    lx = lg(y);
    for (i=1; i<lx; i++)
    {
      t[0] = y[i];
      y[i] = (long)STRtoGENstr(t);
    }
    settyp(y,t_VEC); return y;
  }
  if (is_graphicvec_t(tx))
  {
    lx=lg(x); y=cgetg(lx,t_VEC);
    for (i=1; i<lx; i++) y[i]=lcopy((GEN)x[i]);
    return y;
  }
  if (tx==t_POL)
  {
    lx=lg(x); y=cgetg(lx-1,t_VEC);
    for (i=1; i<=lx-2; i++) y[i]=lcopy((GEN)x[lx-i]);
    return y;
  }
  if (tx==t_LIST)
  {
    lx=lgeflist(x); y=cgetg(lx-1,t_VEC); x++;
    for (i=1; i<=lx-2; i++) y[i]=lcopy((GEN)x[i]);
    return y;
  }
  if (tx == t_VECSMALL) return zv_ZV(x);
  if (!signe(x)) { y=cgetg(2,t_VEC); y[1]=zero; return y; }
  lx=lg(x); y=cgetg(lx-1,t_VEC); x++;
  for (i=1; i<=lx-2; i++) y[i]=lcopy((GEN)x[i]);
  return y;
}

GEN
gtocol(GEN x)
{
  long lx, tx, i, j, h;
  GEN y;
  if (!x) return cgetg(1,t_COL);
  tx = typ(x);
  if (tx != t_MAT) { y = gtovec(x); settyp(y, t_COL); return y; }
  lx = lg(x); if (lx == 1) return cgetg(1, t_COL);
  h = lg(x[1]); y = cgetg(h, t_COL);
  for (i = 1 ; i < h; i++) {
    y[i] = lgetg(lx, t_VEC);
    for (j = 1; j < lx; j++) mael(y,i,j) = lcopy(gcoeff(x,i,j));
  }
  return y;
}

GEN
gtovecsmall(GEN x)
{
  GEN V;
  long tx, l,i;
  
  if (!x) return cgetg(1,t_VECSMALL);
  tx = typ(x);
  if (tx == t_VECSMALL) return gcopy(x);
  if (tx == t_INT)
  {
    GEN u = cgetg(2, t_VECSMALL);
    u[1] = itos(x); return u;
  } 
  if (tx == t_STR) return str_to_vecsmall(x);
  if (!is_vec_t(tx)) err(typeer,"vectosmall");
  l = lg(x);
  V = cgetg(l,t_VECSMALL);
  for(i=1;i<l;i++) V[i] = itos((GEN)x[i]);
  return V;
}

GEN
compo(GEN x, long n)
{
  long l,tx=typ(x);

  if (tx==t_POL && n+1 >= lg(x)) return gzero;
  if (tx==t_SER && !signe(x)) return gzero;
  if (!is_recursive_t(tx))
    err(talker, "this object doesn't have components (is a leaf)");
  l=lontyp[tx]+n-1;
  if (n<1 || l>=lg(x))
    err(talker,"nonexistent component");
  return gcopy((GEN)x[l]);
}

/* assume v > varn(x), extract coeff of polx[v]^n */
static GEN
multi_coeff(GEN x, long n, long v, long dx)
{
  long i, lx = dx+3;
  GEN z = cgetg(lx, t_POL); z[1] = x[1];
  for (i = 2; i < lx; i++) z[i] = (long)polcoeff_i((GEN)x[i], n, v);
  return normalizepol_i(z, lx);
}

/* assume x a t_POL */
static GEN
_polcoeff(GEN x, long n, long v)
{
  long w, dx;
  dx = degpol(x);
  if (dx < 0) return gzero;
  if (v < 0 || v == (w=varn(x)))
    return (n < 0 || n > dx)? gzero: (GEN)x[n+2];
  if (w > v) return n? gzero: x;
  /* w < v */
  return multi_coeff(x, n, v, dx);
}

/* assume x a t_SER */
static GEN
_sercoeff(GEN x, long n, long v)
{
  long w, dx, ex = valp(x), N = n - ex;
  GEN z;
  if (!signe(x))
  {
    if (N >= 0) err(talker,"non existent component in truecoeff");
    return gzero;
  }
  dx = lg(x)-3;
  if (v < 0 || v == (w=varn(x)))
  {
    if (N > dx) err(talker,"non existent component in truecoeff");
    return (N < 0)? gzero: (GEN)x[N+2];
  }
  if (w > v) return N? gzero: x;
  /* w < v */
  z = multi_coeff(x, n, v, dx);
  if (ex) z = gmul(z, gpowgs(polx[w], ex));
  return z;
}

/* assume x a t_RFRAC(n) */
static GEN
_rfraccoeff(GEN x, long n, long v)
{
  GEN P,Q, p = (GEN)x[1], q = (GEN)x[2];
  long vp = gvar(p), vq = gvar(q);
  if (v < 0) v = min(vp, vq);
  P = (vp == v)? p: swap_vars(p, v);
  Q = (vq == v)? q: swap_vars(q, v);
  if (!ismonome(Q)) err(typeer, "polcoeff");
  n += degpol(Q);
  return gdiv(_polcoeff(P, n, v), leading_term(Q));
}

GEN
polcoeff_i(GEN x, long n, long v)
{
  switch(typ(x))
  {
    case t_POL: return _polcoeff(x,n,v);
    case t_SER: return _sercoeff(x,n,v);
    case t_RFRAC: return _rfraccoeff(x,n,v);
    default: return n? gzero: x;
  }
}

/* with respect to the main variable if v<0, with respect to the variable v
   otherwise. v ignored if x is not a polynomial/series. */
GEN
polcoeff0(GEN x, long n, long v)
{
  long tx=typ(x);
  pari_sp av;

  if (is_scalar_t(tx)) return n? gzero: gcopy(x);

  av = avma;
  switch(tx)
  {
    case t_POL: x = _polcoeff(x,n,v); break;
    case t_SER: x = _sercoeff(x,n,v); break;
    case t_RFRAC: x = _rfraccoeff(x,n,v); break;
   
    case t_QFR: case t_QFI: case t_VEC: case t_COL: case t_MAT:
      if (n>=1 && n<lg(x)) return gcopy((GEN)x[n]);
    /* fall through */

    default: err(talker,"nonexistent component in truecoeff");
  }
  if (x == gzero) return x;
  if (avma == av) return gcopy(x);
  return gerepilecopy(av, x);
}

GEN
truecoeff(GEN x, long n)
{
  return polcoeff0(x,n,-1);
}

GEN
denom(GEN x)
{
  long lx, i;
  pari_sp av, tetpil;
  GEN s,t;

  switch(typ(x))
  {
    case t_INT: case t_REAL: case t_INTMOD: case t_PADIC: case t_SER:
      return gun;

    case t_FRAC:
      return absi((GEN)x[2]);

    case t_COMPLEX:
      av=avma; t=denom((GEN)x[1]); s=denom((GEN)x[2]); tetpil=avma;
      return gerepile(av,tetpil,glcm(s,t));

    case t_QUAD:
      av=avma; t=denom((GEN)x[2]); s=denom((GEN)x[3]); tetpil=avma;
      return gerepile(av,tetpil,glcm(s,t));

    case t_POLMOD:
      return denom((GEN)x[2]);

    case t_RFRAC:
      return gcopy((GEN)x[2]);

    case t_POL:
      return polun[varn(x)];

    case t_VEC: case t_COL: case t_MAT:
      lx=lg(x); if (lx==1) return gun;
      av = tetpil = avma; s = denom((GEN)x[1]);
      for (i=2; i<lx; i++)
      {
        t = denom((GEN)x[i]);
        /* t != gun est volontaire */
        if (t != gun) { tetpil=avma; s=glcm(s,t); }
      }
      return gerepile(av,tetpil,s);
  }
  err(typeer,"denom");
  return NULL; /* not reached */
}

GEN
numer(GEN x)
{
  pari_sp av, tetpil;
  GEN s;

  switch(typ(x))
  {
    case t_INT: case t_REAL: case t_INTMOD:
    case t_PADIC: case t_POL: case t_SER:
      return gcopy(x);

    case t_FRAC:
      return (signe(x[2])>0)? gcopy((GEN)x[1]): gneg((GEN)x[1]);

    case t_POLMOD:
      av=avma; s=numer((GEN)x[2]); tetpil=avma;
      return gerepile(av,tetpil,gmodulcp(s,(GEN)x[1]));

    case t_RFRAC:
      return gcopy((GEN)x[1]);

    case t_COMPLEX: case t_QUAD: case t_VEC: case t_COL: case t_MAT:
      av=avma; s=denom(x); tetpil=avma;
      return gerepile(av,tetpil,gmul(s,x));
  }
  err(typeer,"numer");
  return NULL; /* not reached */
}

/* Lift only intmods if v does not occur in x, lift with respect to main
 * variable of x if v < 0, with respect to variable v otherwise.
 */
GEN
lift0(GEN x, long v)
{
  long lx,tx=typ(x),i;
  GEN y;

  switch(tx)
  {
    case t_INT: case t_REAL:
      return gcopy(x);

    case t_INTMOD:
      return gcopy((GEN)x[2]);

    case t_POLMOD:
      if (v < 0 || v == varn((GEN)x[1])) return gcopy((GEN)x[2]);
      y = cgetg(3,tx);
      y[1] = (long)lift0((GEN)x[1],v);
      y[2] = (long)lift0((GEN)x[2],v); return y;

    case t_SER:
      if (!signe(x)) return gcopy(x);
      lx=lg(x); y=cgetg(lx,tx); y[1]=x[1];
      for (i=2; i<lx; i++) y[i]=(long)lift0((GEN)x[i],v);
      return y;

    case t_PADIC:
      return gtrunc(x);

    case t_FRAC: case t_COMPLEX: case t_RFRAC:
    case t_VEC: case t_COL: case t_MAT:
      lx=lg(x); y=cgetg(lx,tx);
      for (i=1; i<lx; i++) y[i]=(long)lift0((GEN)x[i],v);
      return y;

    case t_POL:
      lx=lg(x); y=cgetg(lx,tx); y[1]=x[1];
      for (i=2; i<lx; i++) y[i]=(long)lift0((GEN)x[i],v);
      return y;

    case t_QUAD:
      y=cgetg(4,tx); copyifstack(x[1],y[1]);
      for (i=2; i<4; i++) y[i]=(long)lift0((GEN)x[i],v);
      return y;
  }
  err(typeer,"lift");
  return NULL; /* not reached */
}

GEN
lift(GEN x)
{
  return lift0(x,-1);
}

/* same as lift, without copy. May DESTROY x. For internal use only.
   Conventions on v as for lift. */
GEN
lift_intern0(GEN x, long v)
{
  long i,lx,tx=typ(x);

  switch(tx)
  {
    case t_INT: case t_REAL:
      return x;

    case t_INTMOD:
      return (GEN)x[2];

    case t_POLMOD:
      if (v < 0 || v == varn((GEN)x[1])) return (GEN)x[2];
      x[1]=(long)lift_intern0((GEN)x[1],v);
      x[2]=(long)lift_intern0((GEN)x[2],v);
      return x;

    case t_SER: if (!signe(x)) return x; /* fall through */
    case t_FRAC: case t_COMPLEX: case t_QUAD: case t_POL:
    case t_RFRAC: case t_VEC: case t_COL: case t_MAT:
      lx = lg(x);
      for (i = lx-1; i>=lontyp[tx]; i--)
        x[i] = (long) lift_intern0((GEN)x[i],v);
      return x;
  }
  err(typeer,"lift_intern");
  return NULL; /* not reached */
}

/* memes conventions pour v que lift */
GEN
centerlift0(GEN x, long v)
{
  long i, lx, tx = typ(x);
  pari_sp av;
  GEN y;

  switch(tx)
  {
    case t_INT:
      return icopy(x);

    case t_INTMOD:
      av = avma; i = cmpii(shifti((GEN)x[2],1), (GEN)x[1]); avma = av;
      return (i > 0)? subii((GEN)x[2],(GEN)x[1]): icopy((GEN)x[2]);

    case t_POLMOD:
      if (v < 0 || v == varn((GEN)x[1])) return gcopy((GEN)x[2]);
      y = cgetg(3, t_POLMOD);
      y[1] = (long)centerlift0((GEN)x[1],v);
      y[2] = (long)centerlift0((GEN)x[2],v); return y;

    case t_POL: case t_SER:
    case t_FRAC: case t_COMPLEX: case t_RFRAC:
    case t_VEC: case t_COL: case t_MAT:
      y = init_gen_op(x, tx, &lx, &i);
      for (; i<lx; i++) y[i] = (long)centerlift0((GEN)x[i],v);
      return y;

    case t_QUAD:
      y=cgetg(4, t_QUAD); copyifstack(x[1],y[1]);
      y[2] = (long)centerlift0((GEN)x[2],v);
      y[3] = (long)centerlift0((GEN)x[3],v); return y;
  }
  err(typeer,"centerlift");
  return NULL; /* not reached */
}

GEN
centerlift(GEN x)
{
  return centerlift0(x,-1);
}

/*******************************************************************/
/*                                                                 */
/*                  PARTIES REELLE ET IMAGINAIRES                  */
/*                                                                 */
/*******************************************************************/

static GEN
op_ReIm(GEN f(GEN), GEN x)
{
  long lx, i, j, tx = typ(x);
  pari_sp av;
  GEN z;

  switch(tx)
  {
    case t_POL:
      lx=lg(x); av=avma;
      for (i=lx-1; i>=2; i--)
        if (!gcmp0(f((GEN)x[i]))) break;
      avma=av; if (i==1) return zeropol(varn(x));

      z = cgetg(i+1,t_POL); z[1] = x[1];
      for (j=2; j<=i; j++) z[j] = (long)f((GEN)x[j]);
      return z;

    case t_SER:
      if (gcmp0(x)) { z=cgetg(2,t_SER); z[1]=x[1]; return z; }
      lx=lg(x); av=avma;
      for (i=2; i<lx; i++)
        if (!gcmp0(f((GEN)x[i]))) break;
      avma=av; if (i==lx) return zeroser(varn(x),lx-2+valp(x));

      z=cgetg(lx-i+2,t_SER); z[1]=x[1]; setvalp(z, valp(x)+i-2);
      for (j=2; i<lx; j++,i++) z[j] = (long) f((GEN)x[i]);
      return z;

    case t_RFRAC: {
      GEN dxb, n, d;
      av = avma; dxb = gconj((GEN)x[2]);
      n = gmul((GEN)x[1], dxb);
      d = gmul((GEN)x[2], dxb);
      return gerepileupto(av, gdiv(f(n), d));
    }

    case t_VEC: case t_COL: case t_MAT:
      lx=lg(x); z=cgetg(lx,tx);
      for (i=1; i<lx; i++) z[i] = (long) f((GEN)x[i]);
      return z;
  }
  err(typeer,"greal/gimag");
  return NULL; /* not reached */
}

GEN
real_i(GEN x)
{
  switch(typ(x))
  {
    case t_INT: case t_REAL: case t_FRAC:
      return x;
    case t_COMPLEX:
      return (GEN)x[1];
    case t_QUAD:
      return (GEN)x[2];
  }
  return op_ReIm(real_i,x);
}
GEN
imag_i(GEN x)
{
  switch(typ(x))
  {
    case t_INT: case t_REAL: case t_FRAC:
      return gzero;
    case t_COMPLEX:
      return (GEN)x[2];
    case t_QUAD:
      return (GEN)x[3];
  }
  return op_ReIm(imag_i,x);
}
GEN
greal(GEN x)
{
  switch(typ(x))
  {
    case t_INT: case t_REAL: case t_FRAC:
      return gcopy(x);

    case t_COMPLEX:
      return gcopy((GEN)x[1]);

    case t_QUAD:
      return gcopy((GEN)x[2]);
  }
  return op_ReIm(greal,x);
}
GEN
gimag(GEN x)
{
  switch(typ(x))
  {
    case t_INT: case t_REAL: case t_FRAC:
      return gzero;

    case t_COMPLEX:
      return gcopy((GEN)x[2]);

    case t_QUAD:
      return gcopy((GEN)x[3]);
  }
  return op_ReIm(gimag,x);
}

/*******************************************************************/
/*                                                                 */
/*                     LOGICAL OPERATIONS                          */
/*                                                                 */
/*******************************************************************/
static long
_egal(GEN x, GEN y)
{
  pari_sp av = avma;
  long r = gegal(simplify_i(x), simplify_i(y));
  avma = av; return r;
}

GEN
glt(GEN x, GEN y) { return gcmp(x,y)<0? gun: gzero; }

GEN
gle(GEN x, GEN y) { return gcmp(x,y)<=0? gun: gzero; }

GEN
gge(GEN x, GEN y) { return gcmp(x,y)>=0? gun: gzero; }

GEN
ggt(GEN x, GEN y) { return gcmp(x,y)>0? gun: gzero; }

GEN
geq(GEN x, GEN y) { return _egal(x,y)? gun: gzero; }

GEN
gne(GEN x, GEN y) { return _egal(x,y)? gzero: gun; }

GEN
gand(GEN x, GEN y) { return gcmp0(x)? gzero: (gcmp0(y)? gzero: gun); }

GEN
gor(GEN x, GEN y) { return gcmp0(x)? (gcmp0(y)? gzero: gun): gun; }

GEN
gnot(GEN x) { return gcmp0(x)? gun: gzero; }

/*******************************************************************/
/*                                                                 */
/*                      FORMAL SIMPLIFICATIONS                     */
/*                                                                 */
/*******************************************************************/

GEN
geval(GEN x)
{
  long lx, i, tx = typ(x);
  pari_sp av, tetpil;
  GEN y,z;

  if (is_const_t(tx)) return gcopy(x);
  if (is_graphicvec_t(tx))
  {
    lx=lg(x); y=cgetg(lx, tx);
    for (i=1; i<lx; i++) y[i] = (long)geval((GEN)x[i]);
    return y;
  }

  switch(tx)
  {
    case t_STR:
      return flisseq(GSTR(x));

    case t_POLMOD: y=cgetg(3,tx);
      y[1]=(long)geval((GEN)x[1]);
      av=avma; z=geval((GEN)x[2]); tetpil=avma;
      y[2]=lpile(av,tetpil,gmod(z,(GEN)y[1]));
      return y;

    case t_POL:
      lx=lg(x); if (lx==2) return gzero;
      {
#define initial_value(ep) (GEN)((ep)+1) /* cf anal.h */
        entree *ep = varentries[varn(x)];
        if (!ep) return gcopy(x);
        z = (GEN)ep->value;
        if (gegal(x, initial_value(ep))) return gcopy(z);
#undef initial_value
      }
      y=gzero; av=avma;
      for (i=lx-1; i>1; i--)
        y = gadd(geval((GEN)x[i]), gmul(z,y));
      return gerepileupto(av, y);

    case t_SER:
      err(impl, "evaluation of a power series");

    case t_RFRAC:
      return gdiv(geval((GEN)x[1]),geval((GEN)x[2]));
  }
  err(typeer,"geval");
  return NULL; /* not reached */
}

GEN
simplify_i(GEN x)
{
  long tx=typ(x),i,lx;
  GEN y;

  switch(tx)
  {
    case t_INT: case t_REAL: case t_FRAC:
    case t_INTMOD: case t_PADIC: case t_QFR: case t_QFI:
    case t_LIST: case t_STR: case t_VECSMALL:
      return x;

    case t_COMPLEX:
      if (isexactzero((GEN)x[2])) return simplify_i((GEN)x[1]);
      y=cgetg(3,t_COMPLEX);
      y[1]=(long)simplify_i((GEN)x[1]);
      y[2]=(long)simplify_i((GEN)x[2]); return y;

    case t_QUAD:
      if (isexactzero((GEN)x[3])) return simplify_i((GEN)x[2]);
      y=cgetg(4,t_QUAD);
      y[1]=x[1];
      y[2]=(long)simplify_i((GEN)x[2]);
      y[3]=(long)simplify_i((GEN)x[3]); return y;

    case t_POLMOD: y=cgetg(3,t_POLMOD);
      y[1]=(long)simplify_i((GEN)x[1]);
      i = typ(y[1]);
      if (i != t_POL)
      {
        if (i == t_INT) y[0] = evallg(3)|evaltyp(t_INTMOD);
        else y[1] = x[1]; /* invalid object otherwise */
      }
      y[2]=lmod(simplify_i((GEN)x[2]),(GEN)y[1]); return y;

    case t_POL:
      lx=lg(x); if (lx==2) return gzero;
      if (lx==3) return simplify_i((GEN)x[2]);
      y=cgetg(lx,t_POL); y[1]=x[1];
      for (i=2; i<lx; i++) y[i]=(long)simplify_i((GEN)x[i]);
      return y;

    case t_SER:
      if (!signe(x)) return gcopy(x);
      lx=lg(x);
      y=cgetg(lx,t_SER); y[1]=x[1];
      for (i=2; i<lx; i++) y[i]=(long)simplify_i((GEN)x[i]);
      return y;

    case t_RFRAC: y=cgetg(3,t_RFRAC);
      y[1]=(long)simplify_i((GEN)x[1]);
      y[2]=(long)simplify_i((GEN)x[2]); return y;

    case t_VEC: case t_COL: case t_MAT:
      lx=lg(x); y=cgetg(lx,tx);
      for (i=1; i<lx; i++) y[i]=(long)simplify_i((GEN)x[i]);
      return y;
  }
  err(typeer,"simplify_i");
  return NULL; /* not reached */
}

GEN
simplify(GEN x)
{
  pari_sp av = avma;
  return gerepilecopy(av, simplify_i(x));
}

/*******************************************************************/
/*                                                                 */
/*                EVALUATION OF SOME SIMPLE OBJECTS                */
/*                                                                 */
/*******************************************************************/
static GEN
qfeval0_i(GEN q, GEN x, long n)
{
  long i, j;
  pari_sp av=avma;
  GEN res=gzero;

  for (i=2;i<n;i++)
    for (j=1;j<i;j++)
      res = gadd(res, gmul(gcoeff(q,i,j), mulii((GEN)x[i],(GEN)x[j])) );
  res=gshift(res,1);
  for (i=1;i<n;i++)
    res = gadd(res, gmul(gcoeff(q,i,i), sqri((GEN)x[i])) );
  return gerepileupto(av,res);
}

#if 0
static GEN
qfeval0(GEN q, GEN x, long n)
{
  long i, j;
  pari_sp av=avma;
  GEN res=gzero;

  for (i=2;i<n;i++)
    for (j=1;j<i;j++)
      res = gadd(res, gmul(gcoeff(q,i,j), gmul((GEN)x[i],(GEN)x[j])) );
  res=gshift(res,1);
  for (i=1;i<n;i++)
    res = gadd(res, gmul(gcoeff(q,i,i), gsqr((GEN)x[i])) );
  return gerepileupto(av,res);
}
#else
static GEN
qfeval0(GEN q, GEN x, long n)
{
  long i, j;
  pari_sp av=avma;
  GEN res = gmul(gcoeff(q,1,1), gsqr((GEN)x[1]));

  for (i=2; i<n; i++)
  {
    GEN l = (GEN)q[i];
    GEN sx = gmul((GEN)l[1], (GEN)x[1]);
    for (j=2; j<i; j++)
      sx = gadd(sx, gmul((GEN)l[j],(GEN)x[j]));
    sx = gadd(gshift(sx,1), gmul((GEN)l[i],(GEN)x[i]));

    res = gadd(res, gmul((GEN)x[i], sx));
  }
  return gerepileupto(av,res);
}
#endif

/* We assume q is a real symetric matrix */
GEN
qfeval(GEN q, GEN x)
{
  long n=lg(q);

  if (n==1)
  {
    if (typ(q) != t_MAT || lg(x) != 1)
      err(talker,"invalid data in qfeval");
    return gzero;
  }
  if (typ(q) != t_MAT || lg(q[1]) != n)
    err(talker,"invalid quadratic form in qfeval");
  if (typ(x) != t_COL || lg(x) != n)
    err(talker,"invalid vector in qfeval");

  return qfeval0(q,x,n);
}

/* the Horner-type evaluation (mul x 2/3) would force us to use gmul and not
 * mulii (more than 4 x slower for small entries). Not worth it.
 */
static GEN
qfbeval0_i(GEN q, GEN x, GEN y, long n)
{
  long i, j;
  pari_sp av=avma;
  GEN res = gmul(gcoeff(q,1,1), mulii((GEN)x[1],(GEN)y[1]));

  for (i=2;i<n;i++)
  {
    if (!signe(x[i]))
    {
      if (!signe(y[i])) continue;
      for (j=1;j<i;j++)
        res = gadd(res, gmul(gcoeff(q,i,j), mulii((GEN)x[j],(GEN)y[i])));
    }
    else if (!signe(y[i]))
    {
      for (j=1;j<i;j++)
        res = gadd(res, gmul(gcoeff(q,i,j), mulii((GEN)x[i],(GEN)y[j])));
    }
    else
    {
      for (j=1;j<i;j++)
      {
        GEN p1 = addii(mulii((GEN)x[i],(GEN)y[j]), mulii((GEN)x[j],(GEN)y[i]));
        res = gadd(res, gmul(gcoeff(q,i,j),p1));
      }
      res = gadd(res, gmul(gcoeff(q,i,i), mulii((GEN)x[i],(GEN)y[i])));
    }
  }
  return gerepileupto(av,res);
}

#if 0
static GEN
qfbeval0(GEN q, GEN x, GEN y, long n)
{
  long i, j;
  pari_sp av=avma;
  GEN res = gmul(gcoeff(q,1,1), gmul((GEN)x[1],(GEN)y[1]));

  for (i=2;i<n;i++)
  {
    for (j=1;j<i;j++)
    {
      GEN p1 = gadd(gmul((GEN)x[i],(GEN)y[j]), gmul((GEN)x[j],(GEN)y[i]));
      res = gadd(res, gmul(gcoeff(q,i,j),p1));
    }
    res = gadd(res, gmul(gcoeff(q,i,i), gmul((GEN)x[i],(GEN)y[i])));
  }
  return gerepileupto(av,res);
}
#else
static GEN
qfbeval0(GEN q, GEN x, GEN y, long n)
{
  long i, j;
  pari_sp av=avma;
  GEN res = gmul(gcoeff(q,1,1), gmul((GEN)x[1], (GEN)y[1]));

  for (i=2; i<n; i++)
  {
    GEN l = (GEN)q[i];
    GEN sx = gmul((GEN)l[1], (GEN)y[1]);
    GEN sy = gmul((GEN)l[1], (GEN)x[1]);
    for (j=2; j<i; j++)
    {
      sx = gadd(sx, gmul((GEN)l[j],(GEN)y[j]));
      sy = gadd(sy, gmul((GEN)l[j],(GEN)x[j]));
    }
    sx = gadd(sx, gmul((GEN)l[i],(GEN)y[i]));

    sx = gmul((GEN)x[i], sx);
    sy = gmul((GEN)y[i], sy);
    res = gadd(res, gadd(sx,sy));
  }
  return gerepileupto(av,res);
}
#endif

/* We assume q is a real symetric matrix */
GEN
qfbeval(GEN q, GEN x, GEN y)
{
  long n=lg(q);

  if (n==1)
  {
    if (typ(q) != t_MAT || lg(x) != 1 || lg(y) != 1)
      err(talker,"invalid data in qfbeval");
    return gzero;
  }
  if (typ(q) != t_MAT || lg(q[1]) != n)
    err(talker,"invalid quadratic form in qfbeval");
  if (typ(x) != t_COL || lg(x) != n || typ(y) != t_COL || lg(y) != n)
    err(talker,"invalid vector in qfbeval");

  return qfbeval0(q,x,y,n);
}

/* yield X = M'.q.M, assuming q is symetric.
 * X_ij are X_ji identical, not copies
 * if flag is set, M has integer entries
 */
GEN
qf_base_change(GEN q, GEN M, int flag)
{
  long i,j, k = lg(M), n=lg(q);
  GEN res = cgetg(k,t_MAT);
  GEN (*qf)(GEN,GEN,long)  = flag? &qfeval0_i:  &qfeval0;
  GEN (*qfb)(GEN,GEN,GEN,long) = flag? &qfbeval0_i: &qfbeval0;

  if (n==1)
  {
    if (typ(q) != t_MAT || k != 1)
      err(talker,"invalid data in qf_base_change");
    return res;
  }
  if (typ(M) != t_MAT || k == 1 || lg(M[1]) != n)
    err(talker,"invalid base change matrix in qf_base_change");

  for (i=1;i<k;i++)
  {
    res[i] = lgetg(k,t_COL);
    coeff(res,i,i) = (long) qf(q,(GEN)M[i],n);
  }
  for (i=2;i<k;i++)
    for (j=1;j<i;j++)
      coeff(res,i,j)=coeff(res,j,i) = (long) qfb(q,(GEN)M[i],(GEN)M[j],n);
  return res;
}

/* return Re(x * y), x and y scalars */
GEN
mul_real(GEN x, GEN y)
{
  if (typ(x) == t_COMPLEX)
  {
    if (typ(y) == t_COMPLEX)
    {
      pari_sp av=avma, tetpil;
      GEN p1 = gmul((GEN)x[1], (GEN) y[1]);
      GEN p2 = gneg(gmul((GEN)x[2], (GEN) y[2]));
      tetpil=avma; return gerepile(av,tetpil,gadd(p1,p2));
    }
    x = (GEN)x[1];
  }
  else if (typ(y) == t_COMPLEX) y = (GEN)y[1];
  return gmul(x,y);
}

/* Compute Re(x * y), x and y matrices of compatible dimensions
 * assume lx, ly > 1, and scalar entries */
GEN
mulmat_real(GEN x, GEN y)
{
  long i, j, k, lx = lg(x), ly = lg(y), l = lg(x[1]);
  pari_sp av;
  GEN p1, z = cgetg(ly,t_MAT);

  for (j=1; j<ly; j++)
  {
    z[j] = lgetg(l,t_COL);
    for (i=1; i<l; i++)
    {
      p1 = gzero; av=avma;
      for (k=1; k<lx; k++)
        p1 = gadd(p1, mul_real(gcoeff(x,i,k),gcoeff(y,k,j)));
      coeff(z,i,j)=lpileupto(av, p1);
    }
  }
  return z;
}

static GEN
hqfeval0(GEN q, GEN x, long n)
{
  long i, j;
  pari_sp av=avma;
  GEN res=gzero;

  for (i=2;i<n;i++)
    for (j=1;j<i;j++)
    {
      GEN p1 = gmul((GEN)x[i],gconj((GEN)x[j]));
      res = gadd(res, mul_real(gcoeff(q,i,j),p1));
    }
  res=gshift(res,1);
  for (i=1;i<n;i++)
    res = gadd(res, gmul(gcoeff(q,i,i), gnorm((GEN)x[i])) );
  return gerepileupto(av,res);
}

/* We assume q is a hermitian complex matrix */
GEN
hqfeval(GEN q, GEN x)
{
  long n=lg(q);

  if (n==1)
  {
    if (typ(q) != t_MAT || lg(x) != 1)
      err(talker,"invalid data in hqfeval");
    return gzero;
  }
  if (typ(q) != t_MAT || lg(q[1]) != n)
    err(talker,"invalid quadratic form in hqfeval");
  if (typ(x) != t_COL || lg(x) != n)
    err(talker,"invalid vector in hqfeval");

  return hqfeval0(q,x,n);
}

GEN
poleval(GEN x, GEN y)
{
  long i, j, imin, tx = typ(x);
  pari_sp av0 = avma, av, lim;
  GEN p1, p2, r, s;

  if (is_scalar_t(tx)) return gcopy(x);
  switch(tx)
  {
    case t_POL:
      i = lg(x)-1; imin = 2; break;

    case t_RFRAC:
      p1 = poleval((GEN)x[1],y);
      p2 = poleval((GEN)x[2],y);
      return gerepileupto(av0, gdiv(p1,p2));

    case t_VEC: case t_COL:
      i = lg(x)-1; imin = 1; break;
    default: err(typeer,"poleval");
      return NULL; /* not reached */
  }
  if (i<=imin)
    return (i==imin)? gcopy((GEN)x[imin]): gzero;

  lim = stack_lim(av0,2);
  p1 = (GEN)x[i]; i--;
  if (typ(y)!=t_COMPLEX)
  {
#if 0 /* standard Horner's rule */
    for ( ; i>=imin; i--)
      p1 = gadd(gmul(p1,y),(GEN)x[i]);
#endif
    /* specific attention to sparse polynomials */
    for ( ; i>=imin; i=j-1)
    {
      for (j=i; isexactzero((GEN)x[j]); j--)
        if (j==imin)
        {
          if (i!=j) y = gpowgs(y, i-j+1);
          return gerepileupto(av0, gmul(p1,y));
        }
      r = (i==j)? y: gpowgs(y, i-j+1);
      p1 = gadd(gmul(p1,r), (GEN)x[j]);
      if (low_stack(lim, stack_lim(av0,2)))
      {
        if (DEBUGMEM>1) err(warnmem,"poleval: i = %ld",i);
        p1 = gerepileupto(av0, p1);
      }
    }
    return gerepileupto(av0,p1);
  }

  p2 = (GEN)x[i]; i--; r = gtrace(y); s = gneg_i(gnorm(y));
  av = avma;
  for ( ; i>=imin; i--)
  {
    GEN p3 = gadd(p2, gmul(r, p1));
    p2 = gadd((GEN)x[i], gmul(s, p1)); p1 = p3;
    if (low_stack(lim, stack_lim(av0,2)))
    {
      if (DEBUGMEM>1) err(warnmem,"poleval: i = %ld",i);
      gerepileall(av, 2, &p1, &p2);
    }
  }
  return gerepileupto(av0, gadd(p2, gmul(y,p1)));
}
