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
/**                        (second part)                           **/
/**                                                                **/
/********************************************************************/
#include "pari.h"
#include "paripriv.h"
/*******************************************************************/
/*                                                                 */
/*                OPERATIONS USING SMALL INTEGERS                  */
/*                                                                 */
/*******************************************************************/

GEN
gopsg2(GEN (*f)(GEN, GEN), long s, GEN y)
{
  long court_p[] = { evaltyp(t_INT) | _evallg(3),0,0 };
  affsi(s,court_p); return f(court_p,y);
}

void
gopsg2z(GEN (*f)(GEN, GEN), long s, GEN y, GEN z) {
  pari_sp av=avma; gaffect(f(stoi(s),y), z); avma=av;
}

/*******************************************************************/
/*                                                                 */
/*                    CREATION OF A P-ADIC GEN                     */
/*                                                                 */
/*******************************************************************/

/* y[4] not filled */
static GEN
cgetp2(GEN x, long vx)
{
  GEN y = cgetg(5,t_PADIC);
  y[1] = evalprecp(precp(x)) | evalvalp(vx);
  gel(y,2) = icopy(gel(x,2));
  gel(y,3) = icopy(gel(x,3)); return y;
}

GEN
cgetp(GEN x)
{
  GEN y = cgetp2(x, 0);
  gel(y,4) = cgeti(lgefint(x[3])); return y;
}

GEN
cgetimag(void) { GEN y = cgetg(3,t_COMPLEX); gel(y,1) = gen_0; return y; }
GEN
pureimag(GEN x) { return mkcomplex(gen_0, x); }

/*******************************************************************/
/*                                                                 */
/*                            SIZES                                */
/*                                                                 */
/*******************************************************************/

long
glength(GEN x)
{
  long tx = typ(x);
  switch(tx)
  {
    case t_INT:  return lgefint(x)-2;
    case t_LIST: return lg(list_data(x))-1;
    case t_REAL: return signe(x)? lg(x)-2: 0;
    case t_STR:  return strlen( GSTR(x) );
    case t_VECSMALL: return lg(x)-1;
  }
  return lg(x) - lontyp[tx];
}

GEN
matsize(GEN x)
{
  long L = lg(x) - 1;
  switch(typ(x))
  {
    case t_VEC: return mkvec2s(1, L);
    case t_COL: return mkvec2s(L, 1);
    case t_MAT: return mkvec2s(L? lg(x[1])-1: 0, L);
  }
  pari_err(typeer,"matsize");
  return NULL; /* not reached */
}

/*******************************************************************/
/*                                                                 */
/*                  Conversion t_POL --> t_SER                     */
/*                                                                 */
/*******************************************************************/
/* enlarge/truncate t_POL x to a t_SER with lg l */
GEN
greffe(GEN x, long l, long use_stack)
{
  long i, lx = lg(x);
  GEN y;

  if (typ(x)!=t_POL) pari_err(notpoler,"greffe");
  if (l <= 2) pari_err(talker, "l <= 2 in greffe");

  /* optimed version of polvaluation + normalize */
  i = 2; while (i<lx && isexactzero(gel(x,i))) i++;
  i -= 2; /* = polvaluation(x, NULL) */

  if (use_stack) y = cgetg(l,t_SER);
  else
  {
    y = (GEN) gpmalloc(l*sizeof(long));
    y[0] = evaltyp(t_SER) | evallg(l);
  }
  y[1] = x[1]; setvalp(y, i);
  x += i; lx -= i;
  if (lx > l) {
    for (i = 2; i < l; i++) gel(y,i) = gel(x,i);
  } else {
    for (i = 2; i <lx; i++) gel(y,i) = gel(x,i);
    for (     ; i < l; i++) gel(y,i) = gen_0;
  }
  return y;
}

/*******************************************************************/
/*                                                                 */
/*                 CONVERSION GEN --> long                         */
/*                                                                 */
/*******************************************************************/

long
gtolong(GEN x)
{
  switch(typ(x))
  {
    case t_INT:
      return itos(x);
    case t_REAL:
      return (long)(rtodbl(x) + 0.5);
    case t_FRAC: {
      pari_sp av = avma;
      long y = itos(ground(x));
      avma = av; return y;
    }
    case t_COMPLEX:
      if (gcmp0(gel(x,2))) return gtolong(gel(x,1)); break;
    case t_QUAD:
      if (gcmp0(gel(x,3))) return gtolong(gel(x,2)); break;
  }
  pari_err(typeer,"gtolong");
  return 0; /* not reached */
}

/*******************************************************************/
/*                                                                 */
/*                         COMPARISONS                             */
/*                                                                 */
/*******************************************************************/

/* returns 1 whenever x = 0, and 0 otherwise */
int
gcmp0(GEN x)
{
  switch(typ(x))
  {
    case t_INT: case t_REAL: case t_POL: case t_SER:
      return !signe(x);

    case t_INTMOD: case t_POLMOD:
      return gcmp0(gel(x,2));

    case t_FFELT:
      return FF_cmp0(x);

    case t_FRAC:
      return 0;

    case t_COMPLEX:
     /* is 0 iff norm(x) would be 0 (can happen with Re(x) and Im(x) != 0
      * only if Re(x) and Im(x) are of type t_REAL). See mp.c:addrr().
      */
      if (gcmp0(gel(x,1)))
      {
	if (gcmp0(gel(x,2))) return 1;
	if (typ(x[1])!=t_REAL || typ(x[2])!=t_REAL) return 0;
	return (expo(x[1])>expo(x[2]));
      }
      if (gcmp0(gel(x,2)))
      {
	if (typ(x[1])!=t_REAL || typ(x[2])!=t_REAL) return 0;
	return (expo(x[2])>expo(x[1]));
      }
      return 0;

    case t_PADIC:
      return !signe(x[4]);

    case t_QUAD:
      return gcmp0(gel(x,2)) && gcmp0(gel(x,3));

    case t_RFRAC:
      return gcmp0(gel(x,1));

    case t_VEC: case t_COL: case t_MAT:
    {
      long i;
      for (i=lg(x)-1; i; i--)
        if (!gcmp0(gel(x,i))) return 0;
      return 1;
    }
  }
  return 0;
}

/* assume x != 0, is |x| == 2^n ? */
int
absrnz_egal2n(GEN x) {
  if ((ulong)x[2]==HIGHBIT)
  {
    long i, lx = lg(x);
    for (i = 3; i < lx; i++)
      if (x[i]) return 0;
    return 1;
  }
  return 0;
}
/* assume x != 0, is |x| == 1 ? */
int
absrnz_egal1(GEN x) { return !expo(x) && absrnz_egal2n(x); }

/* returns 1 whenever x = 1, 0 otherwise */
int
gcmp1(GEN x)
{
  switch(typ(x))
  {
    case t_INT:
      return is_pm1(x) && signe(x)==1;

    case t_REAL:
      return signe(x) > 0 ? absrnz_egal1(x): 0;

    case t_INTMOD: case t_POLMOD: 
      return gcmp1(gel(x,2));
  
    case t_FFELT:
      return FF_cmp1(x);

    case t_FRAC:
      return 0;

    case t_COMPLEX:
      return gcmp1(gel(x,1)) && gcmp0(gel(x,2));

    case t_PADIC:
      return !valp(x) && gcmp1(gel(x,4));

    case t_QUAD:
      return gcmp1(gel(x,2)) && gcmp0(gel(x,3));

    case t_POL:
      return lg(x)==3 && gcmp1(gel(x,2));
  }
  return 0;
}

