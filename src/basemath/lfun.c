/* Copyright (C) 2015  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

/********************************************************************/
/**                                                                **/
/**                       L-functions                              **/
/**                                                                **/
/********************************************************************/

#include "pari.h"
#include "paripriv.h"

/*******************************************************************/
/*  Accessors                                                      */
/*******************************************************************/

static GEN
mysercoeff(GEN x, long n)
{
  long N = n - valp(x);
  return (N < 0)? gen_0: gel(x, N+2);
}

long
ldata_get_type(GEN ldata) { return mael3(ldata, 1, 1, 1); }

GEN
ldata_get_an(GEN ldata) { return gel(ldata, 1); }

long
ldata_get_selfdual(GEN ldata) { return itos(gel(ldata, 2)); }

long
ldata_isreal(GEN ldata) {return gequal0(gel(ldata, 2));}

GEN
ldata_get_gammavec(GEN ldata) { return gel(ldata, 3); }

long
ldata_get_degree(GEN ldata) { return lg(gel(ldata, 3))-1; }

long
ldata_get_k(GEN ldata) { return itos(gel(ldata, 4)); }

GEN
ldata_get_conductor(GEN ldata) { return gel(ldata, 5); }

GEN
ldata_get_rootno(GEN ldata) { return gel(ldata, 6); }

GEN
ldata_get_residue(GEN ldata) { return lg(ldata) == 7 ? NULL: gel(ldata, 7); }

long
linit_get_type(GEN linit) { return mael(linit, 1, 1); }

GEN
linit_get_ldata(GEN linit) { return gel(linit, 2); }

GEN
linit_get_tech(GEN linit) { return gel(linit, 3); }

long
is_linit(GEN data)
{
  return lg(data) == 4 && typ(data) == t_VEC
                       && typ(gel(data, 1)) == t_VECSMALL;
}

GEN
lfun_get_step(GEN tech) { return gmael(tech, 2, 1);}

GEN
lfun_get_pol(GEN tech) { return gmael(tech, 2, 2);}

GEN
lfun_get_Residue(GEN tech) { return gmael(tech, 2, 3);}

GEN
lfun_get_k2(GEN tech) { return gmael(tech, 3, 1);}

GEN
lfun_get_w2(GEN tech) { return gmael(tech, 3, 2);}

GEN
lfun_get_expot(GEN tech) { return gmael(tech, 3, 3);}

GEN
lfun_get_factgammavec(GEN tech) { return gmael(tech, 3, 4); }

static GEN
domain_get_dom(GEN domain)  { return gel(domain,1); }
static long
domain_get_der(GEN domain)  { return mael2(domain, 2, 1); }
static long
domain_get_bitprec(GEN domain)  { return mael2(domain, 2, 2); }
GEN
lfun_get_domain(GEN tech) { return gel(tech,1); }
long
lfun_get_bitprec(GEN tech){ return domain_get_bitprec(lfun_get_domain(tech)); }
GEN
lfun_get_dom(GEN tech) { return domain_get_dom(lfun_get_domain(tech)); }

GEN
lfunprod_get_fact(GEN tech)  { return gel(tech, 2); }

GEN
theta_get_an(GEN tdata)        { return gel(tdata, 1);}
GEN
theta_get_K(GEN tdata)         { return gel(tdata, 2);}
GEN
theta_get_R(GEN tdata)         { return gel(tdata, 3);}
long
theta_get_bitprec(GEN tdata)   { return itos(gel(tdata, 4));}
long
theta_get_m(GEN tdata)         { return itos(gel(tdata, 5));}
GEN
theta_get_tdom(GEN tdata)      { return gel(tdata, 6);}
GEN
theta_get_sqrtN(GEN tdata)     { return gel(tdata, 7);}

/*******************************************************************/
/*  Helper functions related to Gamma products                     */
/*******************************************************************/

/* return -itos(s) >= 0 if s is (approximately) equal to a non-positive
 * integer, and -1 otherwise */
static long
isnegint(GEN s)
{
  GEN r = ground(real_i(s));
  if (signe(r) <= 0 && gequal(s, r)) return -itos(r);
  return -1;
}

/* pi^(-s/2) Gamma(s/2) */
static GEN
gamma_R(GEN s, long prec)
{
  GEN s2 = gdivgs(s, 2), pi = mppi(prec);
  long ms = isnegint(s2);
  if (ms >= 0)
  {
    GEN pr = gmul(powru(pi, ms), gdivsg(odd(ms)? -2: 2, mpfact(ms)));
    GEN S = scalarser(pr, 0, 1);
    setvalp(S,-1); return S;
  }
  return gdiv(ggamma(s2,prec), gpow(pi,s2,prec));
}

/* gamma_R(s)gamma_R(s+1) = 2 (2pi)^(-s) Gamma(s) */
static GEN
gamma_C(GEN s, long prec)
{
  GEN pi2 = Pi2n(1,prec);
  long ms = isnegint(s);
  if (ms >= 0)
  {
    GEN pr = gmul(powrs(pi2, ms), gdivsg(odd(ms)? -2: 2, mpfact(ms)));
    GEN S = scalarser(pr, 0, 1);
    setvalp(S,-1); return S;
  }
  return gmul2n(gdiv(ggamma(s,prec), gpow(pi2,s,prec)), 1);
}

static GEN
gammafrac(GEN r, long d)
{
  GEN pr, a = gmul2n(r, -1);
  GEN polj = cgetg(labs(d)+1, t_COL);
  long i, v=0;
  if (d > 0)
    for (i = 1; i <= d; ++i)
      gel(polj, i) = deg1pol_shallow(ghalf, gaddgs(a, i-1), v);
  else
    for (i = 1; i <= -d; ++i)
      gel(polj, i) = deg1pol_shallow(ghalf, gsubgs(a, i), v);
  pr = RgV_prod(polj);
  return d < 0 ? ginv(pr): pr;
}

static GEN
gammafactor(GEN Vga)
{
  pari_sp av = avma;
  long i, m, d = lg(Vga)-1, dr, dc;
  GEN pol = pol_1(0), pi = gen_0, R = cgetg(d+1,t_VEC);
  GEN P, F, FR, FC, E, ER, EC;
  for (i = 1; i <= d; ++i)
  {
    GEN a = gel(Vga,i), qr = gdiventres(real_i(a), gen_2);
    long q = itos(gel(qr,1));
    gel(R, i) = gadd(gel(qr,2), imag_i(a));
    if (q)
    {
      pol = gmul(pol, gammafrac(gel(R,i), q));
      pi  = addis(pi, q);
    }
  }
  gen_sort_inplace(R, (void*)cmp_universal, cmp_nodata, &P);
  F = cgetg(d+1, t_VEC); E = cgetg(d+1, t_VECSMALL);
  for (i = 1, m = 0; i <= d;)
  {
    long k;
    GEN u = gel(R, i);
    for(k = i + 1; k <= d; ++k)
      if (cmp_universal(gel(R, k), u)) break;
    m++;
    E[m] = k - i;
    gel(F, m) = u;
    i = k;
  }
  setlg(F, m+1); setlg(E, m+1);
  R = cgetg(m+1, t_VEC);
  for (i = 1; i <= m; i++)
  {
    GEN qr = gdiventres(gel(F,i), gen_1);
    gel(R, i) = mkvec2(gel(qr,2), stoi(E[i]));
  }
  gen_sort_inplace(R, (void*)cmp_universal, cmp_nodata, &P);
  FR = cgetg(m+1, t_VEC); ER = cgetg(m+1, t_VECSMALL);
  FC = cgetg(m+1, t_VEC); EC = cgetg(m+1, t_VECSMALL);
  for (i = 1, dr = 1, dc = 1; i <= m;)
  {
    if (i==m || cmp_universal(gel(R,i), gel(R,i+1)))
    {
      gel(FR, dr) = gel(F, P[i]);
      ER[dr] = E[P[i]];
      dr++; i++;
    } else
    {
      if (gequal(gaddgs(gmael(R,i,1), 1), gmael(R,i+1,1)))
        gel(FC, dc) = gel(F, P[i+1]);
      else
        gel(FC, dc) = gel(F, P[i]);
      EC[dc] = E[P[i]];
      dc++; i+=2;
    }
  }
  setlg(FR, dr); setlg(ER, dr);
  setlg(FC, dc); setlg(EC, dc);
  return gerepilecopy(av, mkvec4(pol, pi, mkvec2(FR,ER), mkvec2(FC,EC)));
}

static GEN
deg1ser_shallow(GEN a1, GEN a0, long v, long e)
{
  return RgX_to_ser(deg1pol_shallow(a1, a0, v), e+2);
}
/*
To test:
GR(s)=Pi^-(s/2)*gamma(s/2);
GC(s)=2*(2*Pi)^-s*gamma(s)
gam_direct(F,s)=prod(i=1,#F,GR(s+F[i]))
gam_fact(F,s)=my([P,p,R,C]=gammafactor(F));subst(P,x,s)*Pi^-p*prod(i=1,#R[1],GR(s+R[1][i])^R[2][i])*prod(i=1,#C[1],GC(s+C[1][i])^C[2][i])
*/

static GEN
polgammaeval(GEN F, GEN s)
{
  GEN r = poleval(F, s);
  if (typ(s)!=t_SER && gequal0(r))
  {
    long e = gvaluation(F, deg1pol(gen_1, gneg(s), varn(F)));
    r = poleval(F, deg1ser_shallow(gen_1, s, 0, e+1));
  }
  return r;
}

static GEN
fracgammaeval(GEN F, GEN s)
{
  if (typ(F)==t_POL)
    return polgammaeval(F, s);
  else if (typ(F)==t_RFRAC)
    return gdiv(polgammaeval(gel(F,1), s), polgammaeval(gel(F,2), s));
  return F;
}

static GEN
gammafactproduct(GEN F, GEN s, long prec)
{
  pari_sp av = avma;
  GEN P = fracgammaeval(gel(F,1), s);
  GEN p = gpow(mppi(prec),gneg(gel(F,2)), prec), z = gmul(P, p);
  GEN R = gel(F,3), Rw = gel(R,1), Re=gel(R,2);
  GEN C = gel(F,4), Cw = gel(C,1), Ce=gel(C,2);
  long i, lR = lg(Rw), lC = lg(Cw);
  for (i=1; i< lR; i++)
    z = gmul(z, gpowgs(gamma_R(gadd(s,gel(Rw, i)), prec), Re[i]));
  for (i=1; i< lC; i++)
    z = gmul(z, gpowgs(gamma_C(gadd(s,gel(Cw, i)), prec), Ce[i]));
  return gerepileupto(av, z);
}

static int
gammaordinary(GEN Vga, GEN s)
{
  long i, d = lg(Vga)-1;
  for (i = 1; i <= d; i++)
  {
    GEN z = gadd(s, gel(Vga,i));
    long e;
    if (gsigne(z) <= 0) { (void)grndtoi(z, &e); if (e < -10) return 0; }
  }
  return 1;
}

