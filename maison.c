#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sqlite3.h>

static void die(sqlite3 *db, const char *msg) {
    fprintf(stderr, "Erreur: %s\n", msg);
    if (db) fprintf(stderr, "SQLite: %s\n", sqlite3_errmsg(db));
    exit(EXIT_FAILURE);
}

static void exec_sql(sqlite3 *db, const char *sql) {
    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err ? err : "(inconnu)");
        sqlite3_free(err);
        die(db, "Echec sqlite3_exec");
    }
}

static long long now_unix(void) {
    return (long long)time(NULL);
}

static void insert_test_data(sqlite3 *db) {
    const char *sql =
        "INSERT INTO mesures(ts, temperature, humidite, luminosite) "
        "VALUES(?, ?, ?, ?);";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        die(db, "Echec sqlite3_prepare_v2 (INSERT)");
    }

    double temps[] = {21.6, 21.8, 22.0, 22.3, 22.1, 22.4};
    double hum[]   = {44.2, 44.5, 45.0, 45.4, 45.1, 45.7};
    int lumi[]     = {320,  340,  360,  390,  410,  430};

    long long base = now_unix() - 50;

    for (int i = 0; i < 6; i++) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);

        sqlite3_bind_int64(stmt, 1, base + (i * 10));
        sqlite3_bind_double(stmt, 2, temps[i]);
        sqlite3_bind_double(stmt, 3, hum[i]);
        sqlite3_bind_int(stmt, 4, lumi[i]);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            die(db, "Echec sqlite3_step (INSERT)");
        }
    }

    sqlite3_finalize(stmt);
}

static void print_latest(sqlite3 *db) {
    const char *sql =
        "SELECT ts, temperature, humidite, luminosite "
        "FROM mesures "
        "ORDER BY ts DESC "
        "LIMIT 1;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        die(db, "Echec sqlite3_prepare_v2 (SELECT latest)");
    }

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        long long ts = sqlite3_column_int64(stmt, 0);
        double t = sqlite3_column_double(stmt, 1);
        double h = sqlite3_column_double(stmt, 2);
        int l = sqlite3_column_int(stmt, 3);

        printf("Derniere mesure:\n");
        printf("  ts          : %lld\n", ts);
        printf("  Temperature : %.1f C\n", t);
        printf("  Humidite    : %.1f %%\n", h);
        printf("  Luminosite  : %d (adc/lux)\n", l);
    } else if (rc == SQLITE_DONE) {
        printf("Aucune mesure dans la base.\n");
    } else {
        sqlite3_finalize(stmt);
        die(db, "Echec sqlite3_step (SELECT latest)");
    }

    sqlite3_finalize(stmt);
}

static void print_history(sqlite3 *db, int limit) {
    const char *sql =
        "SELECT ts, temperature, humidite, luminosite "
        "FROM mesures "
        "ORDER BY ts DESC "
        "LIMIT ?;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        die(db, "Echec sqlite3_prepare_v2 (SELECT history)");
    }

    sqlite3_bind_int(stmt, 1, limit);

    printf("\nHistorique (max %d):\n", limit);
    printf("ts\t\ttemp(C)\thum(%%)\tlum\n");

    while (1) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            long long ts = sqlite3_column_int64(stmt, 0);
            double t = sqlite3_column_double(stmt, 1);
            double h = sqlite3_column_double(stmt, 2);
            int l = sqlite3_column_int(stmt, 3);

            printf("%lld\t%.1f\t%.1f\t%d\n", ts, t, h, l);
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            sqlite3_finalize(stmt);
            die(db, "Echec sqlite3_step (SELECT history)");
        }
    }

    sqlite3_finalize(stmt);
}

int main(void) {
    sqlite3 *db = NULL;

    if (sqlite3_open("capteurs.db", &db) != SQLITE_OK) {
        die(db, "Impossible d'ouvrir capteurs.db");
    }

    exec_sql(db,
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE IF NOT EXISTS mesures ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  ts INTEGER NOT NULL,"
        "  temperature REAL NOT NULL,"
        "  humidite REAL NOT NULL,"
        "  luminosite INTEGER NOT NULL"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_mesures_ts ON mesures(ts);"
    );

    exec_sql(db, "DELETE FROM mesures;");
    insert_test_data(db);

    print_latest(db);
    print_history(db, 10);

    sqlite3_close(db);
    return 0;
}
