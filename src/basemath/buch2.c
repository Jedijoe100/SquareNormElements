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
#include "pari.h"
#include "parinf.h"
/********************************************************************/
/**                                                                **/
/**              INSERT PERMANENT OBJECT IN STRUCTURE              **/
/**                                                                **/
/********************************************************************/
static const int OBJMAX = 2; /* maximum number of insertable objects */

/* insert O in S [last position] */
static void
obj_insert(GEN S, GEN O, long K)
{
  long l = lg(S)-1;
  GEN v = (GEN)S[l];
  if (typ(v) != t_VEC)
  {
    GEN w = zerovec(OBJMAX); w[K] = (long)O;
    S[l] = lclone(w);
  }
  else
    v[K] = lclone(O);
}

static GEN
get_extra_obj(GEN S, long K)
{
  GEN v = (GEN)S[lg(S)-1];
  if (typ(v) == t_VEC)
  {
    GEN O = (GEN)v[K];
    if (typ(O) != t_INT) return O;
  }
  return NULL;
}

GEN
check_and_build_obj(GEN S, int tag, GEN (*build)(GEN))
{
  GEN O = get_extra_obj(S, tag);
  if (!O)
  {
    pari_sp av = avma;
    obj_insert(S, build(S), tag); avma = av;
    O = get_extra_obj(S, tag);
  }
  return O;
}
/*******************************************************************/
/*                                                                 */
/*         CLASS GROUP AND REGULATOR (McCURLEY, BUCHMANN)          */
/*                    GENERAL NUMBER FIELDS                        */
/*                                                                 */
/*******************************************************************/
extern GEN u_idmat(long n);
extern GEN lllint_fp_ip(GEN x, long D);
extern GEN R_from_QR(GEN x, long prec);
extern GEN hnf_invimage(GEN A, GEN b);
extern GEN vecconst(GEN v, GEN x);
extern GEN nfbasic_to_nf(nfbasic_t *T, GEN ro, long prec);
extern GEN get_nfindex(GEN bas);
extern GEN sqred1_from_QR(GEN x, long prec);
extern GEN computeGtwist(GEN nf, GEN vdir);
extern GEN famat_mul(GEN f, GEN g);
extern GEN famat_to_arch(GEN nf, GEN fa, long prec);
extern GEN to_famat_all(GEN x, GEN y);
extern double check_bach(double cbach, double B);
extern GEN get_arch(GEN nf,GEN x,long prec);
extern GEN get_arch_real(GEN nf,GEN x,GEN *emb,long prec);
extern GEN get_roots(GEN x,long r1,long prec);
extern long int_elt_val(GEN nf, GEN x, GEN p, GEN b, GEN *t);
extern GEN norm_by_embed(long r1, GEN x);
extern void minim_alloc(long n,double ***q,long **x,double **y,double **z,double **v);
extern GEN arch_mul(GEN x, GEN y);
extern void wr_rel(GEN col);
extern void dbg_rel(long s, GEN col);

#define SFB_MAX 3

static const int RANDOM_BITS = 4;
static const int MAXRELSUP = 50;

/* used by factor[elt|gen|gensimple] to return factorizations of smooth elts
 * HACK: MAX_FACTOR_LEN never checked, we assume default value is enough
 * (since elts have small norm) */
static long primfact[500], exprimfact[500];

/* a factor base contains only non-inert primes
 * KC = # of P in factor base (p <= n, NP <= n2)
 * KC2= # of P assumed to generate class group (NP <= n2)
 *
 * KCZ = # of rational primes under ideals counted by KC
 * KCZ2= same for KC2 */

typedef struct powFB_t {
  GEN id2; /* id2[1] = P */
  GEN alg; /* alg[1] = 1, (id2[i],alg[i]) = red( id2[i-1] * P ) */
  GEN ord; /* ord[i] = known exponent of P in Cl(K) */
  GEN arc; /* arc[i] = multiplicative arch component of x
              such that id2[i] = x P^(i-1) */
  struct powFB_t *prev; /* previously used powFB */
} powFB_t;

typedef struct FB_t {
  GEN FB; /* FB[i] = i-th rational prime used in factor base */
  GEN LP; /* vector of all prime ideals in FB */
  GEN *LV; /* LV[p] = vector of P|p, NP <= n2
            * isclone() is set for LV[p] iff all P|p are in FB
            * LV[i], i not prime or i > n2, is undefined! */
  GEN iLP; /* iLP[p] = i such that LV[p] = [LP[i],...] */
  long KC, KCZ, KCZ2;
  GEN subFB; /* LP o subFB =  part of FB used to build random relations */
  int sfb_chg; /* need to change subFB ? */
  int newpow; /* need to compute powFB */
  powFB_t *pow;/* array of [P^0,...,P^{k_P}], P in LP o subFB */
  GEN perm; /* permutation of LP used to represent relations [updated by
               hnfspec/hnfadd: dense rows come first] */
  GEN vecG;
} FB_t;

enum { sfb_UNSUITABLE = -1, sfb_CHANGE = 1, sfb_INCREASE = 2 };

typedef struct REL_t {
  GEN R; /* relation vector as t_VECSMALL */
  int nz; /* index of first non-zero elt in R (hash) */
  GEN m; /* pseudo-minimum yielding the relation */
  GEN ex; /* exponents of subFB elts used to find R */
  powFB_t *pow; /* powsubFB associated to ex [ shared between rels ] */
} REL_t;

typedef struct RELCACHE_t {
  REL_t *chk; /* last checkpoint */
  REL_t *base; /* first rel found */
  REL_t *last; /* last rel found so far */
  REL_t *end; /* target for last relation. base <= last <= end */
  size_t len; /* number of rels pre-allocated in base */
} RELCACHE_t;

/* x a t_VECSMALL */
static long
ccontent(GEN x)
{
  long i, l = lg(x), s = labs(x[1]);
  for (i=2; i<l && s!=1; i++) s = cgcd(x[i],s);
  return s;
}

static void
delete_cache(RELCACHE_t *M)
{
  REL_t *rel;
  for (rel = M->base+1; rel <= M->last; rel++)
  {
    free((void*)rel->R);
    if (!rel->m) continue;
    gunclone(rel->m); 
    if (!rel->ex) continue;
    gunclone(rel->ex); 
  }
  free((void*)M->base); M->base = NULL;
}

static void
delete_FB(FB_t *F)
{
  powFB_t *S = F->pow;
  while (S)
  {
    powFB_t *T = S;
    gunclone(S->id2);
    gunclone(S->alg);
    gunclone(S->ord);
    if (S->arc) gunclone(S->arc);
    S = S->prev; free((void*)T);
  }
  gunclone(F->subFB);
}

INLINE GEN
col_0(long n)
{
   GEN c = (GEN)calloc(n + 1, sizeof(long));
   if (!c) err(memer);
   c[0] = evaltyp(t_VECSMALL) | evallg(n + 1);
   return c;
}

GEN
cgetalloc(GEN x, size_t l, long t)
{
  x = (GEN)gprealloc((void*)x, l * sizeof(long));
  x[0] = evaltyp(t) | evallg(l); return x;
}

static void
reallocate(RELCACHE_t *M, long len)
{
  REL_t *old = M->base;
  M->len = len;
  M->base = (REL_t*)gprealloc((void*)old, (len+1) * sizeof(REL_t));
  if (old)
  {
    size_t last = M->last - old, chk = M->chk - old, end = M->end - old;
    M->last = M->base + last;
    M->chk  = M->base + chk;
    M->end  = M->base + end;
  }
}

/* don't take P|p if P ramified, or all other Q|p already there */
static int
ok_subFB(FB_t *F, long t, GEN D)
{
  GEN LP, P = (GEN)F->LP[t];
  long p = itos((GEN)P[1]);
  LP = F->LV[p];
  return smodis(D,p) && (!isclone(LP) || t != F->iLP[p] + lg(LP)-1);
}

/* set subFB, reset F->pow
 * Fill F->perm (if != NULL): primes ideals sorted by increasing norm (except
 * the ones in subFB come first [dense rows for hnfspec]) */
static int
subFBgen(FB_t *F, GEN nf, double PROD, long minsFB)
{
  GEN y, perm, yes, no, D = (GEN)nf[3];
  long i, j, k, iyes, ino, lv = F->KC + 1;
  double prod;
  pari_sp av;

  F->LP   = cgetg(lv, t_VEC);
  F->perm = cgetg(lv, t_VECSMALL);
  av = avma;
  y = cgetg(lv,t_COL); /* Norm P */
  for (k=0, i=1; i <= F->KCZ; i++)
  {
    GEN LP = F->LV[F->FB[i]];
    long l = lg(LP);
    for (j = 1; j < l; j++)
    {
      GEN P = (GEN)LP[j];
      k++;
      y[k]     = (long)powgi((GEN)P[1], (GEN)P[4]);
      F->LP[k] = (long)P;
    }
  }
  /* perm sorts LP by increasing norm */
  perm = sindexsort(y);
  no  = cgetg(lv, t_VECSMALL); ino  = 1;
  yes = cgetg(lv, t_VECSMALL); iyes = 1;
  prod = 1.0;
  for (i = 1; i < lv; i++)
  {
    long t = perm[i];
    if (!ok_subFB(F, t, D)) { no[ino++] = t; continue; }

    yes[iyes++] = t;
    prod *= (double)itos((GEN)y[t]);
    if (iyes > minsFB && prod > PROD) break;
  }
  if (i == lv) return 0;
  setlg(yes, iyes);
  for (j=1; j<iyes; j++)     F->perm[j] = yes[j];
  for (i=1; i<ino; i++, j++) F->perm[j] =  no[i];
  for (   ; j<lv; j++)       F->perm[j] =  perm[j];
  F->subFB = gclone(yes);
  F->newpow = 1; 
  F->pow = NULL;
  if (DEBUGLEVEL)
    msgtimer("sub factorbase (%ld elements)",lg(F->subFB)-1);
  avma = av; return 1;
}
static int
subFB_change(FB_t *F, GEN nf, GEN L_jid)
{
  GEN yes, D = (GEN)nf[3];
  long i, iyes, minsFB, chg = F->sfb_chg, lv = F->KC + 1, l = lg(F->subFB)-1;
  pari_sp av = avma;

  switch (chg)
  {
    case sfb_INCREASE: minsFB = l + 1; break;
    default: minsFB = l; break;
  }

  if (DEBUGLEVEL) fprintferr("*** Changing sub factor base\n");
  yes = cgetg(minsFB+1, t_VECSMALL); iyes = 1;
  if (L_jid)
  {
    for (i = 1; i < lg(L_jid); i++)
    {
      long t = L_jid[i];
      if (!ok_subFB(F, t, D)) continue;

      yes[iyes++] = t;
      if (iyes > minsFB) break;
    }
  }
  else i = 1;
  if (iyes <= minsFB)
  {
    for ( ; i < lv; i++)
    {
      long t = F->perm[i];
      if (!ok_subFB(F, t, D)) continue;

      yes[iyes++] = t;
      if (iyes > minsFB) break;
    }
    if (i == lv) return 0;
  }
  if (gegal(F->subFB, yes))
  {
    if (chg != sfb_UNSUITABLE) F->sfb_chg = 0;
  }
  else
  {
    gunclone(F->subFB);
    F->subFB = gclone(yes);
    F->sfb_chg = 0;
  }
  F->newpow = 1; avma = av; return 1;
}

static GEN
red(GEN nf, GEN I, GEN *pm)
{
  GEN m, y = cgetg(3,t_VEC);
  y[1] = (long)I;
  y[2] = lgetg(1, t_MAT);
  y = ideallllred(nf, y, NULL, 0);
  m = (GEN)y[2];
  y = (GEN)y[1]; *pm = lg(m)==1? gun: gmael(m, 1, 1);
  return is_pm1(gcoeff(y,1,1))? NULL: ideal_two_elt(nf,y);
}

/* make sure enough room to store n more relations */
static void
pre_allocate(RELCACHE_t *cache, size_t n)
{
  size_t len = (cache->last - cache->base) + n;
  if (len >= cache->len) reallocate(cache, len << 1);
}

/* Compute powers of prime ideals (P^0,...,P^a) in subFB (a > 1) */
static void
powFBgen(FB_t *F, RELCACHE_t *cache, GEN nf)
{
  const int a = 1<<RANDOM_BITS;
  pari_sp av = avma;
  long i, j, c = 1, n = lg(F->subFB);
  GEN Id2, Alg, Ord;
  powFB_t *old = F->pow, *New;

  if (DEBUGLEVEL) fprintferr("Computing powers for subFB: %Z\n",F->subFB);
  F->pow = New = (powFB_t*) gpmalloc(sizeof(powFB_t));
  Id2 = cgetg(n, t_VEC);
  Alg = cgetg(n, t_VEC);
  Ord = cgetg(n, t_VECSMALL);
  New->arc = NULL;
  if (cache) pre_allocate(cache, n);
  for (i=1; i<n; i++)
  {
    GEN M, m, alg, id2, vp = (GEN)F->LP[ F->subFB[i] ];
    GEN z = cgetg(3,t_VEC); z[1] = vp[1]; z[2] = vp[2];
    id2 = cgetg(a+1,t_VEC); Id2[i] = (long)id2; id2[1] = (long)z;
    alg = cgetg(a+1,t_VEC); Alg[i] = (long)alg; alg[1] = un;
    vp = prime_to_ideal(nf,vp);
    for (j=2; j<=a; j++)
    {
      GEN J = red(nf, idealmulh(nf,vp,(GEN)id2[j-1]), &m);
      if (DEBUGLEVEL>1) fprintferr(" %ld",j);
      if (!J)
      {
        if (j == 2 && !red(nf, vp, &M)) { j = 1; m = M; }
        break;
      }
      if (gegal(J, (GEN)id2[j-1])) { j = 1; break; }
      id2[j] = (long)J;
      alg[j] = (long)m;
    }
    if (cache && j <= a)
    { /* vp^j principal */
      long k;
      REL_t *rel = cache->last + 1;
      rel->R = col_0(F->KC); rel->nz = F->subFB[i];
      rel->R[ rel->nz ] = j;
      for (k = 2; k < j; k++) m = element_mul(nf, m, (GEN)alg[k]);
      rel->m = gclone(m);
      rel->ex= NULL;
      rel->pow = New;
      cache->last = rel;
    }
    /* trouble with subFB: include ideal even though it's principal */
    if (j == 1 && F->sfb_chg == sfb_UNSUITABLE) j = 2;
    setlg(id2, j);
    setlg(alg, j); Ord[i] = j; if (c < 64) c *= j;
    if (DEBUGLEVEL>1) fprintferr("\n");
  }
  New->prev = old;
  New->id2 = gclone(Id2);
  New->ord = gclone(Ord);
  New->alg = gclone(Alg); avma = av;
  if (DEBUGLEVEL) msgtimer("powFBgen");
  /* if c too small we'd better change the subFB soon */
  F->sfb_chg = (c < 6)? sfb_UNSUITABLE: 0;
  F->newpow = 0;
}

/* Compute FB, LV, iLP + KC*. Reset perm
 * n2: bound for norm of tested prime ideals (includes be_honest())
 * n : bound for p, such that P|p (NP <= n2) used to build relations

 * Return prod_{p<=n2} (1-1/p) / prod_{Norm(P)<=n2} (1-1/Norm(P)),
 * close to residue of zeta_K at 1 = 2^r1 (2pi)^r2 h R / (w D) */
