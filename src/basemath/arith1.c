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

/*********************************************************************/
/**                                                                 **/
/**                     ARITHMETIC FUNCTIONS                        **/
/**                         (first part)                            **/
/**                                                                 **/
/*********************************************************************/
#include "pari.h"

extern GEN ishiftr_spec(GEN x, long lx, long n);
/*********************************************************************/
/**                                                                 **/
/**                  ARITHMETIC FUNCTION PROTOTYPES                 **/
/**                                                                 **/
/*********************************************************************/
GEN
garith_proto(GEN f(GEN), GEN x, int do_error)
{
  long tx=typ(x),lx,i;
  GEN y;

  if (is_matvec_t(tx))
  {
    lx=lg(x); y=cgetg(lx,tx);
    for (i=1; i<lx; i++) y[i] = (long) garith_proto(f,(GEN) x[i], do_error);
    return y;
  }
  if (tx != t_INT && do_error) err(arither1);
  return f(x);
}

GEN
arith_proto(long f(GEN), GEN x, int do_error)
{
  long tx=typ(x),lx,i;
  GEN y;

  if (is_matvec_t(tx))
  {
    lx=lg(x); y=cgetg(lx,tx);
    for (i=1; i<lx; i++) y[i] = (long) arith_proto(f,(GEN) x[i], do_error);
    return y;
  }
  if (tx != t_INT && do_error) err(arither1);
  return stoi(f(x));
}

GEN
arith_proto2(long f(GEN,GEN), GEN x, GEN n)
{
  long l,i,tx = typ(x);
  GEN y;

  if (is_matvec_t(tx))
  {
    l=lg(x); y=cgetg(l,tx);
    for (i=1; i<l; i++) y[i] = (long) arith_proto2(f,(GEN) x[i],n);
    return y;
  }
  if (tx != t_INT) err(arither1);
  tx=typ(n);
  if (is_matvec_t(tx))
  {
    l=lg(n); y=cgetg(l,tx);
    for (i=1; i<l; i++) y[i] = (long) arith_proto2(f,x,(GEN) n[i]);
    return y;
  }
  if (tx != t_INT) err(arither1);
  return stoi(f(x,n));
}

GEN
arith_proto2gs(long f(GEN,long), GEN x, long y)
{
  long l,i,tx = typ(x);
  GEN t;

  if (is_matvec_t(tx))
  {
    l=lg(x); t=cgetg(l,tx);
    for (i=1; i<l; i++) t[i]= (long) arith_proto2gs(f,(GEN) x[i],y);
    return t;
  }
  if (tx != t_INT) err(arither1);
  return stoi(f(x,y));
}

GEN
garith_proto2gs(GEN f(GEN,long), GEN x, long y)
{
  long l,i,tx = typ(x);
  GEN t;

  if (is_matvec_t(tx))
  {
    l=lg(x); t=cgetg(l,tx);
    for (i=1; i<l; i++) t[i]= (long) garith_proto2gs(f,(GEN) x[i],y);
    return t;
  }
  if (tx != t_INT) err(arither1);
  return f(x,y);
}

GEN
garith_proto3ggs(GEN f(GEN,GEN,long), GEN x, GEN y, long z)
{
  long l,i,tx = typ(x);
  GEN t;

  if (is_matvec_t(tx))
  {
    l=lg(x); t=cgetg(l,tx);
    for (i=1; i<l; i++) t[i]= (long) garith_proto3ggs(f,(GEN) x[i],y,z);
    return t;
  }
  if (tx != t_INT) err(arither1);
  tx = typ(y);
  if (is_matvec_t(tx))
  {
    l=lg(y); t=cgetg(l,tx);
    for (i=1; i<l; i++) t[i]= (long) garith_proto3ggs(f,x,(GEN) y[i],z);
    return t;
  }
  if (tx != t_INT) err(arither1);
  return f(x,y,z);
}

GEN
gassoc_proto(GEN f(GEN,GEN), GEN x, GEN y)
{
  int tx=typ(x);
  if (!y)
  {
    pari_sp av = avma;
    if (tx!=t_VEC && tx!=t_COL)
      err(typeer,"association");
    return gerepileupto(av,divide_conquer_prod(x,f));
  }
  return f(x,y);
}

/*********************************************************************/
/**                                                                 **/
/**               ORDER of INTEGERMOD x  in  (Z/nZ)*                **/
/**                                                                 **/
/*********************************************************************/

GEN
order(GEN x)
{
  pari_sp av = avma,av1;
  long i,e;
  GEN o,m,p;

  if (typ(x) != t_INTMOD || !gcmp1(mppgcd((GEN) x[1],(GEN) x[2])))
    err(talker,"not an element of (Z/nZ)* in order");
  o=phi((GEN) x[1]); m=decomp(o);
  for (i = lg(m[1])-1; i; i--)
  {
    p=gcoeff(m,i,1); e=itos(gcoeff(m,i,2));
    do
    {
      GEN o1=diviiexact(o,p), y=powgi(x,o1);
      if (!gcmp1((GEN)y[2])) break;
      e--; o = o1;
    }
    while (e);
  }
  av1=avma; return gerepile(av,av1,icopy(o));
}

/******************************************************************/
/*                                                                */
/*                 GENERATOR of (Z/mZ)*                           */
/*                                                                */
/******************************************************************/

GEN
ggener(GEN m)
{
  return garith_proto(gener,m,1);
}

/* assume p prime */
ulong
u_gener_fact(ulong p, GEN fa)
{
  const pari_sp av = avma;
  const ulong q = p - 1;
  int i, x, k ;
  GEN L;
  if (p == 2) return 1;

  if (!fa) { fa = L = (GEN)decomp(utoi(q))[1]; k = lg(fa)-1; } 
  else { fa = (GEN)fa[1]; k = lg(fa)-1; L = cgetg(k + 1, t_VECSMALL); }
 
  for (i=1; i<=k; i++) L[i] = (long)(q / itou((GEN)fa[i]));
  for (x=2;;x++)
    if (x % p)
    {
      for (i=k; i; i--)
	if (powuumod(x, (ulong)L[i], p) == 1) break;
      if (!i) break;
    }
  avma = av; return x;
}
ulong
u_gener(ulong p) { return u_gener_fact(p, NULL); }

/* assume p prime */
GEN
Fp_gener_fact(GEN p, GEN fa)
{
  pari_sp av0 = avma;
  long k, i;
  GEN x, q, V;
  if (egalii(p, gdeux)) return gun;
  if (lgefint(p) == 3) return utoi(u_gener_fact((ulong)p[2], fa));

  q = subis(p, 1);
  if (!fa) { fa = V = (GEN)decomp(q)[1]; k = lg(fa)-1; }
  else { fa = (GEN)fa[1]; k = lg(fa)-1; V = cgetg(k + 1, t_VEC); }
 
  for (i=1; i<=k; i++) V[i] = (long)diviiexact(q, (GEN)fa[i]);
  x = utoi(2UL);
  for (;; x[2]++)
  {
    GEN d = mppgcd(p,x);
    if (!is_pm1(d)) continue;
    for (i = k; i; i--) {
      GEN e = powmodulo(x, (GEN)V[i], p);
      if (is_pm1(e)) break;
    }
    if (!i) { avma = av0; return utoi((ulong)x[2]); }
  }
}

GEN 
Fp_gener(GEN p) { return Fp_gener_fact(p, NULL); }

GEN
gener(GEN m)
{
  pari_sp av = avma;
  long e;
  GEN x, t, p;

  if (typ(m) != t_INT) err(arither1);
  e = signe(m);
  if (!e) err(talker,"zero modulus in znprimroot");
  if (is_pm1(m)) return gmodulss(0,1);
  if (e < 0) m = absi(m);

  e = mod4(m);
  if (e == 0) /* m = 0 mod 4 */
  { /* m != 4, non cyclic */
    if (cmpis(m,4)) err(talker,"primitive root mod %Z does not exist", m);
    return gmodulsg(3, m);
  }
  if (e == 2) /* m = 0 mod 2 */
  {
    GEN q = shifti(m,-1); x = (GEN) gener(q)[2];
    if (!mod2(x)) x = addii(x,q);
    return gerepileupto(av, gmodulcp(x,m));
  }

  t = decomp(m);
  if (lg(t[1]) != 2) err(talker,"primitive root mod %Z does not exist", m);
  p = gcoeff(t,1,1);
  e = itos(gcoeff(t,1,2));
  x = Fp_gener(p);
  if (e >= 2)
  {
    GEN M = (e == 2)? m: sqri(p); 
    if (gcmp1(powmodulo(x, subis(p,1), M))) x = addii(x,p);
  }
  return gerepileupto(av, gmodulcp(x,m));
}

GEN
znstar(GEN n)
{
  GEN p1,z,q,u,v,d,list,ep,h,gen,moduli,p,a;
  long i,j,nbp,sizeh;
  pari_sp av;

  if (typ(n) != t_INT) err(arither1);
  if (!signe(n))
  {
    z=cgetg(4,t_VEC);
    z[1]=deux; p1=cgetg(2,t_VEC);
    z[2]=(long)p1; p1[1]=deux; p1=cgetg(2,t_VEC);
    z[3]=(long)p1; p1[1]=lneg(gun);
    return z;
  }
  av=avma; n=absi(n);
  if (cmpis(n,2)<=0)
  {
    avma=av; z=cgetg(4,t_VEC);
    z[1]=un;
    z[2]=lgetg(1,t_VEC);
    z[3]=lgetg(1,t_VEC);
    return z;
  }
  list=factor(n); ep=(GEN)list[2]; list=(GEN)list[1]; nbp=lg(list)-1;
  h      = cgetg(nbp+2,t_VEC);
  gen    = cgetg(nbp+2,t_VEC);
  moduli = cgetg(nbp+2,t_VEC);
  switch(mod8(n))
  {
    case 0:
      h[1] = lmul2n(gun,itos((GEN)ep[1])-2); h[2] = deux;
      gen[1] = lstoi(5); gen[2] = laddis(gmul2n((GEN)h[1],1),-1);
      moduli[1] = moduli[2] = lmul2n(gun,itos((GEN)ep[1]));
      sizeh = nbp+1; i=3; j=2; break;
    case 4:
      h[1] = deux;
      gen[1] = lstoi(3);
      moduli[1] = lmul2n(gun,itos((GEN)ep[1]));
      sizeh = nbp; i=j=2; break;
    case 2: case 6:
      sizeh = nbp-1; i=1; j=2; break;
    default: /* 1, 3, 5, 7 */
      sizeh = nbp; i=j=1;
  }
  for ( ; j<=nbp; i++,j++)
  {
    p = (GEN)list[j]; q = gpowgs(p, itos((GEN)ep[j])-1);
    h[i] = lmulii(addis(p,-1),q); p1 = mulii(p,q);
    gen[i] = gener(p1)[2];
    moduli[i] = (long)p1;
  }
#if 0
  if (nbp == 1 && is_pm1(ep[1]))
    gen[1] = lmodulcp((GEN)gen[1],n);
  else
#endif
    for (i=1; i<=sizeh; i++)
    {
      q = (GEN)moduli[i]; a = (GEN)gen[i];
      u = mpinvmod(q, diviiexact(n,q));
      gen[i] = lmodulcp(addii(a,mulii(mulii(subsi(1,a),u),q)), n);
    }
  
  for (i=sizeh; i>=2; i--)
    for (j=i-1; j>=1; j--)
      if (!divise((GEN)h[j],(GEN)h[i]))
      {
	d=bezout((GEN)h[i],(GEN)h[j],&u,&v);
        q=diviiexact((GEN)h[j],d);
	h[j]=(long)mulii((GEN)h[i],q); h[i]=(long)d;
	gen[j]=ldiv((GEN)gen[j], (GEN)gen[i]);
	gen[i]=lmul((GEN)gen[i], powgi((GEN)gen[j], mulii(v,q)));
      }
  q=gun; for (i=1; i<=sizeh && !gcmp1((GEN)h[i]); i++) q=mulii(q,(GEN)h[i]);
  setlg(h,i); setlg(gen,i); z=cgetg(4,t_VEC);
  z[1]=(long)q; z[2]=(long)h; z[3]=(long)gen;
  return gerepilecopy(av,z);
}

/*********************************************************************/
/**                                                                 **/
/**                     INTEGRAL SQUARE ROOT                        **/
/**                                                                 **/
/*********************************************************************/
extern ulong mpsqrtl(GEN a);

GEN
gracine(GEN a)
{
  return garith_proto(racine,a,1); /* hm. --GN */
}

GEN
racine_i(GEN a, int roundup)
{
  pari_sp av = avma;
  long k,m;
  GEN x = isqrti(a);
  if (roundup && signe(x))
  {
    long l=lgefint(a);
    m = modBIL(x);
    k = (m * m != a[l-1] || !egalii(sqri(x),a));
    avma = (pari_sp)x;
    if (k) x = gerepileuptoint(av, addis(x,1));
  }
  return x;
}

GEN
racine(GEN a)
{
  if (typ(a) != t_INT) err(arither1);
  switch (signe(a))
  {
    case 1: return isqrti(a);
    case 0: return gzero;
    default: err(talker, "negative integer in sqrtint");
  }
  return NULL; /* not reached */
}

/*********************************************************************/
/**                                                                 **/
/**                      PERFECT SQUARE                             **/
/**                                                                 **/
/*********************************************************************/
extern ulong usqrtsafe(ulong a);

static int
carremod(ulong A)
{
  static int carresmod64[]={
    1,1,0,0,1,0,0,0,0,1, 0,0,0,0,0,0,1,1,0,0, 0,0,0,0,0,1,0,0,0,0,
    0,0,0,1,0,0,1,0,0,0, 0,1,0,0,0,0,0,0,0,1, 0,0,0,0,0,0,0,1,0,0, 0,0,0,0};
  static int carresmod63[]={
    1,1,0,0,1,0,0,1,0,1, 0,0,0,0,0,0,1,0,1,0, 0,0,1,0,0,1,0,0,1,0,
    0,0,0,0,0,0,1,1,0,0, 0,0,0,1,0,0,1,0,0,1, 0,0,0,0,0,0,0,0,1,0, 0,0,0};
  static int carresmod65[]={
    1,1,0,0,1,0,0,0,0,1, 1,0,0,0,1,0,1,0,0,0, 0,0,0,0,0,1,1,0,0,1,
    1,0,0,0,0,1,1,0,0,1, 1,0,0,0,0,0,0,0,0,1, 0,1,0,0,0,1,1,0,0,0, 0,1,0,0,1};
  static int carresmod11[]={1,1,0,1,1,1,0,0,0,1, 0};
  return (carresmod64[A & 0x3fUL]
    && carresmod63[A % 63UL]
    && carresmod65[A % 65UL]
    && carresmod11[A % 11UL]);
}

/* emulate carrecomplet on single-word positive integers */
ulong
ucarrecomplet(ulong A)
{
  if (carremod(A))
  {
    ulong a = usqrtsafe(A);
    if (a * a == A) return a;
  }
  return 0;
}

long
carrecomplet(GEN x, GEN *pt)
{
  pari_sp av;
  GEN y;

  switch(signe(x))
  {
    case -1: return 0;
    case 0: if (pt) *pt=gzero; return 1;
  }
  if (lgefint(x) == 3)
  {
    long a = ucarrecomplet((ulong)x[2]);
    if (!a) return 0;
    if (pt) *pt = stoi(a);
    return 1;
  }
  if (!carremod((ulong)smodis(x, 64*63*65*11))) return 0;
  av=avma; y = racine(x);
  if (!egalii(sqri(y),x)) { avma=av; return 0; }
  if (pt) { *pt = y; avma=(pari_sp)y; } else avma=av; 
  return 1;
}

