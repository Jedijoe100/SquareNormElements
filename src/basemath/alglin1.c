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
/**                          (first part)                          **/
/**                                                                **/
/********************************************************************/
#include "pari.h"

/* for linear algebra mod p */
#ifdef LONG_IS_64BIT
#  define MASK (0xffffffff00000000UL)
#else
#  define MASK (0xffff0000UL)
#endif

extern GEN ZM_init_CRT(GEN Hp, ulong p);
extern int ZM_incremental_CRT(GEN H, GEN Hp, GEN q, GEN qp, ulong p);

/*******************************************************************/
/*                                                                 */
/*                         TRANSPOSE                               */
/*                                                                 */
/*******************************************************************/

/* No copy*/
GEN
gtrans_i(GEN x)
{
  long i,j,lx,dx, tx=typ(x);
  GEN y,p1;

  if (! is_matvec_t(tx)) err(typeer,"gtrans_i");
  switch(tx)
  {
    case t_VEC:
      y=dummycopy(x); settyp(y,t_COL); break;

    case t_COL:
      y=dummycopy(x); settyp(y,t_VEC); break;

    case t_MAT:
      lx=lg(x); if (lx==1) return cgetg(1,t_MAT);
      dx=lg(x[1]); y=cgetg(dx,tx);
      for (i=1; i<dx; i++)
      {
	p1=cgetg(lx,t_COL); y[i]=(long)p1;
	for (j=1; j<lx; j++) p1[j]=coeff(x,i,j);
      }
      break;

    default: y=x; break;
  }
  return y;
}

GEN
gtrans(GEN x)
{
  long i,j,lx,dx, tx=typ(x);
  GEN y,p1;

  if (! is_matvec_t(tx)) err(typeer,"gtrans");
  switch(tx)
  {
    case t_VEC:
      y=gcopy(x); settyp(y,t_COL); break;

    case t_COL:
      y=gcopy(x); settyp(y,t_VEC); break;

    case t_MAT:
      lx=lg(x); if (lx==1) return cgetg(1,t_MAT);
      dx=lg(x[1]); y=cgetg(dx,tx);
      for (i=1; i<dx; i++)
      {
	p1=cgetg(lx,t_COL); y[i]=(long)p1;
	for (j=1; j<lx; j++) p1[j]=lcopy(gcoeff(x,i,j));
      }
      break;

    default: y=gcopy(x); break;
  }
  return y;
}

/*******************************************************************/
/*                                                                 */
/*                    CONCATENATION & EXTRACTION                   */
/*                                                                 */
/*******************************************************************/

static GEN
strconcat(GEN x, GEN y)
{
  int flx = 0, fly = 0;
  size_t l;
  char *sx,*sy,*str;

  if (typ(x)==t_STR) sx = GSTR(x); else { flx=1; sx = GENtostr(x); }
  if (typ(y)==t_STR) sy = GSTR(y); else { fly=1; sy = GENtostr(y); }
  l = nchar2nlong(strlen(sx) + strlen(sy) + 1);
  x = cgetg(l + 1, t_STR); str = GSTR(x);
  strcpy(str,sx);
  strcat(str,sy);
  if (flx) free(sx);
  if (fly) free(sy);
  return x;
}

/* concat 3 matrices. Internal */
GEN
concatsp3(GEN x, GEN y, GEN z)
{
  long i, lx = lg(x), ly = lg(y), lz = lg(z);
  GEN t, r = cgetg(lx+ly+lz-2, t_MAT);
  t = r;
  for (i=1; i<lx; i++) *++t = *++x;
  for (i=1; i<ly; i++) *++t = *++y;
  for (i=1; i<lz; i++) *++t = *++z;
  return r;
}

/* concat A and B vertically. Internal */
GEN
vconcat(GEN A, GEN B)
{
  long la,ha,hb,hc,i,j;
  GEN M,a,b,c;

  if (!A) return B;
  if (!B) return A;
  la = lg(A); if (la==1) return A;
  ha = lg(A[1]); M = cgetg(la,t_MAT);
  hb = lg(B[1]); hc = ha+hb-1;
  for (j=1; j<la; j++)
  {
    c = cgetg(hc,t_COL); M[j] = (long)c; a = (GEN)A[j]; b = (GEN)B[j];
    for (i=1; i<ha; i++) *++c = *++a;
    for (i=1; i<hb; i++) *++c = *++b;
  }
  return M;
}

GEN
_veccopy(GEN x) { GEN v = cgetg(2, t_VEC); v[1] = lcopy(x); return v; }
GEN
_vec(GEN x) { GEN v = cgetg(2, t_VEC); v[1] = (long)x; return v; }
GEN
_col(GEN x) { GEN v = cgetg(2, t_COL); v[1] = (long)x; return v; }
GEN
_mat(GEN x) { GEN v = cgetg(2, t_MAT); v[1] = (long)x; return v; }

/* routines for naive growarrays */
GEN
cget1(long l, long t)
{
  GEN z = new_chunk(l);
  z[0] = evaltyp(t) | _evallg(1); return z;
}
void
appendL(GEN x, GEN t)
{
  long l = lg(x); x[l] = (long)t; setlg(x, l+1);
}

GEN
concatsp(GEN x, GEN y)
{
  long tx=typ(x),ty=typ(y),lx=lg(x),ly=lg(y),i;
  GEN z,p1;

  if (tx==t_LIST || ty==t_LIST) return listconcat(x,y);
  if (tx==t_STR  || ty==t_STR)  return strconcat(x,y);

  if (tx==t_MAT && lx==1)
  {
    if (ty!=t_VEC || ly==1) return gtomat(y);
    err(concater);
  }
  if (ty==t_MAT && ly==1)
  {
    if (tx!=t_VEC || lx==1) return gtomat(x);
    err(concater);
  }

  if (! is_matvec_t(tx))
  {
    if (! is_matvec_t(ty))
    {
      z=cgetg(3,t_VEC); z[1]=(long)x; z[2]=(long)y;
      return z;
    }
    z=cgetg(ly+1,ty);
    if (ty != t_MAT) p1 = x;
    else
    {
      if (lg(y[1])!=2) err(concater);
      p1=cgetg(2,t_COL); p1[1]=(long)x;
    }
    for (i=2; i<=ly; i++) z[i]=y[i-1];
    z[1]=(long)p1; return z;
  }
  if (! is_matvec_t(ty))
  {
    z=cgetg(lx+1,tx);
    if (tx != t_MAT) p1 = y;
    else
    {
      if (lg(x[1])!=2) err(concater);
      p1=cgetg(2,t_COL); p1[1]=(long)y;
    }
    for (i=1; i<lx; i++) z[i]=x[i];
    z[lx]=(long)p1; return z;
  }

  if (tx == ty)
  {
    if (tx == t_MAT && lg(x[1]) != lg(y[1])) err(concater);
    z=cgetg(lx+ly-1,tx);
    for (i=1; i<lx; i++) z[i]=x[i];
    for (i=1; i<ly; i++) z[lx+i-1]=y[i];
    return z;
  }

  switch(tx)
  {
    case t_VEC:
      switch(ty)
      {
	case t_COL:
	  if (lx<=2) return (lx==1)? y: concatsp((GEN) x[1],y);
          if (ly>=3) break;
          return (ly==1)? x: concatsp(x,(GEN) y[1]);
	case t_MAT:
	  z=cgetg(ly,ty); if (lx != ly) break;
	  for (i=1; i<ly; i++) z[i]=(long)concatsp((GEN) x[i],(GEN) y[i]);
          return z;
      }
      break;

    case t_COL:
      switch(ty)
      {
	case t_VEC:
	  if (lx<=2) return (lx==1)? y: concatsp((GEN) x[1],y);
	  if (ly>=3) break;
	  return (ly==1)? x: concatsp(x,(GEN) y[1]);
	case t_MAT:
	  if (lx != lg(y[1])) break;
	  z=cgetg(ly+1,ty); z[1]=(long)x;
	  for (i=2; i<=ly; i++) z[i]=y[i-1];
          return z;
      }
      break;

    case t_MAT:
      switch(ty)
      {
	case t_VEC:
	  z=cgetg(lx,tx); if (ly != lx) break;
	  for (i=1; i<lx; i++) z[i]=(long)concatsp((GEN) x[i],(GEN) y[i]);
          return z;
	case t_COL:
	  if (ly != lg(x[1])) break;
	  z=cgetg(lx+1,tx); z[lx]=(long)y;
	  for (i=1; i<lx; i++) z[i]=x[i];
          return z;
      }
      break;
  }
  err(concater);
  return NULL; /* not reached */
}

GEN
concat(GEN x, GEN y)
{
  long tx = typ(x), lx,ty,ly,i;
  GEN z,p1;

  if (!y)
  {
    pari_sp av = avma;
    if (tx == t_LIST)
      { lx = lgeflist(x); i = 2; }
    else if (tx == t_VEC)
      { lx = lg(x); i = 1; }
    else
    {
      err(concater);
      return NULL; /* not reached */
    }
    if (i>=lx) err(talker,"trying to concat elements of an empty vector");
    y = (GEN)x[i++];
    for (; i<lx; i++) y = concatsp(y, (GEN)x[i]);
    return gerepilecopy(av,y);
  }
  ty = typ(y);
  if (tx==t_STR  || ty==t_STR)  return strconcat(x,y);
  if (tx==t_LIST || ty==t_LIST) return listconcat(x,y);
  lx=lg(x); ly=lg(y);

  if (tx==t_MAT && lx==1)
  {
    if (ty!=t_VEC || ly==1) return gtomat(y);
    err(concater);
  }
  if (ty==t_MAT && ly==1)
  {
    if (tx!=t_VEC || lx==1) return gtomat(x);
    err(concater);
  }

  if (! is_matvec_t(tx))
  {
    if (! is_matvec_t(ty))
    {
      z=cgetg(3,t_VEC); z[1]=lcopy(x); z[2]=lcopy(y);
      return z;
    }
    z=cgetg(ly+1,ty);
    if (ty != t_MAT) p1 = gcopy(x);
    else
    {
      if (lg(y[1])!=2) err(concater);
      p1=cgetg(2,t_COL); p1[1]=lcopy(x);
    }
    for (i=2; i<=ly; i++) z[i]=lcopy((GEN) y[i-1]);
    z[1]=(long)p1; return z;
  }
  if (! is_matvec_t(ty))
  {
    z=cgetg(lx+1,tx);
    if (tx != t_MAT) p1 = gcopy(y);
    else
    {
      if (lg(x[1])!=2) err(concater);
      p1=cgetg(2,t_COL); p1[1]=lcopy(y);
    }
    for (i=1; i<lx; i++) z[i]=lcopy((GEN) x[i]);
    z[lx]=(long)p1; return z;
  }

  if (tx == ty)
  {
    if (tx == t_MAT && lg(x[1]) != lg(y[1])) err(concater);
    z=cgetg(lx+ly-1,tx);
    for (i=1; i<lx; i++) z[i]=lcopy((GEN) x[i]);
    for (i=1; i<ly; i++) z[lx+i-1]=lcopy((GEN) y[i]);
    return z;
  }

  switch(tx)
  {
    case t_VEC:
      switch(ty)
      {
	case t_COL:
	  if (lx<=2) return (lx==1)? gcopy(y): concat((GEN) x[1],y);
          if (ly>=3) break;
          return (ly==1)? gcopy(x): concat(x,(GEN) y[1]);
	case t_MAT:
	  z=cgetg(ly,ty); if (lx != ly) break;
	  for (i=1; i<ly; i++) z[i]=lconcat((GEN) x[i],(GEN) y[i]);
          return z;
      }
      break;

    case t_COL:
      switch(ty)
      {
	case t_VEC:
	  if (lx<=2) return (lx==1)? gcopy(y): concat((GEN) x[1],y);
	  if (ly>=3) break;
	  return (ly==1)? gcopy(x): concat(x,(GEN) y[1]);
	case t_MAT:
	  if (lx != lg(y[1])) break;
	  z=cgetg(ly+1,ty); z[1]=lcopy(x);
	  for (i=2; i<=ly; i++) z[i]=lcopy((GEN) y[i-1]);
          return z;
      }
      break;

    case t_MAT:
      switch(ty)
      {
	case t_VEC:
	  z=cgetg(lx,tx); if (ly != lx) break;
	  for (i=1; i<lx; i++) z[i]=lconcat((GEN) x[i],(GEN) y[i]);
          return z;
	case t_COL:
	  if (ly != lg(x[1])) break;
	  z=cgetg(lx+1,tx); z[lx]=lcopy(y);
	  for (i=1; i<lx; i++) z[i]=lcopy((GEN) x[i]);
          return z;
      }
      break;
  }
  err(concater);
  return NULL; /* not reached */
}

static long
str_to_long(char *s, char **pt)
{
  long a = atol(s);
  while (isspace((int)*s)) s++;
  if (*s == '-' || *s == '+') s++;
  while (isdigit((int)*s) || isspace((int)*s)) s++;
  *pt = s; return a;
}

static int
get_range(char *s, long *a, long *b, long *cmpl, long lx)
{
  long max = lx - 1;

  *a = 1; *b = max;
  if (*s == '^') { *cmpl = 1; s++; } else *cmpl = 0;
  if (!*s) return 0;
  if (*s != '.')
  {
    *a = str_to_long(s, &s);
    if (*a < 0) *a += lx;
    if (*a<1 || *a>max) return 0;
  }
  if (*s == '.')
  {
    s++; if (*s != '.') return 0;
    do s++; while (isspace((int)*s));
    if (*s)
    {
      *b = str_to_long(s, &s);
      if (*b < 0) *b += lx;
      if (*b<1 || *b>max || *s) return 0;
    }
    return 1;
  }
  if (*s) return 0;
  *b = *a; return 1;
}

GEN
vecextract_i(GEN A, long y1, long y2)
{
  long i,lB = y2 - y1 + 2;
  GEN B = cgetg(lB, typ(A));
  for (i=1; i<lB; i++) B[i] = A[y1-1+i];
  return B;
}