static GEN
FBgen(FB_t *F, GEN nf,long n2,long n)
{
  byteptr delta = diffptr;
  long i, p, ip;
  GEN prim, Res;

  maxprime_check((ulong)n2);
  F->sfb_chg = 0;
  F->FB  = cgetg(n2+1, t_VECSMALL);
  F->iLP = cgetg(n2+1, t_VECSMALL);
  F->LV = (GEN*)new_chunk(n2+1);

  Res = realun(DEFAULTPREC);
  prim = icopy(gun);
  i = ip = 0;
  F->KC = F->KCZ = 0;
  for (p = 0;;) /* p <= n2 */
  {
    pari_sp av = avma, av1;
    long k, l;
    GEN P, a, b;

    NEXT_PRIME_VIADIFF(p, delta);
    if (!F->KC && p > n) { F->KCZ = i; F->KC = ip; }
    if (p > n2) break;

    if (DEBUGLEVEL>1) { fprintferr(" %ld",p); flusherr(); }
    prim[2] = p; P = primedec(nf,prim); l = lg(P);

    /* a/b := (p-1)/p * prod_{P|p, NP <= n2} NP / (NP-1) */
    av1 = avma; a = b = NULL;
    for (k=1; k<l; k++)
    {
      GEN NormP = powgi(prim, gmael(P,k,4));
      long nor;
      if (is_bigint(NormP) || (nor=NormP[2]) > n2) break;

      if (a) { a = mului(nor, a); b = mului(nor-1, b); }
      else   { a = utoi(nor / p); b = utoi((nor-1) / (p-1)); }
    }
    if (a) affrr(divri(mulir(a,Res),b),   Res);
    else   affrr(divrs(mulsr(p-1,Res),p), Res);
    avma = av1;
    if (l == 2 && itos(gmael(P,1,3)) == 1) continue; /* p inert */

    /* keep non-inert ideals with Norm <= n2 */
    if (k == l)
      setisclone(P); /* flag it: all prime divisors in FB */
    else
      { setlg(P,k); P = gerepilecopy(av,P); }
    F->FB[++i]= p;
    F->LV[p]  = P;
    F->iLP[p] = ip; ip += k-1;
  }
  if (! F->KC) return NULL;
  setlg(F->FB, F->KCZ+1); F->KCZ2 = i;
  if (DEBUGLEVEL)
  {
    if (DEBUGLEVEL>1) fprintferr("\n");
    if (DEBUGLEVEL>6)
    {
      fprintferr("########## FACTORBASE ##########\n\n");
      fprintferr("KC2=%ld, KC=%ld, KCZ=%ld, KCZ2=%ld\n",
                  ip, F->KC, F->KCZ, F->KCZ2);
      for (i=1; i<=F->KCZ; i++) fprintferr("++ LV[%ld] = %Z",i,F->LV[F->FB[i]]);
    }
    msgtimer("factor base");
  }
  F->perm = NULL; return Res;
}

/*  SMOOTH IDEALS */
static void
store(long i, long e)
{
  primfact[++primfact[0]] = i; /* index */
  exprimfact[primfact[0]] = e; /* exponent */
}

/* divide out x by all P|p, where x as in can_factor().  k = v_p(Nx) */
static int
divide_p_elt(FB_t *F, long p, long k, GEN nf, GEN m)
{
  GEN P, LP = F->LV[p];
  long j, v, l = lg(LP), ip = F->iLP[p];
  for (j=1; j<l; j++)
  {
    P = (GEN)LP[j];
    v = int_elt_val(nf, m, (GEN)P[1], (GEN)P[5], NULL); /* v_P(m) */
    if (!v) continue;
    store(ip + j, v); /* v = v_P(m) > 0 */
    k -= v * itos((GEN)P[4]);
    if (!k) return 1;
  }
  return 0;
}
static int
divide_p_id(FB_t *F, long p, long k, GEN nf, GEN I)
{
  GEN P, LP = F->LV[p];
  long j, v, l = lg(LP), ip = F->iLP[p];
  for (j=1; j<l; j++)
  {
    P = (GEN)LP[j];
    v = idealval(nf,I, P);
    if (!v) continue;
    store(ip + j, v); /* v = v_P(I) > 0 */
    k -= v * itos((GEN)P[4]);
    if (!k) return 1;
  }
  return 0;
}
static int
divide_p_quo(FB_t *F, long p, long k, GEN nf, GEN I, GEN m)
{
  GEN P, LP = F->LV[p];
  long j, v, l = lg(LP), ip = F->iLP[p];
  for (j=1; j<l; j++)
  {
    P = (GEN)LP[j];
    v = int_elt_val(nf, m, (GEN)P[1], (GEN)P[5], NULL); /* v_P(m) */
    if (!v) continue;
    v -= idealval(nf,I, P);
    if (!v) continue;
    store(ip + j, v); /* v = v_P(m / I) > 0 */
    k -= v * itos((GEN)P[4]);
    if (!k) return 1;
  }
  return 0;
}

/* is *N > 0 a smooth rational integer wrt F ? (put the exponents in *ex) */
static int
smooth_int(FB_t *F, GEN *N, GEN *ex)
{
  GEN q, r, FB = F->FB;
  const long KCZ = F->KCZ;
  const long limp = FB[KCZ]; /* last p in FB */
  long i, p, k;

  *ex = new_chunk(KCZ+1);
  for (i=1; ; i++)
  {
    p = FB[i]; q = dvmdis(*N,p,&r);
    for (k=0; !signe(r); k++) { *N = q; q = dvmdis(*N, p, &r); }
    (*ex)[i] = k;
    if (cmpis(q,p) <= 0) break;
    if (i == KCZ) return 0;
  }
  (*ex)[0] = i;
  return (cmpis(*N,limp) <= 0);
}

static int
divide_p(FB_t *F, long p, long k, GEN nf, GEN I, GEN m)
{
  if (!m) return divide_p_id (F,p,k,nf,I);
  if (!I) return divide_p_elt(F,p,k,nf,m);
  return divide_p_quo(F,p,k,nf,I,m);
}

/* Let x = m if I == NULL,
 *         I if m == NULL,
 *         m/I otherwise.
 * Can we factor the integral ideal x ? N = Norm x > 0 */
static long
can_factor(FB_t *F, GEN nf, GEN I, GEN m, GEN N)
{
  GEN ex;
  long i;
  primfact[0] = 0;
  if (is_pm1(N)) return 1;
  if (!smooth_int(F, &N, &ex)) return 0;
  for (i=1; i<=ex[0]; i++)
    if (ex[i] && !divide_p(F, F->FB[i], ex[i], nf, I, m)) return 0;
  return is_pm1(N) || divide_p(F, itos(N), 1, nf, I, m);
}

/* can we factor m/I ? [m in I from pseudomin] */
static long
factorgen(FB_t *F, GEN nf, GEN I, GEN m)
{
  GEN Nm = absi( subres(gmul((GEN)nf[7],m), (GEN)nf[1]) ); /* |Nm| */
  GEN N  = diviiexact(Nm, dethnf_i(I)); /* N(m / I) */
  return can_factor(F, nf, I, m, N);
}

/*  FUNDAMENTAL UNITS */

/* a, m real. Return  (Re(x) + a) + I * (Im(x) % m) */
static GEN
addRe_modIm(GEN x, GEN a, GEN m)
{
  GEN re, im, z;
  if (typ(x) == t_COMPLEX)
  {
    re = gadd((GEN)x[1], a);
    im = gmod((GEN)x[2], m);
    if (gcmp0(im)) z = re;
    else
    {
      z = cgetg(3,t_COMPLEX);
      z[1] = (long)re;
      z[2] = (long)im;
    }
  }
  else
    z = gadd(x, a);
  return z;
}

/* clean archimedean components */
static GEN
cleanarch(GEN x, long N, long prec)
{
  long i, R1, RU, tx = typ(x);
  GEN s, y, pi2;

  if (tx == t_MAT)
  {
    y = cgetg(lg(x), tx);
    for (i=1; i < lg(x); i++)
      y[i] = (long)cleanarch((GEN)x[i], N, prec);
    return y;
  }
  if (!is_vec_t(tx)) err(talker,"not a vector/matrix in cleanarch");
  RU = lg(x)-1; R1 = (RU<<1)-N;
  s = gdivgs(sum(real_i(x), 1, RU), -N); /* -log |norm(x)| / N */
  y = cgetg(RU+1,tx);
  pi2 = Pi2n(1, prec);
  for (i=1; i<=R1; i++) y[i] = (long)addRe_modIm((GEN)x[i], s, pi2);
  if (i <= RU)
  {
    GEN pi4 = Pi2n(2, prec), s2 = gmul2n(s, 1);
    for (   ; i<=RU; i++) y[i] = (long)addRe_modIm((GEN)x[i], s2, pi4);
  }
  return y;
}

static GEN
not_given(pari_sp av, long fl, long reason)
{
  char *s;
  switch(reason)
  {
    case fupb_LARGE: s="fundamental units too large"; break;
    case fupb_PRECI: s="insufficient precision for fundamental units"; break;
    default: s="unknown problem with fundamental units";
  }
  if (fl & nf_FORCE)
  { if (reason != fupb_PRECI) err(talker, "bnfinit: %s", s); }
  else
    err(warner,"%s, not given",s);
  avma = av; return cgetg(1,t_MAT);
}

/* check whether exp(x) will get too big */
static long
expgexpo(GEN x)
{
  long i,j,e, E = - (long)HIGHEXPOBIT;
  GEN p1;

  for (i=1; i<lg(x); i++)
    for (j=1; j<lg(x[1]); j++)
    {
      p1 = gmael(x,i,j);
      if (typ(p1)==t_COMPLEX) p1 = (GEN)p1[1];
      e = gexpo(p1); if (e>E) E=e;
    }
  return E;
}

static GEN
split_realimag_col(GEN z, long r1, long r2)
{
  long i, ru = r1+r2;
  GEN a, x = cgetg(ru+r2+1,t_COL), y = x + r2;
  for (i=1; i<=r1; i++) { a = (GEN)z[i]; x[i] = lreal(a); }
  for (   ; i<=ru; i++) { a = (GEN)z[i]; x[i] = lreal(a); y[i] = limag(a); }
  return x;
}

GEN
split_realimag(GEN x, long r1, long r2)
{
  long i,l; GEN y;
  if (typ(x) == t_COL) return split_realimag_col(x,r1,r2);
  l = lg(x); y = cgetg(l, t_MAT);
  for (i=1; i<l; i++) y[i] = (long)split_realimag_col((GEN)x[i], r1, r2);
  return y;
}

/* assume x = (r1+r2) x (r1+2r2) matrix and y compatible vector
 * r1 first lines of x,y are real. Solve the system obtained by splitting
 * real and imaginary parts. If x is of nf type, use M instead.
 */
GEN
gauss_realimag(GEN x, GEN y)
{
  GEN M = (typ(x)==t_VEC)? gmael(checknf(x),5,1): x;
  long l = lg(M), r2 = l - lg(M[1]), r1 = l-1 - 2*r2;
  M = split_realimag(M,r1,r2);
  y = split_realimag(y,r1,r2); return gauss(M, y);
}

static GEN
getfu(GEN nf,GEN *ptA,long fl,long *pte,long prec)
{
  long e, i, j, R1, RU, N=degpol(nf[1]);
  pari_sp av = avma;
  GEN p1,p2,u,y,matep,s,A,vec;

  if (DEBUGLEVEL) fprintferr("\n#### Computing fundamental units\n");
  R1 = itos(gmael(nf,2,1)); RU = (N+R1)>>1;
  if (RU==1) { *pte=BIGINT; return cgetg(1,t_VEC); }

  *pte = 0; A = *ptA;
  matep = cgetg(RU,t_MAT);
  for (j=1; j<RU; j++)
  {
    s = gzero; for (i=1; i<=RU; i++) s = gadd(s,real_i(gcoeff(A,i,j)));
    s = gdivgs(s, -N);
    p1=cgetg(RU+1,t_COL); matep[j]=(long)p1;
    for (i=1; i<=R1; i++) p1[i] = ladd(s, gcoeff(A,i,j));
    for (   ; i<=RU; i++) p1[i] = ladd(s, gmul2n(gcoeff(A,i,j),-1));
  }
  if (prec <= 0) prec = gprecision(A);
  u = lllintern(real_i(matep),100,1,prec);
  if (!u) return not_given(av,fl,fupb_PRECI);

  p1 = gmul(matep,u);
  if (expgexpo(p1) > 20) { *pte = BIGINT; return not_given(av,fl,fupb_LARGE); }
  matep = gexp(p1,prec);
  y = grndtoi(gauss_realimag(nf,matep), &e);
  *pte = -e;
  if (e >= 0) return not_given(av,fl,fupb_PRECI);
  for (j=1; j<RU; j++)
    if (!gcmp1(idealnorm(nf, (GEN)y[j]))) break;
  if (j < RU) { *pte = 0; return not_given(av,fl,fupb_PRECI); }
  A = gmul(A,u);

  /* y[i] are unit generators. Normalize: smallest L2 norm + lead coeff > 0 */
  y = gmul((GEN)nf[7], y);
  vec = cgetg(RU+1,t_COL);
  p1 = PiI2n(0,prec); for (i=1; i<=R1; i++) vec[i] = (long)p1;
  p2 = PiI2n(1,prec); for (   ; i<=RU; i++) vec[i] = (long)p2;
  for (j=1; j<RU; j++)
  {
    p1 = (GEN)y[j]; p2 = QX_invmod(p1, (GEN)nf[1]);
    if (gcmp(QuickNormL2(p2,DEFAULTPREC),
             QuickNormL2(p1,DEFAULTPREC)) < 0)
    {
      A[j] = lneg((GEN)A[j]);
      p1 = p2;
    }
    if (gsigne(leading_term(p1)) < 0)
    {
      A[j] = ladd((GEN)A[j], vec);
      p1 = gneg(p1);
    }
    y[j] = (long)p1;
  }
  if (DEBUGLEVEL) msgtimer("getfu");
  *ptA = A; return y;
}

GEN
buchfu(GEN bnf)
{
  pari_sp av = avma;
  GEN nf, A, res;
  long c;

  bnf = checkbnf(bnf); A = (GEN)bnf[3]; nf = (GEN)bnf[7];
  res = (GEN)bnf[8];
  if (lg(res)==6 && lg(res[5])==lg(nf[6])-1) return gcopy((GEN)res[5]);
  return gerepilecopy(av, getfu(nf, &A, nf_UNITS, &c, 0));
}

/*******************************************************************/
/*                                                                 */
/*           PRINCIPAL IDEAL ALGORITHM (DISCRETE LOG)              */
/*                                                                 */
/*******************************************************************/

/* gen: prime ideals, return Norm (product), bound for denominator.
 * C = possible extra prime (^1) or NULL */
static GEN
get_norm_fact_primes(GEN gen, GEN ex, GEN C, GEN *pd)
{
  GEN N,d,P,p,e;
  long i,s,c = lg(ex);
  d = N = gun;
  for (i=1; i<c; i++)
    if ((s = signe(ex[i])))
    {
      P = (GEN)gen[i]; e = (GEN)ex[i]; p = (GEN)P[1];
      N = gmul(N, powgi(p, mulii((GEN)P[4], e)));
      if (s < 0)
      {
        e = gceil(gdiv(negi(e), (GEN)P[3]));
        d = mulii(d, powgi(p, e));
      }
    }
  if (C)
  {
    P = C; p = (GEN)P[1];
    N = gmul(N, powgi(p, (GEN)P[4]));
  }
  *pd = d; return N;
}

