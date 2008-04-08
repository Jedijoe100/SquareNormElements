/* $Id: ellsea.c 9561 2008-01-30 14:30:52Z kb $

Copyright (C) 2008  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* This file is a C version by Bill Allombert of the 'ellsea' GP package
 * whose copyright statement is as follows:
Authors:
  Christophe Doche   <cdoche@math.u-bordeaux.fr>
  Sylvain Duquesne <duquesne@math.u-bordeaux.fr>

Universite Bordeaux I, Laboratoire A2X
For the AREHCC project, see http://www.arehcc.com/

Contributors:
  Karim Belabas (code cleanup and package release, faster polynomial arithmetic)

'ellsea' is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER. */

#include "pari.h"
#include "paripriv.h"

static THREAD GEN modular_eqn;

void
pari_init_seadata(void) { modular_eqn = NULL; }
void
pari_close_seadata(void) { if (modular_eqn) gunclone(modular_eqn); }

static int
get_seadata(void)
{
  char *s = pari_sprintf("%s/seadata/sea0", pari_datadir);
  pariFILE *F = pari_fopengz(s);
  free(s); if (!F) return 0;
  modular_eqn = gclone(gp_readvec_stream(F->file));
  pari_fclose(F); return 1;
}

/*Builds the modular equation corresponding to the vector list */
static GEN
list_to_pol(GEN list)
{
  pari_sp ltop = avma;
  long i, l = lg(list);
  GEN P = cgetg(l, t_VEC);
  for (i = 1; i < l; i++) gel(P, i) = gtopoly(gel(list,i), MAXVARN);
  return gerepileupto(ltop, gtopoly(P, -1));
}

static GEN
get_modular_eqn(long ell, char *type)
{
  long i;
  for (i=1; i<lg(modular_eqn); i++)
    if (equalis(gmael(modular_eqn, i, 1), ell))
    {
      *type = *GSTR(gmael(modular_eqn, i, 2));
      return list_to_pol(gmael(modular_eqn, i, 3));
    }
  return NULL;
}

/*Gives the first precS terms of the Weierstrass series related to */
/*E: y^2 = x^3 + a4x + a6.  Assumes (precS-2)*(2precS+3) < ULONG_MAX, i.e.
 * precS < 46342 in 32-bit machines */
static GEN
find_coeff(GEN a4, GEN a6, GEN p, long precS)
{
  GEN res = cgetg(precS+1, t_VEC);
  long k, h;
  if (precS == 0) return res;
  gel(res, 1) = Fp_div(a4, stoi(-5), p);
  if (precS == 1) return res;
  gel(res, 2) = Fp_div(a6, stoi(-7), p);
  for (k = 3; k <= precS; ++k)
  {
    pari_sp btop = avma;
    GEN a = gen_0;
    for (h = 1; h <= k-2; h++)
      a = Fp_add(a, Fp_mul(gel(res, h), gel(res, k-1-h), p), p);
    a = Fp_div(Fp_mulu(a, 3, p), utoi((k-2) * (2*k + 3)), p);
    gel(res, k) = gerepileuptoint(btop, a);
  }
  return res;
}

/*Computes the n-division polynomial modulo the polynomial h \in Fp[x] */
static GEN
elldivpol(GEN a4, GEN a6, long n, GEN h, GEN p)
{
  pari_sp ltop = avma;
  GEN f, f2, ff, rhs, inv2y, a42, res;
  long N, m, l;
  if (n <= 2) return n==1 ?gen_1: gen_2;
  N = maxss(5, (n+1)/2 + 3);
  f  = cgetg(N+1, t_VEC);
  f2 = cgetg(N+1, t_VEC); /*f2[m]=f[m]^2 */
  ff = cgetg(N+1, t_VEC); /*ff[m]=f[m]*f[m-2] */
  rhs = FpX_rem(mkpoln(4, gen_1, gen_0, a4, a6), h, p);
  inv2y = FpXQ_inv(FpX_Fp_mul(rhs, gen_2, p), h, p);
  gel(f, 2) = scalarpol(gen_2,0);
  gel(f2, 2) = FpX_Fp_mul(rhs, utoi(4), p);
  a42 = Fp_sqr(a4, p);
  gel(f, 3) = FpX_rem(mkpoln(5, utoi(3), gen_0, Fp_mul(utoi(6), a4, p), 
                        Fp_mul(utoi(12), a6, p), Fp_neg(a42, p)), h, p);
  if (n == 3) return gerepileupto(ltop, gel(f, 3));
  gel(f, 4) = FpX_rem(FpX_Fp_mul(mkpoln(7, gen_1, gen_0,
    Fp_mul(utoi(5), a4, p), Fp_mul(utoi(20), a6, p),
    Fp_mul(a42,subis(p,5), p), Fp_mul(Fp_mul(a4, a6, p), subis(p,4), p),
    Fp_sub(Fp_mul(Fp_sqr(a6, p), subis(p,8), p), Fp_mul(a4,a42, p), p)),
                                 utoi(4), p), h, p);
  if (n == 4) return gerepileupto(ltop, gel(f, 4));
  gel(f2, 3) = FpXQ_sqr(gel(f, 3), h, p);
  gel(ff, 3) = gel(f, 3);
  gel(ff, 4) = FpX_Fp_mul(FpXQ_mul(rhs, gel(f, 4), h, p), gen_2, p);
  gel(f, 5)  = FpX_sub(FpXQ_mul(gel(ff, 4), gel(f2, 2), h, p), 
                      FpXQ_mul(gel(ff, 3), gel(f2, 3), h, p), p);
  if (n == 5) return gerepileupto(ltop, gel(f, 5));
  gel(f2, 4) = FpXQ_mul(rhs, FpXQ_sqr(gel(f, 4), h, p), h, p);
  gel(f2, 5) = FpXQ_sqr(gel(f, 5), h, p);
  gel(ff, 5) = FpXQ_mul(gel(f, 3), gel(f, 5), h, p);
  l = ((n/2) + 2)/2;
  for (m = 3; m <= l; m++)
  {
    gel(f, 2*m) = FpXQ_mul(
                      FpX_sub(FpXQ_mul(gel(ff, m+2), gel(f2, m-1), h, p),
                              FpXQ_mul(gel(ff, m), gel(f2, m+1), h, p), p),
                      inv2y, h, p);
    gel(f2, 2*m) = FpXQ_mul(rhs, FpXQ_sqr(gel(f, 2*m), h, p), h, p);
    gel(ff, 2*m) = FpXQ_mul(rhs,
                            FpXQ_mul(gel(f, 2*m), gel(f, 2*m-2), h, p), h, p);
    gel(f, 2*m+1) = FpX_sub(FpXQ_mul(gel(ff, m+2), gel(f2, m), h, p),
                            FpXQ_mul(gel(ff, m+1), gel(f2, m+1), h, p), p);
    gel(f2, 2*m+1) = FpXQ_sqr(gel(f, 2*m+1), h, p);
    gel(ff, 2*m+1) = FpXQ_mul(gel(f, 2*m+1), gel(f, 2*m-1), h, p);
  }
  m = n/2;
  if (n&1L)
    res = FpX_sub(FpXQ_mul(gel(ff, m+2), gel(f2, m), h, p), 
                  FpXQ_mul(gel(ff, m+1), gel(f2, m+1), h, p), p);
  else
    res = FpXQ_mul(
               FpX_sub(FpXQ_mul(gel(ff, m+2), gel(f2, m-1), h, p),
                       FpXQ_mul(gel(ff, m), gel(f2, m+1), h, p), p),
               inv2y, h, p);
  return gerepileupto(ltop, res);
}

/* Given power series s1 and s2, finds a polynomial P such that s2 = P(s1) */
static GEN
find_transformation(GEN s2, GEN s1)
{
  pari_sp ltop = avma, btop, st_lim;
  long i, vx = varn(s1), vs1 = valp(s1), vs2 = valp(s2), degP = vs2/vs1;
  GEN invs1coeff = ginv(gel(s1, 2)), P = gen_0, s1pl = cgetg(degP+1, t_VEC);

  gel(s1pl, 1) = s1;
  for (i = 2; i <= degP; i++) gel(s1pl, i) = gmul(s1, gel(s1pl, i-1));
  btop = avma; st_lim = stack_lim(btop, 1);
  for (i = 0; i < degP; i++)
  {
    GEN Pcoeff = gmul(gel(s2,2), invs1coeff);
    P = gadd(P, gmul(Pcoeff, monomial(gen_1, degP-i, vx)));
    s2 = gsub(s2, gmul(Pcoeff, gel(s1pl, degP-i)));
    if (low_stack(st_lim, stack_lim(btop, 1))) gerepileall(btop, 2, &P, &s2);
  }
  P = gadd(P, gmul(gel(s2,2), invs1coeff));
  return gerepileupto(ltop, P);
}

