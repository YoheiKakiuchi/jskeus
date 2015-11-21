///////////////////////////////////////////////////////////////////////////////
///
/// $Id$
///
/// Copyright (c) 1987- JSK, The University of Tokyo.  All Rights Reserved.
///
/// This software is a collection of EusLisp code for robot applications,
/// which has been developed by the JSK Laboratory for the IRT project.
/// For more information on EusLisp and its application to the robotics,
/// please refer to the following papers.
///
/// Toshihiro Matsui
/// Multithread object-oriented language euslisp for parallel and
///  asynchronous programming in robotics
/// Workshop on Concurrent Object-based Systems,
///  IEEE 6th Symposium on Parallel and Distributed Processing, 1994
///
/// Permission to use this software for educational, research
/// and non-profit purposes, without fee, and without a written
/// agreement is hereby granted to all researchers working on
/// the IRT project at the University of Tokyo, provided that the
/// above copyright notice remains intact.  
///

#include "eus.h"
#include "nr.h"
#include <math.h>
extern pointer ___irtc();
static register_irtc()
{ add_module_initializer("___irtc", ___irtc);}

#define colsize(p) (intval(p->c.ary.dim[1]))
#define rowsize(p) (intval(p->c.ary.dim[0]))
#define ismatrix(p) ((isarray(p) && \
                      p->c.ary.rank==makeint(2) && \
                      elmtypeof(p->c.ary.entity)==ELM_FLOAT))

/*
 *
 */

pointer SV_SOLVE(ctx,n,argv)
register context *ctx;
int n;
pointer argv[];
/* (SV_SOLVE mat vec &optional ret) */
{ 
  pointer a,b,x;
  eusfloat_t **aa, *bb, *xx;
  int i, j, s;

  ckarg2(2,3);
  a=argv[0];  b=argv[1];
  if (!ismatrix(a)) error(E_NOVECTOR);
  s=colsize(a);
  if (!isfltvector(b)) error(E_NOVECTOR);
  if (s!=vecsize(b)) error(E_VECSIZE);
  if (n==3) {
    x=argv[2];
    if (!isvector(x)) error(E_NOVECTOR);
    if (s!=vecsize(x)) error(E_VECSIZE); }
  else  x=(pointer)makefvector(s);

  aa = nr_matrix(1,s,1,s);
  bb = nr_vector(1,s);
  xx = nr_vector(1,s);
  for (i = 0; i < s; i++){
    for (j = 0; j < s; j++){
      aa[j+1][i+1]=a->c.ary.entity->c.fvec.fv[j*s+i];
    }
  }
  for (i = 0; i < s; i++){
    bb[i+1] = b->c.fvec.fv[i];
  }
  if ( svdsolve(aa, s, s, bb, xx) < 0 ) {
    return NIL;
  }
  
  for (i = 0; i < s; i++){
    x->c.fvec.fv[i] = xx[i+1];
  }

  free_nr_matrix(aa,1,s,1,s);
  free_nr_vector(bb,1,s);
  free_nr_vector(xx,1,s);

  return(x);}

pointer SV_DECOMPOSE(ctx,n,argv)
register context *ctx;
int n;
pointer argv[];
/* (SV_DECOMPOSE mat) */
{ 
  pointer a,ru,rv,rw, rr;
  eusfloat_t **u, **v, *w, y;
  int c, r, i, j, *idx, k, pc=0;;

  ckarg(1);
  a=argv[0];
  if (!ismatrix(a)) error(E_NOVECTOR);
  c = colsize(a);
  r = rowsize(a);

  u = nr_matrix(1,r,1,c);
  v = nr_matrix(1,c,1,c);
  w = nr_vector(1,c);
  for (i = 0; i < c; i++){
    for (j = 0; j < r; j++){
      u[j+1][i+1]=a->c.ary.entity->c.fvec.fv[j*c+i];
    }
  }
  if ( svdcmp(u, r, c, w, v) < 0 ) {
    free_nr_matrix(u,1,r,1,c);
    free_nr_matrix(v,1,c,1,c);
    free_nr_vector(w,1,c);
    return NIL;
  }

  ru = makematrix(ctx,r,c); vpush(ru); pc++;
  rw = makefvector(c);      vpush(rw); pc++;
  rv = makematrix(ctx,c,c); vpush(rv); pc++;

  idx = malloc(sizeof(int)*(c+1));

  for (i = 0; i < c; i++){ idx[i+1] = i+1 ;}
  for (i = 0; i < c; i++) {
    for (j = i+1; j < c; j++) {
      if ( w[i+1] < w[j+1] ) {
	SWAP(w[i+1], w[j+1]);
	k = idx[i+1]; idx[i+1] = idx[j+1]; idx[j+1] = k;
      }
    }
  }
  
  for (i = 0; i < c; i++){
    for (j = 0; j < r; j++){
      ru->c.ary.entity->c.fvec.fv[j*c+i] = u[j+1][idx[i+1]];
    }
  }
  for (i = 0; i < c; i++){
    rw->c.fvec.fv[i] = w[i+1];
  }
  for (i = 0; i < c; i++){
    for (j = 0; j < c; j++){
      rv->c.ary.entity->c.fvec.fv[j*c+i] = v[j+1][idx[i+1]];
    }
  }

  free_nr_matrix(u,1,r,1,c);
  free_nr_matrix(v,1,c,1,c);
  free_nr_vector(w,1,c);

  free(idx);
  
  while(pc-->0) vpop();
  return(cons(ctx,ru,cons(ctx,rw,(cons(ctx,rv,NIL)))));}
  
