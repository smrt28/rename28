AC_PREREQ(2.61)
AC_INIT(rename28, 1.0, ondrej.holecek@gmail.com)
AC_CONFIG_SRCDIR([src/main.cc])

m4_ifdef([AM_SILENT_RULES],
  [AM_SILENT_RULES([yes])],
  [AC_SUBST([AM_DEFAULT_VERBOSITY], [1])])

AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_MAINTAINER_MODE([enable])

LT_INIT
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CC_STDC

AC_CHECK_FUNCS([memset_s explicit_bzero])
AC_ARG_ENABLE(optimizations,
    AS_HELP_STRING([--enable-optimizations],
                   [enable optimizations, default: yes]),
             [case "${enableval}" in
             yes)
                optimizations=true
                CXXFLAGS="${CXXFLAGS} -O2"
                ;;
             no)
                optimizations=false
                CXXFLAGS="${CXXFLAGS} -O0 -g3"
                ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --enable-optimizations]) ;;
             esac],
[optimizations=false])


# check for boost program options
AC_LANG_CPLUSPLUS
AC_CHECK_HEADER([boost/program_options.hpp], [], AC_MSG_ERROR([boost program_options missing], 1))
AC_CHECK_LIB(boost_program_options-mt, main, , [ 
    AC_CHECK_LIB(boost_program_options, main, , [ 
        AC_MSG_ERROR([boost program_options library missing], 1)])])



#libcrypto

PKG_CHECK_MODULES([REMOVE28], [libcrypto])
AC_LANG_WERROR



AC_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([Makefile
                 gtest/Makefile])
AC_OUTPUT
