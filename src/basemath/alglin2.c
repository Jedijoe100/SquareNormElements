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
/**                         LINEAR ALGEBRA                         **/
/**                         (second part)                          **/
/**                                                                **/
/********************************************************************/
#include "pari.h"
extern GEN quicktrace(GEN x, GEN sym);
extern GEN imagecomplspec(GEN x, long *nlze);
extern void ZV_neg_ip(GEN M);

/*******************************************************************/
/*                                                                 */
/*                   CHARACTERISTIC POLYNOMIAL                     */
/*                                                                 */
/*******************************************************************/

GEN
charpoly0(GEN x, int v, long flag)
{
  if (v<0) v = 0;
  switch(flag)
  {
    case 0: return caradj(x,v,0);
    case 1: return caract(x,v);
    case 2: return carhess(x,v);
  }
  err(flagerr,"charpoly"); return NULL; /* not reached */
}

static GEN
caract2_i(GEN p, GEN x, int v, GEN (subres_f)(GEN,GEN,GEN*))
{
  pari_sp av = avma;
  long d;
  GEN ch, L = leading_term(p);

  if (!signe(x)) return gpowgs(polx[v], degpol(p));
  if (typ(x) != t_POL)
    return gerepileupto(av, gpowgs(gsub(polx[v], x), degpol(p)));
  x = gneg_i(x);
  if (varn(x) == MAXVARN) { setvarn(x, 0); p = dummycopy(p); setvarn(p, 0); }
  x[2] = ladd((GEN)x[2], polx[MAXVARN]);
  ch = subres_f(p, x, NULL);
  if (v != MAXVARN)
  {
    if (typ(ch) == t_POL && varn(ch) == MAXVARN)
      setvarn(ch, v);
    else
      ch = gsubst(ch, MAXVARN, polx[v]);
  }

  if (!gcmp1(L) && (d = degpol(x)) > 0) ch = gdiv(ch, gpowgs(L,d));
  return gerepileupto(av, ch);
}

/* return caract(Mod(x,p)) in variable v */
GEN
caract2(GEN p, GEN x, int v)
{
  return caract2_i(p,x,v, subresall);
}
GEN
caractducos(GEN p, GEN x, int v)
{
  return caract2_i(p,x,v, (GEN (*)(GEN,GEN,GEN*))resultantducos);
}

/* characteristic pol. Easy cases. Return NULL in case it's not so easy. */
static GEN
easychar(GEN x, long v, GEN *py)
{
  pari_sp av;
  long lx;
  GEN p1,p2;

  switch(typ(x))
  {
    case t_INT: case t_REAL: case t_INTMOD:
    case t_FRAC: case t_FRACN: case t_PADIC:
      p1=cgetg(4,t_POL);
      p1[1]=evalsigne(1) | evalvarn(v);
      p1[2]=lneg(x); p1[3]=un;
      if (py)
      {
	p2=cgetg(2,t_MAT);
	p2[1]=lgetg(2,t_COL);
	coeff(p2,1,1)=lcopy(x);
	*py=p2;
      }
      return p1;

    case t_COMPLEX: case t_QUAD:
      if (py) err(typeer,"easychar");
      p1 = cgetg(5,t_POL);
      p1[1] = evalsigne(1) | evalvarn(v);
      p1[2] = lnorm(x); av = avma;
      p1[3] = lpileupto(av, gneg(gtrace(x)));
      p1[4] = un; return p1;

    case t_POLMOD:
      if (py) err(typeer,"easychar");
      return caract2((GEN)x[1], (GEN)x[2], v);

    case t_MAT:
      lx=lg(x);
      if (lx==1)
      {
	if (py) *py = cgetg(1,t_MAT);
	return polun[v];
      }
      if (lg(x[1]) != lx) break;
      return NULL;
  }
  err(mattype1,"easychar");
  return NULL; /* not reached */
}

GEN
caract(GEN x, int v)
{
  long k, n;
  pari_sp av=avma;
  GEN  p1,p2,p3,p4,x_k;

  if ((p1 = easychar(x,v,NULL))) return p1;

  p1 = gzero; p2 = gun;
  n = lg(x)-1; if (n&1) p2 = negi(p2);
  x_k = dummycopy(polx[v]);
  p4 = cgetg(3,t_RFRACN); p4[2] = (long)x_k;
  for (k=0; k<=n; k++)
  {
    p3 = det(gsub(gscalmat(stoi(k),n), x));
    p4[1] = lmul(p3,p2); x_k[2] = lstoi(-k);
    p1 = gadd(p4,p1);
    if (k == n) break;

    p2 = gdivgs(gmulsg(k-n,p2),k+1);
  }
  return gerepileupto(av, gdiv((GEN)p1[1], mpfact(n)));
}

GEN
caradj0(GEN x, long v)
{
  return caradj(x,v,NULL);
}

/* assume x square matrice */
static GEN
mattrace(GEN x)
{
  pari_sp av;
  long i, lx = lg(x);
  GEN t;
  if (lx < 3) return lx == 1? gzero: gcopy(gcoeff(x,1,1));
  av = avma; t = gcoeff(x,1,1);
  for (i = 2; i < lx; i++) t = gadd(t, gcoeff(x,i,i));
  return gerepileupto(av, t);
}

/* Using traces: return the characteristic polynomial of x (in variable v).
 * If py != NULL, the adjoint matrix is put there. */
GEN
caradj(GEN x, long v, GEN *py)
{
  pari_sp av, av0;
  long i, k, l;
  GEN p, y, t;

  if ((p = easychar(x, v, py))) return p;

  l = lg(x); av0 = avma;
  p = cgetg(l+2,t_POL); p[1] = evalsigne(1) | evalvarn(v);
  p[l+1] = un;
  if (l == 1) { if (py) *py = cgetg(1,t_MAT); return p; }
  av = avma; t = gerepileupto(av, gneg(mattrace(x)));
  p[l] = (long)t;
  if (l == 2) { if (py) *py = idmat(1); return p; }
  if (l == 3) {
    GEN a = gcoeff(x,1,1), b = gcoeff(x,1,2);
    GEN c = gcoeff(x,2,1), d = gcoeff(x,2,2);
    if (py) {
      y = cgetg(3, t_MAT);
      y[1] = (long)coefs_to_col(2, gcopy(d), gneg(c));
      y[2] = (long)coefs_to_col(2, gneg(b), gcopy(a));
      *py = y;
    }
    av = avma;
    p[2] = lpileupto(av, gadd(gmul(a,d), gneg(gmul(b,c))));
    return p;
  }
  av = avma; y = dummycopy(x);
  for (i = 1; i < l; i++) coeff(y,i,i) = ladd(gcoeff(y,i,i), t);
  for (k = 2; k < l-1; k++)
  {
    GEN y0 = y;
    y = gmul(y, x);
    t = gdivgs(mattrace(y), -k);
    for (i = 1; i < l; i++) coeff(y,i,i) = ladd(gcoeff(y,i,i), t);
    y = gclone(y);
    /* beware: since y is a clone and t computed from it some components
     * may be out of stack (eg. INTMOD/POLMOD) */
    p[l-k+1] = lpileupto(av, forcecopy(t)); av = avma;
    if (k > 2) gunclone(y0);
  }
  t = gzero;
  for (i=1; i<l; i++) t = gadd(t, gmul(gcoeff(x,1,i),gcoeff(y,i,1)));
  p[2] = lpileupto(av, forcecopy(gneg(t)));
  i = gvar2(p);
  if (i == v) err(talker,"incorrect variable in caradj");
  if (i < v) p = gerepileupto(av0, poleval(p, polx[v]));
  if (py) *py = (l & 1)? stackify(gneg(y)): forcecopy(y);
  gunclone(y); return p;
}

GEN
adj(GEN x)
{
  GEN y;
  (void)caradj(x,MAXVARN,&y); return y;
}

/*******************************************************************/
/*                                                                 */
/*                       HESSENBERG FORM                           */
/*                                                                 */
/*******************************************************************/
#define lswap(x,y) { long _t=x; x=y; y=_t; }
#define swap(x,y) { GEN _t=x; x=y; y=_t; }

GEN
hess(GEN x)
{
  pari_sp av = avma;
  long lx=lg(x),m,i,j;
  GEN p,p1,p2;

  if (typ(x) != t_MAT) err(mattype1,"hess");
  if (lx==1) return cgetg(1,t_MAT);
  if (lg(x[1])!=lx) err(mattype1,"hess");
  x = dummycopy(x);

  for (m=2; m<lx-1; m++)
    for (i=m+1; i<lx; i++)
    {
      p = gcoeff(x,i,m-1);
      if (gcmp0(p)) continue;

      for (j=m-1; j<lx; j++) lswap(coeff(x,i,j), coeff(x,m,j));
      lswap(x[i], x[m]); p = ginv(p);
      for (i=m+1; i<lx; i++)
      {
        p1 = gcoeff(x,i,m-1);
        if (gcmp0(p1)) continue;

        p1 = gmul(p1,p); p2 = gneg_i(p1);
        coeff(x,i,m-1) = zero;
        for (j=m; j<lx; j++)
          coeff(x,i,j) = ladd(gcoeff(x,i,j), gmul(p2,gcoeff(x,m,j)));
        for (j=1; j<lx; j++)
          coeff(x,j,m) = ladd(gcoeff(x,j,m), gmul(p1,gcoeff(x,j,i)));
      }
      break;
    }
  return gerepilecopy(av,x);
}

GEN
carhess(GEN x, long v)
{
  pari_sp av,tetpil;
  long lx,r,i;
  GEN *y,p1,p2,p3,p4;

  if ((p1 = easychar(x,v,NULL))) return p1;

  lx=lg(x); av=avma; y = (GEN*) new_chunk(lx);
  y[0]=polun[v]; p1=hess(x); p2=polx[v];
  tetpil=avma;
  for (r=1; r<lx; r++)
  {
    y[r]=gmul(y[r-1], gsub(p2,gcoeff(p1,r,r)));
    p3=gun; p4=gzero;
    for (i=r-1; i; i--)
    {
      p3=gmul(p3,gcoeff(p1,i+1,i));
      p4=gadd(p4,gmul(gmul(p3,gcoeff(p1,i,r)),y[i-1]));
    }
    tetpil=avma; y[r]=gsub(y[r],p4);
  }
  return gerepile(av,tetpil,y[lx-1]);
}

/*******************************************************************/
/*                                                                 */
/*                            NORM                                 */
/*                                                                 */
/*******************************************************************/

GEN
gnorm(GEN x)
{
  pari_sp av;
  long lx,i, tx=typ(x);
  GEN p1,p2,y;

  switch(tx)
  {
    case t_INT:
      return sqri(x);

    case t_REAL:
      return mulrr(x,x);

    case t_FRAC: case t_FRACN:
      return gsqr(x);

    case t_COMPLEX: av = avma;
      return gerepileupto(av, gadd(gsqr((GEN)x[1]), gsqr((GEN)x[2])));

    case t_QUAD: av = avma;
      p1 = (GEN)x[1];
      p2 = gmul((GEN)p1[2], gsqr((GEN)x[3]));
      p1 = gcmp0((GEN)p1[3])? gsqr((GEN)x[2])
                            : gmul((GEN)x[2], gadd((GEN)x[2],(GEN)x[3]));
      return gerepileupto(av, gadd(p1,p2));

    case t_POL: case t_SER: case t_RFRAC: case t_RFRACN: av = avma;
      return gerepileupto(av, greal(gmul(gconj(x),x)));

    case t_POLMOD:
    {
      GEN T = (GEN)x[1], A = (GEN)x[2];
      if (typ(A) != t_POL) return gpowgs(A, degpol(T));

      y = subres(T, A); p1 = leading_term(T);
      if (gcmp1(p1) || gcmp0(A)) return y;
      av = avma; T = gpowgs(p1, degpol(A));
      return gerepileupto(av, gdiv(y,T));
    }

    case t_VEC: case t_COL: case t_MAT:
      lx=lg(x); y=cgetg(lx,tx);
      for (i=1; i<lx; i++) y[i]=lnorm((GEN) x[i]);
      return y;
  }
  err(typeer,"gnorm");
  return NULL; /* not reached */
}

GEN
gnorml2(GEN x)
{
  pari_sp av,lim;
  GEN y;
  long i,tx=typ(x),lx;

  if (! is_matvec_t(tx)) return gnorm(x);
  lx=lg(x); if (lx==1) return gzero;

  av=avma; lim = stack_lim(av,1); y = gnorml2((GEN) x[1]);
  for (i=2; i<lx; i++)
  {
    y = gadd(y, gnorml2((GEN) x[i]));
    if (low_stack(lim, stack_lim(av,1)))
    {
      if (DEBUGMEM>1) err(warnmem,"gnorml2");
      y = gerepileupto(av, y);
    }
  }
  return gerepileupto(av,y);
}

GEN
QuickNormL2(GEN x, long prec)
{
  pari_sp av = avma;
  GEN y = gmul(x, realun(prec));
  if (typ(x) == t_POL) *++y = evaltyp(t_VEC) | evallg(lg(x)-1);
  return gerepileupto(av, gnorml2(y));
}

GEN
gnorml1(GEN x,long prec)
{
  pari_sp av = avma;
  long lx,i;
  GEN s;
  switch(typ(x))
  {
    case t_INT: case t_REAL: case t_COMPLEX: case t_FRAC:
    case t_FRACN: case t_QUAD:
      return gabs(x,prec);

    case t_POL:
      lx = lg(x); s = gzero;
      for (i=2; i<lx; i++) s = gadd(s, gabs((GEN)x[i],prec));
      break;

    case t_VEC: case t_COL: case t_MAT:
      lx = lg(x); s = gzero;
      for (i=1; i<lx; i++) s = gadd(s, gabs((GEN)x[i],prec));
      break;

    default: err(typeer,"gnorml1");
      return NULL; /* not reached */
  }
  return gerepileupto(av, s);
}

GEN
QuickNormL1(GEN x,long prec)
{
  pari_sp av = avma;
  long lx,i;
  GEN p1,p2,s;
  switch(typ(x))
  {
    case t_INT: case t_REAL: case t_FRAC: case t_FRACN:
      return gabs(x,prec);

    case t_INTMOD: case t_PADIC: case t_POLMOD:
    case t_SER: case t_RFRAC: case t_RFRACN:
      return gcopy(x);

    case t_COMPLEX:
      p1=gabs((GEN)x[1],prec); p2=gabs((GEN)x[2],prec);
      return gerepileupto(av, gadd(p1,p2));

    case t_QUAD:
      p1=gabs((GEN)x[2],prec); p2=gabs((GEN)x[3],prec);
      return gerepileupto(av, gadd(p1,p2));

    case t_POL:
      lx=lg(x); s=gzero;
      for (i=2; i<lx; i++) s=gadd(s,QuickNormL1((GEN)x[i],prec));
      return gerepileupto(av, s);

    case t_VEC: case t_COL: case t_MAT:
      lx=lg(x); s=gzero;
      for (i=1; i<lx; i++) s=gadd(s,QuickNormL1((GEN)x[i],prec));
      return gerepileupto(av, s);
  }
  err(typeer,"QuickNormL1");
  return NULL; /* not reached */
}

/*******************************************************************/
/*                                                                 */
/*                          CONJUGATION                            */
/*                                                                 */
/*******************************************************************/