/*
 *
 */

pointer LU_SOLVE2(ctx,n,argv) /* re-definition */
register context *ctx;
int n;
pointer argv[];
/* (LU-SOLVE mat perm bvector [result]) */
{ pointer a,p,b,x;
  int i, j, s;
  eusfloat_t **aa, *cols;
  int *indx;

  ckarg2(3,4);
  a=argv[0];  p=argv[1]; b=argv[2];
  if (!ismatrix(a)) error(E_NOVECTOR);
  s=colsize(a);
  if (!isvector(p) || !isfltvector(b)) error(E_NOVECTOR);
  if (s!=vecsize(p) || s!=vecsize(b)) error(E_VECSIZE);
  if (n==4) {
    x=argv[3];
    if (!isvector(x)) error(E_NOVECTOR);
    if (s!=vecsize(x)) error(E_VECSIZE); }
  else  x=(pointer)makefvector(s);

  aa = nr_matrix(1,s,1,s);
  cols = nr_vector(1,s);
  indx = malloc(sizeof(int)*(s+1));
  for (i=0; i<s; i++)
    for (j=0; j<s; j++)
      aa[i+1][j+1]=a->c.ary.entity->c.fvec.fv[i*s+j];
  for (i=0; i<s; i++) indx[i+1] = intval(p->c.vec.v[i]);
  for (i=0; i<s; i++) cols[i+1] = b->c.fvec.fv[i];
  lubksb(aa,s,indx,cols);
  for (i=0; i<s; i++) x->c.fvec.fv[i] = cols[i+1];
  
  free_nr_matrix(aa,1,s,1,s);
  free_nr_vector(cols,1,s);
  free(indx);

  return(x);}

pointer LU_DECOMPOSE2(ctx,n,argv) /* re-definition */
register context *ctx;
int n;
pointer argv[];
/* (LU-DECOMPOSE mat [result] [tmp-vector]) */
{ pointer a,result,pv;
  eusfloat_t **aa, d;
  int i, j, s, stat, *indx;

  ckarg2(1,3);
  a=argv[0];
  if (!ismatrix(a)) error(E_NOVECTOR);
  s=colsize(a);
  if (s!=rowsize(a)) error(E_VECSIZE);
  if (n==1) result=a;
  else {
    result=argv[1];
    if (!ismatrix(result)) error(E_NOVECTOR);
    if (s!=colsize(result)) error(E_VECSIZE);
    copymat(result,a,s); 
  }
  if (n==3) {
    pv=argv[2];
    if (!isvector(pv)) error(E_NOVECTOR);
    if (s!=vecsize(pv)) error(E_VECSIZE);
  }else{
    pv=makevector(C_VECTOR,s);
  }

  aa = nr_matrix(1,s,1,s);
  indx = malloc(sizeof(int)*(s+1));

  for (i=0; i<s; i++)
    for (j=0; j<s; j++)
      aa[i+1][j+1]=a->c.ary.entity->c.fvec.fv[i*s+j];
  stat = ludcmp(aa, s, indx, &d);
  for (i=0; i<s; i++) pv->c.vec.v[i]=makeint(indx[i+1]);
  for (i=0; i<s; i++)
    for (j=0; j<s; j++)
      result->c.ary.entity->c.fvec.fv[i*s+j] = aa[i+1][j+1];

  free_nr_matrix(aa,1,s,1,s);
  free(indx);

  if (stat<0) return(NIL);
  else return(pv);}

pointer MATRIX_DETERMINANT(ctx,n,argv)
register context *ctx;
int n;
pointer argv[];
{ pointer a,result;
  numunion nu;
  eusfloat_t **aa, d;
  int i, j, s, stat, *indx;

  ckarg2(1,2);
  a=argv[0];
  if (!ismatrix(a)) error(E_NOVECTOR);
  s=colsize(a);
  if (s!=rowsize(a)) error(E_VECSIZE);
  if (n==1) result=a;
  else {
    result=argv[1];
    if (!ismatrix(result)) error(E_NOVECTOR);
    if (s!=colsize(result)) error(E_VECSIZE);
    copymat(result,a,s); 
  }

  aa = nr_matrix(1,s,1,s);
  indx = malloc(sizeof(int)*(s+1));

  for (i=0; i<s; i++)
    for (j=0; j<s; j++)
      aa[i+1][j+1]=a->c.ary.entity->c.fvec.fv[i*s+j];
  stat = ludcmp(aa, s, indx, &d);
  for (i=0; i<s; i++) d = d*aa[i+1][i+1];
  if ( ((-1 * TINY) <= d) && (d <= TINY) ) d = 0.0;

  free_nr_matrix(aa,1,s,1,s);
  free(indx);

  if (stat<0) return(makeflt(0.0));
  else return(makeflt(d));}