GEN
vecextract_ip(GEN A, GEN p, long y1, long y2)
{
  long i,lB = y2 - y1 + 2;
  GEN B = cgetg(lB, typ(A));
  for (i=1; i<lB; i++) B[i] = A[p[y1-1+i]];
  return B;
}

/* rowextract_i(rowextract_p(A,p), x1, x2) */
GEN
rowextract_ip(GEN A, GEN p, long x1, long x2)
{
  long i, lB = lg(A);
  GEN B = cgetg(lB, typ(A));
  for (i=1; i<lB; i++) B[i] = (long)vecextract_ip((GEN)A[i],p,x1,x2);
  return B;
}

GEN
rowextract_i(GEN A, long x1, long x2)
{
  long i, lB = lg(A);
  GEN B = cgetg(lB, typ(A));
  for (i=1; i<lB; i++) B[i] = (long)vecextract_i((GEN)A[i],x1,x2);
  return B;
}

/* A[x0,] */
GEN
row(GEN A, long x0)
{
  long i, lB = lg(A);
  GEN B  = cgetg(lB, t_VEC);
  for (i=1; i<lB; i++) B[i] = coeff(A, x0, i);
  return B;
}

/* A[x0, x1..x2] */
GEN
row_i(GEN A, long x0, long x1, long x2)
{
  long i, lB = x2 - x1 + 2;
  GEN B  = cgetg(lB, t_VEC);
  for (i=x1; i<=x2; i++) B[i] = coeff(A, x0, i);
  return B;
}

GEN
vecextract_p(GEN A, GEN p)
{
  long i,lB = lg(p);
  GEN B = cgetg(lB, typ(A));
  for (i=1; i<lB; i++) B[i] = A[p[i]];
  return B;
}

GEN
rowextract_p(GEN A, GEN p)
{
  long i, lB = lg(A);
  GEN B = cgetg(lB, typ(A));
  for (i=1; i<lB; i++) B[i] = (long)vecextract_p((GEN)A[i],p);
  return B;
}

void
vecselect_p(GEN A, GEN B, GEN p, long init, long lB)
{
  long i; setlg(B, lB);
  for (i=init; i<lB; i++) B[i] = A[p[i]];
}

/* B := p . A = row selection according to permutation p. Treat only lower
 * right corner init x init */
void
rowselect_p(GEN A, GEN B, GEN p, long init)
{
  long i, lB = lg(A), lp = lg(p);
  for (i=1; i<init; i++) setlg(B[i],lp);
  for (   ; i<lB;   i++) vecselect_p((GEN)A[i],(GEN)B[i],p,init,lp);
}

GEN
extract(GEN x, GEN l)
{
  pari_sp av;
  long i,j, tl = typ(l), tx = typ(x), lx = lg(x);
  GEN y;

  if (! is_matvec_t(tx)) err(typeer,"extract");
  if (tl==t_INT)
  {
    /* extract components of x as per the bits of mask l */
    if (!signe(l)) return cgetg(1,tx);
    av=avma; y = (GEN) gpmalloc(lx*sizeof(long));
    i = j = 1; while (!mpodd(l)) { l=shifti(l,-1); i++; }
    while (signe(l) && i<lx)
    {
      if (mod2(l)) y[j++] = x[i];
      i++; l=shifti(l,-1);
    }
    if (signe(l)) err(talker,"mask too large in vecextract");
    y[0] = evaltyp(tx) | evallg(j);
    avma=av; x = gcopy(y); free(y); return x;
  }
  if (tl==t_STR)
  {
    char *s = GSTR(l);
    long first, last, cmpl;
    if (! get_range(s, &first, &last, &cmpl, lx))
      err(talker, "incorrect range in extract");
    if (lx == 1) return gcopy(x);
    if (cmpl)
    {
      if (first <= last)
      {
        y = cgetg(lx - (last - first + 1),tx);
        for (j=1; j<first; j++) y[j] = lcopy((GEN)x[j]);
        for (i=last+1; i<lx; i++,j++) y[j] = lcopy((GEN)x[i]);
      }
      else
      {
        y = cgetg(lx - (first - last + 1),tx);
        for (j=1,i=lx-1; i>first; i--,j++) y[j] = lcopy((GEN)x[i]);
        for (i=last-1; i>0; i--,j++) y[j] = lcopy((GEN)x[i]);
      }
    }
    else
    {
      if (first <= last)
      {
        y = cgetg(last-first+2,tx);
        for (i=first,j=1; i<=last; i++,j++) y[j] = lcopy((GEN)x[i]);
      }
      else
      {
        y = cgetg(first-last+2,tx);
        for (i=first,j=1; i>=last; i--,j++) y[j] = lcopy((GEN)x[i]);
      }
    }
    return y;
  }

  if (is_vec_t(tl))
  {
    long ll=lg(l); y=cgetg(ll,tx);
    for (i=1; i<ll; i++)
    {
      j = itos((GEN) l[i]);
      if (j>=lx || j<=0) err(talker,"no such component in vecextract");
      y[i] = lcopy((GEN) x[j]);
    }
    return y;
  }
  if (tl == t_VECSMALL)
  {
    long ll=lg(l); y=cgetg(ll,tx);
    for (i=1; i<ll; i++)
    {
      j = l[i];
      if (j>=lx || j<=0) err(talker,"no such component in vecextract");
      y[i] = lcopy((GEN) x[j]);
    }
    return y;
  }
  err(talker,"incorrect mask in vecextract");
  return NULL; /* not reached */
}

GEN
matextract(GEN x, GEN l1, GEN l2)
{
  pari_sp av = avma, tetpil;

  if (typ(x)!=t_MAT) err(typeer,"matextract");
  x = extract(gtrans(extract(x,l2)),l1); tetpil=avma;
  return gerepile(av,tetpil, gtrans(x));
}

GEN
extract0(GEN x, GEN l1, GEN l2)
{
  if (! l2) return extract(x,l1);
  return matextract(x,l1,l2);
}

/* v[a] + ... + v[b] */
GEN
sum(GEN v, long a, long b)
{
  GEN p;
  long i;
  if (a > b) return gzero;
  if (b > lg(v)-1) err(talker,"incorrect length in sum");
  p = (GEN)v[a];
  for (i=a+1; i<=b; i++) p = gadd(p, (GEN)v[i]);
  return p;
}

/*******************************************************************/
/*                                                                 */
/*                     SCALAR-MATRIX OPERATIONS                    */
/*                                                                 */
/*******************************************************************/

/* create the square nxn matrix equal to z*Id */
static GEN
gscalmat_proto(GEN z, GEN myzero, long n, int flag)
{
  long i,j;
  GEN y = cgetg(n+1,t_MAT);
  if (n < 0) err(talker,"negative size in scalmat");
  if (flag) z = (flag==1)? stoi((long)z): gcopy(z);
  for (i=1; i<=n; i++)
  {
    y[i]=lgetg(n+1,t_COL);
    for (j=1; j<=n; j++)
      coeff(y,j,i) = (i==j)? (long)z: (long)myzero;
  }
  return y;
}

GEN
gscalmat(GEN x, long n) { return gscalmat_proto(x,gzero,n,2); }

GEN
gscalsmat(long x, long n) { return gscalmat_proto((GEN)x,gzero,n,1); }

GEN
idmat(long n) { return gscalmat_proto(gun,gzero,n,0); }

GEN
idmat_intern(long n,GEN myun,GEN z) { return gscalmat_proto(myun,z,n,0); }

GEN
gscalcol_proto(GEN z, GEN myzero, long n)
{
  GEN y = cgetg(n+1,t_COL);
  long i;

  if (n)
  {
    y[1]=(long)z;
    for (i=2; i<=n; i++) y[i]=(long)myzero;
  }
  return y;
}

GEN
zerocol(long n)
{
  GEN y = cgetg(n+1,t_COL);
  long i; for (i=1; i<=n; i++) y[i]=zero;
  return y;
}

GEN
zerovec(long n)
{
  GEN y = cgetg(n+1,t_VEC);
  long i; for (i=1; i<=n; i++) y[i]=zero;
  return y;
}

GEN
zeromat(long m, long n)
{
  GEN y = cgetg(n+1,t_MAT);
  GEN v = zerocol(m);
  long i; for (i=1; i<=n; i++) y[i]=(long)v;
  return y;
}

GEN
gscalcol(GEN x, long n)
{
  GEN y=gscalcol_proto(gzero,gzero,n);
  if (n) y[1]=lcopy(x);
  return y;
}

GEN
gscalcol_i(GEN x, long n) { return gscalcol_proto(x,gzero,n); }

GEN
gtomat(GEN x)
{
  long tx,lx,i;
  GEN y,p1;

  if (!x) return cgetg(1, t_MAT);
  tx = typ(x);
  if (! is_matvec_t(tx))
  {
    y=cgetg(2,t_MAT); p1=cgetg(2,t_COL); y[1]=(long)p1;
    p1[1]=lcopy(x); return y;
  }
  switch(tx)
  {
    case t_VEC:
      lx=lg(x); y=cgetg(lx,t_MAT);
      for (i=1; i<lx; i++)
      {
	p1=cgetg(2,t_COL); y[i]=(long)p1;
	p1[1]=lcopy((GEN) x[i]);
      }
      break;
    case t_COL:
      y=cgetg(2,t_MAT); y[1]=lcopy(x); break;
    default: /* case t_MAT: */
      y=gcopy(x); break;
  }
  return y;
}

long
isscalarmat(GEN x, GEN s)
{
  long nco,i,j;

  if (typ(x)!=t_MAT) err(typeer,"isdiagonal");
  nco=lg(x)-1; if (!nco) return 1;
  if (nco != lg(x[1])-1) return 0;

  for (j=1; j<=nco; j++)
  {
    GEN *col = (GEN*) x[j];
    for (i=1; i<=nco; i++)
      if (i == j)
      {
        if (!gegal(col[i],s)) return 0;
      }
      else if (!gcmp0(col[i])) return 0;
  }
  return 1;
}

long
isdiagonal(GEN x)
{
  long nco,i,j;

  if (typ(x)!=t_MAT) err(typeer,"isdiagonal");
  nco=lg(x)-1; if (!nco) return 1;
  if (nco != lg(x[1])-1) return 0;

  for (j=1; j<=nco; j++)
  {
    GEN *col = (GEN*) x[j];
    for (i=1; i<=nco; i++)
      if (i!=j && !gcmp0(col[i])) return 0;
  }
  return 1;
}

/* create the diagonal matrix, whose diagonal is given by x */
GEN
diagonal(GEN x)
{
  long i,j,lx,tx=typ(x);
  GEN y,p1;

  if (! is_matvec_t(tx)) return gscalmat(x,1);
  if (tx==t_MAT)
  {
    if (isdiagonal(x)) return gcopy(x);
    err(talker,"incorrect object in diagonal");
  }
  lx=lg(x); y=cgetg(lx,t_MAT);
  for (j=1; j<lx; j++)
  {
    p1=cgetg(lx,t_COL); y[j]=(long)p1;
    for (i=1; i<lx; i++)
      p1[i] = (i==j)? lcopy((GEN) x[i]): zero;
  }
  return y;
}

/* compute m*diagonal(d) */
GEN
matmuldiagonal(GEN m, GEN d)
{
  long j=typ(d),lx=lg(m);
  GEN y;

  if (typ(m)!=t_MAT) err(typeer,"matmuldiagonal");
  if (! is_vec_t(j) || lg(d)!=lx)
    err(talker,"incorrect vector in matmuldiagonal");
  y=cgetg(lx,t_MAT);
  for (j=1; j<lx; j++) y[j] = lmul((GEN) d[j],(GEN) m[j]);
  return y;
}

/* compute A*B assuming the result is a diagonal matrix */
GEN
matmultodiagonal(GEN A, GEN B)
{
  long i, j, hA, hB, lA = lg(A), lB = lg(B);
  GEN y = idmat(lB-1);

  if (typ(A) != t_MAT || typ(B) != t_MAT) err(typeer,"matmultodiagonal");
  hA = (lA == 1)? lB: lg(A[1]);
  hB = (lB == 1)? lA: lg(B[1]);
  if (lA != hB || lB != hA) err(consister,"matmultodiagonal");
  for (i=1; i<lB; i++)
  {
    GEN z = gzero;
    for (j=1; j<lA; j++) z = gadd(z, gmul(gcoeff(A,i,j),gcoeff(B,j,i)));
    coeff(y,i,i) = (long)z;
  }
  return y;
}

/* [m[1,1], ..., m[l,l]], internal */
GEN
mattodiagonal_i(GEN m)
{
  long i, lx = lg(m);
  GEN y = cgetg(lx,t_VEC);
  for (i=1; i<lx; i++) y[i] = coeff(m,i,i);
  return y;
}

/* same, public function */
GEN
mattodiagonal(GEN m)
{
  long i, lx = lg(m);
  GEN y = cgetg(lx,t_VEC);

  if (typ(m)!=t_MAT) err(typeer,"mattodiagonal");
  for (i=1; i<lx; i++) y[i] = lcopy(gcoeff(m,i,i));
  return y;
}

/*******************************************************************/
/*                                                                 */
/*                    ADDITION SCALAR + MATRIX                     */
/*                                                                 */
/*******************************************************************/

/* create the square matrix x*Id + y */
GEN
gaddmat(GEN x, GEN y)
{
  long l,d,i,j;
  GEN z, cz,cy;

  l = lg(y); if (l==1) return cgetg(1,t_MAT);
  d = lg(y[1]);
  if (typ(y)!=t_MAT || l!=d) err(mattype1,"gaddmat");
  z=cgetg(l,t_MAT);
  for (i=1; i<l; i++)
  {
    cz = cgetg(d,t_COL); z[i] = (long)cz; cy = (GEN)y[i];
    for (j=1; j<d; j++)
      cz[j] = i==j? ladd(x,(GEN)cy[j]): lcopy((GEN)cy[j]);
  }
  return z;
}