static GEN
compute_W(GEN a4, GEN a6, GEN p, long vx, long precS)
{
  pari_sp ltop = avma;
  GEN c  = find_coeff(a4, a6, p, precS);
  GEN s  = RgX_inflate(RgV_to_RgX(c,vx), 2);
  GEN z2 = monomial(gen_1, 2, vx);
  s = gadd(gadd(ginv(z2), gmul(s, z2)), zeroser(vx, 2*precS));
  return gerepileupto(ltop, s);
}

/*Finds numerator phi of the isogeny between Eb and Ec whose denominator is h*/
static GEN
find_numerator_isogeny(GEN Eba4, GEN Eba6, GEN Eca4, GEN Eca6, GEN h, GEN p,
                       long precS)
{
  pari_sp ltop = avma;
  GEN WEb = gmul(compute_W(Eba4, Eba6, p, varn(h), precS), gmodulsg(1,p));
  GEN WEc = gmul(compute_W(Eca4, Eca6, p, varn(h), precS), gmodulsg(1,p));
  GEN den = poleval(h, WEb);
  return gerepileupto(ltop, find_transformation(gmul(gsqr(den), WEc), WEb));
}

/****************************************************************************/
/*               SIMPLE ELLIPTIC CURVE OVER Fp                              */
/****************************************************************************/

INLINE int
is_inf(GEN z) { return lg(z) < 3; }

static GEN
Fp_ell_j(GEN a4, GEN a6, GEN p)
{
  pari_sp ltop=avma;
  GEN a43 = Fp_mulu(Fp_powu(a4, 3, p), 4, p);
  GEN j   = Fp_div(Fp_mulu(a43, 1728, p),
                   Fp_add(a43, Fp_mulu(Fp_sqr(a6, p), 27, p), p), p);
  return gerepileupto(ltop, j);
}

static GEN
Fp_ell_dbl(GEN P, GEN a4, GEN p)
{
  pari_sp ltop = avma;
  GEN lambda, C, D, x = gel(P,1), y = gel(P,2);
  if (is_inf(P) || !signe(y)) return mkvec(gen_0);
  lambda = Fp_div(Fp_add(Fp_mulu(Fp_sqr(x,p), 3, p), a4, p), 
                  Fp_mulu(y, 2, p), p);
  C = Fp_sub(Fp_sqr(lambda, p), Fp_mulu(x, 2, p), p);
  D = Fp_sub(Fp_mul(lambda, Fp_sub(x, C, p), p), y, p);
  return gerepilecopy(ltop, mkvec2(C,D));
}

static GEN
Fp_ell_add_i(GEN P, GEN Q, GEN a4, GEN p)
{
  GEN Px = gel(P,1), Py = gel(P,2);
  GEN Qx = gel(Q,1), Qy = gel(Q,2), lambda, C, D;
  if (is_inf(P)) return gcopy(Q);
  if (is_inf(Q)) return gcopy(P);
  if (equalii(Px, Qx))
  {
    if (equalii(Py, Qy))
      return Fp_ell_dbl(P, a4, p);
    else
      return mkvec(gen_0);
  }
  lambda = Fp_div(Fp_sub(Py, Qy, p), Fp_sub(Px, Qx, p), p);
  C = Fp_sub(Fp_sub(Fp_sqr(lambda, p), Px, p), Qx, p);
  D = Fp_sub(Fp_mul(lambda, Fp_sub(Px, C, p), p), Py, p);
  return mkvec2(C,D);
}
static GEN
Fp_ell_add(GEN P, GEN Q, GEN a4, GEN p)
{
  pari_sp av = avma;
  return gerepilecopy(av, Fp_ell_add_i(P,Q,a4,p));
}

static GEN
Fp_ell_inv(GEN P, GEN p)
{
  GEN t;
  if (is_inf(P)) return P;
  t = cgetg(3, t_VEC);
  gel(t, 1) = icopy(gel(P,1));
  gel(t, 2) = Fp_neg(gel(P,2), p);
  return t;
}

static GEN
Fp_ell_sub(GEN P, GEN Q, GEN a4, GEN p)
{
  pari_sp av = avma;
  return gerepilecopy(av, Fp_ell_add_i(P, Fp_ell_inv(Q, p), a4, p));
}

struct _Fp_ell
{
  GEN a4;
  GEN p;
};
static GEN
_Fp_ell_dbl(void *E, GEN P)
{
  struct _Fp_ell *ell=(struct _Fp_ell *) E;
  return Fp_ell_dbl(P, ell->a4, ell->p);
}
static GEN
_Fp_ell_add(void *E, GEN P, GEN Q)
{
  struct _Fp_ell *ell=(struct _Fp_ell *) E;
  return Fp_ell_add(P, Q, ell->a4, ell->p);
}

static GEN
Fp_ell_pow(GEN P, GEN n, GEN a4, GEN p)
{
  pari_sp av = avma;
  struct _Fp_ell E;
  if (!signe(n) || is_inf(P)) return mkvec(gen_0);
  if (is_pm1(n)) return gcopy(P);
  E.a4= a4; E.p = p;
  return gerepileupto(av, leftright_pow(P, n, &E, &_Fp_ell_dbl, &_Fp_ell_add));
}

/****************************************************************************/
/*                              EIGENVALUE                                  */
/****************************************************************************/

struct eigen_ellinit
{
  GEN a4;
  GEN h;
  GEN p;
  GEN RHS;
  GEN DRHS;
  GEN X12;
  GEN Gr;
};

static void
init_eigen(struct eigen_ellinit *Edat, GEN a4, GEN a6, GEN h, GEN p)
{
  pari_sp ltop = avma;
  GEN RHS  = mkpoln(4, gen_1, gen_0, a4, a6);
  GEN DRHS = mkpoln(3, utoi(3), gen_0, a4);
  GEN lambda = FpXQ_div(DRHS, FpX_Fp_mul(RHS, utoi(4), p), h, p);
  GEN C = FpX_sub(FpXQ_mul(lambda, DRHS, h, p), monomial(gen_2,1,0), p);
  GEN D = FpXQ_mul(FpX_Fp_mul(lambda, gen_2, p),FpX_sub(pol_x(0), C, p), h, p);
  GEN X12 = mkvec2(C, FpX_Fp_add(D, gen_m1, p));
  GEN Gr = FpXQ_pow(RHS, shifti(p, -1), h, p);
  gerepileall(ltop, 4, &RHS, &DRHS, &X12, &Gr);
  Edat->a4    = icopy(a4);
  Edat->h     = ZX_copy(h);
  Edat->p     = icopy(p);
  Edat->RHS   = RHS;
  Edat->DRHS  = DRHS;
  Edat->X12   = X12;
  Edat->Gr    = Gr;
}

static GEN
eigen_elldbl(void *E, GEN P)
{
  pari_sp ltop = avma;
  struct eigen_ellinit *Edat=(struct eigen_ellinit *)E;
  GEN p = Edat->p, h = Edat->h, x = gel(P,1), y = gel(P,2);
  if (is_inf(P)) return gcopy(P);
  if (gequal(x, pol_x(0)) && gequal(y, gen_1))
    return Edat->X12;   
  else
  {
    GEN t1 = FpX_Fp_add(FpX_Fp_mul(FpXQ_sqr(x,h,p),utoi(3),p), Edat->a4, p);
    GEN t2 = FpXQ_mul(FpX_Fp_mul(y, gen_2, p), Edat->RHS, h, p);
    GEN lambda = FpXQ_div(t1, t2, h, p);
    GEN C = FpX_sub(FpXQ_mul(FpXQ_sqr(lambda, h, p), Edat->RHS, h, p),
                    FpX_Fp_mul(x, gen_2, p), p);
    GEN D = FpX_sub(FpXQ_mul(lambda, FpX_sub(x, C, p), h, p), y, p);
    return gerepilecopy(ltop, mkvec2(C,D));
  }
}

/* Returns the addition of [P[1], P[2]*Y] and of [Q[1], Q[2]*Y]
 * Computations are done modulo Y^2 - (X^3 + a4X + a6)
 * An inversion is equivalent to 4M, so that this function requires about 7M
 * which is the same as with the method using ell-division polynomials
 * Working in mixed projective coordinates would require 11M */ 
static GEN
eigen_elladd(void *E, GEN P, GEN Q)
{
  pari_sp ltop = avma;
  struct eigen_ellinit *Edat=(struct eigen_ellinit *)E;
  GEN Px = gel(P,1), Py = gel(P,2);
  GEN Qx = gel(Q,1), Qy = gel(Q,2);
  GEN p = Edat->p, h = Edat->h, lambda, C, D;
  if (is_inf(P)) return gcopy(Q);
  if (is_inf(Q)) return gcopy(P);
  if (gequal(Px, Qx))
  {
    if (gequal(Py, Qy))
      return eigen_elldbl(E, P);
    else
      return mkvec(gen_0);
  }
  lambda = FpXQ_div(FpX_sub(Py, Qy, p), FpX_sub(Px, Qx, p), h, p);
  C = FpX_sub(FpX_sub(FpXQ_mul(FpXQ_sqr(lambda, h, p), Edat->RHS, h, p), Px, p), Qx, p);
  D = FpX_sub(FpXQ_mul(lambda, FpX_sub(Px, C, p), h, p), Py, p);
  return gerepilecopy(ltop, mkvec2(C,D));
}

