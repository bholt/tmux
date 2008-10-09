/* $Id: cmd-select-prompt.c,v 1.3 2008-09-29 16:36:56 nicm Exp $ */

/*
 * Copyright (c) 2008 Nicholas Marriott <nicm@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <stdlib.h>

#include "tmux.h"

/*
 * Prompt for window index and select it.
 */

void	cmd_select_prompt_exec(struct cmd *, struct cmd_ctx *);

void	cmd_select_prompt_callback(void *, char *);

const struct cmd_entry cmd_select_prompt_entry = {
	"select-prompt", NULL,
	CMD_TARGET_CLIENT_USAGE,
	0,
	cmd_target_init,
	cmd_target_parse,
	cmd_select_prompt_exec,
	cmd_target_send,
	cmd_target_recv,
	cmd_target_free,
	cmd_target_print
};

void
cmd_select_prompt_exec(struct cmd *self, struct cmd_ctx *ctx)
{
	struct cmd_target_data	*data = self->data;
	struct client		*c;

	if ((c = cmd_find_client(ctx, data->target)) == NULL)
		return;

	if (c->prompt_string != NULL)
		return;

	server_set_client_prompt(c, "index ", cmd_select_prompt_callback, c);

	if (ctx->cmdclient != NULL)
		server_write_client(ctx->cmdclient, MSG_EXIT, NULL, 0);
}

void
cmd_select_prompt_callback(void *data, char *s)
{
	struct client	*c = data;
	struct winlink	*wl;
	const char	*errstr;
	char		 msg[128];
	u_int		 idx;

	if (s == NULL)
		return;

	idx = strtonum(s, 0, UINT_MAX, &errstr);
	if (errstr != NULL) {
		xsnprintf(msg, sizeof msg, "Index %s: %s", errstr, s);
		server_set_client_message(c, msg);
		return;
	}

	if ((wl = winlink_find_by_index(&c->session->windows, idx)) == NULL) {
		xsnprintf(msg, sizeof msg,
		    "Window not found: %s:%d", c->session->name, idx);
		server_set_client_message(c, msg);
		return;
	}

	if (session_select(c->session, idx) == 0)
		server_redraw_session(c->session);
	recalculate_sizes();
}