/* gen: HNF ideals */
static GEN
get_norm_fact(GEN gen, GEN ex, GEN *pd)
{
  long i, c = lg(ex);
  GEN d,N,I,e,n,ne,de;
  d = N = gun;
  for (i=1; i<c; i++)
    if (signe(ex[i]))
    {
      I = (GEN)gen[i]; e = (GEN)ex[i]; n = dethnf_i(I);
      ne = powgi(n,e);
      de = egalii(n, gcoeff(I,1,1))? ne: powgi(gcoeff(I,1,1), e);
      N = mulii(N, ne);
      d = mulii(d, de);
    }
  *pd = d; return N;
}

static GEN
get_pr_lists(GEN FB, long N, int list_pr)
{
  GEN pr, L;
  long i, l = lg(FB), p, pmax;

  pmax = 0;
  for (i=1; i<l; i++)
  {
    pr = (GEN)FB[i]; p = itos((GEN)pr[1]);
    if (p > pmax) pmax = p;
  }
  L = cgetg(pmax+1, t_VEC);
  for (p=1; p<=pmax; p++) L[p] = 0;
  if (list_pr)
  {
    for (i=1; i<l; i++)
    {
      pr = (GEN)FB[i]; p = itos((GEN)pr[1]);
      if (!L[p]) L[p] = (long)cget1(N+1, t_VEC);
      appendL((GEN)L[p], pr);
    }
    for (p=1; p<=pmax; p++)
      if (L[p]) L[p] = (long)gen_sort((GEN)L[p],0,cmp_prime_over_p);
  }
  else
  {
    for (i=1; i<l; i++)
    {
      pr = (GEN)FB[i]; p = itos((GEN)pr[1]);
      if (!L[p]) L[p] = (long)cget1(N+1, t_VECSMALL);
      appendL((GEN)L[p], (GEN)i);
    }
  }
  return L;
}

/* recover FB, LV, iLP, KCZ from Vbase */
static GEN
recover_partFB(FB_t *F, GEN Vbase, long N)
{
  GEN FB, LV, iLP, L = get_pr_lists(Vbase, N, 0);
  long l = lg(L), p, ip, i;

  i = ip = 0;
  FB = cgetg(l, t_VECSMALL);
  iLP= cgetg(l, t_VECSMALL);
  LV = cgetg(l, t_VEC);
#if 1 /* for debugging */
  for (p=1;p<l;p++) FB[p]=iLP[p]=LV[p]=0;
#endif
  for (p = 2; p < l; p++)
  {
    if (!L[p]) continue;
    FB[++i] = p;
    LV[p] = (long)vecextract_p(Vbase, (GEN)L[p]);
    iLP[p]= ip; ip += lg(L[p])-1;
  }
  F->KCZ = i;
  F->FB = FB; setlg(FB, i+1);
  F->LV = (GEN*)LV;
  F->iLP= iLP; return L;
}

static GEN
init_famat(GEN x)
{
  GEN y = cgetg(3, t_VEC);
  y[1] = (long)x;
  y[2] = lgetg(1,t_MAT); return y;
}

/* add v^e to factorization */
static void
add_to_fact(long v, long e)
{
  long i, l = primfact[0];
  for (i=1; i<=l && primfact[i] < v; i++)/*empty*/;
  if (i <= l && primfact[i] == v) exprimfact[i] += e; else store(v, e);
}

/* L (small) list of primes above the same p including pr. Return pr index */
static int
pr_index(GEN L, GEN pr)
{
  long j, l = lg(L);
  GEN al = (GEN)pr[2];
  for (j=1; j<l; j++)
    if (gegal(al, gmael(L,j,2))) return j;
  err(bugparier,"codeprime");
  return 0; /* not reached */
}

static long
Vbase_to_FB(FB_t *F, GEN pr)
{
  long p = itos((GEN)pr[1]);
  return F->iLP[p] + pr_index(F->LV[p], pr);
}

/* return famat y (principal ideal) such that y / x is smooth [wrt Vbase] */
static GEN
SPLIT(FB_t *F, GEN nf, GEN x, GEN Vbase)
{
  GEN vdir, id, z, ex, y, x0;
  long nbtest_lim, nbtest, bou, i, ru, lgsub;
  int flag = (gexpo(gcoeff(x,1,1)) < 100);

  /* try without reduction if x is small */
  if (flag && can_factor(F, nf, x, NULL, dethnf_i(x))) return NULL;

  /* if reduction fails (y scalar), do not retry can_factor */
  y = idealred_elt(nf,x);
  if ((!flag || !isnfscalar(y)) && factorgen(F, nf, x, y)) return y;

  /* reduce in various directions */
  ru = lg(nf[6]);
  vdir = cgetg(ru,t_VECSMALL);
  for (i=2; i<ru; i++) vdir[i]=0;
  for (i=1; i<ru; i++)
  {
    vdir[i] = 10;
    y = ideallllred_elt(nf,x,vdir);
    if (factorgen(F, nf, x, y)) return y;
    vdir[i] = 0;
  }

  /* tough case, multiply by random products */
  lgsub = 3;
  ex = cgetg(lgsub, t_VECSMALL);
  z  = init_famat(NULL);
  x0 = init_famat(x);
  nbtest = 1; nbtest_lim = 4;
  for(;;)
  {
    pari_sp av = avma;
    if (DEBUGLEVEL>2) fprintferr("# ideals tried = %ld\n",nbtest);
    id = x0;
    for (i=1; i<lgsub; i++)
    {
      ex[i] = random_bits(RANDOM_BITS);
      if (ex[i])
      { /* avoid prec pb: don't let id become too large as lgsub increases */
        if (id != x0) id = ideallllred(nf,id,NULL,0);
        z[1] = Vbase[i];
        id = idealmulh(nf, id, idealpowred(nf,z,stoi(ex[i]),0));
      }
    }
    if (id == x0) continue;

    for (i=1; i<ru; i++) vdir[i] = random_bits(RANDOM_BITS);
    for (bou=1; bou<ru; bou++)
    {
      y = ideallllred_elt(nf, (GEN)id[1], vdir);
      if (factorgen(F, nf, (GEN)id[1], y))
      {
        for (i=1; i<lgsub; i++)
          if (ex[i]) add_to_fact(Vbase_to_FB(F,(GEN)Vbase[i]), ex[i]);
        return famat_mul((GEN)id[2], y);
      }
      for (i=1; i<ru; i++) vdir[i] = 0;
      vdir[bou] = 10;
    }
    avma = av;
    if (++nbtest > nbtest_lim)
    {
      nbtest = 0;
      if (++lgsub < 7)
      {
        nbtest_lim <<= 1;
        ex = cgetg(lgsub, t_VECSMALL);
      }
      else nbtest_lim = VERYBIGINT; /* don't increase further */
      if (DEBUGLEVEL) fprintferr("SPLIT: increasing factor base [%ld]\n",lgsub);
    }
  }
}

/* return principal y such that y / x is smooth. Store factorization of latter*/
static GEN
split_ideal(GEN nf, GEN x, GEN Vbase)
{
  FB_t F;
  GEN L = recover_partFB(&F, Vbase, lg(x)-1);
  GEN y = SPLIT(&F, nf, x, Vbase);
  long p,j, i, l = lg(F.FB);

  p = j = 0; /* -Wall */
  for (i=1; i<=primfact[0]; i++)
  { /* decode index C = ip+j --> (p,j) */
    long q,k,t, C = primfact[i];
    for (t=1; t<l; t++)
    {
      q = F.FB[t];
      k = C - F.iLP[q];
      if (k <= 0) break;
      p = q;
      j = k;
    }
    primfact[i] = mael(L, p, j);
  }
  return y;
}

/* return sorted vectbase [sorted in bnf since version 2.2.4] */
static GEN
get_Vbase(GEN bnf)
{
  GEN vectbase = (GEN)bnf[5], perm = (GEN)bnf[6], Vbase;
  long i, l, tx = typ(perm);

  if (tx == t_INT) return vectbase;
  /* old format */
  l = lg(vectbase); Vbase = cgetg(l,t_VEC);
  for (i=1; i<l; i++) Vbase[i] = vectbase[itos((GEN)perm[i])];
  return Vbase;
}

/* all primes up to Minkowski bound factor on factorbase ? */
void
testprimes(GEN bnf, long bound)
{
  pari_sp av0 = avma, av;
  long p,i,nbideal,k,pmax;
  GEN f, dK, p1, Vbase, vP, fb, nf=checknf(bnf);
  byteptr d = diffptr;
  FB_t F;

  if (DEBUGLEVEL>1)
    fprintferr("PHASE 1: check primes to Zimmert bound = %ld\n\n",bound);
  dK= (GEN)nf[3];
  f = (GEN)nf[4];
  if (!gcmp1(f))
  {
    GEN D = gmael(nf,5,5);
    if (DEBUGLEVEL>1) fprintferr("**** Testing Different = %Z\n",D);
    p1 = isprincipalall(bnf, D, nf_FORCE);
    if (DEBUGLEVEL>1) fprintferr("     is %Z\n", p1);
  }
  /* sort factorbase for tablesearch */
  fb = gen_sort((GEN)bnf[5], 0, &cmp_prime_ideal);
  p1 = gmael(fb, lg(fb)-1, 1); /* largest p in factorbase */
  pmax = is_bigint(p1)? VERYBIGINT: itos(p1);
  maxprime_check((ulong)bound);
  Vbase = get_Vbase(bnf);
  (void)recover_partFB(&F, Vbase, degpol(nf[1]));
  for (av=avma, p=0; p < bound; avma=av)
  {
    NEXT_PRIME_VIADIFF(p, d);
    if (DEBUGLEVEL>1) fprintferr("*** p = %ld\n",p);
    vP = primedec(bnf, stoi(p));
    nbideal = lg(vP)-1;
    /* loop through all P | p if ramified, all but one otherwise */
    if (!smodis(dK,p)) nbideal++;
    for (i=1; i<nbideal; i++)
    {
      GEN P = (GEN)vP[i];
      if (DEBUGLEVEL>1) fprintferr("  Testing P = %Z\n",P);
      if (cmpis(idealnorm(bnf,P), bound) >= 0)
      {
        if (DEBUGLEVEL>1) fprintferr("    Norm(P) > Zimmert bound\n");
        continue;
      }
      if (p <= pmax && (k = tablesearch(fb, P, &cmp_prime_ideal)))
      { if (DEBUGLEVEL>1) fprintferr("    #%ld in factor base\n",k); }
      else if (DEBUGLEVEL>1)
        fprintferr("    is %Z\n", isprincipal(bnf,P));
      else /* faster: don't compute result */
        (void)SPLIT(&F, nf, prime_to_ideal(nf,P), Vbase);
    }
  }
  if (DEBUGLEVEL>1) { fprintferr("End of PHASE 1.\n\n"); flusherr(); }
  avma = av0;
}

GEN
init_red_mod_units(GEN bnf, long prec)
{
  GEN z, s = gzero, p1,s1,mat, matunit = (GEN)bnf[3];
  long i,j, RU = lg(matunit);

  if (RU == 1) return NULL;
  mat = cgetg(RU,t_MAT);
  for (j=1; j<RU; j++)
  {
    p1=cgetg(RU+1,t_COL); mat[j]=(long)p1;
    s1=gzero;
    for (i=1; i<RU; i++)
    {
      p1[i] = lreal(gcoeff(matunit,i,j));
      s1 = gadd(s1, gsqr((GEN)p1[i]));
    }
    p1[RU]=zero; if (gcmp(s1,s) > 0) s = s1;
  }
  s = gsqrt(gmul2n(s,RU),prec);
  if (gcmpgs(s,100000000) < 0) s = stoi(100000000);
  z = cgetg(3,t_VEC);
  z[1] = (long)mat;
  z[2] = (long)s; return z;
}

/* z computed above. Return unit exponents that would reduce col (arch) */
GEN
red_mod_units(GEN col, GEN z, long prec)
{
  long i,RU;
  GEN x,mat,N2;

  if (!z) return NULL;
  mat= (GEN)z[1];
  N2 = (GEN)z[2];
  RU = lg(mat); x = cgetg(RU+1,t_COL);
  for (i=1; i<RU; i++) x[i]=lreal((GEN)col[i]);
  x[RU] = (long)N2;
  x = lllintern(concatsp(mat,x),100, 1,prec);
  if (!x) return NULL;
  x = (GEN)x[RU];
  if (signe(x[RU]) < 0) x = gneg_i(x);
  if (!gcmp1((GEN)x[RU])) err(bugparier,"red_mod_units");
  setlg(x,RU); return x;
}

/* clg2 format changed for version 2.0.21 (contained ideals, now archs)
 * Compatibility mode: old clg2 format */
static GEN
get_Garch(GEN nf, GEN gen, GEN clg2, long prec)
{
  long i,c;
  GEN g,z,J,Garch, clorig = (GEN)clg2[3];

  c = lg(gen); Garch = cgetg(c,t_MAT);
  for (i=1; i<c; i++)
  {
    g = (GEN)gen[i];
    z = (GEN)clorig[i]; J = (GEN)z[1];
    if (!gegal(g,J))
    {
      z = idealinv(nf,z); J = (GEN)z[1];
      J = gmul(J,denom(J));
      if (!gegal(g,J))
      {
        z = ideallllred(nf,z,NULL,prec); J = (GEN)z[1];
        if (!gegal(g,J))
          err(bugparier,"isprincipal (incompatible bnf generators)");
      }
    }
    Garch[i] = z[2];
  }
  return Garch;
}

/* [x] archimedian components, A column vector. return [x] A
 * x may be a translated GEN (y + k) */
static GEN
act_arch(GEN A, GEN x)
{
  GEN a;
  long i,l = lg(A), tA = typ(A);
  if (tA == t_MAT)
  {
    /* assume lg(x) >= l */
    a = cgetg(l, t_VEC);
    for (i=1; i<l; i++) a[i] = (long)act_arch((GEN)A[i], x);
    return a;
  }
  if (l==1) return cgetg(1, t_VEC);
  if (tA == t_VECSMALL)
  {
    a = gmulsg(A[1], (GEN)x[1]);
    for (i=2; i<l; i++)
      if (A[i]) a = gadd(a, gmulsg(A[i], (GEN)x[i]));
  }
  else
  { /* A a t_COL of t_INT. Assume lg(A)==lg(x) */
    a = gmul((GEN)A[1], (GEN)x[1]);
    for (i=2; i<l; i++)
      if (signe(A[i])) a = gadd(a, gmul((GEN)A[i], (GEN)x[i]));
  }
  settyp(a, t_VEC); return a;
}

static long
prec_arch(GEN bnf)
{
  GEN a = (GEN)bnf[4];
  long i, l = lg(a), prec;

  for (i=1; i<l; i++)
    if ( (prec = gprecision((GEN)a[i])) ) return prec;
  return DEFAULTPREC;
}

/* col = archimedian components of x, Nx = kNx^e its norm, dx a bound for its
 * denominator. Return x or NULL (fail) */