static GEN
eigen_ellpow(struct eigen_ellinit *E, GEN z, ulong n)
{
  pari_sp av = avma;
  if (!n || is_inf(z)) return mkvec(gen_0);
  if (n == 1) return gcopy(z);
  return gerepileupto(av, leftright_pow_u(z, n, E, &eigen_elldbl, &eigen_elladd));
}

/*Finds the eigenvalue of the Frobenius given E, ell odd prime, h factor of the
 *ell-division polynomial, p and tr the possible values for the trace
 *(useful for primes with one root)*/
static ulong
find_eigen_value(GEN a4, GEN a6, ulong ell, GEN h, GEN p, GEN tr)
{
  pari_sp ltop = avma;
  GEN BP, Dr;
  ulong t;
  struct eigen_ellinit Edat;
  init_eigen(&Edat, a4, a6, h, p);
  Dr = BP = mkvec2(pol_x(0), pol_1(0));
  /*[0,Gr], BP, Dr are not points on the curve. */
  /*To obtain the corresponding points, multiply the y-coordinates by Y */
  if (!tr || lg(tr)==1)
  {
    pari_sp btop = avma;
    for (t = 1; t <= (ell>>1); t++)
    {
      if (gequal(gel(Dr,2), Edat.Gr)) { avma = ltop; return t; }
      if (gequal(gel(Dr,2), FpX_neg(Edat.Gr, p))) { avma = ltop; return ell-t; }
      Dr = gerepileupto(btop, eigen_elladd((void*)&Edat, Dr, BP));
    }
    pari_err(bugparier,"find_eigen_value_power");
    return 0; /* NOT REACHED */
  }
  else
  {
    t = Fl_div(tr[1], 2, ell);
    if (t < (ell>>1)) t = ell - t;
    Dr = eigen_ellpow(&Edat, BP, t);
    if (!gequal(gel(Dr,2), Edat.Gr)) t = ell - t;
    avma = ltop; return t;
  }
}

/*Finds the eigenvalue of the Frobenius modulo ell^k given E, ell, k, h factor
 *of the ell-division polynomial, lambda the previous eigen value and p */
static ulong
find_eigen_value_power(GEN a4, GEN a6, ulong ell, long k, GEN h, ulong lambda, GEN p)
{
  pari_sp ltop = avma;
  pari_sp btop, st_lim;
  struct eigen_ellinit Edat;
  GEN P, BP, Dr, Gr, negGr;
  /*[0,Gr], BP, Dr are not points on the curve. */
  /*To obtain the corresponding points, multiply the y-coordinates by Y */
  ulong t, ellk1 = upowuu(ell, k-1), ellk = ell*ellk1;
  init_eigen(&Edat, a4, a6, h, p);
  P = mkvec2(pol_x(0), pol_1(0));
  BP = eigen_ellpow(&Edat, P, ellk1);
  Dr = eigen_ellpow(&Edat, P, lambda);
  Gr = Edat.Gr;
  negGr = FpX_neg(Edat.Gr, p);

  btop = avma; st_lim = stack_lim(btop, 1);
  for (t = 0; t < ellk; t += ellk1)
  {
    if (gequal(gel(Dr,2), Gr)) { avma = ltop; return t+lambda; }
    if (gequal(gel(Dr,2), negGr)) { avma = ltop; return ellk-(t+lambda); }
    Dr = eigen_elladd((void*)&Edat, Dr, BP);
    if (low_stack(st_lim, stack_lim(btop, 1))) Dr = gerepileupto(btop, Dr);
  }
  pari_err(bugparier,"find_eigen_value_power");
  return 0; /* NOT REACHED */
}

/*Finds the kernel polynomial h, dividing the ell-division polynomial from the
  isogenous curve Eb and trace term pp1.
  Uses CCR algorithm and returns [h, 1] if the result is correct or [h, 0]
  otherwise (E and Eb not isogenous) */
static GEN
find_kernel(GEN a4, GEN a6, ulong ell, GEN a4t, GEN a6t, GEN pp1, GEN p, long *ptr_iszero)
{
  const long ext = 2;
  pari_sp ltop = avma;
  GEN M, N, V, K, K1, K2, v, tlist, res;
  long i, j, k;
  long deg = (ell - 1)/2, dim = deg + ext;
  long iszero = 1;
  GEN Coeff  = find_coeff(a4, a6, p, dim);
  GEN Coefft = find_coeff(a4t, a6t, p, dim);
  GEN psi2  = mkpoln(4, utoi(4), gen_0, Fp_mulu(a4, 4, p), Fp_mulu(a6, 4, p));
  GEN list  = cgetg(dim+1, t_VEC);
  GEN Dpsi2 = mkpoln(3, utoi(6), gen_0, Fp_mulu(a4, 2, p));
  gel(list, 1) = Dpsi2;
  for (k = 2; k <= dim; k++)
  {
    pari_sp btop = avma;
    GEN tsil = gel(list, k-1);
    GEN r = FpX_Fp_mul(Dpsi2, gel(tsil,3), p);
    for (j = 4; j < lg(tsil); j++)
    {
      long o = j - 2;
      GEN D = FpX_add(RgX_shift(Dpsi2, 1), FpX_Fp_mul(psi2, utoi(o-1), p), p);
      GEN E = FpX_Fp_mul(D, Fp_mulu(gel(tsil, j), o, p), p);
      r = FpX_add(r, RgX_shift(E, o-2), p);
    }
    gel(list, k) = gerepileupto(btop, r);
  }
  for (k = 2; k <= dim; k++)
  {
     GEN C = Fp_inv(shifti(mpfact(2*k),-1), p); 
     gel(list, k) = FpX_Fp_mul(gel(list, k), C, p);
  }
  M = shallowtrans(RgXV_to_RgM(list, dim+2));
  N = vecslice(M, 1, dim);
  V = FpC_sub(Coefft, Coeff, p);
  v = shallowconcat(FpM_gauss(N, V, p), mkcol2(gen_0, gen_0));
  K = FpM_ker(M, p);
  if (lg(K) != 3) pari_err(talker, "trace not determined in a unique way");
  K1 = FpC_Fp_mul(gel(K,1), Fp_inv(gcoeff(K,1,1), p), p);
  K2 = FpC_sub(gel(K,2), FpC_Fp_mul(K1, gcoeff(K,1,2), p), p);
  K2 = FpC_Fp_mul(K2, Fp_inv(gel(K2,2), p), p);
  K1 = FpC_sub(K1, FpC_Fp_mul(K2, gel(K1,2), p), p);
  v = FpC_add(v, FpC_Fp_mul(K1, Fp_sub(utoi(deg), gel(v,1), p), p), p);
  v = FpC_add(v, FpC_Fp_mul(K2, Fp_sub(pp1, gel(v,2), p), p), p);
  tlist = cgetg(dim+2, t_VEC);
  gel(tlist, dim+1) = gen_1;
  for (k = 1; k <= dim; k++)
  {
    pari_sp btop = avma;
    GEN s = gel(v, k+1);
    for (i = 1; i < k; i++)
      s = Fp_add(s, Fp_mul(gel(tlist, dim-i+1), gel(v, k-i+1), p), p);
    gel(tlist, dim-k+1) = gerepileuptoint(btop, Fp_div(s, stoi(-k), p));
  }
  for (i = 1; i <= ext; i++)
    if (signe(gel(tlist, i))) { iszero = 0; break; }
  res = vecslice(tlist, ext+1, dim+1);
  *ptr_iszero = iszero;
  return gerepilecopy(ltop, RgV_to_RgX(res, 0));
}

static GEN
compute_u(GEN gprime, GEN Dxxg, GEN DxJg, GEN DJJg, GEN j, GEN pJ, GEN px, ulong q, GEN E4, GEN E6, GEN p)
{
  pari_sp ltop = avma;
  GEN dxxgj = FpX_eval(Dxxg, j, p);
  GEN dxJgj = FpX_eval(DxJg, j, p);
  GEN dJJgj = FpX_eval(DJJg, j, p);
  GEN E42 = Fp_sqr(E4, p), E6ovE4 = Fp_div(E6, E4, p);
  GEN a = Fp_mul(gprime, dxxgj, p);
  GEN b = Fp_mul(Fp_mul(Fp_mulu(j,2*q, p), dxJgj, p), E6ovE4, p);
  GEN c = Fp_mul(Fp_div(Fp_sqr(E6ovE4, p), gprime, p), j, p);
  GEN d = Fp_mul(Fp_mul(c,sqru(q), p), Fp_add(pJ, Fp_mul(j, dJJgj, p), p), p);
  GEN e = Fp_sub(Fp_div(E6ovE4,utoi(3), p), Fp_div(E42, Fp_mulu(E6,2,p), p), p);
  GEN f = Fp_sub(Fp_sub(b,a,p), d, p);
  return gerepileuptoint(ltop, Fp_add(Fp_div(f,px,p), Fp_mulu(e,q,p), p));
}

