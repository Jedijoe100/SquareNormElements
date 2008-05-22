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
/*               S-CLASS GROUP AND NORM SYMBOLS                    */
/*          (Denis Simon, desimon@math.u-bordeaux.fr)              */
/*                                                                 */
/*******************************************************************/
#include "pari.h"
#include "paripriv.h"

/* p > 2 */
static long
lemma6(GEN pol,GEN p,long nu,GEN x)
{
  long la, mu;
  pari_sp av = avma;
  GEN gpx, gx = poleval(pol, x);

  if (Zp_issquare(gx,p)) { avma = av; return 1; }

  la = Z_pval(gx, p);
  gpx = poleval(ZX_deriv(pol), x);
  mu = signe(gpx)? Z_pval(gpx,p)
                 : la+nu+1; /* mu = +oo */
  avma = av;
  if (la > mu<<1) return 1;
  if (la >= nu<<1 && mu >= nu) return 0;
  return -1;
}
/* p = 2: retur 1 = yes, -1 = no, 0 = inconclusive */
static long
lemma7(GEN pol,long nu,GEN x)
{
  long odd4, la, mu;
  pari_sp av = avma;
  GEN gpx, oddgx, gx = poleval(pol, x);

  if (Zp_issquare(gx,gen_2)) return 1;

  gpx = poleval(ZX_deriv(pol), x);
  la = Z_lvalrem(gx, 2, &oddgx);
  odd4 = umodiu(oddgx,4); avma = av;

  mu = vali(gpx);
  if (mu < 0) mu = la+nu+1; /* mu = +oo */

  if (la > mu<<1) return 1;
  if (nu > mu)
  {
    long mnl = mu+nu-la;
    if (odd(la)) return -1;
    if (mnl==1) return 1;
    if (mnl==2 && odd4==1) return 1;
  }
  else
  {
    long nu2 = nu << 1;
    if (la >= nu2) return 0;
    if (la == nu2 - 2 && odd4==1) return 0;
  }
  return -1;
}

static long
zpsol(GEN pol,GEN p,long nu,GEN pnu,GEN x0)
{
  long i, res;
  pari_sp av = avma;
  GEN x,pnup;

  res = equaliu(p,2)? lemma7(pol,nu,x0): lemma6(pol,p,nu,x0);
  if (res== 1) return 1;
  if (res==-1) return 0;
  x = x0; pnup = mulii(pnu,p);
  for (i=0; i < itos(p); i++)
  {
    x = addii(x,pnu);
    if (zpsol(pol,p,nu+1,pnup,x)) { avma=av; return 1; }
  }
  avma=av; return 0;
}

/* return 1 if equation y^2=T(x) has an integral p-adic solution, 0
 * otherwise. */
long
zpsoluble(GEN T,GEN p)
{
  if (typ(T)!=t_POL || typ(p)!=t_INT) pari_err(typeer,"zpsoluble");
  RgX_check_ZX(T, "zpsoluble");
  return zpsol(T,p,0,gen_1,gen_0);
}

/* return 1 if equation y^2=T(x) has a rational p-adic solution (possibly
 * infinite), 0 otherwise. */
long
qpsoluble(GEN T,GEN p)
{
  pari_sp av = avma;
  long res = zpsoluble(T,p) || zpsol(polrecip_i(T),p,1,p,gen_0);
  avma = av; return res;
}

/* is t a square in (O_K/pr), assume v_pr(t) >= 0 ? */
static long
quad_char(GEN nf, GEN t, GEN pr)
{
  GEN ord, ordp, T, p, modpr = nf_to_Fq_init(nf, &pr,&T,&p);
  t = nf_to_Fq(nf,t,modpr);
  if (T)
  {
    ord = subis( pr_norm(pr), 1 ); /* |(O_K / pr)^*| */
    ordp= subis( p, 1);            /* |F_p^*|        */
    t = Fq_pow(t, diviiexact(ord, ordp), T,p); /* in F_p^* */
    if (typ(t) == t_POL)
    {
      if (degpol(t)) pari_err(bugparier,"nfhilbertp");
      t = constant_term(t);
    }
  }
  return kronecker(t, p);
}