/* Exponent A of t in asymptotic expansion; K(t) ~ C t^A exp(-pi d t^(2/d)).
 * suma = vecsum(Vga)*/
static double
gammavec_expo(long d, double suma) { return (1 - d + suma) / d; }

/*******************************************************************/
/*       First part: computations only involving Theta(t)          */
/*******************************************************************/

static void
get_cone(GEN t, double *r, double *a)
{
  const long prec = LOWDEFAULTPREC;
  if (typ(t) == t_COMPLEX)
  {
    t  = gprec_w(t, prec);
    *r = gtodouble(gabs(t, prec));
    *a = fabs(gtodouble(garg(t, prec)));
  }
  else
  {
    *r = fabs(gtodouble(t));
    *a = 0.;
  }
  if (!*r && !*a) pari_err_DOMAIN("lfunthetainit","t","=",gen_0,t);
}
/* slightly larger cone than necessary, to avoid round-off problems */
static void
get_cone_fuzz(GEN t, double *r, double *a)
{ get_cone(t, r, a); *r -= 1e-10; if (*a) *a += 1e-10; }

/* Initialization m-th Theta derivative. tdom is either
 * - [rho,alpha]: assume |t| >= rho and |arg(t)| <= alpha
 * - a positive real scalar: assume t real, t >= tdom;
 * - a complex number t: compute at t;
 * N is the conductor (either the true one from ldata or a guess from
 * lfunconductor) */
static long
lfunthetaneed_bitprec(GEN ldata, GEN tdom, long m, long bitprec)
{
  pari_sp av = avma;
  GEN Vga = ldata_get_gammavec(ldata);
  long d = lg(Vga)-1;
  long k = ldata_get_k(ldata), k1;
  double c = d/2., a, A, B, logC, al, rho;
  double N = gtodouble(ldata_get_conductor(ldata));

  if (!N) pari_err_TYPE("lfunthetaneed [missing conductor]", ldata);
  if (typ(tdom) == t_VEC && lg(tdom) == 3)
  {
    rho= gtodouble(gel(tdom,1));
    al = gtodouble(gel(tdom,2));
  }
  else
    get_cone_fuzz(tdom, &rho, &al);
  k1 = ldata_get_residue(ldata)? k-1: (k-1)/2.;
  A = gammavec_expo(d, gtodouble(vecsum(Vga))); avma = av;
  a = (A+k1+1) + (m-1)/c;
  if (fabs(a) < 1e-10) a = 0.;
  logC = c*LOG2 - log(c)/2;
  /* +1: fudge factor */
  B = LOG2*bitprec+logC+m*log(2*M_PI) + 1 + (k1+1)*log(N)/2 - (k1+m+1)*log(rho);
  if (al)
  { /* t = rho e^(i*al), T^(1/c) = Re(t^(1/c)) > 0, T = rho cos^c(al/c) */
    double z = cos(al/c), T = rho*pow(z,c);
    if (z <= 0)
      pari_err_DOMAIN("lfunthetaneed", "arg t", ">", dbltor(c*M_PI/2), tdom);
    B -= log(z) * (c * (k1+A+1) + m);
    B = dbllemma526(a, M_PI*d*z, c, B) / T;
  }
  else
    B = dblcoro526(a,c,B) / rho;
  return ceil(B * sqrt(N));
}

static long
fracgammadegree(GEN F)
{ return (typ(F)==t_RFRAC)? degpol(gel(F,2)): 0; }

/* Poles of a L-function can be represented in the following ways:
 * 1) Nothing (ldata has only 6 components, ldata_get_residue = NULL).
 * 2) a complex number (single pole at s = k with given residue, unknown if 0).
 * 3) A vector (possibly empty) of 2-component vectors [a, ra], where a is the
 * pole, ra a t_SER: its Taylor expansion at a. */
static GEN
rtoR(GEN a, GEN r, GEN FVga, GEN N, long prec)
{
  GEN as = deg1ser_shallow(gen_1, a, varn(r), lg(r)-2);
  GEN Na = gpow(N, gdivgs(as, 2), prec);
  long d = fracgammadegree(gel(FVga,1));
  /* make up for a possible loss of accuracy */
  if (d) as = deg1ser_shallow(gen_1, a, varn(r), lg(r)-2+d);
  return gmul(gmul(r, Na), gammafactproduct(FVga, as, prec));
}

/* r / x + O(1) */
static GEN
simple_pole(GEN r)
{
  GEN S = deg1ser_shallow(gen_0, r, 0, 1);
  setvalp(S, -1); return S;
}

/* assume r in normalized form: t_VEC of pairs [be,re] */
GEN
lfunrtopoles(GEN r)
{
  long j, l = lg(r);
  GEN v = cgetg(l, t_VEC);
  for (j = 1; j < l; j++)
  {
    GEN rj = gel(r,j), a = gel(rj,1);
    gel(v,j) = a;

  }
  gen_sort_inplace(v, (void*)&cmp_universal, cmp_nodata, NULL);
  return v;
}

static GEN
normalizepoles(GEN r, long k)
{
  long tx = typ(r), iv, j, l;
  GEN v;
  if (tx != t_VEC)
  {
    if (!is_scalar_t(tx)) pari_err_TYPE("normalizepoles", r);
    return mkvec(mkvec2(stoi(k), simple_pole(r)));
  }
  v = cgetg_copy(r, &l);
  for (j = iv = 1; j < l; j++)
  {
    GEN rj = gel(r,j), a = gel(rj,1), ra = gel(rj,2);
    if (!is_scalar_t(typ(a)) || typ(ra) != t_SER)
      pari_err_TYPE("normalizepoles",r);
    if (valp(ra) >= 0) continue;
    gel(v,iv++) = rj;
  }
  setlg(v, iv); return v;
}

/* Compute R's from r's (r = Taylor devts of L(s), R of Lambda(s)).
 * 'r/eno' passed to override the one from ldata  */
static GEN
lfunrtoR_i(GEN ldata, GEN r, GEN eno, long prec)
{
  GEN Vga = ldata_get_gammavec(ldata);
  GEN N = ldata_get_conductor(ldata);
  pari_sp av = avma;
  GEN R, vr, FVga;
  long lr, j, jR, k = ldata_get_k(ldata);
  if (!r || isintzero(r) || isintzero(eno)) return gen_0;
  r = normalizepoles(r, k);
  vr = lfunrtopoles(r); lr = lg(vr);
  FVga = gammafactor(Vga);
  R = cgetg(2*lr, t_VEC);
  for (j = jR = 1; j < lr; j++)
  {
    GEN rj = gel(r,j), a = gel(rj,1), ra = gel(rj,2);
    GEN Ra = rtoR(a, ra, FVga, N, prec);
    GEN b = gsubsg(k, gconj(a));
    if (lg(Ra)-2 < -valp(Ra))
      pari_err(e_MISC,
        "please give more terms in L function's Taylor development at %Ps", a);
    gel(R,jR++) = mkvec2(a, Ra);
    if (!tablesearch(vr, b, (int (*)(GEN,GEN))&cmp_universal))
    {
      GEN mX = gneg(pol_x(varn(Ra)));
      GEN Rb = gmul(eno, gsubst(gconj(Ra), varn(Ra), mX));
      gel(R,jR++) = mkvec2(b, Rb);
    }
  }
  setlg(R, jR); return gerepilecopy(av, R);
}
static GEN
lfunrtoR_eno(GEN ldata, GEN eno, long prec)
{ return lfunrtoR_i(ldata, ldata_get_residue(ldata), eno, prec); }
static GEN
lfunrtoR(GEN ldata, long prec)
{ return lfunrtoR_eno(ldata, ldata_get_rootno(ldata), prec); }

/* thetainit using {an: n <= L} */
static GEN
lfunthetainit0_bitprec(GEN ldata, GEN tdom, GEN vecan, long m,
    long bitprec, long extrabit)
{
  long prec = nbits2prec(bitprec);
  GEN tech, N = ldata_get_conductor(ldata);
  GEN Vga = ldata_get_gammavec(ldata);
  GEN K = gammamellininvinit_bitprec(Vga, m, bitprec + extrabit);
  GEN R = lfunrtoR(ldata, prec);
  if (!tdom) tdom = gen_1;
  if (typ(tdom) != t_VEC)
  {
    double r, a;
    get_cone_fuzz(tdom, &r, &a);
    tdom = mkvec2(dbltor(r), a? dbltor(a): gen_0);
  }
  tech = mkvecn(7, vecan,K,R, stoi(bitprec), stoi(m), tdom, gsqrt(N,prec));
  return mkvec3(mkvecsmall(t_LDESC_THETA), ldata, tech);
}

/* tdom: 1) positive real number r, t real, t >= r; or
 *       2) [r,a], describing the cone |t| >= r, |arg(t)| <= a */
GEN
lfunthetainit_bitprec(GEN data, GEN tdom, long m, long bitprec)
{
  GEN ldata = lfunmisc_to_ldata_shallow(data);
  long L = lfunthetaneed_bitprec(ldata, tdom, m, bitprec);
  GEN vecan = ldata_vecan(ldata_get_an(ldata), L, nbits2prec(bitprec));
  return lfunthetainit0_bitprec(ldata, tdom, vecan, m, bitprec, 32);
}
GEN
lfunthetainit(GEN ldata, GEN tdom, long m, long prec)
{
  pari_sp av = avma;
  GEN S = lfunthetainit_bitprec(ldata, tdom? tdom: gen_1, m, prec2nbits(prec));
  return gerepilecopy(av, S);
}

GEN
lfunan(GEN ldata, long L, long prec)
{
  pari_sp av = avma;
  ldata = lfunmisc_to_ldata_shallow(ldata);
  return gerepilecopy(av, ldata_vecan(ldata_get_an(ldata), L, prec));
}

/* return [1^a 2^a,...,lim^a] */
static GEN
mkvpow(GEN a, long lim, long prec)
{
  pari_sp av = avma;
  GEN v = const_vec(lim, gen_1);
  long n;
  forprime_t iter;
  ulong p;

  u_forprime_init(&iter, 2, lim);
  while ((p = u_forprime_next(&iter)))
  {
    long limp = lim/p + 1, q = p;
    GEN tp = gpow(utoipos(p), a, prec), tq = tp;
    for(;;)
    {
      long nq;
      for (n=1, nq=q; nq <= lim; n++, nq+=q)
        if (n % p) gel(v,nq) = gmul(gel(v,nq), tq);
      if (q >= limp) break;
      q *= p;
      tq = gmul(tq, tp); /* q^(2/d) */
    }
  }
  return gerepilecopy(av, v);
}