pointer PSEUDO_INVERSE2(ctx,n,argv)
register context *ctx;
int n;
pointer argv[];
{ pointer a,result;
  numunion nu;
  eusfloat_t **u, **v, *w, y;
  int c, r, i, j, k, *idx;

  ckarg2(1,2);
  a=argv[0];
  if (!ismatrix(a)) error(E_NOVECTOR);
  c=colsize(a);
  r=rowsize(a);
  if (n==1) {
    result=makematrix(ctx,c,r); vpush(result);
  }else {
    result=argv[1];
    if (!ismatrix(result)) error(E_NOVECTOR);
    if (r!=colsize(result)||c!=rowsize(result)) error(E_VECSIZE);
  }

  u = nr_matrix(1,r,1,c);
  v = nr_matrix(1,c,1,c);
  w = nr_vector(1,c);
  for (i = 0; i < c; i++){
    for (j = 0; j < r; j++){
      u[j+1][i+1]=a->c.ary.entity->c.fvec.fv[j*c+i];
    }
  }
  if ( svdcmp(u, r, c, w, v) < 0 ) {
    nrerror("svdcmp() returns error"); 
    free_nr_matrix(u,1,r,1,c);
    free_nr_matrix(v,1,c,1,c);
    free_nr_vector(w,1,c);
    return NIL;
  }
  idx = malloc(sizeof(int)*(c+1));

  for (i = 0; i < c; i++){ idx[i+1] = i+1 ;}
  for (i = 0; i < c; i++) {
    for (j = i+1; j < c; j++) {
      if ( w[i+1] < w[j+1] ) {
	SWAP(w[i+1], w[j+1]);
	k = idx[i+1]; idx[i+1] = idx[j+1]; idx[j+1] = k;
      }
    }
  }
  
  // A* = v w ut
  for (i=1;i<=c;i++) {
    if (w[i]>0.0001) w[i] = 1.0/w[i];
  }
  for (i=0;i<c;i++) {
    for (j=0;j<r;j++) {
      result->c.ary.entity->c.fvec.fv[(i)*r+(j)]=0.0;
      for (k=0;k<c;k++) {
	result->c.ary.entity->c.fvec.fv[(i)*r+(j)]+=
	  v[(i+1)][idx[(k+1)]]*w[(k+1)]*u[(j+1)][idx[(k+1)]];
      }
    }
  }

  free_nr_matrix(u,1,r,1,c);
  free_nr_matrix(v,1,c,1,c);
  free_nr_vector(w,1,c);

  free(idx);

  vpop(); // vpush(result)
  return(result);}

/*
 *
 */

int matrix2quaternion(eusfloat_t *c, eusfloat_t *q){
  eusfloat_t q02, q12, q22, q32;
  q02 = (1 + c[0*3+0] + c[1*3+1] + c[2*3+2]) / 4;
  q12 = (1 + c[0*3+0] - c[1*3+1] - c[2*3+2]) / 4;
  q22 = (1 - c[0*3+0] + c[1*3+1] - c[2*3+2]) / 4;
  q32 = (1 - c[0*3+0] - c[1*3+1] + c[2*3+2]) / 4;

  if ( (q02 >= q12) && (q02 >= q22) && (q02 >= q32) ) {
    q[0] = sqrt(q02);
    q[1] = (c[2*3+1] - c[1*3+2]) / (4 * q[0]);
    q[2] = (c[0*3+2] - c[2*3+0]) / (4 * q[0]);
    q[3] = (c[1*3+0] - c[0*3+1]) / (4 * q[0]);
  } else if ( (q12 >= q02) && (q12 >= q22) && (q12 >= q32) ) {
    q[1] = sqrt(q12);
    q[0] = (c[2*3+1] - c[1*3+2]) / (4 * q[1]);
    q[2] = (c[0*3+1] + c[1*3+0]) / (4 * q[1]);
    q[3] = (c[0*3+2] + c[2*3+0]) / (4 * q[1]);
  } else if ( (q22 >= q02) && (q22 >= q12) && (q22 >= q32) ) {
    q[2] = sqrt(q22);
    q[0] = (c[0*3+2] - c[2*3+0]) / (4 * q[2]);
    q[1] = (c[0*3+1] + c[1*3+0]) / (4 * q[2]);
    q[3] = (c[1*3+2] + c[2*3+1]) / (4 * q[2]);
  } else if ( (q32 >= q02) && (q32 >= q12) && (q32 >= q22) ) {
    q[3] = sqrt(q32);
    q[0] = (c[1*3+0] - c[0*3+1]) / (4 * q[3]);
    q[1] = (c[0*3+2] + c[2*3+0]) / (4 * q[3]);
    q[2] = (c[1*3+2] + c[2*3+1]) / (4 * q[3]);
  } else {
    fprintf(stderr, ";; matrix2quaternion q02=%f,q12=%f,q22=%f,q32=%f\n",
	    q02,q12,q22,q32);
    error(E_USER,(pointer)";; matrix2quaternion\n");
  }
}