/* (pr,2) = 1. return 1 if a square in (ZK / pr), 0 otherwise */
static long
psquarenf(GEN nf,GEN a,GEN pr)
{
  pari_sp av = avma;
  long v;

  if (gcmp0(a)) return 1;
  v = idealval(nf,a,pr); if (v&1) return 0;
  if (v) a = gdiv(a, gpowgs(coltoalg(nf, gel(pr,2)), v));

  v = quad_char(nf, a, pr); avma = av; return v;
}

static long
check2(GEN nf, GEN a, GEN zinit)
{
  GEN zlog = zideallog(nf,a,zinit), cyc = gmael(zinit,2,2);
  long i, l = lg(cyc);

  for (i=1; i<l; i++)
  {
    if (mpodd(gel(cyc,i))) break;
    if (mpodd(gel(zlog,i))) return 0;
  }
  return 1;
}

/* pr | 2. Return 1 if a square in (ZK / pr), 0 otherwise */
static long
psquare2nf(GEN nf,GEN a,GEN pr,GEN zinit)
{
  long v;
  pari_sp av = avma;

  if (gcmp0(a)) return 1;
  v = idealval(nf,a,pr); if (v&1) return 0;
  if (v) a = gdiv(a, gpowgs(coltoalg(nf, gel(pr,2)), v));
  /* now (a,pr) = 1 */
  v = check2(nf,a,zinit); avma = av; return v;
}

static long
lemma6nf(GEN nf,GEN pol,GEN pr,long nu,GEN x)
{
  pari_sp av = avma;
  long la, mu;
  GEN gpx, gx = poleval(pol, x);

  if (psquarenf(nf,gx,pr)) return 1;

  la = element_val(nf,gx,pr);
  gpx = poleval(RgX_deriv(pol), x);
  mu = gcmp0(gpx)? la+nu+1: idealval(nf,gpx,pr);
  avma = av;
  if (la > mu<<1) return 1;
  if (la >= nu<<1  && mu >= nu) return 0;
  return -1;
}

static long
lemma7nf(GEN nf,GEN pol,GEN pr,long nu,GEN x,GEN zinit)
{
  long res, la, mu, q;
  GEN gpx, p1, gx = poleval(pol, x);

  if (psquare2nf(nf,gx,pr,zinit)) return 1;

  gpx = poleval(RgX_deriv(pol), x);
  la = element_val(nf,gx,pr);
  mu = gcmp0(gpx)? la+nu+1: idealval(nf,gpx,pr);

  if (la > mu<<1) return 1;
  if (nu > mu)
  {
    if (la&1) return -1;
    q = mu+nu-la; res = 1;
  }
  else
  {
    long nu2 = nu<<1;
    if (la >= nu2) return 0;
    if (odd(la)) return -1;
    q = nu2-la; res = 0;
  }
  if (q > itos(gel(pr,3))<<1)  return -1;
  p1 = gpowgs(coltoalg(nf, gel(pr,2)), la);

  zinit = Idealstar(nf, idealpows(nf,pr,q), nf_INIT);
  if (!check2(nf,gdiv(gx,p1),zinit)) res = -1;
  return res;
}

static long
zpsolnf(GEN nf,GEN pol,GEN pr,long nu,GEN pnu,GEN x0,GEN repr,GEN zinit)
{
  long i, res;
  pari_sp av = avma;
  GEN pnup;

  res = zinit? lemma7nf(nf,pol,pr,nu,x0,zinit)
             : lemma6nf(nf,pol,pr,nu,x0);
  avma = av;
  if (res== 1) return 1;
  if (res==-1) return 0;
  pnup = gmul(pnu, coltoalg(nf,gel(pr,2)));
  nu++;
  for (i=1; i<lg(repr); i++)
  {
    GEN x = gadd(x0,gmul(pnu,gel(repr,i)));
    if (zpsolnf(nf,pol,pr,nu,pnup,x,repr,zinit)) { avma = av; return 1; }
  }
  avma = av; return 0;
}

