#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([fm], [1.0])
LT_INIT
AM_INIT_AUTOMAKE([-Wall foreign -Wno-extra-portability])
#AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC

AC_CHECK_LIB(pthread, pthread_create, ,
        [
        echo "Error, no pthread libary found." 
        exit 1
        ])

AC_CHECK_LIB(event, event_base_new, ,
        [
        echo "Error, no event libary found." 
        exit 1
        ])

AC_CHECK_LIB(jansson, json_pack, ,
        [
        echo "Error, no jansson libary found." 
        exit 1
        ])


# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.


AC_CONFIG_FILES([
                Makefile
                logging/Makefile
                net/Makefile
                cmagent/Makefile
                 ])
AC_OUTPUT
