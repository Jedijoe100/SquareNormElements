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

/*******************************************************************/
/*                                                                 */
/*                       BASIC NF OPERATIONS                       */
/*                           (continued)                           */
/*                                                                 */
/*******************************************************************/
#include "pari.h"
#include "paripriv.h"

/*******************************************************************/
/*                                                                 */
/*                     IDEAL OPERATIONS                            */
/*                                                                 */
/*******************************************************************/

/* A valid ideal is either principal (valid nf_element), or prime, or a matrix
 * on the integer basis (preferably HNF).
 * A prime ideal is of the form [p,a,e,f,b], where the ideal is p.Z_K+a.Z_K,
 * p is a rational prime, a belongs to Z_K, e=e(P/p), f=f(P/p), and b
 * Lenstra constant (p.P^(-1)= p Z_K + b Z_K).
 *
 * An extended ideal is a couple[I,F] where I is a valid ideal and F a
 * factorization matrix associated to an algebraic number.
 * All routines work with either extended ideals or ideals (an omitted F
 * is assumed to be [;] <-> 1).
 *
 * All ideals are output in HNF form. */

/* types and conversions */

long
idealtyp(GEN *ideal, GEN *arch)
{
  GEN x = *ideal;
  long t,lx,tx = typ(x);

  if (tx==t_VEC && lg(x)==3)
  { *arch = gel(x,2); x = gel(x,1); tx = typ(x); }
  else
    *arch = NULL;
  switch(tx)
  {
    case t_MAT: lx = lg(x);
      if (lx == 1) { t = id_PRINCIPAL; x = gen_0; break; }
      if (lx != lg(x[1])) pari_err(talker,"non-square t_MAT in idealtyp");
      t = id_MAT;
      break;

    case t_VEC: if (lg(x)!=6) pari_err(talker, "incorrect ideal in idealtyp");
      t = id_PRIME; break;

    case t_POL: case t_POLMOD: case t_COL:
    case t_INT: case t_FRAC:
      t = id_PRINCIPAL; break;
    default:
      pari_err(talker, "incorrect ideal in idealtyp");
      return 0; /*not reached*/
  }
  *ideal = x; return t;
}

/* pr = [a,x,...], a in Z. Return (a,x)  [HACK: pr need not be prime] */
GEN
idealhnf_two(GEN nf, GEN pr)
{
  GEN p = gel(pr,1), pi = gel(pr,2), m = zk_scalar_or_multable(nf, pi);
  if (typ(m) == t_INT) return scalarmat(p, lg(pi)-1);
  return ZM_hnfmodid(m, p);
}

static GEN
ZM_Q_mul(GEN x, GEN y) 
{ return typ(y) == t_INT? ZM_Z_mul(x,y): RgM_Rg_mul(x,y); }

/* x integral ideal in t_MAT form, nx columns */
static GEN
vec_mulid(GEN nf, GEN x, long nx, long N)
{
  GEN m = cgetg(nx*N + 1, t_MAT);
  long i, j, k;
  for (i=k=1; i<=nx; i++)
    for (j=1; j<=N; j++) gel(m, k++) = elementi_mulid(nf, gel(x,i),j);
  return m;
}

GEN
idealhnf_principal(GEN nf, GEN x)
{
  GEN cx;
  x = nf_to_scalar_or_basis(nf, x);
  switch(typ(x))
  {
    case t_COL: break;
    case t_INT:  if (!signe(x)) return cgetg(1,t_MAT);
      return scalarmat(absi(x), degpol(nf[1]));
    case t_FRAC:
      return scalarmat(Q_abs(x), degpol(nf[1]));
    default: pari_err(typeer,"idealhnf");
  }
  x = Q_primitive_part(x, &cx);
  RgV_check_ZV(x, "idealhnf");
  x = zk_multable(nf, x);
  x = ZM_hnfmod(x, ZM_detmult(x));
  return cx? ZM_Q_mul(x,cx): x;
}

GEN
idealhnf_shallow(GEN nf, GEN x)
{
  GEN cx;
  long tx = typ(x), lx = lg(x);
  
  /* cannot use idealtyp because here we allow non-square matrices */
  if (tx == t_VEC && lx == 3) { x = gel(x,1); tx = typ(x); lx = lg(x); }
  if (tx == t_VEC && lx == 6) return idealhnf_two(nf,x); /* PRIME */
  if (tx == t_MAT) {
    long nx = lx-1, N = degpol(nf[1]);
    if (nx == 0) return cgetg(1, t_MAT);
    if (lg(x[1])-1 != N) pari_err(talker,"incorrect dimension in idealhnf");
    if (nx == 1) return idealhnf_principal(nf, gel(x,1));

    if (nx == N && RgM_ishnf(x)) return x;
    x = Q_primitive_part(x, &cx);
    if (nx < N) x = vec_mulid(nf, x, nx, N);
    x = ZM_hnfmod(x, ZM_detmult(x));
    return cx? ZM_Q_mul(x,cx): x;
  }
  return idealhnf_principal(nf, x); /* PRINCIPAL */
}
GEN
idealhnf(GEN nf, GEN x)
{
  pari_sp av = avma;
  GEN y = idealhnf_shallow(checknf(nf), x);
  return (avma == av)? gcopy(y): gerepileupto(av, y);
}

static GEN
mylog(GEN x, long prec)
{
  if (gcmp0(x)) pari_err(precer,"get_arch");
  return glog(x,prec);
}

/* For internal use. Get archimedean components: [e_i Log( sigma_i(X) )],
 * where X = primpart(x), and e_i = 1 (resp 2.) for i <= R1 (resp. > R1) */
GEN
get_arch(GEN nf,GEN x,long prec)
{
  long i, R1, RU;
  GEN v;
  if (typ(x) == t_MAT) return famat_to_arch(nf,x,prec);
  x = nf_to_scalar_or_basis(nf,x);
  RU = lg(nf[6]) - 1;
  if (typ(x) != t_COL) return zerovec(RU);
  x = RgM_RgC_mul(gmael(nf,5,1), Q_primpart(x));
  v = cgetg(RU+1,t_VEC); R1 = nf_get_r1(nf);
  for (i=1; i<=R1; i++) gel(v,i) = mylog(gel(x,i),prec);
  for (   ; i<=RU; i++) gel(v,i) = gmul2n(mylog(gel(x,i),prec),1);
  return v;
}

GEN get_arch_real(GEN nf,GEN x,GEN *emb,long prec);

static GEN
famat_get_arch_real(GEN nf,GEN x,GEN *emb,long prec)
{
  GEN A, T, a, t, g = gel(x,1), e = gel(x,2);
  long i, l = lg(e);

  if (l <= 1)
    return get_arch_real(nf, gen_1, emb, prec);
  A = T = NULL; /* -Wall */
  for (i=1; i<l; i++)
  {
    a = get_arch_real(nf, gel(g,i), &t, prec);
    if (!a) return NULL;
    a = RgC_Rg_mul(a, gel(e,i));
    t = vecpow(t, gel(e,i));
    if (i == 1) { A = a;          T = t; }
    else        { A = gadd(A, a); T = vecmul(T, t); }
  }
  *emb = T; return A;
}

static GEN
scalar_get_arch_real(long R1, long RU, GEN u, GEN *emb, long prec)
{
  GEN v, x, p1;
  long i, s;

  s = gsigne(u);
  if (!s) pari_err(talker,"0 in get_arch_real");
  x = cgetg(RU+1, t_COL);
  for (i=1; i<=RU; i++) gel(x,i) = u;

  v = cgetg(RU+1, t_COL);
  if (s < 0) u = gneg(u);
  p1 = glog(u,prec);
  for (i=1; i<=R1; i++) gel(v,i) = p1;
  if (i <= RU)
  {
    p1 = gmul2n(p1,1);
    for (   ; i<=RU; i++) gel(v,i) = p1;
  }
  *emb = x; return v;
}

static int
low_prec(GEN x)
{
  return gcmp0(x) || (typ(x) == t_REAL && lg(x) == 3);
}

/* For internal use. Get archimedean components: [e_i log( | sigma_i(x) | )],
 * with e_i = 1 (resp 2.) for i <= R1 (resp. > R1)
 * Return NULL if precision problem, and set *emb to the embeddings of x */
GEN
get_arch_real(GEN nf, GEN x, GEN *emb, long prec)
{
  long i, RU, R1;
  GEN v, t;

  if (typ(x) == t_MAT) return famat_get_arch_real(nf,x,emb,prec);
  x = nf_to_scalar_or_basis(nf,x);
  RU = lg(nf[6])-1;
  R1 = nf_get_r1(nf);
  if (typ(x) != t_COL) return scalar_get_arch_real(R1, RU, x, emb, prec);
  x = RgM_RgC_mul(gmael(nf,5,1), x);
  v = cgetg(RU+1,t_COL);
  for (i=1; i<=R1; i++)
  {
    t = gabs(gel(x,i),prec); if (low_prec(t)) return NULL;
    gel(v,i) = glog(t,prec);
  }
  for (   ; i<=RU; i++)
  {
    t = gnorm(gel(x,i)); if (low_prec(t)) return NULL;
    gel(v,i) = glog(t,prec);
  }
  *emb = x; return v;
}

/* GP functions */

GEN
ideal_two_elt0(GEN nf, GEN x, GEN a)
{
  if (!a) return ideal_two_elt(nf,x);
  return ideal_two_elt2(nf,x,a);
}

GEN
idealpow0(GEN nf, GEN x, GEN n, long flag, long prec)
{
  if (flag) return idealpowred(nf,x,n,prec);
  return idealpow(nf,x,n);
}

GEN
idealmul0(GEN nf, GEN x, GEN y, long flag, long prec)
{
  if (flag) return idealmulred(nf,x,y,prec);
  return idealmul(nf,x,y);
}

GEN
idealdiv0(GEN nf, GEN x, GEN y, long flag)
{
  switch(flag)
  {
    case 0: return idealdiv(nf,x,y);
    case 1: return idealdivexact(nf,x,y);
    default: pari_err(flagerr,"idealdiv");
  }
  return NULL; /* not reached */
}

GEN
idealaddtoone0(GEN nf, GEN arg1, GEN arg2)
{
  if (!arg2) return idealaddmultoone(nf,arg1);
  return idealaddtoone(nf,arg1,arg2);
}

static GEN
hnf_Z_QC(GEN nf, GEN a, GEN b)
{
  GEN db;
  b = Q_remove_denom(b, &db);
  if (db) a = mulii(a, db);
  b = zk_scalar_or_multable(nf, b);
  if (typ(b) == t_INT) {
    b = gcdii(b,a);
    return db? mkfraccopy(b, db): a;
  } else {
    b = hnfmodid(b, a);
    return db? RgM_Rg_div(b, db): b;
  }
}
static GEN
hnf_Q_QC(GEN nf, GEN a, GEN b)
{
  GEN db, da;

  if (typ(a) == t_INT) return hnf_Z_QC(nf, a, b);
  da = gel(a,2);
  a = gel(a,1);
  b = Q_remove_denom(b, &db);
  b = ZC_Z_mul(b, da);
  if (db) {
    GEN d = gcdii(da,db);
    db = diviiexact(db,d);
    a = mulii(a, db);
    da = mulii(da, db); /* lcm(denom(a),denom(b)) */
  }
  b = zk_scalar_or_multable(nf, b);
  if (typ(b) == t_INT) return gdiv(gcdii(b, a), da);
  return RgM_Rg_div(hnfmodid(b, a), da);
}
static GEN
hnf_QC_QC(GEN nf, GEN a, GEN b)
{
  GEN da, db, d, x;
  a = Q_remove_denom(a, &da);
  b = Q_remove_denom(b, &db);
  if (da) b = ZC_Z_mul(b, da);
  if (db) a = ZC_Z_mul(a, db);
  d = mul_denom(da, db);
  x = shallowconcat(zk_multable(nf,a), zk_multable(nf,b));
  x = ZM_hnfmod(x, ZM_detmult(x));
  return d? RgM_Rg_div(x, d): x;
}
static GEN
hnf_Q_Q(GEN nf, GEN a, GEN b) {return scalarmat(ggcd(a,b), degpol(gel(nf,1)));}
GEN
idealhnf0(GEN nf, GEN a, GEN b)
{
  long ta, tb;
  pari_sp av;
  GEN x;
  if (!b) return idealhnf(nf,a);

  /* HNF of aZ_K+bZ_K */
  av = avma; nf = checknf(nf);
  a = nf_to_scalar_or_basis(nf,a); ta = typ(a);
  b = nf_to_scalar_or_basis(nf,b); tb = typ(b);
  if (ta == t_COL)
    x = (tb==t_COL)? hnf_QC_QC(nf, a,b): hnf_Q_QC(nf, b,a);
  else
    x = (tb==t_COL)? hnf_Q_QC(nf, a,b): hnf_Q_Q(nf, a,b);
  return gerepileupto(av, x);
}

/*******************************************************************/
/*                                                                 */
/*                       TWO-ELEMENT FORM                          */
/*                                                                 */
/*******************************************************************/
static GEN idealapprfact_i(GEN nf, GEN x, int nored);

static int
ok_elt(GEN x, GEN xZ, GEN y)
{
  pari_sp av = avma;
  int r = ZM_equal(x, ZM_hnfmodid(y, xZ));
  avma = av; return r;
}

static GEN
addmul_col(GEN a, long s, GEN b)
{
  long i,l;
  if (!s) return a? shallowcopy(a): a;
  if (!a) return gmulsg(s,b);
  l = lg(a);
  for (i=1; i<l; i++)
    if (signe(b[i])) gel(a,i) = addii(gel(a,i), mulsi(s, gel(b,i)));
  return a;
}