int quaternion2matrix(eusfloat_t *q, eusfloat_t *c){
  eusfloat_t q0 = q[0], q1 = q[1], q2 = q[2], q3 = q[3];
  // (+ (* q0 q0) (* q1 q1) (- (* q2 q2)) (- (* q3 q3)))
  c[0*3+0] = q0*q0 + q1*q1 - q2*q2 - q3*q3;
  // (* 2 (- (* q1 q2) (* q0 q3)))
  c[0*3+1] = 2 * (q1*q2 - q0*q3);
  // (* 2 (+ (* q1 q3) (* q0 q2)))
  c[0*3+2] = 2 * (q1*q3 + q0*q2);
  // (* 2 (+ (* q1 q2) (* q0 q3)))
  c[1*3+0] = 2 * (q1*q2 + q0*q3);
  // (+ (* q0 q0) (- (* q1 q1)) (* q2 q2) (- (* q3 q3)))
  c[1*3+1] = q0*q0 - q1*q1 + q2*q2 - q3*q3;
  // (* 2 (- (* q2 q3) (* q0 q1)))
  c[1*3+2] = 2 * (q2*q3 - q0*q1);
  // (* 2 (- (* q1 q3) (* q0 q2)))
  c[2*3+0] = 2 * (q1*q3 - q0*q2);
  // (* 2 (+ (* q2 q3) (* q0 q1)))
  c[2*3+1] = 2 * (q2*q3 + q0*q1);
  // (+ (* q0 q0) (- (* q1 q1)) (- (* q2 q2)) (* q3 q3))
  c[2*3+2] = q0*q0 - q1*q1 - q2*q2 + q3*q3;
}


int quaternion_multiply(eusfloat_t *q1, eusfloat_t *q2, eusfloat_t *q3){
  eusfloat_t q10 = q1[0], q11 = q1[1], q12 = q1[2], q13 = q1[3];
  eusfloat_t q20 = q2[0], q21 = q2[1], q22 = q2[2], q23 = q2[3];
  // (+ (* q10 q20) (- (* q11 q21)) (- (* q12 q22)) (- (* q13 q23)))
  q3[0] = q10*q20 - q11*q21 - q12*q22 - q13*q23;
  // (+ (* q10 q21)    (* q11 q20)     (* q12 q23)  (- (* q13 q22)))
  q3[1] = q10*q21 + q11*q20 + q12*q23 - q13*q22;
  // (+ (* q10 q22) (- (* q11 q23))    (* q12 q20)     (* q13 q21))
  q3[2] = q10*q22 - q11*q23 + q12*q20 + q13*q21;
  // (+ (* q10 q23)    (* q11 q22)  (- (* q12 q21))    (* q13 q20))
  q3[3] = q10*q23 + q11*q22 - q12*q21 + q13*q20;
}

pointer MATTIMES3(ctx,n,argv)
     register context *ctx;
     int n;
     register pointer *argv;
{
  register int i;
  register pointer p,result;
  eusfloat_t *c1,*c2,*c3;
  eusfloat_t q1[4], q2[4], q3[4], q;
  
  ckarg2(2,3);
  c1 = argv[0]->c.ary.entity->c.fvec.fv;
  c2 = argv[1]->c.ary.entity->c.fvec.fv;
  if (n==3) result = argv[2];
  else result = makematrix(ctx,3,3);
  c3 = result->c.ary.entity->c.fvec.fv;

  /*
     (setf c3 (quaternion2matrix 
	       (normalize-vector (quaternion*
				  (matrix2quaternion c1) 
				  (matrix2quaternion c2)))))
  */
  matrix2quaternion(c1, q1);
  matrix2quaternion(c2, q2);
  quaternion_multiply(q1, q2, q3);
  //noromalize-vector
  q = sqrt(q3[0]*q3[0]+q3[1]*q3[1]+q3[2]*q3[2]+q3[3]*q3[3]);
  q3[0] /= q; q3[1] /= q; q3[2] /= q; q3[3] /= q;
  quaternion2matrix(q3, c3);

  return(result);
}

