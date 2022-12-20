/*
 * Copyright (c) 2015 Andreas Schneider <asn@samba.org>
 * Copyright (c) 2015 Jakub Hrozek <jakub.hrozek@posteo.se>
 * Copyright (c) 2020 Bastien Nocera <hadess@hadess.net>
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

#ifndef discard_const
#define discard_const(ptr) ((void *)((uintptr_t)(ptr)))
#endif

#ifndef discard_const_p
#define discard_const_p(type, ptr) ((type *)discard_const(ptr))
#endif

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

#define ERROR_KEY	"error"
#define INFO_KEY	"info"
#define NUM_LINES_KEY	"num_lines="

#define DEFAULT_NUM_LINES 3

/* We only return up to 16 messages from the PAM conversation.
 * Value from src/python/pypamtest.c */
#define PAM_CONV_MSG_MAX        16

#define PAM_CHATTY_FLG_ERROR	(1 << 0)
#define PAM_CHATTY_FLG_INFO	(1 << 1)

static int pam_chatty_conv(pam_handle_t *pamh,
			   const int msg_style,
			   const char *msg)
{
	int ret;
	const struct pam_conv *conv = NULL;
	const struct pam_message *mesg[1];
	struct pam_response *r = NULL;
	struct pam_message *pam_msg = NULL;

	ret = pam_get_item(pamh, PAM_CONV, (const void **) &conv);
	if (ret != PAM_SUCCESS) {
		return ret;
	}

	pam_msg = calloc(sizeof(struct pam_message), 1);
	if (pam_msg == NULL) {
		return PAM_BUF_ERR;
	}

	pam_msg->msg_style = msg_style;
	pam_msg->msg = discard_const_p(char, msg);

	mesg[0] = (const struct pam_message *) pam_msg;
	ret = conv->conv(1, mesg, &r, conv->appdata_ptr);
	free(pam_msg);

	return ret;
}

/* Evaluate command line arguments and store info about them in the
 * pam_matrix context
 */
static uint32_t parse_args(int argc,
			   const char *argv[],
			   unsigned int *num_lines)
{
	uint32_t flags = 0;

	*num_lines = DEFAULT_NUM_LINES;

	for (; argc-- > 0; ++argv) {
		if (strncmp(*argv, NUM_LINES_KEY, strlen(NUM_LINES_KEY)) == 0) {
			if (*(*argv+strlen(NUM_LINES_KEY)) != '\0') {
				*num_lines = atoi(*argv+strlen(NUM_LINES_KEY));
				if (*num_lines <= DEFAULT_NUM_LINES)
					*num_lines = DEFAULT_NUM_LINES;
				if (*num_lines > PAM_CONV_MSG_MAX)
					*num_lines = PAM_CONV_MSG_MAX;
			}
		} else if (strncmp(*argv, ERROR_KEY,
				   strlen(ERROR_KEY)) == 0) {
			flags |= PAM_CHATTY_FLG_ERROR;
		} else if (strncmp(*argv, INFO_KEY,
				   strlen(INFO_KEY)) == 0) {
			flags |= PAM_CHATTY_FLG_INFO;
		}
	}

	return flags;
}

PAM_EXTERN int
pam_sm_authenticate(pam_handle_t *pamh, int flags,
		    int argc, const char *argv[])
{
	uint32_t optflags;
	unsigned int num_lines;

	(void) flags;	/* unused */

	optflags = parse_args (argc, argv, &num_lines);
	if (optflags & PAM_CHATTY_FLG_INFO) {
		unsigned int i;

		for (i = 0; i < num_lines; i++) {
			pam_chatty_conv(pamh,
					PAM_TEXT_INFO,
					"Authentication succeeded");
		}
	}

	if (optflags & PAM_CHATTY_FLG_ERROR) {
		unsigned int i;

		for (i = 0; i < num_lines; i++) {
			pam_chatty_conv(pamh,
					PAM_ERROR_MSG,
					"Authentication generated an error");
		}
	}

	return PAM_SUCCESS;
}