static long
polcarrecomplet(GEN x, GEN *pt)
{
  pari_sp av,av2;
  long i,l;
  GEN y,a,b;

  if (!signe(x)) return 1;
  l=lg(x); if ((l&1) == 0) return 0; /* odd degree */
  i=2; while (isexactzero((GEN)x[i])) i++;
  if (i&1) return 0;
  av2 = avma; a = (GEN)x[i];
  switch (typ(a))
  {
    case t_POL: case t_INT:
      y = gcarrecomplet(a,&b); break;
    default:
      y = gcarreparfait(a); b = NULL; break;
  }
  if (y == gzero) { avma = av2; return 0; }
  av = avma; x = gdiv(x,a);
  y = gtrunc(gsqrt(greffe(x,l,1),0)); av2 = avma;
  if (!gegal(gsqr(y), x)) { avma = av; return 0; }
  if (pt)
  { 
    avma = av2; 
    if (!gcmp1(a))
    {
      if (!b) b = gsqrt(a,DEFAULTPREC);
      y = gmul(b,y);
    }
    *pt = gerepileupto(av,y);
  }
  else avma = av;
  return 1;
}

GEN
gcarrecomplet(GEN x, GEN *pt)
{
  long l, tx = typ(x);
  GEN y;
  pari_sp av;

  if (!pt) return gcarreparfait(x);
  if (is_matvec_t(tx))
  {
    long i, l = lg(x);
    GEN z, p, t;
    y = cgetg(l,tx);
    z = cgetg(l,tx);
    for (i=1; i<l; i++)
    {
      t = gcarrecomplet((GEN)x[i],&p);
      y[i] = (long)t;
      z[i] = gcmp0(t)? zero: (long)p;
    }
    *pt = z; return y;
  }
  switch(tx)
  {
    case t_INT: return stoi( carrecomplet(x, pt) );
    case t_FRAC:
      av = avma;
      l = carrecomplet(mulii((GEN)x[1],(GEN)x[2]), pt);
      break;

    case t_POL: return stoi( polcarrecomplet(x,pt) );
    case t_RFRAC:
      av = avma;
      l = polcarrecomplet(gmul((GEN)x[1],(GEN)x[2]), pt);
      break;

    default: err(arither1);
      return NULL; /* not reached */
  }
  if (l) *pt = gerepileupto(av, gdiv(*pt, (GEN)x[2])); else avma = av;
  return l? gun: gzero;
}

GEN
gcarreparfait(GEN x)
{
  pari_sp av;
  GEN p1,a,p;
  long tx=typ(x),l,i,v;

  switch(tx)
  {
    case t_INT:
      return carreparfait(x)? gun: gzero;

    case t_REAL:
      return (signe(x)>=0)? gun: gzero;

    case t_INTMOD:
    {
      GEN b, q;
      long w;
      a = (GEN)x[2]; if (!signe(a)) return gun;
      av = avma;
      q = absi((GEN)x[1]); v = vali(q);
      if (v) /* > 0 */
      {
        long dv;
        w = vali(a); dv = v - w;
        if (dv > 0)
        {
          if (w & 1) { avma = av; return gzero; }
          if (dv >= 2)
          {
            b = w? shifti(a,-w): a;
            if ((dv>=3 && mod8(b) != 1) ||
                (dv==2 && mod4(b) != 1)) { avma = av; return gzero; }
          }
        }
        q = shifti(q, -v);
      }
      /* q is now odd */
      i = kronecker(a,q);
      if (i < 0) { avma = av; return gzero; }
      if (i==0)
      {
        GEN d = mppgcd(a,q);
        p = (GEN)factor(d)[1]; l = lg(p);
        for (i=1; i<l; i++)
        {
          v = pvaluation(a,(GEN)p[i],&p1);
          w = pvaluation(q,(GEN)p[i], &q);
          if (v < w && (v&1 || kronecker(p1,(GEN)p[i]) == -1)) 
            { avma = av; return gzero; }
        }
        if (kronecker(a,q) == -1) { avma = av; return gzero; }
      }
      /* kro(a,q) = 1, q odd: need to factor q */
      p = (GEN)factor(q)[1]; l = lg(p) - 1;
      /* kro(a,q) = 1, check all p|q but the last (product formula) */
      for (i=1; i<l; i++)
        if (kronecker(a,(GEN)p[i]) == -1) { avma = av; return gzero; }
      return gun;
    }

    case t_FRAC:
      av=avma; l=carreparfait(mulii((GEN)x[1],(GEN)x[2]));
      avma=av; return l? gun: gzero;

    case t_COMPLEX:
      return gun;

    case t_PADIC:
      a = (GEN)x[4]; if (!signe(a)) return gun;
      if (valp(x)&1) return gzero;
      p = (GEN)x[2];
      if (!egalii(p, gdeux))
        return (kronecker(a,p)== -1)? gzero: gun;

      v = precp(x); /* here p=2, a is odd */
      if ((v>=3 && mod8(a) != 1 ) ||
          (v==2 && mod4(a) != 1)) return gzero;
      return gun;

    case t_POL:
      return stoi( polcarrecomplet(x,NULL) );

    case t_SER:
      if (!signe(x)) return gun;
      if (valp(x)&1) return gzero;
      return gcarreparfait((GEN)x[2]);

    case t_RFRAC:
      av=avma;
      l=itos(gcarreparfait(gmul((GEN)x[1],(GEN)x[2])));
      avma=av; return stoi(l);

    case t_QFR: case t_QFI:
      return gcarreparfait((GEN)x[1]);

    case t_VEC: case t_COL: case t_MAT:
      l=lg(x); p1=cgetg(l,tx);
      for (i=1; i<l; i++) p1[i]=(long)gcarreparfait((GEN)x[i]);
      return p1;
  }
  err(typeer,"issquare");
  return NULL; /* not reached */
}

/*********************************************************************/
/**                                                                 **/
/**                        KRONECKER SYMBOL                         **/
/**                                                                 **/
/*********************************************************************/
/* u = 3,5 mod 8 ?  (= 2 not a square mod u) */
#define  ome(t) (labs(((t)&7)-4) == 1)
#define gome(t) (ome(modBIL(t)))

/* assume y odd, return kronecker(x,y) * s */
static long
krouu(ulong x, ulong y, long s)
{
  ulong x1 = x, y1 = y, z;
  while (x1)
  {
    long r = vals(x1);
    if (r)
    {
      if (odd(r) && ome(y1)) s = -s;
      x1 >>= r;
    }
    if (x1 & y1 & 2) s = -s;
    z = y1 % x1; y1 = x1; x1 = z;
  }
  return (y1 == 1)? s: 0;
}

GEN
gkronecker(GEN x, GEN y) { return arith_proto2(kronecker,x,y); }

long
kronecker(GEN x, GEN y)
{
  const pari_sp av = avma;
  GEN z;
  long s = 1, r;

  switch (signe(y))
  {
    case -1: y = negi(y); if (signe(x) < 0) s = -1; break;
    case 0: return is_pm1(x);
  }
  r = vali(y);
  if (r)
  {
    if (!mpodd(x)) { avma = av; return 0; }
    if (odd(r) && gome(x)) s = -s;
    y = shifti(y,-r);
  }
  x = modii(x,y);
  while (lgefint(y) > 3) /* x < y */
  {
    r = vali(x);
    if (r)
    {
      if (odd(r) && gome(y)) s = -s;
      x = shifti(x,-r);
    }
    /* x=3 mod 4 && y=3 mod 4 ? (both are odd here) */
    if (modBIL(x) & modBIL(y) & 2) s = -s;
    z = resii(y,x); y = x; x = z;
  }
  avma = av;
  return krouu(itou(x), itou(y), s);
}

GEN
gkrogs(GEN x, long y) { return arith_proto2gs(krogs,x,y); }

long
krogs(GEN x, long y)
{
  ulong y1;
  long s = 1, r;

  if (y <= 0)
  {
    if (y == 0) return is_pm1(x);
    y1 = (ulong)-y; if (signe(x) < 0) s = -1;
  }
  else
    y1 = (ulong)y;
  r = vals(y1);
  if (r)
  {
    if (!mpodd(x)) return 0;
    if (odd(r) && gome(x)) s = -s;
    y1 >>= r;
  }
  return krouu(umodiu(x, y1), y1, s);
}

long
krosg(long x, GEN y)
{
  const pari_sp av = avma;
  long s = 1, r;
  ulong u;

  switch (signe(y))
  {
    case -1: y = negi(y); if (x < 0) s = -1; break;
    case 0: return (x==1 || x==-1);
  }
  r = vali(y);
  if (r)
  {
    if (!odd(x)) { avma = av; return 0; }
    if (odd(r) && ome(x)) s = -s;
    y = shifti(y,-r);
  }
  (void)sdivsi_rem(x,y, &x);
  r = vals(x);
  if (r)
  {
    if (odd(r) && gome(y)) s = -s;
    x >>= r;
  }
  /* x=3 mod 4 && y=3 mod 4 ? (both are odd here) */
  if (x & modBIL(y) & 2) s = -s;
  u = umodiu(y, (ulong)x);
  avma = av; return krouu(u, (ulong)x, s);
}

long
kross(long x, long y)
{
  ulong y1;
  long s = 1, r;

  if (y <= 0)
  {
    if (y == 0) return (labs(x)==1);
    y1 = (ulong)-y; if (x < 0) s = -1;
  }
  else
    y1 = (ulong)y;
  r = vals(y1);
  if (r)
  {
    if (!odd(x)) return 0;
    if (odd(r) && ome(x)) s = -s;
    y1 >>= r;
  }
  x = x % (long)y1; if (x < 0) x += y1;
  return krouu((ulong)x, y1, s);
}

/*********************************************************************/
/**                                                                 **/
/**                          HILBERT SYMBOL                         **/
/**                                                                 **/
/*********************************************************************/

long
hil0(GEN x, GEN y, GEN p)
{
  return hil(x,y, p? p: gzero);
}

#define eps(t) (((signe(t)*(modBIL(t)))&3)==3)
long
hilii(GEN x, GEN y, GEN p)
{
  pari_sp av;
  long a, b, z;
  GEN u, v;

  if (signe(p)<=0)
    return (signe(x)<0 && signe(y)<0)? -1: 1;
  av = avma;
  a = odd(pvaluation(x,p,&u));
  b = odd(pvaluation(y,p,&v));
  if (egalii(p,gdeux))
  {
    z = (eps(u) && eps(v))? -1: 1;
    if (a && gome(v)) z = -z;
    if (b && gome(u)) z = -z;
  }
  else
  {
    z = (a && b && eps(p))? -1: 1;
    if (a && kronecker(v,p)<0) z = -z;
    if (b && kronecker(u,p)<0) z = -z;
  }
  avma = av; return z;
}

static void
err_at2() { err(talker, "insufficient precision for p = 2 in hilbert"); }

long
hil(GEN x, GEN y, GEN p)
{
  pari_sp av;
  long a,tx,ty,z;
  GEN p1,p2;

  if (gcmp0(x) || gcmp0(y)) return 0;
  av = avma; tx = typ(x); ty = typ(y);
  if (tx>ty) { p1=x; x=y; y=p1; a=tx,tx=ty; ty=a; }
  switch(tx) /* <= ty */
  {
    case t_INT:
      switch(ty)
      {
	case t_INT: return hilii(x,y,p);
	case t_REAL:
	  return (signe(x)<0 && signe(y)<0)? -1: 1;
	case t_INTMOD:
          p = (GEN)y[1]; if (egalii(gdeux,p)) err_at2();
	  return hilii(x, (GEN)y[2], p);
	case t_FRAC:
	  z = hilii(x, mulii((GEN)y[1],(GEN)y[2]), p);
	  avma = av; return z;
	case t_PADIC:
          p = (GEN)y[2];
	  if (egalii(gdeux,p) && precp(y) <= 1) err_at2();
	  p1 = odd(valp(y))? mulii(p,(GEN)y[4]): (GEN)y[4];
	  z = hilii(x, p1, p); avma = av; return z;
      }
      break;

    case t_REAL:
      if (ty != t_FRAC) break;
      if (signe(x) > 0) return 1;
      return signe(y[1])*signe(y[2]);

    case t_INTMOD:
      p = (GEN)x[1]; if (egalii(gdeux,p)) err_at2();
      switch(ty)
      {
        case t_INTMOD:
          if (!egalii(p, (GEN)y[1])) break;
          return hilii((GEN)x[2],(GEN)y[2],p);
        case t_FRAC:
	  return hil((GEN)x[2],y,p);
        case t_PADIC:
          if (!egalii(p, (GEN)y[2])) break;
          return hil((GEN)x[2],y,p);
      }
      break;

    case t_FRAC:
      p1 = mulii((GEN)x[1],(GEN)x[2]);
      switch(ty)
      {
	case t_FRAC:
	  p2 = mulii((GEN)y[1],(GEN)y[2]);
	  z = hilii(p1,p2,p); avma = av; return z;
	case t_PADIC:
	  z = hil(p1,y,NULL); avma = av; return z;
      }
      break;

    case t_PADIC:
      p = (GEN)x[2];
      if (ty != t_PADIC || !egalii(p,(GEN)y[2])) break;
      if (egalii(p, gdeux) && (precp(x) <= 1 || precp(y) <= 1)) err_at2();
      p1 = odd(valp(x))? mulii(p,(GEN)x[4]): (GEN)x[4];
      p2 = odd(valp(y))? mulii(p,(GEN)y[4]): (GEN)y[4];
      z = hilii(p1,p2,p); avma = av; return z;
  }
  err(talker,"forbidden or incompatible types in hil");
  return 0; /* not reached */
}
#undef eps
#undef ome
#undef gome

/*******************************************************************/
/*                                                                 */
/*                       SQUARE ROOT MODULO p                      */
/*                                                                 */
/*******************************************************************/

#define sqrmod(x,p) (resii(sqri(x),p))

static GEN ffsqrtmod(GEN a, GEN p);