pointer MATPLUS(ctx,n,argv)
     register context *ctx;
     int n;
     register pointer *argv;
{
  register int i, j, row, col;
  register pointer p,result;
  eusfloat_t *c1,*c2,*c3;
  
  ckarg2(2,3);
  if (!ismatrix(argv[0]) || !ismatrix(argv[1])) error(E_NOVECTOR);
  c1 = argv[0]->c.ary.entity->c.fvec.fv;
  c2 = argv[1]->c.ary.entity->c.fvec.fv;
  row = rowsize(argv[0]); col = colsize(argv[0]); 
  if (!((row==rowsize(argv[1])) && (col==colsize(argv[1]))) )
    error(E_VECINDEX);
  if (n==3) {
    if (!((row==rowsize(argv[2])) &&
	  (col==colsize(argv[2])))) error(E_VECINDEX);
    result = argv[2];
  } else {
    result = makematrix(ctx,row,col);
  }
  c3 = result->c.ary.entity->c.fvec.fv;

  for (i = 0; i< row; i++ ) {
    for (j = 0; j< col; j++ ) {
      c3[i*col+j] = c1[i*col+j] + c2[i*col+j];
    }
  }

  return(result);
}

pointer MATMINUS(ctx,n,argv)
     register context *ctx;
     int n;
     register pointer *argv;
{
  register int i, j, row, col;
  register pointer p,result;
  eusfloat_t *c1,*c2,*c3;
  
  ckarg2(2,3);
  if (!ismatrix(argv[0]) || !ismatrix(argv[1])) error(E_NOVECTOR);
  c1 = argv[0]->c.ary.entity->c.fvec.fv;
  c2 = argv[1]->c.ary.entity->c.fvec.fv;
  row = rowsize(argv[0]); col = colsize(argv[0]); 
  if (!((row==rowsize(argv[1])) && (col==colsize(argv[1]))) )
    error(E_VECINDEX);
  if (n==3) {
    if (!((row==rowsize(argv[2])) &&
	  (col==colsize(argv[2])))) error(E_VECINDEX);
    result = argv[2];
  } else {
    result = makematrix(ctx,row,col);
  }
  c3 = result->c.ary.entity->c.fvec.fv;

  for (i = 0; i< row; i++ ) {
    for (j = 0; j< col; j++ ) {
      c3[i*col+j] = c1[i*col+j] - c2[i*col+j];
    }
  }

  return(result);
}

void balanc(eusfloat_t **a, int n)
{
  eusfloat_t RADIX = 2.0;
  int last,j,i;
  eusfloat_t s,r,g,f,c,sqrdx;
  sqrdx=RADIX*RADIX;
  last=0;
  while (last == 0) {
    last=1;
    for (i=1;i<=n;i++) { // Calculate row and column norms.
      r=c=0.0;
      for (j=1;j<=n;j++)
	if (j != i) {
	  c += fabs(a[j][i]);
	  r += fabs(a[i][j]);
	}
      if (c && r) { // If both are nonzero,
	g=r/RADIX;
	f=1.0;
	s=c+r;
	while (c<g) { // find the integer power of the machine radix that comes closest to balancing the matrix.
	  f *= RADIX;
	  c *= sqrdx;
	}
	g=r*RADIX;
	while (c>g) {
	  f /= RADIX;
	  c /= sqrdx;
	}
	if ((c+r)/f < 0.95*s) {
	  last=0;
	  g=1.0/f;
	  for (j=1;j<=n;j++) a[i][j] *= g; // Apply similarity transformation.
	  for (j=1;j<=n;j++) a[j][i] *= f;
	}
      }
    }
  }
}

#define SWAP(g,h) {y=(g);(g)=(h);(h)=y;}
void elmhes(eusfloat_t **a, int n)
{
  int m,j,i;
  eusfloat_t y,x;
  for (m=2;m<n;m++) { // m is called r + 1 in the text.
    x=0.0;
    i=m;
    for (j=m;j<=n;j++) { // Find the pivot.
      if (fabs(a[j][m-1]) > fabs(x)) {
	x=a[j][m-1];
	i=j;
      }
    }
    if (i != m) { // Interchange rows and columns.
      for (j=m-1;j<=n;j++) SWAP(a[i][j],a[m][j]);
      for (j=1;j<=n;j++) SWAP(a[j][i],a[j][m]);
    }
    if (x) { // Carry out the elimination.
      for (i=m+1;i<=n;i++) {
	if ((y=a[i][m-1]) != 0.0) {
	  y /= x;
	  a[i][m-1]=y;
	  for (j=m;j<=n;j++)
	    a[i][j] -= y*a[m][j];
	  for (j=1;j<=n;j++)
	    a[j][m] += y*a[j][i];
	}
      }
    }
  }
}