/* same, no copy */
GEN
gaddmat_i(GEN x, GEN y)
{
  long l,d,i,j;
  GEN z, cz,cy;

  l = lg(y); if (l==1) return cgetg(1,t_MAT);
  d = lg(y[1]);
  if (typ(y)!=t_MAT || l!=d) err(mattype1,"gaddmat");
  z = cgetg(l,t_MAT);
  for (i=1; i<l; i++)
  {
    cz = cgetg(d,t_COL); z[i] = (long)cz; cy = (GEN)y[i];
    for (j=1; j<d; j++)
      cz[j] = i==j? ladd(x,(GEN)cy[j]): cy[j];
  }
  return z;
}

/*******************************************************************/
/*                                                                 */
/*                       Solve A*X=B (Gauss pivot)                 */
/*                                                                 */
/*******************************************************************/
#define swap(x,y) { long _t=x; x=y; y=_t; }

/* Assume x is a non-empty matrix. Return 0 if maximal pivot should not be
 * used, 1 otherwise */
static int
use_maximal_pivot(GEN x)
{
  int res = 0;
  long tx,i,j, lx = lg(x), ly = lg(x[1]);
  GEN p1;
  for (i=1; i<lx; i++)
    for (j=1; j<ly; j++)
    {
      p1 = gmael(x,i,j); tx = typ(p1);
      if (!is_scalar_t(tx)) return 0;
      if (precision(p1)) res = 1;
    }
  return res;
}

/* C = A^(-1)(tB) where A, B, C are integral, A is upper triangular, t t_INT */
GEN
gauss_triangle_i(GEN A, GEN B, GEN t)
{
  long n = lg(A)-1, i,j,k;
  GEN m, c = cgetg(n+1,t_MAT);

  if (!n) return c;
  for (k=1; k<=n; k++)
  {
    GEN u = cgetg(n+1, t_COL), b = (GEN)B[k];
    pari_sp av = avma;
    c[k] = (long)u; m = mulii((GEN)b[n],t);
    u[n] = lpileuptoint(av, diviiexact(m, gcoeff(A,n,n)));
    for (i=n-1; i>0; i--)
    {
      av = avma; m = mulii(negi((GEN)b[i]),t);
      for (j=i+1; j<=n; j++)
        m = addii(m, mulii(gcoeff(A,i,j),(GEN) u[j]));
      u[i] = lpileuptoint(av, diviiexact(negi(m), gcoeff(A,i,i)));
    }
  }
  return c;
}

/* A, B integral upper HNF. A^(-1) B integral ? */
int
hnfdivide(GEN A, GEN B)
{
  pari_sp av = avma;
  long n = lg(A)-1, i,j,k;
  GEN u, b, m, r;

  if (!n) return 1;
  if (lg(B)-1 != n) err(consister,"hnfdivide");
  u = cgetg(n+1, t_COL);
  for (k=1; k<=n; k++)
  {
    b = (GEN)B[k];
    m = (GEN)b[k];
    u[k] = ldvmdii(m, gcoeff(A,k,k), &r);
    if (r != gzero) { avma = av; return 0; }
    for (i=k-1; i>0; i--)
    {
      m = negi((GEN)b[i]);
      for (j=i+1; j<=k; j++)
        m = addii(m, mulii(gcoeff(A,i,j),(GEN) u[j]));
      m = dvmdii(m, gcoeff(A,i,i), &r);
      if (r != gzero) { avma = av; return 0; }
      u[i] = lnegi(m);
    }
  }
  avma = av; return 1;
}

/* A upper HNF, b integral vector. Return A^(-1) b if integral,
 * NULL otherwise. */
GEN
hnf_invimage(GEN A, GEN b)
{
  pari_sp av = avma, av2;
  long n = lg(A)-1, i,j;
  GEN u, m, r;

  if (!n) return NULL;
  u = cgetg(n+1, t_COL);
  m = (GEN)b[n];
  if (typ(m) != t_INT) err(typeer,"hnf_invimage");
  u[n] = ldvmdii(m, gcoeff(A,n,n), &r);
  if (r != gzero) { avma = av; return NULL; }
  for (i=n-1; i>0; i--)
  {
    av2 = avma;
    if (typ(b[i]) != t_INT) err(typeer,"hnf_invimage");
    m = negi((GEN)b[i]);
    for (j=i+1; j<=n; j++)
      m = addii(m, mulii(gcoeff(A,i,j),(GEN) u[j]));
    m = dvmdii(m, gcoeff(A,i,i), &r);
    if (r != gzero) { avma = av; return NULL; }
    u[i] = lpileuptoint(av2, negi(m));
  }
  return u;
}

/* A upper HNF, B integral matrix or column. Return A^(-1) B if integral,
 * NULL otherwise. Not memory clean */
GEN 
hnf_gauss(GEN A, GEN B)
{
  long i, l;
  GEN C;

  if (typ(B) == t_COL) return hnf_invimage(A, B);
  l = lg(B);
  C = cgetg(l, t_MAT);
  for (i = 1; i < l; i++)
  {
    C[i] = (long)hnf_invimage(A, (GEN)B[i]);
    if (!C[i]) return NULL;
  }
  return C;
}

GEN
gauss_get_col(GEN a, GEN b, GEN p, long li)
{
  GEN m, u=cgetg(li+1,t_COL);
  long i,j;

  u[li] = ldiv((GEN)b[li], p);
  for (i=li-1; i>0; i--)
  {
    pari_sp av = avma;
    m = gneg_i((GEN)b[i]);
    for (j=i+1; j<=li; j++)
      m = gadd(m, gmul(gcoeff(a,i,j), (GEN)u[j]));
    u[i] = lpileupto(av, gdiv(gneg_i(m), gcoeff(a,i,i)));
  }
  return u;
}

static GEN
Fp_gauss_get_col(GEN a, GEN b, GEN invpiv, long li, GEN p)
{
  GEN m, u=cgetg(li+1,t_COL);
  long i,j;

  u[li] = lresii(mulii((GEN)b[li], invpiv), p);
  for (i=li-1; i>0; i--)
  {
    pari_sp av = avma;
    m = (GEN)b[i];
    for (j=i+1; j<=li; j++)
      m = subii(m, mulii(gcoeff(a,i,j), (GEN)u[j]));
    m = resii(m, p);
    u[i] = lpileuptoint(av, resii(mulii(m, mpinvmod(gcoeff(a,i,i), p)), p));
  }
  return u;
}
static GEN
Fq_gauss_get_col(GEN a, GEN b, GEN invpiv, long li, GEN T, GEN p)
{
  GEN m, u=cgetg(li+1,t_COL);
  long i,j;

  u[li] = (long)Fq_mul((GEN)b[li], invpiv, T,p);
  for (i=li-1; i>0; i--)
  {
    pari_sp av = avma;
    m = (GEN)b[i];
    for (j=i+1; j<=li; j++)
      m = Fq_sub(m, Fq_mul(gcoeff(a,i,j), (GEN)u[j], T, p), NULL,p);
    m = Fq_red(m, T,p);
    u[i] = lpileupto(av, Fq_mul(m, Fq_inv(gcoeff(a,i,i), T,p), T,p));
  }
  return u;
}

typedef ulong* uGEN;
#define ucoeff(a,i,j)  ( ( (uGEN) ( (GEN) (a))[j]) [i])

/* assume 0 <= a[i,j], b[i], invpiv < p */
static uGEN
u_Fp_gauss_get_col_OK(GEN a, uGEN b, ulong invpiv, long li, ulong p)
{
  uGEN u = (uGEN)cgetg(li+1,t_VECSMALL);
  ulong m = b[li] % p;
  long i,j;

  u[li] = (m * invpiv) % p;
  for (i = li-1; i > 0; i--)
  {
    m = p - b[i]%p;
    for (j = i+1; j <= li; j++) {
      if (m & HIGHBIT) m %= p;
      m += ucoeff(a,i,j) * u[j]; /* 0 <= u[j] < p */
    }
    m %= p;
    if (m) m = ((p-m) * invumod(ucoeff(a,i,i), p)) % p; 
    u[i] = m;
  }
  return u;
}
static uGEN
u_Fp_gauss_get_col(GEN a, uGEN b, ulong invpiv, long li, ulong p)
{
  uGEN u = (uGEN)cgetg(li+1,t_VECSMALL);
  ulong m = b[li] % p;
  long i,j;

  u[li] = muluumod(m, invpiv, p);
  for (i=li-1; i>0; i--)
  {
    m = p - b[i]%p;
    for (j = i+1; j <= li; j++)
      m += muluumod(ucoeff(a,i,j), u[j], p);
    m %= p;
    if (m) m = muluumod(p-m, invumod(ucoeff(a,i,i), p), p);
    u[i] = m;
  }
  return u;
}

/* bk += m * bi */
static void
_addmul(GEN b, long k, long i, GEN m)
{
  b[k] = ladd((GEN)b[k], gmul(m, (GEN)b[i]));
}

/* same, reduce mod p */
static void
_Fp_addmul(GEN b, long k, long i, GEN m, GEN p)
{
  if (lgefint(b[i]) > lgefint(p)) b[i] = lresii((GEN)b[i], p);
  b[k] = laddii((GEN)b[k], mulii(m, (GEN)b[i]));
}

/* same, reduce mod (T,p) */
static void
_Fq_addmul(GEN b, long k, long i, GEN m, GEN T, GEN p)
{
  b[i] = (long)Fq_red((GEN)b[i], T,p);
  b[k] = (long)ladd((GEN)b[k], gmul(m, (GEN)b[i]));
}

/* assume m < p && u_OK_ULONG(p) && (! (b[i] & b[k] & MASK)) */
static void
_u_Fp_addmul_OK(uGEN b, long k, long i, ulong m, ulong p)
{
  b[k] += m * b[i];
  if (b[k] & MASK) b[k] %= p;
}
/* assume m < p */
static void
_u_Fp_addmul(uGEN b, long k, long i, ulong m, ulong p)
{
  b[i] %= p;
  b[k] += muluumod(m, b[i], p);
  if (b[k] & MASK) b[k] %= p;
}
/* same m = 1 */
static void
_u_Fp_add(uGEN b, long k, long i, ulong p)
{
  b[k] += b[i];
  if (b[k] & MASK) b[k] %= p;
}

static int
init_gauss(GEN a, GEN *b, long *aco, long *li, int *iscol)
{
  if (typ(a)!=t_MAT) err(mattype1,"gauss");
  *aco = lg(a) - 1;
  if (!*aco) /* a empty */
  {
    if (*b && lg(*b) != 1) err(consister,"gauss");
    return 0;
  }
  *li = lg(a[1])-1;
  if (*li < *aco) err(mattype1,"gauss");
  *iscol = 0;
  if (*b)
  {
    switch(typ(*b))
    {
      case t_MAT:
        if (lg(*b) == 1) return 0;
        *b = dummycopy(*b);
        if (lg((*b)[1])-1 != *aco) err(consister,"gauss");
        break;
      case t_COL: *iscol = 1;
        if (lg(*b)-1 != *aco) err(consister,"gauss");
        *b = _mat( dummycopy(*b) );
        break;
      default: err(typeer,"gauss");
    }
  }
  else
    *b = idmat(*li);
  return 1;
}

/* Gaussan Elimination. Compute a^(-1)*b
 * b is a matrix or column vector, NULL meaning: take the identity matrix
 * If a and b are empty, the result is the empty matrix.
 *
 * li: nb lines of a and b
 * aco: nb columns of a
 * bco: nb columns of b (if matrix)
 *
 * li > aco is allowed if b = NULL, in which case return c such that c a = Id */
GEN
gauss_intern(GEN a, GEN b)
{
  pari_sp av = avma, lim = stack_lim(av,1);
  long i, j, k, li, bco, aco;
  int inexact, iscol;
  GEN p,m,u;

  if (!init_gauss(a, &b, &aco, &li, &iscol)) return cgetg(1, t_MAT);
  a = dummycopy(a);
  bco = lg(b)-1;
  inexact = use_maximal_pivot(a);
  if(DEBUGLEVEL>4) fprintferr("Entering gauss with inexact=%ld\n",inexact);

  p = NULL; /* gcc -Wall */
  for (i=1; i<=aco; i++)
  {
    /* k is the line where we find the pivot */
    p = gcoeff(a,i,i); k = i;
    if (inexact) /* maximal pivot */
    {
      long e, ex = gexpo(p);
      for (j=i+1; j<=li; j++)
      {
        e = gexpo(gcoeff(a,j,i));
        if (e > ex) { ex=e; k=j; }
      }
      if (gcmp0(gcoeff(a,k,i))) return NULL;
    }
    else if (gcmp0(p)) /* first non-zero pivot */
    {
      do k++; while (k<=li && gcmp0(gcoeff(a,k,i)));
      if (k>li) return NULL;
    }

    /* if (k!=i), exchange the lines s.t. k = i */
    if (k != i)
    {
      for (j=i; j<=aco; j++) swap(coeff(a,i,j), coeff(a,k,j));
      for (j=1; j<=bco; j++) swap(coeff(b,i,j), coeff(b,k,j));
      p = gcoeff(a,i,i);
    }
    if (i == aco) break;

    for (k=i+1; k<=li; k++)
    {
      m = gcoeff(a,k,i);
      if (!gcmp0(m))
      {
	m = gneg_i(gdiv(m,p));
	for (j=i+1; j<=aco; j++) _addmul((GEN)a[j],k,i,m);
        for (j=1;   j<=bco; j++) _addmul((GEN)b[j],k,i,m);
      }
    }
    if (low_stack(lim, stack_lim(av,1)))
    {
      if(DEBUGMEM>1) err(warnmem,"gauss. i=%ld",i);
      gerepileall(av,2, &a,&b);
    }
  }

  if(DEBUGLEVEL>4) fprintferr("Solving the triangular system\n");
  u = cgetg(bco+1,t_MAT);
  for (j=1; j<=bco; j++) u[j] = (long)gauss_get_col(a,(GEN)b[j],p,aco);
  return gerepilecopy(av, iscol? (GEN)u[1]: u);
}

GEN
gauss(GEN a, GEN b)
{
  GEN z = gauss_intern(a,b);
  if (!z) err(matinv1);
  return z;
}