/* Tonelli-Shanks. Assume p is prime and return NULL if (a,p) = -1. */
GEN
mpsqrtmod(GEN a, GEN p)
{
  pari_sp av = avma, av1,lim;
  long i,k,e;
  GEN p1,q,v,y,w,m;

  if (typ(a) != t_INT || typ(p) != t_INT) err(arither1);
  if (signe(p) <= 0 || is_pm1(p)) err(talker,"not a prime in mpsqrtmod");
  p1 = addsi(-1,p); e = vali(p1);
  
  /* If `e' is "too big", use Cipolla algorithm ! [GTL] */
  if (e*(e-1) > 20 + 8 * bit_accuracy(lgefint(p)))
  {
    v = ffsqrtmod(a,p);
    if (!v) { avma = av; return NULL; }
    return gerepileuptoint(av,v);
  }

  if (e == 0) /* p = 2 */
  {
    avma = av;
    if (!egalii(p, gdeux))
      err(talker,"composite modulus in mpsqrtmod: %Z",p);
    if (!signe(a) || !mod2(a)) return gzero;
    return gun;
  }
  q = shifti(p1,-e); /* q = (p-1)/2^oo is odd */
  if (e == 1) y = p1;
  else /* look for an odd power of a primitive root */
    for (k=2; ; k++)
    { /* loop terminates for k < p (even if p composite) */
  
      i = krosg(k,p);
      if (i >= 0)
      {
        if (i) continue;
        err(talker,"composite modulus in mpsqrtmod: %Z",p);
      }
      av1 = avma;
      y = m = powmodulo(stoi(k),q,p);
      for (i=1; i<e; i++)
	if (gcmp1(m = sqrmod(m,p))) break;
      if (i == e) break; /* success */
      avma = av1;
    }

  p1 = powmodulo(a, shifti(q,-1), p); /* a ^ [(q-1)/2] */
  if (!signe(p1)) { avma=av; return gzero; }
  v = modii(mulii(a, p1), p);
  w = modii(mulii(v, p1), p);
  lim = stack_lim(av,1);
  while (!gcmp1(w))
  { /* a*w = v^2, y primitive 2^e-th root of 1
       a square --> w even power of y, hence w^(2^(e-1)) = 1 */
    p1 = sqrmod(w,p);
    for (k=1; !gcmp1(p1) && k < e; k++) p1 = sqrmod(p1,p);
    if (k == e) { avma=av; return NULL; } /* p composite or (a/p) != 1 */
    /* w ^ (2^k) = 1 --> w = y ^ (u * 2^(e-k)), u odd */
    p1 = y;
    for (i=1; i < e-k; i++) p1 = sqrmod(p1,p);
    y = sqrmod(p1, p); e = k;
    w = modii(mulii(y, w), p);
    v = modii(mulii(v, p1), p);
    if (low_stack(lim, stack_lim(av,1)))
    {
      GEN *gptr[3]; gptr[0]=&y; gptr[1]=&w; gptr[2]=&v;
      if(DEBUGMEM>1) err(warnmem,"mpsqrtmod");
      gerepilemany(av,gptr,3);
    }
  }
  av1 = avma;
  p1 = subii(p,v); if (cmpii(v,p1) > 0) v = p1; else avma = av1;
  return gerepileuptoint(av, v);
}

/* Cipolla's algorithm is better when e = v_2(p-1) is "too big".
 * Otherwise, is a constant times worse than the above one.
 * For p = 3 (mod 4), is about 3 times worse, and in average
 * is about 2 or 2.5 times worse.
 * 
 * But try both algorithms for e.g. S(n)=(2^n+3)^2-8 with
 * n = 750, 771, 779, 790, 874, 1176, 1728, 2604, etc.
 *
 * If X^2 = t^2 - a  is not a square in F_p, then
 * 
 *   (t+X)^(p+1) = (t+X)(t-X) = a
 *    
 * so we get sqrt(a) in F_p^2 by
 * 
 *   sqrt(a) = (t+X)^((p+1)/2)
 *    
 * If (a|p)=1, then sqrt(a) is in F_p.
 *
 * cf: LNCS 2286, pp 430-434 (2002)  [Gonzalo Tornaria] */
static GEN  
ffsqrtmod(GEN a, GEN p)
{
  pari_sp av = avma, av1, lim;
  long e, t, man, k, nb;
  GEN q, n, u, v, d, d2, b, u2, v2, qptr;

  if (kronecker(a, p) < 0) return NULL;
  
  av1 = avma;
  for(t=1; ; t++)
  {
    n = subsi(t*t, a);
    if (kronecker(n, p) < 0) break;
    avma = av1;
  }

  u = stoi(t); v = gun; /* u+vX = t+X */
  e = vali(addsi(-1,p)); q = shifti(p, -e);
  /* p = 2^e q + 1  and  (p+1)/2 = 2^(e-1)q + 1 */

  /* Compute u+vX := (t+X)^q */
  av1 = avma; lim = stack_lim(av1, 1); 
  qptr = q+2; man = *qptr;
  k = 1+bfffo(man); man<<=k; k=BITS_IN_LONG-k;
  for (nb=lgefint(q)-2;nb; nb--)
  {
    for (; k; man<<=1, k--)
    {
      if (man < 0)
      { /* u+vX := (u+vX)^2 (t+X) */
        d = addii(u, mulsi(t,v));
        d2 = sqri(d);
        b = resii(mulii(a,v), p);
        u = modii(subii(mulsi(t,d2), mulii(b,addii(u,d))), p);
        v = modii(subii(d2, mulii(b,v)), p);
      }
      else
      { /* u+vX := (u+vX)^2 */
        u2 = sqri(u);
        v2 = sqri(v);
        v = modii(subii(sqri(addii(v,u)), addii(u2,v2)), p);
        u = modii(addii(u2, mulii(v2,n)), p);
      }
    }
    
    if (low_stack(lim, stack_lim(av1, 1)))
    {
       GEN *gptr[2]; gptr[0]=&u; gptr[1]=&v;
       if (DEBUGMEM>1) err(warnmem, "ffsqrtmod");
       gerepilemany(av1,gptr,2);
    }
    
    man = *++qptr; k = BITS_IN_LONG;
  }

  while (--e)
  { /* u+vX := (u+vX)^2 */
    u2 = sqri(u);
    v2 = sqri(v);
    v = modii(subii(sqri(addii(v,u)), addii(u2,v2)), p);
    u = modii(addii(u2, mulii(v2,n)), p);

    if (low_stack(lim, stack_lim(av1, 1)))
    {
       GEN *gptr[2]; gptr[0]=&u; gptr[1]=&v;
       if (DEBUGMEM>1) err(warnmem, "ffsqrtmod");
       gerepilemany(av1,gptr,2);
    }
  }
     
  /* Now u+vX = (t+X)^(2^(e-1)q); thus
   * 
   *   (u+vX)(t+X) = sqrt(a) + 0 X
   *    
   * Whence,
   * 
   *   sqrt(a) = (u+vt)t - v*a
   *   0       = (u+vt) 
   *    
   * Thus a square root is v*a */
     
  v = modii(mulii(v,a), p);

  u = subii(p,v); if (cmpii(v,u) > 0) v = u;
  return gerepileuptoint(av,v);
}
 
/*******************************************************************/
/*                                                                 */
/*                       n-th ROOT MODULO p                        */
/*                                                                 */
/*******************************************************************/
extern GEN Fp_shanks(GEN x,GEN g0,GEN p, GEN q);

/* Assume l is prime. Return a non l-th power residue and set *zeta to a
 * primitive l-th root of 1.
 *
 * q = p-1 = l^e*r, e>=1, (r,l)=1
 * UNCLEAN */
static GEN
mplgenmod(GEN l, long e, GEN r,GEN p,GEN *zeta)
{
  const pari_sp av1 = avma;
  GEN m, m1;
  long k, i; 
  for (k=1; ; k++)
  {
    m1 = m = powmodulo(stoi(k+1), r, p);
    if (gcmp1(m)) { avma = av1; continue; }
    for (i=1; i<e; i++)
      if (gcmp1(m = powmodulo(m,l,p))) break;
    if (i==e) break;
    avma = av1;
  }
  *zeta = m; return m1;
}

/* solve x^l = a mod (p), l prime
 *
 * q = p-1 = (l^e)*r, e >= 1, (r,l) = 1
 * y is not an l-th power, hence generates the l-Sylow of (Z/p)^*
 * m = y^(q/l) != 1 */
static GEN
mpsqrtlmod(GEN a, GEN l, GEN p, GEN q,long e, GEN r, GEN y, GEN m)
{
  pari_sp av = avma, tetpil,lim;
  long k;
  GEN p1, u1, u2, v, w, z, dl;

  (void)bezout(r,l,&u1,&u2);
  v = powmodulo(a,u2,p);
  w = powmodulo(a,modii(mulii(negi(u1),r),q),p);
  lim = stack_lim(av,1);
  while (!gcmp1(w))
  {
    k = 0;
    p1 = w;
    do
    { /* if p is not prime, this loop will not end */
      z = p1; p1 = powmodulo(p1,l,p);
      k++;
    } while(!gcmp1(p1));
    if (k==e) { avma = av; return NULL; }
    dl = Fp_shanks(mpinvmod(z,p),m,p,l);
    p1 = powmodulo(y, modii(mulii(dl,gpowgs(l,e-k-1)),q), p);
    m = powmodulo(m,dl,p);
    e = k;
    v = modii(mulii(p1,v),p);
    y = powmodulo(p1,l,p);
    w = modii(mulii(y,w),p);
    if (low_stack(lim, stack_lim(av,1)))
    {
      if(DEBUGMEM>1) err(warnmem,"mpsqrtlmod");
      gerepileall(av,4, &y,&v,&w,&m);
    }
  }
  tetpil=avma; return gerepile(av,tetpil,icopy(v));
}
/* a, n t_INT, p is prime. Return one solution of x^n = a mod p 
*
* 1) If there is no solution, return NULL and if zetan!=NULL set *zetan=gzero.
*
* 2) If there is a solution, there are exactly m of them [m = gcd(p-1,n) if
* a != 0, and m = 1 otherwise].
* If zetan!=NULL, *zetan is set to a primitive mth root of unity so that
* the set of solutions is { x*zetan^k; k=0..m-1 } */
GEN 
mpsqrtnmod(GEN a, GEN n, GEN p, GEN *zetan)
{
  pari_sp ltop = avma, lbot = 0, lim;
  GEN m, u1, u2, q, z;

  if (typ(a) != t_INT || typ(n) != t_INT || typ(p)!=t_INT)
    err(typeer,"mpsqrtnmod");
  if (!signe(n)) err(talker,"1/0 exponent in mpsqrtnmod");
  if (gcmp1(n)) { if (zetan) *zetan = gun; return icopy(a);}
  a = modii(a,p);
  if (gcmp0(a)) { if (zetan) *zetan = gun; avma = ltop; return gzero;}
  q = addsi(-1,p);
  m = bezout(n,q,&u1,&u2);
  z = gun;
  lim = stack_lim(ltop,1);
  if (!gcmp1(m))
  {
    GEN F = decomp(m);
    long i, j, e; 
    GEN r, zeta, y, l;
    pari_sp av1 = avma;
    for (i = lg(F[1])-1; i; i--)
    {
      l = gcoeff(F,i,1);
      j = itos(gcoeff(F,i,2));
      e = pvaluation(q,l,&r);
      y = mplgenmod(l,e,r,p,&zeta);
      if (zetan) z = modii(mulii(z, powmodulo(y,gpowgs(l,e-j),p)), p);
      do
      {
	lbot = avma;
	if (!is_pm1(a) || signe(a)<0)
        {
	  a = mpsqrtlmod(a,l,p,q,e,r,y,zeta);
          if (!a) { avma = ltop; if (zetan) *zetan = gzero; return NULL;}
        }
	else
	  a = icopy(a);
      } while (--j);
      if (low_stack(lim, stack_lim(ltop,1)))
      { /* n can have lots of prime factors*/
	if(DEBUGMEM>1) err(warnmem,"mpsqrtnmod");
        gerepileall(av1, zetan? 2: 1, &a, &z);
	lbot = av1;
      }
    }
  }
  if (!egalii(m, n))
  {
    GEN b = modii(u1,q);
    lbot = avma; a = powmodulo(a,b,p);
  }
  if (zetan)
  {
    GEN *gptr[2];
    *zetan = icopy(z);
    gptr[0] = &a;
    gptr[1] = zetan; gerepilemanysp(ltop,lbot,gptr,2);
  }
  else
    a = gerepileuptoint(ltop, a);
  return a;
}

/*********************************************************************/
/**                                                                 **/
/**                        GCD & BEZOUT                             **/
/**                                                                 **/
/*********************************************************************/

GEN
mppgcd(GEN a, GEN b)
{
  if (typ(a) != t_INT || typ(b) != t_INT) err(arither1);
  return gcdii(a,b);
}

GEN
mpppcm(GEN x, GEN y)
{
  pari_sp av;
  GEN p1,p2;
  if (typ(x) != t_INT || typ(y) != t_INT) err(arither1);
  if (!signe(x)) return gzero;
  av = avma;
  p1 = gcdii(x,y); if (!is_pm1(p1)) y = diviiexact(y,p1);
  p2 = mulii(x,y); if (signe(p2) < 0) setsigne(p2,1);
  return gerepileuptoint(av, p2);
}

/*********************************************************************/
/**                                                                 **/
/**                      CHINESE REMAINDERS                         **/
/**                                                                 **/
/*********************************************************************/

/*  P.M. & M.H.
 *
 *  Chinese Remainder Theorem.  x and y must have the same type (integermod,
 *  polymod, or polynomial/vector/matrix recursively constructed with these
 *  as coefficients). Creates (with the same type) a z in the same residue
 *  class as x and the same residue class as y, if it is possible.
 *
 *  We also allow (during recursion) two identical objects even if they are
 *  not integermod or polymod. For example, if
 *
 *    x = [1. mod(5, 11), mod(X + mod(2, 7), X^2 + 1)]
 *    y = [1, mod(7, 17), mod(X + mod(0, 3), X^2 + 1)],
 *
 *  then chinois(x, y) returns
 *
 *    [1, mod(16, 187), mod(X + mod(9, 21), X^2 + 1)]
 *
 *  Someone else may want to allow power series, complex numbers, and
 *  quadratic numbers.
 */

GEN chinese(GEN x, GEN y)
{
  return gassoc_proto(chinois,x,y);
}

GEN
chinois(GEN x, GEN y)
{
  pari_sp av,tetpil;
  long i,lx, tx = typ(x);
  GEN z,p1,p2,d,u,v;

  if (gegal(x,y)) return gcopy(x);
  if (tx == typ(y)) switch(tx)
  {
    case t_POLMOD:
      if (gegal((GEN)x[1],(GEN)y[1]))  /* same modulus */
      {
	z=cgetg(3,tx);
	z[1]=lcopy((GEN)x[1]);
	z[2]=(long)chinois((GEN)x[2],(GEN)y[2]);
        return z;
      }  /* fall through */
    case t_INTMOD:
      z=cgetg(3,tx); av=avma;
      d=gbezout((GEN)x[1],(GEN)y[1],&u,&v);
      if (!gegal(gmod((GEN)x[2],d), gmod((GEN)y[2],d))) break;
      p1 = gdiv((GEN)x[1],d);
      p2 = gadd((GEN)x[2], gmul(gmul(u,p1), gadd((GEN)y[2],gneg((GEN)x[2]))));

      tetpil=avma; z[1]=lmul(p1,(GEN)y[1]); z[2]=lmod(p2,(GEN)z[1]);
      gerepilemanyvec(av,tetpil,z+1,2); return z;

    case t_POL:
      lx=lg(x); z = cgetg(lx,tx); z[1] = x[1];
      if (lx != lg(y) || varn(x) != varn(y)) break;
      for (i=2; i<lx; i++) z[i]=(long)chinois((GEN)x[i],(GEN)y[i]);
      return z;

    case t_VEC: case t_COL: case t_MAT:
      lx=lg(x); z=cgetg(lx,tx); if (lx!=lg(y)) break;
      for (i=1; i<lx; i++) z[i]=(long)chinois((GEN)x[i],(GEN)y[i]);
      return z;
  }
  err(talker,"incompatible arguments in chinois");
  return NULL; /* not reached */
}

/* return lift(chinois(x2 mod x1, y2 mod y1))
 * assume(x1,y1)=1, xi,yi integers and z1 = x1*y1
 */
