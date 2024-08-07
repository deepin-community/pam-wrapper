/*
 * Copyright (c) 2015 Andreas Schneider <asn@samba.org>
 * Copyright (c) 2015 Jakub Hrozek <jakub.hrozek@posteo.se>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif
#ifdef HAVE_SECURITY_PAM_MODULES_H
#include <security/pam_modules.h>
#endif
#ifdef HAVE_SECURITY_PAM_EXT_H
#include <security/pam_ext.h>
#endif

#include "pwrap_compat.h"
#include "libpamtest.h"

/* GCC have printf type attribute check. */
#ifdef HAVE_FUNCTION_ATTRIBUTE_FORMAT
#define PRINTF_ATTRIBUTE(a,b) __attribute__ ((__format__ (__printf__, a, b)))
#else
#define PRINTF_ATTRIBUTE(a,b)
#endif /* HAVE_FUNCTION_ATTRIBUTE_FORMAT */

#ifndef ZERO_STRUCT
#define ZERO_STRUCT(x) memset((char *)&(x), 0, sizeof(x))
#endif

struct pwrap_test_ctx {
	struct pam_conv conv;
	pam_handle_t *ph;
};

struct pwrap_conv_data {
	const char **authtoks;
	size_t authtok_index;
};

static int pwrap_conv(int num_msg, const struct pam_message **msgm,
		      struct pam_response **response,
		      void *appdata_ptr)
{
	int i;
	struct pam_response *reply;
	const char *password;
	size_t pwlen;
	struct pwrap_conv_data *cdata = (struct pwrap_conv_data *) appdata_ptr;

	if (cdata == NULL) {
		return PAM_CONV_ERR;
	}

	reply = (struct pam_response *) calloc(num_msg, sizeof(struct pam_response));
	if (reply == NULL) {
		return PAM_CONV_ERR;
	}

	for (i=0; i < num_msg; i++) {
		switch (msgm[i]->msg_style) {
		case PAM_PROMPT_ECHO_OFF:
			password = (const char *) cdata->authtoks[cdata->authtok_index];
			if (password == NULL) {
				free(reply);
				return PAM_CONV_ERR;
			}

			pwlen = strlen(password) + 1;

			cdata->authtok_index++;

			reply[i].resp = calloc(pwlen, sizeof(char));
			if (reply[i].resp == NULL) {
				free(reply);
				return PAM_CONV_ERR;
			}
			memcpy(reply[i].resp, password, pwlen);
			break;
		default:
			continue;
		}
	}

	*response = reply;
	return PAM_SUCCESS;
}

static int setup_passdb(void **state)
{
	int rv;
	const char *db;
	FILE *fp = NULL;
	char passdb_path[PATH_MAX];

	(void) state;	/* unused */

	db = getcwd(passdb_path, PATH_MAX);
	assert_non_null(db);
	assert_true(strlen(passdb_path) + sizeof("/passdb") < PATH_MAX);
	db = strncat(passdb_path, "/passdb", strlen("/passdb") + 1);

	rv = setenv("PAM_MATRIX_PASSWD", passdb_path, 1);
	assert_int_equal(rv, 0);

	fp = fopen(db, "w");
	assert_non_null(fp);

	fprintf(fp, "trinity:secret:matrix\n");
	fprintf(fp, "neo:secret:pwrap_wrong_svc");

	fflush(fp);
	fclose(fp);

	return 0;
}

static int teardown_passdb(void **state)
{
	const char *db;

	(void) state;	/* unused */

	db = getenv("PAM_MATRIX_PASSWD");
	assert_non_null(db);
	unlink(db);

	/* Don't pollute environment for other tests */
	unsetenv("PAM_MATRIX_PASSWD");

	return 0;
}

static int setup_ctx_only(void **state)
{
	struct pwrap_test_ctx *test_ctx;

	setup_passdb(NULL);

	test_ctx = malloc(sizeof(struct pwrap_test_ctx));
	assert_non_null(test_ctx);

	test_ctx->conv.conv = pwrap_conv;

	*state = test_ctx;
	return 0;
}

