      subroutine dqawse(f,a,b,alfa,beta,integr,epsabs,epsrel,limit,
     *   result,abserr,neval,ier,alist,blist,rlist,elist,iord,last)
c***begin prologue  dqawse
c***date written   800101   (yymmdd)
c***revision date  830518   (yymmdd)
c***category no.  h2a2a1
c***keywords  automatic integrator, special-purpose,
c             algebraico-logarithmic end point singularities,
c             clenshaw-curtis method
c***author  piessens,robert,appl. math. & progr. div. - k.u.leuven
c           de doncker,elise,appl. math. & progr. div. - k.u.leuven
c***purpose  the routine calculates an approximation result to a given
c            definite integral i = integral of f*w over (a,b),
c            (where w shows a singular behaviour at the end points,
c            see parameter integr).
c            hopefully satisfying following claim for accuracy
c            abs(i-result).le.max(epsabs,epsrel*abs(i)).
c***description
c
c        integration of functions having algebraico-logarithmic
c        end point singularities
c        standard fortran subroutine
c        double precision version
c***references  (none)
c***routines called  d1mach,dqc25s,dqmomo,dqpsrt
c***end prologue  dqawse
c
      double precision a,abserr,alfa,alist,area,area1,area12,area2,a1,
     *  a2,b,beta,blist,b1,b2,centre,dabs,dmax1,d1mach,elist,epmach,
     *  epsabs,epsrel,errbnd,errmax,error1,erro12,error2,errsum,f,
     *  resas1,resas2,result,rg,rh,ri,rj,rlist,uflow
      integer ier,integr,iord,iroff1,iroff2,k,last,limit,maxerr,nev,
     *  neval,nrmax
c
      external f
c
      dimension alist(limit),blist(limit),rlist(limit),elist(limit),
     *  iord(limit),ri(25),rj(25),rh(25),rg(25)
c
c            list of major variables
c            -----------------------
c
c
c***first executable statement  dqawse
      epmach = d1mach(4)
      uflow = d1mach(1)
c
c           test on validity of parameters
c           ------------------------------
c
      ier = 6
      neval = 0
      last = 0
      rlist(1) = 0.0d+00
      elist(1) = 0.0d+00
      iord(1) = 0
      result = 0.0d+00
      abserr = 0.0d+00
      if(b.le.a.or.(epsabs.eq.0.0d+00.and.
     *  epsrel.lt.dmax1(0.5d+02*epmach,0.5d-28)).or.alfa.le.(-0.1d+01)
     *  .or.beta.le.(-0.1d+01).or.integr.lt.1.or.integr.gt.4.or.
     *  limit.lt.2) go to 999
      ier = 0
c
c           compute the modified chebyshev moments.
c
      call dqmomo(alfa,beta,ri,rj,rg,rh,integr)
c
c           integrate over the intervals (a,(a+b)/2) and ((a+b)/2,b).
c
      centre = 0.5d+00*(b+a)
      call dqc25s(f,a,b,a,centre,alfa,beta,ri,rj,rg,rh,area1,
     *  error1,resas1,integr,nev)
      neval = nev
      call dqc25s(f,a,b,centre,b,alfa,beta,ri,rj,rg,rh,area2,
     *  error2,resas2,integr,nev)
      last = 2
      neval = neval+nev
      result = area1+area2
      abserr = error1+error2
c
c           test on accuracy.
c
      errbnd = dmax1(epsabs,epsrel*dabs(result))
c
c           initialization
c           --------------
c
      if(error2.gt.error1) go to 10
      alist(1) = a
      alist(2) = centre
      blist(1) = centre
      blist(2) = b
      rlist(1) = area1
      rlist(2) = area2
      elist(1) = error1
      elist(2) = error2
      go to 20
   10 alist(1) = centre
      alist(2) = a
      blist(1) = b
      blist(2) = centre
      rlist(1) = area2
      rlist(2) = area1
      elist(1) = error2
      elist(2) = error1
   20 iord(1) = 1
      iord(2) = 2
      if(limit.eq.2) ier = 1
      if(abserr.le.errbnd.or.ier.eq.1) go to 999
      errmax = elist(1)
      maxerr = 1
      nrmax = 1
      area = result
      errsum = abserr
      iroff1 = 0
      iroff2 = 0
c
c            main do-loop
c            ------------
c
      do 60 last = 3,limit
c
c           bisect the subinterval with largest error estimate.
c
        a1 = alist(maxerr)
        b1 = 0.5d+00*(alist(maxerr)+blist(maxerr))
        a2 = b1
        b2 = blist(maxerr)
c
        call dqc25s(f,a,b,a1,b1,alfa,beta,ri,rj,rg,rh,area1,
     *  error1,resas1,integr,nev)
        neval = neval+nev
        call dqc25s(f,a,b,a2,b2,alfa,beta,ri,rj,rg,rh,area2,
     *  error2,resas2,integr,nev)
        neval = neval+nev
c
c           improve previous approximations integral and error
c           and test for accuracy.
c
        area12 = area1+area2
        erro12 = error1+error2
        errsum = errsum+erro12-errmax
        area = area+area12-rlist(maxerr)
        if(a.eq.a1.or.b.eq.b2) go to 30
        if(resas1.eq.error1.or.resas2.eq.error2) go to 30
c
c           test for roundoff error.
c
        if(dabs(rlist(maxerr)-area12).lt.0.1d-04*dabs(area12)
     *  .and.erro12.ge.0.99d+00*errmax) iroff1 = iroff1+1
        if(last.gt.10.and.erro12.gt.errmax) iroff2 = iroff2+1
   30   rlist(maxerr) = area1
        rlist(last) = area2
c
c           test on accuracy.
c
        errbnd = dmax1(epsabs,epsrel*dabs(area))
        if(errsum.le.errbnd) go to 35
c
c           set error flag in the case that the number of interval
c           bisections exceeds limit.
c
        if(last.eq.limit) ier = 1
c
c
c           set error flag in the case of roundoff error.
c
        if(iroff1.ge.6.or.iroff2.ge.20) ier = 2
c
c           set error flag in the case of bad integrand behaviour
c           at interior points of integration range.
c
        if(dmax1(dabs(a1),dabs(b2)).le.(0.1d+01+0.1d+03*epmach)*
     *  (dabs(a2)+0.1d+04*uflow)) ier = 3
c
c           append the newly-created intervals to the list.
c
   35   if(error2.gt.error1) go to 40
        alist(last) = a2
        blist(maxerr) = b1
        blist(last) = b2
        elist(maxerr) = error1
        elist(last) = error2
        go to 50
   40   alist(maxerr) = a2
        alist(last) = a1
        blist(last) = b1
        rlist(maxerr) = area2
        rlist(last) = area1
        elist(maxerr) = error2
        elist(last) = error1
c
c           call subroutine dqpsrt to maintain the descending ordering
c           in the list of error estimates and select the subinterval
c           with largest error estimate (to be bisected next).
c
   50   call dqpsrt(limit,last,maxerr,errmax,elist,iord,nrmax)
c ***jump out of do-loop
        if (ier.ne.0.or.errsum.le.errbnd) go to 70
   60 continue
c
c           compute final result.
c           ---------------------
c
   70 result = 0.0d+00
      do 80 k=1,last
        result = result+rlist(k)
   80 continue
      abserr = errsum
  999 return
      end
