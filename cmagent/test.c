#include <event2/event.h>
#include <assert.h>
#include <signal.h>
#include <stdio.h>

static int nr = 0;

void cb_func(evutil_socket_t fd, short what , void *arg)
{
    struct event **ev = arg;
    if (what & EV_SIGNAL) {
        printf("%s\n", "sighup\n");
    } else if (what & EV_TIMEOUT) {
        if (++nr == 1) {
            printf("Deleting timer\n");
            assert(event_del(*ev) != -1);
        } else {
            printf("%s\n", "TIMEOUT");
        }
    }
}


int main(void)
{
    int i, fd;
    struct event_base *base = event_base_new();
    struct event *ev = NULL;
    struct event *ev_sig = NULL;
    struct timeval five_seconds = {3, 0};
    const char ** m = event_get_supported_methods();
    ev = event_new(base, -1, EV_PERSIST, cb_func, &ev);
    ev_sig = event_new(base, SIGHUP, EV_SIGNAL | EV_PERSIST, cb_func, NULL);
    event_add(ev, &five_seconds);
    event_add(ev_sig, NULL);
    event_base_dispatch(base);

    return 0;
}