static int setup_noconv(void **state)
{
	struct pwrap_test_ctx *test_ctx;
	int rv;

	setup_ctx_only(state);
	test_ctx = *state;

	/* We'll get an error if the test module talks to us */
	test_ctx->conv.appdata_ptr = NULL;

	rv = pam_start("matrix", "trinity",
		       &test_ctx->conv, &test_ctx->ph);
	assert_int_equal(rv, PAM_SUCCESS);

	*state = test_ctx;
	return 0;
}

static int teardown_simple(void **state)
{
	struct pwrap_test_ctx *test_ctx;
	test_ctx = (struct pwrap_test_ctx *) *state;

	free(test_ctx);
	return 0;
}

static void test_env(void **state)
{
	const char *v;
	struct stat sb;
	int ret;

	(void) state; /* unused */

	v = getenv("PAM_WRAPPER_RUNTIME_DIR");
	assert_non_null(v);

	ret = stat(v, &sb);
	assert_int_not_equal(ret, -1);
	assert_true(S_ISDIR(sb.st_mode));
}

static void test_pam_start(void **state)
{
	int rv;
	pam_handle_t *ph;
	struct pwrap_test_ctx *test_ctx;

	test_ctx = (struct pwrap_test_ctx *) *state;
	test_ctx->conv.appdata_ptr = (void *) "testpassword";

	rv = pam_start("matrix", "trinity", &test_ctx->conv, &ph);
	assert_int_equal(rv, PAM_SUCCESS);

	rv = pam_end(ph, PAM_SUCCESS);
	assert_int_equal(rv, PAM_SUCCESS);
}

static int teardown(void **state)
{
	struct pwrap_test_ctx *test_ctx;
	int rv;

	teardown_passdb(NULL);

	test_ctx = (struct pwrap_test_ctx *) *state;

	rv = pam_end(test_ctx->ph, PAM_SUCCESS);
	assert_int_equal(rv, PAM_SUCCESS);

	return teardown_simple(state);
}

static void test_pam_authenticate(void **state)
{
	enum pamtest_err perr;
	struct pamtest_conv_data conv_data;
	const char *trinity_authtoks[] = {
		"secret",
		NULL,
	};
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_AUTHENTICATE, PAM_SUCCESS),
	};

	(void) state;	/* unused */

	ZERO_STRUCT(conv_data);
	conv_data.in_echo_off = trinity_authtoks;

	perr = run_pamtest("matrix", "trinity", &conv_data, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);
}

static void test_pam_authenticate_null_password(void **state)
{
	enum pamtest_err perr;
	struct pamtest_conv_data conv_data;
	const char *empty_authtoks[] = {
		NULL,
	};
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_AUTHENTICATE, PAM_CRED_ERR),
	};

	(void) state;	/* unused */

	ZERO_STRUCT(conv_data);
	conv_data.in_echo_off = empty_authtoks;

	perr = run_pamtest("matrix", "trinity", &conv_data, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);
}

static void test_pam_authenticate_err(void **state)
{
	enum pamtest_err perr;
	struct pamtest_conv_data conv_data;
	const char *trinity_authtoks[] = {
		"wrong_password",
		NULL,
	};
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_AUTHENTICATE, PAM_AUTH_ERR),
	};

	(void) state;	/* unused */

	ZERO_STRUCT(conv_data);
	conv_data.in_echo_off = trinity_authtoks;

	perr = run_pamtest("matrix", "trinity", &conv_data, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);
}

static void test_pam_acct(void **state)
{
	enum pamtest_err perr;
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_ACCOUNT, PAM_SUCCESS),
	};

	(void) state;	/* unused */

	perr = run_pamtest("matrix", "trinity", NULL, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);
}

static void test_pam_acct_err(void **state)
{
	enum pamtest_err perr;
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_ACCOUNT, PAM_PERM_DENIED),
	};

	(void) state;	/* unused */

	perr = run_pamtest("matrix", "neo", NULL, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);
}