/* system of representatives for Zk/pr */
static GEN
repres(GEN nf, GEN pr)
{
  long i, j, k, f = itos(gel(pr,4)), pp, ppf, ppi;
  GEN fond, rep, bas = gel(nf,7);

  fond = cgetg(f+1, t_VEC);
  gel(fond,1) = gel(bas,1);
  if (f > 1) {
    GEN mat = prime_to_ideal(nf,pr);
    for (i = k = 2; k <= f; i++)
      if (!is_pm1(gmael(mat,i,i))) gel(fond,k++) = gel(bas,i);
  }

  pp = itos(gel(pr,1));
  ppf = upowuu(pp, f);
  rep = cgetg(ppf+1,t_VEC);
  gel(rep,1) = gen_0; ppi=1;
  for (i=0; i<f; i++,ppi*=pp)
    for (j=1; j<pp; j++)
      for (k=1; k<=ppi; k++)
	gel(rep, j*ppi+k) = gadd(gel(rep,k),gmulsg(j,gel(fond,i+1)));
  return gmodulo(rep,gel(nf,1));
}

/* = 1 if equation y^2 = z^deg(T) * T(x/z) has a pr-adic rational solution
 * (possibly (1,y,0) = oo), 0 otherwise.
 * coeffs of T are algebraic integers in nf */
long
qpsolublenf(GEN nf,GEN T,GEN pr)
{
  GEN repr, zinit, p1;
  pari_sp av = avma;

  if (gcmp0(T)) return 1;
  if (typ(T)!=t_POL) pari_err(notpoler,"qpsolublenf");
  checkprimeid(pr); nf = checknf(nf);

  if (equaliu(gel(pr,1), 2))
  { /* tough case */
    zinit = Idealstar(nf, idealpows(nf,pr,1+2*idealval(nf,gen_2,pr)), nf_INIT);
    if (psquare2nf(nf,constant_term(T),pr,zinit)) return 1;
    if (psquare2nf(nf, leading_term(T),pr,zinit)) return 1;
  }
  else
  {
    if (psquarenf(nf,constant_term(T),pr)) return 1;
    if (psquarenf(nf, leading_term(T),pr)) return 1;
    zinit = NULL;
  }
  repr = repres(nf,pr);
  if (zpsolnf(nf,T,pr,0,gen_1,gen_0,repr,zinit)) { avma=av; return 1; }
  p1 = coltoalg(nf, gel(pr,2));
  if (zpsolnf(nf,polrecip_i(T),pr,1,p1,gen_0,repr,zinit)) { avma=av; return 1; }

  avma = av; return 0;
}

/* = 1 if equation y^2 = z^deg(T) * T(x/z) has a pr-adic integral solution
 * (possibly (1,y,0) = oo), 0 otherwise.
 * coeffs of T are algebraic integers in nf */
long
zpsolublenf(GEN nf,GEN T,GEN pr)
{
  GEN repr,zinit;
  pari_sp av = avma;

  if (gcmp0(T)) return 1;
  if (typ(T)!=t_POL) pari_err(notpoler,"zpsolublenf");
  checkprimeid(pr); nf = checknf(nf);

  if (equaliu(gel(pr,1),2))
  {
    zinit = Idealstar(nf,idealpows(nf,pr,1+2*idealval(nf,gen_2,pr)), nf_INIT);
    if (psquare2nf(nf,constant_term(T),pr,zinit)) return 1;
  }
  else
  {
    if (psquarenf(nf,constant_term(T),pr)) return 1;
    zinit = NULL;
  }
  repr = repres(nf,pr);
  if (zpsolnf(nf,T,pr,0,gen_1,gen_0,repr,zinit)) { avma=av; return 1; }
  avma=av; return 0;
}