/* a <-- a + s * b, all coeffs integers */
static GEN
addmul_mat(GEN a, long s, GEN b)
{
  long j,l;
  if (!s) return a? shallowcopy(a): a; /* copy otherwise next call corrupts a */
  if (!a) return gmulsg(s,b);
  l = lg(a);
  for (j=1; j<l; j++)
    (void)addmul_col(gel(a,j), s, gel(b,j));
  return a;
}

static GEN
get_random_a(GEN nf, GEN x, GEN xZ)
{
  pari_sp av1;
  long i, lm, l = lg(x);
  GEN a, z, beta, mul;

  beta= cgetg(l, t_VEC);
  mul = cgetg(l, t_VEC); lm = 1; /* = lg(mul) */
  /* look for a in x such that a O/xZ = x O/xZ */
  for (i = 2; i < l; i++)
  {
    GEN t, y, xi = gel(x,i);
    av1 = avma;
    y = zk_scalar_or_multable(nf, xi); /* ZM, cannot be a scalar */
    t = FpM_red(y, xZ);
    if (gcmp0(t)) { avma = av1; continue; }
    if (ok_elt(x,xZ, t)) return xi;
    gel(beta,lm) = xi;
    /* mul[i] = { canonical generators for x[i] O/xZ as Z-module } */
    gel(mul,lm) = t; lm++;
  }
  setlg(mul, lm);
  setlg(beta,lm);
  z = cgetg(lm, t_VECSMALL);
  for(av1=avma;;avma=av1)
  {
    for (a=NULL,i=1; i<lm; i++)
    {
      long t = random_bits(4) - 7; /* in [-7,8] */
      z[i] = t;
      a = addmul_mat(a, t, gel(mul,i));
    }
    /* a = matrix (NOT HNF) of ideal generated by beta.z in O/xZ */
    if (a && ok_elt(x,xZ, a)) break;
  }
  for (a=NULL,i=1; i<lm; i++)
    a = addmul_col(a, z[i], gel(beta,i));
  return a;
}

/* if x square matrix, assume it is HNF */
static GEN
mat_ideal_two_elt(GEN nf, GEN x)
{
  GEN y, a, cx, xZ;
  long N = degpol(nf[1]);
  pari_sp av, tetpil;

  if (N == 2) return mkvec2copy(gcoeff(x,1,1), gel(x,2));

  y = cgetg(3,t_VEC); av = avma;
  x = Q_primitive_part(x, &cx); if (!cx) cx = gen_1;
  xZ = gcoeff(x,1,1);
  if (gcmp1(xZ))
  {
    cx = gerepilecopy(av,cx);
    gel(y,1) = cx;
    gel(y,2) = scalarcol_shallow(gen_0, N); return y;
  }
  if (N < 6)
    a = get_random_a(nf, x, xZ);
  else
  {
    const long lim = 47;
    GEN a1, fa = auxdecomp(xZ, lim), P = gel(fa,1), E = gel(fa,2);
    long l = lg(P)-1;

    a1 = powgi(gel(P, l), gel(E, l));
    if (cmpis(a1, lim) <= 0)
      a = idealapprfact_i(nf, idealfactor(nf,x), 1);
    else if (equalii(xZ, a1))
      a = get_random_a(nf, x, xZ);
    else
    {
      GEN A0, A1, a0, u0, u1, v0, v1, pi0, pi1, t, u;
      a0 = diviiexact(xZ, a1);
      A0 = ZM_hnfmodid(x, a0); /* smooth part of x */
      A1 = ZM_hnfmodid(x, a1); /* cofactor */
      pi0 = idealapprfact_i(nf, idealfactor(nf,A0), 1);
      pi1 = get_random_a(nf, A1, a1);
      (void)bezout(a0, a1, &v0,&v1);
      u0 = mulii(a0, v0);
      u1 = mulii(a1, v1);
      t = ZC_Z_mul(pi0, u1); gel(t,1) = addii(gel(t,1), u0);
      u = ZC_Z_mul(pi1, u0); gel(u,1) = addii(gel(u,1), u1);
      a = element_muli(nf, centermod(u, xZ), centermod(t, xZ));
    }
  }
  a = centermod(a, xZ);
  tetpil = avma;
  gel(y,1) = gmul(xZ, cx);
  gel(y,2) = RgC_Rg_mul(a, cx);
  gerepilecoeffssp(av,tetpil,y+1,2); return y;
}

/* Given an ideal x, returns [a,alpha] such that a is in Q,
 * x = a Z_K + alpha Z_K, alpha in K^*
 * a = 0 or alpha = 0 are possible, but do not try to determine whether
 * x is principal. */
GEN
ideal_two_elt(GEN nf, GEN x)
{
  GEN z;
  long N, tx = idealtyp(&x,&z);

  nf = checknf(nf);
  if (tx == id_MAT) return mat_ideal_two_elt(nf,x);
  if (tx == id_PRIME) return mkvec2copy(gel(x,1), gel(x,2));
  /* id_PRINCIPAL */

  N = degpol(nf[1]); z = cgetg(3,t_VEC);
  switch(typ(x))
  {
    case t_INT: case t_FRAC:
      gel(z,1) = gcopy(x);
      gel(z,2) = zerocol(N); return z;

    case t_POLMOD:
      x = checknfelt_mod(nf, x, "ideal_two_elt"); /* fall through */
    case t_POL:
      gel(z,1) = gen_0;
      gel(z,2) = algtobasis(nf,x); return z;
    case t_COL:
      if (lg(x)==N+1) {
        gel(z,1) = gen_0;
        gel(z,2) = gcopy(x); return z;
      }
  }
  pari_err(typeer,"ideal_two_elt");
  return NULL; /* not reached */
}

/*******************************************************************/
/*                                                                 */
/*                         FACTORIZATION                           */
/*                                                                 */
/*******************************************************************/
/* x integral ideal in HNF, return v_p(Nx), *vz = v_p(x \cap Z)
 * Use x[1,1] = x \cap Z */
long
val_norm(GEN x, GEN p, long *vz)
{
  long i,l = lg(x), v;
  *vz = v = Z_pval(gcoeff(x,1,1), p);
  if (!v) return 0;
  for (i=2; i<l; i++) v += Z_pval(gcoeff(x,i,i), p);
  return v;
}

/* return factorization of Nx, x in HNF */
GEN
factor_norm(GEN x)
{
  GEN f = factor(gcoeff(x,1,1)), p = gel(f,1), e = gel(f,2);
  long i,k, l = lg(p);
  for (i=1; i<l; i++) e[i] = val_norm(x,gel(p,i), &k);
  settyp(e, t_VECSMALL); return f;
}

GEN
idealfactor(GEN nf, GEN x)
{
  pari_sp av;
  long tx, i, j, k, lf, lc, N, v, vc, e;
  GEN X, f, f1, f2, c1, c2, y1, y2, y, p1, cx, P;

  tx = idealtyp(&x,&y);
  if (tx == id_PRIME)
  {
    y = cgetg(3,t_MAT);
    gel(y,1) = mkcolcopy(x);
    gel(y,2) = mkcol(gen_1); return y;
  }
  av = avma; nf = checknf(nf);
  if (tx == id_PRINCIPAL)
  {
    x = nf_to_scalar_or_basis(nf, x);
    if (typ(x) != t_COL)
    {
      long lfa;
      f = factor(Q_abs(x));
      c1 = gel(f,1); lfa = lg(c1);
      if (lfa == 1) { avma = av; return trivfact(); }
      c2 = gel(f,2);
      settyp(c1, t_VEC); /* for shallowconcat */
      settyp(c2, t_VEC); /* for shallowconcat */
      for (i = 1; i < lfa; i++)
      {
	GEN P = primedec(nf, gel(c1,i)), E = gel(c2,i), z;
	long lP = lg(P);
	z = cgetg(lP, t_COL);
	for (j = 1; j < lP; j++) gel(z,j) = mulii(gmael(P,j,3), E);
        gel(c1,i) = P;
        gel(c2,i) = z;
      }
      c1 = shallowconcat1(c1); settyp(c1, t_COL);
      c2 = shallowconcat1(c2);
      gel(f,1) = c1;
      gel(f,2) = c2; return gerepilecopy(av, f);
    }
    /* faster valuations for principal ideal x than for X = x in HNF form */
    x = Q_primitive_part(x, &cx);
    X = idealhnf_principal(nf, x);
  }
  else /* id_MAT */
  {
    x = Q_primitive_part(x, &cx);
    X = x;
  }
  N = lg(X)-1; if (!N) pari_err(talker,"zero ideal in idealfactor");
  if (!cx)
  {
    c1 = c2 = NULL; /* gcc -Wall */
    lc = 1;
  }
  else
  {
    f = factor(cx);
    c1 = gel(f,1);
    c2 = gel(f,2); lc = lg(c1);
  }
  f = factor_norm(X);
  f1 = gel(f,1);
  f2 = gel(f,2); lf = lg(f1);
  y1 = cgetg((lf+lc-2)*N+1, t_COL);
  y2 = cgetg((lf+lc-2)*N+1, t_VECSMALL);
  k = 1;
  for (i=1; i<lf; i++)
  {
    long l = f2[i]; /* = v_p(Nx) */
    p1 = primedec(nf,gel(f1,i));
    vc = cx? Q_pval(cx,gel(f1,i)): 0;
    for (j=1; j<lg(p1); j++)
    {
      P = gel(p1,j); e = itos(gel(P,3));
      v = idealval(nf,x,P);
      l -= v*itos(gel(P,4));
      v += vc*e; if (!v) continue;
      gel(y1,k) = P;
      y2[k] = v; k++;
      if (l == 0) break; /* now only the content contributes */
    }
    if (vc == 0) continue;
    for (j++; j<lg(p1); j++)
    {
      P = gel(p1,j); e = itos(gel(P,3));
      gel(y1,k) = P;
      y2[k] = vc*e; k++;
    }
  }
  for (i=1; i<lc; i++)
  {
    /* p | Nx already treated */
    if (dvdii(gcoeff(X,1,1),gel(c1,i))) continue;
    p1 = primedec(nf,gel(c1,i));
    vc = itos(gel(c2,i));
    for (j=1; j<lg(p1); j++)
    {
      P = gel(p1,j); e = itos(gel(P,3));
      gel(y1,k) = P;
      y2[k] = vc*e; k++;
    }
  }
  setlg(y1, k);
  setlg(y2, k); y = gerepilecopy(av, mkmat2(y1,y2));
  y2 = gel(y,2); for (i=1; i<k; i++) gel(y2,i) = stoi(y2[i]);
  settyp(y2, t_COL);
  return sort_factor(y, (void*)&cmp_prime_ideal, &cmp_nodata);
}

/* P prime ideal in primedec format. Return valuation(ix) at P */
long
idealval(GEN nf, GEN ix, GEN P)
{
  pari_sp av = avma, av1, lim;
  long N, vmax, vd, v, e, f, i, j, k, tx = typ(ix);
  GEN mul, B, a, x, y, r, t0, p, pk, cx, vals;
  int do_mul;

  if (is_extscalar_t(tx) || tx==t_COL) return element_val(nf,ix,P);
  tx = idealtyp(&ix,&a);
  if (tx == id_PRINCIPAL) return element_val(nf,ix,P);
  checkprid(P);
  p = gel(P,1);
  if (tx == id_PRIME) {
    if (!equalii(p, gel(ix,1))) return 0;
    return (ZV_equal(gel(P,2), gel(ix,2))
	 || element_val(nf, gel(ix,2), P))? 1: 0;
  }
  /* id_MAT */
  nf = checknf(nf);
  N = lg(ix)-1; if (!N) pari_err(talker,"zero ideal in idealval");
  ix = Q_primitive_part(ix, &cx);
  i = val_norm(ix,p, &k);
  if (!i) { avma = av; return cx? itos(gel(P,3)) * Q_pval(cx,p): 0; }

  e = itos(gel(P,3));
  f = itos(gel(P,4));
  vd = cx? e * Q_pval(cx,p): 0;
  /* 0 <= ceil[v_P(ix) / e] <= v_p(ix \cap Z) --> v_P <= e * v_p */
  j = k * e;
  /* 0 <= v_P(ix) <= floor[v_p(Nix) / f] */
  i = i / f;
  vmax = min(i,j); /* v_P(ix) <= vmax */

  t0 = gel(P,5);
  if (typ(t0) == t_MAT)
  { /* t0 given by a multiplication table */
    mul = t0;
    do_mul = 0;
  }
  else
  {
    mul = cgetg(N+1,t_MAT);
    gel(mul,1) = t0;
    do_mul = 1;
  }
  B = cgetg(N+1,t_MAT);
  pk = powiu(p, (ulong)ceil((double)vmax / e));
  /* B[1] not needed: v_pr(ix[1]) = v_pr(ix \cap Z) is known already */
  gel(B,1) = gen_0; /* dummy */
  for (j=2; j<=N; j++)
  {
    if (do_mul) gel(mul,j) = elementi_mulid(nf,t0,j);
    x = gel(ix,j);
    y = cgetg(N+1, t_COL); gel(B,j) = y;
    for (i=1; i<=N; i++)
    { /* compute a = (x.t0)_i, ix in HNF ==> x[j+1..N] = 0 */
      a = mulii(gel(x,1), gcoeff(mul,i,1));
      for (k=2; k<=j; k++) a = addii(a, mulii(gel(x,k), gcoeff(mul,i,k)));
      /* p | a ? */
      gel(y,i) = dvmdii(a,p,&r);
      if (signe(r)) { avma = av; return vd; }
    }
  }
  vals = cgetg(N+1, t_VECSMALL);
  /* vals[1] not needed */
  for (j = 2; j <= N; j++)
  {
    gel(B,j) = Q_primitive_part(gel(B,j), &cx);
    vals[j] = cx? 1 + e * Q_pval(cx, p): 1;
  }
  av1 = avma; lim = stack_lim(av1,3);
  y = cgetg(N+1,t_COL);
  /* can compute mod p^ceil((vmax-v)/e) */
  for (v = 1; v < vmax; v++)
  { /* we know v_pr(Bj) >= v for all j */
    if (e == 1 || (vmax - v) % e == 0) pk = diviiexact(pk, p);
    for (j = 2; j <= N; j++)
    {
      x = gel(B,j); if (v < vals[j]) continue;
      for (i=1; i<=N; i++)
      {
	pari_sp av2 = avma;
	a = mulii(gel(x,1), gcoeff(mul,i,1));
	for (k=2; k<=N; k++) a = addii(a, mulii(gel(x,k), gcoeff(mul,i,k)));
	/* a = (x.t_0)_i; p | a ? */
	a = dvmdii(a,p,&r);
	if (signe(r)) { avma = av; return v + vd; }
	if (lgefint(a) > lgefint(pk)) a = remii(a, pk);
	gel(y,i) = gerepileuptoint(av2, a);
      }
      gel(B,j) = y; y = x;
      if (low_stack(lim,stack_lim(av1,3)))
      {
	if(DEBUGMEM>1) pari_warn(warnmem,"idealval");
	gerepileall(av1,3, &y,&B,&pk);
      }
    }
  }
  avma = av; return v + vd;
}