/* returns 1 whenever the x = -1, 0 otherwise */
int
gcmp_1(GEN x)
{
  pari_sp av;
  long y;
  GEN p1;

  switch(typ(x))
  {
    case t_INT:
      return is_pm1(x) && signe(x)== -1;

    case t_REAL:
      return signe(x) < 0 ? absrnz_egal1(x): 0;

    case t_INTMOD:
      av=avma; y=equalii(addsi(1,gel(x,2)), gel(x,1)); avma=av; return y;

    case t_FRAC:
      return 0;
      
    case t_FFELT:
      return FF_cmp_1(x);

    case t_COMPLEX:
      return gcmp_1(gel(x,1)) && gcmp0(gel(x,2));

    case t_QUAD:
      return gcmp_1(gel(x,2)) && gcmp0(gel(x,3));

    case t_PADIC:
      av=avma; y=gequal(addsi(1,gel(x,4)), gel(x,3)); avma=av; return y;

    case t_POLMOD:
      av=avma; p1=gadd(gen_1,gel(x,2));
      y = signe(p1) && !gequal(p1,gel(x,1)); avma=av; return !y;

    case t_POL:
      return lg(x)==3 && gcmp_1(gel(x,2));
  }
  return 0;
}

/* returns the sign of x - y when it makes sense. 0 otherwise */
int
gcmp(GEN x, GEN y)
{
  long tx = typ(x), ty = typ(y), f;
  pari_sp av;

  if (is_intreal_t(tx))
    { if (is_intreal_t(ty)) return mpcmp(x,y); }
  else
  {
    if (tx==t_STR)
    {
      if (ty != t_STR) return 1;
      f = strcmp(GSTR(x),GSTR(y));
      return f > 0? 1
                  : f? -1: 0;
    }
    if (tx != t_FRAC)
    {
      if (ty == t_STR) return -1;
      pari_err(typeer,"comparison");
    }
  }
  if (ty == t_STR) return -1;
  if (!is_intreal_t(ty) && ty != t_FRAC) pari_err(typeer,"comparison");
  av=avma; f = gsigne( gadd(x,gneg_i(y)) ); avma=av; return f;
}

int
gcmpsg(long s, GEN y)
{
  long ty = typ(y);
  switch(ty) {
    case t_INT:  return cmpsi(s,y);
    case t_REAL: return cmpsr(s,y);
    case t_FRAC: {
      pari_sp av = avma;
      GEN n = gel(y,1), d = gel(y,2);
      int f = cmpii(mulsi(s,d), n); avma = av; return f;
    }
    case t_STR: return -1;
  }
  pari_err(typeer,"comparison");
  return 0; /* not reached */
}

static int
lexcmp_scal_vec(GEN x, GEN y)
{
  long fl;
  if (lg(y)==1) return 1;
  fl = lexcmp(x,gel(y,1));
  if (fl) return fl;
  return -1;
}

static int
lexcmp_vec_mat(GEN x, GEN y)
{
  if (lg(x)==1) return -1;
  return lexcmp_scal_vec(x,y);
}

/* as gcmp for vector/matrices, using lexicographic ordering on components */
int
lexcmp(GEN x, GEN y)
{
  const long tx=typ(x), ty=typ(y);
  long lx,ly,l,fl,i;

  if (!is_matvec_t(tx))
  {
    if (!is_matvec_t(ty)) return gcmp(x,y);
    return  lexcmp_scal_vec(x,y);
  }
  if (!is_matvec_t(ty))
    return -lexcmp_scal_vec(y,x);

  /* x and y are matvec_t */
  if (ty==t_MAT)
  {
    if (tx != t_MAT)
      return lexcmp_vec_mat(x,y);
  }
  else if (tx==t_MAT)
    return -lexcmp_vec_mat(y,x);

  /* tx = ty = t_MAT, or x and y are both vect_t */
  lx = lg(x);
  ly = lg(y); l = min(lx,ly);
  for (i=1; i<l; i++)
  {
    fl = lexcmp(gel(x,i),gel(y,i));
    if (fl) return fl;
  }
  if (lx == ly) return 0;
  return (lx < ly)? -1 : 1;
}

/*****************************************************************/
/*                                                               */
/*                          EQUALITY                             */
/*                returns 1 if x == y, 0 otherwise               */
/*                                                               */
/*****************************************************************/
#define MASK(x) (((ulong)(x)) & (TYPBITS | LGBITS))

/* x,y t_POL */
static int
polegal(GEN x, GEN y)
{
  long i, lx;

  while (lg(x) == 3) { x = gel(x,2); if (typ(x) != t_POL) break; }
  while (lg(y) == 3) { y = gel(y,2); if (typ(y) != t_POL) break; }
  if (MASK(x[0]) != MASK(y[0]))
    return (typ(x) == t_POL || typ(y) == t_POL)? 0: gequal(x, y);
  /* same typ, lg */
  if (typ(x) != t_POL) return gequal(x, y);
  /* typ = t_POL, non-constant or 0 */
  lx = lg(x); if (lx == 2) return 1; /* 0 in two different vars are = */
  if (x[1] != y[1]) return 0;
  for (i = 2; i < lx; i++)
    if (!gequal(gel(x,i),gel(y,i))) return 0;
  return 1;
}

/* typ(x) = typ(y) = t_VEC/COL/MAT */
static int
vecegal(GEN x, GEN y)
{
  long i;
  if (MASK(x[0]) != MASK(y[0])) return 0;

  for (i = lg(x)-1; i; i--)
    if (! gequal(gel(x,i),gel(y,i)) ) return 0;
  return 1;
}

static int
gegal_try(GEN x, GEN y)
{
  int i;
  CATCH(CATCH_ALL) {
    CATCH_RELEASE(); return 0;
  } TRY {
    i = gcmp0(gadd(x, gneg_i(y)));
  } ENDCATCH;
  return i;
}

int
gequal(GEN x, GEN y)
{
  pari_sp av;
  long tx;
  long i;

  if (x == y) return 1;
  tx = typ(x);
  if (tx==typ(y))
    switch(tx)
    {
      case t_INT:
        return equalii(x,y);

      case t_REAL:
        return cmprr(x,y) == 0;

      case t_FRAC: case t_INTMOD:
	return equalii(gel(x,2), gel(y,2)) && equalii(gel(x,1), gel(y,1));
      
      case t_COMPLEX: case t_POLMOD: 
	return gequal(gel(x,2),gel(y,2)) && gequal(gel(x,1),gel(y,1));

      case t_POL:
        return polegal(x,y);

      case t_FFELT:
        return FF_equal(x,y);

      case t_QFR:
	    if (!gequal(gel(x,4),gel(y,4))) return 0; /* fall through */
      case t_QFI:
	return equalii(gel(x,1),gel(y,1))
	    && equalii(gel(x,2),gel(y,2))
	    && equalii(gel(x,3),gel(y,3));

      case t_QUAD: 
	return gequal(gel(x,1),gel(y,1))
	    && gequal(gel(x,2),gel(y,2))
	    && gequal(gel(x,3),gel(y,3));

      case t_RFRAC:
	av=avma; i=gequal(gmul(gel(x,1),gel(y,2)),gmul(gel(x,2),gel(y,1)));
	avma=av; return i;

      case t_STR:
        return !strcmp(GSTR(x),GSTR(y));

      case t_VEC: case t_COL: case t_MAT:
        return vecegal(x,y);
      case t_VECSMALL:
        if (MASK(x[0]) != MASK(y[0])) return 0;
        for (i = lg(x)-1; i; i--)
          if (x[i] != y[i]) return 0;
        return 1;
    }
  (void)&av; /* emulate volatile */
  av = avma; i = gegal_try(x, y);
  avma = av; return i;
}
#undef MASK

