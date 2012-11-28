/* $Id$

Copyright (C) 2000-2004  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

/***********************************************************************/
/**                                                                   **/
/**               ARITHMETIC OPERATIONS ON POLYNOMIALS                **/
/**                         (first part)                              **/
/**                                                                   **/
/***********************************************************************/
#include "pari.h"
#include "paripriv.h"

/*******************************************************************/
/*                                                                 */
/*                  POLYNOMIAL EUCLIDEAN DIVISION                  */
/*                                                                 */
/*******************************************************************/
GEN
poldivrem(GEN x, GEN y, GEN *pr)
{
  long ty = typ(y), tx, vx = gvar(x), vy = gvar(y);
  GEN p1;

  if (is_scalar_t(ty) || varncmp(vx, vy) < 0)
  {
    if (pr == ONLY_REM) {
      if (gequal0(y)) pari_err_INV("poldivrem",y);
      return gen_0;
    }
    if (pr && pr != ONLY_DIVIDES) *pr=gen_0;
    return gdiv(x,y);
  }
  if (ty != t_POL) pari_err_TYPE2("euclidean division",x,y);
  tx = typ(x);
  if (is_scalar_t(tx) || varncmp(vx, vy) > 0)
  {
    if (!signe(y)) pari_err_INV("poldivrem",y);
    if (!degpol(y)) /* constant */
    {
      if (pr == ONLY_REM) return pol_0(vy);
      p1 = gdiv(x, gel(y,2));
      if (pr == ONLY_DIVIDES) return p1;
      if (pr) *pr = pol_0(vy);
      return p1;
    }
    if (pr == ONLY_REM) return gcopy(x);
    if (pr == ONLY_DIVIDES) return gequal0(x)? gen_0: NULL;
    if (pr) *pr = gcopy(x);
    return gen_0;
  }
  if (tx != t_POL) pari_err_TYPE2("euclidean division",x,y);

  if (varncmp(vx, vy) < 0)
  {
    if (pr && pr != ONLY_DIVIDES)
    {
      p1 = pol_0(vx); if (pr == ONLY_REM) return p1;
      *pr = p1;
    }
    return gdiv(x,y);
  }
  return RgX_divrem(x, y, pr);
}
GEN
gdeuc(GEN x, GEN y)
{
  long ty = typ(y), tx, vx = gvar(x), vy = gvar(y);

  if (is_scalar_t(ty) || varncmp(vx, vy) < 0) return gdiv(x,y);
  if (ty != t_POL) pari_err_TYPE2("euclidean division",x,y);
  tx = typ(x);
  if (is_scalar_t(tx) || varncmp(vx, vy) > 0)
  {
    if (!signe(y)) pari_err_INV("gdeuc",y);
    if (!degpol(y)) return gdiv(x, gel(y,2)); /* constant */
    return gen_0;
  }
  if (tx != t_POL) pari_err_TYPE2("euclidean division",x,y);
  if (varncmp(vx, vy) < 0) return gdiv(x,y);
  return RgX_div(x, y);
}
GEN
grem(GEN x, GEN y)
{
  long ty = typ(y), tx, vx = gvar(x), vy = gvar(y);

  if (is_scalar_t(ty) || varncmp(vx, vy) < 0)
  {
    if (gequal0(y)) pari_err_INV("grem",y);
    return gen_0;
  }
  if (ty != t_POL) pari_err_TYPE2("euclidean division",x,y);
  tx = typ(x);
  if (is_scalar_t(tx) || varncmp(vx, vy) > 0)
  {
    if (!signe(y)) pari_err_INV("grem",y);
    if (!degpol(y)) return pol_0(vy); /* constant */
    return gcopy(x);
  }
  if (tx != t_POL) pari_err_TYPE2("euclidean division",x,y);

  if (varncmp(vx, vy) < 0) return pol_0(vx);
  return RgX_rem(x, y);
}