/* destroy a, b */
static GEN
Flm_gauss_sp(GEN a, GEN b, ulong p)
{
  long iscol, i, j, k, li, bco, aco = lg(a)-1;
  ulong piv, invpiv, m;
  const int OK_ulong = u_OK_ULONG(p);
  GEN u;

  if (!aco) return cgetg(1,t_MAT);
  li = lg(a[1])-1;
  bco = lg(b)-1;
  iscol = (typ(b)!=t_MAT);
  if (iscol) b = _mat(b);
  piv = invpiv = 0; /* -Wall */
  for (i=1; i<=aco; i++)
  {
    /* k is the line where we find the pivot */
    if (OK_ulong) /* u_Fp_gauss_get_col wants 0 <= a[i,j] < p for all i,j */
      for (k = 1; k < i; k++) ucoeff(a,k,i) %= p;
    for (k = i; k <= li; k++)
    {
      piv = ( ucoeff(a,k,i) %= p );
      if (piv) break;
    }
    if (!piv) return NULL;
    invpiv = invumod(piv, p);

    /* if (k!=i), exchange the lines s.t. k = i */
    if (k != i)
    {
      for (j=i; j<=aco; j++) swap(coeff(a,i,j), coeff(a,k,j));
      for (j=1; j<=bco; j++) swap(coeff(b,i,j), coeff(b,k,j));
    }
    if (i == aco) break;

    for (k=i+1; k<=li; k++)
    {
      m = ( ucoeff(a,k,i) %= p );
      if (!m) continue;

      m = p - muluumod(m, invpiv, p); /* - 1/piv mod p */
      if (m == 1)
      {
        for (j=i+1; j<=aco; j++) _u_Fp_add((uGEN)a[j],k,i, p);
        for (j=1;   j<=bco; j++) _u_Fp_add((uGEN)b[j],k,i, p);
      }
      else if (OK_ulong)
      {
        for (j=i+1; j<=aco; j++) _u_Fp_addmul_OK((uGEN)a[j],k,i,m, p);
        for (j=1;   j<=bco; j++) _u_Fp_addmul_OK((uGEN)b[j],k,i,m, p);
      }
      else
      {
        for (j=i+1; j<=aco; j++) _u_Fp_addmul((uGEN)a[j],k,i,m, p);
        for (j=1;   j<=bco; j++) _u_Fp_addmul((uGEN)b[j],k,i,m, p);
      }
    }
  }
  u = cgetg(bco+1,t_MAT);
  if (OK_ulong)
    for (j=1; j<=bco; j++)
      u[j] = (long)u_Fp_gauss_get_col_OK(a,(uGEN)b[j], invpiv, aco, p);
  else
    for (j=1; j<=bco; j++)
      u[j] = (long)u_Fp_gauss_get_col(a,(uGEN)b[j], invpiv, aco, p);
  return iscol? (GEN)u[1]: u;
}

GEN
u_idmat(long n)
{
  GEN y = cgetg(n+1,t_MAT);
  long i;
  if (n < 0) err(talker,"negative size in u_idmat");
  for (i=1; i<=n; i++) { y[i] = (long)vecsmall_const(n, 0); coeff(y, i,i) = 1; }
  return y;
}

GEN
Flm_gauss(GEN a, GEN b, ulong p) {
  return Flm_gauss_sp(dummycopy(a), dummycopy(b), p);
}
static GEN
Flm_inv_sp(GEN a, ulong p) {
  return Flm_gauss_sp(a, u_idmat(lg(a)-1), p);
}
GEN
Flm_inv(GEN a, ulong p) {
  return Flm_inv_sp(dummycopy(a), p);
}

GEN
FpM_gauss(GEN a, GEN b, GEN p)
{
  pari_sp av = avma, lim;
  long i, j, k, li, bco, aco;
  int iscol;
  GEN piv, invpiv, m, u;

  if (!init_gauss(a, &b, &aco, &li, &iscol)) return cgetg(1, t_MAT);
  if (lgefint(p) == 3)
  {
    ulong pp = (ulong)p[2];
    a = ZM_Flm(a, pp);
    b = ZM_Flm(b, pp);
    u = Flm_gauss_sp(a,b, pp);
    u = iscol? zv_ZC((GEN)u[1]): zm_ZM(u);
    return gerepileupto(av, u);
  }
  lim = stack_lim(av,1);
  a = dummycopy(a);
  bco = lg(b)-1;
  piv = invpiv = NULL; /* -Wall */
  for (i=1; i<=aco; i++)
  {
    GEN minvpiv;
    /* k is the line where we find the pivot */
    for (k = i; k <= li; k++)
    {
      piv = resii(gcoeff(a,k,i), p);
      coeff(a,k,i) = (long)piv;
      if (signe(piv)) break;
    }
    if (k > li) return NULL;
    invpiv  = mpinvmod(piv, p);

    /* if (k!=i), exchange the lines s.t. k = i */
    if (k != i)
    {
      for (j=i; j<=aco; j++) swap(coeff(a,i,j), coeff(a,k,j));
      for (j=1; j<=bco; j++) swap(coeff(b,i,j), coeff(b,k,j));
    }
    if (i == aco) break;

    minvpiv = negi(invpiv);
    for (k=i+1; k<=li; k++)
    {
      coeff(a,k,i) = lresii(gcoeff(a,k,i), p);
      m = gcoeff(a,k,i); coeff(a,k,i) = zero;
      if (signe(m))
      {
	
        m = resii(mulii(m, minvpiv), p); /* -1/piv mod p */
	for (j=i+1; j<=aco; j++) _Fp_addmul((GEN)a[j],k,i,m, p);
        for (j=1  ; j<=bco; j++) _Fp_addmul((GEN)b[j],k,i,m, p);
      }
    }
    if (low_stack(lim, stack_lim(av,1)))
    {
      if(DEBUGMEM>1) err(warnmem,"FpM_gauss. i=%ld",i);
      gerepileall(av,2, &a,&b);
    }
  }

  if(DEBUGLEVEL>4) fprintferr("Solving the triangular system\n");
  u = cgetg(bco+1,t_MAT);
  for (j=1; j<=bco; j++)
    u[j] = (long)Fp_gauss_get_col(a, (GEN)b[j], invpiv, aco, p);
  return gerepilecopy(av, iscol? (GEN)u[1]: u);
}
GEN
FqM_gauss(GEN a, GEN b, GEN T, GEN p)
{
  pari_sp  av = avma, lim;
  long i,j,k,li,bco, aco = lg(a)-1;
  int iscol;
  GEN piv, invpiv, m, u;

  if (!T) return FpM_gauss(a,b,p);
  if (!init_gauss(a, &b, &aco, &li, &iscol)) return cgetg(1, t_MAT);
 
  lim = stack_lim(av,1);
  a = dummycopy(a);
  bco = lg(b)-1;
  piv = invpiv = NULL; /* -Wall */
  for (i=1; i<=aco; i++)
  {
    /* k is the line where we find the pivot */
    for (k = i; k <= li; k++)
    {
      piv = Fq_red(gcoeff(a,k,i), T,p);
      coeff(a,k,i) = (long)piv;
      if (signe(piv)) break;
    }
    if (k > li) return NULL;
    invpiv = Fq_inv(piv,T,p);

    /* if (k!=i), exchange the lines s.t. k = i */
    if (k != i)
    {
      for (j=i; j<=aco; j++) swap(coeff(a,i,j), coeff(a,k,j));
      for (j=1; j<=bco; j++) swap(coeff(b,i,j), coeff(b,k,j));
      piv = gcoeff(a,i,i);
    }
    if (i == aco) break;

    for (k=i+1; k<=li; k++)
    {
      coeff(a,k,i) = (long)Fq_red(gcoeff(a,k,i), T,p);
      m = gcoeff(a,k,i); coeff(a,k,i) = zero;
      if (signe(m))
      {
	m = Fq_neg(Fq_mul(m, invpiv, T,p), T, p);
	for (j=i+1; j<=aco; j++) _Fq_addmul((GEN)a[j],k,i,m, T,p);
        for (j=1;   j<=bco; j++) _Fq_addmul((GEN)b[j],k,i,m, T,p);
      }
    }
    if (low_stack(lim, stack_lim(av,1)))
    {
      if(DEBUGMEM>1) err(warnmem,"FpM_gauss. i=%ld",i);
      gerepileall(av, 2, &a,&b);
    }
  }

  if(DEBUGLEVEL>4) fprintferr("Solving the triangular system\n");
  u = cgetg(bco+1,t_MAT);
  for (j=1; j<=bco; j++)
    u[j] = (long)Fq_gauss_get_col(a, (GEN)b[j], invpiv, aco, T, p);
  return gerepilecopy(av, iscol? (GEN)u[1]: u);
}


GEN
FpM_inv(GEN x, GEN p) { return FpM_gauss(x, NULL, p); }

/* set y = x * y */
GEN
Flm_Fl_mul_inplace(GEN y, ulong x, ulong p)
{
  long i, j, m = lg(y[1]), l = lg(y);
  if (HIGHWORD(x | p))
    for(j=1; j<l; j++)
      for(i=1; i<m; i++)
        ucoeff(y,i,j) = muluumod(ucoeff(y,i,j), x, p);
  else
    for(j=1; j<l; j++)
      for(i=1; i<m; i++)
        ucoeff(y,i,j) = (ucoeff(y,i,j) * x) % p;
  return y;
}

/* M integral, dM such that M' = dM M^-1 is integral [e.g det(M)]. Return M' */
GEN
ZM_inv(GEN M, GEN dM)
{
  pari_sp av2, av = avma, lim = stack_lim(av,1);
  GEN Hp,q,H;
  ulong p,dMp;
  byteptr d = diffptr;
  long lM = lg(M), stable = 0;

  if (lM == 1) return cgetg(1,t_MAT);
  if (!dM) dM = det(M);

  av2 = avma;
  H = NULL;
  d += 3000; /* 27449 = prime(3000) */
  for(p = 27449; ; )
  {
    NEXT_PRIME_VIADIFF_CHECK(p,d);
    dMp = umodiu(dM,p);
    if (!dMp) continue;
    Hp = Flm_inv_sp(ZM_Flm(M,p), p);
    if (dMp != 1) Hp = Flm_Fl_mul_inplace(Hp, dMp, p);

    if (!H)
    {
      H = ZM_init_CRT(Hp, p);
      q = utoi(p);
    }
    else
    {
      GEN qp = muliu(q,p);
      stable = ZM_incremental_CRT(H, Hp, q,qp, p);
      q = qp;
    }
    if (DEBUGLEVEL>5) msgtimer("inverse mod %ld (stable=%ld)", p,stable);
    if (stable && isscalarmat(gmul(M, H), dM)) break; /* DONE */

    if (low_stack(lim, stack_lim(av,2)))
    {
      GEN *gptr[2]; gptr[0] = &H; gptr[1] = &q;
      if (DEBUGMEM>1) err(warnmem,"ZM_inv");
      gerepilemany(av2,gptr, 2);
    }
  }
  if (DEBUGLEVEL>5) msgtimer("ZM_inv done");
  return gerepilecopy(av, H);
}

/* same as above, M rational */
GEN
QM_inv(GEN M, GEN dM)
{
  pari_sp av = avma;
  GEN cM, pM = Q_primitive_part(M, &cM);
  if (!cM) return ZM_inv(pM,dM);
  return gerepileupto(av, ZM_inv(pM, gdiv(dM,cM)));
}

/* x a matrix with integer coefficients. Return a multiple of the determinant
 * of the lattice generated by the columns of x (to be used with hnfmod)
 */
GEN
detint(GEN x)
{
  pari_sp av = avma, av1, lim;
  GEN pass,c,v,det1,piv,pivprec,vi,p1;
  long i,j,k,rg,n,m,m1;

  if (typ(x)!=t_MAT) err(typeer,"detint");
  n=lg(x)-1; if (!n) return gun;
  m1=lg(x[1]); m=m1-1;
  if (n < m) return gzero;
  lim=stack_lim(av,1);
  c=new_chunk(m1); for (k=1; k<=m; k++) c[k]=0;
  av1=avma; pass=cgetg(m1,t_MAT);
  for (j=1; j<=m; j++)
  {
    p1=cgetg(m1,t_COL); pass[j]=(long)p1;
    for (i=1; i<=m; i++) p1[i]=zero;
  }
  for (k=1; k<=n; k++)
    for (j=1; j<=m; j++)
      if (typ(gcoeff(x,j,k)) != t_INT)
        err(talker,"not an integer matrix in detint");
  v=cgetg(m1,t_COL);
  det1=gzero; piv=pivprec=gun;
  for (rg=0,k=1; k<=n; k++)
  {
    long t = 0;
    for (i=1; i<=m; i++)
      if (!c[i])
      {
	vi=mulii(piv,gcoeff(x,i,k));
	for (j=1; j<=m; j++)
	  if (c[j]) vi=addii(vi,mulii(gcoeff(pass,i,j),gcoeff(x,j,k)));
	v[i]=(long)vi; if (!t && signe(vi)) t=i;
      }
    if (t)
    {
      if (rg == m-1)
        { det1=mppgcd((GEN)v[t],det1); c[t]=0; }
      else
      {
        rg++; pivprec = piv; piv=(GEN)v[t]; c[t]=k;
	for (i=1; i<=m; i++)
	  if (!c[i])
	  {
            GEN p2 = negi((GEN)v[i]);
	    for (j=1; j<=m; j++)
	      if (c[j] && j!=t)
	      {
	        p1 = addii(mulii(gcoeff(pass,i,j), piv),
	 	 	   mulii(gcoeff(pass,t,j), p2));
                if (rg>1) p1 = diviiexact(p1,pivprec);
	        coeff(pass,i,j) = (long)p1;
	      }
	    coeff(pass,i,t) = (long)p2;
	  }
      }
    }
    if (low_stack(lim, stack_lim(av,1)))
    {
      GEN *gptr[5];
      if(DEBUGMEM>1) err(warnmem,"detint. k=%ld",k);
      gptr[0]=&det1; gptr[1]=&piv; gptr[2]=&pivprec;
      gptr[3]=&pass; gptr[4]=&v; gerepilemany(av1,gptr,5);
    }
  }
  return gerepileupto(av, absi(det1));
}