/* Finds the isogenous EC, and the sum of the x-coordinates of the points in
 * the kernel of the isogeny E -> Eb 
 * E: elliptic curve, q: a prime, meqn: Atkin modular equation
 * g: root of meqn defining isogenous curve Eb. */
static GEN
find_isogenous_from_Atkin(GEN a4, GEN a6, long q, GEN meqn, GEN g, GEN p)
{
  pari_sp ltop = avma, btop;
  GEN Roots, gprime, u1;
  long vx = 0, vJ = MAXVARN;
  GEN E4 = Fp_div(a4, stoi(-3), p);
  GEN E6 = Fp_mul(a6, shifti(p, -1), p);
  GEN E42 = Fp_sqr(E4, p);
  GEN E43 = Fp_mul(E4, E42, p);
  GEN E62 = Fp_sqr(E6, p);
  GEN delta = Fp_div(Fp_sub(E43, E62, p), utoi(1728), p);
  GEN j = Fp_div(E43, delta, p);
  GEN Dx = deriv(meqn, vx);
  GEN DJ = deriv(meqn, vJ);
  GEN Dxg = FpXY_evaly(Dx, g, p, vJ);
  GEN px = FpX_eval(Dxg, j, p), dx = Fp_mul(px, g, p);
  GEN DJg = FpXY_evaly(DJ, g, p, vJ);
  GEN pJ = FpX_eval(DJg, j, p), dJ = Fp_mul(pJ, j, p);
  GEN Dxx = deriv(Dx, vx);
  GEN DxJg = FpX_deriv(Dxg, p);

  GEN Dxxg = FpXY_evaly(Dxx, g, p, vJ);
  GEN DJJg = FpX_deriv(DJg, p);
  long k;
  if (!signe(dx) || !signe(E4))
  {
    if (DEBUGLEVEL)
      fprintferr("find_isogenous_from_Atkin: division by zero at prime %ld", q);
    avma = ltop; return NULL;
  }
  gprime = Fp_div(Fp_mul(dJ, Fp_mul(g, E6, p), p), Fp_mul(E4, dx, p), p);
  u1 = compute_u(gprime, Dxxg, DxJg, DJJg, j, pJ, px, 1, E4, E6, p);
  Roots = FpX_roots(FpXY_evaly(meqn, g, p, vJ), p);
  btop = avma;
  for (k = lg(Roots)-1; k >= 1; k--, avma = btop)
  {
    GEN jt = gel(Roots, k);
    GEN pxstar = FpX_eval(Dxg, jt, p);
    GEN dxstar = Fp_mul(pxstar, g, p);
    GEN pJstar = FpX_eval(DJg, jt, p);
    GEN dJstar = Fp_mul(Fp_mulu(jt, q, p), pJstar, p);
    GEN u = Fp_mul(Fp_mul(dxstar, dJ, p), E6, p);
    GEN v = Fp_mul(Fp_mul(dJstar, dx, p), E4, p);
    GEN E4t = Fp_div(Fp_mul(Fp_sqr(u, p), jt, p), Fp_mul(Fp_sqr(v, p), Fp_sub(jt, utoi(1728), p), p), p);
    GEN E6t = Fp_div(Fp_mul(u, E4t, p), v, p);
    GEN u2 = compute_u(gprime, Dxxg, DxJg, DJJg, jt, pJstar, pxstar, q, E4t, E6t, p);
    GEN pp1 = Fp_mulu(Fp_sub(u1, u2, p), 3*q, p);
    GEN a4t = Fp_mul(mulsi(-3, powuu(q,4)), E4t, p);
    GEN a6t = Fp_mul(mulsi(-2, powuu(q,6)), E6t, p);
    long iszero;
    GEN check = find_kernel(a4, a6, q, a4t, a6t, pp1, p, &iszero);
    if (iszero) return gerepilecopy(ltop, mkvec3(a4t, a6t, check));
  }
  pari_err(bugparier, "find_isogenous_from_Atkin, kernel not found");
  return NULL;
}

/* Finds E' q-isogenous to E and the trace term p1 from canonical modular
 *   equation meqn
 * E: elliptic curve, q: a prime, meqn: canonical modular equation
 * g: root of meqn defining isogenous curve Eb. */
static GEN
find_isogenous_from_canonical(GEN a4, GEN a6, long q, GEN meqn, GEN g, GEN p)
{
  pari_sp ltop = avma;
  long vx = 0, vJ = MAXVARN;
  GEN E4 = Fp_div(a4, stoi(-3), p);
  GEN E6 = Fp_mul(a6, shifti(p, -1), p);
  GEN E42 = Fp_sqr(E4, p);
  GEN E43 = Fp_mul(E4, E42, p);
  GEN E62 = Fp_sqr(E6, p);
  GEN delta = Fp_div(Fp_sub(E43, E62, p), utoi(1728), p);
  GEN j = Fp_div(E43, delta, p);
  GEN Dx = deriv(meqn, vx);
  GEN DJ = deriv(meqn, vJ);
  GEN Dxg = FpXY_evaly(Dx, g, p, vJ);
  GEN px  = FpX_eval(Dxg, j, p), dx  = Fp_mul(px, g, p);
  GEN DJg = FpXY_evaly(DJ, g, p, vJ);
  GEN pJ = FpX_eval(DJg, j, p), dJ = Fp_mul(j, pJ, p);
  GEN Dxx = deriv(Dx, vx);
  GEN DxJg = FpX_deriv(Dxg, p);

  GEN ExJ = FpX_eval(DxJg, j, p);
  ulong tis = ugcd(12, q-1), is = 12 / tis;
  GEN itis = Fp_inv(stoi(-tis), p);
  GEN deltal = Fp_div(Fp_mul(delta, Fp_powu(g, tis, p), p), powuu(q, 12), p);
  GEN E4l, E6l, a4tilde, a6tilde, p_1;
  if (gcmp0(dJ))
  {
    GEN jl;
    if (DEBUGLEVEL) fprintferr("Division by zero for prime %Zs\n", p);
    E4l = Fp_div(E4, sqru(q), p);
    jl  = Fp_div(Fp_powu(E4l, 3, p), deltal, p);
    E6l = Fp_sqrt(Fp_mul(Fp_sub(jl, utoi(1728), p), deltal, p), p);
    p_1 = gen_0;
  }
  else
  {
    GEN jl, f, fd, Dgs, Djs, jld;
    GEN E2s = Fp_div(Fp_mul(Fp_neg(Fp_mulu(E6, 12, p), p), dJ, p), Fp_mul(Fp_mulu(E4, is, p), dx, p), p);
    GEN gd = Fp_mul(Fp_mul(E2s, itis, p), g, p);
    GEN jd = Fp_div(Fp_mul(Fp_neg(E42, p), E6, p), delta, p);
    GEN E0b = Fp_div(E6, Fp_mul(E4, E2s, p), p);
    GEN Dxxgj = FpXY_eval(Dxx, g, j, p);
    GEN Dgd = Fp_add(Fp_mul(gd, px, p), Fp_mul(g, Fp_add(Fp_mul(gd, Dxxgj, p), Fp_mul(jd, ExJ, p), p), p), p);
    GEN DJgJj = FpX_eval(FpX_deriv(DJg, p), j, p);
    GEN Djd = Fp_add(Fp_mul(jd, pJ, p), Fp_mul(j, Fp_add(Fp_mul(jd, DJgJj, p), Fp_mul(gd, ExJ, p), p), p), p);
    GEN E0bd = Fp_div(Fp_sub(Fp_mul(Dgd, itis, p), Fp_mul(E0b, Djd, p), p), dJ, p);
    E4l = Fp_div(Fp_sub(E4, Fp_mul(E2s, Fp_sub(Fp_sub(Fp_add(Fp_div(Fp_mulu(E0bd, 12, p), E0b, p), Fp_div(Fp_mulu(E42, 6, p), E6, p), p), Fp_div(Fp_mulu(E6, 4, p), E4, p), p), E2s, p), p), p), sqru(q), p);
    jl = Fp_div(Fp_powu(E4l, 3, p), deltal, p);
    f =  Fp_div(powuu(q, is), g, p);
    fd = Fp_neg(Fp_mul(Fp_mul(E2s, f, p), itis, p), p);
    Dgs = FpXY_eval(Dx, f, jl, p);
    Djs = FpXY_eval(DJ, f, jl, p);
    jld = Fp_div(Fp_mul(Fp_neg(fd, p), Dgs, p), Fp_mulu(Djs, q, p), p);
    E6l = Fp_div(Fp_mul(Fp_neg(E4l, p), jld, p), jl, p);
    p_1 = Fp_mul(Fp_mulu(E2s, q, p), shifti(p, -1), p);
  }
  a4tilde = Fp_mul(Fp_mul(stoi(-3), powuu(q,4), p), E4l, p);
  a6tilde = Fp_mul(Fp_mul(stoi(-2), powuu(q,6), p), E6l, p);
  return gerepilecopy(ltop, mkvec3(a4tilde, a6tilde, p_1));
}