int hqr(eusfloat_t **a, int n, eusfloat_t wr[], eusfloat_t wi[])
{
  int nn,m,l,k,j,its,i,mmin;
  eusfloat_t z,y,x,w,v,u,t,s,r,q,p,anorm;
  anorm=0.0; // Compute matrix norm for possible use inlocating  single small subdiagonal element. 
  for (i=1;i<=n;i++)
    for (j=max(i-1,1);j<=n;j++)
      anorm += fabs(a[i][j]);
  nn=n;
  t=0.0; //Gets changed only by an exceptional shift.
  while (nn >= 1) { // Begin search for next eigenvalue.
    its=0;
    do {
      for (l=nn;l>=2;l--) { // Begin iteration: look for single small subdiagonal element. 
	s=fabs(a[l-1][l-1])+fabs(a[l][l]);
	if (s == 0.0) s=anorm;
	if ((eusfloat_t)(fabs(a[l][l-1]) + s) == s) {
	  a[l][l-1]=0.0;
	  break;
	}
      }
      x=a[nn][nn];
      if (l == nn) { // One root found.
	wr[nn]=x+t;
	wi[nn--]=0.0;
      } else {
	y=a[nn-1][nn-1];
	w=a[nn][nn-1]*a[nn-1][nn];
	if (l == (nn-1)) { // Two roots found...
	  p=0.5*(y-x);
	  q=p*p+w;
	  z=sqrt(fabs(q));
	  x += t;
	  if (q >= 0.0) { // ...a real pair.
	    z=p+SIGN(z,p);
	    wr[nn-1]=wr[nn]=x+z;
	    if (z) wr[nn]=x-w/z;
	    wi[nn-1]=wi[nn]=0.0;
	  } else { // ...a complex pair.
	    wr[nn-1]=wr[nn]=x+p;
	    wi[nn-1]= -(wi[nn]=z);
	  }
	  nn -= 2;
	} else { // No roots found. Continue iteration.
	  if (its == 30) {nrerror("Too many iterations in hqr"); return -1;}
	  if (its == 10 || its == 20) { // Form exceptional shift.
	    t += x;
	    for (i=1;i<=nn;i++) a[i][i] -= x;
	    s=fabs(a[nn][nn-1])+fabs(a[nn-1][nn-2]);
	    y=x=0.75*s;
	    w = -0.4375*s*s;
	  }
	  ++its;
	  for (m=(nn-2);m>=l;m--) { // Form shift and then look for 2 consecutive small subdiagonal elements.
	    z=a[m][m];
	    r=x-z;
	    s=y-z;
	    p=(r*s-w)/a[m+1][m]+a[m][m+1]; // Equation (11.6.23).
	    q=a[m+1][m+1]-z-r-s;
	    r=a[m+2][m+1];
	    s=fabs(p)+fabs(q)+fabs(r); // Scale to prevent overflow or underflow.
	    p /= s;
	    q /= s;
	    r /= s;
	    if (m == l) break;
	    u=fabs(a[m][m-1])*(fabs(q)+fabs(r));
	    v=fabs(p)*(fabs(a[m-1][m-1])+fabs(z)+fabs(a[m+1][m+1]));
	    if ((eusfloat_t)(u+v) == v) break; // Equation (11.6.26).
	  }
	  for (i=m+2;i<=nn;i++) {
	    a[i][i-2]=0.0;
	    if (i != (m+2)) a[i][i-3]=0.0;
	  }
	  for (k=m;k<=nn-1;k++) {
	    // Double QR step on rows l to nn and columns m to nn.
	    if (k != m) {
	      p=a[k][k-1]; // Begin setup of Householder vector.
	      q=a[k+1][k-1];
	      r=0.0;
	      if (k != (nn-1)) r=a[k+2][k-1];
	      if ((x=fabs(p)+fabs(q)+fabs(r)) != 0.0) {
		p /= x; // Scale to prevent overflow or underflow.
		q /= x;
		r /= x;
	      }
	    }
	    if ((s=SIGN(sqrt(p*p+q*q+r*r),p)) != 0.0) {
	      if (k == m) {
		if (l != m)
		  a[k][k-1] = -a[k][k-1];
	      } else
		a[k][k-1] = -s*x;
	      p += s; // Equations (11.6.24).
	      x=p/s;
	      y=q/s;
	      z=r/s;
	      q /= p;
	      r /= p;
	      for (j=k;j<=nn;j++) { // Row modification.
		p=a[k][j]+q*a[k+1][j];
		if (k != (nn-1)) {
		  p += r*a[k+2][j];
		  a[k+2][j] -= p*z;
		}
		a[k+1][j] -= p*y;
		a[k][j] -= p*x;
	      }
	      mmin = nn<k+3 ? nn : k+3;
	      for (i=l;i<=mmin;i++) { // Column modification.
		p=x*a[i][k]+y*a[i][k+1];
		if (k != (nn-1)) {
		  p += z*a[i][k+2];
		  a[i][k+2] -= p*r;
		}
		a[i][k+1] -= p*q;
		a[i][k] -= p;
	      }
	    }
	  }
	}
      }
    } while (l < nn-1);
  }
  return 1;
}

eusfloat_t pythag(eusfloat_t a, eusfloat_t b)
{
  eusfloat_t absa, absb;
  absa=fabs(a);
  absb=fabs(b);
  if (absa > absb) return absa*sqrt(1.0+SQR(absb/absa));
  else return (absb == 0.0 ? 0.0 : absb*sqrt(1.0+SQR(absa/absb)));
}

