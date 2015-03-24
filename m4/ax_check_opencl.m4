# ===========================================================================
#   http://www.gnu.org/software/autoconf-archive/ax_check_compile_flag.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_CHECK_OPENCL
#
# DESCRIPTION
#
#   Check whether OpenCL path to the headers and libraries are correctly specified.
#   Also checks that the library version is OpenCL 1.1 or greater.
#
# LICENSE
#
#   Copyright (c) 2015 Jorge Bellon <jbellon@bsc.es>
#
#   This program is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation, either version 3 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <http://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

AC_DEFUN([AX_CHECK_OPENCL],
[
AC_PREREQ(2.59)dnl for _AC_LANG_PREFIX

#Check if an OpenCL implementation is installed.
AC_ARG_WITH(opencl,
[AS_HELP_STRING([--with-opencl,--with-opencl=PATH],
                [search in system directories or specify prefix directory for installed OpenCL package.])])
AC_ARG_WITH(opencl-include,
[AS_HELP_STRING([--with-opencl-include=PATH],
                [specify directory for installed OpenCL include files])])
AC_ARG_WITH(opencl-lib,
[AS_HELP_STRING([--with-opencl-lib=PATH],
                [specify directory for the installed OpenCL library])])

# If the user specifies --with-opencl, $with_opencl value will be 'yes'
#                       --without-opencl, $with_opencl value will be 'no'
#                       --with-opencl=somevalue, $with_opencl value will be 'somevalue'
if [[[ ! "x$with_opencl" =~  x"yes"|"no"|"" ]]]; then
  openclinc="-I$with_opencl/include"
  AC_CHECK_FILE([$with_opencl/lib64],
    [opencllib=-L$with_opencl/lib64],
    [opencllib=-L$with_opencl/lib])
fi

if test $with_opencl_include; then
  openclinc="-I$with_opencl_include"
fi

if test $with_opencl_lib; then
  opencllib="-L$with_opencl_lib"
fi

# This is fulfilled even if $with_opencl="yes" 
# This happens when user leaves --with-value empty
# In this case, both openclinc and opencllib will be empty
# so the test should search in default locations and LD_LIBRARY_PATH
if test "x$with_opencl" != xno -a "x$with_opencl$with_opencl_include$with_opencl_lib" != x; then
    #tests if provided headers and libraries are usable and correct
    bak_CFLAGS="$CFLAGS"
    bak_CxXFLAGS="$CXXFLAGS"
    bak_CPPFLAGS="$CPPFLAGS"
    bak_LIBS="$LIBS"
    bak_LDFLAGS="$LDFLAGS"

    CFLAGS=
    CXXFLAGS=
    CPPFLAGS=$openclinc
    LIBS=
    LDFLAGS=$opencllib

    # One of the following two header files has to exist
    AC_CHECK_HEADERS([CL/opencl.h OpenCL/opencl.h], [opencl=yes; break])
    # Look for clGetPlatformIDs function in either libmali.so or libOpenCL.so libraries
    if test x$opencl = xyes; then
        AC_SEARCH_LIBS([clGetPlatformIDs],
                  [mali OpenCL],
                  [opencl=yes],
                  [opencl=no])
    fi

    if test x$opencl = xyes; then
      AC_CACHE_CHECK([OpenCL version],[ac_cv_opencl_version],
        [AC_RUN_IFELSE(
          [AC_LANG_PROGRAM(
            [
               #ifdef HAVE_CL_OPENCL_H
                   #include <CL/opencl.h>
               #elif HAVE_OPENCL_OPENCL_H
                   #include <OpenCL/opencl.h>
               #endif
               #include <stdio.h>
               #include <stdlib.h>

               cl_int err;
               cl_platform_id platform = 0;
               cl_device_id device = 0;
               size_t len;
               char *ocl_ver;
               int ret = 0;
            ],
            [
               /* Setup OpenCL environment. */
               err = clGetPlatformIDs(1, &platform, NULL);
               if (err != CL_SUCCESS) {
                   printf( "clGetPlatformIDs() failed with %d\n", err );
                   return 1;
               }
               
               err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, NULL);
               if (err != CL_SUCCESS) {
                   printf( "clGetDeviceIDs() failed with %d\n", err );
                   return 1;
               }
               
               err = clGetDeviceInfo(device,  CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &len);
               ocl_ver = (char *)malloc(sizeof(char)*len);
               err = clGetDeviceInfo(device,  CL_DEVICE_OPENCL_C_VERSION, len, ocl_ver, NULL);

               FILE* out = fopen("conftest.out","w");
               fprintf(out,"%s\n", ocl_ver);
               fclose(out);
               
               free(ocl_ver);
               return ret;
            ])],
          [ac_cv_opencl_version=$(cat conftest.out)
          ],
          [AC_MSG_FAILURE([
------------------------------
OpenCL version test execution failed
------------------------------])
          ])
        ])
      ac_cv_opencl_version=$(expr "x$ac_cv_opencl_version" : 'xOpenCL [a-zA-Z\+]* \(.*\)$')
    fi

    opencllib="$opencllib $LIBS"

    CFLAGS="$bak_CFLAGS"
    CPPFLAGS="$bak_CPPFLAGS"
    LIBS="$bak_LIBS"
    LDFLAGS="$bak_LDFLAGS"

    if test x$opencl != xyes; then
        AC_MSG_ERROR([
------------------------------
OpenCL path was not correctly specified. 
Please, check that the provided directories are correct.
------------------------------])
    fi

    AX_COMPARE_VERSION([$ac_cv_opencl_version],[lt],[1.1],[opencl=no])
    if test x$opencl != xyes; then
      AC_MSG_ERROR([
------------------------------
Version of the provided OpenCL package is too old.
OpenCL 1.1 or greater is required.
------------------------------])
    fi
        
fi

if test x$opencl = xyes; then
    ARCHITECTURES="$ARCHITECTURES opencl"

    AC_DEFINE([OpenCL_DEV],[],[Indicates the presence of the OpenCL arch plugin.])
    AC_DEFINE([CL_USE_DEPRECATED_OPENCL_2_0_APIS],[],[Disables warnings when using functions deprecated in OpenCL 2.0])
fi

AM_CONDITIONAL([OPENCL_SUPPORT],[test x$opencl = xyes])

AC_SUBST([opencl])
AC_SUBST([openclinc])
AC_SUBST([opencllib])

])dnl AX_CHECK_OPENCL
