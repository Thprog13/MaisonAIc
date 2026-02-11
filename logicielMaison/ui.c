#include "ui.h"
#include "db.h"
#include <stdio.h>

static AppState *G_ST = NULL;

static void apply_css(void) {
    const char *css =
        "* { font-family: Segoe UI, Arial, sans-serif; }"
        "window { background: #0b1220; }"
        ".header { color: #e7eefc; font-size: 22px; font-weight: 700; }"
        ".sub { color: #b9c7e6; font-size: 13px; }"
        ".card { background: rgba(255,255,255,0.06); border-radius: 16px; border: 1px solid rgba(255,255,255,0.10); }"
        ".card-title { color: #b9c7e6; font-size: 14px; font-weight: 600; }"
        ".card-value { color: #e7eefc; font-size: 34px; font-weight: 800; }"
        ".status { color: #b9c7e6; font-size: 12px; }";

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, css);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

static GtkWidget* make_card(const char *title, GtkLabel **out_value_label) {
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_widget_add_css_class(frame, "card");

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(box, 14);
    gtk_widget_set_margin_bottom(box, 14);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);

    GtkWidget *lbl_title = gtk_label_new(title);
    gtk_widget_add_css_class(lbl_title, "card-title");
    gtk_label_set_xalign(GTK_LABEL(lbl_title), 0.0f);

    GtkWidget *lbl_value = gtk_label_new("--");
    gtk_widget_add_css_class(lbl_value, "card-value");
    gtk_label_set_xalign(GTK_LABEL(lbl_value), 0.0f);

    gtk_box_append(GTK_BOX(box), lbl_title);
    gtk_box_append(GTK_BOX(box), lbl_value);
    gtk_frame_set_child(GTK_FRAME(frame), box);

    *out_value_label = GTK_LABEL(lbl_value);
    return frame;
}

static gboolean refresh_ui_cb(gpointer user_data) {
    AppState *st = (AppState*)user_data;

    double t = 0, h = 0, l = 0, r = 0;
    char ts[64] = {0};

    if (!db_get_latest(st->db, &t, &h, &l, &r, ts, sizeof(ts))) {
        gtk_label_set_text(st->status_label, "Statut: erreur lecture SQLite");
        return G_SOURCE_CONTINUE;
    }

    char buf[64];

    snprintf(buf, sizeof(buf), "%.1f °C", t);
    gtk_label_set_text(st->temp_value, buf);

    snprintf(buf, sizeof(buf), "%.1f %%", h);
    gtk_label_set_text(st->hum_value, buf);

    snprintf(buf, sizeof(buf), "%.0f lux", l);
    gtk_label_set_text(st->lux_value, buf);

    snprintf(buf, sizeof(buf), "%.1f mm", r);
    gtk_label_set_text(st->rain_value, buf);

    char status[128];
    snprintf(status, sizeof(status), "Statut: OK (dernière maj: %s)", ts[0] ? ts : "inconnue");
    gtk_label_set_text(st->status_label, status);

    return G_SOURCE_CONTINUE;
}

static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    if (!G_ST) return;

    apply_css();

    GtkWidget *win = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(win), "Dashboard Capteurs (SQLite)");
    gtk_window_set_default_size(GTK_WINDOW(win), 980, 480);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_top(root, 18);
    gtk_widget_set_margin_bottom(root, 18);
    gtk_widget_set_margin_start(root, 18);
    gtk_widget_set_margin_end(root, 18);

    GtkWidget *main_title = gtk_label_new("Maison Intelligente");
    gtk_widget_add_css_class(main_title, "header");
    gtk_label_set_xalign(GTK_LABEL(main_title), 0.5f);
    
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *title = gtk_label_new("Dashboard capteurs");
    gtk_widget_add_css_class(title, "header");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);

    GtkWidget *subtitle = gtk_label_new("Mise à jour automatique en temps réel");
    gtk_widget_add_css_class(subtitle, "sub");
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0f);

    gtk_box_append(GTK_BOX(header), title);
    gtk_box_append(GTK_BOX(header), subtitle);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 14);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 14);

    GtkWidget *card1 = make_card("Température", &G_ST->temp_value);
    GtkWidget *card2 = make_card("Humidité", &G_ST->hum_value);
    GtkWidget *card3 = make_card("Luminosité", &G_ST->lux_value);
    GtkWidget *card4 = make_card("Pluie (mm)", &G_ST->rain_value);

    gtk_grid_attach(GTK_GRID(grid), card1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card2, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card3, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card4, 3, 0, 1, 1);

    G_ST->status_label = GTK_LABEL(gtk_label_new("Statut: démarrage..."));
    gtk_widget_add_css_class(GTK_WIDGET(G_ST->status_label), "status");
    gtk_label_set_xalign(G_ST->status_label, 0.0f);

    gtk_box_append(GTK_BOX(root), main_title);
    gtk_box_append(GTK_BOX(root), header);
    gtk_box_append(GTK_BOX(root), grid);
    gtk_box_append(GTK_BOX(root), GTK_WIDGET(G_ST->status_label));

    gtk_window_set_child(GTK_WINDOW(win), root);
    gtk_window_present(GTK_WINDOW(win));

    g_timeout_add_seconds(1, refresh_ui_cb, G_ST);
}

GtkApplication* ui_create_app(void) {
    GtkApplication *app = gtk_application_new("com.example.sensordash", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    return app;
}

void ui_set_app_state(AppState *st) {
    G_ST = st;
}