int
gequalsg(long s, GEN x)
{
  pari_sp av = avma;
  int f = gequal(stoi(s), x);
  avma = av; return f;
}
/*******************************************************************/
/*                                                                 */
/*                          VALUATION                              */
/*             p is either a t_INT or a t_POL.                     */
/*  returns the largest exponent of p dividing x when this makes   */
/*  sense : error for types real, integermod and polymod if p does */
/*  not divide the modulus, q-adic if q!=p.                        */
/*                                                                 */
/*******************************************************************/

static long
minval(GEN x, GEN p, long first, long lx)
{
  long i,k, val = VERYBIGINT;
  for (i=first; i<lx; i++)
  {
    k = ggval(gel(x,i),p);
    if (k < val) val = k;
  }
  return val;
}

long
polvaluation(GEN x, GEN *Z)
{
  long vx;
  if (lg(x) == 2) { if (Z) *Z = zeropol(varn(x)); return VERYBIGINT; }
  for (vx = 0;; vx++)
    if (!isexactzero(gel(x,2+vx))) break;
  if (Z) *Z = RgX_shift_shallow(x, -vx);
  return vx;
}
long
ZX_valuation(GEN x, GEN *Z)
{
  long vx;
  if (!signe(x)) { if (Z) *Z = zeropol(varn(x)); return VERYBIGINT; }
  for (vx = 0;; vx++)
    if (signe(gel(x,2+vx))) break;
  if (Z) *Z = RgX_shift_shallow(x, -vx);
  return vx;
}
long
polvaluation_inexact(GEN x, GEN *Z)
{
  long vx;
  if (!signe(x)) { if (Z) *Z = zeropol(varn(x)); return VERYBIGINT; }
  for (vx = 0;; vx++)
    if (!gcmp0(gel(x,2+vx))) break;
  if (Z) *Z = RgX_shift_shallow(x, -vx);
  return vx;
}
static int
intdvd(GEN x, GEN y, GEN *z) {
  GEN r, q = dvmdii(x,y,&r);
  if (r != gen_0) return 0; 
  *z = q; return 1;
}

/* x t_FRAC, p t_INT, return v_p(x) */
static long
ratval(GEN x, GEN p) { return Z_pval(gel(x,1),p) - Z_pval(gel(x,2),p); }

long
Q_pval(GEN x, GEN p) { return (typ(x) == t_INT)? Z_pval(x, p): ratval(x, p); }

long
ggval(GEN x, GEN p)
{
  long tx = typ(x), tp = typ(p), vx, vp, i, val;
  pari_sp av, limit;
  GEN p1, p2;

  if (isexactzero(x)) return VERYBIGINT;
  if (is_const_t(tx) && tp==t_POL) return 0;
  if (tp == t_INT && (!signe(p) || is_pm1(p)))
    pari_err(talker, "forbidden divisor %Z in ggval", p);
  switch(tx)
  {
    case t_INT:
      if (tp != t_INT) break;
      return Z_pval(x,p);

    case t_INTMOD:
      if (tp != t_INT) break;
      av = avma;
      if (!intdvd(gel(x,1),p, &p1)) break;
      if (!intdvd(gel(x,2),p, &p2)) { avma = av; return 0; }
      val = 1; while (intdvd(p1,p,&p1) && intdvd(p2,p,&p2)) val++;
      avma = av; return val;

    case t_FRAC:
      if (tp != t_INT) break;
      return ratval(x, p);

    case t_PADIC:
      if (!gequal(p,gel(x,2))) break;
      return valp(x);

    case t_POLMOD:
      if (tp == t_INT) return ggval(gel(x,2),p);
      if (tp != t_POL) break;
      if (varn(x[1]) != varn(p)) return 0;
      av = avma;
      if (!poldvd(gel(x,1),p,&p1)) break;
      if (!poldvd(gel(x,2),p,&p2)) { avma = av; return 0; }
      val = 1; while (poldvd(p1,p,&p1) && poldvd(p2,p,&p2)) val++;
      avma = av; return val;

    case t_POL:
      if (tp==t_POL)
      {
        if (degpol(p) <= 0)
          pari_err(talker, "forbidden divisor %Z in ggval", p);
	vp = varn(p);
        vx = varn(x);
	if (vp == vx)
	{
	  if (ismonome(p)) return polvaluation(x, NULL) / degpol(p);
	  av = avma; limit=stack_lim(av,1);
	  for (val=0; ; val++)
	  {
	    if (!poldvd(x,p,&x)) break;
            if (low_stack(limit, stack_lim(av,1)))
	    {
	      if(DEBUGMEM>1) pari_warn(warnmem,"ggval");
	      x = gerepilecopy(av, x);
	    }
	  }
	  avma = av; return val;
	}
	if (varncmp(vx, vp) > 0) return 0;
      }
      else
        if (tp != t_INT) break;
      i=2; while (isexactzero(gel(x,i))) i++;
      return minval(x,p,i,lg(x));

    case t_SER:
      if (tp!=t_POL && tp!=t_SER && tp!=t_INT) break;
      vp = gvar(p);
      vx = varn(x);
      if (vp == vx) {
        vp = polvaluation(p, NULL);
        if (!vp) pari_err(talker, "forbidden divisor %Z in ggval", p);
        return (long)(valp(x) / vp);
      }
      if (varncmp(vx, vp) > 0) return 0;
      return minval(x,p,2,lg(x));

    case t_RFRAC:
      return ggval(gel(x,1),p) - ggval(gel(x,2),p);

    case t_COMPLEX: case t_QUAD: case t_VEC: case t_COL: case t_MAT:
      return minval(x,p,1,lg(x));
  }
  pari_err(talker,"forbidden or conflicting type in gval");
  return 0; /* not reached */
}

/* x is non-zero */
long
u_lvalrem(ulong x, ulong p, ulong *py)
{
  ulong vx;
  if (p == 2) { vx = vals(x); *py = x >> vx; return vx; }
  for(vx = 0;;)
  {
    if (x % p) { *py = x; return vx; }
    x /= p; /* gcc is smart enough to make a single div */
    vx++;
  }
}
long
u_lval(ulong x, ulong p)
{
  ulong vx;
  if (p == 2) return vals(x);
  for(vx = 0;;)
  {
    if (x % p) return vx;
    x /= p; /* gcc is smart enough to make a single div */
    vx++;
  }
}

/* assume |p| > 1 */
long
z_pval(long n, GEN p)
{
  if (lgefint(p) > 3) return 0;
  return u_lval((ulong)labs(n), (ulong)p[2]);
}

/* return v_q(x) and set *py = x / q^v_q(x), using divide & conquer */
static long 
Z_pvalrem_DC(GEN x, GEN q, GEN *py)
{
  GEN r, z = dvmdii(x, q, &r);
  long v;
  if (r != gen_0) { *py = x; return 0; }
  if (2 * lgefint(q) <= lgefint(z)+3) /* avoid squaring if pointless */
    v = Z_pvalrem_DC(z, sqri(q), py) << 1;
  else 
  { v = 0; *py = z; }
  z = dvmdii(*py, q, &r);
  if (r != gen_0) return v + 1;
  *py = z; return v + 2;
}

