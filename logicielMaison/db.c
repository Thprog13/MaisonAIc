#include "db.h"
#include <stdio.h>

static int db_exec(sqlite3 *db, const char *sql) {
    char *err = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQLite error: %s\nSQL: %s\n", err ? err : "(no msg)", sql);
        sqlite3_free(err);
        return 0;
    }
    return 1;
}

int db_init(sqlite3 **out_db, const char *filename) {
    sqlite3 *db = NULL;
    int rc = sqlite3_open(filename, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Impossible d'ouvrir %s: %s\n", filename, sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    const char *create_sql =
        "CREATE TABLE IF NOT EXISTS readings ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  ts TEXT NOT NULL DEFAULT (datetime('now')),"
        "  temperature REAL NOT NULL,"
        "  humidity REAL NOT NULL,"
        "  luminosity REAL NOT NULL,"
        "  rainfall REAL NOT NULL DEFAULT 0.0"
        ");";
    if (!db_exec(db, create_sql)) {
        sqlite3_close(db);
        return 0;
    }

    const char *migration_sql = "ALTER TABLE readings ADD COLUMN rainfall REAL NOT NULL DEFAULT 0.0;";
    sqlite3_exec(db, migration_sql, NULL, NULL, NULL); 

    const char *seed_sql =
        "INSERT INTO readings (temperature, humidity, luminosity, rainfall) "
        "SELECT 22.5, 45.0, 320.0, 0.0 "
        "WHERE NOT EXISTS (SELECT 1 FROM readings);";
    if (!db_exec(db, seed_sql)) {
        sqlite3_close(db);
        return 0;
    }

    *out_db = db;
    return 1;
}

void db_close(sqlite3 *db) {
    if (db) sqlite3_close(db);
}

int db_get_latest(sqlite3 *db, double *t, double *h, double *l, double *r, char *ts_buf, size_t ts_buf_sz) {
    const char *sql =
        "SELECT temperature, humidity, luminosity, rainfall, ts "
        "FROM readings ORDER BY id DESC LIMIT 1;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "prepare failed: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    int ok = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *t = sqlite3_column_double(stmt, 0);
        *h = sqlite3_column_double(stmt, 1);
        *l = sqlite3_column_double(stmt, 2);
        *r = sqlite3_column_double(stmt, 3);

        const unsigned char *ts = sqlite3_column_text(stmt, 4);
        if (ts && ts_buf && ts_buf_sz > 0) {
            snprintf(ts_buf, ts_buf_sz, "%s", (const char*)ts);
        }
        ok = 1;
    }

    sqlite3_finalize(stmt);
    return ok;
}

int db_insert_reading(sqlite3 *db, double t, double h, double l, double r) {
    const char *sql = "INSERT INTO readings (temperature, humidity, luminosity, rainfall) VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt = NULL;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "prepare insert failed: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_double(stmt, 1, t);
    sqlite3_bind_double(stmt, 2, h);
    sqlite3_bind_double(stmt, 3, l);
    sqlite3_bind_double(stmt, 4, r);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "insert failed: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    return 1;
}
