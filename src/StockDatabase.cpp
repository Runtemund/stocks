//
//  Main.cpp
//  stocks
//
//  Created by Uwe Runtemund on 01.09.2025
//  Copyright © 2025 Jameo Software. All rights reserved.
//

#include "Precompiled.hpp"

StockDatabase::StockDatabase(const jm::String& dbFile) 
{
    if (sqlite3_open(dbFile.toCString().constData(), &mDb)) 
    {
        std::cerr << "Can't open DB: " << sqlite3_errmsg(mDb) << std::endl;
        mDb = nullptr;
    }
}

StockDatabase::~StockDatabase() 
{
    if (mDb) sqlite3_close(mDb);
}

bool StockDatabase::initSchema() 
{
    const char* sql =
       " CREATE TABLE IF NOT EXISTS stocks ("
       "     id INTEGER PRIMARY KEY AUTOINCREMENT,"
       "     symbol TEXT NOT NULL UNIQUE,"
       "     name TEXT,"
       "     currency TEXT"
       " );"

       " CREATE TABLE IF NOT EXISTS prices ("
       "     id INTEGER PRIMARY KEY AUTOINCREMENT,"
       "     stock_id INTEGER,"
       "     date TEXT,"
       "     open REAL,"
       "     high REAL,"
       "     low REAL,"
       "     close REAL,"
       "     volume INTEGER,"
       "     FOREIGN KEY(stock_id) REFERENCES stocks(id)"
       " );";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(mDb, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) 
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

int StockDatabase::addStock(const jm::String& symbol, 
                            const jm::String& name,
                            const jm::String& currency) 
{
    const char* sql = "INSERT OR IGNORE INTO stocks (symbol, name, currency) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(mDb, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, symbol.toCString().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.toCString().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, currency.toCString().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return getStockId(symbol);
}

int StockDatabase::getStockId(const jm::String& symbol) 
{
    const char* sql = "SELECT id FROM stocks WHERE symbol = ?;";
    sqlite3_stmt* stmt;
    int stockId = -1;

    sqlite3_prepare_v2(mDb, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, symbol.toCString().constData(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        stockId = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return stockId;
}

bool StockDatabase::getStockData(const jm::String& symbol,
                                 jm::String& name, 
                                 jm::String& currency) 
{
    const char* sql = "SELECT name,currency FROM stocks WHERE symbol = ?;";
    sqlite3_stmt* stmt;
    jm::String stockName;

    sqlite3_prepare_v2(mDb, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, symbol.toCString().constData(), -1, SQLITE_TRANSIENT);

    bool status = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        name = jm::String((const char*)sqlite3_column_text(stmt, 0));
        currency = jm::String((const char*)sqlite3_column_text(stmt, 1));
        status = true;
    }

    sqlite3_finalize(stmt);
    return status;
}

bool StockDatabase::insertPrice(const jm::String& symbol, const PriceRecord& r) 
{
    int stock_id = getStockId(symbol);
    if (stock_id < 0) return false;

    const char* sql =
       "INSERT INTO prices (stock_id, date, open, high, low, close, volume)"
       " VALUES (?, ?, ?, ?, ?, ?, ?);";

   jm::DateFormatter df=jm::DateFormatter("yyyy-MM-dd");

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(mDb, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, stock_id);
    sqlite3_bind_text(stmt, 2, df.format(r.date).toCString().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, r.open);
    sqlite3_bind_double(stmt, 4, r.high);
    sqlite3_bind_double(stmt, 5, r.low);
    sqlite3_bind_double(stmt, 6, r.close);
    sqlite3_bind_int64(stmt, 8, r.volume);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

jm::Date sqlToDate(const jm::String &date)
{
   return jm::Date(date.substring(0,4).toInt(),
                   date.substring(5,7).toInt()-1,
                   date.substring(8,10).toInt());
}

std::vector<PriceRecord> StockDatabase::getPrices(const jm::String& symbol) 
{
    std::vector<PriceRecord> results;
    int stock_id = getStockId(symbol);
    if (stock_id < 0) return results;

    const char* sql =
        "SELECT date, open, high, low, close, volume"
        " FROM prices"
        " WHERE stock_id = ?"
        " ORDER BY date;";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(mDb, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, stock_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        PriceRecord r;

        jm::String date = jm::String((const char*)sqlite3_column_text(stmt, 0));

        r.date = sqlToDate(date);

        r.open = sqlite3_column_double(stmt, 1);
        r.high = sqlite3_column_double(stmt, 2);
        r.low = sqlite3_column_double(stmt, 3);
        r.close = sqlite3_column_double(stmt, 4);
        r.volume = sqlite3_column_int64(stmt, 5);
        results.push_back(r);
    }

    sqlite3_finalize(stmt);

    std::cout<<"Prices collected "<<results.size();
    return results;
}

Stock* StockDatabase::stock(const jm::String& symbol)
{
   if(getStockId(symbol)<1)return nullptr;

   Stock* stock = new Stock();
   stock->symbol=symbol;

   getStockData(symbol,
                stock->name,
                stock->currency);
   stock->priceHistory=getPrices(symbol);

   return stock;
}