static GEN
find_kernel_power(GEN Eba4, GEN Eba6, GEN Eca4, GEN Eca6, ulong ell, GEN meqn, char meqntype, GEN mpoly, GEN kpoly, GEN Ib, GEN p)
{
  pari_sp ltop = avma, btop;
  GEN a4t, a6t, p1c, gtmp;
  GEN num_iso = find_numerator_isogeny(Eba4, Eba6, Eca4, Eca6, kpoly, p, ell+1);
  GEN mroots = FpX_roots(mpoly, p);
  long i, check, vx = 0, l1 = lg(mroots);
  btop = avma;
  for (i = 1; i < l1; i++)
  {
    if (meqntype=='C')
    {
      GEN tmp = find_isogenous_from_canonical(Eca4, Eca6, ell, meqn, gel(mroots, i), p);
      a4t = gel(tmp, 1);
      a6t = gel(tmp, 2);
      p1c = gel(tmp, 3);
      gtmp = find_kernel(Eca4, Eca6, ell, a4t, a6t, p1c, p, &check);
    }
    else
    {
      GEN tmp = find_isogenous_from_Atkin(Eca4, Eca6, ell, meqn, gel(mroots, i), p);
      if (!tmp) { avma=ltop; return NULL; }
      a4t =  gel(tmp, 1);
      a6t =  gel(tmp, 2);
      gtmp = gel(tmp, 3); check = 1;
    }
    /*check that the kernel kpoly is the good one */
    if (check)
    {    
      GEN kpoly2 = FpX_sqr(kpoly, p);
      GEN h = lift(numer(gsubst(gtmp, vx, gdiv(num_iso, kpoly2))));
      if (signe(elldivpol(Eba4, Eba6, ell, h, p)))
      {
        GEN Ic = gdiv(gsubst(num_iso, vx, Ib), gsqr(gsubst(kpoly, vx, Ib)));
        GEN kpoly_new = lift(numer(gsubst(gtmp, vx, Ic)));
        return gerepilecopy(ltop, mkvecn(5, a4t, a6t, kpoly_new, gtmp, Ic));
      }
    }
    avma = btop;
  }
  pari_err(talker, "failed to find kernel polynomial");
  return NULL; /*NOT REACHED*/
}

/****************************************************************************/
/*                                  TRACE                                   */
/****************************************************************************/
enum mod_type {MTpathological, MTAtkin, MTElkies, MTone_root, MTroots};

/* Berlekamp variant */
static GEN
study_modular_eqn(long q, GEN mpoly, GEN p, enum mod_type *mt, long *ptr_r)
{
  pari_sp ltop = avma;
  long r = 0;
  GEN g = gen_0;
  if (degpol(FpX_gcd(mpoly, FpX_deriv(mpoly,p), p)) > 0)
    *mt = MTpathological;
  else
  {
    GEN XP = FpXQ_pow(pol_x(0), p, mpoly, p);
    GEN G  = FpX_gcd(ZX_sub(XP, pol_x(0)), mpoly, p);
    long dG = degpol(G);
    if (!dG)
    {
      GEN L = FpXQ_matrix_pow(XP, q+1, q+1, mpoly, p);
      long s = q + 1 - FpM_rank(RgM_Rg_add(L, gen_m1), p);
      r = (q + 1)/s;
      *mt = MTAtkin;
    }
    else
    {
      g = FpX_oneroot(G, p);
      switch(dG)
      {
        case 1:  *mt = MTone_root; break;
        case 2:  *mt = MTElkies;   break;
        default: *mt = (dG == q + 1)? MTroots: MTpathological;
      }
    }
  }
  *ptr_r = r;
  if (DEBUGLEVEL) switch(*mt)
  {
    case MTone_root: fprintferr("One root\t"); break;
    case MTElkies: fprintferr("Elkies\t"); break;
    case MTroots: fprintferr("l+1 roots\t"); break;
    case MTAtkin: fprintferr("Atkin\t"); break;
    case MTpathological: fprintferr("Pathological\n"); break;
  }
  return gerepilecopy(ltop, g);
}

/*Returns the trace modulo ell^k when ell is an Elkies prime */
static GEN
find_trace_Elkies_power(GEN a4, GEN a6, ulong ell, long k, GEN meqn, char meqntype, GEN g, GEN tr, GEN p, long EARLY_ABORT)
{
  pari_sp ltop = avma, btop, st_lim;
  GEN Eba4, Eba6, Eca4, Eca6, Ib, p_1, kpoly;
  ulong lambda, ellk = upowuu(ell, k), pellk = umodiu(p, ellk);
  long cnt;
  if (DEBUGLEVEL) fprintferr("Compute trace mod %ld", ell);
  Eba4 = a4; Eba6 = a6;
  if (meqntype=='C')
  {
    GEN tmp = find_isogenous_from_canonical(Eba4, Eba6, ell, meqn, g, p);
    long check;
    Eca4 = gel(tmp, 1);
    Eca6 = gel(tmp, 2);
    p_1  = gel(tmp, 3);
    kpoly = find_kernel(a4, a6, ell, Eca4, Eca6, p_1, p, &check);
  }
  else
  {
    GEN tmp = find_isogenous_from_Atkin(a4, a6, ell, meqn, g, p);
    if (!tmp) { avma = ltop; return NULL; }
    Eca4 =  gel(tmp, 1);
    Eca6 =  gel(tmp, 2);
    kpoly = gel(tmp, 3);
  }
  Ib = pol_x(0);
  lambda = find_eigen_value(a4, a6, ell, kpoly, p, tr);
  if (EARLY_ABORT)
  {
    ulong pell = pellk%ell;
    ulong ap = Fl_add(lambda, Fl_div(pell, lambda, ell), ell);
    if (Fl_add(pell, ap, ell)==ell-1) { avma = ltop; return mkvecsmall(ap); }
  }
  btop = avma; st_lim = stack_lim(btop, 1);
  for (cnt = 2; cnt <= k; cnt++)
  {
    GEN tmp, mpoly;
    if (DEBUGLEVEL) fprintferr(", %Zs", powuu(ell, cnt));
    mpoly = FpXY_evalx(meqn, Fp_ell_j(Eca4, Eca6, p), p);
    tmp = find_kernel_power(Eba4, Eba6, Eca4, Eca6, ell, meqn, meqntype, mpoly, kpoly, Ib, p);
    if (!tmp) { avma = ltop; return NULL; }
    lambda = find_eigen_value_power(a4, a6, ell, cnt, gel(tmp,3), lambda, p);
    Eba4 = Eca4;
    Eba6 = Eca6;
    Eca4 = gel(tmp,1);
    Eca6 = gel(tmp,2);
    kpoly = gel(tmp,4);
    Ib = gel(tmp, 5);
    if (low_stack(st_lim, stack_lim(btop, 1)))
      gerepileall(btop, 6, &Eba4, &Eba6, &Eca4, &Eca6, &kpoly, &Ib);
  }
  avma = ltop;
  return mkvecsmall(Fl_add(lambda, Fl_div(pellk, lambda, ellk), ellk));
}

/*Returns the possible values of the trace when ell is an Atkin prime, */
/*given r the splitting degree of the modular equation at J = E.j */
static GEN
find_trace_Atkin(ulong ell, long r, GEN p)
{
  pari_sp ltop = avma;
  long nval = 0;
  ulong teta, pell = umodiu(p, ell), invp = Fl_inv(pell, ell);
  GEN val_pos = cgetg(1+ell, t_VECSMALL), P = gel(factoru(r), 1);
  GEN T = mkvecsmall4(0, pell, 0, 1);
  GEN U = mkvecsmall3(0, ell-1, 0);
  pari_sp btop = avma;
  for (teta = 0; teta < ell; teta++, avma = btop)
  {
    ulong disc = Fl_sub(Fl_sqr(teta,ell), Fl_mul(4UL,pell,ell), ell);
    GEN a;
    if (krouu(disc, ell) >= 0) continue;
    T[3] = Fl_neg(teta, ell);
    U[3] = Fl_mul(invp, teta, ell);
    a = Flxq_pow(U, utoi(r/P[1]), T, ell);
    if (!Flx_cmp1(a) && Flx_cmp1(Flxq_pow(a, utoi(P[1]), T, ell)))
    {
      pari_sp av = avma;
      long i, l=lg(P);
      for (i = 2; i < l; i++, avma = av)
        if (Flx_cmp1(Flxq_pow(U, utoi(r/P[i]), T, ell))) break;
      if (i==l) val_pos[++nval] = teta;
    }
  }
  return gerepileupto(ltop, vecsmall_shorten(val_pos, nval));
}

/*Returns the possible traces when there is only one root */
static GEN
find_trace_one_root(ulong ell, GEN p)
{
  ulong a = Fl_mul(Fl_sqrt(umodiu(p,ell), ell), 2, ell);
  return mkvecsmall2(a, ell - a);
}