/* return [1^(2/d), 2^(2/d),...,lim^(2/d)] */
static GEN
mkvroots(long d, long lim, long prec)
{
  if (d <= 4)
  {
    GEN v = cgetg(lim+1,t_VEC);
    long n;
    switch(d)
    {
      case 1:
        for (n=1; n <= lim; n++) gel(v,n) = sqru(n);
        return v;
      case 2:
        for (n=1; n <= lim; n++) gel(v,n) = utoipos(n);
        return v;
      case 4:
        for (n=1; n <= lim; n++) gel(v,n) = sqrtr(stor(n, prec));
        return v;
    }
  }
  return mkvpow(gdivgs(gen_2,d), lim, prec);
}

GEN
lfunthetacheckinit(GEN data, GEN t, long m, long *pbitprec, long fl)
{
  long bitprec = *pbitprec;
  if (is_linit(data) && linit_get_type(data)==t_LDESC_THETA)
  {
    GEN tdom, thetainit = linit_get_tech(data);
    long bitprecnew = theta_get_bitprec(thetainit);
    long m0 = theta_get_m(thetainit);
    double r, al, rt, alt;
    if (m0 != m)
      pari_err_DOMAIN("lfuntheta","derivative order","!=", stoi(m),stoi(m0));
    if (bitprec > bitprecnew) goto INIT;
    *pbitprec = bitprecnew;
    get_cone(t, &rt, &alt);
    tdom = theta_get_tdom(thetainit);
    r = rtodbl(gel(tdom,1));
    al= rtodbl(gel(tdom,2)); if (rt >= r && alt <= al) return data;
  }
INIT:
  if (fl) { bitprec += BITS_IN_LONG; *pbitprec = bitprec; }
  return lfunthetainit_bitprec(data, t, m, bitprec);
}

long
lfunisvgaell(GEN Vga, long flag)
{
  GEN al1, al2;
  long d = lg(Vga)-1;
  if (d != 2) return 0;
  al1 = gel(Vga, 1); al2 = gel(Vga, 2);
  if (flag) return gequal1(gabs(gsub(al1, al2), LOWDEFAULTPREC));
  else return (gequal0(al1) && gequal1(al2)) || (gequal0(al2) && gequal1(al1));
}

/* generic */
static GEN
vecan_nv_cmul(void *E, GEN P, long a, GEN x)
{
  GEN vroots = (GEN)E;
  return (a==0 || !gel(P,a))? NULL: gmul(gmul(gel(vroots,a), gel(P,a)), x);
}
/* al = 1 */
static GEN
vecan_n_cmul(void *E, GEN P, long a, GEN x)
{
  (void)E;
  return (a==0 || !gel(P,a))? NULL: gmul(gmulsg(a,gel(P,a)), x);
}
/* al = 0 */
static GEN
vecan_cmul(void *E, GEN P, long a, GEN x)
{
  (void)E;
  return (a==0 || !gel(P,a))? NULL: gmul(gel(P,a), x);
}
/* d=2, 2 sum_{n <= limt} a_n (n t)^al q^n, q = exp(-2pi t) */
static GEN
theta2(GEN vecan, long limt, GEN t, GEN al, long prec)
{
  GEN S, q, pi2 = Pi2n(1,prec), vroots = NULL;
  const struct bb_algebra *alg = get_Rg_algebra();
  GEN (*cmul)(void *, GEN, long, GEN);
  long flag;
  if (gequal0(al))
  {
    cmul = vecan_cmul;
    flag = 0;
  }
  else if (gequal1(al))
  {
    cmul = vecan_n_cmul;
    flag = 1;
  }
  else
  {
    vroots = mkvpow(al, limt, prec);
    cmul = vecan_nv_cmul;
    flag = 2;
  }
  setsigne(pi2,-1); q = gexp(gmul(pi2, t), prec);
  S = gen_bkeval(vecan, limt, q, 1, (void*)vroots, alg, cmul);
  switch (flag)
  {
    case 1: S = gmul(t, S); break;
    case 2: S = gmul(gpow(t,al,prec), S);
  }
  return gmul2n(S,1);
}

/* d=1, 2 sum_{n <= limt} a_n (n t)^al q^(n^2), q = exp(-pi t^2) */
static GEN
theta1(GEN vecan, long limt, GEN t, GEN al, long prec)
{
  GEN q = gexp(gmul(negr(mppi(prec)), gsqr(t)), prec);
  GEN vexp = gsqrpowers(q, limt), S = gen_0;
  pari_sp av = avma;
  long n;
  if (gcmp0(al))
    for (n = 1; n <= limt; ++n)
    {
      GEN an = gel(vecan, n);
      if (gequal0(an)) continue;
      S = gadd(S, gmul(an, gel(vexp, n)));
      if (gc_needed(av, 3)) S = gerepileupto(av, S);
    }
  else if (gcmp1(al))
  {
    for (n = 1; n <= limt; ++n)
    {
      GEN an = gel(vecan, n);
      if (gequal0(an)) continue;
      S = gadd(S, gmul(gmulgs(an, n), gel(vexp, n)));
      if (gc_needed(av, 3)) S = gerepileupto(av, S);
    }
    S = gmul(S,t);
  }
  else
  {
    GEN vroots = mkvpow(al, limt, prec);
    av = avma;
    for (n = 1; n <= limt; ++n)
    {
      GEN an = gel(vecan, n);
      if (gequal0(an)) continue;
      S = gadd(S, gmul(gmul(an, gel(vroots,n)), gel(vexp, n)));
      if (gc_needed(av, 3)) S = gerepileupto(av, S);
    }
    S = gmul(S, gpow(t,al,prec));
  }
  return gmul2n(S,1);
}

/* If m > 0, compute m-th derivative of theta(t) = theta0(t/sqrt(N))
 * with absolute error 2^-bitprec */
GEN
lfuntheta_bitprec(GEN data, GEN t, long m, long bitprec)
{
  pari_sp ltop = avma;
  long limt, d;
  GEN sqN, vecan, Vga, ldata, theta, thetainit, S;
  long n, bitprecnew = bitprec, prec = nbits2prec(bitprec);
  t = gprec_w(t, prec);
  theta = lfunthetacheckinit(data, t, m, &bitprecnew, 0);
  ldata = linit_get_ldata(theta);
  thetainit = linit_get_tech(theta);
  vecan = theta_get_an(thetainit);
  sqN = theta_get_sqrtN(thetainit);
  limt = lg(vecan)-1;
  if (theta == data)
    limt = minss(limt, lfunthetaneed_bitprec(ldata, t, m, bitprec));
  t = gdiv(t, sqN);
  Vga = ldata_get_gammavec(ldata);
  d = lg(Vga) - 1;
  if (m == 0 && d == 1)
  {
    S = theta1(vecan, limt, t, gel(Vga,1), prec);
    return gerepileupto(ltop, S);
  }
  if (m == 0 && lfunisvgaell(Vga, 1))
  {
    S = theta2(vecan, limt, t, vecmin(Vga), prec);
    return gerepileupto(ltop, S);
  }
  else
  {
    GEN K = theta_get_K(thetainit);
    GEN vroots = mkvroots(d, limt, prec);
    t = gpow(t, gdivgs(gen_2,d), prec);
    S = gen_0;
    for (n = 1; n <= limt; ++n)
    {
      GEN nt, an = gel(vecan, n);
      if (gequal0(an)) continue;
      nt = gmul(gel(vroots,n), t);
      if (m) an = gmul(an, powuu(n, m));
      S = gadd(S, gmul(an, gammamellininvrt_bitprec(K, nt, bitprecnew)));
    }
    if (m) S = gdiv(S, gpowgs(sqN, m));
    return gerepileupto(ltop, S);
  }
}

/* theta(t)=\sum_{n\ge1}a(n)K(nt/N^(1/2)) */
GEN
lfuntheta(GEN data, GEN t, long m, long prec)
{ return lfuntheta_bitprec(data, t, m, prec2nbits(prec)); }

/*******************************************************************/
/* Second part: Computation of L-Functions.                        */
/*******************************************************************/

struct lfunp {
  long precmax, Dmax, D, M, m0, nmax, d;
  double k1, E, logN2, logC, A, hd, dc, dw, dh, MAXs, sub;
  GEN L, vprec, an;
};

static void
lfunparams(GEN ldata, long der, long bitprec, struct lfunp *S)
{
  const long derprec = (der > 1)? dbllog2(mpfact(der)): 0; /* log2(der!) */
  GEN Vga, N, L;
  long k, k1, d, m, M, flag, nmax;
  double a, E, hd, Ep, d2, suma, maxs, mins, sub;

  Vga = ldata_get_gammavec(ldata);
  S->d = d = lg(Vga)-1; d2 = d/2.;
  suma = gtodouble(vecsum(Vga));
  k = ldata_get_k(ldata);
  N = ldata_get_conductor(ldata);
  S->logN2 = log(gtodouble(N)) / 2;
  maxs = S->dc + S->dw;
  mins = S->dc - S->dw;
  S->MAXs = maxss(maxs, k-mins);

  /* we compute Lambda^(der)(s) / der!; need to compensate for L^(der)(s)
   * ln |gamma(s)| ~ (pi/4) d |t|; max with 1: fudge factor */
  S->D = (long)ceil(bitprec + derprec + maxdd((M_PI/(4*LOG2))*d*S->dh, 1));
  S->E = E = LOG2*S->D; /* D:= required absolute bitprec */

  Ep = E + maxdd(M_PI * S->dh * d2, (d*S->MAXs + suma - 1) * log(E));
  hd = d2*M_PI*M_PI / Ep;
  S->m0 = (long)ceil(LOG2/hd);
  S->hd = LOG2/S->m0;

  S->logC = d2*LOG2 - log(d2)/2;
  k1 = (double)(k - 1);
  if (!ldata_get_residue(ldata)) k1 /= 2.;
  S->k1 = k1; /* assume |a_n| << n^k1 with small implied constant */
  S->A  = gammavec_expo(d, suma);

  sub = 0.;
  if (mins > 1)
  {
    GEN sig = dbltor(mins);
    sub += S->logN2*mins;
    if (gammaordinary(Vga, sig))
    {
      GEN FVga = gammafactor(Vga);
      GEN gas = gammafactproduct(FVga, sig, LOWDEFAULTPREC);
      if (typ(gas) != t_SER)
      {
        double dg = dbllog2(gas);
        if (dg > 0) sub += dg * LOG2;
      }
    }
  }
  S->sub = sub;
  M = 1000;
  L = cgetg(M+2, t_VECSMALL);
  a = S->k1 + S->A;
  nmax = 0;
  flag = 0;
  for (m = 0;; m++)
  {
    double mh = m*S->hd, H = S->logN2-mh;
    double B = S->E + maxdd(mh*S->MAXs - S->sub, 0) + S->k1*H + S->logC;
    double x = dblcoro526(a, d/2., B);
    long n = floor(x*exp(H) + 0.5); /* 0.5: fudge factor */
    if (n > nmax) nmax = n;
    if (m > M)
    {
      M *= 2;
      L = vecsmall_lengthen(L,M+2);
    }
    L[m+1] = n;
    if (n == 0) { if (++flag == 2) break; } else flag = 0;
  }
  S->M = m-3; setlg(L, S->M+1);
  S->L = L;
  S->nmax = nmax;

  S->Dmax = S->D + (long)ceil((S->M * S->hd * S->MAXs - S->sub) / LOG2);
  if (S->Dmax < S->D) S->Dmax = S->D;
  S->precmax = nbits2prec(S->Dmax);
  if (DEBUGLEVEL > 1)
    err_printf("Dmax=%ld, D=%ld, M = %ld, nmax = %ld, m0 = %ld\n",
               S->Dmax,S->D,S->M,S->nmax, S->m0);
}