long
Z_lval(GEN x, ulong p)
{
  long vx;
  pari_sp av;
  if (p == 2) return vali(x);
  if (lgefint(x) == 3) return u_lval((ulong)x[2], p);
  av = avma;
  for(vx = 0;;)
  {
    ulong r;
    GEN q = diviu_rem(x, p, &r);
    if (r) break;
    vx++; x = q;
    if (vx == 32) {
      if (p == 1) pari_err(talker, "p = 1 in Z_lvalrem");
      vx = 32 + Z_pvalrem_DC(x, utoipos(p), &x); break;
    }
  }
  avma = av; return vx;
}
long
Z_lvalrem(GEN x, ulong p, GEN *py)
{
  long vx, sx;
  pari_sp av;
  if (p == 2) { vx = vali(x); *py = shifti(x, -vx); return vx; }
  if (lgefint(x) == 3) {
    ulong u;
    vx = u_lvalrem((ulong)x[2], p, &u);
    *py = signe(x) < 0? utoineg(u): utoipos(u);
    return vx;
  }
  av = avma; (void)new_chunk(lgefint(x));
  sx = signe(x);
  for(vx = 0;;)
  {
    ulong r;
    GEN q = diviu_rem(x, p, &r);
    if (r) break;
    vx++; x = q;
    if (vx == 32) {
      if (p == 1) pari_err(talker, "p = 1 in Z_lvalrem");
      vx = 32 + Z_pvalrem_DC(x, utoipos(p), &x); break;
    }
  }
  avma = av; *py = icopy(x); setsigne(*py, sx); return vx;
}

/* Is |q| <= p ? */
static int
isless_iu(GEN q, ulong p) {
  long l = lgefint(q);
  return l==2 || (l == 3 && (ulong)q[2] <= p);
}

long
u_lvalrem_stop(ulong *n, ulong p, int *stop)
{
  ulong N = *n, q = N / p, r = N % p; /* gcc makes a single div */
  long v = 0;
  if (!r)
  {
    do { v++; N = q; q = N / p; r = N % p; } while (!r);
    *n = N;
  }
  *stop = q <= p; return v;
}
/* Assume n > 0. Return v_p(n), assign n := n/p^v_p(n). Set 'stop' if now
 * n < p^2 [implies n prime if no prime < p divides n] */
long
Z_lvalrem_stop(GEN n, ulong p, int *stop)
{
  long v = 0;
  if (lgefint(n) == 3)
  {
    ulong N = (ulong)n[2], q = N / p, r = N % p; /* gcc makes a single div */
    if (!r)
    {
      do { v++; N = q; q = N / p; r = N % p; } while (!r);
      affui(N, n);
    }
    *stop = q <= p;
  }
  else
  {
    pari_sp av = avma;
    ulong r;
    GEN N, q = diviu_rem(n, p, &r);
    if (!r)
    {
      do { 
        v++; N = q;
        if (v == 32) { v = 32 + Z_pvalrem_DC(N, utoipos(p), &N); break; }
        q = diviu_rem(N, p, &r);
      } while (!r);
      affii(N, n);
    }
    *stop = isless_iu(q,p); avma = av;
  }
  return v;
}

/* x is a non-zero integer, |p| > 1 */
long
Z_pvalrem(GEN x, GEN p, GEN *py)
{
  long vx;
  pari_sp av;

  if (lgefint(p) == 3) return Z_lvalrem(x, (ulong)p[2], py);
  if (lgefint(x) == 3) { *py = icopy(x); return 0; }
  av = avma; vx = 0; (void)new_chunk(lgefint(x));
  for(;;)
  {
    GEN r, q = dvmdii(x,p,&r);
    if (r != gen_0) { avma = av; *py = icopy(x); return vx; }
    vx++; x = q;
  }
}
long
u_pvalrem(ulong x, GEN p, ulong *py)
{
  if (lgefint(p) == 3) return u_lvalrem(x, (ulong)p[2], py);
  *py = x; return 0;
}
long
Z_pval(GEN x, GEN p) { 
  long vx;
  pari_sp av;

  if (lgefint(p) == 3) return Z_lval(x, (ulong)p[2]);
  if (lgefint(x) == 3) return 0;
  av = avma; vx = 0;
  for(;;)
  {
    GEN r, q = dvmdii(x,p,&r);
    if (r != gen_0) { avma = av; return vx; }
    vx++; x = q;
  }
}

/*******************************************************************/
/*                                                                 */
/*                       NEGATION: Create -x                       */
/*                                                                 */
/*******************************************************************/

GEN
gneg(GEN x)
{
  long tx=typ(x),lx,i;
  GEN y;

  switch(tx)
  {
    case t_INT:
      return signe(x)? negi(x): gen_0;
    case t_REAL:
      return mpneg(x);

    case t_COMPLEX:
      y=cgetg(3, t_COMPLEX);
      gel(y,1) = gneg(gel(x,1));
      gel(y,2) = gneg(gel(x,2));
      break;

    case t_INTMOD: y=cgetg(3,t_INTMOD);
      gel(y,1) = icopy(gel(x,1));
      gel(y,2) = subii(gel(y,1),gel(x,2));
      break;

    case t_POLMOD: y=cgetg(3,t_POLMOD);
      gel(y,1) = gcopy(gel(x,1));
      gel(y,2) = gneg(gel(x,2)); break;

    case t_FRAC: case t_RFRAC:
      y=cgetg(3,tx);
      gel(y,1) = gneg(gel(x,1));
      gel(y,2) = gcopy(gel(x,2)); break;

    case t_PADIC:
      y=cgetp2(x,valp(x));
      gel(y,4) = subii(gel(x,3),gel(x,4));
      break;

    case t_QUAD:
      y=cgetg(4,t_QUAD);
      gel(y,1) = gcopy(gel(x,1));
      gel(y,2) = gneg(gel(x,2));
      gel(y,3) = gneg(gel(x,3)); break;

    case t_FFELT: return FF_neg(x);

    case t_POL: case t_SER:
    case t_VEC: case t_COL: case t_MAT:
      y = init_gen_op(x, tx, &lx, &i);
      for (; i<lx; i++) gel(y,i) = gneg(gel(x,i));
      break;

    default:
      pari_err(typeer,"negation");
      return NULL; /* not reached */
  }
  return y;
}

GEN
gneg_i(GEN x)
{
  long tx=typ(x),lx,i;
  GEN y;

  switch(tx)
  {
    case t_INT:
      return signe(x)? negi(x): gen_0;
    case t_REAL:
      return mpneg(x);

    case t_INTMOD: y=cgetg(3,t_INTMOD); y[1]=x[1];
      gel(y,2) = subii(gel(y,1),gel(x,2));
      break;

    case t_FRAC: case t_RFRAC:
      y=cgetg(3,tx); y[2]=x[2];
      gel(y,1) = gneg_i(gel(x,1)); break;

    case t_PADIC: y = cgetg(5,t_PADIC); y[2]=x[2]; y[3]=x[3];
      y[1] = evalprecp(precp(x)) | evalvalp(valp(x));
      gel(y,4) = subii(gel(x,3),gel(x,4)); break;

    case t_POLMOD: y=cgetg(3,t_POLMOD); y[1]=x[1];
      gel(y,2) = gneg_i(gel(x,2)); break;

    case t_FFELT: return FF_neg_i(x);

    case t_QUAD: y=cgetg(4,t_QUAD); y[1]=x[1];
      gel(y,2) = gneg_i(gel(x,2));
      gel(y,3) = gneg_i(gel(x,3)); break;

    case t_COMPLEX: case t_VEC: case t_COL: case t_MAT:
      lx=lg(x); y=cgetg(lx,tx);
      for (i=1; i<lx; i++) gel(y,i) = gneg_i(gel(x,i));
      break;

    case t_POL: case t_SER:
      lx=lg(x); y=cgetg(lx,tx); y[1]=x[1];
      for (i=2; i<lx; i++) gel(y,i) = gneg_i(gel(x,i));
      break;

    default:
      pari_err(typeer,"negation");
      return NULL; /* not reached */
  }
  return y;
}