/* gcd and generalized Bezout */

GEN
idealadd(GEN nf, GEN x, GEN y)
{
  pari_sp av = avma;
  long tx, ty;
  GEN z, a, dx, dy, dz;

  tx = idealtyp(&x,&z);
  ty = idealtyp(&y,&z); nf = checknf(nf);
  if (tx != id_MAT) x = idealhnf_shallow(nf,x);
  if (ty != id_MAT) y = idealhnf_shallow(nf,y);
  if (lg(x) == 1) return gerepilecopy(av,y);
  if (lg(y) == 1) return gerepilecopy(av,x); /* check for 0 ideal */
  dx = Q_denom(x);
  dy = Q_denom(y); dz = lcmii(dx,dy);
  if (is_pm1(dz)) dz = NULL; else {
    x = Q_muli_to_int(x, dz);
    y = Q_muli_to_int(y, dz);
  }
  a = gcdii(gcoeff(x,1,1), gcoeff(y,1,1));
  if (is_pm1(a))
  {
    long N = lg(x)-1;
    if (!dz) { avma = av; return matid(N); }
    return gerepileupto(av, scalarmat(ginv(dz), N));
  }
  z = ZM_hnfmodid(shallowconcat(x,y), a);
  if (dz) z = RgM_Rg_div(z,dz);
  return gerepileupto(av,z);
}

GEN
idealaddtoone_i(GEN nf, GEN x, GEN y)
{
  GEN a;
  long tx = idealtyp(&x, &a/*junk*/);
  long ty = idealtyp(&y, &a/*junk*/);
  if (tx != id_MAT) x = idealhnf_shallow(nf, x);
  if (ty != id_MAT) y = idealhnf_shallow(nf, y);
  a = hnfmerge_get_1(x, y);
  return reducemodlll(a, idealmul_HNF(nf,x,y));
}

GEN
unnf_minus_x(GEN x)
{
  long i, N = lg(x);
  GEN y = cgetg(N,t_COL);

  gel(y,1) = gsubsg(1, gel(x,1));
  for (i=2; i<N; i++) gel(y,i) = gneg(gel(x,i));
  return y;
}

GEN
idealaddtoone(GEN nf, GEN x, GEN y)
{
  GEN z = cgetg(3,t_VEC), a;
  pari_sp av = avma;
  nf = checknf(nf);
  a = gerepileupto(av, idealaddtoone_i(nf,x,y));
  gel(z,1) = a;
  gel(z,2) = unnf_minus_x(a); return z;
}

/* assume elements of list are integral ideals */
GEN
idealaddmultoone(GEN nf, GEN list)
{
  pari_sp av = avma;
  long N, i, l, tx = typ(list);
  GEN z, H, U, perm, L;

  nf = checknf(nf); N = degpol(nf[1]);
  if (!is_vec_t(tx)) pari_err(talker,"not a vector of ideals in idealaddmultoone");
  l = lg(list); z = cgetg(1,t_MAT);
  L = cgetg(l, tx);
  if (l == 1) pari_err(talker,"ideals don't sum to Z_K in idealaddmultoone");
  for (i=1; i<l; i++)
  {
    GEN I = gel(list,i);
    if (typ(I) != t_MAT) I = idealhnf_shallow(nf,I);
    if (lg(I) == 1)
      I = zeromat(N,N);
    else
    {
      RgM_check_ZM(I,"idealaddmultoone");
      if (lg(gel(I,1)) != N+1)
        pari_err(talker,"%Zs: not an ideal in idealaddmultoone", I);
    }
    gel(L,i) = I; z = shallowconcat(z, I);
  }
  H = ZM_hnfperm(z, &U, &perm);
  if (lg(H) == 1 || !gcmp1(gcoeff(H,1,1)))
    pari_err(talker,"ideals don't sum to Z_K in idealaddmultoone");
  for (i=1; i<=N; i++)
    if (perm[i] == 1) break;
  U = gel(U,(l-2)*N + i); /* z U = 1 */
  for (i=1; i<l; i++)
    gel(L,i) = ZM_ZC_mul(gel(L,i), vecslice(U, (i-1)*N + 1, i*N));
  return gerepilecopy(av, L);
}

/* multiplication */

/* x integral ideal (without archimedean component) in HNF form
 * y = [a,alpha] corresponds to the integral ideal aZ_K+alpha Z_K, a in Z,
 * alpha a ZV or a ZM (multiplication table). Multiply them */
static GEN
idealmul_HNF_two(GEN nf, GEN x, GEN y)
{
  GEN m, a = gel(y,1), alpha = gel(y,2);
  long i, N;

  if (typ(alpha) != t_MAT)
  {
    alpha = zk_scalar_or_multable(nf, alpha);
    if (typ(alpha) == t_INT) /* e.g. y inert ? 0 should not (but may) occur */
      return signe(a)? ZM_Z_mul(x, gcdii(a, alpha)): cgetg(1,t_MAT);
  }
  N = lg(x)-1; m = cgetg((N<<1)+1,t_MAT);
  for (i=1; i<=N; i++) gel(m,i)   = ZM_ZC_mul(alpha,gel(x,i));
  for (i=1; i<=N; i++) gel(m,i+N) = ZC_Z_mul(gel(x,i), a);
  return ZM_hnfmodid(m, mulii(a, gcoeff(x,1,1)));
}

/* x ideal (matrix form,maximal rank), vp prime ideal (primedec). Output the
 * product. Can be used for arbitrary vp of the form [p,a,e,f,b], IF vp
 * =pZ_K+aZ_K, p is an integer, and norm(vp) = p^f; e and b are not used.
 * For internal use. */
GEN
idealmulprime(GEN nf, GEN x, GEN vp)
{
  GEN cx;
  x = Q_primitive_part(x, &cx);
  x = idealmul_HNF_two(nf,x,vp);
  return cx? RgM_Rg_mul(x,cx): x;
}

/* Assume ix and iy are integral in HNF form [NOT extended]. Not memory clean.
 * HACK: ideal in iy can be of the form [a,b], a in Z, b in Z_K */
GEN
idealmul_HNF(GEN nf, GEN x, GEN y)
{
  GEN z;
  if (typ(y) == t_VEC)
    z = idealmul_HNF_two(nf,x,y);
  else
  { /* reduce one ideal to two-elt form. The smallest */
    GEN xZ = gcoeff(x,1,1), yZ = gcoeff(y,1,1);
    if (cmpii(xZ, yZ) < 0)
    {
      if (is_pm1(xZ)) return gcopy(y);
      z = idealmul_HNF_two(nf, y, mat_ideal_two_elt(nf,x));
    }
    else
    {
      if (is_pm1(yZ)) return gcopy(x);
      z = idealmul_HNF_two(nf, x, mat_ideal_two_elt(nf,y));
    }
  }
  return z;
}

/* operations on elements in factored form */
GEN
to_famat(GEN g, GEN e)
{
  if (typ(g) != t_COL) { g = shallowcopy(g); settyp(g, t_COL); }
  if (typ(e) != t_COL) { e = shallowcopy(e); settyp(e, t_COL); }
  return mkmat2(g, e);
}

GEN
to_famat_all(GEN x, GEN y) { return to_famat(mkcol(x), mkcol(y)); }

static GEN
append(GEN v, GEN x)
{
  long i, l = lg(v);
  GEN w = cgetg(l+1, typ(v));
  for (i=1; i<l; i++) gel(w,i) = gcopy(gel(v,i));
  gel(w,i) = gcopy(x); return w;
}

/* add x^1 to factorisation f */
static GEN
famat_add(GEN f, GEN x)
{
  GEN h = cgetg(3,t_MAT);
  if (lg(f) == 1)
  {
    gel(h,1) = mkcolcopy(x);
    gel(h,2) = mkcol(gen_1);
  }
  else
  {
    gel(h,1) = append(gel(f,1), x); /* x may be a t_COL */
    gel(h,2) = concat(gel(f,2), gen_1);
  }
  return h;
}

/* cf merge_factor_i */
GEN
famat_mul(GEN f, GEN g)
{
  GEN h;
  if (typ(g) != t_MAT) return famat_add(f, g);
  if (lg(f) == 1) return gcopy(g);
  if (lg(g) == 1) return gcopy(f);
  h = cgetg(3,t_MAT);
  gel(h,1) = concat(gel(f,1), gel(g,1));
  gel(h,2) = concat(gel(f,2), gel(g,2));
  return h;
}

static GEN
famat_sqr(GEN f)
{
  GEN h;
  if (lg(f) == 1) return cgetg(1,t_MAT);
  h = cgetg(3,t_MAT);
  gel(h,1) = gcopy(gel(f,1));
  gel(h,2) = gmul2n(gel(f,2),1);
  return h;
}
GEN
famat_inv(GEN f)
{
  GEN h;
  if (lg(f) == 1) return cgetg(1,t_MAT);
  h = cgetg(3,t_MAT);
  gel(h,1) = gcopy(gel(f,1));
  gel(h,2) = gneg(gel(f,2));
  return h;
}
GEN
famat_pow(GEN f, GEN n)
{
  GEN h;
  if (lg(f) == 1) return cgetg(1,t_MAT);
  if (typ(f) != t_MAT) return to_famat_all(f,n);
  h = cgetg(3,t_MAT);
  gel(h,1) = gcopy(gel(f,1));
  gel(h,2) = ZC_Z_mul(gel(f,2),n);
  return h;
}

/* x assumed to be a t_MATs (factorization matrix), or compatible with
 * the element_* functions. */
static GEN
ext_mul(GEN nf, GEN x, GEN y) {
  if (typ(x) == t_MAT) return (x == y)? famat_sqr(x): famat_mul(x,y);
  return element_mul(nf, x, y);
}
static GEN
ext_inv(GEN nf, GEN x) {
  if (typ(x) == t_MAT) return famat_inv(x);
  return element_inv(nf, x);
}
static GEN
ext_pow(GEN nf, GEN x, GEN n) {
  if (typ(x) == t_MAT) return famat_pow(x,n);
  return element_pow(nf, x, n);
}

/* x, y 2 extended ideals whose first component is an integral HNF */
GEN
extideal_HNF_mul(GEN nf, GEN x, GEN y)
{
  return mkvec2(idealmul_HNF(nf, gel(x,1), gel(y,1)),
                ext_mul(nf, gel(x,2), gel(y,2)));
}

GEN
mul_content(GEN cx, GEN cy)
{
  if (!cx) return cy;
  if (!cy) return cx;
  return gmul(cx,cy);
}
GEN
mul_denom(GEN dx, GEN dy)
{
  if (!dx) return dy;
  if (!dy) return dx;
  return mulii(dx,dy);
}

/* x and y are ideals in non-empty matrix form */
static GEN
idealmat_mul(GEN nf, GEN x, GEN y)
{
  GEN cx, cy;
  x = Q_primitive_part(x, &cx);
  y = Q_primitive_part(y, &cy); cx = mul_content(cx,cy);
  y = idealmul_HNF(nf,x,y);
  return cx? ZM_Q_mul(y,cx): y;
}

GEN
famat_to_nf(GEN nf, GEN f)
{
  GEN t, x, e;
  long i;
  if (lg(f) == 1) return gen_1;

  x = gel(f,1);
  e = gel(f,2);
  t = element_pow(nf, gel(x,1), gel(e,1));
  for (i=lg(x)-1; i>1; i--)
    t = element_mul(nf, t, element_pow(nf, gel(x,i), gel(e,i)));
  return t;
}

/* "compare" two nf elt. Goal is to quickly sort for uniqueness of
 * representation, not uniqueness of represented element ! */
static int
elt_cmp(GEN x, GEN y)
{
  long tx = typ(x), ty = typ(y);
  if (ty == tx)
    return (tx == t_POL || tx == t_POLMOD)? cmp_RgX(x,y): lexcmp(x,y);
  return tx - ty;
}
static int
elt_egal(GEN x, GEN y)
{
  if (typ(x) == typ(y)) return gequal(x,y);
  return 0;
}