/*******************************************************************/
/*                                                                 */
/*                  CONVERSIONS RELATED TO p-ADICS                 */
/*                                                                 */
/*******************************************************************/
/* x t_PADIC, p a prime or NULL (unset). Consistency check */
static void
check_padic_p(GEN x, GEN p)
{
  GEN q = gel(x,2);
  if (p && !equalii(p, q)) pari_err_MODULUS("Zp_to_Z", p,q);
}
static GEN
Zp_to_Z(GEN x, GEN p) {
  switch(typ(x))
  {
    case t_INT: break;
    case t_PADIC:
      check_padic_p(x, p);
      x = gtrunc(x); break;
    default: pari_err_TYPE("Zp_to_Z",x);
  }
  return x;
}
static GEN
ZpX_to_ZX(GEN f, GEN p) {
  long i, l = lg(f);
  GEN F = cgetg(l, t_POL); F[1] = f[1];
  for (i=2; i<l; i++) gel(f,i) = Zp_to_Z(gel(f,i), p);
  return f;
}

static GEN
get_padic_content(GEN f, GEN p)
{
  GEN c = content(f);
  if (gequal0(c)) /*  O(p^n) can occur */
  {
    if (typ(c) != t_PADIC) pari_err_TYPE("QpX_to_ZX",f);
    check_padic_p(c, p);
    c = powis(p, valp(c));
  }
  return c;
}
/* make f suitable for [root|factor]padic */
static GEN
QpX_to_ZX(GEN f, GEN p)
{
  GEN c = get_padic_content(f, p);
  f = RgX_Rg_div(f, c);
  return ZpX_to_ZX(f, p);
}

/* x in Z return x + O(pr), pr = p^r */
static GEN
Z_to_Zp(GEN x, GEN p, GEN pr, long r)
{
  GEN y;
  long v, sx = signe(x);

  if (!sx) return zeropadic_shallow(p,r);
  v = Z_pvalrem(x,p,&x);
  if (v) {
    r -= v; if (r <= 0) return zeropadic_shallow(p,r);
    pr = powiu(p,r);
  }
  y = cgetg(5,t_PADIC);
  y[1] = evalprecp(r)|evalvalp(v);
  gel(y,2) = p;
  gel(y,3) = pr;
  gel(y,4) = modii(x,pr); return y;
}
static GEN
ZV_to_ZpV(GEN z, GEN p, long prec)
{
  long i, l = lg(z);
  GEN Z = cgetg(l, typ(z)), q = powiu(p, prec);
  for (i=1; i<lg(z); i++) gel(Z,i) = Z_to_Zp(gel(z,i),p,q,prec);
  return Z;
}
static GEN
ZX_to_ZpX(GEN z, GEN p, GEN q, long prec)
{
  long i, l = lg(z);
  GEN Z = cgetg(l, t_POL); Z[1] = z[1];
  for (i=2; i<lg(z); i++) gel(Z,i) = Z_to_Zp(gel(z,i),p,q,prec);
  return Z;
}
/* return (x + O(p^r)) normalized (multiply by a unit such that leading coeff
 * is a power of p), x in Z[X] (or Z_p[X]) */
static GEN
ZX_to_ZpX_normalized(GEN x, GEN p, GEN pr, long r)
{
  long i, lx = lg(x);
  GEN z, lead = leading_term(x);

  if (gequal1(lead)) return ZX_to_ZpX(x, p, pr, r);
  (void)Z_pvalrem(lead, p, &lead); lead = Fp_inv(lead, pr);
  z = cgetg(lx,t_POL);
  for (i=2; i < lx; i++) gel(z,i) = Z_to_Zp(mulii(lead,gel(x,i)),p,pr,r);
  z[1] = x[1]; return z;
}
static GEN
ZXV_to_ZpXQV(GEN z, GEN T, GEN p, long prec)
{
  long i, l = lg(z);
  GEN Z = cgetg(l, typ(z)), q = powiu(p, prec);
  T = ZX_copy(T);
  for (i=1; i<lg(z); i++) gel(Z,i) = mkpolmod(ZX_to_ZpX(gel(z,i),p,q,prec),T);
  return Z;
}

static GEN
QpXQX_to_ZXY(GEN f, GEN p)
{
  GEN c = get_padic_content(f, p);
  long i, l = lg(f);
  f = RgX_Rg_div(f,c);
  for (i=2; i<l; i++)
  {
    GEN t = gel(f,i);
    switch(typ(t))
    {
      case t_POLMOD:
        t = gel(t,2);
        t = (typ(t) == t_POL)? ZpX_to_ZX(t, p): Zp_to_Z(t, p);
        break;
      case t_POL: t = ZpX_to_ZX(t, p); break;
      default: t = Zp_to_Z(t, p); break;
    }
    gel(f,i) = t;
  }
  return f;
}