/******************************************************************/
/*                                                                */
/*                       ABSOLUTE VALUE                           */
/*    Create abs(x) if x is integer, real, fraction or complex.   */
/*                       Error otherwise.                         */
/*                                                                */
/******************************************************************/

static int
is_negative(GEN x) {
  switch(typ(x))
  {
    case t_INT: case t_REAL: case t_FRAC:
      if (gsigne(x) < 0) return 1;
  }
  return 0;
}

static GEN
absq(GEN x)
{
  GEN y = cgetg(3, t_FRAC);
  gel(y,1) = absi(gel(x,1));
  gel(y,2) = icopy(gel(x,2)); return y;
}
GEN
Q_abs(GEN x)
{
  return (typ(x) == t_INT)? absi(x): absq(x);
}

GEN
gabs(GEN x, long prec)
{
  long tx=typ(x), lx, i;
  pari_sp av, tetpil;
  GEN y,p1;

  switch(tx)
  {
    case t_INT: case t_REAL:
      return mpabs(x);

    case t_FRAC: 
      return absq(x);

    case t_COMPLEX:
      av=avma; p1=cxnorm(x);
      switch(typ(p1))
      {
        case t_INT:
          if (!Z_issquarerem(p1, &y)) break;
          return gerepileupto(av, y);
        case t_FRAC: {
          GEN a,b;
          if (!Z_issquarerem(gel(p1,1), &a)) break;
          if (!Z_issquarerem(gel(p1,2), &b)) break;
          return gerepileupto(av, gdiv(a,b));
        }
      }
      tetpil=avma;
      return gerepile(av,tetpil,gsqrt(p1,prec));

    case t_QUAD:
      av = avma;
      return gerepileuptoleaf(av, gabs(quadtoc(x, prec), prec));

    case t_POL:
      lx = lg(x); if (lx<=2) return gcopy(x);
      return is_negative(gel(x,lx-1))? gneg(x): gcopy(x);

    case t_SER:
     if (valp(x) || !signe(x))
       pari_err(talker, "abs is not meromorphic at 0");
     return is_negative(gel(x,2))? gneg(x): gcopy(x);

    case t_VEC: case t_COL: case t_MAT:
      lx = lg(x); y = cgetg(lx,tx);
      for (i=1; i<lx; i++) gel(y,i) = gabs(gel(x,i),prec);
      return y;
  }
  pari_err(typeer,"gabs");
  return NULL; /* not reached */
}

GEN
gmax(GEN x, GEN y) { return gcopy(gcmp(x,y)<0? y: x); }
GEN
gmaxgs(GEN x, long s) { return (gcmpsg(s,x)>=0)? stoi(s): gcopy(x); }

GEN
gmin(GEN x, GEN y) { return gcopy(gcmp(x,y)<0? x: y); }
GEN
gmings(GEN x, long s) { return (gcmpsg(s,x)>0)? gcopy(x): stoi(s); }

GEN
vecmax(GEN x)
{
  long lx = lg(x), i, j;
  GEN s;

  if (lx==1) pari_err(talker, "empty vector in vecmax");
  switch(typ(x))
  {
    case t_VEC: case t_COL:
      s = gel(x,1);
      for (i=2; i<lx; i++)
        if (gcmp(gel(x,i),s) > 0) s = gel(x,i);
      return gcopy(s);
    case t_MAT: {
      long lx2 = lg(x[1]);
      if (lx2==1) pari_err(talker, "empty vector in vecmax");
      s = gcoeff(x,1,1); i = 2;
      for (j=1; j<lx; j++,i=1)
      {
        GEN c = gel(x,j);
        for (; i<lx2; i++)
          if (gcmp(gel(c,i),s) > 0) s = gel(c,i);
      }
      return gcopy(s);
    }
    case t_VECSMALL: {
      long t = x[1];
      for (i=2; i<lx; i++)
        if (x[i] > t) t = x[i];
      return stoi(t);
    }
    default: return gcopy(x);
  }
}

GEN
vecmin(GEN x)
{
  long lx = lg(x), i, j;
  GEN s;

  if (lx==1) pari_err(talker, "empty vector in vecmin");
  switch(typ(x))
  {
    case t_VEC: case t_COL:
      s = gel(x,1);
      for (i=2; i<lx; i++)
        if (gcmp(gel(x,i),s) < 0) s = gel(x,i);
      return gcopy(s);
    case t_MAT: {
      long lx2 = lg(x[1]);
      if (lx2==1) pari_err(talker, "empty vector in vecmin");
      s = gcoeff(x,1,1); i = 2;
      for (j=1; j<lx; j++,i=1)
      {
        GEN c = gel(x,j);
        for (; i<lx2; i++)
          if (gcmp(gel(c,i),s) < 0) s = gel(c,i);
      }
      return gcopy(s);
    }
    case t_VECSMALL: {
      long t = x[1];
      for (i=2; i<lx; i++)
        if (x[i] < t) t = x[i];
      return stoi(t);
    }
    default: return gcopy(x);
  }
}

/*******************************************************************/
/*                                                                 */
/*                      AFFECT long --> GEN                        */
/*         affect long s to GEN x. Useful for initialization.      */
/*                                                                 */
/*******************************************************************/

static void
padicaff0(GEN x)
{
  if (signe(x[4]))
  {
    setvalp(x, valp(x)|precp(x));
    affsi(0,gel(x,4));
  }
}

void
gaffsg(long s, GEN x)
{
  switch(typ(x))
  {
    case t_INT: affsi(s,x); break;
    case t_REAL: affsr(s,x); break;
    case t_INTMOD: modsiz(s,gel(x,1),gel(x,2)); break;
    case t_FRAC: affsi(s,gel(x,1)); affsi(1,gel(x,2)); break;
    case t_COMPLEX: gaffsg(s,gel(x,1)); gaffsg(0,gel(x,2)); break;
    case t_PADIC: {
      long vx;
      GEN y;
      if (!s) { padicaff0(x); break; }
      vx = Z_pvalrem(stoi(s), gel(x,2), &y);
      setvalp(x,vx); modiiz(y,gel(x,3),gel(x,4));
      break;
    }
    case t_QUAD: gaffsg(s,gel(x,2)); gaffsg(0,gel(x,3)); break;
    default: pari_err(operf,"",stoi(s),x);
  }
}