GEN
chinois_int_coprime(GEN x2, GEN y2, GEN x1, GEN y1, GEN z1)
{
  pari_sp av = avma;
  GEN ax,p1;
  (void)new_chunk((lgefint(z1)<<1)+lgefint(x1)+lgefint(y1)); /* HACK */
  ax = mulii(mpinvmod(x1,y1), x1);
  p1 = addii(x2, mulii(ax, subii(y2,x2)));
  avma = av; return modii(p1,z1);
}

/*********************************************************************/
/**                                                                 **/
/**                      INVERSE MODULO b                           **/
/**                                                                 **/
/*********************************************************************/

GEN
mpinvmod(GEN a, GEN m)
{
  GEN res;
  if (! invmod(a,m,&res))
    err(invmoder,"%Z",gmodulcp(res,m));
  return res;
}

GEN
mpinvmodsafe(GEN a, GEN m)
{
  GEN res;
  if (! invmod(a,m,&res))
    return NULL;
  return res;
}

/*********************************************************************/
/**                                                                 **/
/**                    MODULAR EXPONENTIATION                       **/
/**                                                                 **/
/*********************************************************************/
extern ulong invrev(ulong b);
extern GEN resiimul(GEN x, GEN y);

static GEN _resii(GEN x, GEN y) { return resii(x,y); }

/* Montgomery reduction */

typedef struct {
  GEN N;
  ulong inv; /* inv = -N^(-1) mod B, */
} montdata;

void
init_montdata(GEN N, montdata *s)
{
  s->N = N;
  s->inv = (ulong) -invrev(modBIL(N));
}

GEN
init_remainder(GEN M)
{
  GEN sM = cgetg(3, t_VEC);
  sM[1] = (long)M;
  sM[2] = (long)linv( itor(M, lgefint(M) + 1) ); /* 1. / M */
  return sM;
}

/* optimal on UltraSPARC */
static long RESIIMUL_LIMIT   = 150;
static long MONTGOMERY_LIMIT = 32;

void
setresiilimit(long n) { RESIIMUL_LIMIT = n; }
void
setmontgomerylimit(long n) { MONTGOMERY_LIMIT = n; }

typedef struct {
  GEN N;
  GEN (*res)(GEN,GEN);
  GEN (*mulred)(GEN,GEN,GEN);
} muldata;

/* reduction for multiplication by 2 */
static GEN
_redsub(GEN x, GEN N)
{
  return (cmpii(x,N) >= 0)? subii(x,N): x;
}
/* Montgomery reduction */
extern GEN red_montgomery(GEN T, GEN N, ulong inv);
static GEN
montred(GEN x, GEN N)
{
  return red_montgomery(x, ((montdata*)N)->N, ((montdata*)N)->inv);
}
/* 2x mod N */
static GEN
_muli2red(GEN x, GEN y/* ignored */, GEN N) {
  (void)y; return _redsub(shifti(x,1), N);
}
static GEN
_muli2montred(GEN x, GEN y/* ignored */, GEN N) {
  GEN n = ((montdata*)N)->N;
  GEN z = _muli2red(x,y, n);
  long l = lgefint(n);
  while (lgefint(z) > l) z = subii(z,n);
  return z;
}
static GEN
_muli2invred(GEN x, GEN y/* ignored */, GEN N) {
  return _muli2red(x,y, (GEN)N[1]);
}
/* xy mod N */
static GEN
_muliired(GEN x, GEN y, GEN N) { return resii(mulii(x,y), N); }
static GEN
_muliimontred(GEN x, GEN y, GEN N) { return montred(mulii(x,y), N); }
static GEN
_muliiinvred(GEN x, GEN y, GEN N) { return resiimul(mulii(x,y), N); }

static GEN
_mul(void *data, GEN x, GEN y)
{
  muldata *D = (muldata *)data;
  return D->mulred(x,y,D->N);
}
static GEN
_sqr(void *data, GEN x)
{
  muldata *D = (muldata *)data;
  return D->res(sqri(x), D->N);
}

/* A^k mod N */
GEN
powmodulo(GEN A, GEN k, GEN N)
{
  pari_sp av = avma;
  long t,s, lN;
  int base_is_2, use_montgomery;
  GEN y;
  muldata  D;
  montdata S;

  if (typ(A) != t_INT || typ(k) != t_INT || typ(N) != t_INT) err(arither1);
  s = signe(k);
  if (!s)
  {
    t = signe(resii(A,N)); avma = av;
    return t? gun: gzero;
  }
  if (s < 0) y = mpinvmod(A,N);
  else
  {
    y = modii(A,N);
    if (!signe(y)) { avma = av; return gzero; }
  }
  if (lgefint(N) == 3 && lgefint(k) == 3)
  {
    ulong a = powuumod(itou(y), itou(k), itou(N));
    avma = av; return utoi(a);
  }

  base_is_2 = 0;
  if (lgefint(y) == 3) switch(y[2])
  {
    case 1: avma = av; return gun;
    case 2: base_is_2 = 1; break;
  }

  /* TODO: Move this out of here and use for general modular computations */
  lN = lgefint(N);
  use_montgomery = mod2(N) && lN < MONTGOMERY_LIMIT;
  if (use_montgomery)
  {
    init_montdata(N, &S);
    y = resii(shifti(y, bit_accuracy(lN)), N);
    D.mulred = base_is_2? &_muli2montred: &_muliimontred;
    D.res = &montred;
    D.N = (GEN)&S;
  }
  else if (lN > RESIIMUL_LIMIT
       && (lgefint(k) > 3 || (((double)k[2])*expi(A)) > 2 + expi(N)))
  {
    D.mulred = base_is_2? &_muli2invred: &_muliiinvred;
    D.res = &resiimul;
    D.N = init_remainder(N);
  }
  else
  {
    D.mulred = base_is_2? &_muli2red: &_muliired;
    D.res = &_resii;
    D.N = N;
  }
  y = leftright_pow(y, k, (void*)&D, &_sqr, &_mul);
  if (use_montgomery)
  {
    y = montred(y, (GEN)&S);
    if (cmpii(y,N) >= 0) y = subii(y,N);
  }
  return gerepileuptoint(av,y);
}

/*********************************************************************/
/**                                                                 **/
/**                NEXT / PRECEDING (PSEUDO) PRIME                  **/
/**                                                                 **/
/*********************************************************************/
GEN
gnextprime(GEN n) { return garith_proto(nextprime,n,0); }

GEN
gprecprime(GEN n) { return garith_proto(precprime,n,0); }

GEN
gisprime(GEN x, long flag)
{
  switch (flag)
  {
    case 0: return arith_proto(isprime,x,1);
    case 1: return garith_proto2gs(plisprime,x,1);
    case 2: return arith_proto(isprimeAPRCL,x,1);
  }
  err(flagerr,"gisprime");
  return 0;
}

long
isprimeSelfridge(GEN x) { return (plisprime(x,0)==gun); }

/* assume x BSW pseudoprime. Check whether it's small enough to be certified
 * prime */
int
BSW_isprime_small(GEN x)
{
  long l = lgefint(x);
  if (l < 4) return 1;
  if (l == 4)
  {
    pari_sp av = avma;
    long t = cmpii(x, u2toi(0x918UL, 0x4e72a000UL)); /* 10^13 */
    avma = av;
    if (t < 0) return 1;
  }
  return 0;
}

/* assume x a BSW pseudoprime */
int
BSW_isprime(GEN x)
{
  pari_sp av = avma;
  long l, res;
  GEN F, p;
 
  if (BSW_isprime_small(x)) return 1;
  F = (GEN)auxdecomp(subis(x,1), 0)[1];
  l = lg(F); p = (GEN)F[l-1];
  if (BSW_psp(p))
  { /* smooth */
    GEN z = cgetg(3, t_VEC);
    z[1] = (long)x;
    z[2] = (long)F; res = isprimeSelfridge(z);
  }
  else
    res = isprimeAPRCL(x);
  avma = av; return res;
}

long
isprime(GEN x)
{
  return BSW_psp(x) && BSW_isprime(x);
}

GEN
gispseudoprime(GEN x, long flag)
{
  if (flag == 0) return arith_proto(BSW_psp,x,1);
  return gmillerrabin(x, flag);
}

long
ispseudoprime(GEN x, long flag)
{
  if (flag == 0) return BSW_psp(x);
  return millerrabin(x, flag);
}

GEN
gispsp(GEN x) { return arith_proto(ispsp,x,1); }

long
ispsp(GEN x) { return millerrabin(x,1); }

GEN
gmillerrabin(GEN n, long k) { return arith_proto2gs(millerrabin,n,k); }

/*********************************************************************/
/**                                                                 **/
/**                    FUNDAMENTAL DISCRIMINANTS                    **/
/**                                                                 **/
/*********************************************************************/
GEN
gisfundamental(GEN x) { return arith_proto(isfundamental,x,1); }

long
isfundamental(GEN x)
{
  long r;

  if (gcmp0(x)) return 0;
  r = mod16(x);
  if (!r) return 0;
  if ((r & 3) == 0)
  {
    pari_sp av;
    r >>= 2; /* |x|/4 mod 4 */
    if (signe(x) < 0) r = 4-r;
    if (r == 1) return 0;
    av = avma;
    r = issquarefree( shifti(x,-2) );
    avma = av; return r;
  }
  r &= 3; /* |x| mod 4 */
  if (signe(x) < 0) r = 4-r;
  return (r==1) ? issquarefree(x) : 0;
}

GEN
quaddisc(GEN x)
{
  const pari_sp av = avma;
  long i,r,tx=typ(x);
  GEN p1,p2,f,s;

  if (!is_rational_t(tx)) err(arither1);
  f=factor(x); p1=(GEN)f[1]; p2=(GEN)f[2];
  s = gun;
  for (i=1; i<lg(p1); i++)
    if (odd(mael(p2,i,2))) s = gmul(s,(GEN)p1[i]);
  r=mod4(s); if (gsigne(x)<0) r=4-r;
  if (r>1) s = shifti(s,2);
  return gerepileuptoint(av, s);
}

/*********************************************************************/
/**                                                                 **/
/**                              FACTORIAL                          **/
/**                                                                 **/
/*********************************************************************/
/* return a * (a+1) * ... * b. Assume a <= b  [ note: factoring out powers of 2
 * first is slower ... ] */
GEN
seq_umul(ulong a, ulong b)
{
  pari_sp av = avma;
  ulong k, l, N, n = b - a + 1;
  long lx;
  GEN x;

  if (n < 61)
  {
    x = utoi(a);
    for (k=a+1; k<=b; k++) x = mului(k,x);
    return gerepileuptoint(av, x);
  }
  lx = 1; x = cgetg(2 + n/2, t_VEC);
  N = b + a;
  for (k = a;; k++)
  {
    l = N - k; if (l <= k) break;
    x[lx++] = (long)muluu(k,l);
  }
  if (l == k) x[lx++] = lutoi(k);
  setlg(x, lx);
  return gerepileuptoint(av, divide_conquer_prod(x, mulii));
}

GEN
mpfact(long n)
{
  if (n < 2)
  {
    if (n < 0) err(talker,"negative argument in factorial function");
    return gun;
  }
  return seq_umul(2UL, (ulong)n);
}

/*******************************************************************/
/**                                                               **/
/**                      LUCAS & FIBONACCI                        **/
/**                                                               **/
/*******************************************************************/

void
lucas(long n, GEN *ln, GEN *ln1)
{
  pari_sp av;
  long taille;
  GEN z, t;

  if (!n) { *ln = stoi(2); *ln1 = stoi(1); return; }

  taille = 3 + (long)(pariC3 * (1+labs(n)));
  *ln = cgeti(taille);
  *ln1= cgeti(taille);
  av = avma; lucas(n/2, &z, &t);
  switch(n % 4)
  {
    case -3:
      addsiz(2,sqri(z), *ln1);
      subiiz(addsi(1,mulii(z,t)),*ln1, *ln); break;
    case -1:
      addsiz(-2,sqri(z), *ln1);
      subiiz(addsi(-1, mulii(z,t)),*ln1, *ln); break;
    case  0: addsiz(-2,sqri(z),    *ln); addsiz(-1,mulii(z,t), *ln1); break;
    case  1: addsiz(-1,mulii(z,t), *ln); addsiz(2,sqri(t),    *ln1); break;
    case -2:
    case  2: addsiz(2,sqri(z),    *ln); addsiz(1,mulii(z,t), *ln1); break;
    case  3: addsiz(1,mulii(z,t), *ln); addsiz(-2,sqri(t),   *ln1);
  }
  avma = av;
}

GEN
fibo(long n)
{
  pari_sp av = avma;
  GEN ln,ln1;

  lucas(n-1,&ln,&ln1);
  return gerepileupto(av, divis(addii(shifti(ln,1),ln1), 5));
}

/*******************************************************************/
/*                                                                 */
/*                      CONTINUED FRACTIONS                        */
/*                                                                 */
/*******************************************************************/
static GEN
icopy_lg(GEN x, long l)
{
  long lx = lgefint(x);
  GEN y;
  
  if (lx >= l) return icopy(x);
  y = cgeti(l); affii(x, y); return y;
}

/* if y != NULL, stop as soon as partial quotients differ from y */
static GEN
Qsfcont(GEN x, GEN y, long k)
{
  GEN  z, p1, p2, p3;
  long i, l, ly;

  ly = lgefint(x[2]);
  l = 3 + (long) ((ly-2) / pariC3);
  if (k > 0 && ++k > 0 && l > k) l = k; /* beware overflow */
  if ((ulong)l > LGBITS) l = LGBITS;

  p1 = (GEN)x[1];
  p2 = (GEN)x[2];
  z = cgetg(l,t_VEC);
  l--;
  if (y) {
    pari_sp av = avma;
    if (l >= lg(y)) l = lg(y)-1;
    for (i = 1; i <= l; i++)
    {
      z[i] = y[i];
      p3 = p2; if (!gcmp1((GEN)z[i])) p3 = mulii((GEN)z[i], p2);
      p3 = subii(p1, p3);
      if (signe(p3) < 0)
      { /* partial quotient too large */
        p3 = addii(p3, p2);
        if (signe(p3) >= 0) i++; /* by 1 */
        break;
      }
      if (cmpii(p3, p2) >= 0)
      { /* partial quotient too small */
        p3 = subii(p3, p2);
        if (cmpii(p3, p2) < 0) i++; /* by 1 */
        break;
      }
      if ((i & 0xff) == 0) gerepileall(av, 2, &p2, &p3);
      p1 = p2; p2 = p3;
    }
  } else {
    p1 = icopy_lg(p1, ly);
    p2 = icopy(p2);
    for (i = 1; i <= l; i++)
    {
      z[i] = (long)truedvmdii(p1,p2,&p3);
      if (p3 == gzero) { i++; break; }
      affii(p3, p1); cgiv(p3); p3 = p1;
      p1 = p2; p2 = p3;
    }
  }
  i--;
  if (i > 1 && gcmp1((GEN)z[i]))
  {
    cgiv((GEN)z[i]); --i;
    if (is_universal_constant(z[i])) 
      z[i] = laddsi(1, (GEN)z[i]); /* may be gzero */
    else
      addsiz(1,(GEN)z[i], (GEN)z[i]);
  }
  setlg(z,i+1); return z;
}