static void
gerepile_gauss_ker(GEN x, long k, long t, pari_sp av)
{
  pari_sp tetpil = avma, A;
  long u,i, n = lg(x)-1, m = n? lg(x[1])-1: 0;
  size_t dec;

  if (DEBUGMEM > 1) err(warnmem,"gauss_pivot_ker. k=%ld, n=%ld",k,n);
  for (u=t+1; u<=m; u++) copyifstack(coeff(x,u,k), coeff(x,u,k));
  for (i=k+1; i<=n; i++)
    for (u=1; u<=m; u++) copyifstack(coeff(x,u,i), coeff(x,u,i));

  (void)gerepile(av,tetpil,NULL); dec = av-tetpil;
  for (u=t+1; u<=m; u++)
  {
    A=(pari_sp)coeff(x,u,k);
    if (A<av && A>=bot) coeff(x,u,k)+=dec;
  }
  for (i=k+1; i<=n; i++)
    for (u=1; u<=m; u++)
    {
      A=(pari_sp)coeff(x,u,i);
      if (A<av && A>=bot) coeff(x,u,i)+=dec;
    }
}

static void
gerepile_gauss_FpM_ker(GEN x, GEN p, long k, long t, pari_sp av)
{
  pari_sp tetpil = avma, A;
  long u,i, n = lg(x)-1, m = n? lg(x[1])-1: 0;
  size_t dec;

  if (DEBUGMEM > 1) err(warnmem,"gauss_pivot_ker. k=%ld, n=%ld",k,n);
  for (u=t+1; u<=m; u++)
    if (isonstack(coeff(x,u,k))) coeff(x,u,k) = lmodii(gcoeff(x,u,k),p);
  for (i=k+1; i<=n; i++)
    for (u=1; u<=m; u++)
      if (isonstack(coeff(x,u,i))) coeff(x,u,i) = lmodii(gcoeff(x,u,i),p);

  (void)gerepile(av,tetpil,NULL); dec = av-tetpil;
  for (u=t+1; u<=m; u++)
  {
    A=(pari_sp)coeff(x,u,k);
    if (A<av && A>=bot) coeff(x,u,k)+=dec;
  }
  for (i=k+1; i<=n; i++)
    for (u=1; u<=m; u++)
    {
      A=(pari_sp)coeff(x,u,i);
      if (A<av && A>=bot) coeff(x,u,i)+=dec;
    }
}

/* special gerepile for huge matrices */

static void
gerepile_gauss(GEN x,long k,long t,pari_sp av, long j, GEN c)
{
  pari_sp tetpil = avma, A;
  long u,i, n = lg(x)-1, m = n? lg(x[1])-1: 0;
  size_t dec;

  if (DEBUGMEM > 1) err(warnmem,"gauss_pivot. k=%ld, n=%ld",k,n);
  for (u=t+1; u<=m; u++)
    if (u==j || !c[u]) copyifstack(coeff(x,u,k), coeff(x,u,k));
  for (u=1; u<=m; u++)
    if (u==j || !c[u])
      for (i=k+1; i<=n; i++) copyifstack(coeff(x,u,i), coeff(x,u,i));

  (void)gerepile(av,tetpil,NULL); dec = av-tetpil;
  for (u=t+1; u<=m; u++)
    if (u==j || !c[u])
    {
      A=(pari_sp)coeff(x,u,k);
      if (A<av && A>=bot) coeff(x,u,k)+=dec;
    }
  for (u=1; u<=m; u++)
    if (u==j || !c[u])
      for (i=k+1; i<=n; i++)
      {
        A=(pari_sp)coeff(x,u,i);
        if (A<av && A>=bot) coeff(x,u,i)+=dec;
      }
}

/*******************************************************************/
/*                                                                 */
/*                    KERNEL of an m x n matrix                    */
/*          return n - rk(x) linearly independent vectors          */
/*                                                                 */
/*******************************************************************/
static long
gauss_get_pivot_NZ(GEN x, GEN x0/*unused*/, GEN c, long i0)
{
  long i,lx = lg(x);
  (void)x0;
  if (c)
    for (i=i0; i<lx; i++)
    {
      if (c[i]) continue; /* already a pivot in line i */
      if (!gcmp0((GEN)x[i])) break;
    }
  else
    for (i=i0; i<lx; i++)
      if (!gcmp0((GEN)x[i])) break;
  return i;

}

/* x ~ 0 compared to reference y */
int
approx_0(GEN x, GEN y)
{
  long tx = typ(x);
  if (tx == t_COMPLEX)
    return approx_0((GEN)x[1], y) && approx_0((GEN)x[2], y);
  return gcmp0(x) ||
         (tx == t_REAL && gexpo(y) - gexpo(x) > bit_accuracy(lg(x)));
}

static long
gauss_get_pivot_max(GEN x, GEN x0, GEN c, long i0)
{
  long i,e, k = i0, ex = - (long)HIGHEXPOBIT, lx = lg(x);
  GEN p,r;
  if (c)
    for (i=i0; i<lx; i++)
    {
      if (c[i]) continue;
      e = gexpo((GEN)x[i]);
      if (e > ex) { ex=e; k=i; }
    }
  else
    for (i=i0; i<lx; i++)
    {
      e = gexpo((GEN)x[i]);
      if (e > ex) { ex=e; k=i; }
    }
  p = (GEN)x[k];
  r = (GEN)x0[k]; if (isexactzero(r)) r = x0;
  return approx_0(p, r)? i: k;
}

/* x has INTEGER coefficients */
GEN
keri(GEN x)
{
  pari_sp av, av0, tetpil, lim;
  GEN c,l,y,p,pp;
  long i,j,k,r,t,n,m;

  if (typ(x)!=t_MAT) err(typeer,"keri");
  n=lg(x)-1; if (!n) return cgetg(1,t_MAT);

  av0=avma; m=lg(x[1])-1; r=0;
  pp=cgetg(n+1,t_COL);
  x=dummycopy(x); p=gun;
  c=cgetg(m+1, t_VECSMALL); for (k=1; k<=m; k++) c[k]=0;
  l=cgetg(n+1, t_VECSMALL);
  av = avma; lim = stack_lim(av,1);
  for (k=1; k<=n; k++)
  {
    j = 1;
    while ( j <= m && (c[j] || !signe(gcoeff(x,j,k))) ) j++;
    if (j > m)
    {
      r++; l[k] = 0;
      for(j=1; j<k; j++)
	if (l[j]) coeff(x,l[j],k) = lclone(gcoeff(x,l[j],k));
      pp[k]=lclone(p);
    }
    else
    {
      GEN p0 = p;
      c[j]=k; l[k]=j; p = gcoeff(x,j,k);

      for (t=1; t<=m; t++)
	if (t!=j)
	{
	  GEN q=gcoeff(x,t,k), p1;
	  for (i=k+1; i<=n; i++)
	  {
	    pari_sp av1 = avma;
	    p1 = subii(mulii(p,gcoeff(x,t,i)), mulii(q,gcoeff(x,j,i)));
	    coeff(x,t,i) = lpileuptoint(av1, diviiexact(p1,p0));
	  }
	  if (low_stack(lim, stack_lim(av,1)))
          {
            GEN _p0 = gclone(p0);
            GEN _p  = gclone(p);
            gerepile_gauss_ker(x,k,t,av);
            p = icopy(_p);  gunclone(_p);
            p0= icopy(_p0); gunclone(_p0);
          }
	}
    }
  }
  if (!r) { avma=av0; y=cgetg(1,t_MAT); return y; }

  /* non trivial kernel */
  tetpil=avma; y=cgetg(r+1,t_MAT);
  for (j=k=1; j<=r; j++,k++)
  {
    p = cgetg(n+1, t_COL);
    y[j]=(long)p; while (l[k]) k++;
    for (i=1; i<k; i++)
      if (l[i])
      {
	c=gcoeff(x,l[i],k);
	p[i] = (long) forcecopy(c); gunclone(c);
      }
      else
	p[i] = zero;
    p[k]=lnegi((GEN)pp[k]); gunclone((GEN)pp[k]);
    for (i=k+1; i<=n; i++) p[i]=zero;
  }
  return gerepile(av0,tetpil,y);
}

GEN
deplin(GEN x0)
{
  pari_sp av = avma;
  long i,j,k,t,nc,nl;
  GEN x,y,piv,q,c,l,*d,*ck,*cj;

  t = typ(x0);
  if (t == t_MAT) x = dummycopy(x0);
  else
  {
    if (t != t_VEC) err(typeer,"deplin");
    x = gtomat(x0);
  }
  nc = lg(x)-1; if (!nc) err(talker,"empty matrix in deplin");
  nl = lg(x[1])-1;
  d = (GEN*)cgetg(nl+1,t_VEC); /* pivot list */
  c = cgetg(nl+1, t_VECSMALL);
  l = cgetg(nc+1, t_VECSMALL); /* not initialized */
  for (i=1; i<=nl; i++) { d[i] = gun; c[i] = 0; }
  ck = NULL; /* gcc -Wall */
  for (k=1; k<=nc; k++)
  {
    ck = (GEN*)x[k];
    for (j=1; j<k; j++)
    {
      cj = (GEN*)x[j]; piv = d[j]; q = gneg(ck[l[j]]);
      for (i=1; i<=nl; i++)
        if (i != l[j]) ck[i] = gadd(gmul(piv, ck[i]), gmul(q, cj[i]));
    }

    i = gauss_get_pivot_NZ((GEN)ck, NULL, c, 1);
    if (i > nl) break;

    d[k] = ck[i];
    c[i] = k; l[k] = i; /* pivot d[k] in x[i,k] */
  }
  if (k > nc) { avma = av; return zerocol(nc); }
  if (k == 1) { avma = av; return gscalcol_i(gun,nc); }
  y = cgetg(nc+1,t_COL);
  y[1] = (long)ck[ l[1] ];
  for (q=d[1],j=2; j<k; j++)
  {
    y[j] = lmul(ck[ l[j] ], q);
    q = gmul(q, d[j]);
  }
  y[j] = lneg(q);
  for (j++; j<=nc; j++) y[j] = zero;
  return gerepileupto(av, gdiv(y,content(y)));
}

/*******************************************************************/
/*                                                                 */
/*         GAUSS REDUCTION OF MATRICES  (m lines x n cols)         */
/*           (kernel, image, complementary image, rank)            */
/*                                                                 */
/*******************************************************************/
/* return the transform of x under a standard Gauss pivot. r = dim ker(x).
 * d[k] contains the index of the first non-zero pivot in column k
 * If a != NULL, use x - a Id instead (for eigen)
 */
static GEN
gauss_pivot_ker(GEN x0, GEN a, GEN *dd, long *rr)
{
  GEN x,c,d,p,mun;
  pari_sp av, lim;
  long i,j,k,r,t,n,m;
  long (*get_pivot)(GEN,GEN,GEN,long);

  if (typ(x0)!=t_MAT) err(typeer,"gauss_pivot");
  n=lg(x0)-1; if (!n) { *dd=NULL; *rr=0; return cgetg(1,t_MAT); }
  m=lg(x0[1])-1; r=0;

  x = dummycopy(x0); mun = negi(gun);
  if (a)
  {
    if (n != m) err(consister,"gauss_pivot_ker");
    for (k=1; k<=n; k++) coeff(x,k,k) = lsub(gcoeff(x,k,k), a);
  }
  get_pivot = use_maximal_pivot(x)? &gauss_get_pivot_max: &gauss_get_pivot_NZ;
  c=cgetg(m+1,t_VECSMALL); for (k=1; k<=m; k++) c[k]=0;
  d=cgetg(n+1,t_VECSMALL);
  av=avma; lim=stack_lim(av,1);
  for (k=1; k<=n; k++)
  {
    j = get_pivot((GEN)x[k], (GEN)x0[k], c, 1);
    if (j > m)
    {
      r++; d[k]=0;
      for(j=1; j<k; j++)
        if (d[j]) coeff(x,d[j],k) = lclone(gcoeff(x,d[j],k));
    }
    else
    { /* pivot for column k on row j */
      c[j]=k; d[k]=j; p = gdiv(mun,gcoeff(x,j,k));
      coeff(x,j,k) = (long)mun;
      /* x[j,] /= - x[j,k] */
      for (i=k+1; i<=n; i++)
	coeff(x,j,i) = lmul(p,gcoeff(x,j,i));
      for (t=1; t<=m; t++)
	if (t!=j)
	{ /* x[t,] -= 1 / x[j,k] x[j,] */
	  p=gcoeff(x,t,k); coeff(x,t,k)=zero;
	  for (i=k+1; i<=n; i++)
	    coeff(x,t,i) = ladd(gcoeff(x,t,i),gmul(p,gcoeff(x,j,i)));
          if (low_stack(lim, stack_lim(av,1)))
            gerepile_gauss_ker(x,k,t,av);
	}
    }
  }
  *dd=d; *rr=r; return x;
}

/* r = dim ker(x).
 * d[k] contains the index of the first non-zero pivot in column k
 */