GEN
isprincipalarch(GEN bnf, GEN col, GEN kNx, GEN e, GEN dx, long *pe)
{
  GEN nf, x, matunit, s;
  long N, R1, RU, i, prec = gprecision(col);
  bnf = checkbnf(bnf); nf = checknf(bnf);
  if (!prec) prec = prec_arch(bnf);
  matunit = (GEN)bnf[3];
  N = degpol(nf[1]);
  R1 = itos(gmael(nf,2,1));
  RU = (N + R1)>>1;
  col = cleanarch(col,N,prec); settyp(col, t_COL);
  if (RU > 1)
  { /* reduce mod units */
    GEN u, z = init_red_mod_units(bnf,prec);
    u = red_mod_units(col,z,prec);
    if (!u && z) return NULL;
    if (u) col = gadd(col, gmul(matunit, u));
  }
  s = gdivgs(gmul(e, glog(kNx,prec)), N);
  for (i=1; i<=R1; i++) col[i] = lexp(gadd(s, (GEN)col[i]),prec);
  for (   ; i<=RU; i++) col[i] = lexp(gadd(s, gmul2n((GEN)col[i],-1)),prec);
  /* d.alpha such that x = alpha \prod gj^ej */
  x = grndtoi(gmul(dx, gauss_realimag(nf,col)), pe);
  return (*pe > -5)? NULL: gdiv(x, dx);
}

/* y = C \prod g[i]^e[i] ? */
static int
fact_ok(GEN nf, GEN y, GEN C, GEN g, GEN e)
{
  pari_sp av = avma;
  long i, c = lg(e);
  GEN z = C? C: gun;
  for (i=1; i<c; i++)
    if (signe(e[i])) z = idealmul(nf, z, idealpow(nf, (GEN)g[i], (GEN)e[i]));
  if (typ(z) != t_MAT) z = idealhermite(nf,z);
  if (typ(y) != t_MAT) y = idealhermite(nf,y);
  i = gegal(y, z); avma = av; return i;
}

/* assume x in HNF. cf class_group_gen for notations */
static GEN
_isprincipal(GEN bnf, GEN x, long *ptprec, long flag)
{
  long i,lW,lB,e,c, prec = *ptprec;
  GEN Q,xar,Wex,Bex,U,y,p1,gen,cyc,xc,ex,d,col,A;
  GEN W       = (GEN)bnf[1];
  GEN B       = (GEN)bnf[2];
  GEN WB_C    = (GEN)bnf[4];
  GEN nf      = (GEN)bnf[7];
  GEN clg2    = (GEN)bnf[9];
  int old_format = (typ(clg2[2]) == t_MAT);

  U = (GEN)clg2[1]; if (old_format) U = ginv(U);
  cyc = gmael3(bnf,8,1,2); c = lg(cyc)-1;
  gen = gmael3(bnf,8,1,3);
  ex = cgetg(c+1,t_COL);
  if (c == 0 && !(flag & (nf_GEN|nf_GENMAT|nf_GEN_IF_PRINCIPAL))) return ex;

  /* factor x */
  x = Q_primitive_part(x, &xc);
  xar = split_ideal(nf,x,get_Vbase(bnf));
  lW = lg(W)-1; Wex = vecsmall_const(lW, 0);
  lB = lg(B)-1; Bex = vecsmall_const(lB, 0);
  for (i=1; i<=primfact[0]; i++)
  {
    long k = primfact[i];
    long l = k - lW;
    if (l <= 0) Wex[k] = exprimfact[i];
    else        Bex[l] = exprimfact[i];
  }

  /* x = -g_W Wex - g_B Bex + [xar]  | x = g_W Wex + g_B Bex if xar = NULL
   *   = g_W A + [xar] - [C_B]Bex    |   = g_W A + [C_B]Bex
   * since g_W B + g_B = [C_B] */
  if (xar)
  {
    A = gsub(ZM_zc_mul(B,Bex), vecsmall_col(Wex));
    Bex = gneg(Bex);
  }
  else
    A = gsub(vecsmall_col(Wex), ZM_zc_mul(B,Bex));
  Q = gmul(U, A);
  for (i=1; i<=c; i++)
    Q[i] = (long)truedvmdii((GEN)Q[i], (GEN)cyc[i], (GEN*)(ex+i));
  if ((flag & nf_GEN_IF_PRINCIPAL))
    { if (!gcmp0(ex)) return gzero; }
  else if (!(flag & (nf_GEN|nf_GENMAT)))
    return gcopy(ex);

  /* compute arch component of the missing principal ideal */
  if (old_format)
  {
    GEN Garch, V = (GEN)clg2[2];
    Bex = vecsmall_col(Bex);
    p1 = c? concatsp(gmul(V,Q), Bex): Bex;
    col = act_arch(p1, WB_C);
    if (c)
    {
      Garch = get_Garch(nf,gen,clg2,prec);
      col = gadd(col, act_arch(ex, Garch));
    }
  }
  else
  { /* g A = G Ur A + [ga]A, Ur A = D Q + R as above (R = ex)
           = G R + [GD]Q + [ga]A */
    GEN ga = (GEN)clg2[2], GD = (GEN)clg2[3];
    if (lB) col = act_arch(Bex, WB_C + lW); else col = zerovec(1); /* nf=Q ! */
    if (lW) col = gadd(col, act_arch(A, ga));
    if (c)  col = gadd(col, act_arch(Q, GD));
  }
  if (xar) col = gadd(col, famat_to_arch(nf, xar, prec));

  /* find coords on Zk; Q = N (x / \prod gj^ej) = N(alpha), denom(alpha) | d */
  Q = gdiv(dethnf_i(x), get_norm_fact(gen, ex, &d));
  col = isprincipalarch(bnf, col, Q, gun, d, &e);
  if (col && !fact_ok(nf,x, col,gen,ex)) col = NULL;
  if (!col && !gcmp0(ex))
  {
    p1 = isprincipalfact(bnf, gen, gneg(ex), x, flag);
    if (typ(p1) != t_VEC) return p1;
    col = (GEN)p1[2];
  }
  if (!col)
  {
    *ptprec = prec + (e >> TWOPOTBITS_IN_LONG) + (MEDDEFAULTPREC-2);
    if (flag & nf_FORCE)
    {
      if (DEBUGLEVEL) err(warner,"precision too low for generators, e = %ld",e);
      return NULL;
    }
    err(warner,"precision too low for generators, not given");
  }
  y = cgetg(3,t_VEC);
  if (xc && col) col = gmul(xc, col);
  if (!col) col = cgetg(1, t_COL);
  if (flag & nf_GEN_IF_PRINCIPAL) return col;
  y[1] = (long)ex;
  y[2] = (long)col; return y;
}

static GEN
triv_gen(GEN nf, GEN x, long c, long flag)
{
  GEN y;
  if (flag & nf_GEN_IF_PRINCIPAL) return _algtobasis_cp(nf,x);
  if (!(flag & (nf_GEN|nf_GENMAT))) return zerocol(c);
  y = cgetg(3,t_VEC);
  y[1] = (long)zerocol(c);
  y[2] = (long)_algtobasis_cp(nf,x); return y;
}

GEN
isprincipalall(GEN bnf,GEN x,long flag)
{
  long c, pr, tx = typ(x);
  pari_sp av = avma;
  GEN nf;

  bnf = checkbnf(bnf); nf = (GEN)bnf[7];
  if (tx == t_POLMOD)
  {
    if (!gegal((GEN)x[1],(GEN)nf[1]))
      err(talker,"not the same number field in isprincipal");
    x = (GEN)x[2]; tx = t_POL;
  }
  if (tx == t_POL || tx == t_COL || tx == t_INT || tx == t_FRAC)
  {
    if (gcmp0(x)) err(talker,"zero ideal in isprincipal");
    return triv_gen(nf, x, lg(mael3(bnf,8,1,2))-1, flag);
  }
  x = idealhermite(nf,x);
  if (lg(x)==1) err(talker,"zero ideal in isprincipal");
  if (degpol(nf[1]) == 1)
    return gerepileupto(av, triv_gen(nf, gcoeff(x,1,1), 0, flag));

  pr = prec_arch(bnf); /* precision of unit matrix */
  c = getrand();
  for (;;)
  {
    pari_sp av1 = avma;
    GEN y = _isprincipal(bnf,x,&pr,flag);
    if (y) return gerepilecopy(av, y);

    if (DEBUGLEVEL) err(warnprec,"isprincipal",pr);
    avma = av1; bnf = bnfnewprec(bnf,pr); (void)setrand(c);
  }
}

static GEN
add_principal_part(GEN nf, GEN u, GEN v, long flag)
{
  if (flag & nf_GENMAT)
    return (isnfscalar(u) && gcmp1((GEN)u[1]))? v: arch_mul(v,u);
  else
    return element_mul(nf, v, u);
}

/* isprincipal for C * \prod P[i]^e[i] (C omitted if NULL) */
GEN
isprincipalfact(GEN bnf,GEN P, GEN e, GEN C, long flag)
{
  long l = lg(e), i, prec, c;
  pari_sp av = avma;
  GEN id,id2, nf = checknf(bnf), z = NULL; /* gcc -Wall */
  int gen = flag & (nf_GEN|nf_GENMAT|nf_GEN_IF_PRINCIPAL);

  prec = prec_arch(bnf);
  if (gen)
  {
    z = cgetg(3,t_VEC);
    z[2] = (flag & nf_GENMAT)? lgetg(1, t_MAT): lmodulcp(gun,(GEN)nf[1]);
  }
  id = C;
  for (i=1; i<l; i++) /* compute prod P[i]^e[i] */
    if (signe(e[i]))
    {
      if (gen) z[1] = P[i]; else z = (GEN)P[i];
      id2 = idealpowred(bnf,z, (GEN)e[i],prec);
      id = id? idealmulred(nf,id,id2,prec): id2;
    }
  if (id == C) /* e = 0 */
  {
    if (!C) return isprincipalall(bnf, gun, flag);
    C = idealhermite(nf,C); id = z;
    if (gen) id[1] = (long)C; else id = C;
  }
  c = getrand();
  for (;;)
  {
    pari_sp av1 = avma;
    GEN y = _isprincipal(bnf, gen? (GEN)id[1]: id,&prec,flag);
    if (y)
    {
      if (flag & nf_GEN_IF_PRINCIPAL)
      {
        if (typ(y) == t_INT) { avma = av; return NULL; }
        y = add_principal_part(nf, y, (GEN)id[2], flag);
      }
      else
      {
        GEN u = (GEN)y[2];
        if (!gen || typ(y) != t_VEC) return gerepileupto(av,y);
        y[2] = (long)add_principal_part(nf, u, (GEN)id[2], flag);
      }
      return gerepilecopy(av, y);
    }

    if (flag & nf_GIVEPREC)
    {
      if (DEBUGLEVEL)
        err(warner,"insufficient precision for generators, not given");
      avma = av; return stoi(prec);
    }
    if (DEBUGLEVEL) err(warnprec,"isprincipal",prec);
    avma = av1; bnf = bnfnewprec(bnf,prec); (void)setrand(c);
  }
}

GEN
isprincipal(GEN bnf,GEN x)
{
  return isprincipalall(bnf,x,0);
}

GEN
isprincipalgen(GEN bnf,GEN x)
{
  return isprincipalall(bnf,x,nf_GEN);
}

GEN
isprincipalforce(GEN bnf,GEN x)
{
  return isprincipalall(bnf,x,nf_FORCE);
}

GEN
isprincipalgenforce(GEN bnf,GEN x)
{
  return isprincipalall(bnf,x,nf_GEN | nf_FORCE);
}

static GEN
rational_unit(GEN x, long n, long RU)
{
  GEN y;
  if (!gcmp1(x) && !gcmp_1(x)) return cgetg(1,t_COL);
  y = zerocol(RU);
  y[RU] = (long)gmodulss((gsigne(x)>0)? 0: n>>1, n);
  return y;
}

/* if x a famat, assume it is an algebraic integer (very costly to check) */
GEN
isunit(GEN bnf,GEN x)
{
  long tx = typ(x), i, R1, RU, n, prec;
  pari_sp av = avma;
  GEN p1, v, rlog, logunit, ex, nf, z, pi2_sur_w, gn, emb;

  bnf = checkbnf(bnf); nf=(GEN)bnf[7];
  logunit = (GEN)bnf[3]; RU = lg(logunit);
  p1 = gmael(bnf,8,4); /* roots of 1 */
  gn = (GEN)p1[1]; n = itos(gn);
  z  = algtobasis(nf, (GEN)p1[2]);
  switch(tx)
  {
    case t_INT: case t_FRAC: case t_FRACN:
      return rational_unit(x, n, RU);

    case t_MAT: /* famat */
      if (lg(x) != 3 || lg(x[1]) != lg(x[2]))
        err(talker, "not a factorization matrix in isunit");
      break;

    case t_COL:
      if (degpol(nf[1]) != lg(x)-1)
        err(talker,"not an algebraic number in isunit");
      break;

    default: x = algtobasis(nf, x);
      break;
  }
  /* assume a famat is integral */
  if (tx != t_MAT && !gcmp1(denom(x))) { avma = av; return cgetg(1,t_COL); }
  if (isnfscalar(x)) return gerepileupto(av, rational_unit((GEN)x[1],n,RU));

  R1 = nf_get_r1(nf); v = cgetg(RU+1,t_COL);
  for (i=1; i<=R1; i++) v[i] = un;
  for (   ; i<=RU; i++) v[i] = deux;
  logunit = concatsp(logunit, v);
  /* ex = fundamental units exponents */
  rlog = real_i(logunit);
  prec = nfgetprec(nf);
  for (i=1;;)
  {
    GEN logN, rx = get_arch_real(nf,x,&emb, MEDDEFAULTPREC);
    long e;
    if (rx)
    {
      logN = sum(rx, 1, RU); /* log(Nx), should be ~ 0 */
      if (gexpo(logN) > -20)
      {
        long p = 2 + max(1, (nfgetprec(nf)-2) / 2);
        if (typ(logN) != t_REAL || gprecision(rx) > p)
          { avma = av; return cgetg(1,t_COL); } /* not a precision problem */
        rx = NULL;
      }
    }
    if (rx)
    {
      ex = grndtoi(gauss(rlog, rx), &e);
      if (gcmp0((GEN)ex[RU]) && e < -4) break;
    }
    if (i == 1)
      prec = MEDDEFAULTPREC + (gexpo(x) >> TWOPOTBITS_IN_LONG);
    else
    {
      if (i > 4) err(precer,"isunit");
      prec = (prec-1)<<1;
    }
    i++;
    if (DEBUGLEVEL) err(warnprec,"isunit",prec);
    nf = nfnewprec(nf, prec);
  }

  setlg(ex, RU);
  p1 = row_i(logunit,1, 1,RU-1);
  p1 = gneg(imag_i(gmul(p1,ex))); if (!R1) p1 = gmul2n(p1, -1);
  p1 = gadd(garg((GEN)emb[1],prec), p1);
  /* p1 = arg(the missing root of 1) */

  pi2_sur_w = divrs(mppi(prec), n>>1); /* 2pi / n */
  p1 = ground(gdiv(p1, pi2_sur_w));
  if (n > 2)
  {
    GEN ro = gmul(row(gmael(nf,5,1), 1), z);
    GEN p2 = ground(gdiv(garg(ro, prec), pi2_sur_w));
    p1 = mulii(p1,  mpinvmod(p2, gn));
  }

  ex[RU] = lmodulcp(p1, gn);
  setlg(ex, RU+1); return gerepilecopy(av, ex);
}

