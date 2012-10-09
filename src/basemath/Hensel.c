/* $Id$

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */
#include "pari.h"
#include "paripriv.h"

/***********************************************************************/
/**                                                                   **/
/**       QUADRATIC HENSEL LIFT (adapted from V. Shoup's NTL)         **/
/**                                                                   **/
/***********************************************************************/
/* Setup for divide/conquer quadratic Hensel lift
 *  a = set of k t_POL in Z[X] = factors over Fp (T=NULL) or Fp[Y]/(T)
 *  V = set of products of factors built as follows
 *  1) V[1..k] = initial a
 *  2) iterate:
 *      append to V the two smallest factors (minimal degree) in a, remove them
 *      from a and replace them by their product [net loss for a = 1 factor]
 *
 * W = bezout coeffs W[i]V[i] + W[i+1]V[i+1] = 1
 *
 * link[i] = -j if V[i] = a[j]
 *            j if V[i] = V[j] * V[j+1]
 * Arrays (link, V, W) pre-allocated for 2k - 2 elements */
static void
BuildTree(GEN link, GEN V, GEN W, GEN a, GEN T, GEN p)
{
  long k = lg(a)-1;
  long i, j, s, minp, mind;

  for (i=1; i<=k; i++) { gel(V,i) = gel(a,i); link[i] = -i; }

  for (j=1; j <= 2*k-5; j+=2,i++)
  {
    minp = j;
    mind = degpol(gel(V,j));
    for (s=j+1; s<i; s++)
      if (degpol(gel(V,s)) < mind) { minp = s; mind = degpol(gel(V,s)); }

    swap(gel(V,j), gel(V,minp));
    lswap(link[j], link[minp]);

    minp = j+1;
    mind = degpol(gel(V,j+1));
    for (s=j+2; s<i; s++)
      if (degpol(gel(V,s)) < mind) { minp = s; mind = degpol(gel(V,s)); }

    swap(gel(V,j+1), gel(V,minp));
    lswap(link[j+1], link[minp]);

    gel(V,i) = FqX_mul(gel(V,j), gel(V,j+1), T, p);
    link[i] = j;
  }

  for (j=1; j <= 2*k-3; j+=2)
  {
    GEN d, u, v;
    d = FqX_extgcd(gel(V,j), gel(V,j+1), T, p, &u, &v);
    if (degpol(d) > 0) pari_err_COPRIME("BuildTree", gel(V,j), gel(V,j+1));
    d = gel(d,2);
    if (!gequal1(d))
    {
      if (typ(d)==t_POL)
      {
        d = FpXQ_inv(d, T, p);
        u = FqX_Fq_mul(u, d, T, p);
        v = FqX_Fq_mul(v, d, T, p);
      }
      else
      {
        d = Fp_inv(d, p);
        u = FqX_Fp_mul(u, d, T,p);
        v = FqX_Fp_mul(v, d, T,p);
      }
    }
    gel(W,j) = u;
    gel(W,j+1) = v;
  }
}

/* au + bv = 1 (p0), ab = f (p0). Lift mod p1 = p0 pd (<= p0^2).
 * If noinv is set, don't lift the inverses u and v */
static void
ZpX_HenselLift(GEN V, GEN W, long j, GEN f, GEN pd, GEN p0, GEN p1, int noinv)
{
  pari_sp av = avma;
  long space = lg(f) * lgefint(p1);
  GEN a2, b2, g, z, s, t;
  GEN a = gel(V,j), b = gel(V,j+1);
  GEN u = gel(W,j), v = gel(W,j+1);

  (void)new_chunk(space); /* HACK */
  g = ZX_sub(f, ZX_mul(a,b));
  g = ZX_Z_divexact(g, p0);
  g = FpX_red(g, pd);
  z = FpX_mul(v,g, pd);
  t = FpX_divrem(z,a, pd, &s);
  t = ZX_add(ZX_mul(u,g), ZX_mul(t,b));
  t = FpX_red(t, pd);
  t = ZX_Z_mul(t,p0);
  s = ZX_Z_mul(s,p0);
  avma = av;
  a2 = ZX_add(a,s);
  b2 = ZX_add(b,t);

  /* already reduced mod p1 = pd p0 */
  gel(V,j)   = a2;
  gel(V,j+1) = b2;
  if (noinv) return;

  av = avma;
  (void)new_chunk(space); /* HACK */
  g = ZX_add(ZX_mul(u,a2), ZX_mul(v,b2));
  g = Z_ZX_sub(gen_1, g);
  g = ZX_Z_divexact(g, p0);
  g = FpX_red(g, pd);
  z = FpX_mul(v,g, pd);
  t = FpX_divrem(z,a, pd, &s);
  t = ZX_add(ZX_mul(u,g), ZX_mul(t,b));
  t = FpX_red(t, pd);
  t = ZX_Z_mul(t,p0);
  s = ZX_Z_mul(s,p0);
  avma = av;
  gel(W,j)   = ZX_add(u,t);
  gel(W,j+1) = ZX_add(v,s);
}