static inline void free_vlist(char **vlist)
{
	size_t i;

	for (i = 0; vlist[i] != NULL; i++) {
		free(vlist[i]);
	}
	free(vlist);
}

static void test_pam_env_functions(void **state)
{
	int rv;
	const char *v;
	char **vlist;
	struct pwrap_test_ctx *test_ctx;

	test_ctx = (struct pwrap_test_ctx *) *state;

	rv = pam_putenv(test_ctx->ph, "KEY=value");
	assert_int_equal(rv, PAM_SUCCESS);
	rv = pam_putenv(test_ctx->ph, "KEY2=value2");
	assert_int_equal(rv, PAM_SUCCESS);

	v = pam_getenv(test_ctx->ph, "KEY");
	assert_non_null(v);

#ifdef HAVE_OPENPAM
	/*
	 * Off-by-one error in pam_getenv()
	 * https://www.openpam.org/wiki/Errata/2019-02-22
	 */
	if (*v == '=') {
		v++;
	}
#endif
	assert_string_equal(v, "value");

	v = pam_getenv(test_ctx->ph, "KEY2");
	assert_non_null(v);

#ifdef HAVE_OPENPAM
	/*
	 * Off-by-one error in pam_getenv()
	 * https://www.openpam.org/wiki/Errata/2019-02-22
	 */
	if (*v == '=') {
		v++;
	}
#endif
	assert_string_equal(v, "value2");

	vlist = pam_getenvlist(test_ctx->ph);
	assert_non_null(vlist);
	assert_non_null(vlist[0]);
	assert_string_equal(vlist[0], "KEY=value");
	assert_non_null(vlist[1]);
	assert_string_equal(vlist[1], "KEY2=value2");
	assert_null(vlist[2]);
	free_vlist(vlist);

	rv = pam_putenv(test_ctx->ph, "KEY2=");
	assert_int_equal(rv, PAM_SUCCESS);

	vlist = pam_getenvlist(test_ctx->ph);
	assert_non_null(vlist);
	assert_non_null(vlist[0]);
	assert_string_equal(vlist[0], "KEY=value");
	assert_non_null(vlist[1]);
	assert_string_equal(vlist[1], "KEY2=");
	assert_null(vlist[2]);
	free_vlist(vlist);

#ifndef HAVE_OPENPAM
	/* OpenPAM does not support this feature */
	rv = pam_putenv(test_ctx->ph, "KEY2");
	assert_int_equal(rv, PAM_SUCCESS);

	vlist = pam_getenvlist(test_ctx->ph);
	assert_non_null(vlist);
	assert_non_null(vlist[0]);
	assert_string_equal(vlist[0], "KEY=value");
	assert_null(vlist[1]);
	free_vlist(vlist);
#endif
}

static const char *string_in_list(char **list, const char *key)
{
	if (list == NULL || key == NULL) {
		return NULL;
	}

	if (strlen(key) > 0) {
		char key_eq[strlen(key) + 1 + 1]; /* trailing = and '\0' */

		snprintf(key_eq, sizeof(key_eq), "%s=", key);
		for (size_t i = 0; list[i] != NULL; i++) {
			if (strncmp(list[i], key_eq, sizeof(key_eq)-1) == 0) {
				return list[i] + sizeof(key_eq)-1;
			}
		}
	}

	return NULL;
}

static void test_pam_session(void **state)
{
	enum pamtest_err perr;
	const char *v;
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_OPEN_SESSION, PAM_SUCCESS),
		pam_test(PAMTEST_GETENVLIST, PAM_SUCCESS),
		pam_test(PAMTEST_CLOSE_SESSION, PAM_SUCCESS),
		pam_test(PAMTEST_GETENVLIST, PAM_SUCCESS),
	};

	(void) state;	/* unused */

	perr = run_pamtest("matrix", "trinity", NULL, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);

	v = string_in_list(tests[1].case_out.envlist, "HOMEDIR");
	assert_non_null(v);
	assert_string_equal(v, "/home/trinity");

	pamtest_free_env(tests[1].case_out.envlist);

	/* environment is cleared after session close */
	assert_non_null(tests[3].case_out.envlist);