GEN
zsignunits(GEN bnf, GEN archp, int add_zu)
{
  GEN y, A = (GEN)bnf[3], pi = mppi(DEFAULTPREC);
  long l, i, j = 1, RU = lg(A);

  if (!archp) archp = perm_identity( nf_get_r1((GEN)bnf[7]) );
  l = lg(archp);
  if (add_zu) { RU++; A--; }
  y = cgetg(RU,t_MAT);
  if (add_zu)
  {
    GEN w = gmael3(bnf,8,4,1), v = cgetg(l, t_COL);
    if (egalii(w,gdeux)) (void)vecconst(v, stoi(-1));
    y[j++] = (long)v;
  }
  for ( ; j < RU; j++)
  {
    GEN c = cgetg(l,t_COL), d = (GEN)A[j];
    pari_sp av = avma;
    y[j] = (long)c;
    for (i=1; i<l; i++)
    {
      GEN p1 = ground( gdiv(imag_i((GEN)d[ archp[i] ]), pi) );
      c[i] = mpodd(p1)? (long)un: zero;
    }
    avma = av;
  }
  return y;
}

/* obsolete */
GEN
signunits(GEN bnf)
{
  pari_sp av = avma;
  GEN y, mun = negi(gun);
  long i, j;

  bnf = checkbnf(bnf);
  y = zsignunits(bnf, NULL, 0);
  for (j = 1; j < lg(y); j++)
  {
    GEN *c = (GEN*)y[j];
    for (i = 1; i < lg(c); i++) c[i] = (c[i] == gzero)? gun: mun;
  }
  return gerepilecopy(av, y);
}

/* LLL-reduce ideal and return Cholesky for T2 | ideal */
static GEN
red_ideal(GEN *ideal, GEN G0, GEN G, long prec)
{
  GEN u = lll(gmul(G0, *ideal), DEFAULTPREC);
  *ideal = gmul(*ideal,u); /* approximate LLL reduction */
  return sqred1_from_QR(gmul(G, *ideal), prec);
}

static GEN
get_log_embed(REL_t *rel, GEN M, long RU, long R1, long prec)
{
  GEN arch, C;
  long i;
  if (!rel->m) return zerocol(RU);
  arch = gmul(M, rel->m);
  if (rel->ex) 
  {
    GEN t, ex = rel->ex, x = NULL;
    long l = lg(ex);
    for (i=1; i<l; i++)
      if (ex[i])
      {
        t = gmael(rel->pow->arc, i, ex[i]);
        x = x? vecmul(x, t): t; /* arch components in MULTIPLICATIVE form */
      }
    if (x) arch = vecmul(x, arch);
  }
  C = cgetg(RU+1, t_COL); arch = glog(arch, prec);
  for (i=1; i<=R1; i++) C[i] = arch[i];
  for (   ; i<=RU; i++) C[i] = lmul2n((GEN)arch[i], 1);
  return C;
}

static void
powFB_fill(RELCACHE_t *cache, GEN M)
{
  powFB_t *old = NULL, *pow;
  pari_sp av = avma;
  REL_t *rel;
  for (rel = cache->base + 1; rel <= cache->last; rel++)
  {
    long i, j, l;
    GEN Alg, Arc;
    if (!rel->ex) continue;
    pow = rel->pow; if (pow == old) continue;
    old = pow;
    if (pow->arc) gunclone(pow->arc);
    Alg = pow->alg; l = lg(Alg); Arc = cgetg(l, t_VEC);
    for (i = 1; i < l; i++)
    {
      GEN z, t = (GEN)Alg[i]; 
      long lt = lg(t);
      z = cgetg(lt, t_VEC);
      Arc[i] = (long)z; if (lt == 1) continue;
      z[1] = M[1];  /* leave t[1] = 1 alone ! */
      for (j = 2; j < lt; j++)
        z[j] = lmul(typ(t[j]) == t_COL? M: (GEN)M[1], (GEN)t[j]);
      for (j = 3; j < lt; j++)
        z[j] = (long)vecmul((GEN)z[j], (GEN)z[j-1]);
    }
    pow->arc = gclone(Arc);
  }
  avma = av;
}

static void
set_fact(REL_t *rel, FB_t *F)
{
  long i;
  GEN c = rel->R = col_0(F->KC); rel->nz = primfact[1];
  for (i=1; i<=primfact[0]; i++) c[primfact[i]] = exprimfact[i];
}

/* If V depends linearly from the columns of the matrix, return 0.
 * Otherwise, update INVP and L and return 1. Compute mod p (much faster)
 * so some kernel vector might not be genuine. */
static int
addcolumn_mod(GEN V, GEN invp, GEN L, ulong p)
{
  pari_sp av = avma;
  GEN a = Flm_Flv_mul(invp, V, p);
  long i,j,k, n = lg(invp);
  ulong invak;

  if (DEBUGLEVEL>6)
  {
    fprintferr("adding vector = %Z\n",V);
    fprintferr("vector in new basis = %Z\n",a);
    fprintferr("list = %Z\n",L);
    fprintferr("base change =\n"); outerr(invp);
  }
  k = 1; while (k<n && (L[k] || !a[k])) k++;
  if (k == n) { avma = av; return 0; }
  invak = invumod((ulong)a[k], p);
  L[k] = 1;
  for (i=k+1; i<n; i++) 
    if (a[i]) a[i] = p - ((a[i] * invak) % p);
  for (j=1; j<=k; j++)
  {
    GEN c = (GEN)invp[j];
    ulong ck = (ulong)c[k];
    if (!ck) continue;
    c[k] = (ck * invak) % p;
    if (j==k)
      for (i=k+1; i<n; i++) c[i] = (a[i] * ck) % p;
    else
      for (i=k+1; i<n; i++) c[i] = (c[i] + a[i] * ck) % p;
  }
  avma = av; return 1;
}

/* compute the rank of A in M_n,r(Z) (C long), where rank(A) = r <= n.
 * Starting from the identity (canonical basis of Q^n), we obtain a base
 * change matrix P by taking the independent columns of A and vectors from
 * the canonical basis. Update invp = 1/P, and L in {0,1}^n, with L[i] = 0
 * if P[i] = e_i, and 1 if P[i] = A_i (i-th column of A)
 */
static GEN
relationrank(RELCACHE_t *cache, GEN L, ulong p)
{
  GEN invp = u_idmat(lg(L) - 1);
  REL_t *rel = cache->base + 1;
  for (; rel <= cache->last; rel++) (void)addcolumn_mod(rel->R, invp, L, p);
  return invp;
}

static void
small_norm(RELCACHE_t *cache, FB_t *F, GEN G0, double LOGD, GEN nf,
           long nbrelpid, double LIMC2)
{
  const ulong mod_p = 27449;
  const int BMULT = 8, maxtry_DEP  = 20, maxtry_FACT = 500;
  const double eps = 0.000001;
  double *y,*z,**q,*v, BOUND;
  pari_sp av;
  long nbsmallnorm, nbfact, j, k, noideal, precbound;
  long N = degpol(nf[1]), R1 = nf_get_r1(nf), prec = nfgetprec(nf);
  GEN x, gx, Mlow, M, G, r;
  GEN L = vecsmall_const(F->KC, 0), invp = relationrank(cache, L, mod_p);
  REL_t *rel = cache->last;

  if (DEBUGLEVEL)
    fprintferr("\n#### Looking for %ld relations (small norms)\n",
               cache->end - cache->base);
  gx = NULL; /* gcc -Wall */
  nbsmallnorm = nbfact = 0;
  M = gmael(nf,5,1);
  G = gmael(nf,5,2);
 /* LLL reduction produces v0 in I such that
  *     T2(v0) <= (4/3)^((n-1)/2) NI^(2/n) disc(K)^(1/n)
  * We consider v with T2(v) <= BMULT * T2(v0)
  * Hence Nv <= ((4/3)^((n-1)/2) * BMULT / n)^(n/2) NI sqrt(disc(K)) */
  precbound = 3 + (long)ceil(
    (N/2. * ((N-1)/2.* log(4./3) + log(BMULT/(double)N)) + log(LIMC2) + LOGD/2)
      / (BITS_IN_LONG * log(2.))); /* enough to compute norms */
  Mlow = (precbound < prec)? gprec_w(M, precbound): M;

  minim_alloc(N+1, &q, &x, &y, &z, &v);
  for (av = avma, noideal = F->KC; noideal; noideal--, avma = av)
  {
    long nbrelideal = 0, dependent = 0, try_factor = 0;
    GEN IDEAL, ideal = (GEN)F->LP[noideal];
    pari_sp av2;

    if (DEBUGLEVEL>1) fprintferr("\n*** Ideal no %ld: %Z\n", noideal, ideal);
    IDEAL = prime_to_ideal(nf,ideal);
    r = red_ideal(&IDEAL, G0, G, prec);
    if (!r) err(bugparier, "small_norm (precision too low)");

    for (k=1; k<=N; k++)
    {
      v[k] = gtodouble(gcoeff(r,k,k));
      for (j=1; j<k; j++) q[j][k] = gtodouble(gcoeff(r,j,k));
      if (DEBUGLEVEL>3) fprintferr("v[%ld]=%.4g ",k,v[k]);
    }

    BOUND = v[2] + v[1] * q[1][2] * q[1][2];
    if (BOUND > v[1]) BOUND = v[1];
    BOUND *= BMULT; /* at most BMULT x smallest known vector */
    if (DEBUGLEVEL>1)
    {
      if (DEBUGLEVEL>3) fprintferr("\n");
      fprintferr("BOUND = %.4g\n",BOUND); flusherr();
    }
    BOUND *= 1 + eps;
    k = N; y[N] = z[N] = 0; x[N] = (long)sqrt(BOUND/v[N]);
    for (av2 = avma;; x[1]--, avma = av2)
    {
      for(;;) /* look for primitive element of small norm, cf minim00 */
      {
        double p;
	if (k>1)
	{
	  long l = k-1;
	  z[l] = 0;
	  for (j=k; j<=N; j++) z[l] += q[l][j]*x[j];
	  p = (double)x[k] + z[k];
	  y[l] = y[k] + p*p*v[k];
	  x[l] = (long) floor(sqrt((BOUND-y[l])/v[l])-z[l]);
          k = l;
	}
	for(;;)
	{
	  p = (double)x[k] + z[k];
	  if (y[k] + p*p*v[k] <= BOUND) break;
	  k++; x[k]--;
	}
        if (k != 1) continue;

	/* element complete */
        if (y[1]<=eps) goto ENDIDEAL; /* skip all scalars: [*,0...0] */
        if (ccontent(x)==1) /* primitive */
        {
          gx = ZM_zc_mul(IDEAL,x);
          if (!isnfscalar(gx))
          {
            GEN Nx, xembed = gmul(Mlow, gx); 
            nbsmallnorm++;
            if (++try_factor > maxtry_FACT) goto ENDIDEAL;
            Nx = ground( norm_by_embed(R1,xembed) );
            setsigne(Nx, 1);
            if (can_factor(F, nf, NULL, gx, Nx)) break;
            if (DEBUGLEVEL > 1) { fprintferr("."); flusherr(); }
          }
        }
        x[1]--;
      }
      set_fact(++rel, F);
      /* make sure we get maximal rank first, then allow all relations */
      if (rel - cache->base > 1 && rel - cache->base <= F->KC
                                && ! addcolumn_mod(rel->R,invp,L, mod_p))
      { /* Q-dependent from previous ones: forget it */
        free((void*)rel->R); rel--;
        if (DEBUGLEVEL>1) fprintferr("*");
        if (++dependent > maxtry_DEP) break;
        continue;
      }
      rel->m = gclone(gx);
      rel->ex= NULL;
      dependent = 0;

      if (DEBUGLEVEL) { nbfact++; dbg_rel(rel - cache->base, rel->R); }
      if (rel >= cache->end) goto END; /* we have enough */
      if (++nbrelideal == nbrelpid) break;
    }
ENDIDEAL:
    if (DEBUGLEVEL>1) msgtimer("for this ideal");
  }
END:
  cache->last = rel;
  if (DEBUGLEVEL)
  {
    fprintferr("\n"); msgtimer("small norm relations");
    fprintferr("  small norms gave %ld relations.\n",
               cache->last - cache->base);
    if (nbsmallnorm)
      fprintferr("  nb. fact./nb. small norm = %ld/%ld = %.3f\n",
                  nbfact,nbsmallnorm,((double)nbfact)/nbsmallnorm);
  }
}

/* I assumed to be integral, G the Cholesky form of a weighted T2 matrix.
 * Return an irrational m in I with T2(m) small */
static GEN
pseudomin(GEN I, GEN G)
{
  GEN m, GI = lllint_fp_ip(gmul(G, I), 100);
  m = gauss(G, (GEN)GI[1]);
  if (isnfscalar(m)) m = gauss(G, (GEN)GI[2]);
  if (DEBUGLEVEL>5) fprintferr("\nm = %Z\n",m);
  return m;
}

static void
dbg_newrel(RELCACHE_t *cache, long jid, long jdir)
{
  fprintferr("\n++++ cglob = %ld: new relation (need %ld)", 
             cache->last - cache->base, cache->end - cache->base);
  wr_rel(cache->last->R);
  if (DEBUGLEVEL>3)
  {
    fprintferr("(jid=%ld,jdir=%ld)", jid,jdir);
    msgtimer("for this relation");
  }
  flusherr() ;
}

static void
dbg_cancelrel(long jid, long jdir, GEN col)
{
  fprintferr("relation cancelled: ");
  if (DEBUGLEVEL>3) fprintferr("(jid=%ld,jdir=%ld)",jid,jdir);
  wr_rel(col); flusherr();
}

/* Check if we already have a column mat[i] equal to mat[s]
 * General check for colinearity useless since exceedingly rare */
static int
already_known(RELCACHE_t *cache, REL_t *rel)
{
  REL_t *r;
  GEN cols = rel->R;
  long bs, l = lg(cols);

  bs = 1; while (bs < l && !cols[bs]) bs++;
  if (bs == l) return -1; /* zero relation */

  for (r = rel - 1; r > cache->base; r--)
  {
    if (bs == r->nz) /* = index of first non zero elt in cols */
    {
      GEN coll = r->R;
      long b = bs;
      do b++; while (b < l && cols[b] == coll[b]);
      if (b == l) return 1;
    }
  }
  rel->nz = bs; return 0;
}

/* I integral ideal in HNF form */
static GEN
remove_content(GEN I)
{
  long N = lg(I)-1;
  if (!gcmp1(gcoeff(I,N,N))) I = Q_primpart(I);
  return I;
}