static GEN
sfcont(GEN x, long k)
{
  pari_sp av;
  long lx,tx=typ(x),e,i,l;
  GEN  y,p1,p2,p3;

  if (is_scalar_t(tx))
  {
    if (gcmp0(x)) return _vec(gzero);
    switch(tx)
    {
      case t_INT: y = cgetg(2,t_VEC); y[1] = (long)icopy(x); return y;
      case t_REAL:
        av = avma; lx = lg(x);
        e = bit_accuracy(lx)-1-expo(x);
        if (e < 0) err(talker,"integral part not significant in sfcont");
        p2=ishiftr_spec(x,lx,0);
        p1 = cgetg(3, t_FRAC);
	p1[1] = (long)p2;
	p1[2] = lshifti(gun, e);
       
        p3 = cgetg(3, t_FRAC);
	p3[1] = laddsi(signe(x), p2);
	p3[2] = p1[2];
	p1 = Qsfcont(p1,NULL,k);
	return gerepilecopy(av, Qsfcont(p3,p1,k));

      case t_FRAC:
        av = avma;
        return gerepileupto(av, Qsfcont(x, NULL, k));
    }
    err(typeer,"sfcont");
  }

  switch(tx)
  {
    case t_POL: return _veccopy(x);
    case t_SER:
      av = avma; p1 = gtrunc(x);
      return gerepileupto(av,sfcont(p1,k));
    case t_RFRAC:
      av = avma;
      l = typ(x[1]) == t_POL? lg(x[1]): 3;
      if (lg(x[2]) > l) l = lg(x[2]);
      if (k > 0 && l > k+1) l = k+1;
      y = cgetg(l,t_VEC);
      p1 = (GEN)x[1];
      p2 = (GEN)x[2];
      for (i=1; i<l; i++)
      {
	y[i] = (long)poldivrem(p1,p2,&p3);
        if (gcmp0(p3)) { i++; break; }
	p1 = p2; p2 = p3;
      }
      setlg(y, i); return gerepilecopy(av, y);
  }
  err(typeer,"sfcont");
  return NULL; /* not reached */
}

static GEN
sfcont2(GEN b, GEN x, long k)
{
  pari_sp av = avma;
  long l1 = lg(b), tx = typ(x), i;
  GEN y,p1;

  if (k)
  {
    if (k>=l1) err(talker,"list of numerators too short in sfcontf2");
    l1 = k+1;
  }
  y=cgetg(l1,t_VEC);
  if (l1==1) return y;
  if (is_scalar_t(tx))
  {
    if (!is_intreal_t(tx) && tx != t_FRAC) err(typeer,"sfcont2");
  }
  else if (tx == t_SER) x = gtrunc(x);

  if (!gcmp1((GEN)b[1])) x = gmul((GEN)b[1],x);
  i = 2; y[1] = lfloor(x); p1 = gsub(x,(GEN)y[1]);
  for (  ; i<l1 && !gcmp0(p1); i++)
  {
    x = gdiv((GEN)b[i],p1);
    if (tx == t_REAL)
    {
      long e = expo(x);
      if (e>0 && (e>>TWOPOTBITS_IN_LONG)+3 > lg(x)) break;
    }
    y[i] = lfloor(x);
    p1 = gsub(x,(GEN)y[i]);
  }
  setlg(y,i); 
  return gerepilecopy(av,y);
}


GEN
gcf(GEN x)
{
  return sfcont(x,0);
}

GEN
gcf2(GEN b, GEN x)
{
  return contfrac0(x,b,0);
}

GEN
gboundcf(GEN x, long k)
{
  return sfcont(x,k);
}

GEN
contfrac0(GEN x, GEN b, long flag)
{
  long lb,tb,i;
  GEN y;

  if (!b || gcmp0(b)) return sfcont(x,flag);
  tb = typ(b);
  if (tb == t_INT) return sfcont(x,itos(b));
  if (! is_matvec_t(tb)) err(typeer,"contfrac0");

  lb=lg(b); if (lb==1) return cgetg(1,t_VEC);
  if (tb != t_MAT) return sfcont2(b,x,flag);
  if (lg(b[1])==1) return sfcont(x,flag);
  y = (GEN) gpmalloc(lb*sizeof(long));
  for (i=1; i<lb; i++) y[i]=coeff(b,1,i);
  x = sfcont2(y,x,flag); free(y); return x;
}

GEN
pnqn(GEN x)
{
  pari_sp av=avma,tetpil;
  long lx,ly,tx=typ(x),i;
  GEN y,p0,p1,q0,q1,a,b,p2,q2;

  if (! is_matvec_t(tx)) err(typeer,"pnqn");
  lx=lg(x); if (lx==1) return idmat(2);
  p0=gun; q0=gzero;
  if (tx != t_MAT)
  {
    p1=(GEN)x[1]; q1=gun;
    for (i=2; i<lx; i++)
    {
      a=(GEN)x[i];
      p2=gadd(gmul(a,p1),p0); p0=p1; p1=p2;
      q2=gadd(gmul(a,q1),q0); q0=q1; q1=q2;
    }
  }
  else
  {
    ly=lg(x[1]);
    if (ly==2)
    {
      p1=cgetg(lx,t_VEC); for (i=1; i<lx; i++) p1[i]=mael(x,i,1);
      tetpil=avma; return gerepile(av,tetpil,pnqn(p1));
    }
    if (ly!=3) err(talker,"incorrect size in pnqn");
    p1=gcoeff(x,2,1); q1=gcoeff(x,1,1);
    for (i=2; i<lx; i++)
    {
      a=gcoeff(x,2,i); b=gcoeff(x,1,i);
      p2=gadd(gmul(a,p1),gmul(b,p0)); p0=p1; p1=p2;
      q2=gadd(gmul(a,q1),gmul(b,q0)); q0=q1; q1=q2;
    }
  }
  tetpil=avma; y=cgetg(3,t_MAT);
  p2=cgetg(3,t_COL); y[1]=(long)p2; p2[1]=lcopy(p1); p2[2]=lcopy(q1);
  p2=cgetg(3,t_COL); y[2]=(long)p2; p2[1]=lcopy(p0); p2[2]=lcopy(q0);
  return gerepile(av,tetpil,y);
}

/* x t_INTMOD. Look for coprime integers a<=A and b<=B, such that a/b = x */
GEN
bestappr_mod(GEN x, GEN A, GEN B)
{
  long i,lx,tx;
  GEN y;
  tx = typ(x);
  switch(tx)
  {
    case t_INTMOD:
    {
      pari_sp av = avma;
      GEN a,b,d, t = cgetg(3, t_FRAC);
      if (! ratlift((GEN)x[2], (GEN)x[1], &a,&b,A,B)) return NULL;
      if (is_pm1(b)) return icopy_av(a, (GEN)av);
      d = mppgcd(a,b);
      if (!is_pm1(d)) { avma = av; return NULL; }
      cgiv(d);
      t[1] = (long)a;
      t[2] = (long)b; return t;
    }
    case t_COMPLEX: case t_POL: case t_SER: case t_RFRAC:
    case t_VEC: case t_COL: case t_MAT:
      y = init_gen_op(x, tx, &lx, &i);
      for (; i<lx; i++)
      {
        GEN t = bestappr_mod((GEN)x[i],A,B);
        if (!t) return NULL;
        y[i] = (long)t;
      }
      return y;
  }
  err(typeer,"bestappr0");
  return NULL; /* not reached */
}

GEN
bestappr(GEN x, GEN k)
{
  pari_sp av = avma;
  long tx = typ(x), tk = typ(k), lx, i;
  GEN p0, p1, p, q0, q1, q, a, y;

  if (tk != t_INT)
  {
    long e;
    if (tk != t_REAL && tk != t_FRAC)
      err(talker,"incorrect bound type in bestappr");
    k = gcvtoi(k,&e);
  }
  if (signe(k) <= 0) k = gun;
  switch(tx)
  {
    case t_INT:
      avma = av; return icopy(x);

    case t_FRAC:
      if (cmpii((GEN)x[2],k) <= 0) { avma = av; return gcopy(x); }
      y = x;
      p1 = gun; a = p0 = gfloor(x); q1 = gzero; q0 = gun;
      while (cmpii(q0,k) <= 0)
      {
	x = gsub(x,a); /* 0 <= x < 1 */
	if (gcmp0(x)) { p1 = p0; q1 = q0; break; }

	x = ginv(x); /* > 1 */
        a = typ(x)==t_INT? x: divii((GEN)x[1], (GEN)x[2]);
        if (cmpii(a,k) > 0)
        { /* next partial quotient will overflow limits */
          GEN n, d;
          a = divii(subii(k, q1), q0);
	  p = addii(mulii(a,p0), p1); p1=p0; p0=p;
          q = addii(mulii(a,q0), q1); q1=q0; q0=q; 
          /* compare |y-p0/q0|, |y-p1/q1| */
          n = (GEN)y[1];
          d = (GEN)y[2];
          if (absi_cmp(mulii(q1, subii(mulii(q0,n), mulii(d,p0))),
                       mulii(q0, subii(mulii(q1,n), mulii(d,p1)))) < 0)
                       { p1 = p0; q1 = q0; }
          break;
        }
	p = addii(mulii(a,p0), p1); p1=p0; p0=p;
        q = addii(mulii(a,q0), q1); q1=q0; q0=q;
      }
      return gerepileupto(av, gdiv(p1,q1));

    case t_REAL: {
      GEN kr;
      
      if (!signe(x)) return gzero; /* faster. Besides itor crashes on x = 0 */
      kr = itor(k, lg(x));
      y = x;
      p1 = gun; a = p0 = mpent(x); q1 = gzero; q0 = gun;
      while (cmpii(q0,k) <= 0)
      {
	x = mpsub(x,a); /* 0 <= x < 1 */
	if (!signe(x)) { p1 = p0; q1 = q0; break; }

	x = ginv(x); /* > 1 */
        if (cmprr(x,kr) > 0)
        { /* next partial quotient will overflow limits */
          a = divii(subii(k, q1), q0);
	  p = addii(mulii(a,p0), p1); p1=p0; p0=p;
          q = addii(mulii(a,q0), q1); q1=q0; q0=q; 
          /* compare |y-p0/q0|, |y-p1/q1| */
          if (absr_cmp(mpmul(q1, mpsub(mulir(q0,y), p0)),
                       mpmul(q0, mpsub(mulir(q1,y), p1))) < 0)
                       { p1 = p0; q1 = q0; }
          break;
        }
        a = mptrunc(x); /* mptrunc(x) may raise precer */
	p = addii(mulii(a,p0), p1); p1=p0; p0=p;
        q = addii(mulii(a,q0), q1); q1=q0; q0=q;
      }
      return gerepileupto(av, gdiv(p1,q1));
   }
   case t_COMPLEX: case t_POL: case t_SER: case t_RFRAC:
   case t_VEC: case t_COL: case t_MAT:
      y = init_gen_op(x, tx, &lx, &i);
      for (; i<lx; i++) y[i] = (long)bestappr((GEN)x[i],k);
      return y;
  }
  err(typeer,"bestappr");
  return NULL; /* not reached */
}

GEN
bestappr0(GEN x, GEN a, GEN b)
{
  pari_sp av;
  GEN t;
  if (!b) return bestappr(x,a);
  av = avma;
  t = bestappr_mod(x,a,b);
  if (!t) { avma = av; return stoi(-1); }
  return t;
}

/*******************************************************************/
/*                                                                 */
/*         QUADRATIC POLYNOMIAL ASSOCIATED TO A DISCRIMINANT       */
/*                                                                 */
/*******************************************************************/

void
check_quaddisc(GEN x, long *s, long *r, char *f)
{
  if (typ(x) != t_INT) err(arither1);
  *s = signe(x); if (!*s) err(talker,"zero discriminant in %s", f);
  if (carreparfait(x)) err(talker,"square discriminant in %s", f);
  *r = mod4(x); if (*s < 0 && *r) *r = 4 - *r;
  if (*r > 1) err(talker, "discriminant not congruent to 0,1 mod 4 in %s", f);
}
void
check_quaddisc_real(GEN x, long *r, char *f)
{
  long sx; check_quaddisc(x, &sx, r, f);
  if (sx < 0) err(talker, "negative discriminant in %s", f);
}
void
check_quaddisc_imag(GEN x, long *r, char *f)
{
  long sx; check_quaddisc(x, &sx, r, f);
  if (sx > 0) err(talker, "positive discriminant in %s", f);
}

GEN
quadpoly0(GEN x, long v)
{
  long res, i, sx, tx = typ(x);
  pari_sp av;
  GEN y, p1;

  if (is_matvec_t(tx))
  {
    long l = lg(x);
    y = cgetg(l, tx);
    for (i=1; i<l; i++) y[i] = (long)quadpoly0((GEN)x[i],v);
    return y;
  }
  if (v < 0) v = 0;
  check_quaddisc(x, &sx, &res, "quadpoly");
  y = cgetg(5,t_POL);
  y[1] = evalsigne(1) | evalvarn(v);

  av = avma; p1 = shifti(x,-2); setsigne(p1,-signe(p1));
  y[2] = (long) p1; /* - floor(x/4) [ = -x/4 or (1-x)/4 ] */
  if (!res) y[3] = zero;
  else
  {
    if (sx < 0) y[2] = lpileuptoint(av, addsi(1,p1));
    y[3] = lnegi(gun);
  }
  y[4] = un; return y;
}

GEN
quadpoly(GEN x) { return quadpoly0(x, -1); }

GEN
quadgen(GEN x)
{
  GEN y = cgetg(4,t_QUAD);
  y[1] = lquadpoly(x); y[2] = zero; y[3] = un; return y;
}

/***********************************************************************/
/**                                                                   **/
/**         FUNDAMENTAL UNIT AND REGULATOR (QUADRATIC FIELDS)         **/
/**                                                                   **/
/***********************************************************************/

GEN
gfundunit(GEN x) { return garith_proto(fundunit,x,1); }

static GEN
get_quad(GEN f, GEN pol, long r)
{
  GEN y = cgetg(4,t_QUAD), c = (GEN)f[2], p1 = (GEN)c[1], q1 = (GEN)c[2];
  
  y[1] = (long)pol;
  y[2] = r? lsubii(p1,q1): (long)p1;
  y[3] = (long)q1; return y;
}

/* replace f by f * [a,1; 1,0] */
static void
update_f(GEN f, GEN a)
{
  GEN p1;
  p1 = gcoeff(f,1,1);
  coeff(f,1,1) = laddii(mulii(a,p1), gcoeff(f,1,2));
  coeff(f,1,2) = (long)p1;

  p1 = gcoeff(f,2,1);
  coeff(f,2,1) = laddii(mulii(a,p1), gcoeff(f,2,2));
  coeff(f,2,2) = (long)p1;
}

GEN
fundunit(GEN x)
{
  pari_sp av = avma, av2, lim;
  long r, flp, flq;
  GEN pol, y, a, u, v, u1, v1, sqd, f;

  check_quaddisc_real(x, &r, "fundunit");
  sqd = racine(x); av2 = avma; lim = stack_lim(av2,2);
  a = shifti(addsi(r,sqd),-1);
  f = cgetg(3,t_MAT); f[1] = lgetg(3,t_COL); f[2] = lgetg(3,t_COL);
  coeff(f,1,1) = (long)a; coeff(f,1,2) = un;
  coeff(f,2,1) = un;      coeff(f,2,2) = zero;
  v = gdeux; u = stoi(r);
  for(;;)
  {
    u1 = subii(mulii(a,v),u);       flp = egalii(u,u1); u = u1;
    v1 = divii(subii(x,sqri(u)),v); flq = egalii(v,v1); v = v1;
    if (flq) break; a = divii(addii(sqd,u),v);
    if (flp) break; update_f(f,a);
    if (low_stack(lim, stack_lim(av2,2)))
    {
      if(DEBUGMEM>1) err(warnmem,"fundunit");
      gerepileall(av2,4, &a,&f,&u,&v);
    }
  }
  pol = quadpoly(x);
  y = get_quad(f,pol,r);
  if (!flq) v1 = y; else { update_f(f,a); v1 = get_quad(f,pol,r); }

  y = gdiv(v1, gconj(y));
  if (signe(y[3]) < 0) y = gneg(y);
  return gerepileupto(av, y);
}