static GEN
find_trace_lp1_roots(long ell, GEN p)
{
  ulong ell2 = ell * ell, pell = umodiu(p, ell2);
  ulong a  = Fl_sqrt(pell%ell, ell);
  ulong pa = Fl_add(Fl_div(pell, a, ell2), a, ell2);
  return mkvecsmall2(pa, ell2 - pa);
}

/*trace modulo ell^k: [], [t] or [t1,...,td] */
static GEN
find_trace(GEN a4, GEN a6, ulong ell, GEN p, long *ptr_kt, long EARLY_ABORT)
{
  pari_sp ltop = avma;
  GEN  g, meqn, meqnj, tr = NULL; /* needed by MTElkies */
  char meqntype;
  long k = 1, kt, r;
  enum mod_type mt;
  if (ell <= 13)
  {
    long lp = bit_accuracy(lg(p))-bfffo(*int_MSW(p));
    switch(ell)
    {
      case 3: k = 3 + (lp > 160) + (lp > 350); break;
      case 5: k = 2 + (lp > 260); break;
      case 7: k = 2 + (lp > 390); break;
      default:k = 1 + (lp > 260);
    }
  }
  kt = k;
  meqn = get_modular_eqn(ell, &meqntype);
  if (!meqn) return gen_0;
  if (DEBUGLEVEL) fprintferr("Process prime %ld.\tType: ", ell);
  meqnj = FpXY_evalx(meqn, Fp_ell_j(a4, a6, p), p);
  g = study_modular_eqn(ell, meqnj, p, &mt, &r);
  /* If l is an Elkies prime, search for a factor of the l-division polynomial.
  * Then deduce the trace by looking for eigenvalues of the Frobenius by
  * computing modulo this factor */
  switch (mt)
  {
  case MTone_root:
    tr = find_trace_one_root(ell, p);
    kt = k = 1;
    /* FIXME: why do we really always take k = 1 even if ell small ? */
    /* fall through */
  case MTElkies:
    tr = find_trace_Elkies_power(a4,a6,ell, k, meqn,meqntype, g, tr, p, EARLY_ABORT);
    if (!tr) { avma = ltop; return NULL; }
    break;
  case MTroots:
    tr = find_trace_lp1_roots(ell, p);
    kt = 2;
    break;
  case MTAtkin:
    tr = find_trace_Atkin(ell, r, p);
    kt = 1;
    break;
  default: /* case MTpathological: */
    avma = ltop; return NULL;
  }
  if (DEBUGLEVEL) {
    long n = lg(tr)-1;
    if (n > 1 || mt == MTAtkin) fprintferr("%3ld trace(s)",n);
    fprintferr("\n");
  }
  *ptr_kt = kt;
  return gerepileupto(ltop, tr);
}

/* A partition of compile_atkin in baby and giant is represented as the binary
   developpement of an integer; if the i-th bit is 1, the i-th prime in
   compile-atkin is a baby. The optimum is obtained when the ratio between
   the number of possibilities for traces modulo giants (p_g) and babies (p_b)
   is near 3/4. */
static long
separation(GEN cnt)
{
  pari_sp btop, st_lim;
  long k = lg(cnt)-1, l = (1L<<k)-1, best_i, i, j;
  GEN best_r, P, P3, r;
 
  P = gen_1;
  for (j = 1; j <= k; ++j) P = mulis(P, cnt[j]);
  /* p_b * p_g = P is constant */
  btop = avma; st_lim = stack_lim(btop, 1);
  best_i = 0;
  best_r = P3 = mulsi(3, P);
  for (i = 1; i < l; i++)
  {
    /* scan all possibilities */
    GEN p_b = gen_1;
    for (j = 0; j < k; j++)
      if (i & (1L<<j)) p_b = mulis(p_b, cnt[1+j]);
    r = subii(shifti(sqri(p_b), 2), P3); /* (p_b/p_g - 3/4)*4*P */
    if (!signe(r)) { best_i = i; break; }
    if (absi_cmp(r, best_r) < 0) { best_i = i; best_r = r; }
    if (low_stack(st_lim, stack_lim(btop, 1)))
      best_r = gerepileuptoint(btop, best_r);
  }
  return best_i;
}

/* chinese(Mod(a,A), Mod(b,B)), A,B coprime */
static GEN
crt(GEN A, GEN a, GEN B, GEN b)
{
  GEN v = cgetg(3, t_VEC), AB = mulii(A, B);
  gel(v,1) = AB;
  gel(v,2) = Z_chinese_coprime(a,b, A,B, AB);
  return v;
}

/* x VEC defined modulo P (= *P), y VECSMALL modulo q, (q,P) = 1 */
/* return the vector mod q P congruent to x (resp. y) mod P (resp. q). */
/* update P ( <-- qP ) */
static GEN
multiple_crt(GEN x, GEN y, GEN q, GEN *P)
{
  pari_sp ltop = avma;
  long i, j, k, lx = lg(x)-1, ly = lg(y)-1;
  GEN  t, a1, a2, u, v;
  (void)bezout(*P,q,&u,&v);
  a1 = mulii(*P,u);
  a2 = mulii(q,v);
  *P = mulii(*P, q);
  t = cgetg(lx*ly+1, t_VEC);
  for (i = 1, k = 1; i <= lx; i++)
  {
    GEN a2x = mulii(a2, gel(x,i));
    for (j = 1; j <= ly; ++j)
    {
      pari_sp av = avma;
      gel(t, k++) = gerepileupto(av, Fp_add(Fp_mulu(a1, y[j], *P), a2x, *P));
    }
  }
  gerepileall(ltop, 2, P, &t); return t;
}

/****************************************************************************/
/*                              MATCH AND SORT                              */
/****************************************************************************/

static GEN
possible_traces(GEN C, GEN *P)
{
  pari_sp ltop = avma, st_lim = stack_lim(ltop, 1);
  GEN v = vecsmall_to_vec(gmael(C,1,2));
  long i;
  *P = gmael(C,1,1);
  for (i = 2; i < lg(C); i++)
  {
    v = multiple_crt(v, gmael(C,i,2), gmael(C,i,1), P);
    if (low_stack(st_lim, stack_lim(ltop, 1))) gerepileall(ltop, 2, P, &v);
  }
  gerepileall(ltop, 2, P, &v);
  return v;
}

/* Finds a random point on E */
static GEN
find_pt_aff(GEN a4, GEN a6, GEN p)
{
  pari_sp ltop = avma;
  GEN x, y, rhs;
  do
  {
    avma= ltop;
    x   = randomi(p); /*  x^3+a4*x+a6 = x*(x^2+a4)+a6  */
    rhs = Fp_add(Fp_mul(x, Fp_add(Fp_sqr(x,p), a4, p), p), a6, p);
  } while (kronecker(rhs, p) < 0);
  y = Fp_sqrt(rhs, p);
  if (!y) pari_err(talker,"not a prime number");
  return gerepilecopy(ltop, mkvec2(x,y));
}

/* E has order o[1], ..., or o[#o], draw random points until all solutions
 * but one are eliminated */
static GEN
choose_card(GEN o, GEN a4, GEN a6, GEN p)
{
  pari_sp ltop = avma, btop;
  GEN lastgood, so, vo;
  long lo = lg(o), nbo=lo-1;
  if (nbo == 1) return icopy(gel(o,1));
  so = ZV_indexsort(o); /* minimize max( o[i+1] - o[i] ) */
  vo = const_vecsmall(lo, 0);
  lastgood = gel(o, so[nbo]);
  btop = avma;
  for(;;)
  {
    GEN lasto = gen_0;
    GEN P = find_pt_aff(a4, a6, p), t = mkvec(gen_0);
    long i;
    for (i = 1; i < lo; i++)
    {
      GEN newo = gel(o, so[i]);
      if (vo[i]) continue;
      t = Fp_ell_add(t, Fp_ell_pow(P, subii(newo,lasto), a4,p), a4,p);/*P^o[i]*/
      lasto = newo;
      if (lg(t) != 2)
      {
        if (--nbo == 1) { avma=ltop; return icopy(lastgood); }
        vo[i] = 1;
      }
      else
        lastgood = lasto;
    }
    avma = btop;
  }
}

static GEN
cost(long mask, GEN cost_vec)
{
  pari_sp ltop = avma;
  long i;
  GEN c = gen_1;
  for (i = 1; i < lg(cost_vec); i++)
    if (mask&(1L<<(i-1)))
      c = mulis(c, cost_vec[i]);
  return gerepileuptoint(ltop, c);
}

static GEN
value(long mask, GEN atkin, long k)
{
  pari_sp ltop = avma;
  long i;
  GEN c = gen_1;
  for (i = 1; i <= k; i++)
    if (mask&(1L<<(i-1)))
      c = mulii(c, gmael(atkin, i, 1));
  return gerepileuptoint(ltop, c);
}