static void
ZpXQ_HenselLift(GEN V, GEN W, long j, GEN f, GEN Td, GEN T1, GEN pd, GEN p0, GEN p1, int noinv)
{
  pari_sp av = avma;
  long space = lg(f) * lgefint(p1) * lg(T1);
  GEN a2, b2, g, z, s, t;
  GEN a = gel(V,j), b = gel(V,j+1);
  GEN u = gel(W,j), v = gel(W,j+1);

  (void)new_chunk(space); /* HACK */
  g = RgX_sub(f, RgX_mul(a,b));
  g = FpXQX_red(g, T1, p1);
  g = RgX_Rg_divexact(g, p0);
  z = FpXQX_mul(v,g, Td,pd);
  t = FpXQX_divrem(z,a, Td,pd, &s);
  t = RgX_add(RgX_mul(u,g), RgX_mul(t,b));
  t = FpXQX_red(t, Td, pd);
  t = RgX_Rg_mul(t,p0);
  s = RgX_Rg_mul(s,p0);
  avma = av;

  a2 = RgX_add(a,s);
  b2 = RgX_add(b,t);
  /* already reduced mod p1 = pd p0 */
  gel(V,j)   = a2;
  gel(V,j+1) = b2;
  if (noinv) return;

  av = avma;
  (void)new_chunk(space); /* HACK */
  g = RgX_add(RgX_mul(u,a2), RgX_mul(v,b2));
  g = Rg_RgX_sub(gen_1, g);
  g = FpXQX_red(g, T1, p1);
  g = RgX_Rg_divexact(g, p0);
  z = FpXQX_mul(v,g, Td,pd);
  t = FpXQX_divrem(z,a, Td,pd, &s);
  t = RgX_add(RgX_mul(u,g), RgX_mul(t,b));
  t = FpXQX_red(t, Td, pd);
  t = RgX_Rg_mul(t,p0);
  s = RgX_Rg_mul(s,p0);
  avma = av;
  gel(W,j)   = RgX_add(u,t);
  gel(W,j+1) = RgX_add(v,s);
}

/* v list of factors, w list of inverses.  f = v[j] v[j+1]
 * Lift v[j] and v[j+1] mod p0 pd (possibly mod T), then all their divisors */
static void
ZpX_RecTreeLift(GEN link, GEN v, GEN w, GEN pd, GEN p0, GEN p1,
                GEN f, long j, int noinv)
{
  if (j < 0) return;
  ZpX_HenselLift(v, w, j, f, pd, p0,p1, noinv);
  ZpX_RecTreeLift(link, v, w, pd, p0,p1, gel(v,j)  , link[j  ], noinv);
  ZpX_RecTreeLift(link, v, w, pd, p0,p1, gel(v,j+1), link[j+1], noinv);
}
static void
ZpXQ_RecTreeLift(GEN link, GEN v, GEN w, GEN Td, GEN T1, GEN pd, GEN p0, GEN p1,
                 GEN f, long j, int noinv)
{
  if (j < 0) return;
  ZpXQ_HenselLift(v, w, j, f, Td,T1, pd, p0,p1, noinv);
  ZpXQ_RecTreeLift(link, v, w, Td,T1, pd, p0,p1, gel(v,j)  , link[j  ], noinv);
  ZpXQ_RecTreeLift(link, v, w, Td,T1, pd, p0,p1, gel(v,j+1), link[j+1], noinv);
}

