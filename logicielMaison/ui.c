#include "ui.h"
#include "db.h"
#include <stdio.h>

static AppState *G_ST = NULL;
static GtkStack *G_STACK = NULL; 

static void apply_css(void) {
    const char *css =
        "* { font-family: Segoe UI, Arial, sans-serif; }"
        "window { background: #0b1220; }"
        ".header { color: #e7eefc; font-size: 22px; font-weight: 700; }"
        ".sub { color: #b9c7e6; font-size: 13px; }"
        ".card { background: rgba(255,255,255,0.06); border-radius: 16px; border: 1px solid rgba(255,255,255,0.10); }"
        ".card-title { color: #b9c7e6; font-size: 14px; font-weight: 600; }"
        ".card-value { color: #e7eefc; font-size: 34px; font-weight: 800; background: rgba(255,255,255,0.06); border: 1px solid rgba(255,255,255,0.10); border-radius: 8px; padding: 8px 16px; }"
        ".status { color: #b9c7e6; font-size: 12px; }"
        ".menu-bar { background: rgba(255,255,255,0.06); border-radius: 10px; border: 1px solid rgba(255,255,255,0.10); }"
        ".menu-title { color: #e7eefc; font-size: 18px; font-weight: 700; }"
        ".card-btn { background: rgba(255,255,255,0.06); border: 1px solid rgba(255,255,255,0.10); border-radius: 16px; color: white; font-weight: 700; font-size: 27px; }";

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

// Construit la vue Dashboard et renseigne les labels dans G_ST
static GtkWidget* build_dashboard_view(void) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    // Titre principal retiré

    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    GtkWidget *subtitle = gtk_label_new("Mise à jour automatique en temps réel");
    gtk_widget_add_css_class(subtitle, "sub");
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0f);

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

    gtk_box_append(GTK_BOX(page), header);
    gtk_box_append(GTK_BOX(page), grid);
    gtk_box_append(GTK_BOX(page), GTK_WIDGET(G_ST->status_label));

    return page;
}

static void on_menu_dashboard_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    if (G_STACK) {
        gtk_stack_set_visible_child_name(GTK_STACK(G_STACK), "dashboard");
    }
}

static void on_menu_home_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    if (G_STACK) {
        gtk_stack_set_visible_child_name(GTK_STACK(G_STACK), "placeholder");
    }
}

static void on_menu_devices_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn; (void)user_data;
    if (G_STACK) {
        gtk_stack_set_visible_child_name(GTK_STACK(G_STACK), "devices");
    }
}