GEN
famat_reduce(GEN fa)
{
  GEN E, G, L, g, e;
  long i, k, l;

  if (lg(fa) == 1) return fa;
  g = gel(fa,1); l = lg(g);
  e = gel(fa,2);
  L = gen_indexsort(g, (void*)&elt_cmp, &cmp_nodata);
  G = cgetg(l, t_COL);
  E = cgetg(l, t_COL);
  /* merge */
  for (k=i=1; i<l; i++,k++)
  {
    G[k] = g[L[i]];
    E[k] = e[L[i]];
    if (k > 1 && elt_egal(gel(G,k), gel(G,k-1)))
    {
      gel(E,k-1) = addii(gel(E,k), gel(E,k-1));
      k--;
    }
  }
  /* kill 0 exponents */
  l = k;
  for (k=i=1; i<l; i++)
    if (!gcmp0(gel(E,i)))
    {
      G[k] = G[i];
      E[k] = E[i]; k++;
    }
  setlg(G, k);
  setlg(E, k); return mkmat2(G,E);
}

/* assume pr has degree 1 and coprime to numerator(x) */
static GEN
nf_to_Fp_simple(GEN x, GEN modpr, GEN p)
{
  GEN c, r = zk_to_Fq(Q_primitive_part(x, &c), modpr);
  if (c) r = Rg_to_Fp(gmul(r, c), p);
  return r;
}

static GEN
famat_to_Fp_simple(GEN nf, GEN x, GEN modpr, GEN p)
{
  GEN h, n, t = gen_1, g = gel(x,1), e = gel(x,2), q = subis(p,1);
  long i, l = lg(g);

  for (i=1; i<l; i++)
  {
    n = gel(e,i); n = modii(n,q);
    if (!signe(n)) continue;

    h = gel(g,i);
    switch(typ(h))
    {
      case t_POL: case t_POLMOD: h = algtobasis(nf, h);  /* fall through */
      case t_COL: h = nf_to_Fp_simple(h, modpr, p); break;
      default: h = Rg_to_Fp(h, p);
    }
    t = mulii(t, Fp_pow(h, n, p)); /* not worth reducing */
  }
  return modii(t, p);
}

/* cf famat_to_nf_modideal_coprime, but id is a prime of degree 1 (=pr) */
GEN
to_Fp_simple(GEN nf, GEN x, GEN pr)
{
  GEN T, p, modpr = zk_to_Fq_init(nf, &pr, &T, &p);
  switch(typ(x))
  {
    case t_COL: return nf_to_Fp_simple(x,modpr,p);
    case t_MAT: return famat_to_Fp_simple(nf,x,modpr,p);
    default: pari_err(impl,"generic conversion to finite field");
  }
  return NULL;
}

/* Compute t = prod g[i]^e[i] mod pr^k, assuming (t, pr) = 1.
 * Method: modify each g[i] so that it becomes coprime to pr :
 *  x / (p^k u) --> x * (b/p)^v_pr(x) / z^k u, where z = b^e/p^(e-1)
 * b/p = vp^(-1) times something prime to p; both numerator and denominator
 * are integral and coprime to pr.  Globally, we multiply by (b/p)^v_pr(t) = 1.
 *
 * EX = multiple of exponent of (O_K / pr^k)^* used to reduce the product in
 * case the e[i] are large */
GEN
famat_makecoprime(GEN nf, GEN g, GEN e, GEN pr, GEN prk, GEN EX)
{
  long i, l = lg(g);
  GEN prkZ, u, p = gel(pr,1), b = gel(pr,5);
  GEN vden = gen_0;
  GEN mul = zk_scalar_or_multable(nf, b);
  GEN newg = cgetg(l+1, t_VEC); /* room for z */

  prkZ = gcoeff(prk, 1,1);
  for (i=1; i < l; i++)
  {
    GEN dx, x = nf_to_scalar_or_basis(nf, gel(g,i));
    long vdx = 0;
    x = Q_remove_denom(x, &dx);
    if (dx)
    {
      vdx = Z_pvalrem(dx, p, &u);
      if (!is_pm1(u))
      { /* could avoid the inversion, but prkZ is small--> cheap */
        u = Fp_inv(u, prkZ);
	x = typ(x) == t_INT? mulii(x,u): ZC_Z_mul(x, u);
      }
      if (vdx) vden = addii(vden, mului(vdx, gel(e,i)));
    }
    if (typ(x) == t_INT) {
      if (!vdx) vden = subii(vden, mului(Z_pvalrem(x, p, &x), gel(e,i)));
    } else {
      (void)int_elt_val(nf, x, p, mul, &x);
      x =  ZC_hnfrem(x, prk);
    }
    gel(newg,i) = x;
  }
  if (vden == gen_0) setlg(newg, l);
  else
  {
    gel(newg,i) = FpC_red(special_anti_uniformizer(nf, pr), prkZ);
    e = shallowconcat(e, negi(vden));
  }
  return famat_to_nf_modideal_coprime(nf, newg, e, prk, EX);
}

/* prod g[i]^e[i] mod bid, assume (g[i], id) = 1 */
GEN
famat_to_nf_moddivisor(GEN nf, GEN g, GEN e, GEN bid)
{
  GEN t,sarch,module,cyc,fa2;
  long lc;
  if (lg(g) == 1) return scalarcol_shallow(gen_1, degpol(nf[1])); /* 1 */
  module = gel(bid,1);
  fa2 = gel(bid,4); sarch = gel(fa2,lg(fa2)-1);
  cyc = gmael(bid,2,2); lc = lg(cyc);
  t = NULL;
  if (lc != 1)
  {
    GEN EX = gel(cyc,1); /* group exponent */
    GEN id = gel(module,1);
    t = famat_to_nf_modideal_coprime(nf, g, e, id, EX);
  }
  if (!t) t = gen_1;
  return set_sign_mod_divisor(nf, to_famat(g,e), t, module, sarch);
}

GEN
famat_to_arch(GEN nf, GEN fa, long prec)
{
  GEN g,e, y = NULL;
  long i,l;

  if (typ(fa) != t_MAT) return get_arch(nf, fa, prec);
  if (lg(fa) == 1) return zerovec(lg(nf[6])-1);
  g = gel(fa,1);
  e = gel(fa,2); l = lg(e);
  for (i=1; i<l; i++)
  {
    GEN t, x = nf_to_scalar_or_basis(nf, gel(g,i));
    /* multiplicative arch would be better (save logs), but exponents overflow
     * [ could keep track of expo separately, but not worth it ] */
    t = get_arch(nf,x,prec); if (gel(t,1) == gen_0) continue; /* rational */
    t = RgV_Rg_mul(t, gel(e,i));
    y = y? RgV_add(y,t): t;
  }
  return y ? y: zerovec(lg(nf[6])-1);
}

GEN
vecmul(GEN x, GEN y)
{
  long i,lx, tx = typ(x);
  GEN z;
  if (is_scalar_t(tx)) return gmul(x,y);
  lx = lg(x); z = cgetg(lx,tx);
  for (i=1; i<lx; i++) gel(z,i) = vecmul(gel(x,i), gel(y,i));
  return z;
}

GEN
vecinv(GEN x)
{
  long i,lx, tx = typ(x);
  GEN z;
  if (is_scalar_t(tx)) return ginv(x);
  lx = lg(x); z = cgetg(lx, tx);
  for (i=1; i<lx; i++) gel(z,i) = vecinv(gel(x,i));
  return z;
}

GEN
vecpow(GEN x, GEN n)
{
  long i,lx, tx = typ(x);
  GEN z;
  if (is_scalar_t(tx)) return powgi(x,n);
  lx = lg(x); z = cgetg(lx, tx);
  for (i=1; i<lx; i++) gel(z,i) = vecpow(gel(x,i), n);
  return z;
}

GEN
vecdiv(GEN x, GEN y)
{
  long i,lx, tx = typ(x);
  GEN z;
  if (is_scalar_t(tx)) return gdiv(x,y);
  lx = lg(x); z = cgetg(lx,tx);
  for (i=1; i<lx; i++) gel(z,i) = vecdiv(gel(x,i), gel(y,i));
  return z;
}

/* v ideal as a square t_MAT */
static GEN
idealmulelt(GEN nf, GEN x, GEN v)
{
  GEN cx;
  if (lg(v) == 1) return cgetg(1, t_MAT);
  x = nf_to_scalar_or_basis(nf,x);
  if (typ(x) != t_COL) return gcmp0(x)? cgetg(1,t_MAT): RgM_Rg_mul(v, Q_abs(x));
  x = element_mulvec(nf, x, v);
  x = Q_primitive_part(x, &cx);
  x = ZM_hnfmod(x, ZM_detmult(x));
  return cx? ZM_Q_mul(x,cx): x;
}
int
prime_is_inert(GEN P) { GEN f = gel(P,4); return f[2] == lg(gel(P,2))-1; }

/* tx <= ty */
static GEN
idealmul_aux(GEN nf, GEN x, GEN y, long tx, long ty)
{
  GEN z;
  switch(tx)
  {
    case id_PRINCIPAL:
      switch(ty)
      {
	case id_PRINCIPAL:
	  return idealhnf_principal(nf, element_mul(nf,x,y));
	case id_PRIME:
	{
	  GEN p = gel(y,1), pi = gel(y,2), cx;
          if (prime_is_inert(y)) return RgM_Rg_mul(idealhnf_principal(nf,x),p);

          x = nf_to_scalar_or_basis(nf, x);
          switch(typ(x))
          {
            case t_INT:
              if (!signe(x)) return cgetg(1,t_MAT);
              return ZM_Z_mul(idealhnf_two(nf,y), absi(x));
            case t_FRAC:
              return RgM_Rg_mul(idealhnf_two(nf,y), Q_abs(x));
          }
          /* t_COL */
          x = Q_primitive_part(x, &cx);
          x = zk_multable(nf, x);
	  z = shallowconcat(ZM_Z_mul(x,p), ZM_ZC_mul(x,pi));
          z = ZM_hnfmod(z, ZM_detmult(z));
          return cx? ZM_Q_mul(z, cx): z;
	}
	default: /* id_MAT */
	  return idealmulelt(nf, x,y);
      }
    case id_PRIME:
      if (ty==id_PRIME) y = idealhnf_two(nf,y);
      return idealmulprime(nf,y,x);

    default: /* id_MAT */
      return idealmat_mul(nf,x,y);
  }
}

/* output the ideal product ix.iy (don't reduce) */
GEN
idealmul(GEN nf, GEN x, GEN y)
{
  pari_sp av;
  GEN res, ax, ay, z;
  long tx = idealtyp(&x,&ax);
  long ty = idealtyp(&y,&ay), f;
  if (tx>ty) { swap(ax,ay); swap(x,y); lswap(tx,ty); }
  f = (ax||ay); res = f? cgetg(3,t_VEC): NULL; /*product is an extended ideal*/
  av = avma;
  z = gerepileupto(av, idealmul_aux(checknf(nf), x,y, tx,ty));
  if (!f) return z;
  if (ax && ay)
    ax = ext_mul(nf, ax, ay);
  else
    ax = gcopy(ax? ax: ay);
  gel(res,1) = z; gel(res,2) = ax; return res;
}

/* assume pr in primedec format */
GEN
pr_norm(GEN pr) {
  GEN F = gel(pr,4), p = gel(pr,1);
  return powiu(p, (ulong)F[2]);
}

/* norm of an ideal */
GEN
idealnorm(GEN nf, GEN x)
{
  pari_sp av;
  GEN y;
  long tx;

  switch(idealtyp(&x,&y))
  {
    case id_PRIME: return pr_norm(x);
    case id_MAT: return RgM_det_triangular(x);
  }
  /* id_PRINCIPAL */
  nf = checknf(nf); av = avma;
  x = RgXQ_norm(basistoalg_i(nf,x), gel(nf,1));
  tx = typ(x);
  if (tx == t_INT) return gerepileuptoint(av, absi(x));
  if (tx != t_FRAC) pari_err(typeer, "idealnorm");
  return gerepileupto(av, Q_abs(x));
}

/* inverse */

/* rewritten from original code by P.M & M.H.
 *
 * I^(-1) = { x \in K, Tr(x D^(-1) I) \in Z }, D different of K/Q
 *
 * nf[5][6] = pp( D^(-1) ) = pp( HNF( T^(-1) ) ), T = (Tr(wi wj))
 * nf[5][7] = same in 2-elt form */
static GEN
hnfideal_inv(GEN nf, GEN I)
{
  GEN J, dI, IZ,dual;

  I = Q_remove_denom(I, &dI); IZ = gcoeff(I,1,1); /* I \cap Z */
  J = idealmul_HNF(nf,I, gmael(nf,5,7));
 /* I in HNF, hence easily inverted; multiply by IZ to get integer coeffs
  * missing content cancels while solving the linear equation */
  dual = shallowtrans( gauss_triangle_i(J, gmael(nf,5,6), IZ) );
  dual = ZM_hnfmodid(dual, IZ);
  if (dI) IZ = gdiv(IZ,dI);
  return RgM_Rg_div(dual,IZ);
}

/* return p * P^(-1)  [integral] */
GEN
pidealprimeinv(GEN nf, GEN x)
{
  GEN y;
  if (prime_is_inert(x)) return matid(lg(gel(x,2)) - 1);
  y = cgetg(6,t_VEC); y[1] = x[1]; y[2] = x[5];
  gel(y,3) = gel(y,5) = gen_0;
  gel(y,4) = subsi(degpol(nf[1]), gel(x,4));
  return idealhnf_two(nf,y);
}