/* Assume n > 0. We want to go to accuracy n, starting from accuracy 1, using
 * a quadratically convergent algorithm. Goal: 9 -> 1,2,3,5,9 instead of
 * 1,2,4,8,9 (sequence of accuracies).
 *
 * Let a0 = 1, a1 = 2, a2, ... ak = n, the sequence of accuracies. To obtain
 * it, work backwards:
 *   a(k) = n, a(i-1) = (a(i) + 1) \ 2,
 * but we do not want to store a(i) explicitly, even as a t_VECSMALL, since
 * this would leave an object on the stack. We store a(i) implicitly in a
 * MASK: let a(0) = 1, if the i-bit of MASK is set, set a(i+1) = 2 a(i) - 1,
 * and 2a(i) otherwise.
 *
 * In fact, we do something a little more complicated to simplify the
 * function interface and avoid returning k and MASK separately: we return
 * MASK + 2^(k+1), so the highest bit of the mask indicates the length of the
 * sequence, and the following ones are as above. */
ulong
quadratic_prec_mask(long n)
{
  long a = n, i;
  ulong mask = 0;
  for(i = 1;; i++, mask <<= 1)
  {
    mask |= (a&1); a = (a+1)>>1;
    if (a==1) return mask | (1UL << i);
  }
}

/* Lift to precision p^e0.
 * a = modular factors of f mod (p,T) [possibly T=NULL]
 *  OR a TreeLift structure [e, link, v, w]: go on lifting
 * flag = 0: standard.
 * flag = 1: return TreeLift structure */
static GEN
MultiLift(GEN f, GEN a, GEN T, GEN p, long e0, long flag)
{
  long i, eold, e, k = lg(a) - 1;
  GEN E, v, w, link, penew, Tnew;
  ulong mask;
  pari_timer Ti;

  if (k < 2) pari_err_DOMAIN("MultiLift", "#(modular factors)", "<", gen_2,a);
  if (e0 < 1) pari_err_DOMAIN("MultiLift", "precision", "<", gen_1,stoi(e0));
  if (e0 == 1) return a;

  if (DEBUGLEVEL > 3) timer_start(&Ti);
  if (typ(gel(a,1)) == t_INT)
  { /* a = TreeLift structure */
    e = itos(gel(a,1));
    link = gel(a,2);
    v    = gel(a,3);
    w    = gel(a,4);
  }
  else
  {
    e = 1;
    v = cgetg(2*k-2 + 1, t_VEC);
    w = cgetg(2*k-2 + 1, t_VEC);
    link=cgetg(2*k-2 + 1, t_VECSMALL);
    BuildTree(link, v, w, a, T? FpX_red(T,p): NULL, p);
    if (DEBUGLEVEL > 3) timer_printf(&Ti, "building tree");
  }
  mask = quadratic_prec_mask(e0);
  eold = 1;
  penew = NULL;
  Tnew = NULL;
  while (mask > 1)
  {
    long enew = eold << 1;
    if (mask & 1) enew--;
    mask >>= 1;
    if (enew >= e) { /* mask == 1: last iteration */
      GEN peold = penew? penew: powiu(p, eold);
      GEN Td = NULL, pd;
      long d = enew - eold; /* = eold or eold-1 */
      /* lift from p^eold to p^enew */
      pd = (d == eold)? peold: diviiexact(peold, p); /* p^d */
      penew = mulii(peold,pd);
      if (T) {
        if (Tnew)
          Td = (d == eold)? Tnew: FpX_red(Tnew,pd);
        else
          Td = FpX_red(T, peold);
        Tnew = FpX_red(T, penew);
        ZpXQ_RecTreeLift(link, v, w, Td, Tnew, pd, peold, penew, f, lgpol(v),
                         (flag == 0 && mask == 1));
      }
      else
        ZpX_RecTreeLift(link, v, w, pd, peold, penew, f, lgpol(v),
                        (flag == 0 && mask == 1));
      if (DEBUGLEVEL > 3) timer_printf(&Ti, "lifting to prec %ld", enew);
    }
    eold = enew;
  }

  if (flag)
    E = mkvec4(utoipos(e0), link, v, w);
  else
  {
    E = cgetg(k+1, t_VEC);
    for (i = 1; i <= 2*k-2; i++)
    {
      long t = link[i];
      if (t < 0) gel(E,-t) = gel(v,i);
    }
  }
  return E;
}

/* Q list of (coprime, monic) factors of pol mod (T,p). Lift mod p^e = pe.
 * T may be NULL */
GEN
ZpX_liftfact(GEN pol, GEN Q, GEN T, GEN p, long e, GEN pe)
{
  pari_sp av = avma;
  if (lg(Q) == 2) return mkvec(pol);
  pol = FqX_normalize(pol, T, pe);
  return gerepilecopy(av, MultiLift(pol, Q, T, p, e, 0));
}

