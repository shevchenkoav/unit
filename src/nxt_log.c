
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) NGINX, Inc.
 */

#include <nxt_main.h>


nxt_uint_t  nxt_debug;
nxt_uint_t  nxt_trace;


nxt_log_t   nxt_main_log = {
    NXT_LOG_INFO,
    0,
    nxt_log_handler,
    NULL,
    NULL
};


nxt_str_t  nxt_log_levels[8] = {
    nxt_string("emerg"),
    nxt_string("alert"),
    nxt_string("crit"),
    nxt_string("error"),
    nxt_string("warn"),
    nxt_string("notice"),
    nxt_string("info"),
    nxt_string("debug")
};


static const u_char  *nxt_log_prefix;


void
nxt_log_start(const char *prefix)
{
    if (prefix != NULL && *prefix != '\0') {
        nxt_log_prefix = (u_char *) prefix;
    }
}


/* STUB */
nxt_log_t *
nxt_log_set_ctx(nxt_log_t *log, nxt_log_ctx_handler_t handler, void *ctx)
{
    nxt_log_t     *old;
    nxt_thread_t  *thr;

    thr = nxt_thread();
    old = thr->log;

    log->level = old->level;
    log->handler = old->handler;
    log->ctx_handler = handler;
    log->ctx = ctx;

    thr->log = log;

    return old;
}


void nxt_cdecl
nxt_log_handler(nxt_uint_t level, nxt_log_t *log, const char *fmt, ...)
{
    u_char   *p, *syslogmsg, *end;
    va_list  args;
    u_char   msg[NXT_MAX_ERROR_STR];

    p = msg;
    end = msg + NXT_MAX_ERROR_STR;

    if (nxt_log_prefix != NULL) {
        p = nxt_cpystrn(p, nxt_log_prefix, end - p);
        *p++ = ':';
        *p++ = ' ';
    }

    syslogmsg = p;

    p = nxt_sprintf(p, end, (log->ident != 0) ? "[%V] *%D " : "[%V] ",
                    &nxt_log_levels[level], log->ident);

    va_start(args, fmt);
    p = nxt_vsprintf(p, end, fmt, args);
    va_end(args);

    if (level != NXT_LOG_DEBUG && log->ctx_handler != NULL) {
        p = log->ctx_handler(log->ctx, p, end);
    }

    if (p > end - NXT_LINEFEED_SIZE) {
        p = end - NXT_LINEFEED_SIZE;
    }

    nxt_linefeed(p);

    (void) nxt_write_console(nxt_stderr, msg, p - msg);

    if (level <= NXT_LOG_ALERT) {
        *(p - NXT_LINEFEED_SIZE) = '\0';

        /*
         * Syslog LOG_ALERT level is enough, because
         * LOG_EMERG level broadcast a message to all users.
         */
        nxt_write_syslog(LOG_ALERT, syslogmsg);
    }
}