static long
hilb2nf(GEN nf,GEN a,GEN b,GEN p)
{
  pari_sp av = avma;
  long rep;
  GEN pol;

  a = basistoalg_i(nf, a);
  b = basistoalg_i(nf, b);
  pol = mkpoln(3, a, gen_0, b);
  /* varn(nf.pol) = 0, pol is not a valid GEN  [as in Pol([x,x], x)].
   * But it is only used as a placeholder, hence it is not a problem */

  rep = qpsolublenf(nf,pol,p)? 1: -1;
  avma = av; return rep;
}

/* local quadratic Hilbert symbol (a,b)_pr, for a,b (non-zero) in nf */
long
nfhilbertp(GEN nf,GEN a,GEN b,GEN pr)
{
  GEN p, t;
  long va, vb, rep;
  pari_sp av = avma;

  if (gcmp0(a) || gcmp0(b)) pari_err (talker,"0 argument in nfhilbertp");
  checkprimeid(pr); nf = checknf(nf);
  p = gel(pr,1);

  if (equaliu(p,2)) return hilb2nf(nf,a,b,pr);

  /* pr not above 2, compute t = tame symbol */
  va = idealval(nf,a,pr);
  vb = idealval(nf,b,pr);
  if (!odd(va) && !odd(vb)) { avma = av; return 1; }
  t = element_div(nf, element_pow(nf,a, stoi(vb)),
		      element_pow(nf,b, stoi(va)));
  if (odd(va) && odd(vb)) t = gneg_i(t); /* t mod pr = tame_pr(a,b) */

  /* quad. symbol is image of t by the quadratic character  */
  rep = quad_char(nf, t, pr);
  avma = av; return rep;
}

/* global quadratic Hilbert symbol (a,b):
 *  =  1 if X^2 - aY^2 - bZ^2 has a point in projective plane
 *  = -1 otherwise
 * a, b should be non-zero
 */
long
nfhilbert(GEN nf,GEN a,GEN b)
{
  pari_sp av = avma;
  long r1, i;
  GEN S, ro;

  if (gcmp0(a) || gcmp0(b)) pari_err (talker,"0 argument in nfhilbert");
  nf = checknf(nf);

  a = basistoalg_i(nf, a);
  b = basistoalg_i(nf, b);
 /* local solutions in real completions ? */
  r1 = nf_get_r1(nf); ro = gel(nf,6);
  for (i=1; i<=r1; i++)
    if (signe(poleval(a,gel(ro,i))) < 0 &&
	signe(poleval(b,gel(ro,i))) < 0)
    {
      if (DEBUGLEVEL>=4)
	fprintferr("nfhilbert not soluble at real place %ld\n",i);
      avma = av; return -1;
    }

  /* local solutions in finite completions ? (pr | 2ab)
   * primes above 2 are toughest. Try the others first */

  S = gel(idealfactor(nf,gmod(gmul(gmulsg(2,a),b), gel(nf,1))), 1);
  /* product of all hilbertp is 1 ==> remove one prime (above 2!) */
  for (i=lg(S)-1; i>1; i--)
    if (nfhilbertp(nf,a,b,gel(S,i)) < 0)
    {
      if (DEBUGLEVEL >=4)
	fprintferr("nfhilbert not soluble at finite place: %Zs\n",S[i]);
      avma = av; return -1;
    }
  avma = av; return 1;
}

long
nfhilbert0(GEN nf,GEN a,GEN b,GEN p)
{
  if (p) return nfhilbertp(nf,a,b,p);
  return nfhilbert(nf,a,b);
}

/* S a list of prime ideal in primedec format. Return res:
 * res[1] = generators of (S-units / units), as polynomials
 * res[2] = [perm, HB, den], for bnfissunit
 * res[3] = [] (was: log. embeddings of res[1])
 * res[4] = S-regulator ( = R * det(res[2]) * \prod log(Norm(S[i])))
 * res[5] = S class group
 * res[6] = S */