/* U = NULL treated as 1 */
static void
BezoutPropagate(GEN link, GEN v, GEN w, GEN pe, GEN U, GEN f, long j)
{
  GEN Q, R;
  if (j < 0) return;

  Q = FpX_mul(gel(v,j), gel(w,j), pe);
  if (U)
  {
    Q = FpXQ_mul(Q, U, f, pe);
    R = FpX_sub(U, Q, pe);
  }
  else
    R = Fp_FpX_sub(gen_1, Q, pe);
  gel(w,j+1) = Q; /*  0 mod U v[j],  1 mod (1-U) v[j+1] */
  gel(w,j) = R; /*  1 mod U v[j],  0 mod (1-U) v[j+1] */
  BezoutPropagate(link, v, w, pe, R, f, link[j  ]);
  BezoutPropagate(link, v, w, pe, Q, f, link[j+1]);
}

/* as above, but return the Bezout coefficients for the lifted modular factors
 *   U[i] = 1 mod Qlift[i]
 *          0 mod Qlift[j], j != i */
GEN
bezout_lift_fact(GEN pol, GEN Q, GEN p, long e)
{
  pari_sp av = avma;
  GEN E, link, v, w, pe;
  long i, k = lg(Q)-1;
  if (k == 1) return mkvec(pol);
  pe = powiu(p, e);
  pol = FpX_normalize(pol, pe);
  E = MultiLift(pol, Q, NULL, p, e, 1);
  link = gel(E,2);
  v    = gel(E,3);
  w    = gel(E,4);
  BezoutPropagate(link, v, w, pe, NULL, pol, lgpol(v));
  E = cgetg(k+1, t_VEC);
  for (i = 1; i <= 2*k-2; i++)
  {
    long t = link[i];
    if (t < 0) E[-t] = w[i];
  }
  return gerepilecopy(av, E);
}

/* Front-end for ZpX_liftfact:
   lift the factorization of pol mod p given by L to p^N (if possible) */
GEN
polhensellift(GEN pol, GEN L, GEN p, long N)
{
  GEN T = NULL;
  long i, l, t;
  pari_sp av = avma;

  if (typ(pol) != t_POL) pari_err_TYPE("polhensellift",pol);
  RgX_check_ZXX(pol, "polhensellift");
  if (!is_vec_t(typ(L)) || lg(L) < 3) pari_err_TYPE("polhensellift",L);
  t = typ(p);
  if (t == t_VEC) /* [p, T] */
  {
    T = gel(p,2);
    if (typ(T) != t_POL) pari_err_TYPE("polhensellift",pol);
    RgX_check_ZX(T, "polhensellift");
    p = gel(p,1); t = typ(p);
  }
  if (t != t_INT) pari_err_TYPE("polhensellift",p);
  if (N < 1) pari_err_DOMAIN("polhensellift", "precision", "<", gen_1,stoi(N));

  l = lg(L); L = leafcopy(L);
  for (i = 1; i < l; i++)
  {
    if (typ(gel(L,i)) != t_POL)
      gel(L,i) = scalar_ZX_shallow(gel(L,i), varn(pol));
    RgX_check_ZXX(gel(L,i), "polhensellift");
  }
  return gerepilecopy(av, ZpX_liftfact(pol, L, T, p, N, powiu(p,N)));
}

/*************************************************************************/
/*                             rootpadicfast                             */
/*************************************************************************/
/* SPEC:
 * p is a t_INT > 1, e >= 1
 * f is a ZX with leading term prime to p.
 * a is a simple root mod l for all l|p.
 * Return roots of f mod p^e, as integers (implicitly mod p^e)
 * STANDARD USE: p is a prime power */
GEN
ZpX_liftroot(GEN f, GEN a, GEN p, long e)
{
  pari_sp av = avma;
  GEN q = p, fr, W;
  ulong mask;

  a = modii(a,q);
  if (e == 1) return a;
  mask = quadratic_prec_mask(e);
  fr = FpX_red(f,q);
  W = Fp_inv(FpX_eval(ZX_deriv(fr), a, q), q); /* 1/f'(a) mod p */
  for(;;)
  {
    q = sqri(q);
    if (mask & 1) q = diviiexact(q, p);
    mask >>= 1;
    fr = FpX_red(f,q);
    a = Fp_sub(a, Fp_mul(W, FpX_eval(fr, a,q), q), q);
    if (mask == 1) return gerepileuptoint(av, a);
    W = Fp_sub(shifti(W,1), Fp_mul(Fp_sqr(W,q), FpX_eval(ZX_deriv(fr),a,q), q), q);
  }
}