/*******************************************************************/
/*                                                                 */
/*                         p-ADIC ROOTS                            */
/*                                                                 */
/*******************************************************************/

/* f primitive ZX, squarefree, leading term prime to p; a in Z such that
 * f(a) = 0 mod p. Return p-adic roots of f equal to a mod p, in
 * precision >= prec */
static GEN
ZX_Zp_root(GEN f, GEN a, GEN p, long prec)
{
  GEN z, R, a0 = modii(a, p);
  long i, j, k, v;

  if (signe(FpX_eval(FpX_deriv(f, p), a0, p)))
  { /* simple zero mod p, go all the way to p^prec */
    if (prec > 1) a0 = ZpX_liftroot(f, a0, p, prec);
    return mkcol(a0);
  }

  f = ZX_unscale_div(RgX_translate(f,a), p); /* f(pX + a) / p */
  v = ZX_pval(f,p);
  if (v) f = ZX_Z_divexact(f, powiu(p,v));
  z = cgetg(degpol(f)+1,t_COL);

  R = FpX_roots(f, p);
  for (j=i=1; i<lg(R); i++)
  {
    GEN u = ZX_Zp_root(f, gel(R,i), p, prec-1);
    for (k=1; k<lg(u); k++) gel(z,j++) = addii(a, mulii(p, gel(u,k)));
  }
  setlg(z,j); return z;
}

/* a t_PADIC, return vector of p-adic roots of f equal to a (mod p)
 * We assume 1) f(a) = 0 mod p,
 *           2) leading coeff prime to p. */
GEN
Zp_appr(GEN f, GEN a)
{
  pari_sp av = avma;
  long prec;
  GEN z, p;
  if (typ(f) != t_POL) pari_err_TYPE("Zp_appr",f);
  if (typ(a) != t_PADIC) pari_err_TYPE("Zp_appr",a);
  p = gel(a,2); prec = gequal0(a)? valp(a): precp(a);
  f = QpX_to_ZX(f, p);
  if (degpol(f) <= 0) pari_err_CONSTPOL("Zp_appr");
  (void)ZX_gcd_all(f, ZX_deriv(f), &f);
  z = ZX_Zp_root(f, gtrunc(a), p, prec);
  return gerepilecopy(av, ZV_to_ZpV(z, p, prec));
}
/* vector of p-adic roots of the ZX f, leading term prime to p */
static GEN
ZX_Zp_roots(GEN f, GEN p, long prec)
{
  GEN y, z, rac;
  long lx, i, j, k;

  (void)ZX_gcd_all(f, ZX_deriv(f), &f);
  rac = FpX_roots(f, p);
  lx = lg(rac); if (lx == 1) return rac;
  y = cgetg(degpol(f)+1,t_COL);
  for (j=i=1; i<lx; i++)
  {
    z = ZX_Zp_root(f, gel(rac,i), p, prec);
    for (k=1; k<lg(z); k++,j++) gel(y,j) = gel(z,k);
  }
  setlg(y,j); return ZV_to_ZpV(y, p, prec);
}

/* f a ZX */
static GEN
pnormalize(GEN f, GEN p, long prec, long n, GEN *plead, long *pprec, int *prev)
{
  *plead = leading_term(f);
  *pprec = prec;
  *prev = 0;
  if (!is_pm1(*plead))
  {
    long v = Z_pval(*plead,p), v1 = Z_pval(constant_term(f),p);
    if (v1 < v)
    {
      *prev = 1; f = RgX_recip_shallow(f);
     /* beware loss of precision from lc(factor), whose valuation is <= v */
      *pprec += v; v = v1;
    }
    *pprec += v * n;
  }
  return ZX_Q_normalize(f, plead);
}