/* Calcule le dual de mat_id pour la forme trace */
GEN
idealinv(GEN nf, GEN x)
{
  GEN res,ax;
  pari_sp av = avma;
  long tx = idealtyp(&x,&ax);

  res = ax? cgetg(3,t_VEC): NULL;
  nf = checknf(nf); av = avma;
  switch (tx)
  {
    case id_MAT:
      if (lg(x)-1 != degpol(nf[1])) pari_err(consister,"idealinv");
      x = hnfideal_inv(nf,x); break;
    case id_PRINCIPAL: tx = typ(x);
      if (is_const_t(tx)) x = ginv(x);
      else
      {
	switch(tx)
	{
	  case t_COL: x = coltoliftalg(nf,x); break;
	  case t_POLMOD: x = gel(x,2); break;
	}
	if (typ(x) != t_POL) { x = ginv(x); break; }
	if (varn(x) != varn(nf[1]))
	  pari_err(talker,"incompatible variables in idealinv");
	x = QXQ_inv(x,gel(nf,1));
      }
      x = idealhnf_principal(nf,x); break;
    case id_PRIME:
      x = RgM_Rg_div(pidealprimeinv(nf,x), gel(x,1));
  }
  x = gerepileupto(av,x); if (!ax) return x;
  gel(res,1) = x;
  gel(res,2) = ext_inv(nf, ax); return res;
}

/* Return x such that vp^n = x/d. Assume n != 0 */
static GEN
idealpowprime(GEN nf, GEN vp, GEN n, GEN *d)
{
  GEN n1, x, r;
  long s = signe(n);

  if (s < 0) n = negi(n);
  /* now n > 0 */
  x = shallowcopy(vp);
  if (is_pm1(n)) /* n = 1 special cased for efficiency */
  {
    if (s < 0)
    {
      gel(x,2) = gel(x,5);
      *d = gel(x,1);
    }
    else
      *d = NULL;
  }
  else
  {
    n1 = dvmdii(n, gel(x,3), &r);
    if (signe(r)) n1 = addis(n1,1); /* n1 = ceil(n/e) */
    gel(x,1) = powgi(gel(x,1),n1);
    if (s < 0)
    {
      GEN q = powgi(gel(vp,1),subii(n,n1));
      gel(x,2) = RgC_Rg_div(element_pow(nf,gel(x,5),n), q);
      *d = gel(x,1);
    }
    else
    {
      gel(x,2) = element_pow(nf,gel(x,2),n);
      *d = NULL;
    }
  }
  return x;
}

/* x * vp^n. Assume x in HNF (possibly non-integral) */
GEN
idealmulpowprime(GEN nf, GEN x, GEN vp, GEN n)
{
  GEN cx,y,dx;

  if (!signe(n)) return x;
  nf = checknf(nf);

  /* inert, special cased for efficiency */
  if (prime_is_inert(vp)) return RgM_Rg_mul(x, powgi(gel(vp,1), n));

  y = idealpowprime(nf, vp, n, &dx);
  x = Q_primitive_part(x, &cx);
  if (cx && dx)
  {
    cx = gdiv(cx, dx);
    if (typ(cx) != t_FRAC) dx = NULL;
    else { dx = gel(cx,2); cx = gel(cx,1); }
    if (is_pm1(cx)) cx = NULL;
  }
  x = idealmul_HNF_two(nf,x,y);
  if (cx) x = RgM_Rg_mul(x,cx);
  if (dx) x = RgM_Rg_div(x,dx);
  return x;
}
GEN
idealdivpowprime(GEN nf, GEN x, GEN vp, GEN n)
{
  return idealmulpowprime(nf,x,vp, negi(n));
}

static GEN
idealpow_aux(GEN nf, GEN x, long tx, GEN n)
{
  GEN T = gel(nf,1), m, cx, n1, a, alpha;
  long N = degpol(T), s = signe(n);
  if (!s) return matid(N);
  switch(tx)
  {
    case id_PRINCIPAL: tx = typ(x);
      x = nf_to_scalar_or_alg(nf, x);
      x = (typ(x) == t_POL)? RgXQ_pow(x,n,T): powgi(x,n);
      return idealhnf_principal(nf,x);
    case id_PRIME: {
      GEN d;
      if (prime_is_inert(x)) return scalarmat(powgi(gel(x,1), n), N);
      x = idealpowprime(nf, x, n, &d);
      x = idealhnf_two(nf,x);
      return d? RgM_Rg_div(x, d): x;
    }
    default:
      if (is_pm1(n)) return (s < 0)? idealinv(nf, x): gcopy(x);
      n1 = (s < 0)? negi(n): n;

      x = Q_primitive_part(x, &cx);
      a = mat_ideal_two_elt(nf,x); alpha = gel(a,2); a = gel(a,1);
      alpha = element_pow(nf,alpha,n1);
      m = zk_scalar_or_multable(nf, alpha);
      if (typ(m) == t_INT) {
        x = gcdii(m, powgi(a,n1));
        if (s<0) x = ginv(x);
        if (cx) x = gmul(x, powgi(cx,n));
        x = scalarmat(x, N);
      }
      else {
        x = ZM_hnfmodid(m, powgi(a,n1));
        if (s<0) x = hnfideal_inv(nf,x);
        if (cx) x = RgM_Rg_mul(x, powgi(cx,n));
      }
      return x;
  }
}

/* raise the ideal x to the power n (in Z) */
GEN
idealpow(GEN nf, GEN x, GEN n)
{
  pari_sp av;
  long tx;
  GEN res, ax;

  if (typ(n) != t_INT) pari_err(talker,"non-integral exponent in idealpow");
  tx = idealtyp(&x,&ax);
  res = ax? cgetg(3,t_VEC): NULL;
  av = avma;
  x = gerepileupto(av, idealpow_aux(checknf(nf), x, tx, n));
  if (!ax) return x;
  ax = ext_pow(nf, ax, n);
  gel(res,1) = x;
  gel(res,2) = ax;
  return res;
}

/* Return ideal^e in number field nf. e is a C integer. */
GEN
idealpows(GEN nf, GEN ideal, long e)
{
  long court[] = {evaltyp(t_INT) | _evallg(3),0,0};
  affsi(e,court); return idealpow(nf,ideal,court);
}

static GEN
_idealmulred(GEN nf, GEN x, GEN y, long prec)
{
  return ideallllred(nf,idealmul(nf,x,y), NULL, prec);
}

typedef struct {
  GEN nf;
  long prec;
} idealred_muldata;

static GEN
_mul(void *data, GEN x, GEN y)
{
  idealred_muldata *D = (idealred_muldata *)data;
  return _idealmulred(D->nf,x,y,D->prec);
}
static GEN
_sqr(void *data, GEN x)
{
  return _mul(data,x,x);
}

/* compute x^n (x ideal, n integer), reducing along the way */
GEN
idealpowred(GEN nf, GEN x, GEN n, long prec)
{
  pari_sp av = avma;
  idealred_muldata D;
  long s;
  GEN y;

  if (typ(n) != t_INT) pari_err(talker,"non-integral exponent in idealpowred");
  s = signe(n); if (s == 0) return idealpow(nf,x,n);
  D.nf  = nf;
  D.prec= prec;
  y = leftright_pow(x, n, (void*)&D, &_sqr, &_mul);

  if (s < 0) y = idealinv(nf,y);
  if (s < 0 || is_pm1(n)) y = ideallllred(nf,y,NULL,prec);
  return gerepileupto(av,y);
}

GEN
idealmulred(GEN nf, GEN x, GEN y, long prec)
{
  pari_sp av = avma;
  return gerepileupto(av, _idealmulred(nf,x,y,prec));
}

long
isideal(GEN nf,GEN x)
{
  long N, i, j, lx, tx = typ(x);
  pari_sp av;
  GEN T;

  nf = checknf(nf); T = gel(nf,1); lx = lg(x);
  if (tx==t_VEC && lx==3) { x = gel(x,1); tx = typ(x); lx = lg(x); }
  switch(tx)
  {
    case t_INT: case t_FRAC: return 1;
    case t_POL: return varn(x) == varn(T);
    case t_POLMOD: return RgX_equal_var(T, gel(x,1));
    case t_VEC: return (lx==6);
    case t_MAT: break;
    default: return 0;
  }
  N = degpol(T);
  if (lx-1 != N) return (lx == 1);
  if (lg(x[1])-1 != N) return 0;

  av = avma; x = Q_primpart(x);
  if (!ZM_ishnf(x)) return 0;
  for (i=2; i<=N; i++)
    for (j=2; j<=N; j++)
      if (! hnf_invimage(x, elementi_mulid(nf,gel(x,i),j)))
      {
	avma = av; return 0;
      }
  avma=av; return 1;
}

GEN
idealdiv(GEN nf, GEN x, GEN y)
{
  pari_sp av = avma, tetpil;
  GEN z = idealinv(nf,y);
  tetpil = avma; return gerepile(av,tetpil, idealmul(nf,x,z));
}

/* This routine computes the quotient x/y of two ideals in the number field nf.
 * It assumes that the quotient is an integral ideal.  The idea is to find an
 * ideal z dividing y such that gcd(Nx/Nz, Nz) = 1.  Then
 *
 *   x + (Nx/Nz)    x
 *   ----------- = ---
 *   y + (Ny/Nz)    y
 *
 * Proof: we can assume x and y are integral. Let p be any prime ideal
 *
 * If p | Nz, then it divides neither Nx/Nz nor Ny/Nz (since Nx/Nz is the
 * product of the integers N(x/y) and N(y/z)).  Both the numerator and the
 * denominator on the left will be coprime to p.  So will x/y, since x/y is
 * assumed integral and its norm N(x/y) is coprime to p.
 *
 * If instead p does not divide Nz, then v_p (Nx/Nz) = v_p (Nx) >= v_p(x).
 * Hence v_p (x + Nx/Nz) = v_p(x).  Likewise for the denominators.  QED.
 *
 *		Peter Montgomery.  July, 1994. */
GEN
idealdivexact(GEN nf, GEN x0, GEN y0)
{
  pari_sp av = avma;
  GEN x,y,Nx,Ny,Nz, cy = Q_content(y0);

  nf = checknf(nf);
  if (gcmp0(cy)) pari_err(talker, "cannot invert zero ideal");

  x = gdiv(x0,cy); Nx = idealnorm(nf,x);
  if (gcmp0(Nx)) { avma = av; return gcopy(x0); } /* numerator is zero */

  y = gdiv(y0,cy); Ny = idealnorm(nf,y);
  if (!gcmp1(denom(x)) || !dvdii(Nx,Ny))
    pari_err(talker, "quotient not integral in idealdivexact");
  /* Find a norm Nz | Ny such that gcd(Nx/Nz, Nz) = 1 */
  for (Nz = Ny;;)
  {
    GEN p1 = gcdii(Nz, diviiexact(Nx,Nz));
    if (is_pm1(p1)) break;
    Nz = diviiexact(Nz,p1);
  }
  /* Replace x/y  by  x+(Nx/Nz) / y+(Ny/Nz) */
  x = idealhnf_shallow(nf, x);
  x = ZM_hnfmodid(x, diviiexact(Nx,Nz));
  /* y reduced to unit ideal ? */
  if (Nz == Ny) return gerepileupto(av, x);

  y = idealhnf_shallow(nf, y);
  y = ZM_hnfmodid(y, diviiexact(Ny,Nz));
  y = hnfideal_inv(nf,y);
  return gerepileupto(av, idealmat_mul(nf,x,y));
}

GEN
idealintersect(GEN nf, GEN x, GEN y)
{
  pari_sp av = avma;
  long lz, lx, i;
  GEN z, dx, dy, xZ, yZ;;

  nf = checknf(nf);
  x = idealhnf_shallow(nf,x);
  y = idealhnf_shallow(nf,y);
  if (lg(x) == 1 || lg(y) == 1) { avma = av; return cgetg(1,t_MAT); }
  x = Q_remove_denom(x, &dx); xZ = gcoeff(x,1,1);
  y = Q_remove_denom(y, &dy); yZ = gcoeff(y,1,1);
  if (dx) y = ZM_Z_mul(y, dx);
  if (dy) x = ZM_Z_mul(x, dy);
  dx = mul_denom(dx,dy);
  z = kerint(shallowconcat(x,y)); lz = lg(z);
  lx = lg(x);
  for (i=1; i<lz; i++) setlg(z[i], lx);
  z = ZM_hnfmodid(ZM_mul(x,z), lcmii(xZ, yZ));
  if (dx) z = RgM_Rg_div(z,dx);
  return gerepileupto(av,z);
}

/*******************************************************************/
/*                                                                 */
/*                      T2-IDEAL REDUCTION                         */
/*                                                                 */
/*******************************************************************/

static GEN
chk_vdir(GEN nf, GEN vdir)
{
  long l, i, t;
  GEN v;
  if (!vdir) return NULL;
  l = lg(vdir);
  if (l != lg(nf[6])) pari_err(talker, "incorrect vector length in idealred");
  t = typ(vdir);
  if (t == t_VECSMALL) return vdir;
  if (t != t_VEC) pari_err(typeer, "idealred");
  v = cgetg(l, t_VECSMALL);
  for (i=1; i<l; i++) v[i] = itos(gceil(gel(vdir,i)));
  return v;
}

static GEN
computeGtwist(GEN nf, GEN vdir)
{
  long i, j, k, l, lG, v, r1;
  GEN G = gmael(nf,5,2);

  vdir = chk_vdir(nf, vdir);
  if (!vdir) return G;
  l = lg(vdir); lG = lg(G);
  G = shallowcopy(G);
  r1 = nf_get_r1(nf);
  for (i=1; i<l; i++)
  {
    v = vdir[i]; if (!v) continue;
    if (i <= r1) {
      for (j=1; j<lG; j++) gcoeff(G,i,j) = gmul2n(gcoeff(G,i,j), v);
    } else {
      k = (i<<1) - r1;
      for (j=1; j<lG; j++)
      {
	gcoeff(G,k-1,j) = gmul2n(gcoeff(G,k-1,j), v);
	gcoeff(G,k  ,j) = gmul2n(gcoeff(G,k  ,j), v);
      }
    }
  }
  return G;
}