GEN
gregula(GEN x, long prec) { return garith_proto2gs(regula,x,prec); }

GEN
regula(GEN x, long prec)
{
  pari_sp av = avma, av2, lim;
  long r, fl, rexp;
  GEN reg, rsqd, y, u, v, u1, v1, sqd = racine(x);

  check_quaddisc_real(x, &r, "regula");
  rsqd = gsqrt(x,prec);
  rexp = 0; reg = stor(2, prec);
  av2 = avma; lim = stack_lim(av2,2);
  u = stoi(r); v = gdeux;
  for(;;)
  {
    u1 = subii(mulii(divii(addii(u,sqd),v), v), u);
    v1 = divii(subii(x,sqri(u1)),v); fl = egalii(v,v1);
    if (fl || egalii(u,u1)) break;
    reg = mulrr(reg, divri(addir(u1,rsqd),v));
    rexp += expo(reg); setexpo(reg,0);
    u = u1; v = v1;
    if (rexp & ~EXPOBITS) err(talker,"exponent overflow in regula");
    if (low_stack(lim, stack_lim(av2,2)))
    {
      if(DEBUGMEM>1) err(warnmem,"regula");
      gerepileall(av2,3, &reg,&u,&v);
    }
  }
  reg = gsqr(reg); setexpo(reg,expo(reg)-1);
  if (fl) reg = mulrr(reg, divri(addir(u1,rsqd),v));
  y = mplog(divri(reg,v));
  if (rexp)
  {
    u1 = mulsr(rexp, mplog2(prec));
    setexpo(u1, expo(u1)+1);
    y = addrr(y,u1);
  }
  return gerepileupto(av, y);
}

/*************************************************************************/
/**                                                                     **/
/**                            CLASS NUMBER                             **/
/**                                                                     **/
/*************************************************************************/

static GEN
gclassno(GEN x) { return garith_proto(classno,x,1); }

static GEN
gclassno2(GEN x) { return garith_proto(classno2,x,1); }

GEN
qfbclassno0(GEN x,long flag)
{
  switch(flag)
  {
    case 0: return gclassno(x);
    case 1: return gclassno2(x);
    default: err(flagerr,"qfbclassno");
  }
  return NULL; /* not reached */
}

/* f^h = 1, return order(f) */
static GEN
find_order(GEN f, GEN h)
{
  GEN fh, p,e;
  long i,j,lim;

  p = decomp(h);
  e =(GEN)p[2];
  p =(GEN)p[1];
  for (i=1; i<lg(p); i++)
  {
    lim = itos((GEN)e[i]);
    for (j=1; j<=lim; j++)
    {
      GEN p1 = diviiexact(h,(GEN)p[i]);
      fh = powgi(f,p1);
      if (!is_pm1(fh[1])) break;
      h = p1;
    }
  }
  return h;
}

static GEN
end_classno(GEN h, GEN hin, GEN forms, long lform)
{
  long i,com;
  GEN a,b,p1,q,fh,fg, f = (GEN)forms[0];

  h = find_order(f,h); /* H = <f> */
  q = diviiround(hin, h); /* approximate order of G/H */
  for (i=1; i < lform; i++)
  {
    pari_sp av = avma; 
    fg = powgi((GEN)forms[i], h);
    fh = powgi(fg, q);
    a = (GEN)fh[1];
    if (is_pm1(a)) continue;
    b = (GEN)fh[2]; p1 = fg;
    for (com=1; ; com++, p1 = gmul(p1,fg))
      if (egalii((GEN)p1[1], a) && absi_equal((GEN)p1[2], b)) break;
    if (signe(p1[2]) == signe(b)) com = -com;
    /* f_i ^ h(q+com) = 1 */
    q = addsi(com,q);
    if (gcmp0(q))
    { /* f^(ih) != 1 for all 0 < i <= oldq. Happen if the original upper bound
         for h was wrong */
      long c;
      p1 = fh;
      for (c=1; ; c++, p1 = gmul(p1,fh))
        if (gcmp1((GEN)p1[1])) break;
      q = mulsi(-com, find_order(fh, stoi(c)));
    }
    q = gerepileuptoint(av, q);
  }
  return mulii(q,h);
}

static GEN
to_form(GEN x, long f)
{
  return redimag(primeform(x, stoi(f), 0));
}

/* r = x mod 4 */
static GEN
conductor_part(GEN x, long r, GEN *ptD, GEN *ptreg, GEN *ptfa)
{
  long n,i,k,s=signe(x),fl2;
  GEN e,p,H,d,D,fa,reg;

  fa = auxdecomp(absi(x),1);
  e = gtovecsmall((GEN)fa[2]);
  fa = (GEN)fa[1];
  n = lg(fa); d = gun;
  for (i=1; i<n; i++)
    if (e[i] & 1) d = mulii(d,(GEN)fa[i]);
  if (r) fl2 = 0; else { fl2 = 1; d = shifti(d,2); }
  H = gun; D = (s<0)? negi(d): d; /* d = abs(D) */
  /* f \prod_{p|f}  [ 1 - (D/p) p^-1 ] */
  for (i=1; i<n; i++)
  {
    p = (GEN)fa[i];
    k = e[i];
    if (fl2 && i==1) k -= 2; /* p = 2 */
    if (k >= 2)
    {
      H = mulii(H, subis(p, kronecker(D,p)));
      if (k>=4) H = mulii(H,gpowgs(p,(k>>1)-1));
    }
  }
  
  /* divide by [ O^* : O_K^* ] */
  if (s < 0)
  {
    reg = NULL;
    switch(itos_or_0(d))
    {
      case 4: H = divis(H,2); break;
      case 3: H = divis(H,3); break;
    }
  } else {
    reg = regula(D,DEFAULTPREC);
    if (!egalii(x,D))
      H = divii(H, ground(gdiv(regula(x,DEFAULTPREC), reg)));
  }
  if (ptreg) *ptreg = reg;
  if (ptfa)  *ptfa = fa;
  *ptD = D; return H;
}

static long
two_rank(GEN x)
{
  GEN p = (GEN)decomp(absi(x))[1];
  long l = lg(p)-1;
#if 0 /* positive disc not needed */
  if (signe(x) > 0)
  {
    long i;
    for (i=1; i<=l; i++)
      if (mod4((GEN)p[i]) == 3) { l--; break; }
  }
#endif
  return l-1;
}

#define MAXFORM 11
#define _low(to, x) { GEN __x = (GEN)(x); to = modBIL(__x); }

/* h(x) for x<0 using Baby Step/Giant Step.
 * Assumes G is not too far from being cyclic.
 * 
 * Compute G^2 instead of G so as to kill most of the non-cyclicity */
GEN
classno(GEN x)
{
  pari_sp av = avma, av2, lim;
  long r2,c,lforms,k,l,i,j,com,s, forms[MAXFORM];
  GEN count,index,tabla,tablb,hash,p1,p2,hin,h,f,fh,fg,ftest;
  GEN Hf, D;
  byteptr p = diffptr;

  if (signe(x) >= 0) return classno2(x);

  check_quaddisc(x, &s, &k, "classno");
  if (cmpis(x,-12) >= 0) return gun;

  Hf = conductor_part(x, k, &D, NULL, NULL);
  if (cmpis(D,-12) >= 0) return gerepilecopy(av, Hf);

  p2 = gsqrt(absi(D),DEFAULTPREC);
  p1 = mulrr(divrr(p2,mppi(DEFAULTPREC)), dbltor(1.005)); /*overshoot by 0.5%*/
  s = itos_or_0( mptrunc(shiftr(mpsqrt(p2), 1)) ); 
  if (!s) err(talker,"discriminant too big in classno");
  if (s < 10) s = 200;
  else if (s < 20) s = 1000;
  else if (s < 5000) s = 5000;

  c = lforms = 0; maxprime_check(s);
  while (c <= s)
  {
    long d;
    NEXT_PRIME_VIADIFF(c,p);

    k = krogs(D,c); if (!k) continue;
    if (k > 0)
    {
      if (lforms < MAXFORM) forms[lforms++] = c; 
      d = c - 1;
    }
    else
      d = c + 1;
    av2 = avma;
    divrsz(mulsr(c,p1),d, p1);
    avma = av2; 
  }
  r2 = two_rank(D);
  h = hin = ground(gmul2n(p1, -r2));
  s = 2*itos(gceil(mpsqrtn(p1, 4)));
  if (s > 10000) s = 10000;

  count = new_chunk(256); for (i=0; i<=255; i++) count[i]=0;
  index = new_chunk(257);
  tabla = new_chunk(10000);
  tablb = new_chunk(10000);
  hash  = new_chunk(10000);
  f = gsqr(to_form(D, forms[0]));
  p1 = fh = powgi(f, h);
  for (i=0; i<s; i++, p1 = compimag(p1,f))
  {
    _low(tabla[i], p1[1]);
    _low(tablb[i], p1[2]); count[tabla[i]&255]++;
  }
  /* follow the idea of counting sort to avoid maintaining linked list� in
   * hashtable */
  index[0]=0; for (i=0; i< 255; i++) index[i+1] = index[i]+count[i];
  /* index[i] = # of forms hashing to <= i */
  for (i=0; i<s; i++) hash[ index[tabla[i]&255]++ ] = i;
  index[0]=0; for (i=0; i<=255; i++) index[i+1] = index[i]+count[i];
  /* hash[index[i-1]..index[i]-1] = forms hashing to i */

  fg = gpowgs(f,s); av2 = avma; lim = stack_lim(av2,2);
  ftest = gpowgs(p1,0);
  for (com=0; ; com++)
  {
    long j1, j2;
    GEN a, b;
    a = (GEN)ftest[1]; _low(k, a);
    b = (GEN)ftest[2]; _low(l, b); j = k&255;
    for (j1=index[j]; j1 < index[j+1]; j1++)
    {
      j2 = hash[j1];
      if (tabla[j2] == k && tablb[j2] == l)
      {
        p1 = gmul(gpowgs(f,j2),fh);
        if (egalii((GEN)p1[1], a) && absi_equal((GEN)p1[2], b))
        { /* p1 = ftest or ftest^(-1), we are done */
          if (signe(p1[2]) == signe(b)) com = -com;
          h = addii(addis(h,j2), mulss(s,com));
          forms[0] = (long)f;
          for (i=1; i<lforms; i++)
            forms[i] = (long)gsqr(to_form(D, forms[i]));
          h = end_classno(h,hin,forms,lforms);
          h = mulii(h,Hf);
          return gerepileuptoint(av, shifti(h, r2));
        }
      }
    }
    ftest = gmul(ftest,fg);
    if (is_pm1(ftest[1])) err(impl,"classno with too small order");
    if (low_stack(lim, stack_lim(av2,2))) ftest = gerepileupto(av2,ftest);
  }
}

/* use Euler products */
GEN
classno2(GEN x)
{
  pari_sp av = avma;
  long n, i, k, r, s;
  GEN p1,p2,p3,p4,p5,p7,Hf,Pi,reg,logd,d,D;

  check_quaddisc(x, &s, &r, "classno2");
  if (s < 0 && cmpis(x,-12) >= 0) return gun;

  Hf = conductor_part(x, r, &D, &reg, NULL);
  if (s < 0 && cmpis(D,-12) >= 0) return gerepilecopy(av, Hf); /* |D| < 12*/

  Pi = mppi(DEFAULTPREC);
  d = absi(D); logd = glog(d,DEFAULTPREC);
  p1 = mpsqrt(gdiv(gmul(d,logd), gmul2n(Pi,1)));
  if (s > 0)
  {
    p2 = subsr(1, gmul2n(divrr(mplog(reg),logd),1));
    if (gcmp(gsqr(p2), divsr(2,logd)) >= 0) p1 = gmul(p2,p1);
  }
  n = itos_or_0( mptrunc(p1) );
  if (!n) err(talker,"discriminant too large in classno");

  p4 = divri(Pi,d); p7 = ginv(mpsqrt(Pi));
  p1 = gsqrt(d,DEFAULTPREC); p3 = gzero;
  if (s > 0)
  {
    for (i=1; i<=n; i++)
    {
      k = krogs(D,i); if (!k) continue;
      p2 = mulir(mulss(i,i),p4);
      p5 = subsr(1,mulrr(p7,incgamc(ghalf,p2,DEFAULTPREC)));
      p5 = addrr(divrs(mulrr(p1,p5),i), eint1(p2,DEFAULTPREC));
      p3 = (k>0)? addrr(p3,p5): subrr(p3,p5);
    }
    p3 = shiftr(divrr(p3,reg),-1);
  }
  else
  {
    p1 = gdiv(p1,Pi);
    for (i=1; i<=n; i++)
    {
      k = krogs(D,i); if (!k) continue;
      p2 = mulir(mulss(i,i),p4);
      p5 = subsr(1,mulrr(p7,incgamc(ghalf,p2,DEFAULTPREC)));
      p5 = addrr(p5, divrr(divrs(p1,i),mpexp(p2)));
      p3 = (k>0)? addrr(p3,p5): subrr(p3,p5);
    }
  }
  return gerepileuptoint(av, mulii(Hf,ground(p3)));
}

GEN
hclassno(GEN x)
{
  long d,a,b,h,b2,f;

  d= -itos(x); if (d>0 || (d & 3) > 1) return gzero;
  if (!d) return gdivgs(gun,-12);
  if (-d > (VERYBIGINT>>1))
    err(talker,"discriminant too big in hclassno. Use quadclassunit");
  h=0; b=d&1; b2=(b-d)>>2; f=0;
  if (!b)
  {
    for (a=1; a*a<b2; a++)
      if (b2%a == 0) h++;
    f = (a*a==b2); b=2; b2=(4-d)>>2;
  }
  while (b2*3+d<0)
  {
    if (b2%b == 0) h++;
    for (a=b+1; a*a<b2; a++)
      if (b2%a == 0) h+=2;
    if (a*a==b2) h++;
    b+=2; b2=(b*b-d)>>2;
  }
  if (b2*3+d==0)
  {
    GEN y = cgetg(3,t_FRAC);
    y[1]=lstoi(3*h+1); y[2]=lstoi(3); return y;
  }
  if (f) return gaddsg(h,ghalf);
  return stoi(h);
}

/***********************************************************************/
/**                                                                   **/
/**                      BINARY QUADRATIC FORMS                       **/
/**                                                                   **/
/***********************************************************************/

GEN
qf_disc(GEN x, GEN y, GEN z)
{
  if (!y) { y=(GEN)x[2]; z=(GEN)x[3]; x=(GEN)x[1]; }
  return subii(sqri(y), shifti(mulii(x,z),2));
}

static GEN
qf_create(GEN x, GEN y, GEN z, long s)
{
  GEN t;
  if (typ(x)!=t_INT || typ(y)!=t_INT || typ(z)!=t_INT) err(typeer,"Qfb");
  if (!s)
  {
    pari_sp av=avma; s = signe(qf_disc(x,y,z)); avma=av;
    if (!s) err(talker,"zero discriminant in Qfb");
  }
  if (s < 0)
  {
    t = cgetg(4,t_QFI);
    if (signe(x) < 0) err(impl,"negative definite t_QFI");
  }
  else
    t = cgetg(5,t_QFR);
  t[1]=(long)icopy(x); t[2]=(long)icopy(y); t[3]=(long)icopy(z);
  return t;
}

