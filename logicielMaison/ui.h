#pragma once
#include "app_state.h"

GtkApplication* ui_create_app(void);
void ui_set_app_state(AppState *st);