#ifdef HAVE_OPENPAM
	v = string_in_list(tests[3].case_out.envlist, "HOMEDIR");
	assert_non_null(v);
	assert_string_equal(v, "");
#else
	assert_null(tests[3].case_out.envlist[0]);
#endif
	pamtest_free_env(tests[3].case_out.envlist);
}

static void test_pam_chauthtok(void **state)
{
	enum pamtest_err perr;
	struct pamtest_conv_data conv_data;
	const char *trinity_new_authtoks[] = {
		"secret",	    /* old password */
		"new_secret",	    /* new password */
		"new_secret",	    /* verify new password */
		"new_secret",	    /* login with the new password */
		NULL,
	};
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_CHAUTHTOK, PAM_SUCCESS),
		pam_test(PAMTEST_AUTHENTICATE, PAM_SUCCESS),
	};

	(void) state;	/* unused */

	ZERO_STRUCT(conv_data);
	conv_data.in_echo_off = trinity_new_authtoks;

	perr = run_pamtest("matrix", "trinity", &conv_data, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);
}

static void test_pam_chauthtok_prelim_failed(void **state)
{
	enum pamtest_err perr;
	struct pamtest_conv_data conv_data;
	const char *trinity_new_authtoks[] = {
		"wrong_secret",	    /* old password */
		"new_secret",	    /* new password */
		"new_secret",	    /* verify new password */
		NULL,
	};
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_CHAUTHTOK, PAM_AUTH_ERR),
	};

	(void) state;	/* unused */

	ZERO_STRUCT(conv_data);
	conv_data.in_echo_off = trinity_new_authtoks;

	perr = run_pamtest("matrix", "trinity", &conv_data, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);
}

static void test_pam_chauthtok_diff_passwords(void **state)
{
	enum pamtest_err perr;
	struct pamtest_conv_data conv_data;
	const char *trinity_new_authtoks[] = {
		"wrong_secret",		    /* old password */
		"new_secret",		    /* new password */
		"different_new_secret",	    /* verify new password */
		NULL,
	};
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_CHAUTHTOK, PAM_AUTH_ERR),
	};

	(void) state;	/* unused */

	ZERO_STRUCT(conv_data);
	conv_data.in_echo_off = trinity_new_authtoks;

	perr = run_pamtest("matrix", "trinity", &conv_data, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);
}

static void test_pam_setcred(void **state)
{
	enum pamtest_err perr;
	const char *v;
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_GETENVLIST, PAM_SUCCESS),
		pam_test(PAMTEST_SETCRED, PAM_SUCCESS),
		pam_test(PAMTEST_GETENVLIST, PAM_SUCCESS),
	};

	(void) state;	/* unused */

	perr = run_pamtest("matrix", "trinity", NULL, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);

	/* environment is clean before setcred */
	assert_non_null(tests[0].case_out.envlist);
	assert_null(tests[0].case_out.envlist[0]);
	pamtest_free_env(tests[0].case_out.envlist);

	/* and has an item after setcred */
	v = string_in_list(tests[2].case_out.envlist, "CRED");
	assert_non_null(v);
	assert_string_equal(v, "/tmp/trinity");
	pamtest_free_env(tests[2].case_out.envlist);
}