GEN
qfi(GEN x, GEN y, GEN z)
{
  return qf_create(x,y,z,-1);
}

GEN
qfr(GEN x, GEN y, GEN z, GEN d)
{
  GEN t = qf_create(x,y,z,1);
  if (typ(d) != t_REAL)
    err(talker,"Shanks distance should be a t_REAL (in qfr)");
  t[4]=lrcopy(d); return t;
}

GEN
Qfb0(GEN x, GEN y, GEN z, GEN d, long prec)
{
  GEN t = qf_create(x,y,z,0);
  if (lg(t)==4) return t;
  if (!d) d = gzero;
  if (typ(d) == t_REAL)
    t[4] = lrcopy(d);
  else
    { t[4]=lgetr(prec); gaffect(d,(GEN)t[4]); }
  return t;
}

/* composition */
static void
sq_gen(GEN z, GEN x)
{
  GEN d1, x2, y2, v1, v2, c3, m, p1, r;

  d1 = bezout((GEN)x[2],(GEN)x[1],&x2,&y2);
  if (gcmp1(d1)) v1 = v2 = (GEN)x[1];
  else
  {
    v1 = diviiexact((GEN)x[1],d1);
    v2 = mulii(v1,mppgcd(d1,(GEN)x[3]));
  }
  m = mulii((GEN)x[3],x2);
  setsigne(m,-signe(m));
  r = modii(m,v2); p1 = mulii(v1,r);
  c3 = addii(mulii((GEN)x[3],d1), mulii(r,addii((GEN)x[2],p1)));
  z[1] = lmulii(v1,v2);
  z[2] = laddii((GEN)x[2], shifti(p1,1));
  z[3] = ldivii(c3,v2);
}

void
comp_gen(GEN z,GEN x,GEN y)
{
  GEN s, n, d, d1, x1, x2, y1, y2, v1, v2, c3, m, p1, r;

  if (x == y) { sq_gen(z,x); return; }
  s = shifti(addii((GEN)x[2],(GEN)y[2]),-1);
  n = subii((GEN)y[2],s);
  d = bezout((GEN)y[1],(GEN)x[1],&y1,&x1);
  d1 = bezout(s,d,&x2,&y2);
  if (gcmp1(d1))
  {
    v1 = (GEN)x[1];
    v2 = (GEN)y[1];
  }
  else
  {
    v1 = diviiexact((GEN)x[1],d1);
    v2 = diviiexact((GEN)y[1],d1);
    v1 = mulii(v1, mppgcd(d1,mppgcd((GEN)x[3],mppgcd((GEN)y[3],n))));
  }
  m = addii(mulii(mulii(y1,y2),n), mulii((GEN)y[3],x2));
  setsigne(m,-signe(m));
  r = modii(m,v1); p1 = mulii(v2,r);
  c3 = addii(mulii((GEN)y[3],d1), mulii(r,addii((GEN)y[2],p1)));
  z[1] = lmulii(v1,v2);
  z[2] = laddii((GEN)y[2], shifti(p1,1));
  z[3] = ldivii(c3,v1);
}

static GEN
compimag0(GEN x, GEN y, int raw)
{
  pari_sp av = avma;
  long tx = typ(x);
  GEN z;

  if (typ(y) != tx || tx!=t_QFI) err(typeer,"composition");
  if (absi_cmp((GEN)x[1], (GEN)y[1]) > 0) { z=x; x=y; y=z; }
  z = cgetg(4,t_QFI); comp_gen(z,x,y);
  if (raw) return gerepilecopy(av,z);
  return gerepileupto(av, redimag(z));
}

static GEN
compreal0(GEN x, GEN y, int raw)
{
  pari_sp av = avma;
  long tx = typ(x);
  GEN z;

  if (typ(y) != tx || tx!=t_QFR) err(typeer,"composition");
  z=cgetg(5,t_QFR); comp_gen(z,x,y);
  z[4]=laddrr((GEN)x[4],(GEN)y[4]);
  if (raw) return gerepilecopy(av,z);
  return gerepileupto(av,redreal(z));
}

GEN
compreal(GEN x, GEN y) { return compreal0(x,y,0); }

GEN
comprealraw(GEN x, GEN y) { return compreal0(x,y,1); }

GEN
compimag(GEN x, GEN y) { return compimag0(x,y,0); }

GEN
compimagraw(GEN x, GEN y) { return compimag0(x,y,1); }

GEN
compraw(GEN x, GEN y)
{
  return (typ(x)==t_QFI)? compimagraw(x,y): comprealraw(x,y);
}

GEN
sqcompimag0(GEN x, long raw)
{
  pari_sp av = avma;
  GEN z = cgetg(4,t_QFI);

  if (typ(x)!=t_QFI) err(typeer,"composition");
  sq_gen(z,x);
  if (raw) return gerepilecopy(av,z);
  return gerepileupto(av,redimag(z));
}

static GEN
sqcompreal0(GEN x, long raw)
{
  pari_sp av = avma;
  GEN z = cgetg(5,t_QFR);

  if (typ(x)!=t_QFR) err(typeer,"composition");
  sq_gen(z,x); z[4]=lshiftr((GEN)x[4],1);
  if (raw) return gerepilecopy(av,z);
  return gerepileupto(av,redreal(z));
}

GEN
sqcompreal(GEN x) { return sqcompreal0(x,0); }

GEN
sqcomprealraw(GEN x) { return sqcompreal0(x,1); }

GEN
sqcompimag(GEN x) { return sqcompimag0(x,0); }

GEN
sqcompimagraw(GEN x) { return sqcompimag0(x,1); }

static GEN
real_unit_form_by_disc(GEN D, long prec)
{
  GEN y = cgetg(5,t_QFR), isqrtD;
  pari_sp av = avma;
  long r;

  check_quaddisc_real(D, /*junk*/&r, "real_unit_form_by_disc");
  y[1] = un; isqrtD = racine(D);
  if ((r & 1) != mod2(isqrtD)) /* we know isqrtD > 0 */
    isqrtD = gerepileuptoint(av, addsi(-1,isqrtD));
  y[2] = (long)isqrtD; av = avma;
  y[3] = lpileuptoint(av, shifti(subii(sqri(isqrtD),D),-2));
  y[4] = (long)realzero(prec); return y;
}

GEN
real_unit_form(GEN x)
{
  pari_sp av = avma;
  long prec;
  GEN D;
  if (typ(x) != t_QFR) err(typeer,"real_unit_form");
  prec = precision((GEN)x[4]);
  if (!prec) err(talker,"not a t_REAL in 4th component of a t_QFR");
  D = qf_disc(x,NULL,NULL);
  return gerepileupto(av, real_unit_form_by_disc(D,prec));
}

static GEN
imag_unit_form_by_disc(GEN D)
{
  GEN y = cgetg(4,t_QFI);
  long r;

  check_quaddisc_imag(D, &r, "imag_unit_form_by_disc");
  y[1] = un;
  y[2] = r? un: zero;
  /* upon return, y[3] = (1-D) / 4 or -D / 4, whichever is an integer */
  y[3] = lshifti(D,-2);
  if (r)
  {
    pari_sp av = avma;
    y[3] = lpileuptoint(av, addis((GEN)y[3],-1));
  }
  /* at this point y[3] < 0 */
  setsigne(y[3], 1); return y;
}

GEN
imag_unit_form(GEN x)
{
  GEN p1,p2, y = cgetg(4,t_QFI);
  pari_sp av;
  if (typ(x) != t_QFI) err(typeer,"imag_unit_form");
  y[1] = un;
  y[2] = mpodd((GEN)x[2])? un: zero;
  av = avma; p1 = mulii((GEN)x[1],(GEN)x[3]);
  p2 = shifti(sqri((GEN)x[2]),-2);
  y[3] = lpileuptoint(av, subii(p1,p2));
  return y;
}

static GEN
invraw(GEN x)
{
  GEN y = gcopy(x);
  setsigne(y[2], -signe(y[2]));
  if (typ(y) == t_QFR) setsigne(y[4], -signe(y[4]));
  return y;
}

GEN
powrealraw(GEN x, long n)
{
  pari_sp av = avma;
  long m;
  GEN y;

  if (typ(x) != t_QFR)
    err(talker,"not a real quadratic form in powrealraw");
  if (!n) return real_unit_form(x);
  if (n== 1) return gcopy(x);
  if (n==-1) return invraw(x);

  y = NULL; m=labs(n);
  for (; m>1; m>>=1)
  {
    if (m&1) y = y? comprealraw(y,x): x;
    x=sqcomprealraw(x);
  }
  y = y? comprealraw(y,x): x;
  if (n<0) y = invraw(y);
  return gerepileupto(av,y);
}

GEN
powimagraw(GEN x, long n)
{
  pari_sp av = avma;
  long m;
  GEN y;

  if (typ(x) != t_QFI)
    err(talker,"not an imaginary quadratic form in powimag");
  if (!n) return imag_unit_form(x);
  if (n== 1) return gcopy(x);
  if (n==-1) return invraw(x);

  y = NULL; m=labs(n);
  for (; m>1; m>>=1)
  {
    if (m&1) y = y? compimagraw(y,x): x;
    x=sqcompimagraw(x);
  }
  y = y? compimagraw(y,x): x;
  if (n<0) y = invraw(y);
  return gerepileupto(av,y);
}

GEN
powraw(GEN x, long n)
{
  return (typ(x)==t_QFI)? powimagraw(x,n): powrealraw(x,n);
}

/* composition: Shanks' NUCOMP & NUDUPL */
/* l = floor((|d|/4)^(1/4)) */
GEN
nucomp(GEN x, GEN y, GEN l)
{
  pari_sp av = avma;
  long cz;
  GEN a, a1, a2, b2, b, d, d1, g, n, p1, q1, q2, s, u, u1, v, v1, v2, v3, z;

  if (x==y) return nudupl(x,l);
  if (typ(x) != t_QFI || typ(y) != t_QFI)
    err(talker,"not an imaginary quadratic form in nucomp");

  if (cmpii((GEN)x[1],(GEN)y[1]) < 0) { s=x; x=y; y=s; }
  s = shifti(addii((GEN)x[2],(GEN)y[2]),-1); n = subii((GEN)y[2],s);
  a1 = (GEN)x[1];
  a2 = (GEN)y[1]; d = bezout(a2,a1,&u,&v);
  if (gcmp1(d)) { a = negi(gmul(u,n)); d1 = d; }
  else
    if (divise(s,d)) /* d | s */
    {
      a = negi(mulii(u,n)); d1 = d;
      a1 = diviiexact(a1,d1);
      a2 = diviiexact(a2,d1);
      s = diviiexact(s,d1);
    }
    else
    {
      GEN p2, p3;
      d1 = bezout(s,d,&u1,&v1);
      if (!gcmp1(d1))
      {
        a1 = diviiexact(a1,d1);
        a2 = diviiexact(a2,d1);
        s = diviiexact(s,d1);
        d = diviiexact(d,d1);
      }
      p1 = resii((GEN)x[3],d);
      p2 = resii((GEN)y[3],d);
      p3 = modii(negi(mulii(u1,addii(mulii(u,p1),mulii(v,p2)))), d);
      a = subii(mulii(p3,divii(a1,d)), mulii(u,divii(n,d)));
    }
  a = modii(a,a1); p1 = subii(a1,a); if (cmpii(a,p1) > 0) a = negi(p1);
  v=gzero; d=a1; v2=gun; v3=a;
  for (cz=0; absi_cmp(v3,l) > 0; cz++)
  {
    GEN t2, t3;
    p1 = dvmdii(d,v3,&t3); t2 = subii(v,mulii(p1,v2));
    v=v2; d=v3; v2=t2; v3=t3;
  }
  z = cgetg(4,t_QFI);
  if (!cz)
  {
    g = divii(addii(mulii(v3,s),(GEN)y[3]), d);
    b = a2;
    b2 = (GEN)y[2];
    v2 = d1;
    z[1] = lmulii(d,b);
  }
  else
  {
    GEN e, q3, q4;
    if (cz&1) { v3 = negi(v3); v2 = negi(v2); }
    b = divii(addii(mulii(a2,d), mulii(n,v)),a1);
    e = divii(addii(mulii(s,d),mulii((GEN)y[3],v)),a1);
    q3 = mulii(e,v2);
    q4 = subii(q3,s);
    g = divii(q4,v);
    b2 = addii(q3,q4);
    if (!gcmp1(d1)) { v2 = mulii(d1,v2); v = mulii(d1,v); b2 = mulii(d1,b2); }
    z[1] = laddii(mulii(d,b), mulii(e,v));
  }
  q1 = mulii(b, v3);
  q2 = addii(q1,n);
  z[2] = laddii(b2, cz? addii(q1,q2): shifti(q1, 1));
  z[3] = laddii(mulii(v3,divii(q2,d)), mulii(g,v2));
  return gerepileupto(av, redimag(z));
}

GEN
nudupl(GEN x, GEN l)
{
  pari_sp av = avma;
  long cz;
  GEN u, v, d, d1, p1, a, b, c, b2, z, v2, v3, t2, t3, e, g;

  if (typ(x) != t_QFI)
    err(talker,"not an imaginary quadratic form in nudupl");
  d1 = bezout((GEN)x[2],(GEN)x[1],&u,&v);
  a = diviiexact((GEN)x[1],d1);
  b = diviiexact((GEN)x[2],d1);
  c = modii(negi(mulii(u,(GEN)x[3])),a);
  p1 = subii(a,c);
  if (cmpii(c,p1)>0) c = negi(p1);
  v=gzero; d=a; v2=gun; v3=c;
  for (cz=0; absi_cmp(v3,l) > 0; cz++)
  {
    p1 = dvmdii(d,v3,&t3); t2 = subii(v,mulii(p1,v2));
    v=v2; d=v3; v2=t2; v3=t3;
  }
  z = cgetg(4,t_QFI);
  if (!cz)
  {
    g = divii(addii(mulii(v3,b),(GEN)x[3]), d);
    b2 = (GEN)x[2];
    v2 = d1;
    z[1] = (long)sqri(d);
  }
  else
  {
    if (cz&1) { v = negi(v); d = negi(d); }
    e = divii(addii(mulii((GEN)x[3],v),mulii(b,d)),a);
    g = divii(subii(mulii(e,v2),b),v);
    b2 = addii(mulii(e,v2),mulii(v,g));
    if (!gcmp1(d1)) { v2 = mulii(d1,v2); v = mulii(d1,v); b2 = mulii(d1,b2); }
    z[1] = laddii(sqri(d), mulii(e,v));
  }
  z[2] = laddii(b2, shifti(mulii(d,v3),1));
  z[3] = laddii(sqri(v3), mulii(g,v2));
  return gerepileupto(av, redimag(z));
}

static GEN
mul_nucomp(void *l, GEN x, GEN y) { return nucomp(x, y, (GEN)l); }
static GEN
mul_nudupl(void *l, GEN x) { return nudupl(x, (GEN)l); }