GEN
gconj(GEN x)
{
  long lx, i, tx = typ(x);
  GEN z;

  switch(tx)
  {
    case t_INT: case t_REAL: case t_INTMOD:
    case t_FRAC: case t_FRACN: case t_PADIC:
      return gcopy(x);

    case t_COMPLEX:
      z = cgetg(3,t_COMPLEX);
      z[1] = lcopy((GEN)x[1]);
      z[2] = lneg((GEN)x[2]);
      break;

    case t_QUAD:
      z = cgetg(4,t_QUAD); copyifstack(x[1],z[1]);
      z[2] = gcmp0(gmael(x,1,3))? lcopy((GEN)x[2])
                                : ladd((GEN)x[2], (GEN)x[3]);
      z[3] = lneg((GEN) x[3]);
      break;

    case t_POL: case t_SER:
      lx = lg(x); z = cgetg(lx,tx); z[1] = x[1];
      for (i=2; i<lx; i++) z[i] = lconj((GEN)x[i]);
      break;

    case t_RFRAC: case t_RFRACN: case t_VEC: case t_COL: case t_MAT:
      lx = lg(x); z = cgetg(lx,tx);
      for (i=1; i<lx; i++) z[i] = lconj((GEN)x[i]);
      break;

    case t_POLMOD:
    {
      long d = degpol(x[1]);
      if (d < 2) return gcopy(x);
      if (d == 2)
      {
        pari_sp av = avma;
        return gerepileupto(av, gadd(gtrace(x), gneg(x)));
      }
    }
    default:
      err(typeer,"gconj");
      return NULL; /* not reached */
  }
  return z;
}

GEN
conjvec(GEN x,long prec)
{
  pari_sp av,tetpil;
  long lx,s,i,tx=typ(x);
  GEN z,y,p1,p2,p;

  switch(tx)
  {
    case t_INT: case t_INTMOD: case t_FRAC: case t_FRACN:
      z=cgetg(2,t_COL); z[1]=lcopy(x); break;

    case t_COMPLEX: case t_QUAD:
      z=cgetg(3,t_COL); z[1]=lcopy(x); z[2]=lconj(x); break;

    case t_VEC: case t_COL:
      lx=lg(x); z=cgetg(lx,t_MAT);
      for (i=1; i<lx; i++) z[i]=(long)conjvec((GEN)x[i],prec);
      if (lx == 1) break;
      s = lg(z[1]);
      for (i=2; i<lx; i++)
        if (lg(z[i])!=s) err(talker,"incompatible field degrees in conjvec");
      break;

    case t_POLMOD:
      y=(GEN)x[1]; lx=lg(y);
      if (lx<=3) return cgetg(1,t_COL);
      av=avma; p=NULL;
      for (i=2; i<lx; i++)
      {
	tx=typ(y[i]);
	if (tx==t_INTMOD) p=gmael(y,i,1);
	else
	  if (tx != t_INT && ! is_frac_t(tx))
	    err(polrationer,"conjvec");
      }
      if (!p)
      {
	p1=roots(y,prec); x = (GEN)x[2]; tetpil=avma;
	z=cgetg(lx-2,t_COL);
	for (i=1; i<=lx-3; i++)
	{
	  p2=(GEN)p1[i]; if (gcmp0((GEN)p2[2])) p2 = (GEN)p2[1];
	  z[i] = (long)poleval(x, p2);
	 }
	return gerepile(av,tetpil,z);
      }
      z=cgetg(lx-2,t_COL); z[1]=lcopy(x);
      for (i=2; i<=lx-3; i++) z[i] = lpui((GEN) z[i-1],p,prec);
      break;

    default:
      err(typeer,"conjvec");
      return NULL; /* not reached */
  }
  return z;
}

/*******************************************************************/
/*                                                                 */
/*                            TRACES                               */
/*                                                                 */
/*******************************************************************/

GEN
assmat(GEN x)
{
  long lx,i,j;
  GEN y,p1,p2;

  if (typ(x)!=t_POL) err(notpoler,"assmat");
  if (gcmp0(x)) err(zeropoler,"assmat");

  lx=lg(x)-2; y=cgetg(lx,t_MAT);
  for (i=1; i<lx-1; i++)
  {
    p1=cgetg(lx,t_COL); y[i]=(long)p1;
    for (j=1; j<lx; j++)
      p1[j] = (j==(i+1))? un: zero;
  }
  p1=cgetg(lx,t_COL); y[i]=(long)p1;
  if (gcmp1((GEN)x[lx+1]))
    for (j=1; j<lx; j++)
      p1[j] = lneg((GEN)x[j+1]);
  else
  {
    pari_sp av = avma;
    p2 = gclone(gneg((GEN)x[lx+1]));
    avma = av;
    for (j=1; j<lx; j++)
      p1[j] = ldiv((GEN)x[j+1],p2);
    gunclone(p2);
  }
  return y;
}

GEN
gtrace(GEN x)
{
  pari_sp av;
  long i, lx, tx = typ(x);
  GEN y;

  switch(tx)
  {
    case t_INT: case t_REAL: case t_FRAC: case t_FRACN:
      return gmul2n(x,1);

    case t_COMPLEX:
      return gmul2n((GEN)x[1],1);

    case t_QUAD:
      y = (GEN)x[1];
      if (!gcmp0((GEN)y[3]))
      { /* assume quad. polynomial is either x^2 + d or x^2 - x + d */
	av = avma;
	return gerepileupto(av, gadd((GEN)x[3], gmul2n((GEN)x[2],1)));
      }
      return gmul2n((GEN)x[2],1);

    case t_POL: case t_SER:
      lx = lg(x); y = cgetg(lx,tx); y[1] = x[1];
      for (i=2; i<lx; i++) y[i] = ltrace((GEN)x[i]);
      return y;

    case t_POLMOD:
      av = avma; y = (GEN)x[1];
      return gerepileupto(av, quicktrace((GEN)x[2], polsym(y, degpol(y)-1)));

    case t_RFRAC: case t_RFRACN:
      return gadd(x, gconj(x));

    case t_VEC: case t_COL:
      lx = lg(x); y = cgetg(lx,tx);
      for (i=1; i<lx; i++) y[i] = ltrace((GEN)x[i]);
      return y;

    case t_MAT:
      lx = lg(x); if (lx == 1) return gzero;
      /*now lx >= 2*/
      if (lx != lg(x[1])) err(mattype1,"gtrace");
      return mattrace(x);
  }
  err(typeer,"gtrace");
  return NULL; /* not reached */
}

/* Cholesky Decomposition for positive definite matrix a [matrix Q, Algo 2.7.6]
 * If a is not positive definite:
 *   if flag is zero: raise an error
 *   else: return NULL. */
GEN
sqred1intern(GEN a)
{
  pari_sp av = avma, lim=stack_lim(av,1);
  GEN b,p;
  long i,j,k, n = lg(a);

  if (typ(a)!=t_MAT) err(typeer,"sqred1");
  if (n == 1) return cgetg(1, t_MAT);
  if (lg(a[1])!=n) err(mattype1,"sqred1");
  b=cgetg(n,t_MAT);
  for (j=1; j<n; j++)
  {
    GEN p1=cgetg(n,t_COL), p2=(GEN)a[j];

    b[j]=(long)p1;
    for (i=1; i<=j; i++) p1[i] = p2[i];
    for (   ; i<n ; i++) p1[i] = zero;
  }
  for (k=1; k<n; k++)
  {
    p = gcoeff(b,k,k);
    if (gsigne(p)<=0) { avma = av; return NULL; } /* not positive definite */
    p = ginv(p);
    for (i=k+1; i<n; i++)
      for (j=i; j<n; j++)
	coeff(b,i,j)=lsub(gcoeff(b,i,j),
	                  gmul(gmul(gcoeff(b,k,i),gcoeff(b,k,j)), p));
    for (j=k+1; j<n; j++)
      coeff(b,k,j)=lmul(gcoeff(b,k,j), p);
    if (low_stack(lim, stack_lim(av,1)))
    {
      if (DEBUGMEM>1) err(warnmem,"sqred1");
      b=gerepilecopy(av,b);
    }
  }
  return gerepilecopy(av,b);
}

GEN
sqred1(GEN a)
{
  GEN x = sqred1intern(a);
  if (!x) err(talker,"not a positive definite matrix in sqred1");
  return x;
}

GEN
sqred3(GEN a)
{
  pari_sp av = avma, lim = stack_lim(av,1);
  long i,j,k,l, n = lg(a);
  GEN p1,b;

  if (typ(a)!=t_MAT) err(typeer,"sqred3");
  if (lg(a[1])!=n) err(mattype1,"sqred3");
  av=avma; b=cgetg(n,t_MAT);
  for (j=1; j<n; j++)
  {
    p1=cgetg(n,t_COL); b[j]=(long)p1;
    for (i=1; i<n; i++) p1[i]=zero;
  }
  for (i=1; i<n; i++)
  {
    for (k=1; k<i; k++)
    {
      p1=gzero;
      for (l=1; l<k; l++)
	p1=gadd(p1, gmul(gmul(gcoeff(b,l,l),gcoeff(b,k,l)), gcoeff(b,i,l)));
      coeff(b,i,k)=ldiv(gsub(gcoeff(a,i,k),p1),gcoeff(b,k,k));
    }
    p1=gzero;
    for (l=1; l<i; l++)
      p1=gadd(p1, gmul(gcoeff(b,l,l), gsqr(gcoeff(b,i,l))));
    coeff(b,i,k)=lsub(gcoeff(a,i,i),p1);
    if (low_stack(lim, stack_lim(av,1)))
    {
      if (DEBUGMEM>1) err(warnmem,"sqred3");
      b=gerepilecopy(av,b);
    }
  }
  return gerepilecopy(av,b);
}

/* Gauss reduction (arbitrary symetric matrix, only the part above the
 * diagonal is considered). If no_signature is zero, return only the
 * signature
 */
static GEN
sqred2(GEN a, long no_signature)
{
  GEN r,p,mun;
  pari_sp av,av1,lim;
  long n,i,j,k,l,sp,sn,t;

  if (typ(a) != t_MAT) err(typeer,"sqred2");
  n = lg(a); if (n > 1 && lg(a[1]) != n) err(mattype1,"sqred2");
  n--;

  av = avma; mun = negi(gun);
  r = vecsmall_const(n, 1);
  av1= avma; lim = stack_lim(av1,1);
  a = dummycopy(a);
  t = n; sp = sn = 0;
  while (t)
  {
    k=1; while (k<=n && (!r[k] || gcmp0(gcoeff(a,k,k)))) k++;
    if (k<=n)
    {
      p = gcoeff(a,k,k); if (gsigne(p) > 0) sp++; else sn++;
      r[k] = 0; t--;
      for (j=1; j<=n; j++)
	coeff(a,k,j) = r[j] ? ldiv(gcoeff(a,k,j),p) : zero;
	
      for (i=1; i<=n; i++) if (r[i])
	for (j=1; j<=n; j++)
	  coeff(a,i,j) = r[j] ? lsub(gcoeff(a,i,j),
	                             gmul(gmul(gcoeff(a,k,i),gcoeff(a,k,j)),p))
			      : zero;
      coeff(a,k,k) = (long)p;
    }
    else
    {
      for (k=1; k<=n; k++) if (r[k])
      {
	l=k+1; while (l<=n && (!r[l] || gcmp0(gcoeff(a,k,l)))) l++;
	if (l <= n)
	{
	  p = gcoeff(a,k,l); r[k] = r[l] = 0;
          sp++; sn++; t -= 2;
	  for (i=1; i<=n; i++) if (r[i])
	    for (j=1; j<=n; j++)
	      coeff(a,i,j) =
		r[j]? lsub(gcoeff(a,i,j),
			   gdiv(gadd(gmul(gcoeff(a,k,i),gcoeff(a,l,j)),
				     gmul(gcoeff(a,k,j),gcoeff(a,l,i))),
				p))
		    : zero;
	  for (i=1; i<=n; i++) if (r[i])
          {
            GEN u = gcoeff(a,k,i);
	    coeff(a,k,i) = ldiv(gadd(u, gcoeff(a,l,i)), p);
	    coeff(a,l,i) = ldiv(gsub(u, gcoeff(a,l,i)), p);
          }
	  coeff(a,k,l) = un;
          coeff(a,l,k) = (long)mun;
	  coeff(a,k,k) = lmul2n(p,-1);
          coeff(a,l,l) = lneg(gcoeff(a,k,k));
	  if (low_stack(lim, stack_lim(av1,1)))
	  {
	    if(DEBUGMEM>1) err(warnmem,"sqred2");
	    a = gerepilecopy(av1, a);
	  }
	  break;
	}
      }
      if (k > n) break;
    }
  }
  if (no_signature) return gerepilecopy(av, a);
  avma = av; a = cgetg(3,t_VEC);
  a[1] = lstoi(sp);
  a[2] = lstoi(sn); return a;
}

GEN
sqred(GEN a) { return sqred2(a,1); }

GEN
signat(GEN a) { return sqred2(a,0); }

/* Diagonalization of a REAL symetric matrix. Return a vector [L, r]:
 * L = vector of eigenvalues
 * r = matrix of eigenvectors */
GEN
jacobi(GEN a, long prec)
{
  pari_sp av1;
  long de, e, e1, e2, i, j, p, q, l = lg(a);
  GEN c, s, t, u, ja, L, r, unr, x, y, x1, y1;

  if (typ(a) != t_MAT) err(mattype1,"jacobi");
  ja = cgetg(3,t_VEC);
  L = cgetg(l,t_COL); ja[1] = (long)L;
  r = cgetg(l,t_MAT); ja[2] = (long)r;
  if (l == 1) return ja;
  if (lg(a[1]) != l) err(mattype1,"jacobi");

  e1 = HIGHEXPOBIT-1;
  for (j=1; j<l; j++)
  {
    gaffect(gcoeff(a,j,j), (GEN)(L[j] = lgetr(prec)));
    e = expo(L[j]); if (e < e1) e1 = e;
  }
  for (j=1; j<l; j++)
  {
    r[j] = lgetg(l,t_COL);
    for (i=1; i<l; i++) coeff(r,i,j) = (long)stor(i==j, prec);
  }
  av1 = avma;

  e2 = -(long)HIGHEXPOBIT; p = q = 1;
  c = cgetg(l,t_MAT);
  for (j=1; j<l; j++)
  {
    c[j] = lgetg(j,t_COL);
    for (i=1; i<j; i++)
    {
      gaffect(gcoeff(a,i,j), (GEN)(coeff(c,i,j) = lgetr(prec)));
      e = expo(gcoeff(c,i,j)); if (e > e2) { e2 = e; p = i; q = j; }
    }
  }
  a = c; unr = realun(prec);
  de = bit_accuracy(prec);

 /* e1 = min expo(a[i,i])
  * e2 = max expo(a[i,j]), i != j */
  while (e1-e2 < de)
  {
    pari_sp av2 = avma;
    /* compute associated rotation in the plane formed by basis vectors number
     * p and q */
    x = divrr(subrr((GEN)L[q],(GEN)L[p]), shiftr(gcoeff(a,p,q),1));
    y = mpsqrt(addrr(unr, mulrr(x,x)));
    t = divrr(unr, (signe(x)>0)? addrr(x,y): subrr(x,y));
    c = divrr(unr, mpsqrt(addrr(unr,mulrr(t,t))));
    s = mulrr(t,c);
    u = divrr(s,addrr(unr,c));

    /* compute successive transforms of a and the matrix of accumulated
     * rotations (r) */
    for (i=1; i<p; i++)
    {
      x = gcoeff(a,i,p); y = gcoeff(a,i,q);
      x1 = subrr(x,mulrr(s,addrr(y,mulrr(u,x))));
      y1 = addrr(y,mulrr(s,subrr(x,mulrr(u,y))));
      affrr(x1,x); affrr(y1,y);
    }
    for (i=p+1; i<q; i++)
    {
      x = gcoeff(a,p,i); y = gcoeff(a,i,q);
      x1 = subrr(x,mulrr(s,addrr(y,mulrr(u,x))));
      y1 = addrr(y,mulrr(s,subrr(x,mulrr(u,y))));
      affrr(x1,x); affrr(y1,y);
    }
    for (i=q+1; i<l; i++)
    {
      x = gcoeff(a,p,i); y = gcoeff(a,q,i);
      x1 = subrr(x,mulrr(s,addrr(y,mulrr(u,x))));
      y1 = addrr(y,mulrr(s,subrr(x,mulrr(u,y))));
      affrr(x1,x); affrr(y1,y);
    }
    y = gcoeff(a,p,q);
    t = mulrr(t, y); setexpo(y, expo(y)-de-1);
    x = (GEN)L[p]; subrrz(x,t, x);
    y = (GEN)L[q]; addrrz(y,t, y);

    for (i=1; i<l; i++)
    {
      x = gcoeff(r,i,p); y = gcoeff(r,i,q);
      x1 = subrr(x,mulrr(s,addrr(y,mulrr(u,x))));
      y1 = addrr(y,mulrr(s,subrr(x,mulrr(u,y))));
      affrr(x1,x); affrr(y1,y);
    }

    e2 = expo(gcoeff(a,1,2)); p = 1; q = 2;
    for (j=1; j<l; j++)
    {
      for (i=1; i<j; i++)
	if ((e=expo(gcoeff(a,i,j))) > e2) { e2=e; p=i; q=j; }
      for (i=j+1; i<l; i++)
	if ((e=expo(gcoeff(a,j,i))) > e2) { e2=e; p=j; q=i; }
    }
    avma = av2;
  }
  avma = av1; return ja;
}

