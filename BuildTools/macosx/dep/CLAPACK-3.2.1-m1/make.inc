# -*- Makefile -*-
####################################################################
#  LAPACK make include file.                                       #
#  LAPACK, Version 3.2.1                                           #
#  June 2009		                                               #
####################################################################
#
# See the INSTALL/ directory for more examples.
#
SHELL = /bin/sh
#
#  The machine (platform) identifier to append to the library names
#
PLAT = 
#  
#  Modify the FORTRAN and OPTS definitions to refer to the
#  compiler and desired compiler options for your machine.  NOOPT
#  refers to the compiler options desired when NO OPTIMIZATION is
#  selected.  Define LOADER and LOADOPTS to refer to the loader
#  and desired load options for your machine.
#
#######################################################
# This is used to compile C libary
CC        = gcc
# if no wrapping of the blas library is needed, uncomment next line
#CC        = gcc -DNO_BLAS_WRAP
CFLAGS    = -O3 -m64 -arch arm64 -isysroot $GDA_WITH_SYSROOT -mmacosx-version-min=10.10 -I$(GEODA_HOME)/temp/CLAPACK-3.2.1/INCLUDE -I$(GEODA_HOME)/temp/CLAPACK-3.2.1/
CFLAGS    = -O3 -m64 -arch arm64  -I$(GEODA_HOME)/temp/CLAPACK-3.2.1/INCLUDE -I$(GEODA_HOME)/temp/CLAPACK-3.2.1/
LOADER    =  gcc
LOADOPTS  =  -m64 -arch arm64 -isysroot $GDA_WITH_SYSROOT -mmacosx-version-min=10.10
LOADOPTS  =  -m64 -arch arm64 
NOOPT     = -O0 -m64 -arch arm64 -isysroot $GDA_WITH_SYSROOT -mmacosx-version-min=10.10 -I$(GEODA_HOME)/temp/CLAPACK-3.2.1/INCLUDE
NOOPT     = -O0 -m64 -arch arm64 -I$(GEODA_HOME)/temp/CLAPACK-3.2.1/INCLUDE
DRVCFLAGS = $(CFLAGS)
F2CCFLAGS = $(CFLAGS)
#######################################################################

#
# Timer for the SECOND and DSECND routines
#
# Default : SECOND and DSECND will use a call to the EXTERNAL FUNCTION ETIME
# TIMER    = EXT_ETIME
# For RS6K : SECOND and DSECND will use a call to the EXTERNAL FUNCTION ETIME_
# TIMER    = EXT_ETIME_
# For gfortran compiler: SECOND and DSECND will use a call to the INTERNAL FUNCTION ETIME
# TIMER    = INT_ETIME
# If your Fortran compiler does not provide etime (like Nag Fortran Compiler, etc...)
# SECOND and DSECND will use a call to the Fortran standard INTERNAL FUNCTION CPU_TIME 
TIMER    = INT_CPU_TIME
# If neither of this works...you can use the NONE value... In that case, SECOND and DSECND will always return 0
# TIMER     = NONE
#
#  The archiver and the flag(s) to use when building archive (library)
#  If you system has no ranlib, set RANLIB = echo.
#
ARCH     = ar
ARCHFLAGS= cr
RANLIB   = ranlib
#
#  The location of BLAS library for linking the testing programs.
#  The target's machine-specific, optimized BLAS library should be
#  used whenever possible.
#
BLASLIB      = ../../blas$(PLAT).a
#
#  Location of the extended-precision BLAS (XBLAS) Fortran library
#  used for building and testing extended-precision routines.  The
#  relevant routines will be compiled and XBLAS will be linked only if
#  USEXBLAS is defined.
#
# USEXBLAS    = Yes
XBLASLIB     =
# XBLASLIB    = -lxblas
#
#  Names of generated libraries.
#
LAPACKLIB    = lapack$(PLAT).a
F2CLIB       = ../../F2CLIBS/libf2c.a
TMGLIB       = tmglib$(PLAT).a
EIGSRCLIB    = eigsrc$(PLAT).a
LINSRCLIB    = linsrc$(PLAT).a
F2CLIB		 = ../../F2CLIBS/libf2c.a