/* return p-adic roots of f, precision prec */
GEN
rootpadic(GEN f, GEN p, long prec)
{
  pari_sp av = avma;
  GEN lead,y;
  long PREC,i,k;
  int reverse;

  if (typ(p)!=t_INT) pari_err_TYPE("rootpadic",p);
  if (typ(f)!=t_POL) pari_err_TYPE("rootpadic",f);
  if (gequal0(f)) pari_err_ROOTS0("rootpadic");
  if (prec <= 0)
    pari_err_DOMAIN("rootpadic", "precision", "<=",gen_0,stoi(prec));
  f = QpX_to_ZX(f, p);
  f = pnormalize(f, p, prec, 1, &lead, &PREC, &reverse);
  y = ZX_Zp_roots(f,p,PREC);
  k = lg(y);
  if (lead != gen_1)
    for (i=1; i<k; i++) gel(y,i) = gdiv(gel(y,i), lead);
  if (reverse)
    for (i=1; i<k; i++) gel(y,i) = ginv(gel(y,i));
  return gerepilecopy(av, y);
}
/* p is prime
 * f in a ZX, with leading term prime to p.
 * f must have no multiple roots mod p.
 *
 * return p-adics roots of f with prec p^e, as integers (implicitly mod p^e) */
GEN
rootpadicfast(GEN f, GEN p, long e)
{
  pari_sp av = avma;
  GEN y, S = FpX_roots(f, p); /*no multiplicity*/
  if (lg(S)==1) { avma = av; return cgetg(1,t_COL); }
  S = gclone(S); avma = av;
  y = ZpX_liftroots(f,S,p,e);
  gunclone(S); return y;
}
/**************************************************************************/

static void
scalar_getprec(GEN x, long *pprec, GEN *pp)
{
  if (typ(x)==t_PADIC)
  {
    long e = valp(x); if (signe(gel(x,4))) e += precp(x);
    if (e < *pprec) *pprec = e;
    check_padic_p(x, *pp);
    *pp = gel(x,2);
  }
}
static void
getprec(GEN x, long *pprec, GEN *pp)
{
  long i;
  if (typ(x) != t_POL) scalar_getprec(x, pprec, pp);
  else
    for (i = lg(x)-1; i>1; i--) scalar_getprec(gel(x,i), pprec, pp);
}

static GEN
ZXY_ZpQ_root(GEN f, GEN a, GEN T, GEN p, long prec)
{
  GEN z, R;
  long i, j, k, lR;
  if (signe(FqX_eval(FqX_deriv(f,T,p), a, T,p)))
  { /* simple zero mod (T,p), go all the way to p^prec */
    if (prec > 1) a = ZpXQX_liftroot(f, a, T, p, prec);
    return mkcol(a);
  }
  f = RgX_unscale(RgXQX_translate(f, a, T), p);
  f = RgX_Rg_div(f, powiu(p, gvaluation(f,p)));
  z = cgetg(degpol(f)+1,t_COL);
  R = FqX_roots(FqX_red(f,T,p), T, p); lR = lg(R);
  for(j=i=1; i<lR; i++)
  {
    GEN u = ZXY_ZpQ_root(f, gel(R,i), T, p, prec-1);
    for (k=1; k<lg(u); k++) gel(z,j++) = gadd(a, gmul(p, gel(u,k)));
  }
  setlg(z,j); return z;
}

/* a belongs to finite extension of Q_p, return all roots of f equal to a
 * mod p. We assume f(a) = 0 (mod p) [mod 4 if p=2] */
GEN
padicappr(GEN f, GEN a)
{
  GEN p, z, T;
  long prec;
  pari_sp av = avma;

  switch(typ(a)) {
    case t_PADIC: return Zp_appr(f,a);
    case t_POLMOD: break;
    default: pari_err_TYPE("padicappr",a);
  }
  if (typ(f)!=t_POL) pari_err_TYPE("padicappr",f);
  if (gequal0(f)) pari_err_ROOTS0("padicappr");
  z = RgX_gcd(f, RgX_deriv(f));
  if (degpol(z) > 0) f = RgX_div(f,z);
  T = gel(a,1); a = gel(a,2);
  p = NULL; prec = LONG_MAX;
  getprec(a, &prec, &p);
  getprec(T, &prec, &p); if (!p) pari_err_TYPE("padicappr",T);
  f = QpXQX_to_ZXY(f, p);
  a = QpX_to_ZX(a,p);
  T = QpX_to_ZX(T,p);
  z = ZXY_ZpQ_root(f, a, T, p, prec);
  return gerepilecopy(av, ZXV_to_ZpXQV(z, T, p, prec));
}