/* assume I in NxN matrix form (not necessarily HNF) */
GEN
ideallllred_elt(GEN nf, GEN I, GEN vdir)
{
  GEN u, G0;

  if (vdir && typ(vdir) == t_MAT)
    G0 = vdir; /* assumed ZM */
  else
  {
    GEN G = computeGtwist(nf, vdir);
    long e, r = lg(G)-1;
    for (e = 4; ; e <<= 1)
    {
      G0 = ground(G);
      if (rank(G0) == r) break; /* maximal rank ? */
      G = gmul2n(G, e);
    }
  }
  u = lllint(ZM_mul(G0, I));
  return ZM_ZC_mul(I, gel(u,1)); /* small elt in I */
}

GEN
idealred_elt(GEN nf, GEN I) { return ideallllred_elt(nf, I, NULL); }

GEN
ideallllred(GEN nf, GEN I, GEN vdir, long prec)
{
  pari_sp av = avma;
  long N, i;
  GEN J, aI, y, x, T, b, c1, c, pol;

  nf = checknf(nf);
  pol = gel(nf,1); N = degpol(pol);
  T = x = c = c1 = NULL;
  switch (idealtyp(&I,&aI))
  {
    case id_PRINCIPAL:
      if (gcmp0(I)) I = cgetg(1,t_MAT); else { c1 = I; I = matid(N); }
      if (!aI) return I;
      goto END;
    case id_PRIME:
      if (prime_is_inert(I)) {
        c1 = gel(I,1); I = matid(N);
        if (!aI) return I;
        goto END;
      }
      I = idealhnf_two(nf,I);
      break;
    case id_MAT:
      I = Q_primitive_part(I, &c1);
  }
  y = ideallllred_elt(nf, I, vdir);
  if (ZV_isscalar(y))
  { /* already reduced */
    if (!aI) return gerepilecopy(av, I);
    goto END;
  }

  x = coltoliftalg(nf, y); /* algebraic integer */
  b = Q_remove_denom(QXQ_inv(x,pol), &T);
  b = poltobasis(nf,b);
  if (T)
  {
    GEN T2; b = Q_primitive_part(b, &T2);
    if (T2) { T = diviiexact(T, T2); if (is_pm1(T)) T = NULL; }
  }
  /* b = T x^(-1), T rat. integer, minimal such that b alg. integer */
  if (!T) /* x is a unit, I already reduced */
  {
    if (!aI) return gerepilecopy(av, I);
    goto END;
  }

  b = zk_multable(nf,b);
  J = cgetg(N+1,t_MAT); /* = I T/ x integral */
  for (i=1; i<=N; i++) gel(J,i) = ZM_ZC_mul(b, gel(I,i));
  J = Q_primitive_part(J, &c);
 /* c = content (I T / x) = T / den(I/x) --> d = den(I/x) = T / c
  * J = (d I / x); I[1,1] = I \cap Z --> d I[1,1] belongs to J and Z */
  I = ZM_hnfmodid(J, mulii(gcoeff(I,1,1), c? diviiexact(T,c): T));
  if (!aI) return gerepileupto(av, I);

  c = mul_content(c,c1);
  y = c? gmul(y, gdiv(c,T)): gdiv(y, T);
  aI = ext_mul(nf, aI,y);
  return gerepilecopy(av, mkvec2(I, aI));

END:
  if (c1) aI = ext_mul(nf, aI,c1);
  return gerepilecopy(av, mkvec2(I, aI));
}

GEN
idealmin(GEN nf, GEN x, GEN vdir)
{
  pari_sp av = avma;
  GEN y;
  nf = checknf(nf);
  switch( idealtyp(&x,&y) )
  {
    case id_PRINCIPAL: return gcopy(x);
    case id_PRIME: x = idealhnf_two(nf,x); break;
    case id_MAT: if (lg(x) == 1) return gen_0;
  }
  vdir = chk_vdir(nf,vdir);
  y = lll( RgM_mul(computeGtwist(nf,vdir), x) );
  y = RgM_RgC_mul(x, gel(y,1));
  return gerepileupto(av, y);
}

/*******************************************************************/
/*                                                                 */
/*                   APPROXIMATION THEOREM                         */
/*                                                                 */
/*******************************************************************/

/* write x = x1 x2, x2 maximal s.t. (x2,f) = 1, return x2 */
GEN
coprime_part(GEN x, GEN f)
{
  for (;;)
  {
    f = gcdii(x, f); if (is_pm1(f)) break;
    x = diviiexact(x, f);
  }
  return x;
}

/* x t_INT, f ideal. Write x = x1 x2, sqf(x1) | f, (x2,f) = 1. Return x2 */
static GEN
nf_coprime_part(GEN nf, GEN x, GEN listpr)
{
  long v, j, lp = lg(listpr), N = degpol(nf[1]);
  GEN x1, x2, ex, p, pr;

#if 0 /*1) via many gcds. Expensive ! */
  GEN f = idealprodprime(nf, listpr);
  f = ZM_hnfmodid(f, x); /* first gcd is less expensive since x in Z */
  x = scalarmat(x, N);
  for (;;)
  {
    if (gcmp1(gcoeff(f,1,1))) break;
    x = idealdivexact(nf, x, f);
    f = ZM_hnfmodid(shallowconcat(f,x), gcoeff(x,1,1)); /* gcd(f,x) */
  }
  x2 = x;
#else /*2) from prime decomposition */
  x1 = NULL;
  for (j=1; j<lp; j++)
  {
    pr = gel(listpr,j); p = gel(pr,1);
    v = Z_pval(x, p); if (!v) continue;

    ex = mului(v, gel(pr,3)); /* = v_pr(x) > 0 */
    x1 = x1? idealmulpowprime(nf, x1, pr, ex)
	   : idealpow(nf, pr, ex);
  }
  x = scalarmat(x, N);
  x2 = x1? idealdivexact(nf, x, x1): x;
#endif
  return x2;
}

/* L0 in K^*, assume (L0,f) = 1. Return L integral, L0 = L mod f  */
GEN
make_integral(GEN nf, GEN L0, GEN f, GEN listpr)
{
  GEN fZ, t, L, D2, d1, d2, d;

  L = Q_remove_denom(L0, &d);
  if (!d) return L0;

  /* L0 = L / d, L integral */
  fZ = gcoeff(f,1,1);
  if (typ(L) == t_INT) return Fp_mul(L, Fp_inv(d, fZ), fZ);
  /* Kill denom part coprime to fZ */
  d2 = coprime_part(d, fZ);
  t = Fp_inv(d2, fZ); if (!is_pm1(t)) L = ZC_Z_mul(L,t);
  if (equalii(d, d2)) return L;

  d1 = diviiexact(d, d2);
  /* L0 = (L / d1) mod f. d1 not coprime to f
   * write (d1) = D1 D2, D2 minimal, (D2,f) = 1. */
  D2 = nf_coprime_part(nf, d1, listpr);
  t = idealaddtoone_i(nf, D2, f); /* in D2, 1 mod f */
  L = element_muli(nf,t,L);

  /* if (L0, f) = 1, then L in D1 ==> in D1 D2 = (d1) */
  return Q_div_to_int(L, d1); /* exact division */
}

/* assume L is a list of prime ideals. Return the product */
GEN
idealprodprime(GEN nf, GEN L)
{
  long l = lg(L), i;
  GEN z;

  if (l == 1) return matid(degpol(nf[1]));
  z = idealhnf_two(nf, gel(L,1));
  for (i=2; i<l; i++) z = idealmulprime(nf,z, gel(L,i));
  return z;
}

/* assume L is a list of prime ideals. Return prod L[i]^e[i] */
GEN
factorbackprime(GEN nf, GEN L, GEN e)
{
  long l = lg(L), i;
  GEN z;

  if (l == 1) return matid(degpol(nf[1]));
  z = idealpow(nf, gel(L,1), gel(e,1));
  for (i=2; i<l; i++)
    if (signe(e[i])) z = idealmulpowprime(nf,z, gel(L,i),gel(e,i));
  return z;
}

/* F in Z squarefree, multiple of p. Return F-uniformizer for pr/p */
GEN
unif_mod_fZ(GEN pr, GEN F)
{
  GEN p = gel(pr,1), t = gel(pr,2);
  if (!equalii(F, p))
  {
    GEN u, v, q, e = gel(pr,3), a = diviiexact(F,p);
    q = is_pm1(e)? sqri(p): p;
    if (!gcmp1(bezout(q, a, &u,&v))) pari_err(bugparier,"unif_mod_fZ");
    u = mulii(u,q);
    v = mulii(v,a);
    t = ZC_Z_mul(t, v);
    gel(t,1) = addii(gel(t,1), u); /* return u + vt */
  }
  return t;
}
/* L = list of prime ideals, return lcm_i (L[i] \cap \ZM) */
GEN
init_unif_mod_fZ(GEN L)
{
  long i, r = lg(L);
  GEN pr, p, F = gen_1;
  for (i = 1; i < r; i++)
  {
    pr = gel(L,i); p = gel(pr,1);
    if (!dvdii(F, p)) F = mulii(F,p);
  }
  return F;
}

void
check_listpr(GEN x)
{
  long l = lg(x), i;
  for (i=1; i<l; i++) checkprid(gel(x,i));
}

/* Given a prime ideal factorization with possibly zero or negative
 * exponents, gives b such that v_p(b) = v_p(x) for all prime ideals pr | x
 * and v_pr(b)> = 0 for all other pr.
 * For optimal performance, all [anti-]uniformizers should be precomputed,
 * but no support for this yet.
 *
 * If nored, do not reduce result.
 * No garbage collecting */
static GEN
idealapprfact_i(GEN nf, GEN x, int nored)
{
  GEN z, d, L, e, e2, F;
  long i, r;
  int flagden;

  nf = checknf(nf);
  L = gel(x,1);
  e = gel(x,2);
  F = init_unif_mod_fZ(L);
  flagden = 0;
  z = NULL; r = lg(e);
  for (i = 1; i < r; i++)
  {
    long s = signe(e[i]);
    GEN pi, q;
    if (!s) continue;
    if (s < 0) flagden = 1;
    pi = unif_mod_fZ(gel(L,i), F);
    q = element_pow(nf, pi, gel(e,i));
    z = z? element_mul(nf, z, q): q;
  }
  if (!z) return scalarcol_shallow(gen_1, degpol(nf[1]));
  if (nored)
  {
    if (flagden) pari_err(impl,"nored + denominator in idealapprfact");
    return z;
  }
  e2 = cgetg(r, t_VEC);
  for (i=1; i<r; i++) gel(e2,i) = addis(gel(e,i), 1);
  x = factorbackprime(nf, L,e2);
  if (flagden) /* denominator */
  {
    z = Q_remove_denom(z, &d);
    d = diviiexact(d, coprime_part(d, F));
    x = RgM_Rg_mul(x, d);
  }
  else
    d = NULL;
  z = reducemodlll(z, x);
  return d? RgC_Rg_div(z,d): z;
}

GEN
idealapprfact(GEN nf, GEN x) {
  pari_sp av = avma;
  if (typ(x) != t_MAT || lg(x) != 3)
    pari_err(talker,"not a factorization in idealapprfact");
  check_listpr(gel(x,1));
  return gerepileupto(av, idealapprfact_i(nf, x, 0));
}

GEN
idealappr(GEN nf, GEN x) {
  pari_sp av = avma;
  return gerepileupto(av, idealapprfact_i(nf, idealfactor(nf, x), 0));
}

GEN
idealappr0(GEN nf, GEN x, long fl) {
  return fl? idealapprfact(nf, x): idealappr(nf, x);
}

/* merge a^e b^f. Assume a and b sorted. Keep 0 exponents */
static void
merge_fact(GEN *pa, GEN *pe, GEN b, GEN f)
{
  GEN A, E, a = *pa, e = *pe;
  long k, i, la = lg(a), lb = lg(b), l = la+lb-1;

  A = cgetg(l, t_COL);
  E = cgetg(l, t_COL);
  k = 1;
  for (i=1; i<la; i++)
  {
    A[i] = a[i];
    E[i] = e[i];
    if (k < lb && gequal(gel(A,i), gel(b,k)))
    {
      gel(E,i) = addii(gel(E,i), gel(f,k));
      k++;
    }
  }
  for (; k < lb; i++,k++)
  {
    A[i] = b[k];
    E[i] = f[k];
  }
  setlg(A, i); *pa = A;
  setlg(E, i); *pe = E;
}

/* Given a prime ideal factorization x with possibly zero or negative exponents,
 * and a vector w of elements of nf, gives a b such that
 * v_p(b-w_p)>=v_p(x) for all prime ideals p in the ideal factorization
 * and v_p(b)>=0 for all other p, using the (standard) proof given in GTM 138.
 * Certainly not the most efficient, but sure. */