/* x0 * [1,x,..., x^n] */
static GEN
powersshift(GEN x, long n, GEN x0)
{
  long i, l = n+2;
  GEN V = cgetg(l, t_VEC);
  gel(V,1) = x0;
  for(i = 2; i < l; i++) gel(V,i) = gmul(gel(V,i-1),x);
  return V;
}

/* d=2 and Vga = [a,a+1] */
static GEN
lfuninit_vecc2(GEN theta, GEN h, struct lfunp *Q)
{
  const long M = Q->M, prec = Q->precmax;
  GEN L = Q->L;
  GEN ldata = linit_get_ldata(theta);
  GEN a = vecmin(ldata_get_gammavec(ldata));
  GEN thetainit = linit_get_tech(theta);
  GEN vecan = RgV_kill0( theta_get_an(thetainit) );
  GEN sqN = theta_get_sqrtN(thetainit);
  GEN qk = powersshift(mpexp(h), M, ginv(sqN));
  GEN v = cgetg(M + 2, t_VEC);
  long m, L0 = lg(vecan)-1;
  for (m = 0; m <= M; m++)
  {
    pari_sp av = avma;
    GEN t = gel(qk, m+1);
    GEN S = theta2(vecan, minss(L[m+1],L0), t, a, prec);
    gel(v, m+1) = gerepileupto(av, S); /* theta(exp(mh)) */
  }
  return v;
}

/* return [\theta(exp(mh)), m=0..M], theta(t) = sum a(n) K(n/sqrt(N) t),
 * h = log(2)/m0 */
static GEN
lfuninit_vecc(GEN theta, GEN h, struct lfunp *S)
{
  const long m0 = S->m0;
  GEN thetainit, vK, L, K, an, d2, sqN, vroots, vecc, eh2d, peh2d, vprec;
  long d, M, prec, m, n, neval;

  if (!S->vprec) return lfuninit_vecc2(theta, h, S);

  d = S->d;
  L = S->L;
  M = S->M;
  an= S->an;
  vprec = S->vprec;
  prec = S->precmax;
  thetainit = linit_get_tech(theta);
  K = theta_get_K(thetainit);
  sqN = theta_get_sqrtN(thetainit);

  /* For all 0<= m <= M, and all n <= L[m+1] such that a_n!=0, we must compute
   *   k[m,n] = K(n exp(mh)/sqrt(N))
   * with ln(absolute error) <= E + max(mh sigma - sub, 0) + k1 * log(n).
   * N.B. we use the 'rt' variant and pass argument (n exp(mh)/sqrt(N))^(2/d).
   * Speedup: if n' = 2n and m' = m - m0 >= 0; then k[m,n] = k[m',n']. */
  /* vroots[n] = n^(2/d) */
  vroots = mkvroots(d, S->nmax, prec);
  d2 = gdivgs(gen_2, d);
  eh2d = gexp(gmul(d2,h), prec); /* exp(2h/d) */
  /* peh2d[m+1] = (exp(mh)/sqrt(N))^(2/d) */
  peh2d = powersshift(eh2d, M, invr(gpow(sqN, d2, prec)));
  neval = 0;
  /* vK[m+1,n] will contain k[m,n]. For each 0 <= m <= M, sum for n<=L[m+1] */
  vK = cgetg(M+2, t_VEC);
  for (m = 0; m <= M; m++)
    gel(vK,m+1) = const_vec(L[m+1], NULL);

  for (m = M; m >= 0; m--)
    for (n = 1; n <= L[m+1]; n++)
    {
      GEN t2d, kmn = gmael(vK,m+1,n);
      long nn, mm, p = 0;

      if (kmn) continue; /* done already */
      /* p = largest (absolute) accuracy to which we need k[m,n] */
      for (mm=m,nn=n; mm>=0 && nn <= L[mm+1]; nn<<=1,mm-=m0)
        if (gel(an, nn)) p = maxuu(p, umael(vprec,mm+1,nn));
      if (!p) continue; /* a_{n 2^v} = 0 for all v in range */
      t2d = mpmul(gel(vroots, n), gel(peh2d,m+1)); /*(n exp(mh)/sqrt(N))^(2/d)*/
      neval++;
      kmn = gammamellininvrt_bitprec(K, t2d, p);
      for (mm=m,nn=n; mm>=0 && nn <= L[mm+1]; nn<<=1,mm-=m0)
        gmael(vK,mm+1,nn) = kmn;
    }
  if (DEBUGLEVEL >= 1) err_printf("true evaluations: %ld\n", neval);

  /* theta(exp(mh)) ~ sum_{n <= L[m]} a(n) k[m,n] */
  vecc = cgetg(M+2, t_VEC);
  for (m = 0; m <= M; ++m)
  {
    pari_sp av = avma;
    GEN s = gen_0;
    long n;
    for (n = 1; n <= L[m+1]; n++)
    {
      if (!gel(an,n)) continue;
      s = gadd(s, gmul(gel(an,n), gmael(vK,m+1,n)));
    }
    gel(vecc,m+1) = gerepileupto(av, s);
  }
  return vecc;
}

static void
parse_dom(GEN dom, struct lfunp *S)
{
  if (typ(dom)!=t_VEC || lg(dom)!=4) pari_err_TYPE("lfuninit [domain]", dom);
  S->dc = gtodouble(gel(dom,1));
  S->dw = gtodouble(gel(dom,2));
  S->dh = gtodouble(gel(dom,3));
  if (S->dw < 0 || S->dh < 0) pari_err_TYPE("lfuninit [domain]", dom);
}

/* do we have dom \subset dom0 ? dom = [center, width, height] */
int
sdomain_isincl(GEN dom, GEN dom0)
{
  struct lfunp S0, S;
  parse_dom(dom, &S);
  parse_dom(dom0, &S0);
  return S0.dc - S0.dw <= S.dc - S.dw
      && S0.dc + S0.dw >= S.dc + S.dw && S0.dh >= S.dh;
}

static int
checklfuninit(GEN linit, GEN dom, long der, long bitprec)
{
  GEN domain = lfun_get_domain(linit_get_tech(linit));
  return domain_get_der(domain) >= der
    && domain_get_bitprec(domain) >= bitprec
    && sdomain_isincl(dom, domain_get_dom(domain));
}

GEN
lfuninit_make(long t, GEN ldata, GEN molin, GEN domain)
{
  GEN Vga = ldata_get_gammavec(ldata);
  long d = lg(Vga)-1;
  long k = ldata_get_k(ldata);
  GEN k2 = gdivgs(stoi(k), 2);
  GEN expot = gdivgs(gadd(gmulsg(d, gsubgs(k2, 1)), vecsum(Vga)), 4);
  GEN eno = ldata_get_rootno(ldata);
  long prec = nbits2prec( domain_get_bitprec(domain) );
  GEN w2 = ginv(gsqrt(eno, prec));
  GEN hardy = mkvec4(k2, w2, expot, gammafactor(Vga));
  return mkvec3(mkvecsmall(t),ldata, mkvec3(domain, molin, hardy));
}

static void
lfunparams2(GEN ldata, GEN an, struct lfunp *S)
{
  GEN vprec, L, Vga = ldata_get_gammavec(ldata);
  double sig0, pmax, sub2;
  long m, nan, nmax, neval, M;
  if (lfunisvgaell(Vga, 1)) { S->an = S->vprec = NULL; return; }
  L = S->L;
  M = S->M;

  /* try to reduce parameters now we know the a_n (some may be 0) */
  nan = lg(an)-1;
  S->an = an = RgV_kill0(an);
  nmax = neval = 0;
  for (m = 0; m <= M; m++)
  {
    long n = minss(nan, L[m+1]);
    while (n > 0 && !gel(an, n)) n--;
    if (n > nmax) nmax = n;
    neval += n;
    L[m+1] = n; /* reduce S->L[m+1] */
  }
  if (DEBUGLEVEL >= 1) err_printf("expected evaluations: %ld\n", neval);
  for (; M > 0; M--)
    if (L[M+1]) break;
  setlg(L, M+2);
  S->M = M;
  S->nmax = nmax;

  pmax = 0;
  sig0 = S->MAXs/S->m0;
  sub2 = S->sub / LOG2;
  vprec = cgetg(S->M+2, t_VEC);
  /* compute accuracy to which we will need k[m,n] = K(n*exp(mh/sqrt(N)))
   * vprec[m+1,n] = absolute accuracy to which we need k[m,n] */
  for (m = 0; m <= S->M; m++)
  {
    double c = S->D + maxdd(m*sig0 - sub2, 0);
    GEN t;
    if (!S->k1)
    {
      t = const_vecsmall(L[m+1]+1, c);
      pmax = maxdd(pmax,c);
    }
    else
    {
      long n;
      t = cgetg(L[m+1]+1, t_VECSMALL);
      for (n = 1; n <= L[m+1]; n++)
      {
        t[n] = c + S->k1 * log2(n);
        pmax = maxdd(pmax, t[n]);
      }
    }
    gel(vprec,m+1) = t;
  }
  S->vprec = vprec;
  S->Dmax = pmax;
  S->precmax = nbits2prec(pmax);
}

static GEN
lfun_init_theta(GEN ldata, GEN eno, struct lfunp *S)
{
  GEN an, tdom = NULL;
  long L;
  if (eno)
    L = S->nmax;
  else
  {
    tdom = dbltor(sqrt(0.5));
    L = maxss(S->nmax, lfunthetaneed_bitprec(ldata, tdom, 0, S->D));
  }
  an = ldata_vecan(ldata_get_an(ldata), L, S->precmax);
  lfunparams2(ldata, an, S);
  return lfunthetainit0_bitprec(ldata, tdom, an, 0, S->Dmax, 0);
}