/*************************************************************************/
/**									**/
/**		    MATRICE RATIONNELLE --> ENTIERE			**/
/**									**/
/*************************************************************************/

GEN
matrixqz0(GEN x,GEN p)
{
  if (typ(p)!=t_INT) err(typeer,"matrixqz0");
  if (signe(p)>=0)  return matrixqz(x,p);
  if (!cmpsi(-1,p)) return matrixqz2(x);
  if (!cmpsi(-2,p)) return matrixqz3(x);
  err(flagerr,"matrixqz"); return NULL; /* not reached */
}

static int
ZV_isin(GEN x)
{
  long i,l = lg(x);
  for (i=1; i<l; i++)
    if (typ(x[i]) != t_INT) return 0;
  return 1;
}

GEN
matrixqz(GEN x, GEN p)
{
  pari_sp av = avma, av1, lim;
  long i,j,j1,m,n,nfact;
  GEN p1,p2,p3;

  if (typ(x) != t_MAT) err(typeer,"matrixqz");
  n = lg(x)-1; if (!n) return gcopy(x);
  m = lg(x[1])-1;
  if (n > m) err(talker,"more rows than columns in matrixqz");
  if (n==m)
  {
    p1 = det(x);
    if (gcmp0(p1)) err(talker,"matrix of non-maximal rank in matrixqz");
    avma = av; return idmat(n);
  }
  /* m > n */
  p1 = x; x = cgetg(n+1,t_MAT);
  for (j=1; j<=n; j++)
  {
    x[j] = (long)primpart((GEN)p1[j]);
    if (!ZV_isin((GEN)p1[j])) err(talker, "not a rational matrix in matrixqz");
  }
  /* x integral */

  if (gcmp0(p))
  {
    p1 = gtrans(x); setlg(p1,n+1);
    p2 = det(p1); p1[n] = p1[n+1]; p2 = ggcd(p2,det(p1));
    if (!signe(p2))
      err(impl,"matrixqz when the first 2 dets are zero");
    if (gcmp1(p2)) return gerepilecopy(av,x);

    p1 = (GEN)factor(p2)[1];
  }
  else p1 = _vec(p);
  nfact = lg(p1)-1;
  av1 = avma; lim = stack_lim(av1,1);
  for (i=1; i<=nfact; i++)
  {
    p = (GEN)p1[i];
    for(;;)
    {
      p2 = FpM_ker(x, p);
      if (lg(p2)==1) break;

      p2 = centermod(p2,p); p3 = gdiv(gmul(x,p2), p);
      for (j=1; j<lg(p2); j++)
      {
	j1=n; while (gcmp0(gcoeff(p2,j1,j))) j1--;
	x[j1] = p3[j];
      }
      if (low_stack(lim, stack_lim(av1,1)))
      {
	if (DEBUGMEM>1) err(warnmem,"matrixqz");
	x = gerepilecopy(av1,x);
      }
    }
  }
  return gerepilecopy(av,x);
}

static GEN
Z_V_mul(GEN u, GEN A)
{
  if (gcmp1(u)) return A;
  if (gcmp_1(u)) return gneg(A);
  if (gcmp0(u)) return zerocol(lg(A)-1);
  return gmul(u,A);
}

static GEN
QV_lincomb(GEN u, GEN v, GEN A, GEN B)
{
  if (!signe(u)) return Z_V_mul(v,B);
  if (!signe(v)) return Z_V_mul(u,A);
  return gadd(Z_V_mul(u,A), Z_V_mul(v,B));
}

/* cf ZV_elem */
/* zero aj = Aij (!= 0)  using  ak = Aik (maybe 0), via linear combination of
 * A[j] and A[k] of determinant 1. */
static void
QV_elem(GEN aj, GEN ak, GEN A, long j, long k)
{
  GEN p1,u,v,d, D;

  if (gcmp0(ak)) { lswap(A[j],A[k]); return; }
  D = mpppcm(denom(aj), denom(ak));
  if (!is_pm1(D)) { aj = gmul(aj,D); ak = gmul(ak,D); }
  d = bezout(aj,ak,&u,&v);
  /* frequent special case (u,v) = (1,0) or (0,1) */
  if (!signe(u))
  { /* ak | aj */
    p1 = negi(divii(aj,ak));
    A[j]   = (long)QV_lincomb(gun, p1, (GEN)A[j], (GEN)A[k]);
    return;
  }
  if (!signe(v))
  { /* aj | ak */
    p1 = negi(divii(ak,aj));
    A[k]   = (long)QV_lincomb(gun, p1, (GEN)A[k], (GEN)A[j]);
    lswap(A[j], A[k]);
    return;
  }

  if (!is_pm1(d)) { aj = divii(aj,d); ak = divii(ak,d); }
  p1 = (GEN)A[k]; aj = negi(aj);
  A[k] = (long)QV_lincomb(u,v, (GEN)A[j],p1);
  A[j] = (long)QV_lincomb(aj,ak, p1,(GEN)A[j]);
}

static GEN
matrixqz_aux(GEN A)
{
  pari_sp av = avma, lim = stack_lim(av,1);
  long i,j,k,n,m;
  GEN a;

  n = lg(A);
  if (n == 1) return cgetg(1,t_MAT);
  if (n == 2) return hnf(A); /* 1 col, maybe 0 */
  m = lg(A[1]);
  for (i=1; i<m; i++)
  {
    for (j = k = 1; j<n; j++)
    {
      GEN a = gcoeff(A,i,j);
      if (gcmp0(a)) continue;

      k = j+1; if (k == n) k = 1;
      /* zero a = Aij  using  b = Aik */
      QV_elem(a, gcoeff(A,i,k), A, j,k);
    }
    a = gcoeff(A,i,k);
    if (!gcmp0(a))
    {
      a = denom(a);
      if (!is_pm1(a)) A[k] = lmul((GEN)A[k], a);
    }
    if (low_stack(lim, stack_lim(av,1)))
    {
      if(DEBUGMEM>1) err(warnmem,"matrixqz_aux");
      A = gerepilecopy(av,A);
    }
  }
  return m > 100? hnfall_i(A,NULL,1): hnf(A);
}

GEN
matrixqz2(GEN x)
{
  pari_sp av = avma;
  if (typ(x)!=t_MAT) err(typeer,"matrixqz2");
  x = dummycopy(x);
  return gerepileupto(av, matrixqz_aux(x));
}

GEN
matrixqz3(GEN x)
{
  pari_sp av = avma, av1, lim;
  long j,j1,k,m,n;
  GEN c;

  if (typ(x)!=t_MAT) err(typeer,"matrixqz3");
  n = lg(x); if (n==1) return gcopy(x);
  m = lg(x[1]); x = dummycopy(x);
  c = cgetg(n,t_VECSMALL);
  for (j=1; j<n; j++) c[j] = 0;
  av1 = avma; lim = stack_lim(av1,1);
  for (k=1; k<m; k++)
  {
    j=1; while (j<n && (c[j] || gcmp0(gcoeff(x,k,j)))) j++;
    if (j==n) continue;

    c[j]=k; x[j]=ldiv((GEN)x[j],gcoeff(x,k,j));
    for (j1=1; j1<n; j1++)
      if (j1!=j)
      {
        GEN t = gcoeff(x,k,j1);
        if (!gcmp0(t)) x[j1] = lsub((GEN)x[j1],gmul(t,(GEN)x[j]));
      }
    if (low_stack(lim, stack_lim(av1,1)))
    {
      if(DEBUGMEM>1) err(warnmem,"matrixqz3");
      x = gerepilecopy(av1,x);
    }
  }
  return gerepileupto(av, matrixqz_aux(x));
}

GEN
intersect(GEN x, GEN y)
{
  pari_sp av,tetpil;
  long j, lx = lg(x);
  GEN z;

  if (typ(x)!=t_MAT || typ(y)!=t_MAT) err(typeer,"intersect");
  if (lx==1 || lg(y)==1) return cgetg(1,t_MAT);

  av=avma; z=ker(concatsp(x,y));
  for (j=lg(z)-1; j; j--) setlg(z[j],lx);
  tetpil=avma; return gerepile(av,tetpil,gmul(x,z));
}

/**************************************************************/
/**                                                          **/
/**		   HERMITE NORMAL FORM REDUCTION	     **/
/**							     **/
/**************************************************************/

GEN
mathnf0(GEN x, long flag)
{
  switch(flag)
  {
    case 0: return hnf(x);
    case 1: return hnfall(x);
    case 3: return hnfperm(x);
    case 4: return hnflll(x);
    default: err(flagerr,"mathnf");
  }
  return NULL; /* not reached */
}

static GEN
init_hnf(GEN x, GEN *denx, long *co, long *li, pari_sp *av)
{
  if (typ(x) != t_MAT) err(typeer,"mathnf");
  *co=lg(x); if (*co==1) return NULL; /* empty matrix */
  *li=lg(x[1]); *denx=denom(x); *av=avma;

  if (gcmp1(*denx)) /* no denominator */
    { *denx = NULL; return dummycopy(x); }
  return Q_muli_to_int(x, *denx);
}

GEN
ZV_copy(GEN x)
{
  long i, lx = lg(x);
  GEN y = cgetg(lx, t_COL);
  for (i=1; i<lx; i++) y[i] = signe(x[i])? licopy((GEN)x[i]): zero;
  return y;
}

GEN
ZM_copy(GEN x)
{
  long i, lx = lg(x);
  GEN y = cgetg(lx, t_MAT);

  for (i=1; i<lx; i++)
    y[i] = (long)ZV_copy((GEN)x[i]);
  return y;
}

/* return c * X. Not memory clean if c = 1 */
GEN
ZV_Z_mul(GEN X, GEN c)
{
  long i,m = lg(X);
  GEN A = new_chunk(m);
  if (signe(c) && is_pm1(c))
  {
    if (signe(c) > 0)
      { for (i=1; i<m; i++) A[i] = X[i]; }
    else
      { for (i=1; i<m; i++) A[i] = lnegi((GEN)X[i]); }
  }
  else /* c = 0 should be exceedingly rare */
    { for (i=1; i<m; i++) A[i] = lmulii(c,(GEN)X[i]); }
  A[0] = X[0]; return A;
}

/* X + v Y */
static GEN
ZV_lincomb1(GEN v, GEN X, GEN Y)
{
  long i, lx = lg(X);
  GEN p1, p2, A = cgetg(lx,t_COL);
  if (is_bigint(v))
  {
    long m = lgefint(v);
    for (i=1; i<lx; i++)
    {
      p1=(GEN)X[i]; p2=(GEN)Y[i];
      if      (!signe(p1)) A[i] = lmulii(v,p2);
      else if (!signe(p2)) A[i] = licopy(p1);
      else
      {
        pari_sp av = avma; (void)new_chunk(m+lgefint(p1)+lgefint(p2)); /*HACK*/
        p2 = mulii(v,p2);
        avma = av; A[i] = laddii(p1,p2);
      }
    }
  }
  else
  {
    long w = itos(v);
    for (i=1; i<lx; i++)
    {
      p1=(GEN)X[i]; p2=(GEN)Y[i];
      if      (!signe(p1)) A[i] = lmulsi(w,p2);
      else if (!signe(p2)) A[i] = licopy(p1);
      else
      {
        pari_sp av = avma; (void)new_chunk(1+lgefint(p1)+lgefint(p2)); /*HACK*/
        p2 = mulsi(w,p2);
        avma = av; A[i] = laddii(p1,p2);
      }
    }
  }
  return A;
}
/* -X + vY */
static GEN
ZV_lincomb_1(GEN v, GEN X, GEN Y)
{
  long i, m, lx = lg(X);
  GEN p1, p2, A = cgetg(lx,t_COL);
  m = lgefint(v);
  for (i=1; i<lx; i++)
  {
    p1=(GEN)X[i]; p2=(GEN)Y[i];
    if      (!signe(p1)) A[i] = lmulii(v,p2);
    else if (!signe(p2)) A[i] = lnegi(p1);
    else
    {
      pari_sp av = avma; (void)new_chunk(m+lgefint(p1)+lgefint(p2)); /* HACK */
      p2 = mulii(v,p2);
      avma = av; A[i] = lsubii(p2,p1);
    }
  }
  return A;
}
GEN
ZV_add(GEN x, GEN y)
{
  long i, lx = lg(x);
  GEN A = cgetg(lx, t_COL);
  for (i=1; i<lx; i++) A[i] = laddii((GEN)x[i], (GEN)y[i]);
  return A;
}
GEN
ZV_sub(GEN x, GEN y)
{
  long i, lx = lg(x);
  GEN A = cgetg(lx, t_COL);
  for (i=1; i<lx; i++) A[i] = lsubii((GEN)x[i], (GEN)y[i]);
  return A;
}
/* X,Y columns; u,v scalars; everybody is integral. return A = u*X + v*Y
 * Not memory clean if (u,v) = (1,0) or (0,1) */
GEN
ZV_lincomb(GEN u, GEN v, GEN X, GEN Y)
{
  pari_sp av;
  long i, lx, m, su, sv;
  GEN p1, p2, A;

  su = signe(u); if (!su) return ZV_Z_mul(Y, v);
  sv = signe(v); if (!sv) return ZV_Z_mul(X, u);
  if (is_pm1(v))
  {
    if (is_pm1(u))
    {
      if (su != sv) A = ZV_sub(X, Y);
      else          A = ZV_add(X, Y);
      if (su < 0) ZV_neg_ip(A);
    }
    else
    {
      if (sv > 0) A = ZV_lincomb1 (u, Y, X);
      else        A = ZV_lincomb_1(u, Y, X);
    }
  }
  else if (is_pm1(u))
  {
    if (su > 0) A = ZV_lincomb1 (v, X, Y);
    else        A = ZV_lincomb_1(v, X, Y);
  }
  else
  {
    lx = lg(X); A = cgetg(lx,t_COL); m = lgefint(u)+lgefint(v);
    for (i=1; i<lx; i++)
    {
      p1=(GEN)X[i]; p2=(GEN)Y[i];
      if      (!signe(p1)) A[i] = lmulii(v,p2);
      else if (!signe(p2)) A[i] = lmulii(u,p1);
      else
      {
        av = avma; (void)new_chunk(m+lgefint(p1)+lgefint(p2)); /* HACK */
        p1 = mulii(u,p1);
        p2 = mulii(v,p2);
        avma = av; A[i] = laddii(p1,p2);
      }
    }
  }
  return A;
}