GEN
idealchinese(GEN nf, GEN x, GEN w)
{
  pari_sp av = avma;
  long ty = typ(w), i, N, r;
  GEN y, L, e, F, s, den;

  nf = checknf(nf); N = degpol(nf[1]);
  if (typ(x) != t_MAT || lg(x) != 3)
    pari_err(talker,"not a prime ideal factorization in idealchinese");
  L = gel(x,1); r = lg(L);
  e = gel(x,2);
  if (!is_vec_t(ty) || lg(w) != r)
    pari_err(talker,"not a suitable vector of elements in idealchinese");
  if (r == 1) return scalarcol_shallow(gen_1,N);

  w = Q_remove_denom(w, &den);
  if (den)
  {
    GEN p = gen_indexsort(x, (void*)&cmp_prime_ideal, cmp_nodata);
    GEN fa = idealfactor(nf, den); /* sorted */
    L = vecpermute(L, p);
    e = vecpermute(e, p);
    w = vecpermute(w, p); settyp(w, t_VEC); /* make sure typ = t_VEC */
    merge_fact(&L, &e, gel(fa,1), gel(fa,2));
    i = lg(L);
    w = shallowconcat(w, zerovec(i - r));
    r = i;
  }
  else
    e = shallowcopy(e); /* do not destroy x[2] */
  for (i=1; i<r; i++)
    if (signe(e[i]) < 0) gel(e,i) = gen_0;

  F = factorbackprime(nf, L, e);
  s = NULL;
  for (i=1; i<r; i++)
  {
    GEN u, t;
    if (gcmp0(gel(w,i))) continue;
    u = hnfmerge_get_1(idealdivpowprime(nf,F, gel(L,i), gel(e,i)),
		       idealpow(nf, gel(L,i), gel(e,i)));
    t = element_mul(nf, u, gel(w,i));
    s = s? gadd(s,t): t;
  }
  if (!s) { avma = av; return zerocol(N); }
  y = reducemodlll(s, F);
  return gerepileupto(av, den? RgM_Rg_div(y,den): y);
}

static GEN
mat_ideal_two_elt2(GEN nf, GEN x, GEN a)
{
  GEN L, e, fact = idealfactor(nf,a);
  long i, r;
  L = gel(fact,1);
  e = gel(fact,2); r = lg(e);
  for (i=1; i<r; i++) gel(e,i) = stoi( idealval(nf,x,gel(L,i)) );
  return idealapprfact_i(nf,fact,1);
}

static void
not_in_ideal() {
  pari_err(talker,"element not in ideal in ideal_two_elt2");
}

/* Given an integral ideal x and a in x, gives a b such that
 * x = aZ_K + bZ_K using the approximation theorem */
GEN
ideal_two_elt2(GEN nf, GEN x, GEN a)
{
  pari_sp av = avma;
  GEN cx, b;

  nf = checknf(nf);
  a = nf_to_scalar_or_basis(nf, a);
  x = idealhnf_shallow(nf,x);
  if (lg(x) == 1)
  {
    if (typ(a) != t_INT || signe(a)) not_in_ideal();
    avma = av; return zerocol(degpol(nf[1]));
  }
  x = Q_primitive_part(x, &cx);
  if (cx) a = gdiv(a, cx);
  if (typ(a) != t_COL)
  { /* rational number */
    if (typ(a) != t_INT || !dvdii(a, gcoeff(x,1,1))) not_in_ideal();
  }
  else
    if (!hnf_invimage(x, a)) not_in_ideal();

  b = mat_ideal_two_elt2(nf, x, a);
  b = centermod(b, gcoeff(x,1,1));
  b = cx? RgC_Rg_mul(b,cx): gcopy(b);
  return gerepileupto(av, b);
}

/* Given 2 integral ideals x and y in nf, returns a beta in nf such that
 * beta * x is an integral ideal coprime to y */
GEN
idealcoprime_fact(GEN nf, GEN x, GEN fy)
{
  GEN L = gel(fy,1), e;
  long i, r = lg(L);

  e = cgetg(r, t_COL);
  for (i=1; i<r; i++) gel(e,i) = stoi( -idealval(nf,x,gel(L,i)) );
  return idealapprfact_i(nf, mkmat2(L,e), 0);
}
GEN
idealcoprime(GEN nf, GEN x, GEN y)
{
  pari_sp av = avma;
  return gerepileupto(av, idealcoprime_fact(nf, x, idealfactor(nf,y)));
}

/*******************************************************************/
/*                                                                 */
/*                  LINEAR ALGEBRA OVER Z_K  (HNF,SNF)             */
/*                                                                 */
/*******************************************************************/
/* A torsion-free module M over Z_K is given by [A,I].
 * I=[a_1,...,a_k] is a row vector of k fractional ideals given in HNF.
 * A is an n x k matrix (same k) such that if A_j is the j-th column of A then
 * M=a_1 A_1+...+a_k A_k. We say that [A,I] is a pseudo-basis if k=n */
void
check_ZKmodule(GEN x, const char *s)
{
  if (typ(x) != t_VEC || lg(x) < 3) pari_err(talker,"not a module in %s", s);
  if (typ(x[1]) != t_MAT) pari_err(talker,"not a matrix in %s", s);
  if (typ(x[2]) != t_VEC || lg(x[2]) != lg(x[1]))
    pari_err(talker,"not a correct ideal list in %s", s);
}

static GEN
element_mulvecrow(GEN nf, GEN x, GEN m, long i, long lim)
{
  long l, j;
  GEN y, dx;
  x = nf_to_scalar_or_basis(nf, x);
  if (typ(x) == t_COL)
    x = zk_multable(nf, Q_remove_denom(x, &dx));
  else
    dx = NULL;

  l = min(lg(m), lim+1); y = cgetg(l, t_VEC);
  for (j=1; j<l; j++)
  {
    GEN t = gmul(x, gcoeff(m,i,j));
    if (dx) t = gdiv(t, dx);
    gel(y,j) = t;
  }
  return y;
}

/* Given an element x and an ideal in matrix form (not necessarily HNF),
 * gives an r such that x-r is in ideal and r is small. No checks */
GEN
element_reduce(GEN nf, GEN x, GEN ideal)
{
  pari_sp av = avma;
  x = algtobasis_i(checknf(nf), x);
  return gerepileupto(av, reducemodinvertible(x, ideal));
}
/* Given an element x and an ideal in HNF, gives an a in ideal such that
 * x-a is small. No checks */
static GEN
element_close(GEN nf, GEN x, GEN ideal)
{
  pari_sp av = avma;
  GEN y = gcoeff(ideal,1,1);
  x = nf_to_scalar_or_basis(nf, x);
  if (typ(y) == t_INT && is_pm1(y)) return ground(x);
  if (typ(x) == t_COL)
    x = closemodinvertible(x, ideal);
  else
    x = gmul(y, gdivround(x,y));
  return gerepileupto(av, x);
}

/* u A + v B */
static GEN
colcomb(GEN nf, GEN u, GEN v, GEN A, GEN B)
{
  GEN z;
  if (gcmp0(u))
    z = element_mulvec(nf,v,B);
  else
  {
    z = u==gen_1? A: element_mulvec(nf,u,A);
    if (!gcmp0(v)) z = gadd(z, element_mulvec(nf,v,B));
  }
  return z;
}

/* u Z[s,] + v Z[t,] */
static GEN
rowcomb(GEN nf, GEN u, GEN v, long s, long t, GEN Z, long lim)
{
  GEN z;
  if (gcmp0(u))
    z = element_mulvecrow(nf,v,Z,t, lim);
  else
  {
    z = element_mulvecrow(nf,u,Z,s, lim);
    if (!gcmp0(v)) z = gadd(z, element_mulvecrow(nf,v,Z,t, lim));
  }
  return z;
}

static GEN
zero_nfbezout(GEN nf,GEN b, GEN A,GEN B,GEN *u,GEN *v,GEN *w,GEN *di)
{
  GEN d = idealmul(nf,b,B);
  pari_sp av;

  *di = idealinv(nf,d);
  av = avma;
  *w = gerepileupto(av, idealmul(nf, idealmul(nf,A,B), *di));
  *v = element_inv(nf,b);
  *u = gen_0; return d;
}

/* Given elements a,b and ideals A, B, outputs d = a.A+b.B and gives
 * di=d^-1, w=A.B.di, u, v such that au+bv=1 and u in A.di, v in B.di.
 * Assume A, B non-zero, but a or b can be zero (not both) */
static GEN
nfbezout(GEN nf,GEN a,GEN b, GEN A,GEN B, GEN *pu,GEN *pv,GEN *pw,GEN *pdi)
{
  GEN w, u,v, d, di, aA, bB;

  if (gcmp0(a)) return zero_nfbezout(nf,b,A,B,pu,pv,pw,pdi);
  if (gcmp0(b)) return zero_nfbezout(nf,a,B,A,pv,pu,pw,pdi);

  if (a != gen_1) /* frequently called with a = gen_1 */
  {
    a = nf_to_scalar_or_basis(nf,a);
    if (typ(a) == t_INT && is_pm1(a) && signe(a) > 0) a = gen_1;
  }

  aA = (a == gen_1)? A: idealmul(nf,a,A);
  bB = idealmul(nf,b,B);
  d = idealadd(nf,aA,bB);
  di = hnfideal_inv(nf,d);
  if (gequal(aA, d))
  { /* aA | bB  (frequent) */
    w = B;
    v = gen_0;
    if (a == gen_1)
      u = col_ei(lg(B)-1, 1);
    else
    {
      u = element_inv(nf, a);
      w = idealmul(nf, u, w); /* AB/d */
    }
  }
  else if (gequal(bB, d))
  { /* bB | aA  (slightly less frequent) */
    w = A;
    u = gen_0;
    v = element_inv(nf, b);
    w = idealmul(nf, v, w); /* AB/d */
  }
  else
  { /* general case. Slow */
    GEN uv;
    w = idealmul(nf,aA,di); /* integral */
    uv = idealaddtoone(nf, w, idealmul(nf,bB,di));
    w = idealmul(nf,w,B);
    u = gel(uv,1);
    v = element_div(nf,gel(uv,2),b);
    if (a != gen_1)
    {
      GEN inva = element_inv(nf, a);
      u =  element_mul(nf,u,inva);
      w = idealmul(nf, inva, w); /* AB/d */
    }
  }
  *pu = u;
  *pv = v;
  *pw = w;
  *pdi = di; return d;
}

/* Given a torsion-free module x outputs a pseudo-basis for x in HNF */
GEN
nfhnf(GEN nf, GEN x)
{
  long i, j, def, k, m;
  pari_sp av0 = avma, av, lim;
  GEN y, A, I, J;

  nf = checknf(nf);
  check_ZKmodule(x, "nfhnf");
  A = gel(x,1);
  I = gel(x,2); k = lg(A)-1;
  if (!k) pari_err(talker,"not a matrix of maximal rank in nfhnf");
  m = lg(A[1])-1;
  if (k < m) pari_err(talker,"not a matrix of maximal rank in nfhnf");

  av = avma; lim = stack_lim(av, 2);
  A = matalgtobasis(nf,A);
  I = shallowcopy(I);
  J = zerovec(k); def = k+1;
  for (i=m; i>=1; i--)
  {
    GEN d, di = NULL;

    def--; j=def; while (j>=1 && gcmp0(gcoeff(A,i,j))) j--;
    if (!j) pari_err(talker,"not a matrix of maximal rank in nfhnf");
    if (j==def) j--; else {
      swap(gel(A,j), gel(A,def)); swap(gel(I,j), gel(I,def));
    }

    y = gcoeff(A,i,def);
    gel(A,def) = element_mulvec(nf, element_inv(nf,y), gel(A,def));
    gel(I,def) = idealmul(nf, y, gel(I,def));
    for (  ; j; j--)
    {
      GEN b, u,v,w, S, T, S0, T0 = gel(A,j);
      b = gel(T0,i); if (gcmp0(b)) continue;

      S0 = gel(A,def);
      d = nfbezout(nf, gen_1,b, gel(I,def),gel(I,j), &u,&v,&w,&di);
      S = colcomb(nf, u,v, S0,T0);
      T = colcomb(nf, gen_1,gneg(b), T0,S0);
      gel(A,def) = S; gel(A,j) = T;
      gel(I,def) = d; gel(I,j) = w;
    }
    d = gel(I,def);
    if (!di) di = idealinv(nf,d);
    gel(J,def) = di;
    for (j=def+1; j<=k; j++)
    {
      GEN c = element_close(nf,gcoeff(A,i,j), idealmul(nf,d,gel(J,j)));
      gel(A,j) = colcomb(nf, gen_1,gneg(c), gel(A,j),gel(A,def));
    }
    if (low_stack(lim, stack_lim(av1,2)))
    {
      if(DEBUGMEM>1) pari_warn(warnmem,"nfhnf, i = %ld", i);
      gerepileall(av,3, &A,&I,&J);
    }
  }
  A += k-m; A[0] = evaltyp(t_MAT)|evallg(m+1);
  I += k-m; I[0] = evaltyp(t_VEC)|evallg(m+1);
  return gerepilecopy(av0, mkvec2(A, I));
}

/* A torsion module M over Z_K will be given by a row vector [A,I,J] with
 * three components. I=[b_1,...,b_n] is a row vector of k fractional ideals
 * given in HNF, J=[a_1,...,a_n] is a row vector of n fractional ideals in
 * HNF. A is an nxn matrix (same n) such that if A_j is the j-th column of A
 * and e_n is the canonical basis of K^n, then
 * M=(b_1e_1+...+b_ne_n)/(a_1A_1+...a_nA_n) */

/* x=[A,I,J] a torsion module as above. Output the
 * smith normal form as K=[c_1,...,c_n] such that x = Z_K/c_1+...+Z_K/c_n */