GEN
lfuninit_bitprec(GEN lmisc, GEN dom, long der, long bitprec)
{
  pari_sp ltop = avma;
  GEN R, h, theta, ldata, qk, poqk, pol, eno, r, vecc, domain, molin;
  long m, M, k;
  struct lfunp S;

  if (is_linit(lmisc))
  {
    long t = linit_get_type(lmisc);
    if (t==t_LDESC_INIT || t==t_LDESC_PRODUCT)
    {
      if (checklfuninit(lmisc, dom, der, bitprec)) return lmisc;
      pari_warn(warner,"lfuninit: insufficient initialization");
    }
  }
  ldata = lfunmisc_to_ldata_shallow(lmisc);

  if (ldata_get_type(ldata)==t_LFUN_NF)
  {
    GEN T = gel(ldata_get_an(ldata), 2);
    return lfunzetakinit_bitprec(T, dom, der, 0, bitprec);
  }
  parse_dom(dom, &S);
  lfunparams(ldata, der, bitprec, &S);
  r = ldata_get_residue(ldata);
  /* Note: all guesses should already have been performed (thetainit more
   * expensive than needed: should be either tdom = 1 or bitprec = S.D).
   * BUT if the root number / polar part do not have an algebraic
   * expression, there is no way to do this until we know the
   * precision, i.e. now. So we can't remove guessing code from here and
   * lfun_init_theta */
  if (r && isintzero(r)) eno = NULL;
  else
  {
    eno = ldata_get_rootno(ldata);
    if (isintzero(eno)) eno = NULL;
  }
  theta = lfun_init_theta(ldata, eno, &S);
  if (eno)
    R = theta_get_R(linit_get_tech(theta));
  else
  {
    GEN v = lfunrootres_bitprec(theta, S.D);
    ldata = shallowcopy(ldata);
    gel(ldata, 6) = gel(v,3);
    r = gel(v,1);
    if (isintzero(r))
      setlg(ldata,7); /* no pole */
    else
      gel(ldata, 7) = r;
    R = lfunrtoR(ldata, nbits2prec(S.D));
  }
  h = divru(mplog2(S.precmax), S.m0);
  vecc = lfuninit_vecc(theta, h, &S);
  k = ldata_get_k(ldata);
  qk = gprec_w(mpexp(gmul2n(gmulsg(k,h), -1)), S.precmax); /* exp(kh/2) */
  M = S.M;
  poqk = gpowers(qk, M);
  pol = cgetg(M+3, t_POL); pol[1] = evalsigne(1) | evalvarn(0);
  gel(pol, 2) = gprec_w(gmul2n(gel(vecc,1), -1),S.precmax);
  for (m = 2; m <= M+1; m++)
    gel(pol, m+1) = gprec_w(gmul(gel(poqk,m), gel(vecc,m)), S.precmax);
  pol = RgX_renormalize_lg(pol, M+3);
  molin = mkvec3(h, pol, R);
  domain = mkvec2(dom, mkvecsmall2(der, bitprec));
  return gerepilecopy(ltop, lfuninit_make(t_LDESC_INIT, ldata, molin, domain));
}

GEN
lfuninit(GEN lmisc, GEN dom, long der, long prec)
{
  GEN z = lfuninit_bitprec(lmisc, dom, der, prec2nbits(prec));
  return z == lmisc? gcopy(z): z;
}

/* If s is a pole of Lambda, return polar part at s; else return NULL */
static GEN
lfunpoleresidue(GEN R, GEN s)
{
  long j;
  for (j = 1; j < lg(R); j++)
  {
    GEN Rj = gel(R, j), be = gel(Rj, 1);
    if (gequal(s, be)) return gel(Rj, 2);
  }
  return NULL;
}

/* Compute contribution of polar part at s when not a pole. */
static GEN
veccothderivn(GEN a, long n)
{
  long i;
  pari_sp av = avma;
  GEN c = pol_x(0), cp = mkpoln(3, gen_m1, gen_0, gen_1);
  GEN v = cgetg(n+2, t_VEC);
  gel(v, 1) = poleval(c, a);
  for(i = 2; i <= n+1; i++)
  {
    c = ZX_mul(ZX_deriv(c), cp);
    gel(v, i) = gdiv(poleval(c, a), mpfact(i-1));
  }
  return gerepilecopy(av, v);
}

static GEN
polepart(long n, GEN h, GEN C)
{
  GEN h2n = gpowgs(gdiv(h, gen_2), n-1);
  GEN res = gmul(h2n, gel(C,n));
  return odd(n)? res : gneg(res);
}

static GEN
lfunsumcoth(GEN R, GEN s, GEN h, long prec)
{
  long i,j;
  GEN S = gen_0;
  for (j = 1; j < lg(R); ++j)
  {
    GEN r = gel(R,j), be = gel(r,1), Rj = gel(r, 2);
    long e = valp(Rj);
    GEN z1 = gexpm1(gmul(h, gsub(s,be)), prec); /* exp(h(s-beta))-1 */
    GEN c1 = gaddgs(gdivsg(2, z1), 1); /* coth((h/2)(s-beta)) */
    GEN C1 = veccothderivn(c1, 1-e);
    for (i = e; i < 0; i++)
    {
      GEN Rbe = mysercoeff(Rj, i);
      GEN p1 = polepart(-i, h, C1);
      S = gadd(S, gmul(Rbe, p1));
    }
  }
  return gmul2n(S, -1);
}

static GEN
lfun_genproduct(GEN data, GEN s, long bitprec,
                GEN (*fun)(GEN ldata, GEN s, long bitprec))
{
  pari_sp av = avma;
  GEN ldata = linit_get_ldata(data), v = lfunprod_get_fact(linit_get_tech(data));
  GEN r = gen_1, F = gel(v, 1), E = gel(v, 2), C = gel(v, 3);
  long i, l = lg(F);
  GEN cs = gconj(s);
  long isreal = gequal(imag_i(s), imag_i(cs));
  for (i = 1; i < l; ++i)
  {
    GEN f = fun(gel(F, i), s, bitprec);
    if (E[i])
      r = gmul(r, gpowgs(f, E[i]));
    if (C[i])
    {
      GEN fc = isreal ? f: fun(gel(F, i), cs, bitprec);
      r = gmul(r, gpowgs(gconj(fc), C[i]));
    }
  }
  if ((ldata_get_selfdual(ldata)==0 && gequal0(gimag(s)))) r = greal(r);
  return gerepileupto(av, r);
}

/* s a t_SER */
static long
der_level(GEN s)
{ return signe(s)? lg(s)-3: valp(s)-1; }

/* s a t_SER; return coeff(s, X^0) */
static GEN
ser_coeff0(GEN s) { return simplify_shallow(polcoeff_i(s, 0, -1)); }

static GEN
get_domain(GEN s, GEN *dom, long *der)
{
  GEN sa = s;
  *der = 0;
  switch(typ(s))
  {
    case t_POL:
    case t_RFRAC: s = toser_i(s);
    case t_SER:
      *der = der_level(s);
      sa = ser_coeff0(s);
  }
  *dom = mkvec3(real_i(sa), gen_0, gabs(imag_i(sa),DEFAULTPREC));
  return s;
}

/* assume lmisc is an linit, s went through get_domain and s/bitprec belong
 * to domain */
static GEN
lfunlambda_bitprec_OK(GEN linit, GEN s, long bitprec)
{
  GEN eno, ldata, tech, h, pol;
  GEN S, S0 = NULL, k2;
  long prec;

  if (linit_get_type(linit) == t_LDESC_PRODUCT)
    return lfun_genproduct(linit, s, bitprec, lfunlambda_bitprec_OK);
  ldata = linit_get_ldata(linit);
  eno = ldata_get_rootno(ldata);
  tech = linit_get_tech(linit);
  h = lfun_get_step(tech); prec = realprec(h);
  pol = lfun_get_pol(tech);
  s = gprec_w(s, prec);
  if (ldata_get_residue(ldata))
  {
    GEN R = lfun_get_Residue(tech);
    GEN Ra = lfunpoleresidue(R, s);
    if (Ra) return gprec_w(Ra, nbits2prec(bitprec));
    S0 = lfunsumcoth(R, s, h, prec);
  }
  k2 = lfun_get_k2(tech);
  if (gequal(real_i(s), k2))
  { /* on critical line: shortcut */
    GEN polz, b = imag_i(s);
    polz = gequal0(b)? poleval(pol,gen_1): poleval(pol, expIr(gmul(h,b)));
    S = gadd(polz, gmul(eno, gconj(polz)));
  }
  else
  {
    GEN z = gexp(gmul(h, gsub(s, k2)), prec);
    GEN zi = gconj(ginv(z));
    S = gadd(poleval(pol, z), gmul(eno, gconj(poleval(pol, zi))));
  }
  if (S0) S = gadd(S,S0);
  return gprec_w(gmul(S,h), nbits2prec(bitprec));
}
GEN
lfunlambda_bitprec(GEN lmisc, GEN s, long bitprec)
{
  pari_sp av = avma;
  GEN linit, dom, z;
  long der;
  s = get_domain(s, &dom, &der);
  linit = lfuninit_bitprec(lmisc, dom, der, bitprec);
  z = lfunlambda_bitprec_OK(linit,s,bitprec);
  return gerepilecopy(av, z);
}
GEN
lfunlambda(GEN lmisc, GEN s, long prec)
{ return lfunlambda_bitprec(lmisc, s, prec2nbits(prec)); }

/* assume lmisc is an linit, s went through get_domain and s/bitprec belong
 * to domain */
static GEN
lfun_bitprec_OK(GEN linit, GEN s, long bitprec)
{
  GEN N, gas, S, FVga, res;
  long prec = nbits2prec(bitprec);

  FVga = lfun_get_factgammavec(linit_get_tech(linit));
  S = lfunlambda_bitprec_OK(linit, s, bitprec);
  gas = gammafactproduct(FVga, s, prec);
  N = ldata_get_conductor(linit_get_ldata(linit));
  res = gdiv(S, gmul(gpow(N, gdivgs(s, 2), prec), gas));
  if (typ(s)!=t_SER && typ(res)==t_SER)
  {
    long v = valp(res);
    if (v > 0) return gen_0;
    if (v == 0) res = gel(res, 2);
  }
  return gprec_w(res, prec);
}
GEN
lfun_bitprec(GEN lmisc, GEN s, long bitprec)
{
  pari_sp av = avma;
  GEN linit, dom, z;
  long der;
  s = get_domain(s, &dom, &der);
  linit = lfuninit_bitprec(lmisc, dom, der, bitprec);
  z = lfun_bitprec_OK(linit,s,bitprec);
  return gerepilecopy(av, z);
}
GEN
lfun(GEN lmisc, GEN s, long prec)
{ return lfun_bitprec(lmisc, s, prec2nbits(prec)); }

/* given a t_SER a+x*s(x), return x*s(x), shallow */
static GEN
sersplit1(GEN s, GEN *head)
{
  long i, l = lg(s);
  GEN y;
  *head = simplify_shallow(mysercoeff(s, 0));
  if (valp(s) > 0) return s;
  y = cgetg(l-1, t_SER); y[1] = s[1];
  setvalp(y, 1);
  for (i=3; i < l; i++) gel(y,i-1) = gel(s,i);
  return normalize(y);
}