GEN
bnfsunit(GEN bnf,GEN S,long prec)
{
  pari_sp av = avma;
  long i,j,ls;
  GEN p1,nf,classgp,gen,M,U,H;
  GEN sunit,card,sreg,res,pow;

  if (typ(S) != t_VEC) pari_err(typeer,"bnfsunit");
  bnf = checkbnf(bnf); nf=gel(bnf,7);
  classgp=gmael(bnf,8,1);
  gen = gel(classgp,3);

  sreg = gmael(bnf,8,2);
  res=cgetg(7,t_VEC);
  gel(res,1) = gel(res,2) = gel(res,3) = cgetg(1,t_VEC);
  gel(res,4) = sreg;
  gel(res,5) = classgp;
  gel(res,6) = S; ls=lg(S);

 /* M = relation matrix for the S class group (in terms of the class group
  * generators given by gen)
  * 1) ideals in S
  */
  M = cgetg(ls,t_MAT);
  for (i=1; i<ls; i++)
  {
    p1 = gel(S,i); checkprimeid(p1);
    gel(M,i) = isprincipal(bnf,p1);
  }
  /* 2) relations from bnf class group */
  M = shallowconcat(M, diagonal_i(gel(classgp,2)));

  /* S class group */
  H = ZM_hnfall(M, &U, 1);
  card = gen_1;
  if (lg(H) > 1)
  { /* non trivial (rare!) */
    GEN u, D = ZM_snfall_i(H, &u, NULL, 1);
    card = detcyc(D, &i);
    setlg(D,i);
    p1=cgetg(i,t_VEC); pow=ZM_inv(u,gen_1);
    for(i--; i; i--)
      gel(p1,i) = factorback_i(gen, gel(pow,i), nf, 1);
    gel(res,5) = mkvec3(card,D,p1);
  }

  /* S-units */
  if (ls>1)
  {
    GEN den, Sperm, perm, dep, B, A, U1 = U;
    long lH, lB, fl = nf_GEN|nf_FORCE;

   /* U1 = upper left corner of U, invertible. S * U1 = principal ideals
    * whose generators generate the S-units */
    setlg(U1,ls); p1 = cgetg(ls, t_MAT); /* p1 is junk for mathnfspec */
    for (i=1; i<ls; i++) { setlg(U1[i],ls); gel(p1,i) = cgetg(1,t_COL); }
    H = mathnfspec(U1,&perm,&dep,&B,&p1);
    lH = lg(H);
    lB = lg(B);
    if (lg(dep) > 1 && lg(dep[1]) > 1) pari_err(bugparier,"bnfsunit");
   /*                   [ H B  ]            [ H^-1   - H^-1 B ]
    * perm o HNF(U1) =  [ 0 Id ], inverse = [  0         Id   ]
    * (permute the rows)
    * S * HNF(U1) = _integral_ generators for S-units  = sunit */
    Sperm = cgetg(ls, t_VEC); sunit = cgetg(ls, t_VEC);
    for (i=1; i<ls; i++) Sperm[i] = S[perm[i]]; /* S o perm */

    setlg(Sperm, lH);
    for (i=1; i<lH; i++)
    {
      GEN v = isprincipalfact(bnf,Sperm,gel(H,i),NULL,fl);
      gel(sunit,i) = coltoliftalg(nf, gel(v,2));
    }
    for (j=1; j<lB; j++,i++)
    {
      GEN v = isprincipalfact(bnf,Sperm,gel(B,j),gel(Sperm,i),fl);
      gel(sunit,i) = coltoliftalg(nf, gel(v,2));
   }
    den = ZM_det_triangular(H); H = ZM_inv(H,den);
    A = shallowconcat(H, ZM_neg(ZM_mul(H,B))); /* top part of inverse * den */
    /* HNF in split form perm + (H B) [0 Id missing] */
    gel(res,1) = sunit;
    gel(res,2) = mkvec3(perm,A,den);
  }

  /* S-regulator */
  sreg = mpmul(sreg,card);
  for (i=1; i<ls; i++)
  {
    GEN p = gel(S,i);
    if (typ(p) == t_VEC) p = gel(p,1);
    sreg = mpmul(sreg,glog(p,prec));
  }
  gel(res,4) = sreg;
  return gerepilecopy(av,res);
}