/* x = [A,U], nbcol(A) = nbcol(U), A integral. Return [AV, UV], with AV HNF */
GEN
hnf_special(GEN x, long remove)
{
  pari_sp av0,av,tetpil,lim;
  long s,li,co,i,j,k,def,ldef;
  GEN p1,u,v,d,denx,a,b, x2,res;

  if (typ(x) != t_VEC || lg(x) !=3) err(typeer,"hnf_special");
  res = cgetg(3,t_VEC);
  av0 = avma;
  x2 = (GEN)x[2];
  x  = (GEN)x[1];
  x = init_hnf(x,&denx,&co,&li,&av);
  if (!x) return cgetg(1,t_MAT);

  lim = stack_lim(av,1);
  def=co-1; ldef=(li>co)? li-co: 0;
  if (lg(x2) != co) err(talker,"incompatible matrices in hnf_special");
  x2 = dummycopy(x2);
  for (i=li-1; i>ldef; i--)
  {
    for (j=def-1; j; j--)
    {
      a = gcoeff(x,i,j);
      if (!signe(a)) continue;

      k = (j==1)? def: j-1;
      b = gcoeff(x,i,k); d = bezout(a,b,&u,&v);
      if (!is_pm1(d)) { a = divii(a,d); b = divii(b,d); }
      p1 = (GEN)x[j]; b = negi(b);
      x[j] = (long)ZV_lincomb(a,b, (GEN)x[k], p1);
      x[k] = (long)ZV_lincomb(u,v, p1, (GEN)x[k]);
      p1 = (GEN)x2[j];
      x2[j]=  ladd(gmul(a, (GEN)x2[k]), gmul(b,p1));
      x2[k] = ladd(gmul(u,p1), gmul(v, (GEN)x2[k]));
      if (low_stack(lim, stack_lim(av,1)))
      {
        GEN *gptr[2]; gptr[0]=&x; gptr[1]=&x2;
        if (DEBUGMEM>1) err(warnmem,"hnf_special[1]. i=%ld",i);
        gerepilemany(av,gptr,2);
      }
    }
    p1 = gcoeff(x,i,def); s = signe(p1);
    if (s)
    {
      if (s < 0)
      {
        x[def] = lneg((GEN)x[def]); p1 = gcoeff(x,i,def);
        x2[def]= lneg((GEN)x2[def]);
      }
      for (j=def+1; j<co; j++)
      {
	b = negi(gdivent(gcoeff(x,i,j),p1));
	x[j] = (long)ZV_lincomb(gun,b, (GEN)x[j], (GEN)x[def]);
        x2[j]= ladd((GEN)x2[j], gmul(b, (GEN)x2[def]));
      }
      def--;
    }
    else
      if (ldef && i==ldef+1) ldef--;
    if (low_stack(lim, stack_lim(av,1)))
    {
      GEN *gptr[2]; gptr[0]=&x; gptr[1]=&x2;
      if (DEBUGMEM>1) err(warnmem,"hnf_special[2]. i=%ld",i);
      gerepilemany(av,gptr,2);
    }
  }
  if (remove)
  {                            /* remove null columns */
    for (i=1,j=1; j<co; j++)
      if (!gcmp0((GEN)x[j]))
      {
        x[i]  = x[j];
        x2[i] = x2[j]; i++;
      }
    setlg(x,i);
    setlg(x2,i);
  }
  tetpil=avma;
  x = denx? gdiv(x,denx): ZM_copy(x);
  x2 = gcopy(x2);
  {
    GEN *gptr[2]; gptr[0]=&x; gptr[1]=&x2;
    gerepilemanysp(av0,tetpil,gptr,2);
  }
  res[1] = (long)x;
  res[2] = (long)x2;
  return res;
}

/*******************************************************************/
/*                                                                 */
/*                SPECIAL HNF (FOR INTERNAL USE !!!)               */
/*                                                                 */
/*******************************************************************/
static int
count(long **mat, long row, long len, long *firstnonzero)
{
  long j, n = 0;

  for (j=1; j<=len; j++)
  {
    long p = mat[j][row];
    if (p)
    {
      if (labs(p)!=1) return -1;
      n++; *firstnonzero=j;
    }
  }
  return n;
}

static int
count2(long **mat, long row, long len)
{
  int j;
  for (j=len; j; j--)
    if (labs(mat[j][row]) == 1) return j;
  return 0;
}

static GEN
hnffinal(GEN matgen,GEN perm,GEN* ptdep,GEN* ptB,GEN* ptC)
{
  GEN p1,p2,U,H,Hnew,Bnew,Cnew,diagH1;
  GEN B = *ptB, C = *ptC, dep = *ptdep, depnew;
  pari_sp av, lim;
  long i,j,k,s,i1,j1,zc;
  long co = lg(C);
  long col = lg(matgen)-1;
  long lnz, nlze, lig;

  if (col == 0) return matgen;
  lnz = lg(matgen[1])-1; /* was called lnz-1 - nr in hnfspec */
  nlze = lg(dep[1])-1;
  lig = nlze + lnz;
  if (DEBUGLEVEL>5)
  {
    fprintferr("Entering hnffinal:\n");
    if (DEBUGLEVEL>6)
    {
      if (nlze) fprintferr("dep = %Z\n",dep);
      fprintferr("mit = %Z\n",matgen);
      fprintferr("B = %Z\n",B);
    }
  }
  /* H: lnz x lnz [disregarding initial 0 cols], U: col x col */
  H = hnflll_i(matgen, &U, 0);
  H += (lg(H)-1 - lnz); H[0] = evaltyp(t_MAT) | evallg(lnz+1);
  /* Only keep the part above the H (above the 0s is 0 since the dep rows
   * are dependent from the ones in matgen) */
  zc = col - lnz; /* # of 0 columns, correspond to units */
  if (nlze) { dep = gmul(dep,U); dep += zc; }

  diagH1 = new_chunk(lnz+1); /* diagH1[i] = 0 iff H[i,i] != 1 (set later) */

  av = avma; lim = stack_lim(av,1);
  Cnew = cgetg(co, typ(C));
  setlg(C, col+1); p1 = gmul(C,U);
  for (j=1; j<=col; j++) Cnew[j] = p1[j];
  for (   ; j<co ; j++)  Cnew[j] = C[j];
  if (DEBUGLEVEL>5) fprintferr("    hnflll done\n");

  /* Clean up B using new H */
  for (s=0,i=lnz; i; i--)
  {
    GEN Di = (GEN)dep[i], Hi = (GEN)H[i];
    GEN h = (GEN)Hi[i]; /* H[i,i] */
    if ( (diagH1[i] = gcmp1(h)) ) { h = NULL; s++; }
    for (j=col+1; j<co; j++)
    {
      GEN z = (GEN)B[j-col];
      p1 = (GEN)z[i+nlze]; if (!signe(p1)) continue;

      if (h) p1 = gdivent(p1,h);
      for (k=1; k<=nlze; k++) z[k] = lsubii((GEN)z[k], mulii(p1, (GEN)Di[k]));
      for (   ; k<=lig;  k++) z[k] = lsubii((GEN)z[k], mulii(p1, (GEN)Hi[k-nlze]));
      Cnew[j] = lsub((GEN)Cnew[j], gmul(p1, (GEN)Cnew[i+zc]));
    }
    if (low_stack(lim, stack_lim(av,1)))
    {
      if(DEBUGMEM>1) err(warnmem,"hnffinal, i = %ld",i);
      gerepileall(av, 2, &Cnew, &B);
    }
  }
  p1 = cgetg(lnz+1,t_VEC); p2 = perm + nlze;
  for (i1=0, j1=lnz-s, i=1; i<=lnz; i++) /* push the 1 rows down */
    if (diagH1[i])
      p1[++j1] = p2[i];
    else
      p2[++i1] = p2[i];
  for (i=i1+1; i<=lnz; i++) p2[i] = p1[i];
  if (DEBUGLEVEL>5) fprintferr("    first pass in hnffinal done\n");

  /* s = # extra redundant generators taken from H
   *          zc  col-s  co   zc = col - lnz
   *       [ 0 |dep |     ]    i = lnze + lnz - s = lig - s
   *  nlze [--------|  B' ]
   *       [ 0 | H' |     ]    H' = H minus the s rows with a 1 on diagonal
   *     i [--------|-----] lig-s           (= "1-rows")
   *       [   0    | Id  ]
   *       [        |     ] li */
  lig -= s; col -= s; lnz -= s;
  Hnew = cgetg(lnz+1,t_MAT);
  depnew = cgetg(lnz+1,t_MAT); /* only used if nlze > 0 */
  Bnew = cgetg(co-col,t_MAT);
  C = dummycopy(Cnew);
  for (j=1,i1=j1=0; j<=lnz+s; j++)
  {
    GEN z = (GEN)H[j];
    if (diagH1[j])
    { /* hit exactly s times */
      i1++; C[i1+col] = Cnew[j+zc];
      p1 = cgetg(lig+1,t_COL); Bnew[i1] = (long)p1;
      for (i=1; i<=nlze; i++) p1[i] = coeff(dep,i,j);
      p1 += nlze;
    }
    else
    {
      j1++; C[j1+zc] = Cnew[j+zc];
      p1 = cgetg(lnz+1,t_COL); Hnew[j1] = (long)p1;
      depnew[j1] = dep[j];
    }
    for (i=k=1; k<=lnz; i++)
      if (!diagH1[i]) p1[k++] = z[i];
  }
  for (j=s+1; j<co-col; j++)
  {
    GEN z = (GEN)B[j-s];
    p1 = cgetg(lig+1,t_COL); Bnew[j] = (long)p1;
    for (i=1; i<=nlze; i++) p1[i] = z[i];
    z += nlze; p1 += nlze;
    for (i=k=1; k<=lnz; i++)
      if (!diagH1[i]) p1[k++] = z[i];
  }
  if (DEBUGLEVEL>5)
  {
    fprintferr("Leaving hnffinal\n");
    if (DEBUGLEVEL>6)
    {
      if (nlze) fprintferr("dep = %Z\n",depnew);
      fprintferr("mit = %Z\nB = %Z\nC = %Z\n", Hnew, Bnew, C);
    }
  }
  *ptdep = depnew;
  *ptC = C;
  *ptB = Bnew; return Hnew;
}

/* for debugging */
static void
p_mat(long **mat, GEN perm, long k)
{
  pari_sp av = avma;
  perm = vecextract_i(perm, k+1, lg(perm)-1);
  fprintferr("Permutation: %Z\n",perm);
  if (DEBUGLEVEL > 6)
    fprintferr("matgen = %Z\n", zm_ZM( rowextract_p((GEN)mat, perm) ));
  avma = av;
}

/* M_k <- M_k + q M_i  (col operations) */
static void
elt_col(GEN Mk, GEN Mi, GEN q)
{
  long j;
  if (is_pm1(q))
  {
    if (signe(q) > 0)
    {
      for (j = lg(Mk)-1; j; j--)
        if (signe(Mi[j])) Mk[j] = laddii((GEN)Mk[j], (GEN)Mi[j]);
    }
    else
    {
      for (j = lg(Mk)-1; j; j--)
        if (signe(Mi[j])) Mk[j] = lsubii((GEN)Mk[j], (GEN)Mi[j]);
    }
  }
  else
    for (j = lg(Mk)-1; j; j--)
      if (signe(Mi[j])) Mk[j] = laddii((GEN)Mk[j], mulii(q, (GEN)Mi[j]));
}

static GEN
col_dup(long l, GEN col)
{
  GEN c = new_chunk(l);
  memcpy(c,col,l * sizeof(long)); return c;
}

