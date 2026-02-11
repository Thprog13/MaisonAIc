#include "simulator.h"
#include "db.h"
#include <stdlib.h>

static double clamp(double v, double lo, double hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static gboolean simulator_cb(gpointer user_data) {
    AppState *st = (AppState*)user_data;

    double t=0, h=0, l=0, r=0;
    char ts[64] = {0};

    if (!db_get_latest(st->db, &t, &h, &l, &r, ts, sizeof(ts))) {
        return G_SOURCE_CONTINUE;
    }

    double dt = ((rand() % 21) - 10) / 20.0;  // -0.5 à +0.5
    double dh = ((rand() % 21) - 10) / 10.0;  // -1.0 à +1.0
    double dl = ((rand() % 101) - 50) * 3.0;  // -150 à +150
    double dr = ((rand() % 6) - 2) * 0.5;     // -1.0 à +2.5 (pluie en mm)

    t = clamp(t + dt, 15.0, 35.0);
    h = clamp(h + dh, 20.0, 80.0);
    l = clamp(l + dl, 0.0, 2000.0);
    r = clamp(r + dr, 0.0, 100.0);

    db_insert_reading(st->db, t, h, l, r);
    return G_SOURCE_CONTINUE;
}

void simulator_start(AppState *st, unsigned interval_seconds) {
    g_timeout_add_seconds(interval_seconds, simulator_cb, st);
}