static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    if (!G_ST) return;

    apply_css();

    GtkWidget *win = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(win), "Dashboard Capteurs (SQLite)");
    gtk_window_set_default_size(GTK_WINDOW(win), 980, 520);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_top(root, 18);
    gtk_widget_set_margin_bottom(root, 18);
    gtk_widget_set_margin_start(root, 18);
    gtk_widget_set_margin_end(root, 18);

    // Barre de menu avec titre et bouton Dashboard centré
    GtkWidget *menu_bar = gtk_center_box_new();
    gtk_widget_add_css_class(menu_bar, "menu-bar");
    gtk_widget_set_margin_bottom(menu_bar, 8);
    gtk_widget_set_margin_start(menu_bar, 4);
    gtk_widget_set_margin_end(menu_bar, 4);
    gtk_widget_set_margin_top(menu_bar, 4);

    // Zone de gauche: bouton Menu + Titre
    GtkWidget *start_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *btn_home = gtk_button_new();
    GtkWidget *home_icon = NULL;
    const char *candidates[] = {
        "assets/icons/home.svg",
        "logicielMaison/assets/icons/home.svg",
        NULL
    };
    for (int i = 0; candidates[i]; ++i) {
        if (g_file_test(candidates[i], G_FILE_TEST_IS_REGULAR)) {
            home_icon = gtk_image_new_from_file(candidates[i]);
            gtk_widget_set_size_request(home_icon, 20, 20);
            break;
        }
    }
    if (!home_icon) {
        home_icon = gtk_image_new_from_icon_name("home-symbolic");
    }
    gtk_button_set_child(GTK_BUTTON(btn_home), home_icon);
    gtk_widget_set_tooltip_text(btn_home, "Menu");
    g_signal_connect(btn_home, "clicked", G_CALLBACK(on_menu_home_clicked), NULL);
    gtk_box_append(GTK_BOX(start_box), btn_home);

    GtkWidget *lbl_title = gtk_label_new("Maison Intelligente");
    gtk_widget_add_css_class(lbl_title, "menu-title");
    gtk_center_box_set_start_widget(GTK_CENTER_BOX(menu_bar), start_box);
    // Centre: Titre centré
    gtk_center_box_set_center_widget(GTK_CENTER_BOX(menu_bar), lbl_title);

    // Droite: espace réservé (pour équilibrer)
    GtkWidget *end_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_center_box_set_end_widget(GTK_CENTER_BOX(menu_bar), end_box);

    // Zone centrale avec un GtkStack (pour naviguer entre plusieurs pages si besoin)
    G_STACK = GTK_STACK(gtk_stack_new());
    gtk_stack_set_transition_type(G_STACK, GTK_STACK_TRANSITION_TYPE_CROSSFADE);

    // Page d'accueil/placeholder avec une card centrale contenant un bouton Dashboard
    GtkWidget *placeholder = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 24);
    gtk_widget_set_hexpand(placeholder, TRUE);
    gtk_widget_set_vexpand(placeholder, TRUE);
    gtk_widget_set_halign(placeholder, GTK_ALIGN_CENTER);

    // Card 1: Capteur -> Dashboard
    GtkWidget *card1 = gtk_frame_new(NULL);
    gtk_widget_add_css_class(card1, "card");
    gtk_widget_set_halign(card1, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(card1, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(card1, 650, 400);

    GtkWidget *card1_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(card1_box, 14);
    gtk_widget_set_margin_bottom(card1_box, 14);
    gtk_widget_set_margin_start(card1_box, 16);
    gtk_widget_set_margin_end(card1_box, 16);

    GtkWidget *btn_dashboard_card = gtk_button_new_with_label("Capteur");
    gtk_widget_add_css_class(btn_dashboard_card, "card-value");
    g_signal_connect(btn_dashboard_card, "clicked", G_CALLBACK(on_menu_dashboard_clicked), NULL);

    gtk_box_append(GTK_BOX(card1_box), btn_dashboard_card);
    gtk_frame_set_child(GTK_FRAME(card1), card1_box);

    // Card 2: Objet connecté -> Nouvelle page
    GtkWidget *card2 = gtk_frame_new(NULL);
    gtk_widget_add_css_class(card2, "card");
    gtk_widget_set_halign(card2, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(card2, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(card2, 650, 400);

    GtkWidget *card2_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(card2_box, 14);
    gtk_widget_set_margin_bottom(card2_box, 14);
    gtk_widget_set_margin_start(card2_box, 16);
    gtk_widget_set_margin_end(card2_box, 16);

    GtkWidget *btn_devices_card = gtk_button_new_with_label("Objet connecté");
    gtk_widget_add_css_class(btn_devices_card, "card-value");
    g_signal_connect(btn_devices_card, "clicked", G_CALLBACK(on_menu_devices_clicked), NULL);

    gtk_box_append(GTK_BOX(card2_box), btn_devices_card);
    gtk_frame_set_child(GTK_FRAME(card2), card2_box);

    gtk_box_append(GTK_BOX(placeholder), card1);
    gtk_box_append(GTK_BOX(placeholder), card2);

    gtk_stack_add_named(G_STACK, placeholder, "placeholder");

    GtkWidget *dashboard = build_dashboard_view();
    gtk_stack_add_named(G_STACK, dashboard, "dashboard");

    // Page "Objet connecté"
    GtkWidget *devices_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    GtkWidget *devices_sub = gtk_label_new("Gérez vos objets connectés");
    gtk_widget_add_css_class(devices_sub, "sub");
    gtk_label_set_xalign(GTK_LABEL(devices_sub), 0.0f);
    gtk_box_append(GTK_BOX(devices_page), devices_sub);

    GtkWidget *devices_body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_append(GTK_BOX(devices_page), devices_body);

    gtk_stack_add_named(G_STACK, devices_page, "devices");

    // Par défaut: ne pas afficher le dashboard tant que l'utilisateur n'a pas cliqué
    gtk_stack_set_visible_child_name(G_STACK, "placeholder");

    gtk_box_append(GTK_BOX(root), menu_bar);
    gtk_box_append(GTK_BOX(root), GTK_WIDGET(G_STACK));

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