static void
set_cost(GEN B, long b, GEN cost_vec, long *pi)
{
  pari_sp av = avma;
  GEN costb = cost(b, cost_vec);
  long i = *pi;
  while (cmpii(costb, cost(B[i], cost_vec)) < 0) --i;
  B[++i] = b;
  *pi = i; avma = av;
}

static GEN
prod_lgatkin(GEN compile_atkin, long k)
{
  pari_sp av = avma;
  long i;
  GEN p = gen_1;
  for (i = 1; i <= k; i++) p = mulis(p, lg(gmael(compile_atkin, i, 2)) - 1);
  return gerepileuptoint(av, p);
}

static GEN
get_lgatkin(GEN compile_atkin, long k)
{
  GEN v = cgetg(k+1, t_VECSMALL);
  long j;
  for (j = 1; j <= k; ++j) v[j] = lg(gmael(compile_atkin, j, 2))-1;
  return v;
}

static GEN
champion(GEN atkin, long k, ulong ell)
{
  const long two_k = 1L<<k;
  pari_sp ltop = avma, btop, st_lim;
  long i, j, n, i1, i2;
  GEN B, Bp, cost_vec, res;
  cost_vec = get_lgatkin(atkin, k);
  B  = const_vecsmall(two_k, 0);
  Bp = const_vecsmall(two_k, 0);
  Bp[2] = 1;
  btop = avma; st_lim = stack_lim(btop, 1);
  for (n = 2, j = 2; j <= k; j++)
  {
    long b;
    i = 1;
    for (i1 = 2, i2 = 1; i1 <= n; )
    {
      pari_sp av = avma;
      long b1 = Bp[i1], b2 = Bp[i2]|(1L<<(j-1));
      if (cmpii(value(b1, atkin, k), value(b2, atkin, k)) < 0)
        { b = b1; i1++; } else { b = b2; i2++; }
      avma = av;
      set_cost(B, b, cost_vec, &i);
    }
    for ( ; i2 <= n; i2++)
    {
      b = Bp[i2]|(1L<<(j-1));
      set_cost(B, b, cost_vec, &i);
    }
    n = i;
    for (i = 1; i <= n; i++)
      Bp[i] = B[i];
  }
  res = cgetg(n, t_VEC);
  for (i = 1, n = 1; i <= two_k; i++)
    if (B[i])
    {
      GEN b = cost (B[i], cost_vec);
      GEN v = value(B[i], atkin, k);
      gel(res, n++) = mkvec3(utoi(B[i]), b, v);
    }
  return gerepilecopy(ltop, res);
}

static GEN
compute_diff(GEN v)
{
  pari_sp av = avma;
  long i, l = lg(v) - 1;
  GEN diff = cgetg(l, t_VEC);
  for (i = 1; i < l; i++) gel(diff, i) = subii(gel(v, i+1), gel(v, i));
  return gerepileupto(av, ZV_sort_uniq(diff));
}

static int 
cmp_atkin(void*E, GEN a, GEN b)
{
  long ta=typ(a)==t_INT, tb=typ(b)==t_INT, c;
  if (ta || tb) return ta-tb;
  c = lg(gel(a,2)) - lg(gel(b,2));
  if (c) return c;
  return cmpii(gel(b,1), gel(a,1));
}

static void 
add_atkin(GEN atkin, GEN trace, long *nb)
{
  long l = lg(atkin)-1;
  long i, k = gen_search(atkin, trace, 1, NULL, cmp_atkin);
  if (k==0 || k > l) return;
  for (i = l; i > k; i--)
    gel(atkin,i) = gel(atkin,i-1);
  if (typ(gel(atkin,l))==t_INT) (*nb)++;
  gel(atkin,k) = trace;
}

/* V = baby / giant, P = Pb / Pg */
static GEN
BSGS_pre(GEN *pdiff, GEN V, GEN P, GEN a4, GEN p)
{
  GEN diff = compute_diff(V);
  GEN pre = cgetg(lg(diff), t_VEC);
  long i, l = lg(diff);
  gel(pre, 1) = Fp_ell_pow(P, gel(diff, 1), a4, p);
  /* what we'd _really_ want here is a hashtable diff[i] -> pre[i]  */
  for (i = 2; i < l; i++)
  { 
    pari_sp av = avma;
    GEN d = subii(gel(diff, i), gel(diff, i-1));
    GEN Q = Fp_ell_add(gel(pre, i-1), Fp_ell_pow(P, d, a4, p), a4, p);
    gel(pre, i) = gerepileupto(av, Q);
  }
  *pdiff = diff; return pre;
}

/* u = trace_elkies, Mu = prod_elkies */
static GEN
match_and_sort(GEN compile_atkin, long k, GEN Mu, GEN u, GEN a4, GEN a6, GEN p)
{
  pari_sp ltop = avma, av1, av2;
  GEN baby, giant, SgMb, Mb, Mg, den, Sg, dec_inf, div, pp1 = addis(p,1);
  GEN P, Pb, Pg, point, diff, pre, d, table, card, table_ind;
  long r=0, s=0, best_i, i;

  if (!k)
  { /*no Atkin prime: Mu >= 4*sqrt(p). */
    card = subii(addis(p,1), u);
    card = mkvec2(card, addii(card, Mu));
    return gerepileuptoint(ltop, choose_card(card, a4, a6, p));
  }
  if (k == 1)
  {
    /*only one Atkin prime, check the cardinality with random points */
    GEN r = gel(compile_atkin, 1), r1 = gel(r,1), r2 = gel(r,2);
    long l = lg(r2);
    GEN card = cgetg(l, t_VEC), s = gmodulo(u, Mu);
    for (i = 1; i < l; i++)
      gel(card, i) = subii(pp1, centerlift(chinese(s, gmodulsg(r2[i], r1))));
    return gerepileuptoint(ltop, choose_card(card, a4, a6, p));
  }
  av1 = avma;
  best_i = separation( get_lgatkin(compile_atkin, k) );
  avma = av1;
  baby  = shallowextract(compile_atkin, stoi(best_i));
  baby  = possible_traces(baby, &Mb);
  av2 = avma;
  giant = shallowextract(compile_atkin, subis(shifti(gen_1, k), best_i+1));
  giant = possible_traces(giant, &Mg);
  den = Fp_inv(Fp_mul(Mu, Mb, Mg), Mg);
  Sg = Fp_mul(negi(u), den, Mg);
  for (i = 1; i < lg(giant); i++)
    gel(giant,i) = Fp_mul(gel(giant,i), den, Mg);
  giant = ZV_sort(giant);
  gerepileall(av2, 3, &giant, &Mg, &Sg);
  /* the variant from Lercier's thesis, section 11.2.3 */
  den = Fp_inv(Fp_mul(Mu, Mg, Mb), Mb);
  for (i = 1; i < lg(baby); i++)
    gel(baby,i) = Fp_mul(subii(gel(baby,i), u), den, Mb);
  SgMb = mulii(Sg, Mb);
  dec_inf = gceil(gsub(gdivgs(Mb, -2), gdiv(SgMb, Mg)));
  div = mulii(truedivii(dec_inf, Mb), Mb);
  for (i = 1; i < lg(baby); i++)
  {
    GEN b = addii(gel(baby,i), div);
    if (cmpii(b, dec_inf) < 0) b = addii(b, Mb);
    gel(baby,i) = b;
  }
  baby = ZV_sort(baby);
  gerepileall(av2, 5, &baby, &Mb, &giant, &Mg, &SgMb);
  P = find_pt_aff(a4, a6, p);
  point = Fp_ell_pow(P, Mu, a4, p);
  Pb = Fp_ell_pow(point, Mg, a4, p);
  Pg = Fp_ell_pow(point, Mb, a4, p);
  /* Precomputation for babies */
  pre = BSGS_pre(&diff, baby, Pb, a4, p);

  /*Now we compute the table of babies, this table contains only the */
  /*lifted x-coordinate of the points in order to use less memory */
  table = cgetg(lg(baby), t_VEC);
  /* (p+1 - u - Mu*Mb*Sg) P - (baby[1]) Pb */
  point = Fp_ell_pow(P, subii(subii(pp1, u), mulii(Mu, addii(SgMb, mulii(Mg, gel(baby,1))))), a4, p);
  gel(table, 1) = gel(point, 1);
  for (i = 2; i < lg(baby); i++)
  {
    av1 = avma;
    d = subii(gel(baby, i), gel(baby, i-1));
    point = Fp_ell_sub(point, gel(pre, ZV_search(diff, d)), a4, p);
    point = gerepileupto(av1, point);
    gel(table, i) = gel(point, 1);
  }
  /* Precomputations for giants */
  pre = BSGS_pre(&diff, giant, Pg, a4, p);

  /* Look for a collision among the x-coordinates */
  table_ind = indexsort(table);
  table = shallowextract(table, table_ind);
  av1 = avma;
  point = Fp_ell_pow(Pg, gel(giant, 1), a4, p);
  for (i = 1; i < lg(giant) - 1; i++)
  {
    s = ZV_search(table, gel(point, 1));
    if (s) { s = table_ind[s]; r = i; break; }

    d = subii(gel(giant, i+1), gel(giant, i));
    point = gerepileupto(av1, Fp_ell_add(point, gel(pre, ZV_search(diff, d)), a4, p));
  }
  /* no match ? */
  if (i==lg(giant)-1) pari_err(bugparier,"match_and_sort");

  /* p+1 - u - Mu (Sg Mb + GIANT Mb + BABY Mg) */
{
  GEN GMb = mulii(gel(giant,r), Mb), BMg = mulii(gel(baby,s), Mg);
  card = subii(subii(pp1, u), mulii(Mu, addii(SgMb, addii(GMb, BMg))));
  card = mkvec2(card, addii(card, mulii(mulsi(2,Mu), GMb)));}
  return gerepileuptoint(ltop, choose_card(card, a4, a6, p));
}

