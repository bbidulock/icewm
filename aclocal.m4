dnl ice_ARG_WITH(PACKAGE, HELP-STRING, ACTION-IF-TRUE [, ACTION-IF-FALSE])
dnl This macro does the same thing as AC_ARG_WITH, but it also defines
dnl with_PACKAGE_arg and with_PACKAGE_sign to avoid complicated logic later.
AC_DEFUN(ice_ARG_WITH, [
AC_ARG_WITH([$1], [$2], [
case "[${with_]patsubst([$1], -, _)}" in
[no)]
 [with_]patsubst([$1], -, _)_sign=no
 ;;
[yes)]
 [with_]patsubst([$1], -, _)_sign=yes
 ;;
[*)]
 [with_]patsubst([$1], -, _)_arg="[${with_]patsubst([$1], -, _)}"
 [with_]patsubst([$1], -, _)_sign=yes
 ;;
esac
$3
], [$4])])

dnl ice_CXX_FLAG_ACCEPT(name,FLAG)
dnl checking whether the C++ accepts FLAG and add this flag to CXXFLAGS
AC_DEFUN(ice_CXX_FLAG_ACCEPT, [
ice_save_CXXFLAGS=$CXXFLAGS
CXXFLAGS="$2 $CXXFLAGS"
AC_MSG_CHECKING(
  [whether the C++ compiler ($CXX) accepts $1])
AC_TRY_COMPILE(, , [ice_tmp_result=yes],
  [ice_tmp_result=no; CXXFLAGS=$ice_save_CXXFLAGS])
AC_MSG_RESULT([$]ice_tmp_result)
$1_ok=$ice_tmp_result
])

dnl ice_PROG_CXX_LIGHT
dnl Checking for C in hope that it understands C++ too
dnl Useful for C++ programs which don't use C++ library at all
AC_DEFUN(ice_PROG_CXX_LIGHT, [
AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING([whether the C compiler ($CC) understands C++])
cat > conftest.C <<EOF
#ifdef __cplusplus
  int main() {return 0;}
#else
  bogus_command;
#endif
EOF
if AC_TRY_COMMAND(rm conftest; $CC -o conftest conftest.C; test -f ./conftest) >/dev/null 2>&1; then
  ice_prog_gxx=yes
  CXX=$CC
else
  ice_prog_gxx=no
fi
AC_MSG_RESULT($ice_prog_gxx)
])


dnl @synopsis AC_DEFINE_DIR(VARNAME, DIR [, DESCRIPTION])
dnl
dnl This macro defines (with AC_DEFINE) VARNAME to the expansion of the DIR
dnl variable, taking care of fixing up ${prefix} and such.
dnl
dnl Note that the 3 argument form is only supported with autoconf 2.13 and
dnl later (i.e. only where AC_DEFINE supports 3 arguments).
dnl
dnl Examples:
dnl
dnl    AC_DEFINE_DIR(DATADIR, datadir)
dnl    AC_DEFINE_DIR(PROG_PATH, bindir, [Location of installed binaries])
dnl
dnl @version $Id$
dnl @author Alexandre Oliva <oliva@lsd.ic.unicamp.br>

AC_DEFUN([AC_DEFINE_DIR], [
	ac_expanded=`(
	    test "x$prefix" = xNONE && prefix="$ac_default_prefix"
	    test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
	    eval echo \""[$]$2"\"
        )`
	ifelse($3, ,
	  AC_DEFINE_UNQUOTED($1, "$ac_expanded"),
	  AC_DEFINE_UNQUOTED($1, "$ac_expanded", $3))
])