static GEN
make_unit(GEN nf, GEN bnfS, GEN *px)
{
  long lB, cH, i, ls;
  GEN den, gen, S, v, p1, xp, xb, N, HB, perm;

  if (gcmp0(*px)) return NULL;
  S = gel(bnfS,6); ls = lg(S);
  if (ls==1) return cgetg(1, t_COL);

  xb = nf_to_scalar_or_basis(nf,*px);
  switch(typ(xb))
  {
    case t_INT:  N = xb; break;
    case t_FRAC: N = mulii(gel(xb,1),gel(xb,2)); break;
    default: { GEN d = Q_denom(xb); N = mulii(idealnorm(nf,gmul(*px,d)), d); }
  } /* relevant primes divide N */
  if (is_pm1(N)) return zerocol(ls -1);

  p1 = gel(bnfS,2);
  perm = gel(p1,1);
  HB   = gel(p1,2);
  den  = gel(p1,3);
  cH = lg(HB[1]) - 1;
  lB = lg(HB) - cH;
  v = cgetg(ls, t_VECSMALL);
  for (i=1; i<ls; i++)
  {
    GEN P = gel(S,i);
    v[i] = (remii(N, gel(P,1)) == gen_0)? element_val(nf,xb,P): 0;
  }
  /* here, x = S v */
  p1 = vecpermute(v, perm);
  v = ZM_zc_mul(HB, p1);
  for (i=1; i<=cH; i++)
  {
    GEN r, w = dvmdii(gel(v,i), den, &r);
    if (r != gen_0) return NULL;
    gel(v,i) = w;
  }
  p1 += cH; p1[0] = evaltyp(t_VECSMALL) | evallg(lB);
  v = shallowconcat(v, zc_to_ZC(p1)); /* append bottom of p1 (= [0 Id] part) */

  gen = gel(bnfS,1);
  xp = cgetg(1, t_MAT);
  for (i=1; i<ls; i++)
  {
    GEN e = gel(v,i);
    if (!signe(e)) continue;
    xp = famat_mul(xp, to_famat_all(gel(gen,i), negi(e)));
  }
  if (lg(xp) > 1) *px = famat_mul(xp, to_famat_all(xb, gen_1));
  return v;
}

/* Analog to isunit, for S-units. Let v the result
 * If x not an S-unit, v = []~, else
 * x = \prod_{i=0}^r e_i^v[i] * prod{i=r+1}^{r+s} s_i^v[i]
 * where the e_i are the field units (cf isunit), and the s_i are
 * the S-units computed by bnfsunit (in the same order) */
GEN
bnfissunit(GEN bnf,GEN bnfS,GEN x)
{
  pari_sp av = avma;
  GEN v, w, nf;

  bnf = checkbnf(bnf);
  nf = checknf(bnf);
  if (typ(bnfS)!=t_VEC || lg(bnfS)!=7) pari_err(typeer,"bnfissunit");
  x = basistoalg_i(nf,x);
  v = NULL;
  if ( (w = make_unit(nf, bnfS, &x)) ) v = isunit(bnf, x);
  if (!v || lg(v) == 1) { avma = av; return cgetg(1,t_COL); }
  return gerepileupto(av, concat(v, w));
}

static void
pr_append(GEN nf, GEN rel, GEN p, GEN *prod, GEN *S1, GEN *S2)
{
  if (dvdii(*prod, p)) return;
  *prod = mulii(*prod, p);
  *S1 = shallowconcat(*S1, primedec(nf,p));
  *S2 = shallowconcat(*S2, primedec(rel,p));
}

static void
fa_pr_append(GEN nf,GEN rel,GEN N,GEN *prod,GEN *S1,GEN *S2)
{
  if (!is_pm1(N))
  {
    GEN v = gel(Z_factor(N),1);
    long i, l = lg(v);
    for (i=1; i<l; i++) pr_append(nf,rel,gel(v,i),prod,S1,S2);
  }
}