static void test_pam_item_functions(void **state)
{
	struct pwrap_test_ctx *test_ctx;
	const char *item;
	int rv;

	test_ctx = (struct pwrap_test_ctx *) *state;

	rv = pam_get_item(test_ctx->ph, PAM_USER, (const void **) &item);
	assert_int_equal(rv, PAM_SUCCESS);
	assert_string_equal(item, "trinity");

	rv = pam_set_item(test_ctx->ph, PAM_USER_PROMPT, "test_login");
	assert_int_equal(rv, PAM_SUCCESS);
	assert_string_equal(item, "trinity");

	rv = pam_get_item(test_ctx->ph, PAM_USER_PROMPT, (const void **) &item);
	assert_int_equal(rv, PAM_SUCCESS);
	assert_string_equal(item, "test_login");

	rv = pam_set_item(test_ctx->ph, PAM_AUTHTOK, "mysecret");
#ifdef HAVE_OPENPAM
	/* OpenPAM allows PAM_AUTHTOK getset */
	assert_int_equal(rv, PAM_SUCCESS);
#else
	assert_int_equal(rv, PAM_BAD_ITEM);
#endif

	rv = pam_get_item(test_ctx->ph, PAM_AUTHTOK, (const void **) &item);
#ifdef HAVE_OPENPAM
	/* OpenPAM allows PAM_AUTHTOK getset */
	assert_int_equal(rv, PAM_SUCCESS);
	assert_string_equal(item, "mysecret");
#else
	assert_int_equal(rv, PAM_BAD_ITEM);
#endif

}

static int add_to_reply(struct pam_response *res,
			const char *s1,
			const char *s2)
{
	size_t res_len;
	int rv;

	res_len = strlen(s1) + strlen(s2) + 1;

	res->resp = calloc(res_len, sizeof(char));
	if (res->resp == NULL) {
		return ENOMEM;
	}

	rv = snprintf(res->resp, res_len, "%s%s", s1, s2);
	if (rv < 0) {
		return EIO;
	}

	return 0;
}

static int pwrap_echo_conv(int num_msg,
			   const struct pam_message **msgm,
			   struct pam_response **response,
			   void *appdata_ptr)
{
	int i;
	struct pam_response *reply;
	int *resp_array = appdata_ptr;

	reply = (struct pam_response *) calloc(num_msg, sizeof(struct pam_response));
	if (reply == NULL) {
		return PAM_CONV_ERR;
	}

	for (i=0; i < num_msg; i++) {
		switch (msgm[i]->msg_style) {
		case PAM_PROMPT_ECHO_OFF:
			add_to_reply(&reply[i], "echo off: ", msgm[i]->msg);
			break;
		case PAM_PROMPT_ECHO_ON:
			add_to_reply(&reply[i], "echo on: ", msgm[i]->msg);
			break;
		case PAM_TEXT_INFO:
			resp_array[0] = 1;
			break;
		case PAM_ERROR_MSG:
			resp_array[1] = 1;
			break;
		default:
			break;
		}
	}

	*response = reply;
	return PAM_SUCCESS;
}

static int vprompt_test_fn(pam_handle_t *pamh, int style,
			   char **response, const char *fmt, ...) PRINTF_ATTRIBUTE(4, 5);

static int vprompt_test_fn(pam_handle_t *pamh, int style,
			   char **response, const char *fmt, ...)
{
	va_list args;
	int rv;

	va_start(args, fmt);
	rv = pam_vprompt(pamh, style, response, fmt, args);
	va_end(args);

	return rv;  
}