/* E is an elliptic curve defined over Z or over Fp in ellinit format, defined
* by the equation E: y^2 + a1*x*y + a2*y = x^3 + a2*x^2 + a4*x + a6
* p is a prime number
* set EARLY_ABORT to stop whenever a small factor of the order is detected.
*   Useful when searching for a good curve for cryptographic applications */
GEN
ellsea(GEN E, GEN p, long EARLY_ABORT)
{
  const long MAX_ATKIN = 20;
  pari_sp ltop = avma, btop, st_lim;
  long i, nb_atkin, lp, M, get_extra_l;
  GEN compile_atkin;
  GEN tr, bound, product, trace_mod, bound_bsgs, growth_factor, best_champ;
  GEN res;
  GEN a4 = modii(mulis(Rg_to_Fp(gel(E,10), p), -27), p);
  GEN a6 = modii(mulis(Rg_to_Fp(gel(E,11), p), -54), p);
  long ell = 2;
  byteptr primepointer = diffptr + 1;

  if (!modular_eqn && !get_seadata()) return NULL;
  /*First compute the trace modulo 2 */
  if (FpX_nbroots(mkpoln(4, gen_1, gen_0, a4, a6), p) > 0)
  {
    if (EARLY_ABORT)
    {
      if (DEBUGLEVEL)
        fprintferr("Aborting: the number of points is divisible by 2\n");
      avma = ltop; return gen_0;
    }
    tr = mkvec2(gen_2, gen_0);
  }
  else
    tr = mkvec2(gen_2, gen_1);
  /* 'product' is the product of the primes we use, the computation 
   *  stops when product > 4*sqrt(p)
   * l is the current prime,
   * compile_atkin is a vector containing informations about Atkin primes,
   *   informations about Elkies primes lie in tr. */
  bound = sqrti(shifti(p, 4));
  M = 1000000;
  lp = bit_accuracy(lg(p)) - bfffo(*int_MSW(p));
  if (lp <= 160)
    bound_bsgs = mulrs(divrs(gpowgs(dbltor(1.048), lp), 9), M);
  else if (lp <= 192)
    bound_bsgs = mulrs(divrr(gpowgs(dbltor(1.052), lp), dbltor(16.65)), M);
  else if (lp <= 306)
    bound_bsgs = mulrs(mulrr(gpowgs(dbltor(1.035), lp), dbltor(1.35)), M);
  else bound_bsgs = mulss(50000, M);
  growth_factor = dbltor(1.26);
  product = gen_2;
  compile_atkin = zerovec(MAX_ATKIN); nb_atkin = 0;
  btop = avma; st_lim = stack_lim(btop, 1);
  while (gcmp(product, bound) <= 0)
  {
    GEN ellkt;
    long kt;
    NEXT_PRIME_VIADIFF(ell, primepointer);
    trace_mod = find_trace(a4, a6, ell, p, &kt, EARLY_ABORT);
    if (trace_mod==gen_0)
    {
      pari_warn(warner,"no more modular polynomials available!");
      if (DEBUGLEVEL)
      {
        GEN p15 = prod_lgatkin(compile_atkin, nb_atkin);
        pari_warn(warner,"no more modular polynomials available, match and sort may be very long: it remains %Zs possibilities for the trace", p15);
      }
      res = match_and_sort(compile_atkin, nb_atkin, gel(tr, 1), gel(tr, 2), a4, a6, p);
      return gerepileuptoint(ltop, subii(addis(p,1), res));
    }
    if (!trace_mod) continue;
    ellkt = powuu(ell, kt);
    if (lg(trace_mod) == 2)
    {
      if (EARLY_ABORT && dvdiu(addis(p, 1 - trace_mod[1]), ell))
      {
        if (DEBUGLEVEL) fprintferr("\nAborting: #E(Fp) divisible by %Zs\n",ell);
        avma = ltop; return gen_0;
      }
      tr = crt(ellkt, stoi(trace_mod[1]), gel(tr,1), gel(tr,2));
    }
    else
      /* compute possible values for the trace using Atkin method. */
      add_atkin(compile_atkin, mkvec2(ellkt, trace_mod), &nb_atkin);
    /*increase the product with this prime and go to the next prime */
    product = mulii(product, ellkt);
    if (low_stack(st_lim, stack_lim(btop, 1)))
      gerepileall(btop, 3, &tr, &compile_atkin, &product);
  }
  best_champ = mkvec2(value(-1, compile_atkin, nb_atkin),
                      prod_lgatkin(compile_atkin, nb_atkin));
  /*If the number of possible traces is too large, we treat a new prime */
  if (DEBUGLEVEL && gcmp(gel(best_champ, 2), bound_bsgs) >= 0)
    fprintferr("Too many possibilities for the trace: %Zs. Look for new primes\n", gel(best_champ, 2));
  btop = avma; st_lim = stack_lim(btop, 1);
  get_extra_l = 1;
  bound_bsgs = gdiv(bound_bsgs, growth_factor);
  while (1)
  {
    long kt;
    GEN ellkt;
    GEN cat, bound_champ, champ;
    NEXT_PRIME_VIADIFF(ell, primepointer);
    bound_bsgs = gmul(bound_bsgs, growth_factor);
    if (gcmp(gel(best_champ, 2), bound_bsgs) < 0) break;
    trace_mod = find_trace(a4, a6, ell, p , &kt, EARLY_ABORT);
    if (trace_mod==gen_0)
    {
      if (DEBUGLEVEL && gcmp(gel(best_champ, 2), bound_bsgs) > 0)
        pari_warn(warner,"no more modular polynomials available, match and sort may be very long: it remains %Zs possibilities for the trace", gel(best_champ, 2));
      break;
    }
    if (!trace_mod) continue;
    ellkt = powuu(ell, kt);
    if (lg(trace_mod) == 2)
      tr = crt(ellkt, stoi(trace_mod[1]), gel(tr, 1), gel(tr, 2));
    else
      add_atkin(compile_atkin, mkvec2(ellkt, trace_mod), &nb_atkin);
    /*Let us now treat an other prime if we are too far from the bound_bsgs */
    if (get_extra_l
        && gcmpgs(gdiv(gel(best_champ, 2), bound_bsgs), 5) >= 0
        && (lg(trace_mod) > 3 || gcmp(gdiv(gel(best_champ, 2), bound_bsgs), 
            gdivgs(sqrs(ell), 25)) >= 0))
    {
      get_extra_l = 0;
      continue;
    }
    get_extra_l = 1;
    bound_champ = gdiv(bound, gel(tr, 2));
    champ = champion(compile_atkin, nb_atkin, ell);
    best_champ = gel(champ, lg(champ) - 1);
    for (i = 1; i < lg(champ); i++)
    {
      GEN C = gel(champ,i);
      if (gcmp(gel(C,3), bound_champ) > 0 
          && gcmp(gel(C,2), gel(best_champ, 2)) < 0) best_champ = C;
    }
    /*best_champ is the champion of lowest cost among champions less than the */
    /*required bound. We now recover the corresponding sets of traces. */
    cat = shallowextract(compile_atkin, gel(best_champ, 1));
    for (i=1; i<lg(cat);   i++) gel(compile_atkin, i) = gel(cat, i);
    for (   ; i<=nb_atkin; i++) gel(compile_atkin, i) = gen_0;
    nb_atkin = lg(cat)-1;
    if (DEBUGLEVEL>=2)
      fprintferr("Keeping %ld primes, %Zs remaining possibilities.\n",
                 nb_atkin, gel(best_champ, 2));
    if (low_stack(st_lim, stack_lim(btop, 1)))
      gerepileall(btop, 3, &tr, &compile_atkin, &bound_bsgs, &best_champ);
  }
  if (DEBUGLEVEL)
    fprintferr("\nComputation of traces done. Entering match-and-sort algorithm.\nIt remains %Zs possibilities for the trace.\n", gel(best_champ, 2));
  res = match_and_sort(compile_atkin, nb_atkin, gel(tr,1), gel(tr,2), a4,a6,p);
  return gerepileuptoint(ltop, subii(addis(p, 1), res));
}