static void
gauss_pivot(GEN x0, GEN *dd, long *rr)
{
  GEN x,c,d,d0,mun,p;
  long i, j, k, r, t, n, m;
  pari_sp av, lim;
  long (*get_pivot)(GEN,GEN,GEN,long);

  if (typ(x0)!=t_MAT) err(typeer,"gauss_pivot");
  n=lg(x0)-1; if (!n) { *dd=NULL; *rr=0; return; }

  d0 = cgetg(n+1, t_VECSMALL);
  if (use_maximal_pivot(x0))
  { /* put exact columns first, then largest inexact ones */
    get_pivot = gauss_get_pivot_max;
    for (k=1; k<=n; k++)
      d0[k] = isinexactreal((GEN)x0[k])? -gexpo((GEN)x0[k]): -VERYBIGINT;
    d0 = gen_sort(d0, cmp_C|cmp_IND, NULL);
    x0 = vecextract_p(x0, d0);
  }
  else
  {
    get_pivot = gauss_get_pivot_NZ;
    for (k=1; k<=n; k++) d0[k] = k;
  }
  x = dummycopy(x0); mun = negi(gun);
  m=lg(x[1])-1; r=0;
  c=cgetg(m+1, t_VECSMALL); for (k=1; k<=m; k++) c[k]=0;
  d=(GEN)gpmalloc((n+1)*sizeof(long)); av=avma; lim=stack_lim(av,1);
  for (k=1; k<=n; k++)
  {
    j = get_pivot((GEN)x[k], (GEN)x0[k], c, 1);
    if (j>m) { r++; d[d0[k]]=0; }
    else
    {
      c[j]=k; d[d0[k]]=j; p = gdiv(mun,gcoeff(x,j,k));
      for (i=k+1; i<=n; i++)
	coeff(x,j,i) = lmul(p,gcoeff(x,j,i));

      for (t=1; t<=m; t++)
        if (!c[t]) /* no pivot on that line yet */
        {
          p=gcoeff(x,t,k); coeff(x,t,k)=zero;
          for (i=k+1; i<=n; i++)
            coeff(x,t,i) = ladd(gcoeff(x,t,i), gmul(p,gcoeff(x,j,i)));
          if (low_stack(lim, stack_lim(av,1)))
            gerepile_gauss(x,k,t,av,j,c);
        }
      for (i=k; i<=n; i++) coeff(x,j,i) = zero; /* dummy */
    }
  }
  *dd=d; *rr=r;
}

/* compute ker(x - aI) */
static GEN
ker0(GEN x, GEN a)
{
  pari_sp av = avma, tetpil;
  GEN d,y;
  long i,j,k,r,n;

  x = gauss_pivot_ker(x,a,&d,&r);
  if (!r) { avma=av; return cgetg(1,t_MAT); }
  n = lg(x)-1; tetpil=avma; y=cgetg(r+1,t_MAT);
  for (j=k=1; j<=r; j++,k++)
  {
    GEN p = cgetg(n+1,t_COL);

    y[j]=(long)p; while (d[k]) k++;
    for (i=1; i<k; i++)
      if (d[i])
      {
	GEN p1=gcoeff(x,d[i],k);
	p[i] = (long)forcecopy(p1); gunclone(p1);
      }
      else
	p[i] = zero;
    p[k]=un; for (i=k+1; i<=n; i++) p[i]=zero;
  }
  return gerepile(av,tetpil,y);
}

GEN
ker(GEN x) /* Programme pour types exacts */
{
  return ker0(x,NULL);
}

GEN
matker0(GEN x,long flag)
{
  return flag? keri(x): ker(x);
}

GEN
image(GEN x)
{
  pari_sp av = avma;
  GEN d,y;
  long j,k,r;

  gauss_pivot(x,&d,&r);

  /* r = dim ker(x) */
  if (!r) { avma=av; if (d) free(d); return gcopy(x); }

  /* r = dim Im(x) */
  r = lg(x)-1 - r; avma=av;
  y=cgetg(r+1,t_MAT);
  for (j=k=1; j<=r; k++)
    if (d[k]) y[j++] = lcopy((GEN)x[k]);
  free(d); return y;
}

GEN
imagecompl(GEN x)
{
  pari_sp av = avma;
  GEN d,y;
  long j,i,r;

  gauss_pivot(x,&d,&r);
  avma=av; y=cgetg(r+1,t_VEC);
  for (i=j=1; j<=r; i++)
    if (!d[i]) y[j++]=lstoi(i);
  if (d) free(d); return y;
}

/* for hnfspec: imagecompl(trans(x)) + image(trans(x)) */
GEN
imagecomplspec(GEN x, long *nlze)
{
  pari_sp av = avma;
  GEN d,y;
  long i,j,k,l,r;

  x = gtrans_i(x); l = lg(x);
  gauss_pivot(x,&d,&r);
  avma=av; y = cgetg(l,t_VECSMALL);
  for (i=j=1, k=r+1; i<l; i++)
    if (d[i]) y[k++]=i; else y[j++]=i;
  *nlze = r;
  if (d) free(d); return y;
}

static GEN
sinverseimage(GEN mat, GEN y)
{
  pari_sp av=avma,tetpil;
  long i, nbcol = lg(mat);
  GEN col,p1 = cgetg(nbcol+1,t_MAT);

  if (nbcol==1) return NULL;
  if (lg(y) != lg(mat[1])) err(consister,"inverseimage");

  p1[nbcol] = (long)y;
  for (i=1; i<nbcol; i++) p1[i]=mat[i];
  p1 = ker(p1); i=lg(p1)-1;
  if (!i) return NULL;

  col = (GEN)p1[i]; p1 = (GEN) col[nbcol];
  if (gcmp0(p1)) return NULL;

  p1 = gneg_i(p1); setlg(col,nbcol); tetpil=avma;
  return gerepile(av,tetpil, gdiv(col, p1));
}

/* Calcule l'image reciproque de v par m */
GEN
inverseimage(GEN m,GEN v)
{
  pari_sp av=avma;
  long j,lv,tv=typ(v);
  GEN y,p1;

  if (typ(m)!=t_MAT) err(typeer,"inverseimage");
  if (tv==t_COL)
  {
    p1 = sinverseimage(m,v);
    if (p1) return p1;
    avma = av; return cgetg(1,t_MAT);
  }
  if (tv!=t_MAT) err(typeer,"inverseimage");

  lv=lg(v)-1; y=cgetg(lv+1,t_MAT);
  for (j=1; j<=lv; j++)
  {
    p1 = sinverseimage(m,(GEN)v[j]);
    if (!p1) { avma = av; return cgetg(1,t_MAT); }
    y[j] = (long)p1;
  }
  return y;
}

/* i-th vector in the standard basis */
GEN
vec_ei(long n, long i) { GEN e = zerocol(n); e[i] = un; return e; }
GEN
vec_Cei(long n, long i, GEN c) { GEN e = zerocol(n); e[i] = (long)c; return e; }

/* NB: d freed */
static GEN
get_suppl(GEN x, GEN d, long r)
{
  pari_sp av;
  GEN y,c;
  long j,k,rx,n;

  rx = lg(x)-1;
  if (!rx) err(talker,"empty matrix in suppl");
  n = lg(x[1])-1;
  if (rx == n && r == 0) { free(d); return gcopy(x); }
  y = cgetg(n+1, t_MAT);
  av = avma;
  c = vecsmall_const(n,0);
  k = 1;
  /* c = lines containing pivots (could get it from gauss_pivot, but cheap)
   * In theory r = 0 and d[j] > 0 for all j, but why take chances? */
  for (j=1; j<=rx; j++)
    if (d[j])
    {
      c[ d[j] ] = 1;
      y[k++] = x[j];
    }
  for (j=1; j<=n; j++)
    if (!c[j]) y[k++] = j;
  avma = av;

  rx -= r;
  for (j=1; j<=rx; j++)
    y[j] = lcopy((GEN)y[j]);
  for (   ; j<=n; j++)
    y[j] = (long)vec_ei(n, y[j]);
  free(d); return y;
}

/* x is an n x k matrix, rank(x) = k <= n. Return an invertible n x n matrix
 * whose first k columns are given by x. If rank(x) < k, undefined result. */
GEN
suppl(GEN x)
{
  pari_sp av = avma;
  GEN d;
  long r;

  gauss_pivot(x,&d,&r);
  avma = av; return get_suppl(x,d,r);
}

static void FpM_gauss_pivot(GEN x, GEN p, GEN *dd, long *rr);
static void FqM_gauss_pivot(GEN x, GEN T, GEN p, GEN *dd, long *rr);

GEN
FpM_suppl(GEN x, GEN p)
{
  pari_sp av = avma;
  GEN d;
  long r;

  FpM_gauss_pivot(x,p, &d,&r);
  avma = av; return get_suppl(x,d,r);
}
GEN
FqM_suppl(GEN x, GEN T, GEN p)
{
  pari_sp av = avma;
  GEN d;
  long r;

  if (!T) return FpM_suppl(x,p);
  FqM_gauss_pivot(x,T,p, &d,&r);
  avma = av; return get_suppl(x,d,r);
}

GEN
image2(GEN x)
{
  pari_sp av=avma,tetpil;
  long k,n,i;
  GEN p1,p2;

  if (typ(x)!=t_MAT) err(typeer,"image2");
  k=lg(x)-1; if (!k) return gcopy(x);
  n=lg(x[1])-1; p1=ker(x); k=lg(p1)-1;
  if (k) { p1=suppl(p1); n=lg(p1)-1; }
  else p1=idmat(n);

  tetpil=avma; p2=cgetg(n-k+1,t_MAT);
  for (i=k+1; i<=n; i++) p2[i-k]=lmul(x,(GEN) p1[i]);
  return gerepile(av,tetpil,p2);
}

GEN
matimage0(GEN x,long flag)
{
  switch(flag)
  {
    case 0: return image(x);
    case 1: return image2(x);
    default: err(flagerr,"matimage");
  }
  return NULL; /* not reached */
}

long
rank(GEN x)
{
  pari_sp av = avma;
  long r;
  GEN d;

  gauss_pivot(x,&d,&r);
  /* yield r = dim ker(x) */

  avma=av; if (d) free(d);
  return lg(x)-1 - r;
}

/* if p != NULL, assume x integral and compute rank over Fp */
static GEN
indexrank0(GEN x, GEN p, int vecsmall)
{
  pari_sp av = avma;
  long i,j,n,r;
  GEN res,d,p1,p2;

  /* yield r = dim ker(x) */
  FpM_gauss_pivot(x,p,&d,&r);

  /* now r = dim Im(x) */
  n = lg(x)-1; r = n - r;

  avma=av; res=cgetg(3,t_VEC);
  p1 = cgetg(r+1,vecsmall? t_VECSMALL: t_VEC); res[1] = (long)p1;
  p2 = cgetg(r+1,vecsmall? t_VECSMALL: t_VEC); res[2] = (long)p2;
  if (d)
  {
    for (i=0,j=1; j<=n; j++)
      if (d[j]) { i++; p1[i] = d[j]; p2[i] = j; }
    free(d);
    qsort(p1+1, (size_t)r, sizeof(long), (QSCOMP)pari_compare_long);
  }
  if (!vecsmall)
    for (i=1;i<=r;i++) { p1[i]=lstoi(p1[i]); p2[i]=lstoi(p2[i]); }
  return res;
}

GEN
indexrank(GEN x) { return indexrank0(x,NULL,0); }

GEN
sindexrank(GEN x) { return indexrank0(x,NULL,1); }

GEN
FpM_sindexrank(GEN x, GEN p) { return indexrank0(x,p,1); }

/*******************************************************************/
/*                                                                 */
/*                    LINEAR ALGEBRA MODULO P                      */
/*                                                                 */
/*******************************************************************/

/*If p is NULL no reduction is performed.*/
GEN
FpM_mul(GEN x, GEN y, GEN p)
{
  long i,j,l,lx=lg(x), ly=lg(y);
  GEN z;
  if (ly==1) return cgetg(1,t_MAT);
  if (lx != lg(y[1])) err(operi,"* [mod p]",x,y);
  z=cgetg(ly,t_MAT);
  if (lx==1)
  {
    for (i=1; i<ly; i++) z[i]=lgetg(1,t_COL);
    return z;
  }
  l=lg(x[1]);
  for (j=1; j<ly; j++)
  {
    z[j] = lgetg(l,t_COL);
    for (i=1; i<l; i++)
    {
      pari_sp av;
      GEN p1,p2;
      long k;
      p1=gzero; av=avma;
      for (k=1; k<lx; k++)
      {
	p2=mulii(gcoeff(x,i,k),gcoeff(y,k,j));
	p1=addii(p1,p2);
      }
      coeff(z,i,j)=lpileupto(av,p?modii(p1,p):p1);
    }
  }
  return z;
}

/*If p is NULL no reduction is performed.*/
/*Multiple a column vector by a line vector to make a matrix*/
GEN
FpV_FpL_mul(GEN x, GEN y, GEN p)
{
  long i,j, lx=lg(x), ly=lg(y);
  GEN z;
  if (ly==1) return cgetg(1,t_MAT);
  z=cgetg(ly,t_MAT);
  for (j=1; j<ly; j++)
  {
    z[j] = lgetg(lx,t_COL);
    for (i=1; i<lx; i++)
    {
      pari_sp av=avma;
      GEN p1=mulii((GEN)x[i],(GEN)y[j]);
      coeff(z,i,j)=p?lpileupto(av,modii(p1,p)):(long)p1;
    }
  }
  return z;
}


/*If p is NULL no reduction is performed.*/
GEN
FpM_FpV_mul(GEN x, GEN y, GEN p)
{
  long i,k,l,lx=lg(x), ly=lg(y);
  GEN z;
  if (lx != ly) err(operi,"* [mod p]",x,y);
  if (lx==1) return cgetg(1,t_COL);
  l = lg(x[1]);
  z = cgetg(l,t_COL);
  for (i=1; i<l; i++)
  {
    pari_sp av = avma;
    GEN p1 = gzero;
    for (k=1; k<lx; k++)
      p1 = addii(p1, mulii(gcoeff(x,i,k),(GEN)y[k]));
    z[i] = lpileupto(av,p?modii(p1,p):p1);
  }
  return z;
}

GEN
Flm_Flv_mul(GEN x, GEN y, ulong p)
{
  long i,k,l,lx=lg(x), ly=lg(y);
  GEN z;
  if (lx != ly) err(operi,"* [mod p]",x,y);
  if (lx==1) return cgetg(1,t_VECSMALL);
  l = lg(x[1]);
  z = cgetg(l,t_VECSMALL);
  if (u_OK_ULONG(p))
  {
    for (i=1; i<l; i++)
    {
      ulong p1 = 0;
      for (k=1; k<lx; k++)
      {
        p1 += coeff(x,i,k) * y[k];
        if (p1 & HIGHBIT) p1 %= p;
      }
      z[i] = p1 % p;
    }
  }
  else
  {
    for (i=1; i<l; i++)
    {
      ulong p1 = 0;
      for (k=1; k<lx; k++)
      {
        ulong t = muluumod(coeff(x,i,k), y[k], p);
        p1 = adduumod(p1, t, p);
      }
      z[i] = p1;
    }
  }
  return z;
}