/*******************************************************************/
/*                                                                 */
/*                     GENERIC AFFECTATION                         */
/*         Affect the content of x to y, whenever possible         */
/*                                                                 */
/*******************************************************************/
/* x PADIC, Y INT, return lift(x * Mod(1,Y)) */
GEN
padic_to_Fp(GEN x, GEN Y) {
  GEN z;
  long vy, vx = valp(x);
  if (!signe(Y)) pari_err(gdiver);
  vy = Z_pvalrem(Y,gel(x,2), &z);
  if (vx < 0 || !gcmp1(z)) pari_err(operi,"",x, mkintmod(gen_1,Y));
  if (vx >= vy) return gen_0;
  z = gel(x,4);
  if (!signe(z) || vy > vx + precp(x)) pari_err(operi,"",x, mkintmod(gen_1,Y));
  if (vx) z = mulii(z, powiu(gel(x,2),vx));
  return remii(z, Y);
}
ulong
padic_to_Fl(GEN x, ulong Y) {
  ulong uz;
  GEN z;
  long vy, vx = valp(x);
  vy = u_pvalrem(Y,gel(x,2), &uz);
  if (vx < 0 || uz != 1) pari_err(operi,"",x, mkintmodu(1,Y));
  if (vx >= vy) return 0;
  z = gel(x,4);
  if (!signe(z) || vy > vx + precp(x)) pari_err(operi,"",x, mkintmodu(1,Y));
  if (vx) z = mulii(z, powiu(gel(x,2),vx));
  return umodiu(z, Y);
}

static void
croak(char *s) {
  pari_err(talker,"trying to overwrite a universal object in gaffect (%s)", s);
}

void
gaffect(GEN x, GEN y)
{
  long vx, i, lx, ly, tx = typ(x), ty = typ(y);
  pari_sp av;
  GEN p1, num, den;

  if (tx == ty) switch(tx) {
    case t_INT:
      if (!is_universal_constant(y)) { affii(x,y); return; }
      /* y = gen_0, gnil, gen_1 or gen_2 */
      if (y==gen_0)  croak("gen_0");
      if (y==gen_1)  croak("gen_1)");
      if (y==gen_m1) croak("gen_m1)");
      if (y==gen_2)  croak("gen_2)");
      croak("gnil)");
    case t_REAL: affrr(x,y); return;
    case t_INTMOD:
      if (!dvdii(gel(x,1),gel(y,1))) pari_err(operi,"",x,y);
      modiiz(gel(x,2),gel(y,1),gel(y,2)); return;
    case t_FRAC:
      affii(gel(x,1),gel(y,1));
      affii(gel(x,2),gel(y,2)); return;
    case t_COMPLEX:
      gaffect(gel(x,1),gel(y,1));
      gaffect(gel(x,2),gel(y,2)); return;
    case t_PADIC:
      if (!equalii(gel(x,2),gel(y,2))) pari_err(operi,"",x,y);
      modiiz(gel(x,4),gel(y,3),gel(y,4));
      setvalp(y,valp(x)); return;
    case t_QUAD:
      if (! gequal(gel(x,1),gel(y,1))) pari_err(operi,"",x,y);
      affii(gel(x,2),gel(y,2));
      affii(gel(x,3),gel(y,3)); return;
    case t_VEC: case t_COL: case t_MAT:
      lx = lg(x); if (lx != lg(y)) pari_err(operi,"",x,y);
      for (i=1; i<lx; i++) gaffect(gel(x,i),gel(y,i));
      return;
  }

  /* Various conversions. Avoid them, use specialized routines ! */

  if (!is_const_t(ty)) pari_err(operf,"",x,y);
  switch(tx)
  {
    case t_INT:
      switch(ty)
      {
        case t_REAL:
          if (y ==  gpi) croak("gpi");
          if (y==geuler) croak("geuler");
          affir(x,y); break;

        case t_INTMOD:
          modiiz(x,gel(y,1),gel(y,2)); break;

        case t_COMPLEX:
          if (y == gi) croak("gi");
          gaffect(x,gel(y,1)); gaffsg(0,gel(y,2)); break;

        case t_PADIC:
          if (!signe(x)) { padicaff0(y); break; }
          av = avma;
          setvalp(y, Z_pvalrem(x,gel(y,2),&p1));
          affii(modii(p1,gel(y,3)), gel(y,4));
          avma = av; break;

        case t_QUAD: gaffect(x,gel(y,2)); gaffsg(0,gel(y,3)); break;
        default: pari_err(operf,"",x,y);
      }
      break;
    
    case t_REAL:
      switch(ty)
      {
        case t_COMPLEX: gaffect(x,gel(y,1)); gaffsg(0,gel(y,2)); break;
        default: pari_err(operf,"",x,y);
      }
      break;
    
    case t_FRAC:
      switch(ty)
      {
        case t_REAL: rdiviiz(gel(x,1),gel(x,2), y); break;
        case t_INTMOD: av = avma;
          p1 = Fp_inv(gel(x,2),gel(y,1));
          affii(modii(mulii(gel(x,1),p1),gel(y,1)), gel(y,2));
          avma = av; break;
        case t_COMPLEX: gaffect(x,gel(y,1)); gaffsg(0,gel(y,2)); break;
        case t_PADIC:
          if (!signe(x[1])) { padicaff0(y); break; }
          num = gel(x,1);
          den = gel(x,2);
          av = avma; vx = Z_pvalrem(num, gel(y,2), &num);
          if (!vx) vx = -Z_pvalrem(den,gel(y,2),&den);
          setvalp(y,vx);
          p1 = mulii(num,Fp_inv(den,gel(y,3)));
          affii(modii(p1,gel(y,3)), gel(y,4)); avma = av; break;
        case t_QUAD: gaffect(x,gel(y,2)); gaffsg(0,gel(y,3)); break;
        default: pari_err(operf,"",x,y);
      }
      break;
    
    case t_PADIC:
      switch(ty)
      {
        case t_INTMOD:
          av = avma; affii(padic_to_Fp(x, gel(y,1)), gel(y,2));
          avma = av; break;
        default: pari_err(operf,"",x,y);
      }
      break;
    
    case t_QUAD:
      switch(ty)
      {
        case t_INT: case t_INTMOD: case t_FRAC: case t_PADIC:
          pari_err(operf,"",x,y);

        case t_REAL:
          av = avma; affgr(quadtoc(x,lg(y)), y); avma = av; break;
        case t_COMPLEX:
          ly = precision(y); if (!ly) pari_err(operf,"",x,y);
          av = avma; gaffect(quadtoc(x,ly), y); avma = av; break;
        default: pari_err(operf,"",x,y);
      }
    default: pari_err(operf,"",x,y);
  }
}

/*******************************************************************/
/*                                                                 */
/*           CONVERSION QUAD --> REAL, COMPLEX OR P-ADIC           */
/*                                                                 */
/*******************************************************************/

GEN
quadtoc(GEN x, long prec)
{
  GEN z, Q, u = gel(x,2), v = gel(x,3);
  pari_sp av;
  if (prec < 3) prec = 3;
  if (gcmp0(v)) return gtofp(u, prec);
  av = avma; Q = gel(x,1);
  /* should be sqri(Q[3]), but is 0,1 ! see quadpoly */
  z = itor(subsi(signe(gel(Q,3))? 1: 0, shifti(gel(Q,2),2)), prec);
  z = gsub(sqrtr(z), gel(Q,3));
  if (signe(gel(Q,2)) < 0) /* Q[2] = -D/4 or (1-D)/4 */
    setexpo(z, expo(z)-1);
  else
  {
    gel(z,1) = gmul2n(gel(z,1),-1);
    setexpo(z[2], expo(z[2])-1);
  }/* z = (-b + sqrt(D)) / 2 */
  return gerepileupto(av, gadd(u, gmul(v,z)));
}