GEN
ZpXQX_liftroot_vald(GEN f, GEN a, long v, GEN T, GEN p, long e)
{
  pari_sp av = avma, av2, lim;
  GEN pv = p, q = p, W, df, Tq, fr, dfr;
  ulong mask;
  long k=1;
  a = Fq_red(a, T, q);
  if (e <= v+1) return a;
  df = RgX_deriv(f);
  if (v) { pv = powiu(p,v); q = mulii(pv,p); df = ZXX_Z_divexact(df, pv); }
  mask = quadratic_prec_mask(e-v);
  Tq = FpX_red(T, q); dfr = FpXQX_red(df, Tq, p);
  W = Fq_inv(FqX_eval(dfr, a, Tq, p), Tq, p); /* 1/f'(a) mod (T,p) */
  av2 = avma; lim = stack_lim(av2, 2);
  for (;;)
  {
    GEN fa;
    k <<= 1;
    if (mask & 1) k--;
    mask >>= 1;
    q = powiu(p, v+k); Tq = FpX_red(T, q); fr = FpXQX_red(f, Tq, q);
    fa = FqX_eval(fr, a, Tq, q);
    if (v) fa = typ(fa)==t_INT? diviiexact(fa,pv): ZX_Z_divexact(fa, pv);
    a = Fq_sub(a, Fq_mul(W, fa, Tq,q), Tq,q);
    if (mask == 1) return gerepileupto(av, a);
    if (v) { q = powiu(p, k); Tq = FpX_red(T, q); }
    dfr = FpXQX_red(df, Tq, q);
    W = Fq_sub(gmul2n(W,1), Fq_mul(Fq_sqr(W,Tq,q), FqX_eval(dfr,a,Tq,q), Tq, q), Tq,q);
    if (low_stack(lim, stack_lim(av2,2)))
    {
      if(DEBUGMEM>1) pari_warn(warnmem,"ZpXQX_liftroot, k = %ld/%ld", k+1,e);
      gerepileall(av2, 2, &a, &W);
    }
  }
}

GEN
ZpXQX_liftroot(GEN f, GEN a, GEN T, GEN p, long e) { return ZpXQX_liftroot_vald(f,a,0,T,p,e); }

/* Apply ZpX_liftroot to all roots in S and trace trick.
 * Elements of S must be distinct simple roots mod p for all p|q. */
GEN
ZpX_liftroots(GEN f, GEN S, GEN q, long e)
{
  long i, d, l = lg(S), n = l-1;
  GEN y = cgetg(l, typ(S));
  if (!n) return y;
  for (i=1; i<n; i++)
    gel(y,i) = ZpX_liftroot(f, gel(S,i), q, e);
  d = degpol(f);
  if (n != d) /* not totally split*/
    gel(y,n) = ZpX_liftroot(f, gel(S,n), q, e);
  else
  { /* totally split: use trace trick */
    pari_sp av = avma;
    GEN z = gel(f, d+1);/* -trace(roots) */
    for(i=1; i<n;i++) z = addii(z, gel(y,i));
    z = modii(negi(z), powiu(q,e));
    gel(y,n) = gerepileuptoint(av,z);
  }
  return y;
}

/* Same as ZpX_liftroot for the polynomial X^2-b */
GEN
Zp_sqrtlift(GEN b, GEN a, GEN p, long e)
{
  pari_sp ltop=avma;
  GEN q, w;
  ulong mask;

  if (e == 1) return icopy(a);
  mask = quadratic_prec_mask(e);
  w = Fp_inv(modii(shifti(a,1), p), p);
  q = p;
  for(;;)
  {
    q = sqri(q);
    if (mask & 1) q = diviiexact(q, p);
    mask >>= 1;
    if (lgefint(q) == 3)
    {
      ulong Q = (ulong)q[2];
      ulong A = umodiu(a, Q);
      ulong B = umodiu(b, Q);
      ulong W = umodiu(w, Q);
      A = Fl_sub(A, Fl_mul(W, Fl_sub(Fl_sqr(A,Q), B, Q), Q), Q);
      a = utoi(A);
      if (mask == 1) break;
      W = Fl_sub(Fl_add(W,W,Q), Fl_mul(Fl_sqr(W,Q), Fl_add(A,A,Q),Q), Q);
      w = utoi(W);
    }
    else
    {
      a = modii(subii(a, mulii(w, subii(Fp_sqr(a,q),b))), q);
      if (mask == 1) break;
      w = subii(shifti(w,1), Fp_mul(Fp_sqr(w,q), shifti(a,1),q));
    }
  }
  return gerepileuptoint(ltop,a);
}
/* Same as ZpX_liftroot for the polynomial X^n-b
 * TODO: generalize to sparse polynomials. */
