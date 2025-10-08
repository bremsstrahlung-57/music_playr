#pragma once
#include <sqlite3.h>

class Database {
   private:
    sqlite3 *db;
    int rc = sqlite3_open("test.db", &db);

   public:
    Database();
    ~Database();
};