/*******************************************************************/
/*                                                                 */
/*                      FACTORIZATION in Zp[X]                     */
/*                                                                 */
/*******************************************************************/

int
cmp_padic(GEN x, GEN y)
{
  long vx, vy;
  if (x == gen_0) return -1;
  if (y == gen_0) return  1;
  vx = valp(x);
  vy = valp(y);
  if (vx < vy) return  1;
  if (vx > vy) return -1;
  return cmpii(gel(x,4), gel(y,4));
}

/*******************************/
/*   Using Buchmann--Lenstra   */
/*******************************/

/* factor T = nf_get_pol(nf) in Zp to precision k */
static GEN
padicff2(GEN nf,GEN p,long k)
{
  GEN z, mat, D, U, fa, pk, dec_p, Ui, mulx;
  GEN invzk = nf_get_invzk(nf);
  long i, l, v = nf_get_varn(nf);

  if (lg(invzk) == 2)
  { /* trivial case deg = 1 */
    z = nf_get_pol(nf);
    retmkcol( ZX_to_ZpX_normalized(z, p, powiu(p, k), k) );
  }
  mulx = zk_scalar_or_multable(nf, gel(invzk,2)); /* mul by (x mod T) */
  dec_p = idealprimedec(nf,p);
  fa = cgetg_copy(dec_p, &l);
  D = NULL; /* -Wall */
  for (i=1; i<l; i++)
  {
    GEN P = gel(dec_p,i);
    long e = pr_get_e(P), ef = e * pr_get_f(P);
    D = ZM_snfall_i(idealpows(nf,P, k*e), &U, NULL, 1);
    Ui= ZM_inv(U, gen_1); setlg(Ui, ef+1); /* cf ZM_snf_group */
    U = rowslice(U, 1, ef);
    mat = ZM_mul(U, ZM_mul(mulx, Ui));
    gel(fa,i) = ZM_charpoly(mat); setvarn(gel(fa,i), v);
  }
  pk = gel(D,1); /* = p^k */
  z = cgetg(l,t_COL); pk = icopy(pk);
  for (i=1; i<l; i++)
    gel(z,i) = ZX_to_ZpX_normalized(gel(fa,i),p,pk,k);
  return z;
}

static GEN
padicff(GEN x,GEN p,long pr)
{
  pari_sp av = avma;
  GEN q, nf, g, e;
  long v = Z_pvalrem(absi(ZX_disc(x)), p, &q);
  nfmaxord_t S;

  if (is_pm1(q)) {
    e = mkcol(utoi(v));
    g = mkcol(p);
  } else {
    e = mkcol2(utoi(v), gen_1);
    g = mkcol2(p, q);
  }
  nfmaxord(&S, x, nf_ROUND2, mkmat2(g,e));

  nf = cgetg(10,t_VEC);
  gel(nf,1) = x;
  gel(nf,3) = S.dK;
  gel(nf,4) = dvdii(S.index, p)? p: gen_1;
  gel(nf,2) = gel(nf,5) = gel(nf,6) = gen_0;
  nf_set_multable(nf, S.basis, NULL);
  return gerepileupto(av, padicff2(nf,p,pr));
}

static GEN
padic_trivfact(GEN x, GEN p, long r)
{
  retmkmat2(mkcol(ZX_to_ZpX_normalized(x, p, powiu(p,r), r)),
                mkcol(gen_1));
}

static GEN
factorpadic2(GEN f, GEN p, long prec)
{
  pari_sp av = avma;
  GEN fa,ex,y;
  long i,l, n = degpol(f);

  if (n==0) return trivial_fact();

  f = QpX_to_ZX(f, p);
  if (n==1) return gerepilecopy(av, padic_trivfact(f,p,prec));
  if (!gequal1(leading_term(f)))
    pari_err_IMPL("factorpadic2 for non-monic polynomial");

  fa = ZX_squff(f, &ex);
  l = lg(fa); n = 0;
  for (i=1; i<l; i++)
  {
    gel(fa,i) = padicff(gel(fa,i),p,prec);
    n += lg(gel(fa,i))-1;
  }

  y = fact_from_DDF(fa,ex,n);
  return gerepileupto(av, sort_factor_pol(y, cmp_padic));
}