GEN
Zp_sqrtnlift(GEN b, GEN n, GEN a, GEN p, long e)
{
  pari_sp ltop=avma;
  GEN q, w, n_1;
  ulong mask;

  if (equalii(n, gen_2)) return Zp_sqrtlift(b,a,p,e);
  if (e == 1) return icopy(a);
  n_1 = subis(n,1);
  mask = quadratic_prec_mask(e);
  w = Fp_inv(Fp_mul(n,Fp_pow(a,n_1,p), p), p);
  q = p;
  for(;;)
  {
    q = sqri(q);
    if (mask & 1) q = diviiexact(q, p);
    mask >>= 1;
    if (lgefint(q) == 3 && lgefint(n) == 3)
    {
      ulong Q = (ulong)q[2], N = (ulong)n[2];
      ulong A = umodiu(a, Q);
      ulong B = umodiu(b, Q);
      ulong W = umodiu(w, Q);

      A = Fl_sub(A, Fl_mul(W, Fl_sub(Fl_powu(A,N,Q), B, Q), Q), Q);
      a = utoi(A);
      if (mask == 1) break;
      W = Fl_sub(Fl_add(W,W,Q),
                 Fl_mul(Fl_sqr(W,Q), Fl_mul(N,Fl_powu(A, N-1, Q), Q), Q), Q);
      w = utoi(W);
    }
    else
    {
      /* a -= w (a^n - b) */
      a = modii(subii(a, mulii(w, subii(Fp_pow(a,n,q),b))), q);
      if (mask == 1) break;
      /* w += w - w^2 n a^(n-1)*/
      w = subii(shifti(w,1), Fp_mul(Fp_sqr(w,q), mulii(n,Fp_pow(a,n_1,q)), q));
    }
  }
  return gerepileuptoint(ltop,a);
}
/* Same as ZpXQX_liftroot for the polynomial X^n-b */
GEN
ZpXQ_sqrtnlift(GEN b, GEN n, GEN a, GEN T, GEN p, long e)
{
  pari_sp av = avma;
  GEN q = p, n_1, w;
  ulong mask;

  a = Fq_red(a, T, q);
  if (e == 1) return a;
  n_1 = subis(n,1);
  mask = quadratic_prec_mask(e);
  w = Fq_inv(Fq_mul(n, Fq_pow(a, n_1, T,q), T,q), T,q);
  for(;;)
  {
    q = sqri(q);
    if (mask & 1) q = diviiexact(q, p);
    mask >>= 1;
    /* a -= w (a^n - b) */
    a = Fq_sub(a, Fq_mul(w, Fq_sub(Fq_pow(a, n, T,q), b, T,q), T,q), T,q);
    if (mask == 1) return gerepileupto(av, a);
    /* w += w - w^2 n a^(n-1)*/
    w = Fq_sub(gmul2n(w,1), Fq_mul(Fq_sqr(w,T,q), Fq_mul(n, Fq_pow(a,n_1,T, q), T,q), T,q), T,q);
  }
}

/* Same as ZpXQX_liftroot for the polynomial b*X-1 */
GEN
ZpXQ_invlift(GEN b, GEN a, GEN T, GEN p, long e)
{
  pari_sp av = avma;
  GEN q = p;
  ulong mask;

  a = FpX_red(a, q);
  if (e == 1) return a;
  mask = quadratic_prec_mask(e);
  for(;;)
  {
    q = sqri(q);
    if (mask & 1) q = diviiexact(q, p);
    mask >>= 1;
    a = FpXQ_mul(a, Fp_FpX_sub(gen_2, FpXQ_mul(a, FpX_red(b, q) , T, q), q), T,q);
    if (mask == 1) return gerepileupto(av, a);
  }
}