static GEN
pol_up(GEN rnfeq, GEN x, long v)
{
  long i, l = lg(x);
  GEN y = cgetg(l, t_POL); y[1] = x[1];
  for (i=2; i<l; i++)
  {
    GEN t = eltreltoabs(rnfeq, gel(x,i));
    if (typ(t) == t_POL) setvarn(t, v);
    gel(y,i) = t;
  }
  return y;
}

GEN
rnfisnorminit(GEN T, GEN relpol, int galois)
{
  pari_sp av = avma;
  long i, l, drel, vbas;
  GEN prod, S1, S2, gen, cyc, bnf, nf, nfabs, rnfeq, bnfabs, res, k, polabs;
  GEN y = cgetg(9, t_VEC);

  T = get_bnfpol(T, &bnf, &nf); vbas = varn(T);
  if (!bnf) bnf = bnfinit0(nf? nf: T, 1, NULL, DEFAULTPREC);
  if (!nf) nf = checknf(bnf);

  relpol = get_bnfpol(relpol, &bnfabs, &nfabs);
  if (!gcmp1(leading_term(relpol))) pari_err(impl,"non monic relative equation");
  drel = degpol(relpol);
  if (varncmp(varn(relpol), vbas) >= 0)
    pari_err(talker,"main variable must be of higher priority in rnfisnorminit");
  if (drel <= 2) galois = 1;

  rnfeq = NULL; /* no reltoabs needed */
  if (degpol(nf[1]) == 1) { /* over Q */
    polabs = lift(relpol); k = gen_0;
  } else if (galois == 2) { /* needs reltoabs */
    rnfeq = rnfequation2(bnf, relpol);
    polabs = gel(rnfeq,1);
    gel(rnfeq,2) = lift_intern(gel(rnfeq,2));
    k = gel(rnfeq,3);
  } else {
    long sk;
    polabs = rnfequation_i(bnf, relpol, &sk, NULL);
    k = stoi(sk);
  }
  if (!bnfabs || !gcmp0(k)) bnfabs = bnfinit0(polabs, 1, NULL, nf_get_prec(nf));
  if (!nfabs) nfabs = checknf(bnfabs);

  if (galois < 0 || galois > 2) pari_err(flagerr, "rnfisnorminit");
  if (galois == 2)
  {
    GEN P = rnfeq? pol_up(rnfeq, relpol, vbas): relpol;
    galois = nfissplit(gsubst(nfabs, varn(nfabs[1]), pol_x(vbas)), P);
  }

  prod = gen_1; S1 = S2 = cgetg(1, t_VEC);
  res = gmael(bnfabs,8,1);
  cyc = gel(res,2);
  gen = gel(res,3); l = lg(cyc);
  for(i=1; i<l; i++)
  {
    if (ugcd(umodiu(gel(cyc,i), drel), drel) == 1) break;
    fa_pr_append(nf,bnfabs,gmael3(gen,i,1,1),&prod,&S1,&S2);
  }
  if (!galois)
  {
    GEN Ndiscrel = diviiexact(gel(nfabs,3), powiu(gel(nf,3), drel));
    fa_pr_append(nf,bnfabs,absi(Ndiscrel), &prod,&S1,&S2);
  }

  gel(y,1) = bnf;
  gel(y,2) = bnfabs;
  gel(y,3) = relpol;
  gel(y,4) = get_theta_abstorel(T, relpol, k);
  gel(y,5) = prod;
  gel(y,6) = S1;
  gel(y,7) = S2;
  gel(y,8) = stoi(galois); return gerepilecopy(av, y);
}

/* T as output by rnfisnorminit
 * if flag=0 assume extension is Galois (==> answer is unconditional)
 * if flag>0 add to S all primes dividing p <= flag
 * if flag<0 add to S all primes dividing abs(flag)

 * answer is a vector v = [a,b] such that
 * x = N(a)*b and x is a norm iff b = 1  [assuming S large enough] */
