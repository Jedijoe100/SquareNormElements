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
/**           Dirichlet series through Euler product               **/
/**                                                                **/
/********************************************************************/
#include "pari.h"
#include "paripriv.h"

static void
err_direuler(GEN x)
{ pari_err_DOMAIN("direuler","constant term","!=", gen_1,x); }

static long
dirmuleuler_small(GEN V, GEN v, long n, ulong p, GEN s)
{
  long d = lg(s)-2, b = lg(V)-1;
  long i,j;
  long m = n;
  long q = 1;
  for (i=1, q=p; i<=d; i++, q*=p)
  {
    GEN aq = gel(s,i+1);
    if (gequal0(aq)) continue;
    for(j=1; j<=m; j++)
    {
      long nj = v[j]*q;
      GEN Vj = gel(V,v[j]);
      if (nj > b) continue;
      gel(V,nj) = gmul(aq, Vj);
      v[++n] = nj;
    }
  }
  return n;
}

static void
dirmuleuler_large(GEN x, ulong p, GEN ap)
{
  if (!gequal0(ap))
  {
    long b = lg(x)-1, j, m = b/p;
    gel(x,p) = ap;
    for(j = 2; j <= m; j++)
      gel(x,j*p) = gmul(ap, gel(x,j));
  }
}

static GEN
eulerfact_raw(GEN s, long p, long l)
{
  long j, n, q;
  GEN V;
  for (n = 1, q = p; q <= l; q*=p, n++);
  s = gtoser(s, gvar(s), n);
  if (signe(s)==0 || valp(s)!=0 || !gequal1(gel(s,2)))
    err_direuler(s);
  n = minss(n, lg(s)-2);
  V = cgetg(n+1, t_VEC);
  for(j=1; j<=n; j++)
    gel(V, j) = gel(s,j+1);
  return V;
}

static GEN
eulerfact_bad(GEN s, long p, long l)
{
  pari_sp av = avma;
  return gerepilecopy(av, eulerfact_raw(s, p, l));
}

static GEN
eulerfact_small(void *E, GEN (*eval)(void *, GEN), long p, long l)
{
  pari_sp av = avma;
  GEN s = eval(E, utoi(p));
  return gerepilecopy(av, eulerfact_raw(s, p, l));
}

static GEN
eulerfact_large(void *E, GEN (*eval)(void *, GEN), long p)
{
  pari_sp av = avma;
  GEN s = eval(E, utoi(p));
  s = gtoser(s, gvar(s), 2);
  if (signe(s)==0 || valp(s)!=0 || !gequal1(gel(s,2)))
    err_direuler(s);
  return gerepilecopy(av, lg(s)>=4 ? gel(s,3): gen_0);
}

static ulong
direulertou(GEN a, GEN fl(GEN))
{
  if (typ(a) != t_INT)
  {
    a = fl(a);
    if (typ(a) != t_INT) pari_err_TYPE("direuler", a);
  }
  return signe(a)<=0 ? 0: itou(a);
}

static GEN
direuler_bad(void *E, GEN (*eval)(void *, GEN), GEN a, GEN b, GEN c, GEN Sbad)
{
  ulong n;
  pari_sp av0 = avma;
  GEN v, V;
  forprime_t T;
  long i;
  ulong au, bu, cu;
  ulong p, bb;
  au = direulertou(a, gceil);
  bu = direulertou(b, gfloor);
  cu = c ? direulertou(c, gfloor): bu;
  if (cu == 0) return cgetg(1,t_VEC);
  if (bu > cu) bu = cu;
  bb = usqrt(cu);
  if (!u_forprime_init(&T, au, bu))
    { avma = av0; return mkvec(gen_1); }
  v = cgetg(cu+1,t_VECSMALL);
  V = zerovec(cu);
  gel(V,1) = gen_1; v[1] = 1; n = 1; p = 1;
  if (Sbad)
  {
    long l = lg(Sbad)-1;
    GEN pbad = gen_1;
    for(i=1; i<=l; i++)
    {
      ulong p;
      GEN ai = gel(Sbad,i), s;
      if (typ(ai)!=t_VEC || lg(ai)!=3)
        pari_err_TYPE("direuler [bad primes]",ai);
      p = gtou(gel(ai, 1));
      s = ginv(gel(ai, 2));
      if (p <= cu)
      {
        n = dirmuleuler_small(V, v, n, p, eulerfact_bad(s, p, cu));
        pbad = muliu(pbad, p);
      }
    }
    Sbad = pbad;
  }
  while ( p <= bb && (p = u_forprime_next(&T)) )
  {
    if (Sbad && !umodiu(Sbad, p)) continue;
    n = dirmuleuler_small(V, v, n, p, eulerfact_small(E,eval,p, cu));
  }
  while ( (p = u_forprime_next(&T)) )
  {
    if (Sbad && !umodiu(Sbad, p)) continue;
    dirmuleuler_large(V, p, eulerfact_large(E,eval,p));
  }
  return gerepilecopy(av0,V);
}

static GEN
localfactor(void *E, GEN p)
{
  GEN v = (GEN)E, L = gel(v,1), a = gel(v,2);
  return ginv(closure_callgen2(a, p, stoi(logint(L, p, NULL))));
}
static GEN
direxpand_bad(GEN a, long L, GEN Sbad)
{
  pari_sp ltop = avma;
  long ta = typ(a), la, tv, i;
  GEN an, v;

  if (ta == t_CLOSURE) switch(closure_arity(a))
  {
    GEN gL;
    case 2:
      gL = stoi(L);
      return direuler_bad((void*)mkvec2(gL,a), localfactor, gen_2, gL,gL, Sbad);
    case 1:
      a = closure_callgen1(a, stoi(L));
      if (typ(a) != t_VEC) pari_err_TYPE("direxpand", a);
      return a;
    default: pari_err_TYPE("direxpand [wrong arity]", a);
  }
  if (ta != t_VEC) pari_err_TYPE("direxpand", a);
  la = lg(a); if (la == 1) pari_err_TYPE("direxpand", a);
  v = gel(a,1); tv = typ(v);
  if (tv != t_CLOSURE && tv != t_VEC)
  { /* regular vector, return it */
    if (la-1 < L) L = la-1;
    an = cgetg(L+1, t_VEC);
    for (i = 1; i <= L; i++) gel(an,i) = gcopy(gel(a,i));
    return an;
  }
  /* vector [an, [p1, 1/L_{p1}], ..., [pk, 1/L_{pk}}]]: exceptional primes */
  if (la > 1)
  {
    Sbad = cgetg(la-1, t_VEC);
    for (i = 2; i < la; i++) gel(Sbad, i-1) = gel(a, i);
  }
  an = direxpand_bad(v, L, Sbad);
  return gerepilecopy(ltop, an);
}
GEN
direxpand(GEN a, long L) { return direxpand_bad(a, L, NULL); }

GEN
direuler(void *E, GEN (*eval)(void *, GEN), GEN a, GEN b, GEN c)
{ return direuler_bad(E, eval, a, b, c, NULL); }