static int
rnd_rel(RELCACHE_t *cache, FB_t *F, GEN nf, GEN L_jid, long *pjid)
{
  long nbG = lg(F->vecG)-1, lgsub = lg(F->subFB), jlist = 1, jid = *pjid;
  long i, j, cptlist = 0, cptzer = 0;
  pari_sp av, av1;
  GEN ideal, m, ex = cgetg(lgsub, t_VECSMALL);
 
  if (DEBUGLEVEL) {
    fprintferr("\n(more relations needed: %ld)\n", cache->end - cache->last);
    if (L_jid) fprintferr("looking hard for %Z\n",L_jid);
  }
  for (av = avma;;)
  {
    REL_t *rel = cache->last;
    GEN P;
    if (L_jid && jlist < lg(L_jid))
    {
      if (++cptlist > 3) { jid = L_jid[jlist++]; cptlist = 0; }
      if (!jid) jid = 1;
    }
    else
    {
      if (jid == F->KC) jid = 1; else jid++;
    }
    avma = av;
    ideal = P = prime_to_ideal(nf, (GEN)F->LP[jid]);
    do {
      for (i=1; i<lgsub; i++)
      { /* reduce mod apparent order */
        ex[i] = random_bits(RANDOM_BITS) % F->pow->ord[i];
        if (ex[i]) ideal = idealmulh(nf,ideal, gmael(F->pow->id2,i,ex[i]));
      }
    } while (ideal == P); /* If ex  = 0, try another */
    ideal = remove_content(ideal);
    if (gcmp1(gcoeff(ideal,1,1))) continue;

    if (DEBUGLEVEL>1) fprintferr("(%ld)", jid);
    for (av1 = avma, j = 1; j <= nbG; j++, avma = av1)
    { /* reduce along various directions */
      m = pseudomin(ideal, (GEN)(F->vecG)[j]);
      if (!factorgen(F,nf,ideal,m))
      {
        if (DEBUGLEVEL>1) { fprintferr("."); flusherr(); }
        continue;
      }
      /* can factor ideal, record relation */
      set_fact(++rel, F); rel->R[jid]++;
      for (i=1; i<lgsub; i++) rel->R[ F->subFB[i] ] += ex[i];
      if (already_known(cache, rel))
      { /* forget it */
        if (DEBUGLEVEL>1) dbg_cancelrel(jid,j,rel->R);
        free((void*)rel->R); rel--;
        if (++cptzer > MAXRELSUP)
        {
          if (L_jid) { cptzer -= 10; break; }
          *pjid = jid; return 0;
        }
        continue;
      }
      rel->m = gclone(m);
      rel->ex = gclone(ex);
      rel->pow = F->pow; cache->last = rel;
      if (DEBUGLEVEL) dbg_newrel(cache, jid, j);
      /* Need more, try next prime ideal */
      if (rel < cache->end) { cptzer = 0; break; }
      /* We have found enough. Return */
      avma = av; *pjid = jid; return 1;
    }
  }
}

/* remark: F->KCZ changes if be_honest() fails */
static int
be_honest(FB_t *F, GEN nf)
{
  GEN P, ideal, m, vdir;
  long ex, i, j, J, k, iz, nbtest, ru;
  long nbG = lg(F->vecG)-1, lgsub = lg(F->subFB), KCZ0 = F->KCZ;
  pari_sp av;

  if (DEBUGLEVEL) {
    fprintferr("Be honest for %ld primes from %ld to %ld\n", F->KCZ2 - F->KCZ,
               F->FB[ F->KCZ+1 ], F->FB[ F->KCZ2 ]);
  }
  ru = lg(nf[6]); vdir = cgetg(ru, t_VECSMALL);
  av = avma;
  for (iz=F->KCZ+1; iz<=F->KCZ2; iz++, avma = av)
  {
    long p = F->FB[iz];
    if (DEBUGLEVEL>1) fprintferr("%ld ", p);
    P = F->LV[p]; J = lg(P);
    /* all P|p in FB + last is unramified --> check all but last */
    if (isclone(P) && itou(gmael(P,J-1,3)) == 1UL /* e = 1 */) J--;

    for (j=1; j<J; j++)
    {
      GEN ideal0 = prime_to_ideal(nf,(GEN)P[j]);
      pari_sp av1, av2 = avma;
      for(nbtest=0;;)
      {
	ideal = ideal0;
	for (i=1; i<lgsub; i++)
	{
	  ex = random_bits(RANDOM_BITS) % F->pow->ord[i];
	  if (ex) ideal = idealmulh(nf,ideal, gmael(F->pow->id2,i,ex));
	}
        ideal = remove_content(ideal);
        for (av1 = avma, k = 1; k <= nbG; k++, avma = av1)
	{
          m = pseudomin(ideal, (GEN)(F->vecG)[j]);
          if (factorgen(F,nf,ideal,m)) break;
	}
	avma = av2; if (k < ru) break;
        if (++nbtest > 50)
        {
          err(warner,"be_honest() failure on prime %Z\n", P[j]);
          return 0;
        }
      }
    }
    F->KCZ++; /* SUCCESS, "enlarge" factorbase */
  }
  if (DEBUGLEVEL)
  {
    if (DEBUGLEVEL>1) fprintferr("\n");
    msgtimer("be honest");
  }
  F->KCZ = KCZ0; avma = av; return 1;
}

int
trunc_error(GEN x)
{
  return typ(x)==t_REAL && signe(x)
                        && (expo(x)>>TWOPOTBITS_IN_LONG) + 3 > lg(x);
}

/* A = complex logarithmic embeddings of units (u_j) found so far */
static GEN
compute_multiple_of_R(GEN A,long RU,long N,GEN *ptlambda)
{
  GEN T,v,mdet,mdet_t,Im_mdet,kR,xreal,lambda;
  long i, zc = lg(A)-1, R1 = 2*RU - N;
  pari_sp av = avma;

  if (DEBUGLEVEL) fprintferr("\n#### Computing regulator multiple\n");
  xreal = real_i(A); /* = (log |sigma_i(u_j)|) */
  T = cgetg(RU+1,t_COL);
  for (i=1; i<=R1; i++) T[i] = un;
  for (   ; i<=RU; i++) T[i] = deux;
  mdet = concatsp(xreal,T); /* det(Span(mdet)) = N * R */

  i = gprecision(mdet); /* truncate to avoid "near dependent" vectors */
  mdet_t = (i <= 4)? mdet: gprec_w(mdet,i-1);
  v = (GEN)sindexrank(mdet_t)[2]; /* list of independent column indices */
  /* check we have full rank for units */
  if (lg(v) != RU+1) { avma=av; return NULL; }

  Im_mdet = vecextract_p(mdet,v);
  /* integral multiple of R: the cols we picked form a Q-basis, they have an
   * index in the full lattice. Last column is T */
  kR = gdivgs(det2(Im_mdet), N);
  /* R > 0.2 uniformly */
  if (gcmp0(kR) || gexpo(kR) < -3) { avma=av; return NULL; }

  kR = mpabs(kR);
  lambda = gauss(Im_mdet,xreal); /* approximate rational entries */
  for (i=1; i<=zc; i++) setlg(lambda[i], RU);
  gerepileall(av,2, &lambda, &kR);
  *ptlambda = lambda; return kR;
}

static GEN
bestappr_noer(GEN x, GEN k)
{
  GEN y;
  CATCH(precer) { y = NULL; }
  TRY { y = bestappr(x,k); } ENDCATCH;
  return y;
}

/* Input:
 * lambda = approximate rational entries: coords of units found so far on a
 * sublattice of maximal rank (sublambda)
 * *ptkR = regulator of sublambda = multiple of regulator of lambda
 * Compute R = true regulator of lambda.
 *
 * If c := Rz ~ 1, by Dirichlet's formula, then lambda is the full group of
 * units AND the full set of relations for the class group has been computed.
 *
 * In fact z is a very rough approximation and we only expect 0.75 < Rz < 1.3
 *
 * Output: *ptkR = R, *ptU = basis of fundamental units (in terms lambda) */
static int
compute_R(GEN lambda, GEN z, GEN *ptL, GEN *ptkR)
{
  pari_sp av = avma;
  long r;
  GEN L,H,D,den,R;
  double c;

  if (DEBUGLEVEL) { fprintferr("\n#### Computing check\n"); flusherr(); }
  D = gmul2n(gmul(*ptkR,z), 1); /* bound for denom(lambda) */
  lambda = bestappr_noer(lambda,D);
  if (!lambda)
  {
    if (DEBUGLEVEL) fprintferr("truncation error in bestappr\n");
    return fupb_PRECI;
  }
  den = Q_denom(lambda);
  if (gcmp(den,D) > 0)
  {
    if (DEBUGLEVEL) fprintferr("D = %Z\nden = %Z\n",D,
                    lgefint(den) <= DEFAULTPREC? den: itor(den,3));
    return fupb_PRECI;
  }
  L = Q_muli_to_int(lambda, den);
  H = hnfall_i(L, NULL, 1); r = lg(H)-1;

  /* tentative regulator */
  R = gmul(*ptkR, gdiv(dethnf_i(H), gpowgs(den, r)));
  c = gtodouble(gmul(R,z)); /* should be n (= 1 if we are done) */
  if (DEBUGLEVEL)
  {
    msgtimer("bestappr/regulator");
    fprintferr("\n#### Tentative regulator : %Z\n", gprec_w(R,3));
    fprintferr("\n ***** check = %f\n",c);
  }
  if (c < 0.75 || c > 1.3) { avma = av; return fupb_RELAT; }
  *ptkR = R; *ptL = L; return fupb_NONE;
}

/* find the smallest (wrt norm) among I, I^-1 and red(I^-1) */
static GEN
inverse_if_smaller(GEN nf, GEN I, long prec)
{
  GEN J, d, dmin, I1;

  J = (GEN)I[1];
  dmin = dethnf_i(J); I1 = idealinv(nf,I);
  J = (GEN)I1[1]; J = gmul(J,denom(J)); I1[1] = (long)J;
  d = dethnf_i(J); if (cmpii(d,dmin) < 0) {I=I1; dmin=d;}
  /* try reducing (often _increases_ the norm) */
  I1 = ideallllred(nf,I1,NULL,prec);
  J = (GEN)I1[1];
  d = dethnf_i(J); if (cmpii(d,dmin) < 0) I=I1;
  return I;
}

/* in place */
static void
neg_row(GEN U, long i)
{
  GEN c = U + lg(U)-1;
  for (; c>U; c--) coeff(c,i,0) = lneg(gcoeff(c,i,0));
}

static void
setlg_col(GEN U, long l)
{
  GEN c = U + lg(U)-1;
  for (; c>U; c--) setlg(*c, l);
}

/* compute class group (clg1) + data for isprincipal (clg2) */
static void
class_group_gen(GEN nf,GEN W,GEN C,GEN Vbase,long prec, GEN nf0,
                GEN *ptclg1,GEN *ptclg2)
{
  GEN z,G,Ga,ga,GD,cyc,X,Y,D,U,V,Ur,Ui,Uir,I,J;
  long i,j,lo,lo0;

  if (DEBUGLEVEL)
    { fprintferr("\n#### Computing class group generators\n"); (void)timer2(); }
  D = smithall(W,&U,&V); /* UWV = D, D diagonal, G = g Ui (G=new gens, g=old) */
  Ui = ginv(U);
  lo0 = lo = lg(D);
 /* we could set lo = lg(cyc) and truncate all matrices below
  *   setlg_col(D && U && Y, lo) + setlg(D && V && X && Ui, lo)
  * but it's not worth the complication:
  * 1) gain is negligible (avoid computing z^0 if lo < lo0)
  * 2) when computing ga, the products XU and VY use the original matrices
  */
  Ur  = reducemodHNF(U, D, &Y);
  Uir = reducemodHNF(Ui,W, &X);
 /* [x] = logarithmic embedding of x (arch. component)
  * NB: z = idealred(I) --> I = y z[1], with [y] = - z[2]
  * P invertible diagonal matrix (\pm 1) which is only implicitly defined
  * G = g Uir P + [Ga],  Uir = Ui + WX
  * g = G P Ur  + [ga],  Ur  = U + DY */
  G = cgetg(lo,t_VEC);
  Ga= cgetg(lo,t_VEC);
  z = init_famat(NULL);
  if (!nf0) nf0 = nf;
  for (j=1; j<lo; j++)
  {
    GEN p1 = gcoeff(Uir,1,j);
    z[1]=Vbase[1]; I = idealpowred(nf0,z,p1,prec);
    for (i=2; i<lo0; i++)
    {
      p1 = gcoeff(Uir,i,j);
      if (signe(p1))
      {
	z[1]=Vbase[i]; J = idealpowred(nf0,z,p1,prec);
	I = idealmulh(nf0,I,J);
	I = ideallllred(nf0,I,NULL,prec);
      }
    }
    J = inverse_if_smaller(nf0, I, prec);
    if (J != I)
    { /* update wrt P */
      neg_row(Y ,j); V[j] = lneg((GEN)V[j]);
      neg_row(Ur,j); X[j] = lneg((GEN)X[j]);
    }
    G[j] = (long)J[1]; /* generator, order cyc[j] */
    Ga[j]= lneg(famat_to_arch(nf, (GEN)J[2], prec));
  }
  /* at this point Y = PY, Ur = PUr, V = VP, X = XP */

  /* G D =: [GD] = g (UiP + W XP) D + [Ga]D = g W (VP + XP D) + [Ga]D
   * NB: DP = PD and Ui D = W V. gW is given by (first lo0-1 cols of) C
   */
  GD = gadd(act_arch(gadd(V, gmul(X,D)), C),
            act_arch(D, Ga));
  /* -[ga] = [GD]PY + G PU - g = [GD]PY + [Ga] PU + gW XP PU
                               = gW (XP PUr + VP PY) + [Ga]PUr */
  ga = gadd(act_arch(gadd(gmul(X,Ur), gmul(V,Y)), C),
            act_arch(Ur, Ga));
  ga = gneg(ga);
  /* TODO: could (LLL)reduce ga and GD mod units ? */

  cyc = cgetg(lo,t_VEC); /* elementary divisors */
  for (j=1; j<lo; j++)
  {
    cyc[j] = coeff(D,j,j);
    if (gcmp1((GEN)cyc[j]))
    { /* strip useless components */
      lo = j; setlg(cyc,lo); setlg_col(Ur,lo);
      setlg(G,lo); setlg(Ga,lo); setlg(GD,lo); break;
    }
  }
  z = cgetg(4,t_VEC); *ptclg1 = z;
  z[1]=(long)dethnf_i(W);
  z[2]=(long)cyc;
  z[3]=(long)G;
  z = cgetg(4,t_VEC); *ptclg2 = z;
  z[1]=(long)Ur;
  z[2]=(long)ga;
  z[3]=(long)GD;
  if (DEBUGLEVEL) msgtimer("classgroup generators");
}

/* SMALLBUCHINIT */

static GEN
decode_pr_lists(GEN nf, GEN pfc)
{
  long i, p, pmax, n = degpol(nf[1]), l = lg(pfc);
  GEN t, L;

  pmax = 0;
  for (i=1; i<l; i++)
  {
    t = (GEN)pfc[i]; p = itos(t) / n;
    if (p > pmax) pmax = p;
  }
  L = cgetg(pmax+1, t_VEC);
  for (p=1; p<=pmax; p++) L[p] = 0;
  for (i=1; i<l; i++)
  {
    t = (GEN)pfc[i]; p = itos(t) / n;
    if (!L[p]) L[p] = (long)primedec(nf, stoi(p));
  }
  return L;
}

static GEN
decodeprime(GEN T, GEN L, long n)
{
  long t = itos(T);
  return gmael(L, t/n, t%n + 1);
}
static GEN
codeprime(GEN L, long N, GEN pr)
{
  long p = itos((GEN)pr[1]);
  return utoi( N*p + pr_index((GEN)L[p], pr)-1 );
}