static GEN
qtop(GEN x, GEN p, long d)
{
  GEN z, D, P, b, c, u = gel(x,2), v = gel(x,3);
  pari_sp av;
  if (gcmp0(v)) return cvtop(u, p, d);
  P = gel(x,1);
  b = gel(P,3);
  c = gel(P,2); av = avma;
  D = is_pm1(b)? subsi(1, shifti(c,2)): shifti(negi(c),2);
  if (equaliu(p,2)) d += 2;
  z = gmul2n(gsub(padic_sqrt(cvtop(D,p,d)), b), -1);
  return gerepileupto(av, gadd(u, gmul(v, z)));
}
static GEN
ctop(GEN x, GEN p, long d)
{
  pari_sp av = avma;
  GEN z, u = gel(x,1), v = gel(x,2);
  if (isexactzero(v)) return cvtop(u, p, d);
  z = padic_sqrt(cvtop(gen_m1, p, d - ggval(v, p))); /* = I */
  return gerepileupto(av, gadd(u, gmul(v, z)) );
}

/* cvtop(x, gel(y,2), precp(y)), internal, not memory-clean */
GEN
cvtop2(GEN x, GEN y)
{
  GEN z, p = gel(y,2);
  long v, d = signe(y[4])? precp(y): 0;
  switch(typ(x))
  {
    case t_INT:
      if (!signe(x)) return zeropadic(p, d);
      v = Z_pvalrem(x, p, &x);
      if (d <= 0) return zeropadic(p, v);
      z = cgetg(5, t_PADIC);
      z[1] = evalprecp(d) | evalvalp(v);
      gel(z,2) = p;
      z[3] = y[3];
      gel(z,4) = modii(x, gel(z,3)); return z;

    case t_INTMOD:
      if (!signe(x[2])) return zeropadic(p, d);
      v = Z_pval(gel(x,1),p);
      if (v <= d) return cvtop2(gel(x,2), y);
      return cvtop(gel(x,2), p, d);

    case t_FRAC: { GEN num = gel(x,1), den = gel(x,2);
      if (!signe(num)) return zeropadic(p, d);
      v = Z_pvalrem(num, p, &num);
      if (!v) v = -Z_pvalrem(den, p, &den); /* assume (num,den) = 1 */
      if (d <= 0) return zeropadic(p, v);
      z = cgetg(5, t_PADIC);
      z[1] = evalprecp(d) | evalvalp(v);
      gel(z,2) = p;
      z[3] = y[3];
      if (!is_pm1(den)) num = mulii(num, Fp_inv(den, gel(z,3)));
      gel(z,4) = modii(num, gel(z,3)); return z;
    }
    case t_COMPLEX: return ctop(x, p, d);
    case t_QUAD:    return qtop(x, p, d);
  }
  pari_err(typeer,"cvtop2");
  return NULL; /* not reached */
}

/* assume is_const_t(tx) */
GEN
cvtop(GEN x, GEN p, long d)
{
  GEN z;
  long v;

  if (typ(p) != t_INT) pari_err(talker,"not an integer modulus in cvtop");
  switch(typ(x))
  {
    case t_INT:
      if (!signe(x)) return zeropadic(p, d);
      v = Z_pvalrem(x, p, &x);
      if (d <= 0) return zeropadic(p, v);
      z = cgetg(5, t_PADIC);
      z[1] = evalprecp(d) | evalvalp(v);
      gel(z,2) = icopy(p);
      gel(z,3) = gpowgs(p, d);
      gel(z,4) = modii(x, gel(z,3)); return z; /* not memory-clean */

    case t_INTMOD:
      if (!signe(x[2])) return zeropadic(p, d);
      v = Z_pval(gel(x,1),p); if (v > d) v = d;
      return cvtop(gel(x,2), p, v);

    case t_FRAC: { GEN num = gel(x,1), den = gel(x,2);
      if (!signe(num)) return zeropadic(p, d);
      v = Z_pvalrem(num, p, &num);
      if (!v) v = -Z_pvalrem(den, p, &den); /* assume (num,den) = 1 */
      if (d <= 0) return zeropadic(p, v);
      z = cgetg(5, t_PADIC);
      z[1] = evalprecp(d) | evalvalp(v);
      gel(z,2) = icopy(p);
      gel(z,3) = gpowgs(p, d);
      if (!is_pm1(den)) num = mulii(num, Fp_inv(den, gel(z,3)));
      gel(z,4) = modii(num, gel(z,3)); return z; /* not memory-clean */
    }
    case t_COMPLEX: return ctop(x, p, d);
    case t_PADIC: return gprec(x,d);
    case t_QUAD: return qtop(x, p, d);
  }
  pari_err(typeer,"cvtop");
  return NULL; /* not reached */
}

GEN
gcvtop(GEN x, GEN p, long r)
{
  long i, lx, tx = typ(x);
  GEN y;

  if (is_const_t(tx)) return cvtop(x,p,r);
  switch(tx)
  {
    case t_POL: case t_SER:
    case t_POLMOD: case t_RFRAC:
    case t_VEC: case t_COL: case t_MAT:
      y = init_gen_op(x, tx, &lx, &i);
      for (; i<lx; i++) gel(y,i) = gcvtop(gel(x,i),p,r);
      return y;
  }
  pari_err(typeer,"gcvtop");
  return NULL; /* not reached */
}

long
gexpo(GEN x)
{
  long tx = typ(x), lx, e, f, i;

  switch(tx)
  {
    case t_INT:
      return expi(x);

    case t_FRAC:
      if (!signe(x[1])) return -(long)HIGHEXPOBIT;
      return expi(gel(x,1)) - expi(gel(x,2));

    case t_REAL:
      return expo(x);

    case t_COMPLEX:
      e = gexpo(gel(x,1));
      f = gexpo(gel(x,2)); return max(e, f);

    case t_QUAD: {
      GEN p = gel(x,1); /* mod = X^2 + {0,1}* X - {D/4, (1-D)/4})*/
      long d = 1 + expi(gel(p,2))/2; /* ~ expo(sqrt(D)) */
      e = gexpo(gel(x,2));
      f = gexpo(gel(x,3)) + d; return max(e, f);
    }
    case t_POL: case t_SER: case t_VEC: case t_COL: case t_MAT:
      lx = lg(x); f = -(long)HIGHEXPOBIT;
      for (i=lontyp[tx]; i<lx; i++) { e=gexpo(gel(x,i)); if (e>f) f=e; }
      return f;
  }
  pari_err(typeer,"gexpo");
  return 0; /* not reached */
}

long
sizedigit(GEN x)
{
  return gcmp0(x)? 0: (long) ((gexpo(x)+1) * L2SL10) + 1;
}

/* normalize series. avma is not updated */
GEN
normalize(GEN x)
{
  long i, lx = lg(x);
  GEN y;

  if (typ(x) != t_SER) pari_err(typeer,"normalize");
  if (lx==2) { setsigne(x,0); return x; }
  for (i=2; i<lx; i++)
    if (! isexactzero(gel(x,i)))
    {
      i -= 2; y = x + i; lx -= i;
      /* don't swap the following two lines! [valp/varn corrupted] */
      y[1] = evalsigne(1) | evalvalp(valp(x)+i) | evalvarn(varn(x));
      y[0] = evaltyp(t_SER) | evallg(lx);
      stackdummy((pari_sp)y, (pari_sp)x);
      for (i = 2; i < lx; i++)
        if (!gcmp0(gel(y, i))) return y;
      setsigne(y, 0); return y;
    }
  return zeroser(varn(x),lx-2+valp(x));
}