static void test_pam_prompt(void **state)
{
	struct pwrap_test_ctx *test_ctx;
	int rv;
	char *response;
	int resp_array[2];

	test_ctx = (struct pwrap_test_ctx *) *state;

	memset(resp_array, 0, sizeof(resp_array));

	test_ctx->conv.conv = pwrap_echo_conv;
	test_ctx->conv.appdata_ptr = resp_array;

	rv = pam_start("matrix", "trinity",
		       &test_ctx->conv, &test_ctx->ph);
	assert_int_equal(rv, PAM_SUCCESS);

	rv = pam_prompt(test_ctx->ph, PAM_PROMPT_ECHO_OFF, &response, "no echo");
	assert_int_equal(rv, PAM_SUCCESS);
	assert_string_equal(response, "echo off: no echo");
	free(response);

	rv = vprompt_test_fn(test_ctx->ph, PAM_PROMPT_ECHO_OFF, &response, "no echo");
	assert_int_equal(rv, PAM_SUCCESS);
	assert_string_equal(response, "echo off: no echo");
	free(response);

	rv = pam_prompt(test_ctx->ph, PAM_PROMPT_ECHO_ON, &response, "echo");
	assert_int_equal(rv, PAM_SUCCESS);
	assert_string_equal(response, "echo on: echo");
	free(response);

	rv = vprompt_test_fn(test_ctx->ph, PAM_PROMPT_ECHO_ON, &response, "echo");
	assert_int_equal(rv, PAM_SUCCESS);
	assert_string_equal(response, "echo on: echo");
	free(response);

	assert_int_equal(resp_array[0], 0);
	pam_info(test_ctx->ph, "info");
	assert_int_equal(resp_array[0], 1);

	assert_int_equal(resp_array[1], 0);
	pam_error(test_ctx->ph, "error");
	assert_int_equal(resp_array[1], 1);
}

static void test_pam_strerror(void **state)
{
	struct pwrap_test_ctx *test_ctx;
	const char *s = NULL;

	test_ctx = (struct pwrap_test_ctx *) *state;

	s = pam_strerror(test_ctx->ph, PAM_AUTH_ERR);
	assert_non_null(s);
}

static void test_pam_authenticate_db_opt(void **state)
{
	enum pamtest_err perr;
	struct pamtest_conv_data conv_data;
	char auth_info_msg[PAM_MAX_MSG_SIZE] = { '\0' };
	char *info_arr[] = {
		auth_info_msg,
		NULL,
	};
	const char *trinity_authtoks[] = {
		"secret_ro",
		NULL,
	};
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_AUTHENTICATE, PAM_SUCCESS),
	};

	(void) state;	/* unused */

	ZERO_STRUCT(conv_data);

	conv_data.in_echo_on = trinity_authtoks;
	conv_data.out_info = info_arr;

	perr = run_pamtest("matrix_opt", "trinity_ro", &conv_data, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);

	assert_string_equal(auth_info_msg, "Authentication succeeded");
}

static void test_pam_authenticate_db_opt_err(void **state)
{
	enum pamtest_err perr;
	struct pamtest_conv_data conv_data;
	char auth_err_msg[PAM_MAX_MSG_SIZE] = { '\0' };
	char *err_arr[] = {
		auth_err_msg,
		NULL,
	};
	const char *trinity_authtoks[] = {
		"wrong_secret",
		NULL,
	};
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_AUTHENTICATE, PAM_AUTH_ERR),
	};

	(void) state;	/* unused */

	ZERO_STRUCT(conv_data);
	conv_data.in_echo_on = trinity_authtoks;
	conv_data.out_err = err_arr;

	perr = run_pamtest("matrix_opt", "trinity_ro", &conv_data, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);

	assert_string_equal(auth_err_msg, "Authentication failed");
}


#ifdef HAVE_PAM_VSYSLOG
static void vsyslog_test_fn(const pam_handle_t *pamh,
			    int priority,
			    const char *fmt, ...) PRINTF_ATTRIBUTE(3, 4);

static void vsyslog_test_fn(const pam_handle_t *pamh,
			    int priority,
			    const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	pam_vsyslog(pamh, priority, fmt, args);
	va_end(args);
}

static void test_pam_vsyslog(void **state)
{
	struct pwrap_test_ctx *test_ctx;

	test_ctx = (struct pwrap_test_ctx *) *state;

	pam_syslog(test_ctx->ph, LOG_INFO, "This is pam_wrapper test\n");
	vsyslog_test_fn(test_ctx->ph, LOG_INFO, "This is pam_wrapper test\n");
}
#endif /* HAVE_PAM_VSYSLOG */

