dnl ICE_ARG_WITH(PACKAGE, HELP-STRING, ACTION-IF-TRUE [, ACTION-IF-FALSE])
dnl This macro does the same thing as AC_ARG_WITH, but it also defines
dnl with_PACKAGE_arg and with_PACKAGE_sign to avoid complicated logic later.
dnl
AC_DEFUN([ICE_ARG_WITH], [
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


dnl ICE_CXX_FLAG_ACCEPT(name,FLAG)
dnl checking whether the C++ accepts FLAG and add this flag to CXXFLAGS
dnl
AC_DEFUN([ICE_CXX_FLAG_ACCEPT], [
ice_save_CXXFLAGS=$CXXFLAGS
CXXFLAGS="$2 $CXXFLAGS"
AC_MSG_CHECKING(
  [whether the C++ compiler ($CXX) accepts $1])
AC_TRY_COMPILE(, , [ice_tmp_result=yes],
  [ice_tmp_result=no; CXXFLAGS=$ice_save_CXXFLAGS])
AC_MSG_RESULT([$]ice_tmp_result)
$1_ok=$ice_tmp_result
])


dnl ICE_PROG_CXX_LIGHT
dnl Checking for C in hope that it understands C++ too
dnl Useful for C++ programs which don't use C++ library at all
AC_DEFUN([ICE_PROG_CXX_LIGHT], [
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


dnl ICE_EXPAND(variable[, value])
dnl Expands the value of variable.
dnl
AC_DEFUN([ICE_EXPAND], [
  ice_stored_prefix="${prefix}"
  test "${prefix}" = "NONE" && prefix="${ac_default_prefix}"

  ice_stored_exec_prefix="${exec_prefix}"
  test "${exec_prefix}" = "NONE" && exec_prefix="${prefix}"

  $1=`eval echo \""ifelse($2,,[$]$1,$2)"\"`
  
  ice_previous_value=''
  until test "${ice_previous_value}" = "[$]{$1}"; do
    ice_previous_value="[$]{$1}"
    $1=`eval echo "[$]{$1}"`
  done
  
  prefix="${ice_stored_prefix}"
  exec_prefix="${ice_stored_exec_prefix}"
])

dnl ICE_MSG_VALUE(label, variable)
dnl Prints the expanded value of variable prefixed by label.
dnl
AC_DEFUN([ICE_MSG_VALUE], [(
  ICE_EXPAND(ice_value, [$]$2)
  AC_MSG_RESULT([$1: $ice_value])
)])


dnl ICE_CHECK_NL_ITEM(nl-item,[if-found[, if-not-found]])
dnl Checks if nl_langinfo supports the requested locale dependent property.
dnl When nl-item is supported and if-found is not defined the shell variable
dnl have_$(nl_item) and the preprocessor macro HAVE_$(NL_ITEM) are set to
dnl yes/1. When nl-item is not supported and if-not-found is not defined
dnl have_$(nl_item) is set to no.
dnl
AC_DEFUN([ICE_CHECK_NL_ITEM], [
  AC_MSG_CHECKING([whether nl_langinfo supports $1])
  AC_TRY_COMPILE([
    #include <langinfo.h>
    #include <stdio.h>],
    [ printf("%s\n", nl_langinfo($1));],
    [ AC_MSG_RESULT(yes)
      ifelse([$2],,
      	     have_$1=yes
	     AC_DEFINE(HAVE_$1, 1, [define if nl_langinfo supports $1]),
	     [$2]) ],
    [ AC_MSG_RESULT(no)
      ifelse([$3],,have_$1=no,[$3]) ])
])


dnl ICE_CHECK_CONVERSION(from,to,result-if-cross-compiling[, extra-libs
dnl			 [, if-supported[, if-not-supported]]])
dnl Checks if iconv supports the requested conversion.
dnl
AC_DEFUN([ICE_CHECK_CONVERSION], [
  AC_MSG_CHECKING([whether iconv converts from $1 to $2])
  AC_TRY_RUN([
    #include <iconv.h>
    #include <locale.h>
    
    int main() {
        iconv_t cd;
	setlocale(LC_ALL, "");
        cd = iconv_open("$2", "$1");
	iconv_close(cd);
	return ((iconv_t) -1 == cd);
    }],
    [ AC_MSG_RESULT(yes); ifelse([$5],,,$5) ],
    [ AC_MSG_RESULT(no); ifelse([$6],,,$6) ],
    [ ifelse([$3],yes,
      [ AC_MSG_RESULT([yes (cross)]); ifelse([$5],,,$5) ],
      [ AC_MSG_RESULT([no (cross)]); ifelse([$6],,,$6) ])])])