/* HNF reduce a relation matrix (column operations + row permutation)
** Input:
**   mat = (li-1) x (co-1) matrix of long
**   C   = r x (co-1) matrix of GEN
**   perm= permutation vector (length li-1), indexing the rows of mat: easier
**     to maintain perm than to copy rows. For columns we can do it directly
**     using e.g. lswap(mat[i], mat[j])
**   k0 = integer. The k0 first lines of mat are dense, the others are sparse.
** Output: cf ASCII art in the function body
**
** row permutations applied to perm
** column operations applied to C. IN PLACE
**/
GEN
hnfspec_i(long** mat0, GEN perm, GEN* ptdep, GEN* ptB, GEN* ptC, long k0)
{
  pari_sp av, lim;
  long co, n, s, nlze, lnz, nr, i, j, k, lk0, col, lig, *p, *matj, **mat;
  GEN p1, p2, matb, matbnew, vmax, matt, T, extramat, B, C, H, dep, permpro;
  const long li = lg(perm); /* = lg(mat0[1]) */
  const long CO = lg(mat0);

  C = *ptC; co = CO;
  if (co > 300 && co > 1.5 * li)
  { /* treat the rest at the end */
    co = (long)(1.2 * li);
    setlg(C, co);
  }

  if (DEBUGLEVEL>5)
  {
    fprintferr("Entering hnfspec\n");
    p_mat(mat0,perm,0);
  }
  matt = cgetg(co, t_MAT); /* dense part of mat (top) */
  mat = (long**)cgetg(co, t_MAT);
  for (j = 1; j < co; j++)
  {
    mat[j] = col_dup(li, (GEN)mat0[j]);
    p1 = cgetg(k0+1,t_COL); matt[j]=(long)p1; matj = mat[j];
    for (i=1; i<=k0; i++) p1[i] = lstoi(matj[perm[i]]);
  }
  vmax = cgetg(co,t_VECSMALL);
  av = avma; lim = stack_lim(av,1);

  i = lig = li-1; col = co-1; lk0 = k0;
  T = (k0 || (lg(C) > 1 && lg(C[1]) > 1))? idmat(col): NULL;
  /* Look for lines with a single non-0 entry, equal to 1 in absolute value */
  while (i > lk0)
    switch( count(mat,perm[i],col,&n) )
    {
      case 0: /* move zero lines between k0+1 and lk0 */
	lk0++; lswap(perm[i], perm[lk0]);
        i = lig; continue;

      case 1: /* move trivial generator between lig+1 and li */
	lswap(perm[i], perm[lig]);
        if (T) lswap(T[n], T[col]);
	swap(mat[n], mat[col]); p = mat[col];
	if (p[perm[lig]] < 0) /* = -1 */
	{ /* convert relation -g = 0 to g = 0 */
	  for (i=lk0+1; i<lig; i++) p[perm[i]] = -p[perm[i]];
          if (T)
          {
            p1 = (GEN)T[col];
            for (i=1; ; i++) /* T is a permuted identity: single non-0 entry */
              if (signe((GEN)p1[i])) { p1[i] = lnegi((GEN)p1[i]); break; }
          }
	}
	lig--; col--; i = lig; continue;

      default: i--;
    }
  if (DEBUGLEVEL>5)
  {
    fprintferr("    after phase1:\n");
    p_mat(mat,perm,0);
  }

#define absmax(s,z) {long _z; _z = labs(z); if (_z > s) s = _z;}
  /* Get rid of all lines containing only 0 and +/- 1, keeping track of column
   * operations in T. Leave the rows 1..lk0 alone [up to k0, coeff
   * explosion, between k0+1 and lk0, row is 0] */
  s = 0;
  while (lig > lk0 && s < (long)(HIGHBIT>>1))
  {
    for (i=lig; i>lk0; i--)
      if (count(mat,perm[i],col,&n) > 0) break;
    if (i == lk0) break;

    /* only 0, +/- 1 entries, at least 2 of them non-zero */
    lswap(perm[i], perm[lig]);
    swap(mat[n], mat[col]); p = mat[col];
    if (T) lswap(T[n], T[col]);
    if (p[perm[lig]] < 0)
    {
      for (i=lk0+1; i<=lig; i++) p[perm[i]] = -p[perm[i]];
      if (T) ZV_neg_ip((GEN)T[col]);
    }
    for (j=1; j<col; j++)
    {
      long t;
      matj = mat[j];
      if (! (t = matj[perm[lig]]) ) continue;
      if (t == 1) {
        for (i=lk0+1; i<=lig; i++) absmax(s, matj[perm[i]] -= p[perm[i]]);
      }
      else { /* t = -1 */
        for (i=lk0+1; i<=lig; i++) absmax(s, matj[perm[i]] += p[perm[i]]);
      }
      if (T) elt_col((GEN)T[j], (GEN)T[col], stoi(-t));
    }
    lig--; col--;
    if (low_stack(lim, stack_lim(av,1)))
    {
      if(DEBUGMEM>1) err(warnmem,"hnfspec[1]");
      if (T) T = gerepilecopy(av, T); else avma = av;
    }
  }
  /* As above with lines containing a +/- 1 (no other assumption).
   * Stop when single precision becomes dangerous */
  for (j=1; j<=col; j++)
  {
    matj = mat[j];
    for (s=0, i=lk0+1; i<=lig; i++) absmax(s, matj[i]);
    vmax[j] = s;
  }
  while (lig > lk0)
  {
    for (i=lig; i>lk0; i--)
      if ( (n = count2(mat,perm[i],col)) ) break;
    if (i == lk0) break;

    lswap(vmax[n], vmax[col]);
    lswap(perm[i], perm[lig]);
    swap(mat[n], mat[col]); p = mat[col];
    if (T) lswap(T[n], T[col]);
    if (p[perm[lig]] < 0)
    {
      for (i=lk0+1; i<=lig; i++) p[perm[i]] = -p[perm[i]];
      if (T) ZV_neg_ip((GEN)T[col]);
    }
    for (j=1; j<col; j++)
    {
      long t;
      matj = mat[j];
      if (! (t = matj[perm[lig]]) ) continue;
      if (vmax[col] && (ulong)labs(t) >= (HIGHBIT-vmax[j]) / vmax[col])
        goto END2;

      for (s=0, i=lk0+1; i<=lig; i++) absmax(s, matj[perm[i]] -= t*p[perm[i]]);
      vmax[j] = s;
      if (T) elt_col((GEN)T[j], (GEN)T[col], stoi(-t));
    }
    lig--; col--;
    if (low_stack(lim, stack_lim(av,1)))
    {
      if(DEBUGMEM>1) err(warnmem,"hnfspec[2]");
      if (T) T = gerepilecopy(av,T); else avma = av;
    }
  }

END2: /* clean up mat: remove everything to the right of the 1s on diagonal */
  /* go multiprecision first */
  matb = cgetg(co,t_MAT); /* bottom part (complement of matt) */
  for (j=1; j<co; j++)
  {
    p1 = cgetg(li-k0,t_COL); matb[j] = (long)p1;
    p1 -= k0; matj = mat[j];
    for (i=k0+1; i<li; i++) p1[i] = lstoi(matj[perm[i]]);
  }
  if (DEBUGLEVEL>5)
  {
    fprintferr("    after phase2:\n");
    p_mat(mat,perm,lk0);
  }
  for (i=li-2; i>lig; i--)
  {
    long h, i0 = i - k0, k = i + co-li;
    GEN Bk = (GEN)matb[k];
    for (j=k+1; j<co; j++)
    {
      GEN Bj = (GEN)matb[j], v = (GEN)Bj[i0];
      s = signe(v); if (!s) continue;

      Bj[i0] = zero;
      if (is_pm1(v))
      {
        if (s > 0) /* v = 1 */
        { for (h=1; h<i0; h++) Bj[h] = lsubii((GEN)Bj[h], (GEN)Bk[h]); }
        else /* v = -1 */
        { for (h=1; h<i0; h++) Bj[h] = laddii((GEN)Bj[h], (GEN)Bk[h]); }
      }
      else {
        for (h=1; h<i0; h++) Bj[h] = lsubii((GEN)Bj[h], mulii(v,(GEN) Bk[h]));
      }
      if (T) elt_col((GEN)T[j], (GEN)T[k], negi(v));
      if (low_stack(lim, stack_lim(av,1)))
      {
        if(DEBUGMEM>1) err(warnmem,"hnfspec[3], (i,j) = %ld,%ld", i,j);
        for (h=1; h<co; h++) setlg(matb[h], i0+1); /* bottom can be forgotten */
        gerepileall(av, T? 2: 1, &matb, &T);
        Bk = (GEN)matb[k];
      }
    }
  }
  for (j=1; j<co; j++) setlg(matb[j], lig-k0+1); /* bottom can be forgotten */
  gerepileall(av, T? 2: 1, &matb, &T);
  if (DEBUGLEVEL>5) fprintferr("    matb cleaned up (using Id block)\n");

  nlze = lk0 - k0;  /* # of 0 rows */
  lnz = lig-nlze+1; /* 1 + # of non-0 rows (!= 0...0 1 0 ... 0) */
  if (T) matt = gmul(matt,T); /* update top rows */
  extramat = cgetg(col+1,t_MAT); /* = new C minus the 0 rows */
  for (j=1; j<=col; j++)
  {
    GEN z = (GEN)matt[j];
    GEN t = ((GEN)matb[j]) + nlze - k0;
    p2=cgetg(lnz,t_COL); extramat[j]=(long)p2;
    for (i=1; i<=k0; i++) p2[i] = z[i]; /* top k0 rows */
    for (   ; i<lnz; i++) p2[i] = t[i]; /* other non-0 rows */
  }
  permpro = imagecomplspec(extramat, &nr); /* lnz = lg(permpro) */

  if (nlze)
  { /* put the nlze 0 rows (trivial generators) at the top */
    p1 = new_chunk(lk0+1);
    for (i=1; i<=nlze; i++) p1[i] = perm[i + k0];
    for (   ; i<=lk0; i++)  p1[i] = perm[i - nlze];
    for (i=1; i<=lk0; i++)  perm[i] = p1[i];
  }
  /* sort other rows according to permpro (nr redundant generators first) */
  p1 = new_chunk(lnz); p2 = perm + nlze;
  for (i=1; i<lnz; i++) p1[i] = p2[permpro[i]];
  for (i=1; i<lnz; i++) p2[i] = p1[i];
  /* perm indexes the rows of mat
   *   |_0__|__redund__|__dense__|__too big__|_____done______|
   *   0  nlze                              lig             li
   *         \___nr___/ \___k0__/
   *         \____________lnz ______________/
   *
   *               col   co
   *       [dep     |     ]
   *    i0 [--------|  B  ] (i0 = nlze + nr)
   *       [matbnew |     ] matbnew has maximal rank = lnz-1 - nr
   * mat = [--------|-----] lig
   *       [   0    | Id  ]
   *       [        |     ] li */

  matbnew = cgetg(col+1,t_MAT); /* dense+toobig, maximal rank. For hnffinal */
  dep    = cgetg(col+1,t_MAT); /* rows dependent from the ones in matbnew */
  for (j=1; j<=col; j++)
  {
    GEN z = (GEN)extramat[j];
    p1 = cgetg(nlze+nr+1,t_COL); dep[j]=(long)p1;
    p2 = cgetg(lnz-nr,t_COL); matbnew[j]=(long)p2;
    for (i=1; i<=nlze; i++) p1[i]=zero;
    p1 += nlze; for (i=1; i<=nr; i++) p1[i] = z[permpro[i]];
    p2 -= nr;   for (   ; i<lnz; i++) p2[i] = z[permpro[i]];
  }

  /* redundant generators in terms of the genuine generators
   * (x_i) = - (g_i) B */
  B = cgetg(co-col,t_MAT);
  for (j=col+1; j<co; j++)
  {
    GEN y = (GEN)matt[j];
    GEN z = (GEN)matb[j];
    p1=cgetg(lig+1,t_COL); B[j-col]=(long)p1;
    for (i=1; i<=nlze; i++) p1[i] = z[i];
    p1 += nlze; z += nlze-k0;
    for (k=1; k<lnz; k++)
    {
      i = permpro[k];
      p1[k] = (i <= k0)? y[i]: z[i];
    }
  }
  if (T) C = gmul(C,T);
  *ptdep = dep;
  *ptB = B;
  H = hnffinal(matbnew, perm, ptdep, ptB, &C);
  if (DEBUGLEVEL)
    msgtimer("hnfspec [%ld x %ld] --> [%ld x %ld]",li-1,co-1, lig-1,col-1);
  if (CO > co)
  { /* treat the rest, N cols at a time (hnflll slow otherwise) */
    const long N = 300;
    long a, L = CO - co, l = min(L, N); /* L columns to add */
    GEN CC = *ptC, m0 = (GEN)mat0;
    setlg(CC, CO); /* restore */
    CC += co-1;
    m0 += co-1;
    for (a = l;;)
    {
      GEN mat = cgetg(l + 1, t_MAT), emb = cgetg(l + 1, t_MAT);
      for (j = 1 ; j <= l; j++)
      {
        mat[j] = (long)m0[j];
        emb[j] = (long)CC[j];
      }
      H = hnfadd_i(H, perm, ptdep, ptB, &C, mat, emb);
      if (a == L) break;
      CC += l;
      m0 += l;
      a += l; if (a > L) { l = L - (a - l); a = L; }
      gerepileall(av, 4, &H,&C,ptB,ptdep); 
    }
  }
  *ptC = C; return H;
}

GEN
hnfspec(long** mat, GEN perm, GEN* ptdep, GEN* ptB, GEN* ptC, long k0)
{
  pari_sp av = avma;
  GEN H = hnfspec_i(mat, perm, ptdep, ptB, ptC, k0);
  gerepileall(av, 4, ptC, ptdep, ptB, &H); return H;
}

/* HNF reduce x, apply same transforms to C */
GEN
mathnfspec(GEN x, GEN *ptperm, GEN *ptdep, GEN *ptB, GEN *ptC)
{
  long i,j,k,ly,lx = lg(x);
  GEN p1,p2,z,perm;
  if (lx == 1) return gcopy(x);
  ly = lg(x[1]);
  z = cgetg(lx,t_MAT);
  perm = cgetg(ly,t_VECSMALL); *ptperm = perm;
  for (i=1; i<ly; i++) perm[i] = i;
  for (i=1; i<lx; i++)
  {
    p1 = cgetg(ly,t_COL); z[i] = (long)p1;
    p2 = (GEN)x[i];
    for (j=1; j<ly; j++)
    {
      if (is_bigint(p2[j])) goto TOOLARGE;
      p1[j] = itos((GEN)p2[j]);
    }
  }
  /*  [ dep |     ]
   *  [-----|  B  ]
   *  [  H  |     ]
   *  [-----|-----]
   *  [  0  | Id  ] */
  return hnfspec((long**)z,perm, ptdep, ptB, ptC, 0);

TOOLARGE:
  if (lg(*ptC) > 1 && lg((*ptC)[1]) > 1)
    err(impl,"mathnfspec with large entries");
  x = hnf(x); lx = lg(x); j = ly; k = 0;
  for (i=1; i<ly; i++)
  {
    if (gcmp1(gcoeff(x,i,i + lx-ly)))
      perm[--j] = i;
    else
      perm[++k] = i;
  }
  setlg(perm,k+1);
  x = rowextract_p(x, perm); /* upper part */
  setlg(perm,ly);
  *ptB = vecextract_i(x, j+lx-ly, lx-1);
  setlg(x, j);
  *ptdep = rowextract_i(x, 1, lx-ly);
  return rowextract_i(x, lx-ly+1, k); /* H */
}

/* same as Flv_ZV, Flv_ZC, Flm_ZM but do not assume positivity */
GEN
zv_ZV(GEN z)
{
  long i, l = lg(z);
  GEN x = cgetg(l, t_VEC);
  for (i=1; i<l; i++) x[i] = lstoi(z[i]);
  return x;
}
GEN
zv_ZC(GEN z)
{
  long i, l = lg(z);
  GEN x = cgetg(l,t_COL);
  for (i=1; i<l; i++) x[i] = lstoi(z[i]);
  return x;
}
GEN
zm_ZM(GEN z)
{
  long i, l = lg(z);
  GEN x = cgetg(l,t_MAT);
  for (i=1; i<l; i++) x[i] = (long)zv_ZC((GEN)z[i]);
  return x;
}
GEN
ZC_zc(GEN z)
{
  long i, l = lg(z);
  GEN x = cgetg(l, t_VECSMALL);
  for (i=1; i<l; i++) x[i] = itos((GEN)z[i]);
  return x;
}
GEN
ZM_zm(GEN z)
{
  long i, l = lg(z);
  GEN x = cgetg(l,t_MAT);
  for (i=1; i<l; i++) x[i] = (long)ZC_zc((GEN)z[i]);
  return x;
}

/* add new relations to a matrix treated by hnfspec (extramat / extraC) */
GEN
hnfadd_i(GEN H, GEN perm, GEN* ptdep, GEN* ptB, GEN* ptC, /* cf hnfspec */
       GEN extramat,GEN extraC)
{
  GEN matb, extratop, Cnew, permpro, B = *ptB, C = *ptC, dep = *ptdep;
  long i;
  long lH = lg(H)-1;
  long lB = lg(B)-1;
  long li = lg(perm)-1, lig = li - lB;
  long co = lg(C)-1,    col = co - lB;
  long nlze = lH? lg(dep[1])-1: lg(B[1])-1;

 /*               col    co
  *       [ 0 |dep |     ]
  *  nlze [--------|  B  ]
  *       [ 0 | H  |     ]
  *       [--------|-----] lig
  *       [   0    | Id  ]
  *       [        |     ] li */
  extratop = zm_ZM( rowextract_ip(extramat, perm, 1, lig) );
  if (li != lig)
  { /* zero out bottom part, using the Id block */
    GEN A = vecextract_i(C, col+1, co);
    GEN c = rowextract_ip(extramat, perm, lig+1, li);
    extraC   = gsub(extraC, typ(A)==t_MAT? RM_zm_mul(A, c): RV_zm_mul(A,c));
    extratop = gsub(extratop, ZM_zm_mul(B, c));
  }

  extramat = concatsp(extratop, vconcat(dep, H));
  Cnew     = concatsp(extraC, vecextract_i(C, col-lH+1, co));
  if (DEBUGLEVEL>5) fprintferr("    1st phase done\n");
  permpro = imagecomplspec(extramat, &nlze);
  extramat = rowextract_p(extramat, permpro);
  *ptB     = rowextract_p(B,        permpro);
  permpro = vecextract_p(perm, permpro);
  for (i=1; i<=lig; i++) perm[i] = permpro[i]; /* perm o= permpro */

  *ptdep  = rowextract_i(extramat, 1, nlze);
  matb    = rowextract_i(extramat, nlze+1, lig);
  if (DEBUGLEVEL>5) fprintferr("    2nd phase done\n");
  H = hnffinal(matb,perm,ptdep,ptB,&Cnew);
  *ptC = concatsp(vecextract_i(C, 1, col-lH), Cnew);
  if (DEBUGLEVEL)
  {
    msgtimer("hnfadd (%ld + %ld)", lg(extratop)-1, lg(dep)-1);
    if (DEBUGLEVEL>7) fprintferr("H = %Z\nC = %Z\n",H,*ptC);
  }
  return H;
}

GEN
hnfadd(GEN H, GEN perm, GEN* ptdep, GEN* ptB, GEN* ptC, /* cf hnfspec */
       GEN extramat,GEN extraC)
{
  pari_sp av = avma;
  H = hnfadd_i(H, perm, ptdep, ptB, ptC, ZM_zm(extramat), extraC);
  gerepileall(av, 4, ptC, ptdep, ptB, &H); return H;
}

static void
ZV_neg(GEN x)
{
  long i, lx = lg(x);
  for (i=1; i<lx ; i++) x[i]=lnegi((GEN)x[i]);
}

/* zero aj = Aij (!= 0)  using  ak = Aik (maybe 0), via linear combination of
 * A[j] and A[k] of determinant 1. If U != NULL, likewise update its columns */