/* Compute (x-1)/(x+1)/p^k */
static GEN
ZpXQ_log_to_ath(GEN x, long k, GEN T, GEN p, long e, GEN pe)
{
  pari_sp av = avma;
  long vT = varn(T);
  GEN bn, bdi;
  GEN bd = ZX_Z_add(x, gen_1);
  if (equaliu(p,2)) /*For p=2, we need to simplify by 2*/
  {
    bn = ZX_shifti(x,-(k+1));
    bdi= ZpXQ_invlift(ZX_shifti(bd ,-1), pol_1(vT), T, p, e);
  }
  else
  {
    bn = ZX_Z_divexact(ZX_Z_sub(x, gen_1),powiu(p,k));
    bdi= ZpXQ_invlift(bd, scalarpol(Fp_inv(gen_2,p),vT), T, p, e);
  }
  return gerepileupto(av, FpXQ_mul(bn, bdi, T, pe));
}

/* Assume p odd, a = 1 [p], return log(a) */
GEN
ZpXQ_log(GEN a, GEN T, GEN p, long N)
{
  pari_sp av = avma;
  long is2 = equaliu(p,2);
  long k = (long) pow((double)(N>>1), 1./3);
  GEN ak = FpXQ_pow(a, powiu(p,k), T, powiu(p,N+k));
  long e = is2 ? N-1: N;
  GEN pe = powiu(p,e), s;
  GEN b = ZpXQ_log_to_ath(ak, k, T, p, e, pe);
  long i, l = (e-2)/(2*(k+is2));
  ulong pp = is2 ? 0: itou_or_0(p);
  GEN pol= cgetg(l+3,t_POL);
  pol[1] = evalsigne(1)|evalvarn(0);
  for(i=0; i<=l; i++)
  {
    GEN g;
    ulong z = 2*i+1;
    if (pp)
    {
      long w = u_lvalrem(z, pp, &z);
      g = powuu(pp,2*i*k-w);
    }
    else g = powiu(p,2*i*k);
    gel(pol,i+2) = Fp_div(g, utoi(z),pe);
  }
  s = FpX_FpXQ_eval(pol, FpXQ_sqr(b, T, pe), T,  pe);
  s = ZX_shifti(FpXQ_mul(b, s, T, pe), 1);
  return gerepileupto(av, is2? s: FpX_red(s, pe));
}

/***********************************************************************/
/**                                                                   **/
/**                 Generic quadratic hensel lift over Zp[X]          **/
/**                                                                   **/
/***********************************************************************/

GEN
gen_ZpX_Dixon(GEN F, GEN V, GEN p, long N, void *E,
                            GEN lin(void *E, GEN F, GEN d, GEN q),
                            GEN invl(void *E, GEN d))
{
  pari_sp av = avma;
  long N2, M;
  GEN VN2, V2, VM, bil;
  GEN q = powiu(p, N), q2;
  V = FpX_red(V, q);
  if (N == 1) return invl(E, V);
  N2 = (N + 1)>>1; M = N - N2;
  F = FpXV_red(F, q);
  VN2 = gen_ZpX_Dixon(F, V, p, N2, E, lin, invl);
  bil = lin(E, F, VN2, q);
  q2 = powiu(p, N2);
  V2 = ZX_Z_divexact(ZX_sub(V, bil), q2);
  VM = gen_ZpX_Dixon(F, V2, p, M, E, lin, invl);
  return gerepileupto(av, FpX_red(ZX_add(VN2, ZX_Z_mul(VM, q2)), q));
}

GEN
gen_ZpX_Newton(GEN x, GEN p, long n, void *E,
                      GEN eval(void *E, GEN f, GEN q),
                      GEN invd(void *E, GEN V, GEN v, long M))
{
  pari_sp ltop = avma, av, st_lim;
  long N = 1, N2, M;
  long mask = quadratic_prec_mask(n);
  av = avma; st_lim = stack_lim(av, 1);
  while (mask > 1)
  {
    GEN q, q2, v, V;
    N2 = N; N <<= 1; M = N-N2;
    if (mask&1UL) N--;
    mask >>= 1; q = powiu(p,N); q2 = powiu(p, N2);
    v = eval(E, x, q);
    V = ZX_Z_divexact(gel(v,1), q2);
    x = FpX_sub(x, ZX_Z_mul(invd(E, V, v, M), q2), q);
    if (low_stack(st_lim, stack_lim(av, 1)))
      x = gerepileupto(av, x);
  }
  return gerepileupto(ltop, x);
}