GEN
normalizepol_approx(GEN x, long lx)
{
  long i;
  for (i = lx-1; i>1; i--)
    if (! gcmp0(gel(x,i))) break;
  stackdummy((pari_sp)(x + lg(x)), (pari_sp)(x + i+1));
  setlg(x, i+1); setsigne(x, i!=1); return x;
}

GEN
normalizepol_i(GEN x, long lx)
{
  long i;
  for (i = lx-1; i>1; i--)
    if (! isexactzero(gel(x,i))) break;
  stackdummy((pari_sp)(x + lg(x)), (pari_sp)(x + i+1));
  setlg(x, i+1);
  for (; i>1; i--)
    if (! gcmp0(gel(x,i)) ) { setsigne(x,1); return x; }
  setsigne(x,0); return x;
}

/* normalize polynomial x in place */
GEN
normalizepol(GEN x)
{
  if (typ(x)!=t_POL) pari_err(typeer,"normalizepol");
  return normalizepol_i(x, lg(x));
}

int
gsigne(GEN x)
{
  switch(typ(x))
  {
    case t_INT: case t_REAL: return signe(x);
    case t_FRAC: return signe(x[1]);
  }
  pari_err(typeer,"gsigne");
  return 0; /* not reached */
}

/*******************************************************************/
/*                                                                 */
/*                              LISTS                              */
/*                                                                 */
/*******************************************************************/
/* make sure L can hold l elements, at least doubling the previous max number
 * of components. */
static void
ensure_nb(GEN L, long l)
{
  long nmax = list_nmax(L);
  GEN v;
  if (l <= nmax) return;
  if (nmax)
  {
    nmax <<= 1;
    if (l > nmax) nmax = l;
    v = (GEN)gprealloc(list_data(L), (nmax+1) * sizeof(long));
  }
  else
  {
    nmax = 32;
    v = (GEN)gpmalloc((nmax+1) * sizeof(long));
    v[0] = evaltyp(t_VEC) | evallg(1);
  }
  list_data(L) = v;
  list_nmax(L) = nmax;
}

void
listkill(GEN L)
{

  if (typ(L) != t_LIST) pari_err(typeer,"listkill");
  if (list_nmax(L)) {
    GEN v = list_data(L);
    long i, l = lg(v);
    for (i=1; i<l; i++) killbloc(gel(v,i));
    gpfree(v); list_nmax(L) = 0;
  }
  ensure_nb(L, 32);
}

GEN
listcreate(void)
{
  GEN L = cgetg(3,t_LIST);
  list_nmax(L) = 0;
  ensure_nb(L, 32);
  return L;
}

GEN
listput(GEN L, GEN x, long index)
{
  long l;
  GEN z;

  if (typ(L) != t_LIST) pari_err(typeer,"listput");
  if (index < 0) pari_err(talker,"negative index (%ld) in listput", index);
  z = list_data(L);
  l = lg(z);

  if (!index || index >= l)
  {
    ensure_nb(L, l);
    z = list_data(L); /* it may change ! */
    index = l;
  } else
    killbloc( gel(z, index) );
  z[0] = evaltyp(t_VEC) | evallg(l+1);
  return gel(z,index) = gclone(x);
}

GEN
listinsert(GEN L, GEN x, long index)
{
  long l, i;
  GEN z;

  if (typ(L) != t_LIST) pari_err(typeer,"listinsert");

  z = list_data(L); l = lg(z);
  if (index <= 0 || index >= l) pari_err(talker,"bad index in listinsert");
  ensure_nb(L, l);
  z = list_data(L);
  for (i=l; i > index; i--) z[i] = z[i-1];
  z[0] = evaltyp(t_VEC) | evallg(l+1);
  return gel(z,index) = gclone(x);
}

void
listpop(GEN L, long index)
{
  long l, i;
  GEN z;

  if (typ(L) != t_LIST) pari_err(typeer,"listinsert");

  if (index < 0) pari_err(talker,"negative index (%ld) in listpop", index);
  z = list_data(L);
  l = lg(z)-1;
  if (!l) pari_err(talker,"empty list in listpop");

  if (!index || index > l) index = l;
  killbloc( gel(z, index) );
  z[0] = evaltyp(t_VEC) | evallg(l);
  for (i=index; i < l; i++) z[i] = z[i+1];
}

/* x a t_VEC/t_COL */
GEN
vectolist(GEN x)
{
  GEN v, L = listcreate();
  long i, l = lg(x);
  ensure_nb(L, l-1);
  v = list_data(L); v[0] = x[0];
  for (i = 1; i < l; i++) gel(v,i) = gclone(gel(x,i));
  return L;
}

GEN
gtolist(GEN x)
{
  GEN L;

  if (!x) return listcreate();
  switch(typ(x))
  {
    case t_VEC: case t_COL: return vectolist(x);
    case t_LIST: return listcopy(x);
    default:
      L = listcreate();
      (void)listput(L, x, 0);
      return L;
  }
}

GEN
listconcat(GEN L1, GEN L2)
{
  long i, l1, lx;
  GEN L, z;

  if (typ(L1) != t_LIST) {
    if (typ(L2) != t_LIST) pari_err(typeer,"listconcat");
    L = listcopy(L2);
    (void)listinsert(L, L1, 1);
    return L;
  } else if (typ(L2) != t_LIST) {
    L = listcopy(L1);
    (void)listput(L, L2, 0);
    return L;
  }
  /* L1, L2 both t_LISTs */
  L1 = list_data(L1);
  L2 = list_data(L2);

  l1 = lg(L1);
  lx = l1-1 + lg(L2);
  z = cgetg(3, t_LIST);
  list_nmax(z) = lx-1;
  list_data(z) = L = (GEN)gpmalloc(lx * sizeof(long));
  L2 -= l1-1;
  for (i=1; i<l1; i++) gel(L,i) = gclone(gel(L1,i));
  for (   ; i<lx; i++) gel(L,i) = gclone(gel(L2,i));
  L[0] = evaltyp(t_VEC) | evallg(lx); return z;
}

GEN
listsort(GEN L, long flag)
{
  long i, l;
  pari_sp av = avma;
  GEN perm, v, vnew;

  if (typ(L) != t_LIST) pari_err(typeer,"listsort");
  v = list_data(L); l = lg(v);
  if (l < 3) return L;
  if (flag)
  {
    long lnew;
    perm = gen_indexsort_uniq(L, (void*)&lexcmp, cmp_nodata);
    lnew = lg(perm); /* may have changed since 'uniq' */
    vnew = cgetg(lnew,t_VEC);
    for (i=1; i<lnew; i++) {
      long c = perm[i];
      gel(vnew,i) = gel(v,c);
      gel(v,c) = NULL;
    }
    if (l != lnew) { /* was shortened */
      for (i=1; i<l; i++)
        if (gel(v,i)) killbloc(gel(v,i));
      l = lnew;
    }
  }
  else
  {
    perm = gen_indexsort(L, (void*)&lexcmp, cmp_nodata);
    vnew = cgetg(l,t_VEC);
    for (i=1; i<l; i++) gel(vnew,i) = gel(v,perm[i]);
  }
  for (i=1; i<l; i++) gel(v,i) = gel(vnew,i);
  v[0] = vnew[0]; avma = av; return L;
}