static void
ZV_elem(GEN aj, GEN ak, GEN A, GEN U, long j, long k)
{
  GEN p1,u,v,d;

  if (!signe(ak)) { lswap(A[j],A[k]); if (U) lswap(U[j],U[k]); return; }
  d = bezout(aj,ak,&u,&v);
  /* frequent special case (u,v) = (1,0) or (0,1) */
  if (!signe(u))
  { /* ak | aj */
    p1 = negi(diviiexact(aj,ak));
    A[j]   = (long)ZV_lincomb(gun, p1, (GEN)A[j], (GEN)A[k]);
    if (U)
      U[j] = (long)ZV_lincomb(gun, p1, (GEN)U[j], (GEN)U[k]);
    return;
  }
  if (!signe(v))
  { /* aj | ak */
    p1 = negi(diviiexact(ak,aj));
    A[k]   = (long)ZV_lincomb(gun, p1, (GEN)A[k], (GEN)A[j]);
    lswap(A[j], A[k]);
    if (U) {
      U[k] = (long)ZV_lincomb(gun, p1, (GEN)U[k], (GEN)U[j]);
      lswap(U[j], U[k]);
    }
    return;
  }

  if (!is_pm1(d)) { aj = diviiexact(aj, d); ak = diviiexact(ak, d); }
  p1 = (GEN)A[k]; aj = negi(aj);
  A[k] = (long)ZV_lincomb(u,v, (GEN)A[j],p1);
  A[j] = (long)ZV_lincomb(aj,ak, p1,(GEN)A[j]);
  if (U)
  {
    p1 = (GEN)U[k];
    U[k] = (long)ZV_lincomb(u,v, (GEN)U[j],p1);
    U[j] = (long)ZV_lincomb(aj,ak, p1,(GEN)U[j]);
  }
}

/* reduce A[i,j] mod A[i,j0] for j=j0+1... via column operations */
static void
ZM_reduce(GEN A, GEN U, long i, long j0)
{
  long j, lA = lg(A);
  GEN d = gcoeff(A,i,j0);
  if (signe(d) < 0)
  {
    ZV_neg((GEN)A[j0]);
    if (U) ZV_neg((GEN)U[j0]);
    d = gcoeff(A,i,j0);
  }
  for (j=j0+1; j<lA; j++)
  {
    GEN q = truedvmdii(gcoeff(A,i,j), d, NULL);
    if (!signe(q)) continue;

    q = negi(q);
    A[j]   = (long)ZV_lincomb(gun,q, (GEN)A[j], (GEN)A[j0]);
    if (U)
      U[j] = (long)ZV_lincomb(gun,q, (GEN)U[j], (GEN)U[j0]);
  }
}

/* A,B integral ideals in HNF. Return u in Z^n (v in Z^n not computed), such
 * that Au + Bv = 1 */
GEN
hnfmerge_get_1(GEN A, GEN B)
{
  pari_sp av = avma;
  long j, k, c, l = lg(A), lb;
  GEN b, t, U = cgetg(l + 1, t_MAT), C = cgetg(l + 1, t_VEC);

  t = NULL; /* -Wall */
  b = gcoeff(B,1,1); lb = lgefint(b);
  if (!signe(b)) {
    if (gcmp1(gcoeff(A,1,1))) return gscalcol(gun, l-1);
    l = 0; /* trigger error */
  }
  for (j = 1; j < l; j++)
  {
    c = j+1;
    U[j] = (long)vec_ei(l-1, j);
    U[c] = (long)zerocol(l-1); /* dummy */
    C[j] = (long)vecextract_i((GEN)A[j], 1,j);
    C[c] = (long)vecextract_i((GEN)B[j], 1,j);
    for (k = j; k > 0; k--)
    {
      t = gcoeff(C,k,c);
      if (gcmp0(t)) continue;
      setlg(C[c], k+1);
      ZV_elem(t, gcoeff(C,k,k), C, U, c, k);
      if (lgefint(gcoeff(C,k,k)) > lb) C[k] = (long)FpV_red((GEN)C[k], b);
      if (j > 4)
      {
        GEN u = (GEN)U[k];
        long h;
        for (h=1; h<l; h++)
          if (lgefint(u[h]) > lb) u[h] = lresii((GEN)u[h], b);
      }
    }
    if (j == 1)
      t = gcoeff(C,1,1);
    else
    {
      GEN u,v;
      t = bezout(b, gcoeff(C,1,1), &u, &v); /* >= 0 */
      if (signe(v) && !gcmp1(v)) U[1] = (long)ZV_Z_mul((GEN)U[1], v);
      coeff(C,1,1) = (long)t;
    }
    if (signe(t) && is_pm1(t)) break;
  }
  if (j >= l) err(talker, "non coprime ideals in hnfmerge");
  return gerepileupto(av, gmul(A,(GEN)U[1]));

}

/* remove: throw away lin.dep.columns */
GEN
hnf0(GEN A, int remove)
{
  pari_sp av0 = avma, av, lim;
  long s,li,co,i,j,k,def,ldef;
  GEN denx, a;

  if (typ(A) == t_VEC) return hnf_special(A,remove);
  A = init_hnf(A,&denx,&co,&li,&av);
  if (!A) return cgetg(1,t_MAT);

  lim = stack_lim(av,1);
  def=co-1; ldef=(li>co)? li-co: 0;
  for (i=li-1; i>ldef; i--)
  {
    for (j=def-1; j; j--)
    {
      a = gcoeff(A,i,j);
      if (!signe(a)) continue;

      /* zero a = Aij  using  b = Aik */
      k = (j==1)? def: j-1;
      ZV_elem(a,gcoeff(A,i,k), A,NULL, j,k);

      if (low_stack(lim, stack_lim(av,1)))
      {
        if (DEBUGMEM>1) err(warnmem,"hnf[1]. i=%ld",i);
        A = gerepilecopy(av, A);
      }
    }
    s = signe(gcoeff(A,i,def));
    if (s)
    {
      if (s < 0) ZV_neg((GEN)A[def]);
      ZM_reduce(A, NULL, i,def);
      def--;
    }
    else
      if (ldef) ldef--;
    if (low_stack(lim, stack_lim(av,1)))
    {
      if (DEBUGMEM>1) err(warnmem,"hnf[2]. i=%ld",i);
      A = gerepilecopy(av, A);
    }
  }
  if (remove)
  {                            /* remove null columns */
    for (i=1,j=1; j<co; j++)
      if (!gcmp0((GEN)A[j])) A[i++] = A[j];
    setlg(A,i);
  }
  A = denx? gdiv(A,denx): ZM_copy(A);
  return gerepileupto(av0, A);
}

GEN
hnf(GEN x) { return hnf0(x,1); }

enum { hnf_MOD = 1, hnf_PART = 2 };

/* u*z[1..k] mod b, in place */
static void
FpV_Fp_mul_part_ip(GEN z, GEN u, GEN p, long k)
{
  long i;
  for (i = 1; i <= k; i++) 
    if (signe(z[i])) z[i] = lmodii(mulii(u,(GEN)z[i]), p);
}
static void
FpV_red_part_ip(GEN z, GEN p, long k)
{
  long i;
  for (i = 1; i <= k; i++) z[i] = lmodii((GEN)z[i], p);
}
/* dm = multiple of diag element (usually detint(x)) */
/* flag & MOD:     don't/do append dm*matid. */
/* flag & PART: don't reduce once diagonal is known; */
static GEN
allhnfmod(GEN x, GEN dm, int flag)
{
  pari_sp av, lim;
  const int modid = ((flag & hnf_MOD) == 0);
  long li, co, i, j, k, def, ldef, ldm;
  GEN a, b, w, p1, p2, u, v, DONE = NULL;

  if (typ(dm)!=t_INT) err(typeer,"allhnfmod");
  if (!signe(dm)) return hnf(x);
  if (typ(x)!=t_MAT) err(typeer,"allhnfmod");

  co = lg(x); if (co == 1) return cgetg(1,t_MAT);
  li = lg(x[1]);
  if (modid) DONE = vecsmall_const(li - 1, 0);
  av = avma; lim = stack_lim(av,1);
  x = dummycopy(x);

  ldef = 0;
  if (li > co)
  {
    ldef = li - co;
    if (!modid) err(talker,"nb lines > nb columns in hnfmod");
  }
  /* Avoid wasteful divisions. To prevent coeff explosion, only reduce mod dm
   * when lg(coeff) > ldm */
  ldm = lgefint(dm);
  for (def = co-1,i = li-1; i > ldef; i--,def--)
  {
    coeff(x,i,def) = lresii(gcoeff(x,i,def), dm);
    for (j = def-1; j; j--)
    {
      coeff(x,i,j) = lresii(gcoeff(x,i,j), dm);
      a = gcoeff(x,i,j);
      if (!signe(a)) continue;

      k = (j==1)? def: j-1;
      coeff(x,i,k) = lresii(gcoeff(x,i,k), dm);
      ZV_elem(a,gcoeff(x,i,k), x,NULL, j,k);
      p1 = (GEN)x[j];
      p2 = (GEN)x[k];
      for (k = 1; k < i; k++)
      {
        if (lgefint(p1[k]) > ldm) p1[k] = lresii((GEN)p1[k], dm);
        if (lgefint(p2[k]) > ldm) p2[k] = lresii((GEN)p2[k], dm);
      }
      if (low_stack(lim, stack_lim(av,1)))
      {
        if (DEBUGMEM>1) err(warnmem,"allhnfmod[1]. i=%ld",i);
	x = gerepilecopy(av, x);
      }
    }
    if (modid && !signe(coeff(x,i,def)))
    { /* missing pivot on line i, insert column */
      GEN a = cgetg(co + 1, t_MAT);
      for (k = 1; k <= def; k++) a[k] = x[k];
      a[k++] = (long)vec_Cei(li-1, i, dm);
      for (     ; k <= co;  k++) a[k] = x[k-1];
      ldef--; if (ldef < 0) ldef = 0;
      co++; def++; x = a; DONE[i] = 1;
    }
  }
  w = cgetg(modid? li+1: li, t_MAT); b = dm;
  for (def = co-1,i = li-1; i > ldef; i--,def--)
  {
    GEN d = bezout(gcoeff(x,i,def),b,&u,&v);
    w[i] = x[def];
    FpV_Fp_mul_part_ip((GEN)w[i], u, b, i-1);
    coeff(w,i,i) = (long)d;
    if (!modid && i > 1) b = diviiexact(b,d);
  }
  for (i = 1; i <= ldef; i++){ w[i] = (long)vec_Cei(li-1, i, dm); DONE[i] = 1; }
  if (modid)
  { /* w[li] is an accumulator, discarded at the end */
    for (i = li-1; i > 1; i--)
    { /* add up the missing dm*Id components */
      GEN d, c;
      if (DONE[i]) continue;
      d = gcoeff(w,i,i); if (is_pm1(d)) continue;

      c = cgetg(li, t_COL);
      for (j = 1; j < i; j++) c[j] = lresii(gcoeff(w,j,i),d);
      for (     ; j <li; j++) c[j] = zero;
      if (!egalii(dm,d)) c = gmul(diviiexact(dm,d), c);
      w[li] = (long)c;
      for (j = i - 1; j > 0; j--)
      {
        GEN a = gcoeff(w, j, li);
        if (!signe(a)) continue;
        ZV_elem(a, gcoeff(w,j,j), w, NULL, li,j);
        FpV_red_part_ip((GEN)w[li], dm, j-1);
        FpV_red_part_ip((GEN)w[j],  dm, j-1);
      }
    }
    setlg(w, li);
    for (i = li-1; i > 0; i--)
    {
      GEN d = bezout(gcoeff(w,i,i),dm,&u,&v);
      FpV_Fp_mul_part_ip((GEN)w[i], u, dm, i-1);
      coeff(w,i,i) = (long)d;
    }
  }
  if (flag & hnf_PART) return w;

  if (!modid)
  { /* compute optimal value for dm */
    dm = gcoeff(w,1,1);
    for (i = 2; i < li; i++) dm = mpppcm(dm, gcoeff(w,i,i));
  }

  ldm = lgefint(dm);
  for (i = li-2; i > 0; i--)
  {
    GEN diag = gcoeff(w,i,i);
    for (j = i+1; j < li; j++)
    {
      b = negi(truedvmdii(gcoeff(w,i,j), diag, NULL));
      p1 = ZV_lincomb(gun,b, (GEN)w[j], (GEN)w[i]);
      w[j] = (long)p1;
      for (k=1; k<i; k++)
        if (lgefint(p1[k]) > ldm) p1[k] = lresii((GEN)p1[k], dm);
      if (low_stack(lim, stack_lim(av,1)))
      {
        if (DEBUGMEM>1) err(warnmem,"allhnfmod[2]. i=%ld", i);
        gerepileall(av, 2, &w, &dm); diag = gcoeff(w,i,i);
      }
    }
  }
  return gerepilecopy(av, w);
}

GEN
hnfmod(GEN x, GEN detmat) { return allhnfmod(x,detmat, hnf_MOD); }

GEN
hnfmodid(GEN x, GEN p) { return allhnfmod(x, p, 0); }

GEN
hnfmodidpart(GEN x, GEN p) { return allhnfmod(x, p, hnf_PART); }

/***********************************************************************/
/*                                                                     */
/*                 HNFLLL (Havas, Majewski, Mathews)                   */
/*                                                                     */
/***********************************************************************/

/* negate in place, except universal constants */
static GEN
mynegi(GEN x)
{
  static long mun[]={evaltyp(t_INT)|_evallg(3),evalsigne(-1)|evallgefint(3),1};
  long s = signe(x);

  if (!s) return gzero;
  if (is_pm1(x))
    return (s>0)? mun: gun;
  setsigne(x,-s); return x;
}

static void
Minus(long j, GEN **lambda)
{
  long k, n = lg(lambda);

  for (k=1  ; k<j; k++) lambda[j][k] = mynegi(lambda[j][k]);
  for (k=j+1; k<n; k++) lambda[k][j] = mynegi(lambda[k][j]);
}

void
ZV_neg_ip(GEN M)
{
  long i;
  for (i = lg(M)-1; i; i--)
    M[i] = (long)mynegi((GEN)M[i]);
}

/* index of first non-zero entry */
static long
findi(GEN M)
{
  long i, n = lg(M);
  for (i=1; i<n; i++)
    if (signe(M[i])) return i;
  return 0;
}

static long
findi_normalize(GEN Aj, GEN B, long j, GEN **lambda)
{
  long r = findi(Aj);
  if (r && signe(Aj[r]) < 0)
  {
    ZV_neg_ip(Aj); if (B) ZV_neg_ip((GEN)B[j]);
    Minus(j,lambda);
  }
  return r;
}

static void
reduce2(GEN A, GEN B, long k, long j, long *row0, long *row1, GEN **lambda, GEN *D)
{
  GEN q;
  long i;

  *row0 = findi_normalize((GEN)A[j], B,j,lambda);
  *row1 = findi_normalize((GEN)A[k], B,k,lambda);
  if (*row0)
    q = truedvmdii(gcoeff(A,*row0,k), gcoeff(A,*row0,j), NULL);
  else if (absi_cmp(shifti(lambda[k][j], 1), D[j]) > 0)
    q = diviiround(lambda[k][j], D[j]);
  else
    return;

  if (signe(q))
  {
    GEN *Lk = lambda[k], *Lj = lambda[j];
    q = mynegi(q);
    if (*row0) elt_col((GEN)A[k],(GEN)A[j],q);
    if (B) elt_col((GEN)B[k],(GEN)B[j],q);
    Lk[j] = addii(Lk[j], mulii(q,D[j]));
    if (is_pm1(q))
    {
      if (signe(q) > 0)
      {
        for (i=1; i<j; i++)
          if (signe(Lj[i])) Lk[i] = addii(Lk[i], Lj[i]);
      }
      else
      {
        for (i=1; i<j; i++)
          if (signe(Lj[i])) Lk[i] = subii(Lk[i], Lj[i]);
      }
    }
    else
    {
      for (i=1; i<j; i++)
        if (signe(Lj[i])) Lk[i] = addii(Lk[i], mulii(q,Lj[i]));
    }
  }
}