static GEN
codeprimes(GEN Vbase, long N)
{
  GEN v, L = get_pr_lists(Vbase, N, 1);
  long i, l = lg(Vbase);
  v = cgetg(l, t_VEC);
  for (i=1; i<l; i++) v[i] = (long)codeprime(L, N, (GEN)Vbase[i]);
  return v;
}

/* compute principal ideals corresponding to (gen[i]^cyc[i]) */
static GEN
makecycgen(GEN bnf)
{
  GEN cyc,gen,h,nf,y,GD,D;
  long e,i,l;

  if (DEBUGLEVEL) err(warner,"completing bnf (building cycgen)");
  nf = checknf(bnf);
  cyc = gmael3(bnf,8,1,2); D = diagonal(cyc);
  gen = gmael3(bnf,8,1,3); GD = gmael(bnf,9,3);
  l = lg(gen); h = cgetg(l, t_VEC);
  for (i=1; i<l; i++)
  {
    if (cmpis((GEN)cyc[i], 16) < 0)
    {
      GEN N = dethnf_i((GEN)gen[i]);
      y = isprincipalarch(bnf,(GEN)GD[i], N, (GEN)cyc[i], gun, &e);
      if (y && !fact_ok(nf,y,NULL,gen,(GEN)D[i])) y = NULL;
      if (y) { h[i] = (long)to_famat_all(y,gun); continue; }
    }
    y = isprincipalfact(bnf, gen, (GEN)D[i], NULL, nf_GENMAT|nf_FORCE);
    h[i] = y[2];
  }
  return h;
}

/* compute principal ideals corresponding to bnf relations */
static GEN
makematal(GEN bnf)
{
  GEN W,B,pFB,nf,ma, WB_C;
  long lW,lma,j,prec;

  if (DEBUGLEVEL) err(warner,"completing bnf (building matal)");
  W   = (GEN)bnf[1];
  B   = (GEN)bnf[2];
  WB_C= (GEN)bnf[4];
  nf  = (GEN)bnf[7];
  lW=lg(W)-1; lma=lW+lg(B);
  pFB = get_Vbase(bnf);
  ma = cgetg(lma,t_MAT);

  prec = prec_arch(bnf);
  for (j=1; j<lma; j++)
  {
    long c = getrand(), e;
    GEN ex = (j<=lW)? (GEN)W[j]: (GEN)B[j-lW];
    GEN C = (j<=lW)? NULL: (GEN)pFB[j];
    GEN dx, Nx = get_norm_fact_primes(pFB, ex, C, &dx);
    GEN y = isprincipalarch(bnf,(GEN)WB_C[j], Nx,gun, dx, &e);
    if (y && !fact_ok(nf,y,C,pFB,ex)) y = NULL;
    if (y)
    {
      if (DEBUGLEVEL>1) fprintferr("*%ld ",j);
      ma[j] = (long)y; continue;
    }

    if (!y) y = isprincipalfact(bnf,pFB,ex,C, nf_GENMAT|nf_FORCE|nf_GIVEPREC);
    if (typ(y) != t_INT)
    {
      if (DEBUGLEVEL>1) fprintferr("%ld ",j);
      ma[j] = y[2]; continue;
    }

    prec = itos(y); j--;
    if (DEBUGLEVEL) err(warnprec,"makematal",prec);
    nf = nfnewprec(nf,prec);
    bnf = bnfinit0(nf,1,NULL,prec); (void)setrand(c);
  }
  if (DEBUGLEVEL>1) fprintferr("\n");
  return ma;
}

#define MATAL  1
#define CYCGEN 2
GEN
check_and_build_cycgen(GEN bnf) {
  return check_and_build_obj(bnf, CYCGEN, &makecycgen);
}
GEN
check_and_build_matal(GEN bnf) {
  return check_and_build_obj(bnf, MATAL, &makematal);
}

GEN
smallbuchinit(GEN pol, double bach, double bach2, long nbrelpid, long prec)
{
  GEN y, bnf, nf, res, p1;
  pari_sp av = avma;

  if (typ(pol)==t_VEC) bnf = checkbnf(pol);
  else
  {
    const long fl = nf_INIT | nf_UNITS | nf_FORCE;
    bnf = buchall(pol, bach, bach2, nbrelpid, fl, prec);
    bnf = checkbnf_discard(bnf);
  }
  nf  = (GEN)bnf[7];
  res = (GEN)bnf[8];

  y = cgetg(13,t_VEC);
  y[1] = nf[1];
  y[2] = mael(nf,2,1);
  y[3] = nf[3];
  y[4] = nf[7];
  y[5] = nf[6];
  y[6] = mael(nf,5,5);
  y[7] = bnf[1];
  y[8] = bnf[2];
  y[9] = (long)codeprimes((GEN)bnf[5], degpol(nf[1]));

  p1 = cgetg(3, t_VEC);
  p1[1] = mael(res,4,1);
  p1[2] = (long)algtobasis(bnf,gmael(res,4,2));
  y[10] = (long)p1;

  y[11] = (long)algtobasis(bnf, (GEN)res[5]);
  (void)check_and_build_matal(bnf);
  y[12] = bnf[10]; return gerepilecopy(av, y);
}

static GEN
get_regulator(GEN mun)
{
  pari_sp av = avma;
  GEN A;

  if (lg(mun)==1) return gun;
  A = gtrans( real_i(mun) );
  setlg(A, lg(A)-1);
  return gerepileupto(av, gabs(det(A), 0));
}

/* return corrected archimedian components for elts of x (vector)
 * (= log(sigma_i(x)) - log(|Nx|) / [K:Q]) */
static GEN
get_archclean(GEN nf, GEN x, long prec, int units)
{
  long k,N, la = lg(x);
  GEN M = cgetg(la,t_MAT);

  if (la == 1) return M;
  N = degpol(nf[1]);
  for (k=1; k<la; k++)
  {
    GEN c = get_arch(nf, (GEN)x[k], prec);
    if (!units) c = cleanarch(c, N, prec);
    M[k] = (long)c;
  }
  return M;
}

static void
my_class_group_gen(GEN bnf, long prec, GEN nf0, GEN *ptcl, GEN *ptcl2)
{
  GEN W=(GEN)bnf[1], C=(GEN)bnf[4], nf=(GEN)bnf[7];
  class_group_gen(nf,W,C,get_Vbase(bnf),prec,nf0, ptcl,ptcl2);
}

GEN
bnfnewprec(GEN bnf, long prec)
{
  GEN nf0 = (GEN)bnf[7], nf, res, funits, mun, matal, clgp, clgp2, y;
  pari_sp av = avma;
  long r1, r2, prec1;

  bnf = checkbnf(bnf);
  if (prec <= 0) return nfnewprec(checknf(bnf),prec);
  nf = (GEN)bnf[7];
  nf_get_sign(nf, &r1, &r2);
  funits = algtobasis(nf, check_units(bnf,"bnfnewprec"));

  prec1 = prec;
  if (r2 > 1 || r1 != 0)
    prec += 1 + (gexpo(funits) >> TWOPOTBITS_IN_LONG);
  nf = nfnewprec(nf0,prec);
  mun = get_archclean(nf,funits,prec,1);
  if (prec != prec1) { mun = gprec_w(mun,prec1); prec = prec1; }
  matal = check_and_build_matal(bnf);

  y = dummycopy(bnf);
  y[3] = (long)mun;
  y[4] = (long)get_archclean(nf,matal,prec,0);
  y[7] = (long)nf;
  my_class_group_gen(y,prec,nf0, &clgp,&clgp2);
  res = dummycopy((GEN)bnf[8]);
  res[1] = (long)clgp;
  res[2] = (long)get_regulator(mun);
  y[8] = (long)res;
  y[9] = (long)clgp2; return gerepilecopy(av, y);
}

GEN
bnrnewprec(GEN bnr, long prec)
{
  GEN y = cgetg(7,t_VEC);
  long i;
  checkbnr(bnr);
  y[1] = (long)bnfnewprec((GEN)bnr[1],prec);
  for (i=2; i<7; i++) y[i]=lcopy((GEN)bnr[i]);
  return y;
}

static void
nfbasic_from_sbnf(GEN sbnf, nfbasic_t *T)
{
  T->x    = (GEN)sbnf[1];
  T->dK   = (GEN)sbnf[3];
  T->bas  = (GEN)sbnf[4];
  T->index= get_nfindex(T->bas);
  T->r1   = itos((GEN)sbnf[2]);
  T->dx   = NULL;
  T->lead = NULL;
  T->basden = NULL;
}

static GEN
get_clfu(GEN clgp, GEN reg, GEN zu, GEN fu, long fl)
{
  long l = (fl & nf_UNITS)? 6
                          : (fl & nf_ROOT1)? 5: 4;
  GEN z = cgetg(6, t_VEC);
  z[1] = (long)clgp;
  z[2] = (long)reg;
  z[3] = un; /* DUMMY */
  z[4] = (long)zu;
  z[5] = (long)fu; setlg(z, l); return z;
}

static GEN
buchall_end(GEN nf,long fl,GEN res, GEN clg2, GEN W, GEN B, GEN A, GEN C,
            GEN Vbase)
{
  GEN p1, z;
  if (! (fl & nf_INIT))
  {
    GEN x = cgetg(5, t_VEC);
    x[1]=nf[1];
    x[2]=nf[2]; p1=cgetg(3,t_VEC); p1[1]=nf[3]; p1[2]=nf[4];
    x[3]=(long)p1;
    x[4]=nf[7];
    z=cgetg(2,t_MAT); z[1]=(long)concatsp(x, res); return z;
  }
  z=cgetg(11,t_VEC);
  z[1]=(long)W;
  z[2]=(long)B;
  z[3]=(long)A;
  z[4]=(long)C;
  z[5]=(long)Vbase;
  z[6]=zero;
  z[7]=(long)nf;
  z[8]=(long)res;
  z[9]=(long)clg2;
  z[10]=zero; /* dummy: we MUST have lg(bnf) != lg(nf) */
  return z;
}

GEN
bnfmake(GEN sbnf, long prec)
{
  long j, k, l, n;
  pari_sp av = avma;
  GEN p1, bas, ro, nf, A, fu, L;
  GEN pfc, C, clgp, clgp2, res, y, W, zu, matal, Vbase;
  nfbasic_t T;

  if (typ(sbnf) != t_VEC || lg(sbnf) != 13) err(typeer,"bnfmake");
  if (prec < DEFAULTPREC) prec = DEFAULTPREC;

  nfbasic_from_sbnf(sbnf, &T);
  ro = (GEN)sbnf[5];
  if (prec > gprecision(ro)) ro = get_roots(T.x,T.r1,prec);
  nf = nfbasic_to_nf(&T, ro, prec);
  bas = (GEN)nf[7];

  p1 = (GEN)sbnf[11]; l = lg(p1); fu = cgetg(l, t_VEC);
  for (k=1; k < l; k++) fu[k] = lmul(bas, (GEN)p1[k]);
  A = get_archclean(nf,fu,prec,1);

  prec = gprecision(ro);
  matal = check_and_build_matal(sbnf);
  C = get_archclean(nf,matal,prec,0);

  pfc = (GEN)sbnf[9];
  l = lg(pfc);
  Vbase = cgetg(l,t_COL);
  L = decode_pr_lists(nf, pfc);
  n = degpol(nf[1]);
  for (j=1; j<l; j++) Vbase[j] = (long)decodeprime((GEN)pfc[j], L, n);
  W = (GEN)sbnf[7];
  class_group_gen(nf,W,C,Vbase,prec,NULL, &clgp,&clgp2);

  p1 = cgetg(3,t_VEC); zu = (GEN)sbnf[10];
  p1[1] = zu[1];
  p1[2] = lmul(bas,(GEN)zu[2]); zu = p1;

  res = get_clfu(clgp, get_regulator(A), zu, fu, nf_UNITS);
  y = buchall_end(nf,nf_INIT,res,clgp2,W,(GEN)sbnf[8],A,C,Vbase);
  y[10] = sbnf[12]; return gerepilecopy(av,y);
}

static GEN
classgroupall(GEN P, GEN data, long flag, long prec)
{
  double bach = 0.3, bach2 = 0.3;
  long fl, lx, nbrelpid = 4;

  if (!data) lx = 1;
  else
  {
    lx = lg(data);
    if (typ(data)!=t_VEC || lx > 5)
      err(talker,"incorrect parameters in classgroup");
  }
  switch(lx)
  {
    case 4: nbrelpid = itos((GEN)data[3]);
    case 3: bach2 = gtodouble( (GEN)data[2]);
    case 2: bach  = gtodouble( (GEN)data[1]);
  }
  switch(flag)
  {
    case 0: fl = nf_INIT | nf_UNITS; break;
    case 1: fl = nf_INIT | nf_UNITS | nf_FORCE; break;
    case 2: fl = nf_INIT | nf_ROOT1; break;
    case 3: return smallbuchinit(P, bach, bach2, nbrelpid, prec);
    case 4: fl = nf_UNITS; break;
    case 5: fl = nf_UNITS | nf_FORCE; break;
    case 6: fl = 0; break;
    default: err(flagerr,"classgroupall");
      return NULL; /* not reached */
  }
  return buchall(P, bach, bach2, nbrelpid, fl, prec);
}

GEN
bnfclassunit0(GEN P, long flag, GEN data, long prec)
{
  if (typ(P)==t_INT) return quadclassunit0(P,0,data,prec);
  if (flag < 0 || flag > 2) err(flagerr,"bnfclassunit");
  return classgroupall(P,data,flag+4,prec);
}

GEN
bnfinit0(GEN P, long flag, GEN data, long prec)
{
#if 0
  /* TODO: synchronize quadclassunit output with bnfinit */
  if (typ(P)==t_INT)
  {
    if (flag<4) err(impl,"specific bnfinit for quadratic fields");
    return quadclassunit0(P,0,data,prec);
  }
#endif
  if (flag < 0 || flag > 3) err(flagerr,"bnfinit");
  return classgroupall(P,data,flag,prec);
}

GEN
classgrouponly(GEN P, GEN data, long prec)
{
  pari_sp av = avma;
  GEN z;

  if (typ(P)==t_INT)
  {
    z=quadclassunit0(P,0,data,prec); setlg(z,4);
    return gerepilecopy(av,z);
  }
  z=(GEN)classgroupall(P,data,6,prec)[1];
  return gerepilecopy(av,(GEN)z[5]);
}

GEN
regulator(GEN P, GEN data, long prec)
{
  pari_sp av = avma;
  GEN z;

  if (typ(P)==t_INT)
  {
    if (signe(P)>0)
    {
      z=quadclassunit0(P,0,data,prec);
      return gerepilecopy(av,(GEN)z[4]);
    }
    return gun;
  }
  z=(GEN)classgroupall(P,data,6,prec)[1];
  return gerepilecopy(av,(GEN)z[6]);
}

GEN
cgetc_col(long n, long prec)
{
  GEN c = cgetg(n+1,t_COL);
  long i;
  for (i=1; i<=n; i++) c[i] = (long)cgetc(prec);
  return c;
}

