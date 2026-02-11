#include "app_state.h"
#include "db.h"
#include "ui.h"
#include "simulator.h"
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
    srand((unsigned)time(NULL));

    AppState st = {0};

    if (!db_init(&st.db, "sensors.db")) {
        return 1;
    }

    GtkApplication *app = ui_create_app();
    ui_set_app_state(&st);

    simulator_start(&st, 2);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    db_close(st.db);
    return status;
}