/* n-th derivative of t_SER x */
static GEN
derivnser(GEN x, long n)
{
  long i, vx = varn(x), e = valp(x), lx = lg(x);
  GEN y;
  if (ser_isexactzero(x))
  {
    x = gcopy(x);
    if (e) setvalp(x,e-n);
    return x;
  }
  if (e < 0 || e >= n)
  {
    y = cgetg(lx,t_SER);
    y[1] = evalsigne(1)| evalvalp(e-n) | evalvarn(vx);
    for (i=0; i<lx-2; i++)
      gel(y,i+2) = gmul(muls_interval(i+e-(n-1),i+e), gel(x,i+2));
  } else {
    if (lx <= n+2) return zeroser(vx, 0);
    lx -= n;
    y = cgetg(lx,t_SER);
    y[1] = evalsigne(1)|_evalvalp(0) | evalvarn(vx);
    for (i=0; i<lx-2; i++)
      gel(y,i+2) = gmul(muls_interval(i-(n-1),i),gel(x,i+2+n));
  }
  return normalize(y);
}

/* order of pole of Lambda at s (0 if regular point) */
static long
lfunlambdaord(GEN linit, GEN s)
{
  GEN tech = linit_get_tech(linit);
  if (linit_get_type(linit)==t_LDESC_PRODUCT)
  {
    GEN v = lfunprod_get_fact(linit_get_tech(linit));
    GEN F = gel(v, 1), E = gel(v, 2), C = gel(v, 3);
    long i, ex = 0, l = lg(F);
    for (i = 1; i < l; i++)
      ex += lfunlambdaord(gel(F,i), s) * (E[i]+C[i]);
    return ex;
  }
  if (ldata_get_residue(linit_get_ldata(linit)))
  {
    GEN r = lfunpoleresidue(lfun_get_Residue(tech), s);
    if (r) return lg(r)-2;
  }
  return 0;
}

/* derivative of order m > 0 of L (flag = 0) or Lambda (flag = 1) */
static GEN
lfunderiv_bitprec(GEN lmisc, long m, GEN s, long flag, long bitprec)
{
  pari_sp ltop = avma;
  GEN res, S = NULL, linit, dom;
  long der, prec = nbits2prec(bitprec);
  s = get_domain(s, &dom, &der);
  linit = lfuninit_bitprec(lmisc, dom, der + m, bitprec);
  if (typ(s) == t_SER)
  {
    long v, l = lg(s)-1;
    GEN sh;
    if (valp(s) < 0) pari_err_DOMAIN("lfun","valuation", "<", gen_0, s);
    S = sersplit1(s, &sh);
    v = valp(S);
    s = deg1ser_shallow(gen_1, sh, varn(S), m + (l+v-1)/v);
  }
  else
  {
    long ex = lfunlambdaord(linit, s);
    /* HACK: pretend lfuninit was done to right accuracy */
    s = deg1ser_shallow(gen_1, s, 0, m+1+ex);
  }
  res = flag ? lfunlambda_bitprec_OK(linit, s, bitprec):
               lfun_bitprec_OK(linit, s, bitprec);
  if (S)
    res = gsubst(derivnser(res, m), varn(S), S);
  else if (typ(res)==t_SER)
  {
    long v = valp(res);
    if (v > m) { avma = ltop; return gen_0; }
    if (v >= 0)
      res = gmul(mysercoeff(res, m), mpfact(m));
    else
      res = derivnser(res, m);
  }
  return gerepilecopy(ltop, gprec_w(res, prec));
}

GEN
lfunlambda0_bitprec(GEN lmisc, GEN s, long der, long bitprec)
{
  return der ? lfunderiv_bitprec(lmisc, der, s, 1, bitprec):
               lfunlambda_bitprec(lmisc, s, bitprec);
}

GEN
lfunlambda0(GEN lmisc, GEN s, long der, long prec)
{
  return lfunlambda0_bitprec(lmisc, s, der, prec2nbits(prec));
}

GEN
lfun0_bitprec(GEN lmisc, GEN s, long der, long bitprec)
{
  return der ? lfunderiv_bitprec(lmisc, der, s, 0, bitprec):
               lfun_bitprec(lmisc, s, bitprec);
}

GEN
lfun0(GEN lmisc, GEN s, long der, long prec)
{ return lfun0_bitprec(lmisc, s, der, prec2nbits(prec)); }

GEN
lfunhardy_bitprec(GEN lmisc, GEN t, long bitprec)
{
  pari_sp ltop = avma;
  long prec = nbits2prec(bitprec), k, d;
  GEN argz, z, linit, ldata, tech, dom, w2, k2, expot, h, a;

  switch(typ(t))
  {
    case t_INT: case t_FRAC: case t_REAL: break;
    default: pari_err_TYPE("lfunhardy",t);
  }

  ldata = lfunmisc_to_ldata_shallow(lmisc);
  if (!is_linit(lmisc)) lmisc = ldata;
  k = ldata_get_k(ldata);
  d = ldata_get_degree(ldata);
  dom = mkvec3(dbltor(k/2.), gen_0, gabs(t,LOWDEFAULTPREC));
  linit = lfuninit_bitprec(lmisc, dom, 0, bitprec);
  tech = linit_get_tech(linit);
  w2 = lfun_get_w2(tech);
  k2 = lfun_get_k2(tech);
  expot = lfun_get_expot(tech);
  z = mkcomplex(k2, t);
  argz = gatan(gdiv(t, k2), prec); /* more accurate than garg since k/2 \in Q */
  /* prec may have increased: don't lose accuracy if |z|^2 is exact */
  prec = precision(argz);
  a = gsub(gmulsg(d, gmul(t, gmul2n(argz,-1))),
           gmul(expot,glog(gnorm(z),prec)));
  h = mulreal(lfunlambda_bitprec_OK(linit, z, bitprec), w2);
  return gerepileupto(ltop, gmul(h, gexp(a, prec)));
}

GEN
lfunhardy(GEN lmisc, GEN s, long prec)
{
  return lfunhardy_bitprec(lmisc, s, prec2nbits(prec));
}

/* L = log(t); return  \sum_{i = 0}^{v-1}  R[-i-1] L^i/i! */
static GEN
theta_pole_contrib(GEN R, long v, GEN L)
{
  GEN s = mysercoeff(R,-v);
  long i;
  for (i = v-1; i >= 1; i--)
    s = gadd(mysercoeff(R,-i), gdivgs(gmul(s,L), i));
  return s;
}
/* subtract successively rather than adding everything then subtracting.
 * The polar part is "large" and suffers from cancellation: a little stabler
 * this way */
static GEN
theta_add_polar_part(GEN S, GEN R, GEN t, long prec)
{
  GEN logt = NULL;
  long j, l = lg(R);
  for (j = 1; j < l; j++)
  {
    GEN Rj = gel(R,j), b = gel(Rj,1), Rb = gel(Rj,2);
    long v = -valp(Rb);
    if (v > 1 && !logt) logt = glog(t, prec);
    S = gsub(S, gmul(theta_pole_contrib(Rb,v,logt), gpow(t,b,prec)));
  }
  return S;
}

/* Check whether the coefficients, conductor, weight, polar part and root
 * number are compatible with the functional equation at t0 and 1/t0.
 * Different from lfunrootres. */
long
lfuncheckfeq_bitprec(GEN lmisc, GEN t0, long bitprec)
{
  GEN ldata, theta, t0i, S0, S0i, w, eno;
  long e, k, sd, prec;
  pari_sp av;

  if (is_linit(lmisc) && linit_get_type(lmisc)==t_LDESC_PRODUCT)
  {
    GEN v = lfunprod_get_fact(linit_get_tech(lmisc)), F = gel(v,1);
    long i, b = -bitprec, l = lg(F);
    for (i = 1; i < l; i++)
      b = maxss(b, lfuncheckfeq_bitprec(gel(F,i), t0, bitprec));
    return b;
  }
  av = avma;
  prec = nbits2prec(bitprec);
  if (!t0)
  {
    t0 = gadd(gdivgs(mppi(prec), 3), gdivgs(gen_I(), 7));
    t0i = ginv(t0);
  }
  else if (gcmpgs(gnorm(t0), 1) < 0)
  {
    t0i = t0;
    t0 = ginv(t0);
  }
  else
    t0i = ginv(t0);
  /* |t0| >= 1 */
  theta = lfunthetacheckinit(lmisc, t0i, 0, &bitprec, 0);
  ldata = linit_get_ldata(theta);
  k = ldata_get_k(ldata);
  sd = ldata_get_selfdual(ldata);
  if (sd)
    S0 = gconj(lfuntheta_bitprec(theta, gconj(t0), 0, bitprec));
  else
    S0 = lfuntheta_bitprec(theta, t0, 0, bitprec);
  S0i = lfuntheta_bitprec(theta, t0i, 0, bitprec);

  eno = ldata_get_rootno(ldata);
  if (ldata_get_residue(ldata))
  {
    GEN R = theta_get_R(linit_get_tech(theta));
    if (gequal0(R))
    {
      GEN v, r;
      if (ldata_get_type(ldata) == t_LFUN_NF)
      { /* inefficient since theta not neeeded; no need to optimize for this
           (artificial) query [e.g. lfuncheckfeq(t_POL)] */
        GEN T = gel(ldata_get_an(ldata), 2);
        GEN L = lfunzetakinit_bitprec(T,zerovec(3),0,0,bitprec);
        long e = lfuncheckfeq_bitprec(L,t0,bitprec);
        avma = av; return e;
      }
      v = lfunrootres_bitprec(theta, bitprec);
      r = gel(v,1);
      if (gequal0(eno)) eno = gel(v,3);
      R = lfunrtoR_i(ldata, r, eno, nbits2prec(bitprec));
    }
    S0i = theta_add_polar_part(S0i, R, t0, prec);
  }
  if (gcmp0(S0i) || gcmp0(S0)) pari_err_PREC("lfuncheckfeq");
  w = gdiv(S0i, gmul(S0, gpowgs(t0, k)));
  /* missing rootno: guess it */
  if (gequal0(eno)) eno = lfunrootno_bitprec(theta, bitprec);
  e = gexpo(gsub(w, eno));
  avma = av; return e;
}

long
lfuncheckfeq(GEN data, GEN t0, long prec)
{ return lfuncheckfeq_bitprec(data, t0, prec2nbits(prec)); }

/*******************************************************************/
/*       Compute root number and residues                          */
/*******************************************************************/
/* round root number to \pm 1 if close to integer. */
static GEN
ropm1(GEN eno, long prec)
{
  long e;
  GEN r = grndtoi(eno, &e);
  return (e < -prec2nbits(prec)/2)? r: eno;
}

/* theta for t=1/sqrt(2) and t2==2t simultaneously, saving 25% of the work.
 * Assume correct initialization (no thetacheck) */