static GEN
buchall_for_degree_one_pol(GEN nf, long flun)
{
  GEN W,B,A,C,Vbase,res;
  GEN fu=cgetg(1,t_VEC), R=gun, zu=cgetg(3,t_VEC);
  GEN clg1=cgetg(4,t_VEC), clg2=cgetg(4,t_VEC);

  clg1[1]=un; clg1[2]=clg1[3]=clg2[2]=clg2[3]=lgetg(1,t_VEC);
  clg2[1]=lgetg(1,t_MAT);
  zu[1]=deux; zu[2]=lnegi(gun);
  W=B=A=C=cgetg(1,t_MAT);
  Vbase=cgetg(1,t_COL);

  res = get_clfu(clg1, R, zu, fu, flun);
  return buchall_end(nf,flun,res,clg2,W,B,A,C,Vbase);
}

/* return (small set of) indices of columns generating the same lattice as x.
 * Assume HNF(x) is inexpensive (few rows, many columns).
 * Dichotomy approach since interesting columns may be at the very end */
GEN
extract_full_lattice(GEN x)
{
  long dj, j, k, l = lg(x);
  GEN h, h2, H, v;

  if (l < 200) return NULL; /* not worth it */

  v = cget1(l, t_VECSMALL);
  H = hnfall_i(x, NULL, 1);
  h = cgetg(1, t_MAT);
  dj = 1;
  for (j = 1; j < l; )
  {
    pari_sp av = avma;
    long lv = lg(v);

    for (k = 0; k < dj; k++) v[lv+k] = j+k;
    setlg(v, lv + dj);
    h2 = hnfall_i(vecextract_p(x, v), NULL, 1);
    if (gegal(h, h2))
    { /* these dj columns can be eliminated */
      avma = av; setlg(v, lv);
      j += dj;
      if (j >= l) break;
      dj <<= 1;
      if (j + dj >= l) { dj = (l - j) >> 1; if (!dj) dj = 1; }
    }
    else if (dj > 1)
    { /* at least one interesting column, try with first half of this set */
      avma = av; setlg(v, lv);
      dj >>= 1; /* > 0 */
    }
    else
    { /* this column should be kept */
      if (gegal(h2, H)) break;
      h = h2; j++;
    }
  }
  return v;
}

static GEN
nf_cloneprec(GEN nf, long prec, GEN *pnf)
{
  pari_sp av = avma;
  nf = gclone( nfnewprec_i(nf, prec) );
  if (*pnf) gunclone(*pnf);
  avma = av; return *pnf = nf;
}

static void
init_rel(RELCACHE_t *cache, FB_t *F, long RU)
{
  const long RELSUP = 5;
  const long n = F->KC + RU-1 + RELSUP; /* expected # of needed relations */
  long i, j, k, p;
  GEN c, P;
  REL_t *rel;

  if (DEBUGLEVEL) fprintferr("KCZ = %ld, KC = %ld, n = %ld\n", F->KCZ,F->KC,n);
  reallocate(cache, 10*n + 50); /* make room for lots of relations */
  cache->chk = cache->base;
  cache->end = cache->base + n;
  for (rel = cache->base + 1, i = 1; i <= F->KCZ; i++)
  { /* trivial relations (p) = prod P^e */
    p = F->FB[i]; P = F->LV[p];
    if (!isclone(P)) continue;

    /* all prime divisors in FB */
    c = col_0(F->KC); k = F->iLP[p];
    rel->nz = k+1;
    rel->R  = c; c += k;
    rel->ex = NULL;
    rel->m  = NULL;
    rel->pow= NULL; /* = F->pow */
    for (j = lg(P)-1; j; j--) c[j] = itos(gmael(P,j,3));
    rel++;
  }
  cache->last = rel - 1;
  if (DEBUGLEVEL)
    for (i = 1, rel = cache->base + 1; rel <= cache->last; rel++, i++)
      dbg_rel(i, rel->R);
}

static void
shift_embed(GEN G, GEN Gtw, long a, long r1)
{
  long j, k, l = lg(G);
  if (a <= r1)
    for (j=1; j<l; j++) coeff(G,a,j) = coeff(Gtw,a,j);
  else
  {
    k = (a<<1) - r1;
    for (j=1; j<l; j++)
    {
      coeff(G,k-1,j) = coeff(Gtw,k-1,j);
      coeff(G,k  ,j) = coeff(Gtw,k,  j);
    }
  }
}

/* G where embeddings a and b are multiplied by 2^10 */
static GEN
shift_G(GEN G, GEN Gtw, long a, long b, long r1)
{
  GEN g = dummycopy(G);
  if (a != b) shift_embed(g,Gtw,a,r1);
  shift_embed(g,Gtw,b,r1); return g;
}

static GEN
compute_vecG(GEN nf, GEN *pG0, long n)
{
  GEN G0, Gtw0, vecG, G = gmael(nf,5,2);
  long e, i, j, ind, r1 = nf_get_r1(nf), r = lg(G)-1;
  if (n == 1) return _vec( ground(G) );
  for (e = 4; ; e <<= 1)
  {
    G0 = ground(G); Gtw0 = ground(gmul2n(G, 10));
    if (rank(G0) == r && rank(Gtw0) == r) break; /* maximal rank ? */
    G = gmul2n(G, e);
  }
  vecG = cgetg(1 + n*(n+1)/2,t_VEC);
  for (ind=j=1; j<=n; j++)
    for (i=1; i<=j; i++) vecG[ind++] = (long)shift_G(G0,Gtw0,i,j,r1);
  if (DEBUGLEVEL) msgtimer("weighted G matrices");
  *pG0 = G0; return vecG;
}

static GEN
buch(GEN *pnf, double cbach, double cbach2, long nbrelpid, long flun, 
     long PRECREG)
{
  pari_sp av, av2;
  long N, R1, R2, RU, LIMC, LIMC2, lim, zc, i, jid;
  long nreldep, sfb_trials, need, precdouble = 0, precadd = 0;
  double drc, LOGD, LOGD2;
  GEN G0, fu, zu, nf, D, A, W, R, Res, z, h, L_jid, PERM;
  GEN res, L, resc, B, C, lambda, dep, clg1, clg2, Vbase;
  char *precpb = NULL;
  const int minsFB = 3;
  RELCACHE_t cache;
  FB_t F;

  nf = *pnf; *pnf = NULL;
  N = degpol(nf[1]);
  if (N <= 1) return buchall_for_degree_one_pol(nf, flun);
  zu = rootsof1(nf);
  zu[2] = lmul((GEN)nf[7],(GEN)zu[2]);
  if (DEBUGLEVEL) msgtimer("initalg & rootsof1");

  nf_get_sign(nf, &R1, &R2); RU = R1+R2;
  F.vecG = compute_vecG(nf, &G0, min(RU, 9));
  D = (GEN)nf[3]; drc = fabs(gtodouble(D));
  LOGD = log(drc); LOGD2 = LOGD*LOGD;
  lim = (long) (exp(-(double)N) * sqrt(2*PI*N*drc) * pow(4/PI,(double)R2));
  if (lim < 3) lim = 3;
  if (cbach > 12.) cbach = 12.;
  if (cbach <= 0.) err(talker,"Bach constant <= 0 in buch");

  /* resc ~ sqrt(D) w / 2^r1 (2pi)^r2 = hR / Res(zeta_K, s=1) */
  resc = gdiv(mulri(gsqrt(absi(D),DEFAULTPREC), (GEN)zu[1]),
              gmul2n(gpowgs(Pi2n(1,DEFAULTPREC), R2), R1));
  if (DEBUGLEVEL) fprintferr("R1 = %ld, R2 = %ld\nD = %Z\n",R1,R2, D);
  av = avma; cache.base = NULL; F.subFB = NULL;
  cbach /= 2;

START:
  avma = av; 
  if (cache.base) delete_cache(&cache);
  if (F.subFB) delete_FB(&F);
  cbach = check_bach(cbach,12.);
  LIMC = (long)(cbach*LOGD2);
  if (LIMC < 20) { LIMC = 20; cbach = (double)LIMC / LOGD2; }
  LIMC2 = max(3 * N, (long)(cbach2*LOGD2));
  if (LIMC2 < LIMC) LIMC2 = LIMC;
  if (DEBUGLEVEL) { fprintferr("LIMC = %ld, LIMC2 = %ld\n",LIMC,LIMC2); }

  Res = FBgen(&F, nf, LIMC2, LIMC);
  if (!Res || !subFBgen(&F, nf, min(lim,LIMC2) + 0.5, minsFB)) goto START;
  PERM = dummycopy(F.perm); /* to be restored in case of precision increase */
  av2 = avma;
  init_rel(&cache, &F, RU); /* trivial relations */
  if (nbrelpid > 0) {
    small_norm(&cache,&F,G0,LOGD,nf,nbrelpid,LIMC2); avma = av2;
  }

  /* Random relations */
  W = L_jid = NULL;
  jid = sfb_trials = nreldep = 0;
  need = cache.end - cache.last;
  if (need > 0)
  {
    if (DEBUGLEVEL) fprintferr("\n#### Looking for random relations\n");
MORE:
    pre_allocate(&cache, need); cache.end = cache.last + need;
    if (++nreldep > MAXRELSUP) {
      F.sfb_chg = sfb_INCREASE;
      if (++sfb_trials > SFB_MAX) goto START;
    }
    if (F.sfb_chg) {
      if (!subFB_change(&F, nf, L_jid)) goto START;
      jid = nreldep = 0;
    }
    if (F.newpow) powFBgen(&F, &cache, nf);
    if (!F.sfb_chg && !rnd_rel(&cache,&F, nf, L_jid, &jid)) goto START;
    L_jid = NULL;
  }
  if (precpb)
  {
PRECPB:
    if (precadd) { PRECREG += precadd; precadd = 0; }
    else           PRECREG = (PRECREG<<1)-2;
    if (DEBUGLEVEL)
    {
      char str[64]; sprintf(str,"buchall (%s)",precpb);
      err(warnprec,str,PRECREG);
    }
    precdouble++; precpb = NULL;
    nf = nf_cloneprec(nf, PRECREG, pnf);
    if (F.pow && F.pow->arc) { gunclone(F.pow->arc); F.pow->arc = NULL; }
    for (i = 1; i < lg(PERM); i++) F.perm[i] = PERM[i];
    cache.chk = cache.base; W = NULL; /* recompute arch components + reduce */
  }
  { /* Reduce relation matrices */
    long l = cache.last - cache.chk + 1, j;
    GEN M = gmael(nf, 5, 1), mat = cgetg(l, t_VEC), emb = cgetg(l, t_MAT);
    int first = (W == NULL); /* never reduced before */
    REL_t *rel;
    
    if (F.pow && !F.pow->arc) powFB_fill(&cache, M);
    for (j=1,rel = cache.chk + 1; rel <= cache.last; rel++,j++)
    {
      mat[j] = (long)rel->R;
      emb[j] = (long)get_log_embed(rel, M, RU, R1, PRECREG);
    }
    if (first) {
      C = emb;
      W = hnfspec_i((long**)mat, F.perm, &dep, &B, &C, lg(F.subFB)-1);
    }
    else
      W = hnfadd_i(W, F.perm, &dep, &B, &C, mat, emb);
    gerepileall(av2, 4, &W,&C,&B,&dep);
    cache.chk = cache.last;
    need = lg(dep)>1? lg(dep[1])-1: lg(B[1])-1;
    if (need)
    { /* dependent rows */
      if (need > 5)
      {
        if (need > 20 && !first) F.sfb_chg = sfb_CHANGE;
        L_jid = vecextract_i(F.perm, 1, need);
        vecsmall_sort(L_jid); jid = 0; 
      }
      goto MORE;
    }
  }

  zc = (cache.last - cache.base) - (lg(B)-1) - (lg(W)-1);
  A = vecextract_i(C, 1, zc); /* cols corresponding to units */
  R = compute_multiple_of_R(A, RU, N, &lambda);
  if (!R)
  { /* not full rank for units */
    if (DEBUGLEVEL) fprintferr("regulator is zero.\n");
    goto MORE;
  }
  if (!lambda) { precpb = "bestappr"; goto PRECPB; }

  h = dethnf_i(W);
  if (DEBUGLEVEL) fprintferr("\n#### Tentative class number: %Z\n", h);

  z = mulrr(Res, resc); /* ~ hR if enough relations, a multiple otherwise */
  switch (compute_R(lambda, divir(h,z), &L, &R))
  {
    case fupb_RELAT: goto MORE; /* not enough relations */
    case fupb_PRECI: /* prec problem unless we cheat on Bach constant */
      if ((precdouble&7) < 7 || cbach>2) { precpb = "compute_R"; goto PRECPB; }
      goto START;
  }
  /* DONE */

  if (F.KCZ2 > F.KCZ)
  {
    if (F.newpow) powFBgen(&F, NULL, nf);
    if (F.sfb_chg) {
      if (!subFB_change(&F, nf, L_jid)) goto START;
      powFBgen(&F, NULL, nf);
    }
    if (!be_honest(&F, nf)) goto START;
  }
  F.KCZ2 = 0; /* be honest only once */

  /* fundamental units */
  if (flun & (nf_UNITS|nf_INIT))
  {
    GEN U, H, v = extract_full_lattice(L); /* L may be very large */
    if (v)
    {
      A = vecextract_p(A, v);
      L = vecextract_p(L, v);
    }
    /* arch. components of fund. units */
    H = hnflll_i(L, &U, 1); U = vecextract_i(U, lg(U)-(RU-1), lg(U)-1);
    U = gmul(U, lll(H, DEFAULTPREC));
    A = cleanarch(gmul(A, U), N, PRECREG);
    if (DEBUGLEVEL) msgtimer("cleanarch");
  }
  fu = NULL;
  if (flun & nf_UNITS)
  {
    long e;
    fu = getfu(nf, &A, flun, &e, PRECREG);
    if (e <= 0 && (flun & nf_FORCE))
    {
      if (e < 0) precadd = (DEFAULTPREC-2) + ((-e) >> TWOPOTBITS_IN_LONG);
      precpb = "getfu"; goto PRECPB;
    }
  }
  delete_cache(&cache); delete_FB(&F);

  /* class group generators */
  i = lg(C)-zc; C += zc; C[0] = evaltyp(t_MAT)|evallg(i);
  C = cleanarch(C, N, PRECREG);
  Vbase = vecextract_p(F.LP, F.perm);
  class_group_gen(nf,W,C,Vbase,PRECREG,NULL, &clg1, &clg2);
  res = get_clfu(clg1, R, zu, fu, flun);
  return buchall_end(nf,flun,res,clg2,W,B,A,C,Vbase);
}

GEN
buchall(GEN P, double cbach, double cbach2, long nbrelpid, long flun, long prec)
{
  pari_sp av = avma;
  long PRECREG = max(prec, MEDDEFAULTPREC);
  GEN z, nf, CHANGE = NULL;

  if (DEBUGLEVEL) (void)timer2();
  P = get_nfpol(P, &nf);
  if (!nf)
  {
    nf = initalg(P, PRECREG); /* P non-monic and nfinit CHANGEd it ? */
    if (lg(nf)==3) { CHANGE = (GEN)nf[2]; nf = (GEN)nf[1]; }
  }
  z = buch(&nf, cbach, cbach2, nbrelpid, flun, PRECREG);
  if (CHANGE) { GEN v=cgetg(3,t_VEC); v[1]=(long)z; v[2]=(long)CHANGE; z = v; }
  z = gerepilecopy(av, z); if (nf) gunclone(nf);
  return z;
}
