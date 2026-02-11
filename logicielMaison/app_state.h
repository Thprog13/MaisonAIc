#pragma once
#include <gtk/gtk.h>
#include <sqlite3.h>

typedef struct {
    sqlite3 *db;
    GtkLabel *temp_value;
    GtkLabel *hum_value;
    GtkLabel *lux_value;
    GtkLabel *rain_value;
    GtkLabel *status_label;
} AppState;
