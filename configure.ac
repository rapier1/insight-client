#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(insight, 0.1, rapier@psc.edu)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([./])
AC_CONFIG_HEADERS([config.h])

LDFLAGS="$LDFLAGS -L/usr/lib64/mysql"

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([estats], [estats_nl_client_init], [], [
	echo "Web10g Userland library (estats) is missing"
	exit -1])
AC_CHECK_LIB([maxminddb], [MMDB_lookup_string], [], [
	echo "Maxmind geoip user library (maxminddb) is missing"
	exit -1])
AC_CHECK_LIB([mysqlclient], [mysql_init], [], [
	echo "MySQL client development (mysqlclient) libraries are missing."
	exit -1])
AC_CHECK_LIB([websock], [libwebsock_init], [], [
	echo "Websocket library (websock) is missing."
	exit -1])
AC_CHECK_LIB([json-c], [json_tokener_parse_ex], [], [
	echo "JSON-C library (json-c) is missing."
	exit -1])



# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h inttypes.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/param.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memmove memset strchr strdup strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