GEN
nfsmith(GEN nf, GEN x)
{
  long i, j, k, l, c, n, m, N;
  pari_sp av, lim;
  GEN z,u,v,w,d,dinv,A,I,J;

  nf = checknf(nf); N = degpol(nf[1]);
  if (typ(x)!=t_VEC || lg(x)!=4) pari_err(talker,"not a module in nfsmith");
  A = gel(x,1);
  I = gel(x,2);
  J = gel(x,3);
  if (typ(A)!=t_MAT) pari_err(talker,"not a matrix in nfsmith");
  n = lg(A)-1;
  if (typ(I)!=t_VEC || lg(I)!=n+1 || typ(J)!=t_VEC || lg(J)!=n+1)
    pari_err(talker,"not a correct ideal list in nfsmith");
  if (!n) pari_err(talker,"not a matrix of maximal rank in nfsmith");
  m = lg(A[1])-1;
  if (n < m) pari_err(talker,"not a matrix of maximal rank in nfsmith");
  if (n > m) pari_err(impl,"nfsmith for non square matrices");

  av = avma; lim = stack_lim(av,1);
  A = shallowcopy(A);
  I = shallowcopy(I);
  J = shallowcopy(J);
  for (i=n; i>=2; i--)
  {
    do
    {
      GEN a, b;
      c = 0;
      for (j=i-1; j>=1; j--)
      {
	GEN S, T, S0, T0 = gel(A,j);
	b = gel(T0,i); if (gcmp0(b)) continue;

	S0 = gel(A,i); a = gel(S0,i);
	d = nfbezout(nf, a,b, gel(J,i),gel(J,j), &u,&v,&w,&dinv);
	S = colcomb(nf, u,v, S0,T0);
	T = colcomb(nf, a,gneg(b), T0,S0);
	gel(A,i) = S; gel(A,j) = T;
	gel(J,i) = d; gel(J,j) = w;
      }
      for (j=i-1; j>=1; j--)
      {
	GEN ri, rj;
	b = gcoeff(A,j,i); if (gcmp0(b)) continue;

	a = gcoeff(A,i,i);
	d = nfbezout(nf, a,b, gel(I,i),gel(I,j), &u,&v,&w,&dinv);
	ri = rowcomb(nf, u,v,       i,j, A, i);
	rj = rowcomb(nf, a,gneg(b), j,i, A, i);
	for (k=1; k<=i; k++) {
          gcoeff(A,j,k) = gel(rj,k);
          gcoeff(A,i,k) = gel(ri,k);
        }
	gel(I,i) = d; gel(I,j) = w; c = 1;
      }
      if (c) continue;

      b = gcoeff(A,i,i); if (gcmp0(b)) break;
      b = idealmul(nf, b, idealmul(nf,gel(J,i),gel(I,i)));
      for (k=1; k<i; k++)
	for (l=1; l<i; l++)
	{
	  GEN p2, p3, p1 = gcoeff(A,k,l);
	  if (gcmp0(p1)) continue;

	  p3 = idealmul(nf, p1, idealmul(nf,gel(J,l),gel(I,k)));
	  if (hnfdivide(b, p3)) continue;

	  b = idealdiv(nf,gel(I,k),gel(I,i));
	  p2 = idealdiv(nf,gel(J,i), idealmul(nf,p1,gel(J,l)));
	  p3 = RgM_solve(p2, b);
	  l=1; while (l<=N && gcmp1(denom(gel(p3,l)))) l++;
	  if (l>N) pari_err(talker,"bug2 in nfsmith");
	  p1 = element_mulvecrow(nf,gel(b,l),A,k,i);
	  for (l=1; l<=i; l++)
	    gcoeff(A,i,l) = gadd(gcoeff(A,i,l),gel(p1,l));

	  k = l = i; c = 1;
	}
      if (low_stack(lim, stack_lim(av,1)))
      {
	if(DEBUGMEM>1) pari_warn(warnmem,"nfsmith");
	gerepileall(av,3, &A,&I,&J);
      }
    }
    while (c);
  }
  gel(J,1) = idealmul(nf, gcoeff(A,1,1), gel(J,1));
  z = cgetg(n+1,t_VEC);
  for (i=1; i<=n; i++) gel(z,i) = idealmul(nf,gel(I,i),gel(J,i));
  return gerepileupto(av, z);
}

GEN
element_mulmodpr(GEN nf, GEN x, GEN y, GEN modpr)
{
  pari_sp av = avma;
  GEN z, p, pr, T;

  nf = checknf(nf);
  modpr = nf_to_Fq_init(nf,&pr,&T,&p);
  z = Fq_to_nf(Fq_mul(nf_to_Fq(nf,x,modpr), nf_to_Fq(nf,y,modpr),T,p), modpr);
  return gerepileupto(av, algtobasis(nf, Fq_to_nf(z,modpr)));
}

GEN
element_divmodpr(GEN nf, GEN x, GEN y, GEN modpr)
{
  pari_sp av = avma;
  nf = checknf(nf);
  return gerepileupto(av, nfreducemodpr(nf, element_div(nf,x,y), modpr));
}

GEN
element_powmodpr(GEN nf,GEN x,GEN k,GEN pr)
{
  pari_sp av=avma;
  GEN z,T,p,modpr;

  nf = checknf(nf);
  modpr = nf_to_Fq_init(nf,&pr,&T,&p);
  z = nf_to_Fq(nf,x,modpr);
  z = Fq_pow(z,k,T,p);
  return gerepileupto(av, algtobasis(nf, Fq_to_nf(z,modpr)));
}

GEN
nfkermodpr(GEN nf, GEN x, GEN pr)
{
  pari_sp av = avma;
  GEN T,p,modpr;

  nf = checknf(nf);
  modpr = nf_to_Fq_init(nf, &pr,&T,&p);
  if (typ(x)!=t_MAT) pari_err(typeer,"nfkermodpr");
  x = nfM_to_FqM(x, nf, modpr);
  return gerepilecopy(av, FqM_to_nfM(FqM_ker(x,T,p), modpr));
}

GEN
nfsolvemodpr(GEN nf, GEN a, GEN b, GEN pr)
{
  pari_sp av = avma;
  GEN T, p, modpr;
  long tb = typ(b);

  nf = checknf(nf);
  modpr = nf_to_Fq_init(nf, &pr,&T,&p);
  if (typ(a)!=t_MAT) pari_err(typeer,"nfsolvemodpr");
  a = nfM_to_FqM(a, nf, modpr);
  switch(tb)
  {
    case t_MAT:
      b = nfM_to_FqM(b, nf, modpr);
      a = FqM_to_nfM(FqM_gauss(a,b,T,p), modpr);
      break;
    case t_COL:
      b = nfV_to_FqV(b, nf, modpr);
      a = FqV_to_nfV(FqM_gauss(a,b,T,p), modpr);
      break;
    default: pari_err(typeer,"nfsolvemodpr");
  }
  return gerepilecopy(av, a);
}

/* Given a pseudo-basis x, outputs a multiple of its ideal determinant */
GEN
nfdetint(GEN nf, GEN x)
{
  GEN pass,c,v,det1,piv,pivprec,vi,p1,A,I,id,idprod;
  long i, j, k, rg, n, m, m1, cm=0, N;
  pari_sp av = avma, av1, lim;

  nf = checknf(nf); N = degpol(nf[1]);
  check_ZKmodule(x, "nfdetint");
  A = gel(x,1);
  I = gel(x,2);
  n = lg(A)-1; if (!n) return gen_1;

  m1 = lg(A[1]); m = m1-1;
  id = matid(N);
  c = new_chunk(m1); for (k=1; k<=m; k++) c[k] = 0;
  piv = pivprec = gen_1;

  av1 = avma; lim = stack_lim(av1,1);
  det1 = idprod = gen_0; /* dummy for gerepilemany */
  pass = cgetg(m1,t_MAT);
  v = cgetg(m1,t_COL);
  for (j=1; j<=m; j++)
  {
    gel(pass,j) = zerocol(m);
    gel(v,j) = gen_0; /* dummy */
  }
  for (rg=0,k=1; k<=n; k++)
  {
    long t = 0;
    for (i=1; i<=m; i++)
      if (!c[i])
      {
	vi=element_mul(nf,piv,gcoeff(A,i,k));
	for (j=1; j<=m; j++)
	  if (c[j]) vi=gadd(vi,element_mul(nf,gcoeff(pass,i,j),gcoeff(A,j,k)));
	gel(v,i) = vi; if (!t && !gcmp0(vi)) t=i;
      }
    if (t)
    {
      pivprec = piv;
      if (rg == m-1)
      {
	if (!cm)
	{
	  cm=1; idprod = id;
	  for (i=1; i<=m; i++)
	    if (i!=t)
	      idprod = (idprod==id)? gel(I,c[i])
				   : idealmul(nf,idprod,gel(I,c[i]));
	}
	p1 = idealmul(nf,gel(v,t),gel(I,k)); c[t]=0;
	det1 = (typ(det1)==t_INT)? p1: idealadd(nf,p1,det1);
      }
      else
      {
	rg++; piv=gel(v,t); c[t]=k;
	for (i=1; i<=m; i++)
	  if (!c[i])
	  {
	    for (j=1; j<=m; j++)
	      if (c[j] && j!=t)
	      {
		p1 = gsub(element_mul(nf,piv,gcoeff(pass,i,j)),
			  element_mul(nf,gel(v,i),gcoeff(pass,t,j)));
		gcoeff(pass,i,j) = rg>1? element_div(nf,p1,pivprec)
				       : p1;
	      }
	    gcoeff(pass,i,t) = gneg(gel(v,i));
	  }
      }
    }
    if (low_stack(lim, stack_lim(av1,1)))
    {
      if(DEBUGMEM>1) pari_warn(warnmem,"nfdetint");
      gerepileall(av1,6, &det1,&piv,&pivprec,&pass,&v,&idprod);
    }
  }
  if (!cm) { avma = av; return cgetg(1,t_MAT); }
  return gerepileupto(av, idealmul(nf,idprod,det1));
}

/* clean in place (destroy x) */
static void
nfcleanmod(GEN nf, GEN x, long lim, GEN D)
{
  long i;
  GEN c;
  D = ZM_lll(Q_primitive_part(D, &c), 0.75, LLL_INPLACE);
  if (c) D = RgM_Rg_mul(D,c);
  for (i=1; i<=lim; i++) gel(x,i) = element_reduce(nf, gel(x,i), D);
}

GEN
nfhnfmod(GEN nf, GEN x, GEN detmat)
{
  long li, co, i, j, def, ldef, N;
  pari_sp av0=avma, av, lim;
  GEN d0, w, p1, d, u, v, A, I, J, di, unnf;

  nf = checknf(nf); N = degpol(nf[1]);
  check_ZKmodule(x, "nfhnfmod");
  A = gel(x,1);
  I = gel(x,2);
  co = lg(A); if (co==1) return cgetg(1,t_MAT);

  li = lg(A[1]);
  unnf = scalarcol_shallow(gen_1,N);
  detmat = Q_remove_denom(detmat, NULL);
  if (typ(detmat)!=t_MAT) pari_err(typeer,"nfhnfmod");
  RgM_check_ZM(detmat, "nfhnfmod");

  av = avma; lim = stack_lim(av,2);
  A = matalgtobasis(nf, A);
  I = shallowcopy(I);
  def = co; ldef = (li>co)? li-co+1: 1;
  for (i=li-1; i>=ldef; i--)
  {
    def--; j=def; while (j>=1 && gcmp0(gcoeff(A,i,j))) j--;
    if (!j) continue;
    if (j==def) j--;
    else {
      swap(gel(A,j), gel(A,def));
      swap(gel(I,j), gel(I,def));
    }
    for (  ; j; j--)
    {
      GEN a, b, S, T, S0, T0 = gel(A,j);
      b = gel(T0,i); if (gcmp0(b)) continue;

      S0 = gel(A,def); a = gel(S0,i);
      d = nfbezout(nf, a,b, gel(I,def),gel(I,j), &u,&v,&w,&di);
      S = colcomb(nf, u,v, S0,T0);
      T = colcomb(nf, a,gneg(b), T0,S0);
      if (u != gen_0 && v != gen_0) /* already reduced otherwise */
	nfcleanmod(nf, S, i, idealmul(nf,detmat,di));
      nfcleanmod(nf, T, i, idealdiv(nf,detmat,w));
      gel(A,def) = S; gel(A,j) = T;
      gel(I,def) = d; gel(I,j) = w;
    }
    if (low_stack(lim, stack_lim(av,2)))
    {
      if(DEBUGMEM>1) pari_warn(warnmem,"[1]: nfhnfmod, i = %ld", i);
      gerepileall(av,2, &A,&I);
    }
  }
  def--; d0 = detmat;
  A += def; A[0] = evaltyp(t_MAT)|evallg(li);
  I += def; I[0] = evaltyp(t_VEC)|evallg(li);
  for (i=li-1; i>=1; i--)
  {
    d = nfbezout(nf, gen_1,gcoeff(A,i,i), d0,gel(I,i), &u,&v,&w,&di);
    p1 = element_mulvec(nf,v,gel(A,i));
    if (i > 1)
    {
      d0 = idealmul(nf,d0,di);
      nfcleanmod(nf, p1, i, d0);
    }
    gel(A,i) = p1; gel(p1,i) = unnf;
    gel(I,i) = d;
  }
  J = cgetg(li,t_VEC); gel(J,1) = gen_0;
  for (j=2; j<li; j++) gel(J,j) = idealinv(nf,gel(I,j));
  for (i=li-2; i>=1; i--)
  {
    d = gel(I,i);
    for (j=i+1; j<li; j++)
    {
      GEN c = element_close(nf, gcoeff(A,i,j), idealmul(nf,d,gel(J,j)));
      gel(A,j) = colcomb(nf, gen_1,gneg(c), gel(A,j),gel(A,i));
    }
    if (low_stack(lim, stack_lim(av,2)))
    {
      if(DEBUGMEM>1) pari_warn(warnmem,"[2]: nfhnfmod, i = %ld", i);
      gerepileall(av,3, &A,&I,&J);
    }
  }
  return gerepilecopy(av0, mkvec2(A, I));
}
