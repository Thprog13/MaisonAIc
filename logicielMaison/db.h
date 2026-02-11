#pragma once
#include <sqlite3.h>
#include <stddef.h>

int db_init(sqlite3 **out_db, const char *filename);
void db_close(sqlite3 *db);

int db_get_latest(sqlite3 *db, double *t, double *h, double *l, double *r, char *ts_buf, size_t ts_buf_sz);
int db_insert_reading(sqlite3 *db, double t, double h, double l, double r);