pointer QL_DECOMPOSE(ctx,n,argv)
register context *ctx;
int n;
pointer argv[];
/* (QL_DECOMPOSE mat) */
{
  pointer a,re,rv;
  eusfloat_t **aa, *d, *e;
  int c, i, j;

  ckarg(1);
  a=argv[0];
  if (!ismatrix(a)) error(E_NOVECTOR);
  c = colsize(a);
  if(c != rowsize(a)) error(E_VECSIZE);

  aa = nr_matrix(1,c,1,c);
  d = nr_vector(1,c);
  e = nr_vector(1,c);
  re = makefvector(c);
  rv = makematrix(ctx,c,c);

  for (i = 0; i < c; i++){
    for (j = 0; j < c; j++){
      aa[j+1][i+1]=a->c.ary.entity->c.fvec.fv[j*c+i];
    }
  }

  tred2(aa, c, d, e);
  if ( tqli(d, e, c, aa) < 0 ) {
    free_nr_matrix(aa,1,c,1,c);
    free_nr_vector(d,1,c);
    free_nr_vector(e,1,c);
    return NIL;
  }

  for (i = 0; i < c; i++){
    re->c.fvec.fv[i] = d[i+1];
  }
  for (i = 0; i < c; i++){
    for (j = 0; j < c; j++){
      rv->c.ary.entity->c.fvec.fv[j*c+i] = aa[j+1][i+1];
    }
  }

  free_nr_matrix(aa,1,c,1,c);
  free_nr_vector(d,1,c);
  free_nr_vector(e,1,c);
  return (cons(ctx,re,cons(ctx,rv,NIL)));}

pointer QR_DECOMPOSE(ctx,n,argv)
register context *ctx;
int n;
pointer argv[];
/* (QR_DECOMPOSE mat) */
{
  pointer a,rr,ri, r;
  eusfloat_t **aa, *wr, *wi;
  int c, i, j, pc=0;

  ckarg(1);
  a=argv[0];
  if (!ismatrix(a)) error(E_NOVECTOR);
  c = colsize(a);
  if(c != rowsize(a)) error(E_VECSIZE);

  aa = nr_matrix(1,c,1,c);
  wr = nr_vector(1,c);
  wi = nr_vector(1,c);
  rr = makefvector(c); vpush(rr); pc++;
  ri = makefvector(c); vpush(ri); pc++;

  for (i = 0; i < c; i++){
    for (j = 0; j < c; j++){
      aa[j+1][i+1]=a->c.ary.entity->c.fvec.fv[j*c+i];
    }
  }

  balanc(aa, c);
  elmhes(aa, c);
  if ( hqr(aa, c, wr, wi) < 0 ) {
    free_nr_matrix(aa,1,c,1,c);
    free_nr_vector(wr,1,c);
    free_nr_vector(wi,1,c);
    while(pc-->0) vpop();
    return NIL;
  }

  for (i = 0; i < c; i++){
    rr->c.fvec.fv[i] = wr[i+1];
    ri->c.fvec.fv[i] = wi[i+1];
  }

  free_nr_matrix(aa,1,c,1,c);
  free_nr_vector(wr,1,c);
  free_nr_vector(wi,1,c);

  while(pc-->0) vpop();
  return (cons(ctx,rr,cons(ctx,ri,NIL)));};

