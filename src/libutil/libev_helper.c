/*-
 * Copyright 2019 Vsevolod Stakhov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "libev_helper.h"

static void
rspamd_ev_watcher_io_cb (EV_P_ struct ev_io *w, int revents)
{
	struct rspamd_io_ev *ev = (struct rspamd_io_ev *)w->data;

	ev->last_activity = ev_now (EV_A);
	ev->cb (ev->io.fd, revents, ev->ud);
}

static void
rspamd_ev_watcher_timer_cb (EV_P_ struct ev_timer *w, int revents)
{
	struct rspamd_io_ev *ev = (struct rspamd_io_ev *)w->data;

	ev_tstamp after = ev->last_activity - ev_now (EV_A) + ev->timeout;

	if (after < 0.) {
		/* Real timeout */
		ev->cb (ev->io.fd, EV_TIMER, ev->ud);
	}
	else {
		/* Start another cycle as there was some activity */
		ev_timer_set (w, after, 0.);
		ev_timer_start (EV_A_ w);
	}
}


void
rspamd_ev_watcher_init (struct rspamd_io_ev *ev,
						int fd,
						short what,
						rspamd_ev_cb cb,
						void *ud)
{
	ev_io_init (&ev->io, rspamd_ev_watcher_io_cb, fd, what);
	ev->io.data = ev;
	ev_init (&ev->tm, rspamd_ev_watcher_timer_cb);
	ev->tm.data = ev;
	ev->ud = ud;
	ev->cb = cb;
}

void
rspamd_ev_watcher_start (struct ev_loop *loop,
						 struct rspamd_io_ev *ev,
						 ev_tstamp timeout)
{
	ev->last_activity = ev_now (EV_A);
	ev_io_start (EV_A_ &ev->io);

	if (timeout > 0) {
		ev->timeout = timeout;
		ev_timer_set (&ev->tm, timeout, 0.0);
		ev_timer_start (EV_A_ &ev->tm);
	}
}

void
rspamd_ev_watcher_stop (struct ev_loop *loop,
						struct rspamd_io_ev *ev)
{
	if (ev_is_pending (&ev->io) || ev_is_active (&ev->io)) {
		ev_io_stop (EV_A_ &ev->io);
	}

	if (ev->timeout > 0) {
		ev_timer_stop (EV_A_ &ev->tm);
	}
}

void
rspamd_ev_watcher_reschedule (struct ev_loop *loop,
							  struct rspamd_io_ev *ev,
							  short what)
{
	if (ev_is_pending (&ev->io) || ev_is_active (&ev->io)) {
		ev_io_stop (EV_A_ &ev->io);
		ev_io_set (&ev->io, ev->io.fd, what);
		ev_io_start (EV_A_ &ev->io);
	}
	else {
		ev_io_set (&ev->io, ev->io.fd, what);
		ev_io_start (EV_A_ &ev->io);
	}
}