static void
hnfswap(GEN A, GEN B, long k, GEN **lambda, GEN *D)
{
  GEN t,p1,p2;
  long i,j,n = lg(A);

  lswap(A[k],A[k-1]);
  if (B) lswap(B[k],B[k-1]);
  for (j=k-2; j; j--) swap(lambda[k-1][j], lambda[k][j]);
  for (i=k+1; i<n; i++)
  {
    p1 = mulii(lambda[i][k-1], D[k]);
    p2 = mulii(lambda[i][k], lambda[k][k-1]);
    t = subii(p1,p2);

    p1 = mulii(lambda[i][k], D[k-2]);
    p2 = mulii(lambda[i][k-1], lambda[k][k-1]);
    lambda[i][k-1] = divii(addii(p1,p2), D[k-1]);

    lambda[i][k] = divii(t, D[k-1]);
  }
  p1 = mulii(D[k-2],D[k]);
  p2 = sqri(lambda[k][k-1]);
  D[k-1] = divii(addii(p1,p2), D[k-1]);
}

/* reverse row order in matrix A */
static GEN
fix_rows(GEN A)
{
  long i,j, h,n = lg(A);
  GEN cB,cA,B = cgetg(n,t_MAT);
  if (n == 1) return B;
  h = lg(A[1]);
  for (j=1; j<n; j++)
  {
    cB = cgetg(h, t_COL);
    cA = (GEN)A[j]; B[j] = (long)cB;
    for (i=h>>1; i; i--) { cB[h-i] = cA[i]; cB[i] = cA[h-i]; }
  }
  return B;
}

GEN
hnflll_i(GEN A, GEN *ptB, int remove)
{
  pari_sp av = avma, lim = stack_lim(av,3);
  long m1 = 1, n1 = 1; /* alpha = m1/n1. Maybe 3/4 here ? */
  long do_swap,i,n,k;
  GEN z,B, **lambda, *D;

  if (typ(A) != t_MAT) err(typeer,"hnflll");
  n = lg(A);
  A = ZM_copy(fix_rows(A));
  B = ptB? idmat(n-1): NULL;
  D = (GEN*)cgetg(n+1,t_VEC); lambda = (GEN**) cgetg(n,t_MAT);
  D++;
  for (i=0; i<n; i++) D[i] = gun;
  for (i=1; i<n; i++) lambda[i] = (GEN*)zerocol(n-1);
  k = 2;
  while (k < n)
  {
    long row0, row1;
    reduce2(A,B,k,k-1,&row0,&row1,lambda,D);
    if (row0)
      do_swap = (!row1 || row0 <= row1);
    else if (!row1)
    { /* row0 == row1 == 0 */
      pari_sp av1 = avma;
      z = addii(mulii(D[k-2],D[k]), sqri(lambda[k][k-1]));
      do_swap = (cmpii(mulsi(n1,z), mulsi(m1,sqri(D[k-1]))) < 0);
      avma = av1;
    }
    else
      do_swap = 0;
    if (do_swap)
    {
      hnfswap(A,B,k,lambda,D);
      if (k > 2) k--;
    }
    else
    {
      for (i=k-2; i; i--)
      {
        long row0, row1;
        reduce2(A,B,k,i,&row0,&row1,lambda,D);
        if (low_stack(lim, stack_lim(av,3)))
        {
          GEN b = (GEN)(D-1);
          if (DEBUGMEM) err(warnmem,"hnflll (reducing), i = %ld",i);
          gerepileall(av, B? 4: 3, &A, &lambda, &b, &B);
          D = (GEN*)(b+1);
        }
      }
      k++;
    }
    if (low_stack(lim, stack_lim(av,3)))
    {
      GEN b = (GEN)(D-1);
      if (DEBUGMEM) err(warnmem,"hnflll, k = %ld / %ld",k,n-1);
      gerepileall(av, B? 4: 3, &A, &lambda, &b, &B);
      D = (GEN*)(b+1);
    }
  }
  /* handle trivial case: return negative diag coeff otherwise */
  if (n == 2) (void)findi_normalize((GEN)A[1], B,1,lambda);
  A = fix_rows(A);
  if (remove)
  {
    for (i = 1; i < n; i++)
      if (findi((GEN)A[i])) break;
    i--;
    A += i; A[0] = evaltyp(t_MAT) | evallg(n-i);
  }
  gerepileall(av, B? 2: 1, &A, &B);
  if (B) *ptB = B;
  return A;
}

GEN
hnflll(GEN A)
{
  GEN B, z = cgetg(3, t_VEC);
  z[1] = (long)hnflll_i(A, &B, 0);
  z[2] = (long)B; return z;
}

/* Variation on HNFLLL: Extended GCD */

static void
reduce1(GEN A, GEN B, long k, long j, GEN **lambda, GEN *D)
{
  GEN q;
  long i;

  if (signe(A[j]))
    q = diviiround((GEN)A[k],(GEN)A[j]);
  else if (absi_cmp(shifti(lambda[k][j], 1), D[j]) > 0)
    q = diviiround(lambda[k][j], D[j]);
  else
    return;

  if (signe(q))
  {
    GEN *Lk = lambda[k], *Lj = lambda[j];
    q = mynegi(q);
    A[k] = laddii((GEN)A[k], mulii(q,(GEN)A[j]));
    elt_col((GEN)B[k],(GEN)B[j],q);
    Lk[j] = addii(Lk[j], mulii(q,D[j]));
    for (i=1; i<j; i++)
      if (signe(Lj[i])) Lk[i] = addii(Lk[i], mulii(q,Lj[i]));
  }
}

GEN
extendedgcd(GEN A)
{
  long m1 = 1, n1 = 1; /* alpha = m1/n1. Maybe 3/4 here ? */
  pari_sp av = avma;
  long do_swap,i,n,k;
  GEN z,B, **lambda, *D;

  n = lg(A);
  for (i=1; i<n; i++)
    if (typ(A[i]) != t_INT) err(typeer,"extendedgcd");
  A = dummycopy(A);
  B = idmat(n-1);
  D = (GEN*)new_chunk(n); lambda = (GEN**) cgetg(n,t_MAT);
  for (i=0; i<n; i++) D[i] = gun;
  for (i=1; i<n; i++) lambda[i] = (GEN*)zerocol(n-1);
  k = 2;
  while (k < n)
  {
    reduce1(A,B,k,k-1,lambda,D);
    if (signe(A[k-1])) do_swap = 1;
    else if (!signe(A[k]))
    {
      pari_sp av1 = avma;
      z = addii(mulii(D[k-2],D[k]), sqri(lambda[k][k-1]));
      do_swap = (cmpii(mulsi(n1,z), mulsi(m1,sqri(D[k-1]))) < 0);
      avma = av1;
    }
    else do_swap = 0;

    if (do_swap)
    {
      hnfswap(A,B,k,lambda,D);
      if (k > 2) k--;
    }
    else
    {
      for (i=k-2; i; i--) reduce1(A,B,k,i,lambda,D);
      k++;
    }
  }
  if (signe(A[n-1]) < 0)
  {
    A[n-1] = (long)mynegi((GEN)A[n-1]);
    ZV_neg_ip((GEN)B[n-1]);
  }
  z = cgetg(3,t_VEC);
  z[1] = A[n-1];
  z[2] = (long)B;
  return gerepilecopy(av, z);
}

/* HNF with permutation. */
GEN
hnfperm_i(GEN A, GEN *ptU, GEN *ptperm)
{
  GEN U, c, l, perm, d, p, q, b;
  pari_sp av = avma, av1, lim;
  long r, t, i, j, j1, k, m, n;

  if (typ(A) != t_MAT) err(typeer,"hnfperm");
  n = lg(A)-1;
  if (!n)
  {
    if (ptU) *ptU = cgetg(1,t_MAT);
    if (ptperm) *ptperm = cgetg(1,t_VEC);
    return cgetg(1, t_MAT);
  }
  m = lg(A[1])-1;
  c = vecsmall_const(m, 0);
  l = vecsmall_const(n, 0);
  perm = cgetg(m+1, t_VECSMALL);
  av1 = avma; lim = stack_lim(av1,1);
  A = dummycopy(A);
  U = ptU? idmat(n): NULL;
  /* U base change matrix : A0*U = A all along */
  for (r=0, k=1; k <= n; k++)
  {
    for (j=1; j<k; j++)
    {
      if (!l[j]) continue;
      t=l[j]; b=gcoeff(A,t,k);
      if (!signe(b)) continue;

      ZV_elem(b,gcoeff(A,t,j), A,U,k,j);
      d = gcoeff(A,t,j);
      if (signe(d) < 0)
      {
        ZV_neg((GEN)A[j]);
        if (U) ZV_neg((GEN)U[j]);
        d = gcoeff(A,t,j);
      }
      for (j1=1; j1<j; j1++)
      {
        if (!l[j1]) continue;
        q = truedvmdii(gcoeff(A,t,j1),d,NULL);
        if (!signe(q)) continue;

        q = negi(q);
        A[j1] = (long)ZV_lincomb(gun,q,(GEN)A[j1],(GEN)A[j]);
        if (U) U[j1] = (long)ZV_lincomb(gun,q,(GEN)U[j1],(GEN)U[j]);
      }
    }
    t = m; while (t && (c[t] || !signe(gcoeff(A,t,k)))) t--;
    if (t)
    {
      p = gcoeff(A,t,k);
      for (i=t-1; i; i--)
      {
        q = gcoeff(A,i,k);
	if (signe(q) && absi_cmp(p,q) > 0) { p = q; t = i; }
      }
      perm[++r] = l[k] = t; c[t] = k;
      if (signe(p) < 0)
      {
        ZV_neg((GEN)A[k]);
        if (U) ZV_neg((GEN)U[k]);
	p = gcoeff(A,t,k);
      }
      /* p > 0 */
      for (j=1; j<k; j++)
      {
        if (!l[j]) continue;
	q = truedvmdii(gcoeff(A,t,j),p,NULL);
	if (!signe(q)) continue;

        q = negi(q);
        A[j] = (long)ZV_lincomb(gun,q,(GEN)A[j],(GEN)A[k]);
        if (U) U[j] = (long)ZV_lincomb(gun,q,(GEN)U[j],(GEN)U[k]);
      }
    }
    if (low_stack(lim, stack_lim(av1,1)))
    {
      if (DEBUGMEM>1) err(warnmem,"hnfperm");
      gerepileall(av1, U? 2: 1, &A, &U);
    }
  }
  if (r < m)
  {
    for (i=1,k=r; i<=m; i++)
      if (!c[i]) perm[++k] = i;
  }

/* We have A0*U=A, U in Gl(n,Z)
 * basis for Im(A):  columns of A s.t l[j]>0 (r   cols)
 * basis for Ker(A): columns of U s.t l[j]=0 (n-r cols) */
  p = cgetg(r+1,t_MAT);
  for (i=1; i<=m/2; i++) lswap(perm[i], perm[m+1-i]);
  if (U)
  {
    GEN u = cgetg(n+1,t_MAT);
    for (t=1,k=r,j=1; j<=n; j++)
      if (l[j])
      {
        u[k + n-r] = U[j];
        p[k--] = (long)vecextract_p((GEN)A[j], perm);
      }
      else
        u[t++] = U[j];
    *ptU = u;
    *ptperm = perm;
    gerepileall(av, 3, &p, ptU, ptperm);
  }
  else
  {
    for (k=r,j=1; j<=n; j++)
      if (l[j]) p[k--] = (long)vecextract_p((GEN)A[j], perm);
    if (ptperm) *ptperm = perm;
    gerepileall(av, ptperm? 2: 1, &p, ptperm);
  }
  return p;
}

GEN
hnfperm(GEN A)
{
  GEN U, perm, y = cgetg(4, t_VEC);
  y[1] = (long)hnfperm_i(A, &U, &perm);
  y[2] = (long)U;
  y[3] = (long)zv_ZV(perm); return y;
}

/* Hermite Normal Form */
GEN
hnfall_i(GEN A, GEN *ptB, long remove)
{
  GEN B,c,h,x,p,a;
  pari_sp av = avma, av1, lim;
  long m,n,r,i,j,k,li;

  if (typ(A)!=t_MAT) err(typeer,"hnfall");
  n=lg(A)-1;
  if (!n)
  {
    if (ptB) *ptB = cgetg(1,t_MAT);
    return cgetg(1,t_MAT);
  }
  m = lg(A[1])-1;
  c = cgetg(m+1,t_VECSMALL); for (i=1; i<=m; i++) c[i]=0;
  h = cgetg(n+1,t_VECSMALL); for (j=1; j<=n; j++) h[j]=m;
  av1 = avma; lim = stack_lim(av1,1);
  A = dummycopy(A);
  B = ptB? idmat(n): NULL;
  r = n+1;
  for (li=m; li; li--)
  {
    for (j=1; j<r; j++)
    {
      for (i=h[j]; i>li; i--)
      {
        a = gcoeff(A,i,j);
	if (!signe(a)) continue;

        k = c[i]; /* zero a = Aij  using  Aik */
        ZV_elem(a,gcoeff(A,i,k), A,B,j,k);
        ZM_reduce(A,B, i,k);
        if (low_stack(lim, stack_lim(av1,1)))
        {
          if (DEBUGMEM>1) err(warnmem,"hnfall[1], li = %ld", li);
          gerepileall(av1, B? 2: 1, &A, &B);
        }	
      }
      x = gcoeff(A,li,j); if (signe(x)) break;
      h[j] = li-1;
    }
    if (j == r) continue;
    r--;
    if (j < r) /* A[j] != 0 */
    {
      lswap(A[j], A[r]);
      if (B) lswap(B[j], B[r]);
      h[j]=h[r]; h[r]=li; c[li]=r;
    }
    p = gcoeff(A,li,r);
    if (signe(p) < 0)
    {
      ZV_neg((GEN)A[r]);
      if (B) ZV_neg((GEN)B[r]);
      p = gcoeff(A,li,r);
    }
    ZM_reduce(A,B, li,r);
    if (low_stack(lim, stack_lim(av1,1)))
    {
      if (DEBUGMEM>1) err(warnmem,"hnfall[2], li = %ld", li);
      gerepileall(av1, B? 2: 1, &A, &B);
    }	
  }
  if (DEBUGLEVEL>5) fprintferr("\nhnfall, final phase: ");
  r--; /* first r cols are in the image the n-r (independent) last ones */
  for (j=1; j<=r; j++)
    for (i=h[j]; i; i--)
    {
      a = gcoeff(A,i,j);
      if (!signe(a)) continue;

      k = c[i];
      ZV_elem(a,gcoeff(A,i,k), A,B, j,k);
      ZM_reduce(A,B, i,k);
      if (low_stack(lim, stack_lim(av1,1)))
      {
        if (DEBUGMEM>1) err(warnmem,"hnfall[3], j = %ld", j);
        gerepileall(av1, B? 2: 1, &A, &B);
      }	
    }
  if (DEBUGLEVEL>5) fprintferr("\n");
  /* remove the first r columns */
  if (remove) { A += r; A[0] = evaltyp(t_MAT) | evallg(n-r+1); }
  gerepileall(av, B? 2: 1, &A, &B);
  if (B) *ptB = B;
  return A;
}

GEN
hnfall0(GEN A, long remove)
{
  GEN B, z = cgetg(3, t_VEC);
  z[1] = (long)hnfall_i(A, &B, remove);
  z[2] = (long)B; return z;
}

GEN
hnfall(GEN x) {return hnfall0(x,1);}