static void test_libpamtest_strerror(void **state)
{
	const char *s;

	(void) state;	/* unused */

	s = pamtest_strerror(PAMTEST_ERR_OK);
	assert_non_null(s);

	s = pamtest_strerror(PAMTEST_ERR_START);
	assert_non_null(s);

	s = pamtest_strerror(PAMTEST_ERR_CASE);
	assert_non_null(s);

	s = pamtest_strerror(PAMTEST_ERR_OP);
	assert_non_null(s);

	s = pamtest_strerror(PAMTEST_ERR_END);
	assert_non_null(s);

	s = pamtest_strerror(PAMTEST_ERR_KEEPHANDLE);
	assert_non_null(s);

	s = pamtest_strerror(PAMTEST_ERR_INTERNAL);
	assert_non_null(s);
}

#define test_setenv(env) setenv(env, "test_"env, 1)

#define test_getenv(envlist, key) do {				    \
	const char *__v;					    \
	__v = string_in_list(envlist, key);			    \
	assert_non_null(__v);					    \
	assert_string_equal(__v, "test_"key);			    \
} while(0);

static void test_get_set(void **state)
{
#ifndef HAVE_OPENPAM
	const char *svc;
#endif
	enum pamtest_err perr;
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_OPEN_SESSION, PAM_SUCCESS),
		pam_test(PAMTEST_GETENVLIST, PAM_SUCCESS),
	};

	(void) state;	/* unused */

#ifndef HAVE_OPENPAM
	test_setenv("PAM_SERVICE");
#endif
	test_setenv("PAM_USER");
	test_setenv("PAM_USER_PROMPT");
	test_setenv("PAM_TTY");
	test_setenv("PAM_RUSER");
	test_setenv("PAM_RHOST");
	test_setenv("PAM_AUTHTOK");
	test_setenv("PAM_OLDAUTHTOK");
#ifdef PAM_XDISPLAY
	test_setenv("PAM_XDISPLAY");
#endif
#ifdef PAM_AUTHTOK_TYPE
	test_setenv("PAM_AUTHTOK_TYPE");
#endif

	perr = run_pamtest("pwrap_get_set", "trinity", NULL, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);

	/* PAM_SERVICE is a special case, Linux's libpam lowercases it.
	 * OpenPAM only allows PAM_SERVICE to be set by pam_start()
	 */
#ifndef HAVE_OPENPAM
	svc = string_in_list(tests[1].case_out.envlist, "PAM_SERVICE");
	assert_non_null(svc);
	assert_string_equal(svc, "test_pam_service");
#endif

	test_getenv(tests[1].case_out.envlist, "PAM_USER");
	test_getenv(tests[1].case_out.envlist, "PAM_USER_PROMPT");
	test_getenv(tests[1].case_out.envlist, "PAM_TTY");
	test_getenv(tests[1].case_out.envlist, "PAM_RUSER");
	test_getenv(tests[1].case_out.envlist, "PAM_RHOST");
	test_getenv(tests[1].case_out.envlist, "PAM_AUTHTOK");
	test_getenv(tests[1].case_out.envlist, "PAM_OLDAUTHTOK");
#ifdef PAM_XDISPLAY
	test_getenv(tests[1].case_out.envlist, "PAM_XDISPLAY");
#endif
#ifdef PAM_AUTHTOK_TYPE
	test_getenv(tests[1].case_out.envlist, "PAM_AUTHTOK_TYPE");
#endif

	pamtest_free_env(tests[1].case_out.envlist);
}

static void test_libpamtest_keepopen(void **state)
{
	int rv;
	enum pamtest_err perr;
	struct pamtest_conv_data conv_data;
	const char *trinity_authtoks[] = {
		"secret",
		NULL,
	};
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_AUTHENTICATE, PAM_SUCCESS),
		pam_test(PAMTEST_KEEPHANDLE, PAM_SUCCESS),
	};

	(void) state;	/* unused */

	ZERO_STRUCT(conv_data);
	conv_data.in_echo_off = trinity_authtoks;

	perr = run_pamtest("matrix", "trinity", &conv_data, tests, NULL);
	assert_int_equal(perr, PAMTEST_ERR_OK);

	assert_non_null(tests[1].case_out.ph);

	rv = pam_end(tests[1].case_out.ph, PAM_SUCCESS);
	assert_int_equal(rv, PAM_SUCCESS);
}