/* in place, destroy x */
GEN
Flm_ker_sp(GEN x, ulong p, long deplin)
{
  GEN y, c, d;
  long i, j, k, r, t, m, n;
  ulong a, piv;
  const int OK_ulong = u_OK_ULONG(p);

  n = lg(x)-1;
  m=lg(x[1])-1; r=0;

  c = new_chunk(m+1); for (k=1; k<=m; k++) c[k] = 0;
  d = new_chunk(n+1);
  a = 0; /* for gcc -Wall */
  for (k=1; k<=n; k++)
  {
    for (j=1; j<=m; j++)
      if (!c[j])
      {
        a = ucoeff(x,j,k) % p;
        if (a) break;
      }
    if (j > m)
    {
      if (deplin) {
        c = cgetg(n+1, t_VECSMALL);
        for (i=1; i<k; i++) c[i] = coeff(x,d[i],k) % p;
        c[k] = 1; for (i=k+1; i<=n; i++) c[i] = 0;
        return c;
      }
      r++; d[k] = 0;
    }
    else
    {
      c[j] = k; d[k] = j;
      piv = p - invumod(a, p); /* -1/a */
      ucoeff(x,j,k) = p-1;
      if (piv == 1) { /* nothing */ }
      else if (OK_ulong)
        for (i=k+1; i<=n; i++)
          ucoeff(x,j,i) = (piv * ucoeff(x,j,i)) % p;
      else
        for (i=k+1; i<=n; i++)
          ucoeff(x,j,i) = muluumod(piv, ucoeff(x,j,i), p);
      for (t=1; t<=m; t++)
      {
	if (t == j) continue;

        piv = ( ucoeff(x,t,k) %= p );
        if (!piv) continue;

        if (piv == 1)
          for (i=k+1; i<=n; i++) _u_Fp_add((uGEN)x[i],t,j,p);
        else if (OK_ulong)
          for (i=k+1; i<=n; i++) _u_Fp_addmul_OK((uGEN)x[i],t,j,piv,p);
        else
          for (i=k+1; i<=n; i++) _u_Fp_addmul((uGEN)x[i],t,j,piv,p);
      }
    }
  }
  if (deplin) return NULL;

  y = cgetg(r+1, t_MAT);
  for (j=k=1; j<=r; j++,k++)
  {
    GEN C = cgetg(n+1, t_VECSMALL);

    y[j] = (long)C; while (d[k]) k++;
    for (i=1; i<k; i++)
      if (d[i])
        C[i] = (long)(ucoeff(x,d[i],k) % p);
      else
	C[i] = 0;
    C[k] = 1; for (i=k+1; i<=n; i++) C[i] = 0;
  }
  return y;
}

/* assume x has integer entries */
static GEN
FpM_ker_i(GEN x, GEN p, long deplin)
{
  pari_sp av0 = avma, av, lim, tetpil;
  GEN y, c, d, piv, mun;
  long i, j, k, r, t, n, m;

  if (typ(x)!=t_MAT) err(typeer,"FpM_ker");
  n=lg(x)-1; if (!n) return cgetg(1,t_MAT);
  if (lgefint(p) == 3)
  {
    ulong pp = (ulong)p[2];
    y = ZM_Flm(x, pp);
    y = Flm_ker_sp(y, pp, deplin);
    if (!y) return y;
    y = deplin? zv_ZC(y): zm_ZM(y);
    return gerepileupto(av0, y);
  }

  m=lg(x[1])-1; r=0;
  x=dummycopy(x); mun=negi(gun);
  c=new_chunk(m+1); for (k=1; k<=m; k++) c[k]=0;
  d=new_chunk(n+1);
  av=avma; lim=stack_lim(av,1);
  for (k=1; k<=n; k++)
  {
    for (j=1; j<=m; j++)
      if (!c[j])
      {
        coeff(x,j,k) = lmodii(gcoeff(x,j,k), p);
        if (signe(coeff(x,j,k))) break;
      }
    if (j>m)
    {
      if (deplin) {
        c = cgetg(n+1, t_COL);
        for (i=1; i<k; i++) c[i] = lmodii(gcoeff(x,d[i],k), p);
        c[k] = un; for (i=k+1; i<=n; i++) c[i] = zero;
        return gerepileupto(av0, c);
      }
      r++; d[k]=0;
      for(j=1; j<k; j++)
        if (d[j]) coeff(x,d[j],k) = lclone(gcoeff(x,d[j],k));
    }
    else
    {
      c[j]=k; d[k]=j; piv = negi(mpinvmod(gcoeff(x,j,k), p));
      coeff(x,j,k) = (long)mun;
      for (i=k+1; i<=n; i++)
	coeff(x,j,i) = lmodii(mulii(piv,gcoeff(x,j,i)), p);
      for (t=1; t<=m; t++)
      {
	if (t==j) continue;

        piv = modii(gcoeff(x,t,k), p);
        if (!signe(piv)) continue;

        coeff(x,t,k)=zero;
        for (i=k+1; i<=n; i++)
          coeff(x,t,i) = laddii(gcoeff(x,t,i),mulii(piv,gcoeff(x,j,i)));
        if (low_stack(lim, stack_lim(av,1)))
          gerepile_gauss_FpM_ker(x,p,k,t,av);
      }
    }
  }
  if (deplin) { avma = av0; return NULL; }

  tetpil=avma; y=cgetg(r+1,t_MAT);
  for (j=k=1; j<=r; j++,k++)
  {
    GEN C = cgetg(n+1,t_COL);

    y[j]=(long)C; while (d[k]) k++;
    for (i=1; i<k; i++)
      if (d[i])
      {
	GEN p1=gcoeff(x,d[i],k);
	C[i] = lmodii(p1, p); gunclone(p1);
      }
      else
	C[i] = zero;
    C[k]=un; for (i=k+1; i<=n; i++) C[i]=zero;
  }
  return gerepile(av0,tetpil,y);
}

GEN
FpM_intersect(GEN x, GEN y, GEN p)
{
  pari_sp av = avma;
  long j, lx = lg(x);
  GEN z;

  if (lx==1 || lg(y)==1) return cgetg(1,t_MAT);
  z = FpM_ker(concatsp(x,y), p);
  for (j=lg(z)-1; j; j--) setlg(z[j],lx);
  return gerepileupto(av, FpM_mul(x,z,p));
}

GEN
FpM_ker(GEN x, GEN p) { return FpM_ker_i(x, p, 0); }
GEN
FpM_deplin(GEN x, GEN p) { return FpM_ker_i(x, p, 1); }
/* not memory clean */
GEN
Flm_ker(GEN x, ulong p) { return Flm_ker_sp(gcopy(x), p, 0); }
GEN
Flm_deplin(GEN x, ulong p) { return Flm_ker_sp(gcopy(x), p, 1); }

static void
FpM_gauss_pivot(GEN x, GEN p, GEN *dd, long *rr)
{
  pari_sp av,lim;
  GEN c,d,piv;
  long i,j,k,r,t,n,m;

  if (!p) { gauss_pivot(x,dd,rr); return; }
  if (typ(x)!=t_MAT) err(typeer,"FpM_gauss_pivot");
  n=lg(x)-1; if (!n) { *dd=NULL; *rr=0; return; }

  m=lg(x[1])-1; r=0;
  x=dummycopy(x);
  c=new_chunk(m+1); for (k=1; k<=m; k++) c[k]=0;
  d=(GEN)gpmalloc((n+1)*sizeof(long)); av=avma; lim=stack_lim(av,1);
  for (k=1; k<=n; k++)
  {
    for (j=1; j<=m; j++)
      if (!c[j])
      {
        coeff(x,j,k) = lmodii(gcoeff(x,j,k), p);
        if (signe(coeff(x,j,k))) break;
      }
    if (j>m) { r++; d[k]=0; }
    else
    {
      c[j]=k; d[k]=j; piv = negi(mpinvmod(gcoeff(x,j,k), p));
      for (i=k+1; i<=n; i++)
	coeff(x,j,i) = lmodii(mulii(piv,gcoeff(x,j,i)), p);
      for (t=1; t<=m; t++)
        if (!c[t]) /* no pivot on that line yet */
        {
          piv=gcoeff(x,t,k);
          if (signe(piv))
          {
            coeff(x,t,k)=zero;
            for (i=k+1; i<=n; i++)
              coeff(x,t,i) = laddii(gcoeff(x,t,i), mulii(piv,gcoeff(x,j,i)));
            if (low_stack(lim, stack_lim(av,1)))
              gerepile_gauss(x,k,t,av,j,c);
          }
        }
      for (i=k; i<=n; i++) coeff(x,j,i) = zero; /* dummy */
    }
  }
  *dd=d; *rr=r;
}
static void
FqM_gauss_pivot(GEN x, GEN T, GEN p, GEN *dd, long *rr)
{
  pari_sp av,lim;
  GEN c,d,piv;
  long i,j,k,r,t,n,m;

  if (typ(x)!=t_MAT) err(typeer,"FqM_gauss_pivot");
  n=lg(x)-1; if (!n) { *dd=NULL; *rr=0; return; }

  m=lg(x[1])-1; r=0;
  x=dummycopy(x);
  c=new_chunk(m+1); for (k=1; k<=m; k++) c[k]=0;
  d=(GEN)gpmalloc((n+1)*sizeof(long)); av=avma; lim=stack_lim(av,1);
  for (k=1; k<=n; k++)
  {
    for (j=1; j<=m; j++)
      if (!c[j])
      {
        coeff(x,j,k) = (long)Fq_red(gcoeff(x,j,k), T,p);
        if (signe(coeff(x,j,k))) break;
      }
    if (j>m) { r++; d[k]=0; }
    else
    {
      c[j]=k; d[k]=j; piv = gneg(Fq_inv(gcoeff(x,j,k), T,p));
      for (i=k+1; i<=n; i++)
	coeff(x,j,i) = (long)Fq_mul(piv,gcoeff(x,j,i), T, p);
      for (t=1; t<=m; t++)
        if (!c[t]) /* no pivot on that line yet */
        {
          piv=gcoeff(x,t,k);
          if (signe(piv))
          {
            coeff(x,t,k)=zero;
            for (i=k+1; i<=n; i++)
              coeff(x,t,i) = ladd(gcoeff(x,t,i), gmul(piv,gcoeff(x,j,i)));
            if (low_stack(lim, stack_lim(av,1)))
              gerepile_gauss(x,k,t,av,j,c);
          }
        }
      for (i=k; i<=n; i++) coeff(x,j,i) = zero; /* dummy */
    }
  }
  *dd=d; *rr=r;
}

GEN
FpM_image(GEN x, GEN p)
{
  pari_sp av = avma;
  GEN d,y;
  long j,k,r;

  FpM_gauss_pivot(x,p,&d,&r);

  /* r = dim ker(x) */
  if (!r) { avma=av; if (d) free(d); return gcopy(x); }

  /* r = dim Im(x) */
  r = lg(x)-1 - r; avma=av;
  y=cgetg(r+1,t_MAT);
  for (j=k=1; j<=r; k++)
    if (d[k]) y[j++] = lcopy((GEN)x[k]);
  free(d); return y;
}

long
FpM_rank(GEN x, GEN p)
{
  pari_sp av = avma;
  long r;
  GEN d;

  FpM_gauss_pivot(x,p,&d,&r);
  /* yield r = dim ker(x) */

  avma=av; if (d) free(d);
  return lg(x)-1 - r;
}

static GEN
sFpM_invimage(GEN mat, GEN y, GEN p)
{
  pari_sp av=avma;
  long i, nbcol = lg(mat);
  GEN col,p1 = cgetg(nbcol+1,t_MAT),res;

  if (nbcol==1) return NULL;
  if (lg(y) != lg(mat[1])) err(consister,"FpM_invimage");

  p1[nbcol] = (long)y;
  for (i=1; i<nbcol; i++) p1[i]=mat[i];
  p1 = FpM_ker(p1,p); i=lg(p1)-1;
  if (!i) return NULL;

  col = (GEN)p1[i]; p1 = (GEN) col[nbcol];
  if (gcmp0(p1)) return NULL;

  p1 = mpinvmod(negi(p1),p);
  setlg(col,nbcol);
  for(i=1;i<nbcol;i++)
    col[i]=lmulii((GEN)col[i],p1);
  res=cgetg(nbcol,t_COL);
  for(i=1;i<nbcol;i++)
    res[i]=lmodii((GEN)col[i],p);
  return gerepileupto(av,res);
}

/* Calcule l'image reciproque de v par m */
GEN
FpM_invimage(GEN m, GEN v, GEN p)
{
  pari_sp av=avma;
  long j,lv,tv=typ(v);
  GEN y,p1;

  if (typ(m)!=t_MAT) err(typeer,"inverseimage");
  if (tv==t_COL)
  {
    p1 = sFpM_invimage(m,v,p);
    if (p1) return p1;
    avma = av; return cgetg(1,t_MAT);
  }
  if (tv!=t_MAT) err(typeer,"inverseimage");

  lv=lg(v)-1; y=cgetg(lv+1,t_MAT);
  for (j=1; j<=lv; j++)
  {
    p1 = sFpM_invimage(m,(GEN)v[j],p);
    if (!p1) { avma = av; return cgetg(1,t_MAT); }
    y[j] = (long)p1;
  }
  return y;
}
/**************************************************************
 Rather stupid implementation of linear algebra in finite fields
 **************************************************************/