//
//
//
#define MAXBIT 30
#define MAXDIM 6
void sobseq(int *n, eusfloat_t x[])
     /*
       When n is negative, internally initializes a set of MAXBIT direction 
       numbers for each of MAXDIM  different Sobol' sequences. When n is 
       positive (but .MAXDIM), returns as the vector x[1..n] the next values 
       from n of these sequences. (n must not be changed between 
       initializations.) 
     */
{
  int j,k,l;
  unsigned long i,im,ipp;
  static eusfloat_t fac;
  static unsigned long in,ix[MAXDIM+1],*iu[MAXBIT+1];
  static unsigned long mdeg[MAXDIM+1]={0,1,2,3,3,4,4};
  static unsigned long ip[MAXDIM+1]={0,0,1,1,2,1,4};
  static unsigned long iv[MAXDIM*MAXBIT+1]={
    0,1,1,1,1,1,1,3,1,3,3,1,1,5,7,7,3,3,5,15,11,5,15,13,9};
  if (*n < 0) { // Initialize, don't return a vector.
    for (k=1;k<=MAXDIM;k++) ix[k]=0;
    in=0;
    if (iv[1] != 1) return;
    fac=1.0/(1L << MAXBIT);
    for (j=1,k=0;j<=MAXBIT;j++,k+=MAXDIM) iu[j] = &iv[k];
    // To allowb oth 1D and 2D addressing.
    for (k=1;k<=MAXDIM;k++) {
      for (j=1;j<=mdeg[k];j++) iu[j][k] <<= (MAXBIT-j);
      //Stored values only require normalization.
      for (j=mdeg[k]+1;j<=MAXBIT;j++) { //Use the recurrence to get other values.
	ipp=ip[k]; 
	i=iu[j-mdeg[k]][k];
	i ^= (i >> mdeg[k]);
	for (l=mdeg[k]-1;l>=1;l--) {
	  if (ipp & 1) i ^= iu[j-l][k];
	  ipp >>= 1;
	}
	iu[j][k]=i;
      }
    }
  } else if (*n > MAXDIM) {
    fprintf (stderr, "max dimension is %d!! ", MAXDIM);
    error(E_VECINDEX);
  } else { // Calculate the next vector in the seiquence.
    im=in++;
    for (j=1;j<=MAXBIT;j++) { // Find the rightmost zero bit.
      if (!(im & 1)) break;
      im >>= 1;
    }
    if (j > MAXBIT) nrerror("MAXBIT too small in sobseq");
    im=(j-1)*MAXDIM;
    for (k=1;k<=IMIN(*n,MAXDIM);k++) { // XOR the appropriate direction number into each component of the vector and convert to a floating number.
      ix[k] ^= iv[im+k];
      x[k]=ix[k]*fac;
    }
  }
}
// quasi-random (n &optional v) 
// if n < 0 then reset values;
pointer QUASI_RANDOM(ctx,n,argv)
     register context *ctx;
     int n;
     register pointer *argv;
{
  int dim;
  eusfloat_t *v;
  register pointer r;

  ckarg2(1,2);
  dim=bigintval(argv[0]);
  if (dim<0){
    sobseq(&dim,v);
    return NIL;
  }
  if (n==2){
    if ( vecsize(argv[1]) != dim ) error(E_VECINDEX);
    r = argv[1];
  }else{
    r = (pointer)makefvector(dim);
  }
  v = r->c.fvec.fv;
  sobseq(&dim,&(v[-1]));
  return r;
}

pointer ___irtc(ctx,n,argv, env)
register context *ctx;
int n;
pointer argv[];
pointer env;
{
  pointer mod=argv[0];
  defun(ctx,"ROTM3*",mod,MATTIMES3);
  defun(ctx,"M+",mod,MATPLUS);
  defun(ctx,"M-",mod,MATMINUS);
  defun(ctx,"SV-SOLVE",mod,SV_SOLVE);
  defun(ctx,"SV-DECOMPOSE",mod,SV_DECOMPOSE);
  defun(ctx,"LU-SOLVE2",mod,LU_SOLVE2);
  defun(ctx,"LU-DECOMPOSE2",mod,LU_DECOMPOSE2);
  defun(ctx,"MATRIX-DETERMINANT",mod,MATRIX_DETERMINANT);
  defun(ctx,"PSEUDO-INVERSE2",mod,PSEUDO_INVERSE2);
  defun(ctx,"QL-DECOMPOSE",mod,QL_DECOMPOSE);
  defun(ctx,"QR-DECOMPOSE",mod,QR_DECOMPOSE);
  defun(ctx,"QUASI-RANDOM",mod,QUASI_RANDOM);

  /* irteus-version */
  extern pointer QVERSION;
  pointer p, v = speval(QVERSION);
  p=cons(ctx,makestring(SVNVERSION,strlen(SVNVERSION)),NIL);
  vpush(v); vpush(p);
  v=NCONC(ctx,2,ctx->vsp-2);
}

///////////////////////////////////////////////////////////////////////////////
///
/// $Id$
///
/// $Log$
/// Revision 1.11  2010-03-13 05:49:12  k-okada
/// split nr code from irtc.c to nr.c
///
/// Revision 1.10  2010/02/03 07:36:06  k-okada
/// float_t->eusfloat_t, integer_t->eusinteger_t
///
/// Revision 1.9  2010/02/02 09:50:35  k-okada
/// fix for 64bit eus float->float_t
///
/// Revision 1.8  2009/11/08 04:08:09  inaba
/// change exit to error for continuing debug of NaN in matrix2quaternion of irtc.c
///
/// Revision 1.7  2009/08/13 16:43:37  fujimoto
/// fix pseudo-inverse2
///
/// Revision 1.6  2009/08/07 11:22:38  k-okada
/// add pseudo-inverse2, use array-dimensions
///
/// Revision 1.5  2009/03/02 12:12:49  k-okada
/// lu-decompose2 accepts LU-DECOMPOSE2 mat [result] [tmp-vector]
///
/// Revision 1.4  2009/02/17 02:04:48  k-okada
/// fix typo on copyright
///
/// Revision 1.3  2008/11/11 11:10:25  k-okada
/// error handling when normalize-vector #f(0 0 0), again
///
/// Revision 1.2  2008/11/11 03:01:18  k-okada
/// error handling when normalize-vector #f(0 0 0) -> 0, add VNORMALIZE in irtc.c remove defun normalize-vector from irtmath.l
///
/// Revision 1.1  2008/09/18 18:11:00  k-okada
/// add irteus
///
///
///