static void test_libpamtest_get_failed_test(void **state)
{
	enum pamtest_err perr;
	struct pamtest_conv_data conv_data;
	const char *trinity_authtoks[] = {
		"secret",
		NULL,
	};
	struct pam_testcase tests[] = {
		pam_test(PAMTEST_AUTHENTICATE, PAM_AUTH_ERR),
	};
	const struct pam_testcase *failed_tc;

	(void) state;	/* unused */

	ZERO_STRUCT(conv_data);
	conv_data.in_echo_off = trinity_authtoks;

	perr = run_pamtest("matrix", "trinity", &conv_data, tests, NULL);
	assert_int_not_equal(perr, PAMTEST_ERR_OK);

	failed_tc = pamtest_failed_case(tests);
	assert_ptr_equal(failed_tc, &tests[0]);
}

int main(int argc, char *argv[])
{
	int rc;

	const struct CMUnitTest init_tests[] = {
		cmocka_unit_test(test_env),
		cmocka_unit_test_setup_teardown(test_pam_start,
						setup_ctx_only,
						teardown_simple),
		cmocka_unit_test_setup_teardown(test_pam_authenticate,
						setup_passdb,
						teardown_passdb),
		cmocka_unit_test_setup_teardown(test_pam_authenticate_null_password,
						setup_passdb,
						teardown_passdb),
		cmocka_unit_test_setup_teardown(test_pam_authenticate_err,
						setup_passdb,
						teardown_passdb),
		cmocka_unit_test_setup_teardown(test_pam_acct,
						setup_passdb,
						teardown_passdb),
		cmocka_unit_test_setup_teardown(test_pam_acct_err,
						setup_passdb,
						teardown_passdb),
		cmocka_unit_test_setup_teardown(test_pam_env_functions,
						setup_noconv,
						teardown),
		cmocka_unit_test_setup_teardown(test_pam_session,
						setup_passdb,
						teardown_passdb),
		cmocka_unit_test_setup_teardown(test_pam_chauthtok,
						setup_passdb,
						teardown_passdb),
		cmocka_unit_test_setup_teardown(test_pam_chauthtok_prelim_failed,
						setup_passdb,
						teardown_passdb),
		cmocka_unit_test_setup_teardown(test_pam_chauthtok_diff_passwords,
						setup_passdb,
						teardown_passdb),
		cmocka_unit_test_setup_teardown(test_pam_setcred,
						setup_passdb,
						teardown_passdb),
		cmocka_unit_test_setup_teardown(test_pam_item_functions,
						setup_noconv,
						teardown),
		cmocka_unit_test_setup_teardown(test_pam_prompt,
						setup_ctx_only,
						teardown),
		cmocka_unit_test_setup_teardown(test_pam_strerror,
						setup_noconv,
						teardown),
		cmocka_unit_test_setup_teardown(test_pam_authenticate_db_opt,
						setup_ctx_only,
						teardown_simple),
		cmocka_unit_test_setup_teardown(test_pam_authenticate_db_opt_err,
						setup_ctx_only,
						teardown_simple),
#ifdef HAVE_PAM_VSYSLOG
		cmocka_unit_test_setup_teardown(test_pam_vsyslog,
						setup_noconv,
						teardown),
#endif
		cmocka_unit_test_setup_teardown(test_libpamtest_keepopen,
						setup_passdb,
						teardown_passdb),
		cmocka_unit_test_setup_teardown(test_libpamtest_get_failed_test,
						setup_passdb,
						teardown_passdb),
		cmocka_unit_test(test_libpamtest_strerror),
		cmocka_unit_test(test_get_set),
	};

	if (argc == 2) {
		cmocka_set_test_filter(argv[1]);
	}

	rc = cmocka_run_group_tests(init_tests, NULL, NULL);

	return rc;
}