GEN
nupow(GEN x, GEN n)
{
  pari_sp av;
  GEN y, l;

  if (typ(n) != t_INT) err(talker,"not an integer exponent in nupow");
  if (gcmp1(n)) return gcopy(x);
  av = avma; y = imag_unit_form(x);
  if (!signe(n)) return y;

  l = gclone( racine(shifti(racine((GEN)y[3]),1)) );
  avma = av;
  y = leftright_pow(x, n, (void*)l, &mul_nudupl, &mul_nucomp);
  gunclone(l);
  if (signe(n) < 0 && !egalii((GEN)y[1],(GEN)y[2])
                   && !egalii((GEN)y[1],(GEN)y[3])) setsigne(y[2],-signe(y[2]));
  return y;
}

/* reduction */

static GEN
abs_dvmdii(GEN b, GEN p1, GEN *pt, long s)
{
  if (s<0) setsigne(b, 1); p1 = dvmdii(b,p1,pt);
  if (s<0) setsigne(b,-1); return p1;
}

static GEN
rhoimag0(GEN x, long *flag)
{
  GEN p1,b,d,z;
  long fl, s = signe(x[2]);

  fl = cmpii((GEN)x[1], (GEN)x[3]);
  if (fl <= 0)
  {
    long fg = absi_cmp((GEN)x[1], (GEN)x[2]);
    if (fg >= 0)
    {
      *flag = (s<0 && (!fl || !fg))? 2 /* set x[2] = negi(x[2]) in caller */
                                   : 1;
      return x;
    }
  }
  p1=shifti((GEN)x[3],1); d = abs_dvmdii((GEN)x[2],p1,&b,s);
  if (s>=0)
  {
    setsigne(d,-signe(d));
    if (cmpii(b,(GEN)x[3])<=0) setsigne(b,-signe(b));
    else { d=addsi(-1,d); b=subii(p1,b); }
  }
  else if (cmpii(b,(GEN)x[3])>=0) { d=addsi(1,d); b=subii(b,p1); }

  z=cgetg(4,t_QFI);
  z[1] = x[3];
  z[3] = laddii((GEN)x[1], mulii(d,shifti(subii((GEN)x[2],b),-1)));
  if (signe(b)<0 && !fl) setsigne(b,1);
  z[2] = (long)b; *flag = 0; return z;
}

static void
fix_expo(GEN x)
{
  long e = expo(x[5]);
  if (e >= EXP220)
  {
    x[4] = laddsi(1,(GEN)x[4]);
    setexpo(x[5], e - EXP220);
  }
}

GEN
rhoreal_aux(GEN x, GEN D, GEN sqrtD, GEN isqrtD)
{
  GEN p1,p2, y = cgetg(6,t_VEC);
  GEN b = (GEN)x[2];
  GEN c = (GEN)x[3];

  y[1] = (long)c;
  p2 = (absi_cmp(isqrtD,c) >= 0)? isqrtD: absi(c);
  p1 = shifti(c,1);
  if (p1 == gzero) err(talker, "reducible form in rhoreal");
  setsigne(p1,1); /* |2c| */
  p2 = divii(addii(p2,b), p1);
  y[2] = lsubii(mulii(p2,p1), b);

  p1 = shifti(subii(sqri((GEN)y[2]),D),-2);
  y[3] = ldivii(p1,(GEN)y[1]);

  if (lg(x) <= 5) setlg(y,4);
  else
  {
    y[4] = x[4];
    y[5] = x[5];
    if (signe(b))
    {
      p1 = divrr(addir(b,sqrtD), subir(b,sqrtD));
      y[5] = lmulrr(p1, (GEN)y[5]);
      fix_expo(y);
    }
  }
  return y;
}

#define qf_NOD  2
#define qf_STEP 1

GEN
codeform5(GEN x, long prec)
{
  GEN y = cgetg(6,t_VEC);
  y[1] = x[1];
  y[2] = x[2];
  y[3] = x[3];
  y[4] = zero;
  y[5] = (long)realun(prec); return y;
}

static GEN
add_distance(GEN x, GEN d0)
{
  GEN y = cgetg(5, t_QFR);
  y[1] = licopy((GEN)x[1]);
  y[2] = licopy((GEN)x[2]);
  y[3] = licopy((GEN)x[3]);
  y[4] = lcopy(d0); return y;
}

/* d0 = initial distance, assume |n| > 1 */
static GEN
decodeform(GEN x, GEN d0)
{
  GEN p1,p2;

  if (lg(x) < 6) return add_distance(x,d0);
  /* x = (a,b,c, expo(d), d), d = exp(2*distance) */
  p1 = absr((GEN)x[5]);
  p2 = (GEN)x[4];
  if (signe(p2))
  {
    long e = expo(p1);
    p1 = shiftr(p1,-e);
    p2 = addis(mulsi(EXP220,p2), e);
    p1 = mplog(p1);
    p1 = mpadd(p1, mulir(p2, mplog2(lg(d0))));
  }
  else
  { /* to avoid loss of precision */
    p1 = gcmp1(p1)? NULL: mplog(p1);
  }
  if (p1)
    d0 = addrr(d0, shiftr(p1,-1));
  return add_distance(x, d0);
}

static long
get_prec(GEN d)
{
  long k = lg(d);
  long l = ((BITS_IN_LONG-1-expo(d))>>TWOPOTBITS_IN_LONG)+2;
  if (l < k) l = k;
  if (l < 3) l = 3;
  return l;
}

static int
real_isreduced(GEN x, GEN isqrtD)
{
  GEN a = (GEN)x[1];
  GEN b = (GEN)x[2];
  if (signe(b) > 0 && cmpii(b,isqrtD) <= 0 )
  {
    GEN p1 = subii(isqrtD, shifti(absi(a),1));
    long l = absi_cmp(b, p1);
    if (l > 0 || (l == 0 && signe(p1) < 0)) return 1;
  }
  return 0;
}

GEN
redrealform5(GEN x, GEN D, GEN sqrtD, GEN isqrtD)
{
  while (!real_isreduced(x,isqrtD))
    x = rhoreal_aux(x,D,sqrtD,isqrtD);
  return x;
}

static GEN
redreal0(GEN x, long flag, GEN D, GEN isqrtD, GEN sqrtD)
{
  pari_sp av = avma;
  long prec;
  GEN d0;

  if (typ(x) != t_QFR) err(talker,"not a real quadratic form in redreal");

  if (!D)
    D = qf_disc(x,NULL,NULL);
  else
    if (typ(D) != t_INT) err(arither1);

  d0 = (GEN)x[4]; prec = get_prec(d0);
  x = codeform5(x,prec);
  if ((flag & qf_NOD)) setlg(x,4);
  else
  {
    if (!sqrtD)
      sqrtD = gsqrt(D,prec);
    else
    {
      long l = typ(sqrtD);
      if (!is_intreal_t(l)) err(arither1);
    }
  }
  if (!isqrtD)
    isqrtD = sqrtD? mptrunc(sqrtD): racine(D);
  else
    if (typ(isqrtD) != t_INT) err(arither1);

  if (flag & qf_STEP)
    x = rhoreal_aux(x,D,sqrtD,isqrtD);
  else
    x = redrealform5(x,D,sqrtD,isqrtD);
  return gerepileupto(av, decodeform(x,d0));
}

GEN
comprealform5(GEN x, GEN y, GEN D, GEN sqrtD, GEN isqrtD)
{
  pari_sp av = avma;
  GEN z = cgetg(6,t_VEC); comp_gen(z,x,y);
  if (x == y)
  {
    z[4] = lshifti((GEN)x[4],1);
    z[5] = lsqr((GEN)x[5]);
  }
  else
  {
    z[4] = laddii((GEN)x[4],(GEN)y[4]);
    z[5] = lmulrr((GEN)x[5],(GEN)y[5]);
  }
  fix_expo(z); z = redrealform5(z,D,sqrtD,isqrtD);
  return gerepilecopy(av,z);
}

/* assume n!=0 */
GEN
powrealform(GEN x, GEN n)
{
  pari_sp av = avma;
  long i,m;
  GEN y,D,sqrtD,isqrtD,d0;

  if (typ(x) != t_QFR)
    err(talker,"not a real quadratic form in powreal");
  if (gcmp1(n)) return gcopy(x);
  if (gcmp_1(n)) return ginv(x);

  d0 = (GEN)x[4];
  D = qf_disc(x,NULL,NULL);
  sqrtD = gsqrt(D, get_prec(d0));
  isqrtD = mptrunc(sqrtD);
  if (signe(n) < 0) { x = ginv(x); d0 = (GEN)x[4]; }
  n = absi(n);
  x = codeform5(x, lg(d0)); y = NULL;
  for (i=lgefint(n)-1; i>1; i--)
  {
    m = n[i];
    for (; m; m>>=1)
    {
      if (m&1) y = y? comprealform5(y,x,D,sqrtD,isqrtD): x;
      if (m == 1 && i == 2) break;
      x = comprealform5(x,x,D,sqrtD,isqrtD);
    }
  }
  d0 = mulri(d0,n);
  return gerepileupto(av, decodeform(y,d0));
}

GEN
redimag(GEN x)
{
  pari_sp av=avma;
  long fl;
  do x = rhoimag0(x, &fl); while (fl == 0);
  x = gerepilecopy(av,x);
  if (fl == 2) setsigne(x[2], -signe(x[2]));
  return x;
}

GEN
redreal(GEN x)
{
  return redreal0(x,0,NULL,NULL,NULL);
}

GEN
rhoreal(GEN x)
{
  return redreal0(x,qf_STEP,NULL,NULL,NULL);
}

GEN
redrealnod(GEN x, GEN isqrtD)
{
  return redreal0(x,qf_NOD,NULL,isqrtD,NULL);
}

GEN
rhorealnod(GEN x, GEN isqrtD)
{
  return redreal0(x,qf_STEP|qf_NOD,NULL,isqrtD,NULL);
}

GEN
qfbred0(GEN x, long flag, GEN D, GEN isqrtD, GEN sqrtD)
{
  long tx=typ(x),fl;
  pari_sp av;

  if (!is_qf_t(tx)) err(talker,"not a quadratic form in qfbred");
  if (!D) D = qf_disc(x,NULL,NULL);
  switch(signe(D))
  {
    case 1 :
      return redreal0(x,flag,D,isqrtD,sqrtD);

    case -1:
      if (!(flag & qf_STEP)) return redimag(x);
      av = avma; x = rhoimag0(x,&fl);
      x = gerepilecopy(av,x);
      if (fl == 2) setsigne(x[2], -signe(x[2]));
      return x;
  }
  err(redpoler,"qfbred");
  return NULL; /* not reached */
}

/* special case: p = 1 return unit form */
GEN
primeform(GEN x, GEN p, long prec)
{
  pari_sp av;
  long s, sx = signe(x);
  GEN y, b, c;

  if (typ(x) != t_INT || !sx) err(arither1);
  if (typ(p) != t_INT || signe(p) <= 0) err(arither1);
  if (is_pm1(p))
    return sx<0? imag_unit_form_by_disc(x)
               : real_unit_form_by_disc(x,prec);
  s = mod8(x);
  if (sx < 0)
  {
    if (s) s = 8-s;
    y = cgetg(4, t_QFI);
  }
  else
  {
    y = cgetg(5, t_QFR);
    y[4] = (long)realzero(prec);
  }
  /* 2 or 3 mod 4 */
  if (s & 2) err(talker,"discriminant not congruent to 0,1 mod 4 in primeform");
  av = avma;
  if (egalii(p, gdeux))
  {
    switch(s)
    {
      case 0: b = gzero; break;
      case 1: b = gun;   break;
      case 4: b = gdeux; break;
      default: err(sqrter5); b = NULL; /* -Wall */
    }
    c = shifti(subsi(s,x), -3);
  }
  else
  {
    b = mpsqrtmod(x,p); if (!b) err(sqrter5);
    s &= 1; /* s = x mod 2 */
    /* mod(b) != mod2(x) ? [Warning: we may have b == 0] */
    if ((!signe(b) && s) || mod2(b) != s) b = gerepileuptoint(av, subii(p,b));

    av = avma;
    c = diviiexact(shifti(subii(sqri(b), x), -2), p);
  }
  y[3] = lpileuptoint(av, c);
  y[2] = (long)b;
  y[1] = licopy(p); return y;
}

GEN 
redimagsl2(GEN V)
{
  pari_sp ltop = avma;
  pari_sp st_lim = stack_lim(ltop, 1);
  GEN a,b,c, z, u1,u2,v1,v2;
  long skip = 1;
  GEN p2,p3; 
  a = (GEN) V[1]; b = (GEN) V[2]; c = (GEN) V[3];
  u1 = v2 = gun;
  u2 = v1 = gzero;
  if (cmpii(negi(a), b) < 0 && cmpii(b, a) <= 0) skip = 0;
  for(;;)
  {
    if (skip)
    {
      GEN a2= shifti(a, 1);
      GEN D = divrem(b, a2, -1);
      GEN q = (GEN) D[1];
      GEN r = (GEN) D[2];
      if (cmpii(r, a) > 0)
      {
        q = addis(q, 1);
        r = subii(r, a2);
      }
      c = subii(c, shifti(mulii(q, addii(b, r)), -1));
      b = r;
      u2 = subii(u2, mulii(q, u1));
      v2 = subii(v2, mulii(q, v1));
    }
    skip = 1;
    if (cmpii(a, c) <= 0) break;
    b = negi(b);
    z = a; a = c; c = z;
    z = u1; u1 = u2; u2 = negi(z);
    z = v1; v1 = v2; v2 = negi(z);
    if (low_stack(st_lim, stack_lim(ltop, 1)))
    {
      GEN *bptr[7];
      bptr[0]=&a; bptr[1]=&b; bptr[2]=&c;
      bptr[3]=&u1; bptr[4]=&u2;
      bptr[5]=&v1; bptr[6]=&v2;
      gerepilemany(ltop, bptr, 7);
    }
  }
  if (egalii(a, c) && signe(b) < 0)
  {
    b = negi(b);
    z = u1; u1 = u2; u2 = negi(z);
    z = v1; v1 = v2; v2 = negi(z);
  }
  p2 = cgetg(3, t_VEC);
  p2[1] = lgetg(4,t_QFI);
  mael(p2, 1, 1) = licopy(a);
  mael(p2, 1, 2) = licopy(b);
  mael(p2, 1, 3) = licopy(c);
  p3 = cgetg(3, t_MAT);
  p3[1] = lgetg(3, t_COL);
  coeff(p3, 1, 1) = licopy(u1);
  coeff(p3, 2, 1) = licopy(v1);
  p3[2] = lgetg(3, t_COL);
  coeff(p3, 1, 2) = licopy(u2);
  coeff(p3, 2, 2) = licopy(v2);
  p2[2] = (long) p3;
  return gerepileupto(ltop, p2);
}

GEN
qfbimagsolvep(GEN Q,GEN p)
{
  pari_sp ltop = avma;
  GEN M, res, N, d = qf_disc(Q, NULL, NULL);
  if (kronecker(d,p) < 0) return gzero;
  N = redimagsl2(Q);
  M = redimagsl2( primeform(d, p, 0) );
  if (!gegal((GEN)M[1], (GEN)N[1])) return gzero;
  res = (GEN)gdiv((GEN)N[2], (GEN)M[2])[1];
  return gerepilecopy(ltop,res);
}

GEN
qfbsolve(GEN Q,GEN n)
{
  if (typ(Q)!=t_QFI || typ(n)!=t_INT) err(typeer,"qfbsolve");
  return qfbimagsolvep(Q,n);
}
