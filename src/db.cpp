#include "db.hpp"

#include <iostream>

Database::Database() {
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open your music files : %s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

Database::~Database() { sqlite3_close(db); }