static void
lfunthetaspec_bitprec(GEN linit, long bitprec, GEN *pv, GEN *pv2)
{
  pari_sp av = avma;
  GEN t, Vga, an, K, ldata, thetainit, v, v2, vroots;
  long L, prec, n, d;

  ldata = linit_get_ldata(linit);
  thetainit = linit_get_tech(linit);
  prec = nbits2prec(bitprec);
  Vga = ldata_get_gammavec(ldata); d = lg(Vga)-1;
  if (d == 1 || lfunisvgaell(Vga, 1))
  {
    GEN v2 = sqrtr(real2n(1, nbits2prec(bitprec)));
    GEN v = shiftr(v2,-1);
    *pv = lfuntheta_bitprec(linit, v,  0, bitprec);
    *pv2= lfuntheta_bitprec(linit, v2, 0, bitprec);
    return;
  }
  an = RgV_kill0( theta_get_an(thetainit) );
  L = lg(an)-1;
  /* to compute theta(1/sqrt(2)) */
  t = ginv(gsqrt(gmul2n(ldata_get_conductor(ldata), 1), prec));
  /* t = 1/sqrt(2N) */

  /* From then on, the code is generic and could be used to compute
   * theta(t) / theta(2t) without assuming t = 1/sqrt(2) */
  K = theta_get_K(thetainit);
  vroots = mkvroots(d, L, prec);
  t = gpow(t, gdivgs(gen_2, d), prec); /* rt variant: t->t^(2/d) */
  /* v = \sum_{n <= L, n odd} a_n K(nt) */
  for (v = gen_0, n = 1; n <= L; n+=2)
  {
    GEN tn, Kn, a = gel(an, n);

    if (!a) continue;
    tn = gmul(t, gel(vroots,n));
    Kn = gammamellininvrt_bitprec(K, tn, bitprec);
    v = gadd(v, gmul(a,Kn));
  }
  /* v += \sum_{n <= L, n even} a_n K(nt), v2 = \sum_{n <= L/2} a_n K(2n t) */
  for (v2 = gen_0, n = 1; n <= L/2; n++)
  {
    GEN t2n, K2n, a = gel(an, n), a2 = gel(an,2*n);

    if (!a && !a2) continue;
    t2n = gmul(t, gel(vroots,2*n));
    K2n = gammamellininvrt_bitprec(K, t2n, bitprec);
    if (a) v2 = gadd(v2, gmul(a, K2n));
    if (a2) v = gadd(v,  gmul(a2,K2n));
  }
  *pv = v;
  *pv2 = v2;
  gerepileall(av, 2, pv,pv2);
}

static GEN
Rtor(GEN a, GEN R, GEN ldata, long prec)
{
  GEN FVga = gammafactor(ldata_get_gammavec(ldata));
  GEN Na = gpow(ldata_get_conductor(ldata), gdivgs(a,2), prec);
  return gdiv(R, gmul(Na, gammafactproduct(FVga, a, prec)));
}

/* v = theta(t), vi = theta(1/t) */
static GEN
get_eno(GEN R, long k, GEN t, GEN v, GEN vi, long vx, long bitprec)
{
  long prec = nbits2prec(bitprec);
  GEN a0, a1;
  GEN S = deg1pol(gmul(gpowgs(t,k), gneg(gconj(v))), vi, vx);

  S = theta_add_polar_part(S, R, t, prec);
  if (typ(S) != t_POL || degpol(S) != 1) return NULL;
  a1 = gel(S,3); if (gexpo(a1) < -bitprec/2) return NULL;
  a0 = gel(S,2);
  return gdiv(a0, gneg(a1));

}
/* Return w using theta(1/t) - w t^k \bar{theta}(t) = polar_part(t,w).
 * The full Taylor development of L must be known */
GEN
lfunrootno_bitprec(GEN linit, long bitprec)
{
  GEN ldata, t, eno, v, vi, R;
  long k, prec = nbits2prec(bitprec), vx = fetch_var();
  pari_sp av;

  /* initialize for t > 1/sqrt(2) */
  linit = lfunthetacheckinit(linit, dbltor(sqrt(0.5)), 0, &bitprec, 1);
  ldata = linit_get_ldata(linit);
  k = ldata_get_k(ldata);
  R = ldata_get_residue(ldata)? lfunrtoR_eno(ldata, pol_x(vx), prec)
                              : cgetg(1, t_VEC);
  t = gen_1;
  v = lfuntheta_bitprec(linit, t, 0, bitprec);
  eno = get_eno(R,k,t,v,v, vx, bitprec);
  if (!eno)
  { /* t = sqrt(2), vi = theta(1/t), v = theta(t) */
    lfunthetaspec_bitprec(linit, bitprec, &vi, &v);
    t = sqrtr(utor(2, prec));
    eno = get_eno(R,k,t,v,vi, vx, bitprec);
  }
  av = avma;
  while (!eno)
  {
    t = addsr(1, shiftr(utor(pari_rand(), prec), -66)); /* in [1,1.25[ */
    v = lfuntheta_bitprec(linit, t, 0, bitprec);
    vi= lfuntheta_bitprec(linit, ginv(t), 0, bitprec);
    eno = get_eno(R,k,t,v,vi, vx, bitprec);
    avma = av;
  }
  delete_var(); return ropm1(eno,prec);
}
GEN
lfunrootno(GEN L, long prec)
{ return lfunrootno_bitprec(L, prec2nbits(prec)); }

static int
residues_known(GEN r)
{
  long i, l = lg(r);
  for (i = 1; i < l; i++)
    if (gequal0(gmael(r, i, 2))) return 0;
  return 1;
}

/* Find root number and/or residues when L-function coefficients and
   conductor are known. For the moment at most a single residue allowed. */
GEN
lfunrootres_bitprec(GEN data, long bitprec)
{
  pari_sp ltop = avma;
  GEN w, r, R, a, b, c, d, e, f, dete, th1, th2;
  GEN v, v2, t0, be, tbe, tkbe, tk2, ldata, linit;
  long k, prec;

  ldata = lfunmisc_to_ldata_shallow(data);
  r = ldata_get_residue(ldata);
  k = ldata_get_k(ldata);
  if (r && typ(r) != t_VEC) r = mkvec(mkvec2(stoi(k), simple_pole(r)));
  if (!r || residues_known(r))
  {
    w = lfunrootno_bitprec(data, bitprec);
    if (!r)
      r = R = gen_0;
    else
      R = lfunrtoR_eno(ldata, w, nbits2prec(bitprec));
    return gerepilecopy(ltop, mkvec3(r, R, w));
  }
  linit = lfunthetacheckinit(data, dbltor(sqrt(0.5)), 0, &bitprec, 1);
  prec = nbits2prec(bitprec);
  if (lg(r) > 2) pari_err_IMPL("multiple poles in lfunrootres");
  /* Now residue unknown, and r = [[be,0]]. */
  be = gmael(r, 1, 1);
  w = ldata_get_rootno(ldata);
  if (ldata_isreal(ldata) && gequalm1(w))
  {
    GEN R = lfuntheta_bitprec(linit, gen_1, 0, bitprec);
    r = Rtor(be, R, ldata, prec);
    return gerepilecopy(ltop, mkvec3(r, R, gen_m1));
  }
  lfunthetaspec_bitprec(linit, bitprec, &v2, &v);
  if (gequalgs(gmulsg(2, be), k)) pari_err_IMPL("pole at k/2 in lfunrootres");
  if (gequalgs(be, k))
  {
    GEN p2k = int2n(k);
    a = gconj(gsub(gmul(p2k, v), v2));
    b = subis(p2k, 1);
    e = gmul(gsqrt(p2k, prec), gsub(v2, v));
  }
  else
  {
    tk2 = gsqrt(int2n(k), prec);
    tbe = gpow(gen_2, be, prec);
    tkbe = gpow(gen_2, gdivgs(gsubsg(k, be), 2), prec);
    a = gconj(gsub(gmul(tbe, v), v2));
    b = gsub(gdiv(tbe, tkbe), tkbe);
    e = gsub(gmul(gdiv(tbe, tk2), v2), gmul(tk2, v));
  }
  if (!gequal0(w)) R = gdiv(gsub(e, gmul(a, w)), b);
  else
  { /* Now residue unknown, r = [[be,0]], and w unknown. */
    t0  = mkfrac(stoi(11),stoi(10));
    th1 = lfuntheta_bitprec(linit, t0,  0, bitprec);
    th2 = lfuntheta_bitprec(linit, ginv(t0), 0, bitprec);
    tbe = gpow(t0, gmulsg(2, be), prec);
    tkbe = gpow(t0, gsubsg(k, be), prec);
    tk2 = gpowgs(t0, k);
    c = gconj(gsub(gmul(tbe, th1), th2));
    d = gsub(gdiv(tbe, tkbe), tkbe);
    f = gsub(gmul(gdiv(tbe, tk2), th2), gmul(tk2, th1));
    dete = gsub(gmul(a, d), gmul(b, c));
    w = gdiv(gsub(gmul(d, e), gmul(b, f)), dete);
    R = gdiv(gsub(gmul(a, f), gmul(c, e)), dete);
  }
  r = Rtor(be, R, ldata, prec);
  return gerepilecopy(ltop, mkvec3(r, R, ropm1(w, prec)));
}

GEN
lfunrootres(GEN data, long prec)
{ return lfunrootres_bitprec(data, prec2nbits(prec)); }

/*******************************************************************/
/*                           Zeros                                 */
/*******************************************************************/
struct lhardyz_t {
  long bitprec, prec;
  GEN linit;
};

static GEN
lfunhardyzeros(void *E, GEN t)
{
  struct lhardyz_t *S = (struct lhardyz_t*)E;
  long prec = S->prec;
  GEN h = lfunhardy_bitprec(S->linit, t, S->bitprec);
  if (typ(h) == t_REAL && realprec(h) < prec) h = gprec_w(h, prec);
  return h;
}

static GEN
lfuncenterinit_bitprec(GEN lmisc, double h, long bitprec)
{
  GEN ldata = lfunmisc_to_ldata_shallow(lmisc);
  long k = ldata_get_k(ldata);
  GEN dom = mkvec3(dbltor(k/2.), gen_0, dbltor(h));
  if (is_linit(lmisc) && linit_get_type(lmisc) == t_LDESC_INIT)
  {
    GEN tech = linit_get_tech(lmisc);
    if (sdomain_isincl(dom, lfun_get_dom(tech))) return lmisc;
  }
  /* initialize for zero of order <= 4 */
  return lfuninit_bitprec(ldata, dom, 4, bitprec);
}

long
lfunorderzero_bitprec(GEN lmisc, long bitprec)
{
  pari_sp ltop = avma;
  GEN eno, ldata, linit, k2;
  long G, c0, c, st, k;

  if (is_linit(lmisc) && linit_get_type(lmisc) == t_LDESC_PRODUCT)
  {
    GEN M = gmael(linit_get_tech(lmisc), 2,1);
    long i;
    for (c=0,i=1; i < lg(M); i++) c += lfunorderzero_bitprec(gel(M,i), bitprec);
    return c;
  }
  linit = lfuncenterinit_bitprec(lmisc, 0, bitprec);
  ldata = linit_get_ldata(linit);
  eno = ldata_get_rootno(ldata);
  G = -bitprec/2;
  c0 = 0; st = 1;
  if (!ldata_get_selfdual(ldata))
  {
    if (!gequal1(eno)) c0 = 1;
    st = 2;
  }
  k = ldata_get_k(ldata);
  k2 = gdivgs(stoi(k), 2);
  for (c = c0;; c += st)
    if (gexpo(lfun0_bitprec(linit, k2, c, bitprec)) > G) break;
  avma = ltop; return c;
}