GEN
rnfisnorm(GEN T, GEN x, long flag)
{
  pari_sp av = avma;
  GEN bnf, rel, relpol, theta, nfpol;
  GEN nf, aux, H, U, Y, M, A, bnfS, sunitrel, futu, tu, w, prod, S1, S2;
  long L, i, drel, itu;

  if (typ(T) != t_VEC || lg(T) != 9)
    pari_err(talker,"please apply rnfisnorminit first");
  bnf = gel(T,1); rel = gel(T,2); relpol = gel(T,3); theta = gel(T,4);
  drel = degpol(relpol);
  bnf = checkbnf(bnf);
  rel = checkbnf(rel);
  nf = checknf(bnf);
  x = basistoalg_i(nf,x);
  if (gcmp0(x)) { avma = av; return mkvec2(gen_0, gen_1); }
  if (gcmp1(x)) { avma = av; return mkvec2(gen_1, gen_1); }
  if (gcmp_1(x) && odd(drel)) { avma = av; return mkvec2(gen_m1, gen_1); }

  /* build set T of ideals involved in the solutions */
  nfpol = gel(nf,1);
  prod = gel(T,5);
  S1   = gel(T,6);
  S2   = gel(T,7);
  if (flag && !gcmp0(gel(T,8)))
    pari_warn(warner,"useless flag in rnfisnorm: the extension is Galois");
  if (flag > 0)
  {
    byteptr d = diffptr;
    long p = 0;
    maxprime_check((ulong)flag);
    for(;;)
    {
      NEXT_PRIME_VIADIFF(p, d);
      if (p > flag) break;
      pr_append(nf,rel, utoipos(p),&prod,&S1,&S2);
    }
  }
  else if (flag < 0)
    fa_pr_append(nf,rel,stoi(-flag),&prod,&S1,&S2);
  /* overkill: prime ideals dividing x would be enough */
  fa_pr_append(nf,rel,idealnorm(nf,x), &prod,&S1,&S2);

  /* computation on T-units */
  w  = gmael3(rel,8,4,1);
  tu = gmael3(rel,8,4,2);
  futu = shallowconcat(check_units(rel,"rnfisnorm"), tu);
  bnfS = bnfsunit(bnf,S1,3);
  sunitrel = shallowconcat(futu, gel(bnfsunit(rel,S2,3), 1));

  A = lift(bnfissunit(bnf,bnfS,x));
  L = lg(sunitrel);
  itu = lg(nf[6])-1; /* index of torsion unit in bnfsunit(nf) output */
  M = cgetg(L+1,t_MAT);
  for (i=1; i<L; i++)
  {
    GEN u = poleval(gel(sunitrel,i), theta); /* abstorel */
    if (typ(u) != t_POLMOD) u = mkpolmod(u, gel(theta,1));
    gel(sunitrel,i) = u;
    u = bnfissunit(bnf,bnfS, gnorm(u));
    if (lg(u) == 1) pari_err(bugparier,"rnfisnorm");
    gel(u,itu) = lift_intern(gel(u,itu)); /* lift root of 1 part */
    gel(M,i) = u;
  }
  aux = zerocol(lg(A)-1); gel(aux,itu) = w;
  gel(M,L) = aux;
  H = ZM_hnfall(M, &U, 2);
  Y = gmul(U, inverseimage(H,A));
  /* Y: sols of MY = A over Q */
  setlg(Y, L);
  aux = factorback(sunitrel, gfloor(Y));
  x = gdiv(mkpolmod(x,nfpol), gnorm(gmodulo(lift_intern(aux), relpol)));
  if (typ(x) == t_POLMOD && (typ(x[2]) != t_POL || !degpol(x[2])))
  {
    x = gel(x,2); /* rational number */
    if (typ(x) == t_POL) x = gel(x,2);
  }
  if (typ(aux) == t_POLMOD && degpol(nfpol) == 1)
    gel(aux,2) = lift_intern(gel(aux,2));
  return gerepilecopy(av, mkvec2(aux, x));
}

GEN
bnfisnorm(GEN bnf,GEN x,long flag,long PREC)
{
  pari_sp av = avma;
  GEN T = rnfisnorminit(pol_x(MAXVARN), bnf, flag == 0? 1: 2);
  return gerepileupto(av, rnfisnorm(T, x, flag == 1? 0: flag));
}