static void
Fq_gerepile_gauss_ker(GEN x, GEN T, GEN p, long k, long t, pari_sp av)
{
  pari_sp tetpil = avma,A;
  long u,i, n = lg(x)-1, m = n? lg(x[1])-1: 0;
  size_t dec;

  if (DEBUGMEM > 1) err(warnmem,"gauss_pivot_ker. k=%ld, n=%ld",k,n);
  for (u=t+1; u<=m; u++)
    if (isonstack(coeff(x,u,k))) 
      coeff(x,u,k) = (long) Fq_red(gcoeff(x,u,k),T,p);
  for (i=k+1; i<=n; i++)
    for (u=1; u<=m; u++)
      if (isonstack(coeff(x,u,i))) 
        coeff(x,u,i) = (long) Fq_red(gcoeff(x,u,i),T,p);

  (void)gerepile(av,tetpil,NULL); dec = av-tetpil;
  for (u=t+1; u<=m; u++)
  {
    A=(pari_sp)coeff(x,u,k);
    if (A<av && A>=bot) coeff(x,u,k)+=dec;
  }
  for (i=k+1; i<=n; i++)
    for (u=1; u<=m; u++)
    {
      A=(pari_sp)coeff(x,u,i);
      if (A<av && A>=bot) coeff(x,u,i)+=dec;
    }
}

static GEN
FqM_ker_i(GEN x, GEN T, GEN p, long deplin)
{
  pari_sp av0,av,lim,tetpil;
  GEN y,c,d,piv,mun;
  long i,j,k,r,t,n,m;

  if (!T) return FpM_ker_i(x,p,deplin);

  if (typ(x)!=t_MAT) err(typeer,"FpM_ker");
  n=lg(x)-1; if (!n) return cgetg(1,t_MAT);

  m=lg(x[1])-1; r=0; av0 = avma;
  x=dummycopy(x); mun=negi(gun);
  c=new_chunk(m+1); for (k=1; k<=m; k++) c[k]=0;
  d=new_chunk(n+1);
  av=avma; lim=stack_lim(av,1);
  for (k=1; k<=n; k++)
  {
    for (j=1; j<=m; j++)
      if (!c[j])
      {
        coeff(x,j,k) = (long) Fq_red(gcoeff(x,j,k), T, p);
        if (signe(coeff(x,j,k))) break;
      }
    if (j>m)
    {
      if (deplin) {
        c = cgetg(n+1, t_COL);
        for (i=1; i<k; i++) c[i] = (long)Fq_red(gcoeff(x,d[i],k), T, p);
        c[k] = un; for (i=k+1; i<=n; i++) c[i] = zero;
        return gerepileupto(av0, c); 
      }
      r++; d[k]=0;
      for(j=1; j<k; j++)
        if (d[j]) coeff(x,d[j],k) = lclone(gcoeff(x,d[j],k));
    }
    else
    {
      c[j]=k; d[k]=j; piv = Fq_neg_inv(gcoeff(x,j,k), T, p);
      coeff(x,j,k) = (long)mun;
      for (i=k+1; i<=n; i++)
	coeff(x,j,i) = (long) Fq_mul(piv,gcoeff(x,j,i), T, p);
      for (t=1; t<=m; t++)
      {
        if (t==j) continue;

        piv = Fq_red(gcoeff(x,t,k), T, p);
        /*Assume signe work for both t_POL and t_INT*/
        if (!signe(piv)) continue;

        coeff(x,t,k)=zero;
        for (i=k+1; i<=n; i++)
          coeff(x,t,i) = (long)Fq_add(gcoeff(x,t,i),Fq_mul(piv,gcoeff(x,j,i), T, p), T, p);
        if (low_stack(lim, stack_lim(av,1)))
          Fq_gerepile_gauss_ker(x,T,p,k,t,av);
      }
    }
  }
  if (deplin) { avma = av0; return NULL; }

  tetpil=avma; y=cgetg(r+1,t_MAT);
  for (j=k=1; j<=r; j++,k++)
  {
    GEN C = cgetg(n+1,t_COL);

    y[j]=(long)C; while (d[k]) k++;
    for (i=1; i<k; i++)
      if (d[i])
      {
	GEN p1=gcoeff(x,d[i],k);
	C[i] = (long) Fq_red(p1, T, p); gunclone(p1);
      }
      else
	C[i] = zero;
    C[k]=un; for (i=k+1; i<=n; i++) C[i]=zero;
  }
  return gerepile(av0,tetpil,y);
}
GEN
FqM_ker(GEN x, GEN T, GEN p)
{
  return FqM_ker_i(x, T, p, 0);
}

/*******************************************************************/
/*                                                                 */
/*                        EIGENVECTORS                             */
/*   (independent eigenvectors, sorted by increasing eigenvalue)   */
/*                                                                 */
/*******************************************************************/

GEN
eigen(GEN x, long prec)
{
  GEN y,rr,p,ssesp,r1,r2,r3;
  long e,i,k,l,ly,ex, n = lg(x);
  pari_sp av = avma;

  if (typ(x)!=t_MAT) err(typeer,"eigen");
  if (n != 1 && n != lg(x[1])) err(mattype1,"eigen");
  if (n<=2) return gcopy(x);

  ex = 16 - bit_accuracy(prec);
  y=cgetg(n,t_MAT);
  p=caradj(x,0,NULL); rr=roots(p,prec);
  for (i=1; i<n; i++)
  {
    GEN p1 = (GEN)rr[i];
    if (!signe(p1[2]) || gexpo((GEN)p1[2]) - gexpo(p1) < ex) rr[i] = p1[1];
  }
  ly=1; k=2; r2=(GEN)rr[1];
  for(;;)
  {
    r3 = grndtoi(r2, &e); if (e < ex) r2 = r3;
    ssesp = ker0(x,r2); l = lg(ssesp);
    if (l == 1 || ly + (l-1) > n)
      err(talker2, "missing eigenspace. Compute the matrix to higher accuracy, then restart eigen at the current precision",NULL,NULL);
    for (i=1; i<l; ) y[ly++]=ssesp[i++]; /* done with this eigenspace */

    r1=r2; /* try to find a different eigenvalue */
    do
    {
      if (k == n || ly == n)
      {
        setlg(y,ly); /* x may not be diagonalizable */
        return gerepilecopy(av,y);
      }
      r2 = (GEN)rr[k++];
      r3 = gsub(r1,r2);
    }
    while (gcmp0(r3) || gexpo(r3) < ex);
  }
}

/*******************************************************************/
/*                                                                 */
/*                           DETERMINANT                           */
/*                                                                 */
/*******************************************************************/

GEN
det0(GEN a,long flag)
{
  switch(flag)
  {
    case 0: return det(a);
    case 1: return det2(a);
    default: err(flagerr,"matdet");
  }
  return NULL; /* not reached */
}

/* Exact types: choose the first non-zero pivot. Otherwise: maximal pivot */
static GEN
det_simple_gauss(GEN a, int inexact)
{
  pari_sp av, av1;
  long i,j,k,s, nbco = lg(a)-1;
  GEN x,p;

  av=avma; s=1; x=gun; a=dummycopy(a);
  for (i=1; i<nbco; i++)
  {
    p=gcoeff(a,i,i); k=i;
    if (inexact)
    {
      long e, ex = gexpo(p);
      GEN p1;

      for (j=i+1; j<=nbco; j++)
      {
        e = gexpo(gcoeff(a,i,j));
        if (e > ex) { ex=e; k=j; }
      }
      p1 = gcoeff(a,i,k);
      if (gcmp0(p1)) return gerepilecopy(av, p1);
    }
    else if (gcmp0(p))
    {
      do k++; while(k<=nbco && gcmp0(gcoeff(a,i,k)));
      if (k>nbco) return gerepilecopy(av, p);
    }
    if (k != i)
    {
      swap(a[i],a[k]); s = -s;
      p = gcoeff(a,i,i);
    }

    x = gmul(x,p);
    for (k=i+1; k<=nbco; k++)
    {
      GEN m = gcoeff(a,i,k);
      if (!gcmp0(m))
      {
	m = gneg_i(gdiv(m,p));
	for (j=i+1; j<=nbco; j++)
	  coeff(a,j,k) = ladd(gcoeff(a,j,k), gmul(m,gcoeff(a,j,i)));
      }
    }
  }
  if (s<0) x = gneg_i(x);
  av1=avma; return gerepile(av,av1,gmul(x,gcoeff(a,nbco,nbco)));
}

GEN
det2(GEN a)
{
  long nbco = lg(a)-1;
  if (typ(a)!=t_MAT) err(mattype1,"det2");
  if (!nbco) return gun;
  if (nbco != lg(a[1])-1) err(mattype1,"det2");
  return det_simple_gauss(a, use_maximal_pivot(a));
}

static GEN
mydiv(GEN x, GEN y)
{
  long tx = typ(x), ty = typ(y);
  if (tx == ty && tx == t_POL && varn(x) == varn(y))
    return gdeuc(x,y);
  return gdiv(x,y);
}

/* determinant in a ring A: all computations are done within A
 * (Gauss-Bareiss algorithm) */
GEN
det(GEN a)
{
  pari_sp av, lim;
  long nbco = lg(a)-1,i,j,k,s;
  GEN p,pprec;

  if (typ(a)!=t_MAT) err(mattype1,"det");
  if (!nbco) return gun;
  if (nbco != lg(a[1])-1) err(mattype1,"det");
  if (use_maximal_pivot(a)) return det_simple_gauss(a,1);
  if (DEBUGLEVEL > 7) (void)timer2();

  av = avma; lim = stack_lim(av,2);
  a = dummycopy(a); s = 1;
  for (pprec=gun,i=1; i<nbco; i++,pprec=p)
  {
    GEN *ci, *ck, m, p1;
    int diveuc = (gcmp1(pprec)==0);

    p = gcoeff(a,i,i);
    if (gcmp0(p))
    {
      k=i+1; while (k<=nbco && gcmp0(gcoeff(a,i,k))) k++;
      if (k>nbco) return gerepilecopy(av, p);
      swap(a[k], a[i]); s = -s;
      p = gcoeff(a,i,i);
    }
    ci = (GEN*)a[i];
    for (k=i+1; k<=nbco; k++)
    {
      ck = (GEN*)a[k]; m = (GEN)ck[i];
      if (gcmp0(m))
      {
        if (gcmp1(p))
        {
          if (diveuc)
            a[k] = (long)mydiv((GEN)a[k], pprec);
        }
        else
          for (j=i+1; j<=nbco; j++)
          {
            p1 = gmul(p,ck[j]);
            if (diveuc) p1 = mydiv(p1,pprec);
            ck[j] = p1;
          }
      }
      else
      {
        m = gneg_i(m);
        for (j=i+1; j<=nbco; j++)
        {
          p1 = gadd(gmul(p,ck[j]), gmul(m,ci[j]));
          if (diveuc) p1 = mydiv(p1,pprec);
          ck[j] = p1;
        }
      }
      if (low_stack(lim,stack_lim(av,2)))
      {
        GEN *gptr[2]; gptr[0]=&a; gptr[1]=&pprec;
        if(DEBUGMEM>1) err(warnmem,"det. col = %ld",i);
        gerepilemany(av,gptr,2); p = gcoeff(a,i,i); ci = (GEN*)a[i];
      }
    }
    if (DEBUGLEVEL > 7) msgtimer("det, col %ld / %ld",i,nbco-1);
  }
  p = gcoeff(a,nbco,nbco);
  if (s < 0) p = gneg(p); else p = gcopy(p);
  return gerepileupto(av, p);
}

/* return a solution of congruence system sum M_{ i,j } X_j = Y_i mod D_i
 * If ptu1 != NULL, put in *ptu1 a Z-basis of the homogeneous system
 */
static GEN
gaussmoduloall(GEN M, GEN D, GEN Y, GEN *ptu1)
{
  pari_sp av = avma;
  long n,m,i,j,lM;
  GEN p1,delta,H,U,u1,u2,x;

  if (typ(M)!=t_MAT) err(typeer,"gaussmodulo");
  lM = lg(M); m = lM-1;
  if (!m)
  {
    if ((typ(Y)!=t_INT && lg(Y)!=1)
     || (typ(D)!=t_INT && lg(D)!=1)) err(consister,"gaussmodulo");
    return gzero;
  }
  n = lg(M[1])-1;
  switch(typ(D))
  {
    case t_VEC:
    case t_COL: delta=diagonal(D); break;
    case t_INT: delta=gscalmat(D,n); break;
    default: err(typeer,"gaussmodulo");
      return NULL; /* not reached */
  }
  if (typ(Y) == t_INT)
  {
    p1 = cgetg(n+1,t_COL);
    for (i=1; i<=n; i++) p1[i]=(long)Y;
    Y = p1;
  }
  p1 = hnfall(concatsp(M,delta));
  H = (GEN)p1[1]; U = (GEN)p1[2];
  Y = gauss(H,Y);
  if (!gcmp1(denom(Y))) return gzero;
  u1 = cgetg(m+1,t_MAT);
  u2 = cgetg(n+1,t_MAT);
  for (j=1; j<=m; j++)
  {
    p1 = (GEN)U[j]; setlg(p1,lM);
    u1[j] = (long)p1;
  }
  U += m;
  for (j=1; j<=n; j++)
  {
    p1 = (GEN)U[j]; setlg(p1,lM);
    u2[j] = (long)p1;
  }
  x = gmul(u2,Y);
  x = lllreducemodmatrix(x, u1);
  if (!ptu1) x = gerepileupto(av, x);
  else
  {
    gerepileall(av, 2, &x, &u1);
    *ptu1 = u1;
  }
  return x;
}

GEN
matsolvemod0(GEN M, GEN D, GEN Y, long flag)
{
  pari_sp av;
  GEN p1,y;

  if (!flag) return gaussmoduloall(M,D,Y,NULL);

  av=avma; y = cgetg(3,t_VEC);
  p1 = gaussmoduloall(M,D,Y, (GEN*)y+2);
  if (p1==gzero) { avma=av; return gzero; }
  y[1] = (long)p1; return y;
}

GEN
gaussmodulo2(GEN M, GEN D, GEN Y)
{
  return matsolvemod0(M,D,Y,1);
}

GEN
gaussmodulo(GEN M, GEN D, GEN Y)
{
  return matsolvemod0(M,D,Y,0);
}