/***************************************************************/
/**							      **/
/**      	    SMITH NORMAL FORM REDUCTION	              **/
/**							      **/
/***************************************************************/

static GEN
col_mul(GEN x, GEN c)
{
  long s = signe(x);
  GEN xc = NULL;
  if (s)
  {
    if (!is_pm1(x)) xc = gmul(x,c);
    else xc = (s>0)? c: gneg_i(c);
  }
  return xc;
}

static void
do_zero(GEN x)
{
  long i, lx = lg(x);
  for (i=1; i<lx; i++) x[i] = zero;
}

/* c1 <-- u.c1 + v.c2; c2 <-- a.c2 - b.c1 */
static void
update(GEN u, GEN v, GEN a, GEN b, GEN *c1, GEN *c2)
{
  GEN p1,p2;

  u = col_mul(u,*c1);
  v = col_mul(v,*c2);
  if (u) p1 = v? gadd(u,v): u;
  else   p1 = v? v: (GEN)NULL;

  a = col_mul(a,*c2);
  b = col_mul(gneg_i(b),*c1);
  if (a) p2 = b? gadd(a,b): a;
  else   p2 = b? b: (GEN)NULL;

  if (!p1) do_zero(*c1); else *c1 = p1;
  if (!p2) do_zero(*c2); else *c2 = p2;
}

static GEN
trivsmith(long all)
{
  GEN z;
  if (!all) return cgetg(1,t_VEC);
  z=cgetg(4,t_VEC);
  z[1]=lgetg(1,t_MAT);
  z[2]=lgetg(1,t_MAT);
  z[3]=lgetg(1,t_MAT); return z;
}

static void
snf_pile(pari_sp av, GEN *x, GEN *U, GEN *V)
{
  GEN *gptr[3];
  int c = 1; gptr[0]=x;
  if (*U) gptr[c++] = U;
  if (*V) gptr[c++] = V;
  gerepilemany(av,gptr,c);
}

static void
bezout_step(GEN *pa, GEN *pb, GEN *pu, GEN *pv, GEN mun)
{
  GEN b = *pb, a = *pa;
  if (absi_equal(a,b))
  {
    long sa = signe(a), sb = signe(b);
    if (sb == sa) { *pa = *pu = gun; *pb = gun; }
    else
    {
      if (sa > 0) { *pa = *pu = gun; *pb = mun; }
      else        { *pa = *pu = mun; *pb = gun; }
    }
    *pv = gzero;
  }
  else {
    GEN d = bezout(a,b, pu,pv);
    *pa = diviiexact(a, d);
    *pb = diviiexact(b, d);
  }
}

static int
invcmpii(GEN x, GEN y) { return -cmpii(x,y); }

/* Return the SNF D of matrix X. If ptU/ptV non-NULL set them to U/V
 * to that D = UXV */
GEN
smithall(GEN x, GEN *ptU, GEN *ptV)
{
  pari_sp av0 = avma, av, lim = stack_lim(av0,1);
  long i, j, k, m0, m, n0, n;
  GEN p1, u, v, U, V, V0, mun, mdet, ys, perm = NULL;

  if (typ(x)!=t_MAT) err(typeer,"smithall");
  n0 = n = lg(x)-1;
  if (!n) {
    if (ptU) *ptU = cgetg(1,t_MAT);
    if (ptV) *ptV = cgetg(1,t_MAT);
    return cgetg(1,t_MAT);
  }
  mun = negi(gun); av = avma;
  m0 = m = lg(x[1])-1;
  for (j=1; j<=n; j++)
    for (i=1; i<=m; i++)
      if (typ(coeff(x,i,j)) != t_INT)
        err(talker,"non integral matrix in smithall");

  U = ptU? gun: NULL; /* TRANSPOSE of row transform matrix [so act on columns]*/
  V = ptV? gun: NULL;
  V0 = NULL;
  x = dummycopy(x);
  if (m == n && Z_ishnfall(x))
  {
    mdet = dethnf_i(x);
    if (V) *ptV = idmat(n);
  }
  else
  {
    mdet = detint(x);
    if (signe(mdet))
    {
      if (!V)
        p1 = hnfmod(x,mdet);
      else
      {
        if (m == n)
        {
          p1 = hnfmod(x,mdet);
          *ptV = gauss(x,p1);
        }
        else
          p1 = hnfperm_i(x, ptV, ptU? &perm: NULL);
      }
      mdet = dethnf_i(p1);
    }
    else
      p1 = hnfperm_i(x, ptV, ptU? &perm: NULL);
    x = p1;
  }
  n = lg(x)-1;
  if (V)
  {
    V = *ptV;
    if (n != n0)
    {
      V0 = vecextract_i(V, 1, n0 - n); /* kernel */
      V  = vecextract_i(V, n0-n+1, n0);
      av = avma;
    }
  }
  /* independent columns */
  if (!signe(mdet))
  {
    if (n)
    {
      x = smithall(gtrans_i(x), ptV, ptU); /* ptV, ptU swapped! */
      if (typ(x) == t_MAT && n != m) x = gtrans_i(x);
      if (V) V = gmul(V, gtrans_i(*ptV));
      if (U) U = *ptU; /* TRANSPOSE */
    }
    else /* 0 matrix */
    {
      x = cgetg(1,t_MAT);
      if (V) V = cgetg(1, t_MAT);
      if (U) U = idmat(m);
    }
    goto THEEND;
  }
  if (U) U = idmat(n);

  /* square, maximal rank n */
  p1 = gen_sort(mattodiagonal_i(x), cmp_IND | cmp_C, &invcmpii);
  ys = cgetg(n+1,t_MAT);
  for (j=1; j<=n; j++) ys[j] = (long)vecextract_p((GEN)x[p1[j]], p1);
  x = ys;
  if (U) U = vecextract_p(U, p1);
  if (V) V = vecextract_p(V, p1);

  p1 = hnfmod(x, mdet);
  if (V) V = gmul(V, gauss(x,p1));
  x = p1;

  if (DEBUGLEVEL>7) fprintferr("starting SNF loop");
  for (i=n; i>1; i--)
  {
    if (DEBUGLEVEL>7) fprintferr("\ni = %ld: ",i);
    for(;;)
    {
      int c = 0;
      GEN a, b;
      for (j=i-1; j>=1; j--)
      {
	b = gcoeff(x,i,j); if (!signe(b)) continue;
        a = gcoeff(x,i,i);
        ZV_elem(b, a, x,V, j,i);
        if (low_stack(lim, stack_lim(av,1)))
        {
          if (DEBUGMEM>1) err(warnmem,"[1]: smithall i = %ld", i);
          snf_pile(av, &x,&U,&V);
        }
      }
      if (DEBUGLEVEL>7) fprintferr("; ");
      for (j=i-1; j>=1; j--)
      {
	b = gcoeff(x,j,i); if (!signe(b)) continue;
        a = gcoeff(x,i,i);
        bezout_step(&a, &b, &u, &v, mun);
        for (k=1; k<=i; k++)
        {
          GEN t = addii(mulii(u,gcoeff(x,i,k)),mulii(v,gcoeff(x,j,k)));
          coeff(x,j,k) = lsubii(mulii(a,gcoeff(x,j,k)),
                                mulii(b,gcoeff(x,i,k)));
          coeff(x,i,k) = (long)t;
        }
        if (U) update(u,v,a,b,(GEN*)(U+i),(GEN*)(U+j));
        if (low_stack(lim, stack_lim(av,1)))
        {
          if (DEBUGMEM>1) err(warnmem,"[2]: smithall, i = %ld", i);
          snf_pile(av, &x,&U,&V);
        }
        c = 1;
      }
      if (!c)
      {
	b = gcoeff(x,i,i); if (!signe(b)) break;

        for (k=1; k<i; k++)
        {
          for (j=1; j<i; j++)
            if (signe(resii(gcoeff(x,k,j),b))) break;
          if (j != i) break;
        }
        if (k == i) break;

        /* x[k,j] != 0 mod b */
        for (j=1; j<=i; j++)
          coeff(x,i,j) = laddii(gcoeff(x,i,j),gcoeff(x,k,j));
        if (U) U[i] = ladd((GEN)U[i],(GEN)U[k]);
      }
      if (low_stack(lim, stack_lim(av,1)))
      {
        if (DEBUGMEM>1) err(warnmem,"[3]: smithall");
        snf_pile(av, &x,&U,&V);
      }
    }
  }
  if (DEBUGLEVEL>7) fprintferr("\n");
  for (k=1; k<=n; k++)
    if (signe(gcoeff(x,k,k)) < 0)
    {
      if (V) V[k]=lneg((GEN)V[k]);
      coeff(x,k,k) = lnegi(gcoeff(x,k,k));
    }
THEEND:
  if (!U && !V)
  {
    if (typ(x) == t_MAT) x = mattodiagonal_i(x);
    m = lg(x)-1;
    if (m0 > m) x = concatsp(zerovec(m0-m), x);
    return gerepilecopy(av0, x);
  }

  if (V0)
  {
    x = concatsp(zeromat(m,n0-n), x);
    if (V) V = concatsp(V0, V);
  }
  if (U)
  {
    U = gtrans_i(U);
    if (perm) U = vecextract_p(U, perm_inv(perm));
  }
  snf_pile(av0, &x,&U,&V);
  if (ptU) *ptU = U;
  if (ptV) *ptV = V;
  return x;
}

GEN
smith(GEN x) { return smithall(x, NULL,NULL); }

GEN
smith2(GEN x)
{
  GEN z = cgetg(4, t_VEC);
  z[3] = (long)smithall(x, (GEN*)(z+1),(GEN*)(z+2));
  return z;
}

/* Assume z was computed by [g]smithall(). Remove the 1s on the diagonal */
GEN
smithclean(GEN z)
{
  long i,j,l,c;
  GEN u,v,y,d,p1;

  if (typ(z) != t_VEC) err(typeer,"smithclean");
  l = lg(z); if (l == 1) return cgetg(1,t_VEC);
  u=(GEN)z[1];
  if (l != 4 || typ(u) != t_MAT)
  {
    if (typ(u) != t_INT) err(typeer,"smithclean");
    for (c=1; c<l; c++)
      if (gcmp1((GEN)z[c])) break;
    return gcopy_i(z, c);
  }
  v=(GEN)z[2]; d=(GEN)z[3]; l = lg(d);
  for (c=1; c<l; c++)
    if (gcmp1(gcoeff(d,c,c))) break;

  y=cgetg(4,t_VEC);
  y[1]=(long)(p1 = cgetg(l,t_MAT));
  for (i=1; i<l; i++) p1[i] = (long)gcopy_i((GEN)u[i], c);
  y[2]=(long)gcopy_i(v, c);
  y[3]=(long)(p1 = cgetg(c,t_MAT));
  for (i=1; i<c; i++)
  {
    GEN p2 = cgetg(c,t_COL); p1[i] = (long)p2;
    for (j=1; j<c; j++) p2[j] = i==j? lcopy(gcoeff(d,i,i)): zero;
  }
  return y;
}

static void
gbezout_step(GEN *pa, GEN *pb, GEN *pu, GEN *pv)
{
  GEN a = *pa, b = *pb;
  if (!signe(a))
  {
    *pa = gzero;
    *pb = gun;
    *pu = gzero;
    *pv = gun;
  }
  else
  {
    *pv = gdiventres(b, a);
    if (gcmp0((GEN)(*pv)[2]))
    {
      *pa = gun;
      *pb = (GEN)(*pv)[1];
      *pu = gun;
      *pv = gzero;
    }
    else
    {
      GEN d = gbezout(a,b, pu,pv);
      *pa = gdiv(a, d);
      *pb = gdiv(b, d);
    }
  }
}

/* return x or -x such that leading term is > 0 if meaningful */
static int
canon(GEN *px)
{
  GEN x = *px;
  if (typ(x) == t_POL && signe(x))
  {
    GEN d = leading_term(x);
    switch (typ(d))
    {
      case t_INT: case t_REAL: case t_FRAC: case t_FRACN:
        if (gsigne(d) < 0) { *px = gneg(x); return 1; }
    }
  }
  return 0;
}

static GEN
gsmithall(GEN x,long all)
{
  pari_sp av, lim;
  long i, j, k, n;
  GEN z, u, v, U, V;

  if (typ(x)!=t_MAT) err(typeer,"gsmithall");
  n = lg(x)-1;
  if (!n) return trivsmith(all);
  if (lg(x[1]) != n+1) err(mattype1,"gsmithall");
  av = avma; lim = stack_lim(av,1);
  x = dummycopy(x);
  if (all) { U = idmat(n); V = idmat(n); }
  for (i=n; i>=2; i--)
  {
    for(;;)
    {
      GEN a, b;
      int c = 0;
      for (j=i-1; j>=1; j--)
      {
	b = gcoeff(x,i,j); if (!signe(b)) continue;
        a = gcoeff(x,i,i);
        gbezout_step(&a, &b, &u, &v);
        for (k=1; k<=i; k++)
        {
          GEN t = gadd(gmul(u,gcoeff(x,k,i)),gmul(v,gcoeff(x,k,j)));
          coeff(x,k,j) = lsub(gmul(a,gcoeff(x,k,j)),gmul(b,gcoeff(x,k,i)));
          coeff(x,k,i) = (long)t;
        }
        if (all) update(u,v,a,b,(GEN*)(V+i),(GEN*)(V+j));
      }
      for (j=i-1; j>=1; j--)
      {
	b = gcoeff(x,j,i); if (!signe(b)) continue;
        a = gcoeff(x,i,i);
        gbezout_step(&a, &b, &u, &v);
        for (k=1; k<=i; k++)
        {
          GEN t = gadd(gmul(u,gcoeff(x,i,k)),gmul(v,gcoeff(x,j,k)));
          coeff(x,j,k) = lsub(gmul(a,gcoeff(x,j,k)),gmul(b,gcoeff(x,i,k)));
          coeff(x,i,k) = (long)t;
        }
        if (all) update(u,v,a,b,(GEN*)(U+i),(GEN*)(U+j));
        c = 1;
      }
      if (!c)
      {
	b = gcoeff(x,i,i); if (!signe(b)) break;

        for (k=1; k<i; k++)
        {
          for (j=1; j<i; j++)
            if (signe(gmod(gcoeff(x,k,j),b))) break;
          if (j != i) break;
        }
        if (k == i) break;

        for (j=1; j<=i; j++)
          coeff(x,i,j) = ladd(gcoeff(x,i,j),gcoeff(x,k,j));
        if (all) U[i] = ladd((GEN)U[i],(GEN)U[k]);
      }
      if (low_stack(lim, stack_lim(av,1)))
      {
	if (DEBUGMEM>1) err(warnmem,"gsmithall");
        gerepileall(av, all? 3: 1, &x, &U, &V);
      }
    }
  }
  for (k=1; k<=n; k++)
  {
    GEN T = gcoeff(x,k,k);
    if (canon(&T))
    {
      if (all) V[k] = lneg((GEN)V[k]);
      coeff(x,k,k) = (long)T;
    }
  }
  if (!all) z = mattodiagonal_i(x);
  else
  {
    z = cgetg(4, t_VEC);
    z[1] = (long)gtrans_i(U);
    z[2] = (long)V;
    z[3] = (long)x;
  }
  return gerepilecopy(av, z);
}

GEN
matsnf0(GEN x,long flag)
{
  pari_sp av = avma;
  if (flag > 7) err(flagerr,"matsnf");
  if (typ(x) == t_VEC && flag & 4) return smithclean(x);
  if (flag & 2) x = flag&1 ? gsmith2(x): gsmith(x);
  else          x = flag&1 ?  smith2(x):  smith(x);
  if (flag & 4) x = gerepileupto(av, smithclean(x));
  return x;
}

GEN
gsmith(GEN x) { return gsmithall(x,0); }

GEN
gsmith2(GEN x) { return gsmithall(x,1); }