long
lfunorderzero(GEN ldata, long prec)
{ return lfunorderzero_bitprec(ldata, prec2nbits(prec)); }

GEN
lfunzeros_bitprec(GEN ldata, GEN lim, long divz, long bitprec)
{
  pari_sp ltop = avma;
  GEN ldataf, linit, N, pi2, cN, pi2div, w, T, Vga, h1, h2;
  long i, d, W, NEWD, precinit, ct, s, prec = nbits2prec(bitprec);
  double maxt;
  struct lhardyz_t S;

  h1 = gen_0;
  if (typ(lim) == t_VEC)
  {
    if (lg(lim) != 3) pari_err_TYPE("lfunzeros",lim);
    h1 = gel(lim,1);
    h2 = gel(lim,2);
    maxt = maxdd(fabs(gtodouble(h1)), fabs(gtodouble(h2)));
  }
  else
  {
    h2 = lim;
    maxt = fabs(gtodouble(h2));
  }
  if (!is_rational_t(typ(h1))) h1 = bestappr(h1, int2n(64));
  if (!is_rational_t(typ(h2))) h2 = bestappr(h2, int2n(64));

  if (is_linit(ldata) && linit_get_type(ldata) == t_LDESC_PRODUCT)
  {
    GEN v, M = gmael(linit_get_tech(ldata), 2,1);
    long l = lg(M);
    v = cgetg(l, t_VEC);
    for (i = 1; i < l; i++) v = lfunzeros_bitprec(gel(M,i), lim, divz, bitprec);
    return gerepileupto(ltop, vecsort0(shallowconcat1(v), NULL, 0));
  }
  S.linit = linit = lfuncenterinit_bitprec(ldata, maxt, bitprec);
  S.bitprec = bitprec;
  S.prec = prec;
  ldataf = linit_get_ldata(linit);
  Vga = ldata_get_gammavec(ldataf); d = lg(Vga) - 1;
  N = ldata_get_conductor(ldataf);
  NEWD = minss((long) ceil(bitprec+(M_PI/(4*LOG2))*d*maxt),
               lfun_get_bitprec(linit_get_tech(linit)));
  precinit = prec; prec = nbits2prec(NEWD);
  pi2 = Pi2n(1, prec);
  cN = gdiv(N, gpowgs(Pi2n(-1, prec), d));
  cN = gexpo(cN) >= 0? gaddsg(d, gmulsg(2, glog(cN, prec))): stoi(d);
  pi2div = gdivgs(pi2, labs(divz));
  ct = 0;
  T = h1;
  if (gequal0(h1))
  {
    GEN r = ldata_get_residue(ldataf);
    if (!r || gequal0(r))
    {
      ct = lfunorderzero(linit, precinit);
      if (ct) T = real2n(-prec2nbits(prec)/(2*ct), prec);
    }
  }
  /* initialize for 100 further zeros, double later if needed */
  W = 100 + ct; w = cgetg(W+1,t_VEC);
  for (i=1; i<=ct; i++) gel(w,i) = gen_0;
  s = gsigne(lfunhardyzeros(&S, T));
  while (gcmpgs(T, maxt) < 0)
  {
    pari_sp av = avma;
    GEN T0 = T, z;
    for(;;)
    {
      long s0;
      GEN L;
      if (gcmp(T, pi2) >= 0)
        L = gadd(cN, gmulsg(d, glog(gdiv(T, pi2), prec)));
      else
        L = cN;
      T = gadd(T, gdiv(pi2div, L));
      if (gcmpgs(T, maxt) > 0) goto END;
      s0 = gsigne(lfunhardyzeros(&S, T));
      if (s0 != s) { s = s0; break; }
    }
    T = gerepileupto(av, T);
    z = zbrent(&S, lfunhardyzeros, T0, T, prec);
    if (gcmpgs(z, maxt) > 0) break;
    if (typ(z) == t_REAL) z  = rtor(z, precinit);
    /* room for twice as many zeros */
    if (ct >= W) { W *= 2; w = vec_lengthen(w, W); }
    gel(w, ++ct) = z;
  }
END:
  setlg(w, ct+1); return gerepilecopy(ltop, w);
}

GEN
lfunzeros(GEN ldata, GEN lim, long divz, long prec)
{ return lfunzeros_bitprec(ldata, lim, divz, prec2nbits(prec)); }

/*******************************************************************/
/*       Guess conductor                                           */
/*******************************************************************/
struct huntcond_t {
  long k;
  GEN data;
  GEN *pM, *psqrtM;
};

/* M should eventually converge to N, the conductor. L has no pole. */
static GEN
wrap1(void *E, GEN M)
{
  struct huntcond_t *S = (struct huntcond_t*)E;
  GEN data = S->data, thetainit, tk, p1, p1inv;
  GEN t = mkfrac(stoi(11), stoi(10));
  long prec, bitprec;

  thetainit = linit_get_tech(data);
  bitprec = theta_get_bitprec(thetainit);
  prec = nbits2prec(bitprec);
  *(S->pM) = M;
  *(S->psqrtM) = gsqrt(M, prec);

  tk = gpowgs(t, S->k);
  p1 = lfuntheta_bitprec(data, t, 0, bitprec);
  p1inv = lfuntheta_bitprec(data, ginv(t), 0, bitprec);
  return glog(gabs(gmul(tk, gdiv(p1, p1inv)), prec), prec);
}

/* M should eventually converge to N, the conductor. L has a pole. */
static GEN
wrap2(void *E, GEN M)
{
  struct huntcond_t *S = (struct huntcond_t*)E;
  GEN data = S->data, t1k, t2k, p1, p1inv, p2, p2inv;
  GEN thetainit, R;
  GEN t1 = mkfrac(stoi(11), stoi(10)), t2 = mkfrac(stoi(13), stoi(11));
  GEN t1be, t2be, t1bemk, t2bemk, t1kmbe, t2kmbe;
  GEN F11, F12, F21, F22, P1, P2, res;
  long k = S->k, prec, bitprec;

  thetainit = linit_get_tech(data);
  bitprec = theta_get_bitprec(thetainit);
  prec = nbits2prec(bitprec);
  *(S->pM) = M;
  *(S->psqrtM) = gsqrt(M, prec);

  p1 = lfuntheta_bitprec(data, t1, 0, bitprec);
  p2 = lfuntheta_bitprec(data, t2, 0, bitprec);
  p1inv = lfuntheta_bitprec(data, ginv(t1), 0, bitprec);
  p2inv = lfuntheta_bitprec(data, ginv(t2), 0, bitprec);
  t1k = gpowgs(t1, k);
  t2k = gpowgs(t2, k);
  R = theta_get_R(thetainit);
  if (typ(R) == t_VEC)
  {
    GEN be = gmael(R, 1, 1);
    t1be = gpow(t1, be, prec); t1bemk = gdiv(gsqr(t1be), t1k);
    t2be = gpow(t2, be, prec); t2bemk = gdiv(gsqr(t2be), t2k);
    t1kmbe = gdiv(t1k, t1be);
    t2kmbe = gdiv(t2k, t2be);
  }
  else
  { /* be = k */
    t1be = t1k; t1bemk = t1k; t1kmbe = gen_1;
    t2be = t2k; t2bemk = t2k; t2kmbe = gen_1;
  }
  F11 = gconj(gsub(gmul(gsqr(t1be), p1), p1inv));
  F12 = gconj(gsub(gmul(gsqr(t2be), p2), p2inv));
  F21 = gsub(gmul(t1k, p1), gmul(t1bemk, p1inv));
  F22 = gsub(gmul(t2k, p2), gmul(t2bemk, p2inv));
  P1 = gsub(gmul(t1bemk, t1be), t1kmbe);
  P2 = gsub(gmul(t2bemk, t2be), t2kmbe);
  res = gdiv(gsub(gmul(P2,F21), gmul(P1,F22)),
             gsub(gmul(P2,F11), gmul(P1,F12)));
  return glog(gabs(res, prec), prec);
}

/* If flag = 0 (default) return all conductors found as integers. If
flag = 1, return the approximations, not the integers. If flag = 2,
return all, even nonintegers. */

static GEN
checkconductor(GEN v, long flag)
{
  GEN w;
  long e, j, k, l = lg(v);
  if (flag == 2) return v;
  w = cgetg(l, t_VEC);
  for (j = k = 1; j < l; j++)
  {
    GEN N = grndtoi(gel(v,j), &e);
    if (e < -13) gel(w,k++) = flag ? gel(v,j): N;
  }
  if (k == 2) return gel(w,1);
  setlg(w,k); return w;
}

static void
parse_maxcond(GEN maxcond, GEN *pm, GEN *pM)
{
  GEN m = gen_1, M;
  if (!maxcond)
    M = utoipos(10000);
  else if (typ(maxcond) == t_VEC)
  {
    if (lg(maxcond) != 3) pari_err_TYPE("lfunconductor", maxcond);
    m = gel(maxcond,1);
    M = gel(maxcond,2);
  }
  else
    M = maxcond;
  m = (typ(m) == t_INT)? gsub(m,ghalf): gfloor(m);
  if (signe(m) <= 0) m = ghalf;
  M = (typ(M) == t_INT)? addiu(M, 1): gceil(M);
  *pm = m;
  *pM = M;
}

GEN
lfunconductor(GEN data, GEN maxcond, long flag, long prec)
{
  struct huntcond_t S;
  pari_sp ltop = avma;
  long bitprec = 3*prec2nbits(prec)/2;
  GEN ld, r, v, ldata, theta, m, M, tdom;
  GEN (*eval)(void *, GEN);

  ldata = lfunmisc_to_ldata_shallow(data);
  parse_maxcond(maxcond, &m,&M);
  r = ldata_get_residue(ldata);
  if (r && typ(r) == t_VEC)
  {
    if (lg(r) > 2) pari_err_IMPL("multiple poles in lfunconductor");
    r = gmael(r,1,2);
  }
  if (!r)
  { eval = wrap1; tdom = mkfrac(stoi(10), stoi(11)); }
  else
  { eval = wrap2; tdom = mkfrac(stoi(11), stoi(13)); }
  ld = shallowcopy(ldata);
  gel(ld, 5) = M;
  theta = lfunthetainit_bitprec(ld, tdom, 0, bitprec);
  gel(theta,3) = shallowcopy(linit_get_tech(theta));
  S.k = ldata_get_k(ldata);
  S.data = theta;
  S.pM = &gel(linit_get_ldata(theta),5);
  S.psqrtM = &gel(linit_get_tech(theta),7);
  v = solvestep((void*)&S, eval, m, M, gen_2, 14, nbits2prec(bitprec));
  return gerepilecopy(ltop, checkconductor(v, flag));
}