/***********************/
/*   Using ROUND 4     */
/***********************/
static int
expo_is_squarefree(GEN e)
{
  long i, l = lg(e);
  for (i=1; i<l; i++)
    if (e[i] != 1) return 0;
  return 1;
}

/* assume f a ZX with leading_term 1, degree > 0 */
GEN
ZX_monic_factorpadic(GEN f, GEN p, long prec)
{
  GEN w, poly, p1, p2, ex, P, E;
  long n=degpol(f), i, k, j;

  if (n==1) return mkmat2(mkcol(f), mkcol(gen_1));

  poly = ZX_squff(f,&ex);
  P = cgetg(n+1,t_COL);
  E = cgetg(n+1,t_COL); n = lg(poly);
  for (j=i=1; i<n; i++)
  {
    pari_sp av1 = avma;
    GEN fx = gel(poly,i), fa = FpX_factor(fx,p);
    w = gel(fa,1);
    if (expo_is_squarefree(gel(fa,2)))
    { /* no repeated factors: Hensel lift */
      p1 = ZpX_liftfact(fx, w, NULL, p, prec, powiu(p,prec));
      p2 = utoipos(ex[i]);
      for (k=1; k<lg(p1); k++,j++)
      {
        gel(P,j) = gel(p1,k);
        gel(E,j) = p2;
      }
      continue;
    }
    /* use Round 4 */
    p2 = maxord_i(p, fx, ZpX_disc_val(fx,p), w, prec);
    if (p2)
    {
      p2 = gerepilecopy(av1,p2);
      p1 = gel(p2,1);
      p2 = gel(p2,2);
      for (k=1; k<lg(p1); k++,j++)
      {
        gel(P,j) = gel(p1,k);
        gel(E,j) = muliu(gel(p2,k),ex[i]);
      }
    }
    else
    {
      avma = av1;
      gel(P,j) = fx;
      gel(E,j) = utoipos(ex[i]); j++;
    }
  }
  setlg(P,j);
  setlg(E,j); return mkmat2(P, E);
}

GEN
factorpadic(GEN f,GEN p,long prec)
{
  pari_sp av = avma;
  GEN y, P, ppow, lead, lt;
  long i, l, pr, n = degpol(f);
  int reverse = 0;

  if (n == 0) return trivial_fact();

  f = QpX_to_ZX(f, p); (void)Z_pvalrem(leading_term(f), p, &lt);
  f = pnormalize(f, p, prec, n-1, &lead, &pr, &reverse);
  y = ZX_monic_factorpadic(f, p, pr);
  P = gel(y,1); l = lg(P);
  if (lead != gen_1)
    for (i=1; i<l; i++) gel(P,i) = Q_primpart( RgX_unscale(gel(P,i), lead) );
  ppow = powiu(p,prec);
  for (i=1; i<l; i++)
  {
    GEN t = gel(P,i);
    if (reverse) t = normalizepol(RgX_recip_shallow(t));
    gel(P,i) = ZX_to_ZpX_normalized(t,p,ppow,prec);
  }
  if (!gequal1(lt)) gel(P,1) = gmul(gel(P,1), lt);
  return gerepilecopy(av, sort_factor_pol(y, cmp_padic));
}

GEN
factorpadic0(GEN f,GEN p,long r,long flag)
{
  if (typ(f)!=t_POL) pari_err_TYPE("factorpadic",f);
  if (typ(p)!=t_INT) pari_err_TYPE("factorpadic",p);
  if (!signe(f)) return zero_fact(f);
  if (r <= 0)
    pari_err_DOMAIN("factorpadic", "precision", "<=",gen_0,stoi(r));
  switch(flag)
  {
     case 0: return factorpadic(f,p,r);
     case 1: return factorpadic2(f,p,r);
     default: pari_err_FLAG("factorpadic");
  }
  return NULL; /* not reached */
}
