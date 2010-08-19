c
c     Copyright (c) 2002-2010 Boyce Griffith
c
c     Permission is hereby granted, free of charge, to any person
c     obtaining a copy of this software and associated documentation
c     files (the "Software"), to deal in the Software without
c     restriction, including without limitation the rights to use, copy,
c     modify, merge, publish, distribute, sublicense, and/or sell copies
c     of the Software, and to permit persons to whom the Software is
c     furnished to do so, subject to the following conditions:
c
c     The above copyright notice and this permission notice shall be
c     included in all copies or substantial portions of the Software.
c
c     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
c     EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
c     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
c     NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
c     HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
c     WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
c     OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
c     DEALINGS IN THE SOFTWARE.
c
dnl Process this file with m4 to produce FORTRAN source code
define(NDIM,3)dnl
define(REAL,`double precision')dnl
define(INTEGER,`integer')dnl
include(SAMRAI_FORTDIR/pdat_m4arrdim3d.i)dnl
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
c     Computes the advective flux corresponding to a face centered value
c     and a face centered advective velocity.
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
      subroutine advect_flux3d(
     &     dt,
     &     ifirst0,ilast0,ifirst1,ilast1,ifirst2,ilast2,
     &     nugc0,nugc1,nugc2,
     &     nqhalfgc0,nqhalfgc1,nqhalfgc2,
     &     nfluxgc0,nfluxgc1,nfluxgc2,
     &     u0,u1,u2,
     &     qhalf0,qhalf1,qhalf2,
     &     flux0,flux1,flux2)
c
      implicit none
include(TOP_SRCDIR/src/fortran/const.i)dnl
c
c     Input.
c
      INTEGER ifirst0,ilast0,ifirst1,ilast1,ifirst2,ilast2

      INTEGER nugc0,nugc1,nugc2
      INTEGER nqhalfgc0,nqhalfgc1,nqhalfgc2
      INTEGER nfluxgc0,nfluxgc1,nfluxgc2

      REAL dt

      REAL u0(FACE3d0VECG(ifirst,ilast,nugc))
      REAL u1(FACE3d1VECG(ifirst,ilast,nugc))
      REAL u2(FACE3d2VECG(ifirst,ilast,nugc))

      REAL qhalf0(FACE3d0VECG(ifirst,ilast,nqhalfgc))
      REAL qhalf1(FACE3d1VECG(ifirst,ilast,nqhalfgc))
      REAL qhalf2(FACE3d2VECG(ifirst,ilast,nqhalfgc))
c
c     Input/Output.
c
      REAL flux0(FACE3d0VECG(ifirst,ilast,nfluxgc))
      REAL flux1(FACE3d1VECG(ifirst,ilast,nfluxgc))
      REAL flux2(FACE3d2VECG(ifirst,ilast,nfluxgc))
c
c     Local variables.
c
      INTEGER ic0,ic1,ic2
c
c     Compute the time integral of the advective flux.
c
      do ic2 = ifirst2,ilast2
         do ic1 = ifirst1,ilast1
            do ic0 = ifirst0-1,ilast0
               flux0(ic0+1,ic1,ic2) =
     &              dt*u0(ic0+1,ic1,ic2)*qhalf0(ic0+1,ic1,ic2)
            enddo
         enddo
      enddo

      do ic0 = ifirst0,ilast0
         do ic2 = ifirst2,ilast2
            do ic1 = ifirst1-1,ilast1
               flux1(ic1+1,ic2,ic0) =
     &              dt*u1(ic1+1,ic2,ic0)*qhalf1(ic1+1,ic2,ic0)
            enddo
         enddo
      enddo

      do ic1 = ifirst1,ilast1
         do ic0 = ifirst0,ilast0
            do ic2 = ifirst2-1,ilast2
               flux2(ic2+1,ic0,ic1) =
     &              dt*u2(ic2+1,ic0,ic1)*qhalf2(ic2+1,ic0,ic1)
            enddo
         enddo
      enddo
c
      return
      end
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
c     Update a quantity using flux differencing.
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
      subroutine advect_consdiff3d(
     &     dx,
     &     ifirst0,ilast0,ifirst1,ilast1,ifirst2,ilast2,
     &     nfluxgc0,nfluxgc1,nfluxgc2,
     &     nqvalgc0,nqvalgc1,nqvalgc2,
     &     flux0,flux1,flux2,
     &     qval)
c
      implicit none
include(TOP_SRCDIR/src/fortran/const.i)dnl
c
c     Input.
c
      INTEGER ifirst0,ilast0,ifirst1,ilast1,ifirst2,ilast2

      INTEGER nfluxgc0,nfluxgc1,nfluxgc2
      INTEGER nqvalgc0,nqvalgc1,nqvalgc2

      REAL dx(0:NDIM-1)

      REAL flux0(FACE3d0VECG(ifirst,ilast,nfluxgc))
      REAL flux1(FACE3d1VECG(ifirst,ilast,nfluxgc))
      REAL flux2(FACE3d2VECG(ifirst,ilast,nfluxgc))
c
c     Input/Output.
c
      REAL qval(CELL3dVECG(ifirst,ilast,nqvalgc))
c
c     Local variables.
c
      INTEGER ic0,ic1,ic2
c
c     Update a quantity using flux differencing.
c
      do ic2 = ifirst2,ilast2
         do ic1 = ifirst1,ilast1
            do ic0 = ifirst0,ilast0
               qval(ic0,ic1,ic2) = qval(ic0,ic1,ic2)
     &              -(flux0(ic0+1,ic1,ic2)-flux0(ic0,ic1,ic2))/dx(0)
     &              -(flux1(ic1+1,ic2,ic0)-flux1(ic1,ic2,ic0))/dx(1)
     &              -(flux2(ic2+1,ic0,ic1)-flux2(ic2,ic0,ic1))/dx(2)
            enddo
         enddo
      enddo
c
      return
      end
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
c     Update a quantity using flux differencing but include the proper
c     source term to account for a non-discretely divergence free
c     advection velocity.
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
      subroutine advect_consdiffwithdivsource3d(
     &     dx,dt,
     &     ifirst0,ilast0,ifirst1,ilast1,ifirst2,ilast2,
     &     nfluxgc0,nfluxgc1,nfluxgc2,
     &     nqfluxgc0,nqfluxgc1,nqfluxgc2,
     &     nufluxgc0,nufluxgc1,nufluxgc2,
     &     nqvalgc0,nqvalgc1,nqvalgc2,
     &     flux0,flux1,flux2,
     &     qflux0,qflux1,qflux2,
     &     uflux0,uflux1,uflux2,
     &     qval)
c
      implicit none
include(TOP_SRCDIR/src/fortran/const.i)dnl
c
c     Input.
c
      INTEGER ifirst0,ilast0,ifirst1,ilast1,ifirst2,ilast2

      INTEGER nfluxgc0,nfluxgc1,nfluxgc2
      INTEGER nqfluxgc0,nqfluxgc1,nqfluxgc2
      INTEGER nufluxgc0,nufluxgc1,nufluxgc2
      INTEGER nqvalgc0,nqvalgc1,nqvalgc2

      REAL dx(0:NDIM-1),dt

      REAL flux0(FACE3d0VECG(ifirst,ilast,nfluxgc))
      REAL flux1(FACE3d1VECG(ifirst,ilast,nfluxgc))
      REAL flux2(FACE3d2VECG(ifirst,ilast,nfluxgc))

      REAL qflux0(FACE3d0VECG(ifirst,ilast,nqfluxgc))
      REAL qflux1(FACE3d1VECG(ifirst,ilast,nqfluxgc))
      REAL qflux2(FACE3d2VECG(ifirst,ilast,nqfluxgc))

      REAL uflux0(FACE3d0VECG(ifirst,ilast,nufluxgc))
      REAL uflux1(FACE3d1VECG(ifirst,ilast,nufluxgc))
      REAL uflux2(FACE3d2VECG(ifirst,ilast,nufluxgc))
c
c     Input/Output.
c
      REAL qval(CELL3dVECG(ifirst,ilast,nqvalgc))
c
c     Local variables.
c
      INTEGER ic0,ic1,ic2
      REAL divsource
c
c     Update a quantity using flux differencing.
c
      do ic2 = ifirst2,ilast2
         do ic1 = ifirst1,ilast1
            do ic0 = ifirst0,ilast0
               divsource = (sixth/dt)*
     &              ( qflux0(ic0+1,ic1,ic2) + qflux0(ic0,ic1,ic2)
     &              + qflux1(ic1+1,ic2,ic0) + qflux1(ic1,ic2,ic0)
     &              + qflux2(ic2+1,ic0,ic1) + qflux2(ic2,ic0,ic1) )*
     &              ( (uflux0(ic0+1,ic1,ic2)-uflux0(ic0,ic1,ic2))/dx(0)
     &              + (uflux1(ic1+1,ic2,ic0)-uflux1(ic1,ic2,ic0))/dx(1)
     &              + (uflux2(ic2+1,ic0,ic1)-uflux2(ic2,ic0,ic1))/dx(2))

               qval(ic0,ic1,ic2) = qval(ic0,ic1,ic2) + divsource
     &              -(flux0(ic0+1,ic1,ic2)-flux0(ic0,ic1,ic2))/dx(0)
     &              -(flux1(ic1+1,ic2,ic0)-flux1(ic1,ic2,ic0))/dx(1)
     &              -(flux2(ic2+1,ic0,ic1)-flux2(ic2,ic0,ic1))/dx(2)
            enddo
         enddo
      enddo
c
      return
      end
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
c     Computes the advective derivative N = [u_ADV*grad(q)].
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
      subroutine advect_derivative3d(
     &     dx,
     &     ifirst0,ilast0,ifirst1,ilast1,ifirst2,ilast2,
     &     nuadvgc0,nuadvgc1,nuadvgc2,
     &     nqgc0,nqgc1,nqgc2,
     &     uadv0,uadv1,uadv2,
     &     q0,q1,q2,
     &     nNgc0,nNgc1,nNgc2,
     &     N)
c
      implicit none
include(TOP_SRCDIR/src/fortran/const.i)dnl
c
c     Input.
c
      INTEGER ifirst0,ilast0,ifirst1,ilast1,ifirst2,ilast2

      INTEGER nuadvgc0,nuadvgc1,nuadvgc2
      INTEGER nqgc0,nqgc1,nqgc2
      INTEGER nNgc0,nNgc1,nNgc2

      REAL dx(0:NDIM-1)

      REAL uadv0(FACE3d0VECG(ifirst,ilast,nuadvgc))
      REAL uadv1(FACE3d1VECG(ifirst,ilast,nuadvgc))
      REAL uadv2(FACE3d2VECG(ifirst,ilast,nuadvgc))

      REAL q0(FACE3d0VECG(ifirst,ilast,nqgc))
      REAL q1(FACE3d1VECG(ifirst,ilast,nqgc))
      REAL q2(FACE3d2VECG(ifirst,ilast,nqgc))
c
c     Input/Output.
c
      REAL N(CELL3dVECG(ifirst,ilast,nNgc))
c
c     Local variables.
c
      INTEGER ic0,ic1,ic2
      REAL U,V,W
      REAL Qx0,Qx1,Qx2
c
c     Compute (U,V,W)*grad(q).
c
      do ic2 = ifirst2,ilast2
         do ic1 = ifirst1,ilast1
            do ic0 = ifirst0,ilast0
               U = 0.5d0*(uadv0(ic0+1,ic1,ic2)+uadv0(ic0,ic1,ic2))
               Qx0 = (q0(ic0+1,ic1,ic2)-q0(ic0,ic1,ic2))/dx(0)
               N(ic0,ic1,ic2) = U*Qx0
            enddo
         enddo
      enddo

      do ic0 = ifirst0,ilast0
         do ic2 = ifirst2,ilast2
            do ic1 = ifirst1,ilast1
               V = 0.5d0*(uadv1(ic1+1,ic2,ic0)+uadv1(ic1,ic2,ic0))
               Qx1 = (q1(ic1+1,ic2,ic0)-q1(ic1,ic2,ic0))/dx(1)
               N(ic0,ic1,ic2) = N(ic0,ic1,ic2) + V*Qx1
            enddo
         enddo
      enddo

      do ic1 = ifirst1,ilast1
         do ic0 = ifirst0,ilast0
            do ic2 = ifirst2,ilast2
               W = 0.5d0*(uadv2(ic2+1,ic0,ic1)+uadv2(ic2,ic0,ic1))
               Qx2 = (q2(ic2+1,ic0,ic1)-q2(ic2,ic0,ic1))/dx(2)
               N(ic0,ic1,ic2) = N(ic0,ic1,ic2) + W*Qx2
            enddo
         enddo
      enddo
c
      return
      end
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
c     Computes the convective derivative N = div[q*u_ADV].
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
      subroutine convect_derivative3d(
     &     dx,
     &     ifirst0,ilast0,ifirst1,ilast1,ifirst2,ilast2,
     &     nuadvgc0,nuadvgc1,nuadvgc2,
     &     nqgc0,nqgc1,nqgc2,
     &     uadv0,uadv1,uadv2,
     &     q0,q1,q2,
     &     nNgc0,nNgc1,nNgc2,
     &     N)
c
      implicit none
include(TOP_SRCDIR/src/fortran/const.i)dnl
c
c     Input.
c
      INTEGER ifirst0,ilast0,ifirst1,ilast1,ifirst2,ilast2

      INTEGER nuadvgc0,nuadvgc1,nuadvgc2
      INTEGER nqgc0,nqgc1,nqgc2
      INTEGER nNgc0,nNgc1,nNgc2

      REAL dx(0:NDIM-1)

      REAL uadv0(FACE3d0VECG(ifirst,ilast,nuadvgc))
      REAL uadv1(FACE3d1VECG(ifirst,ilast,nuadvgc))
      REAL uadv2(FACE3d2VECG(ifirst,ilast,nuadvgc))

      REAL q0(FACE3d0VECG(ifirst,ilast,nqgc))
      REAL q1(FACE3d1VECG(ifirst,ilast,nqgc))
      REAL q2(FACE3d2VECG(ifirst,ilast,nqgc))
c
c     Input/Output.
c
      REAL N(CELL3dVECG(ifirst,ilast,nNgc))
c
c     Local variables.
c
      INTEGER ic0,ic1,ic2
      REAL QUx0,QVx1,QWx2
c
c     Compute (U,V,W)*grad(q).
c
      do ic2 = ifirst2,ilast2
         do ic1 = ifirst1,ilast1
            do ic0 = ifirst0,ilast0
               QUx0 = (uadv0(ic0+1,ic1,ic2)*q0(ic0+1,ic1,ic2)-
     &              uadv0(ic0,ic1,ic2)*q0(ic0,ic1,ic2))/dx(0)
               N(ic0,ic1,ic2) = QUx0
            enddo
         enddo
      enddo

      do ic0 = ifirst0,ilast0
         do ic2 = ifirst2,ilast2
            do ic1 = ifirst1,ilast1
               QVx1 = (uadv1(ic1+1,ic2,ic0)*q1(ic1+1,ic2,ic0)-
     &              uadv1(ic1,ic2,ic0)*q1(ic1,ic2,ic0))/dx(1)
               N(ic0,ic1,ic2) = N(ic0,ic1,ic2) + QVx1
            enddo
         enddo
      enddo

      do ic1 = ifirst1,ilast1
         do ic0 = ifirst0,ilast0
            do ic2 = ifirst2,ilast2
               QWx2 = (uadv2(ic2+1,ic0,ic1)*q2(ic2+1,ic0,ic1)-
     &              uadv2(ic2,ic0,ic1)*q2(ic2,ic0,ic1))/dx(2)
               N(ic0,ic1,ic2) = N(ic0,ic1,ic2) + QWx2
            enddo
         enddo
      enddo
c
      return
      end
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
