#include "sqlite.hpp"
#include <iostream>

int main()
{
    /// Opening a new connection
    sqlite::Connection connection("example.db");

    /// Executing a statement
    sqlite::Statement(connection, "CREATE TABLE IF NOT EXISTS exampleTable ("
                                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                  "textData TEXT, "
                                  "intData INTEGER, "
                                  "floatData REAL)");

    /// Executing a statement with parameters
    sqlite::Statement(connection,
                      "INSERT INTO exampleTable (textData, intData, floatData) VALUES (?, ?, ?)",
                      "Hello world",
                      1234,
                      5.6789);

    /// Executing a query
    sqlite::Result res = sqlite::Query(connection, "SELECT textData, intData, floatData "
                                                   "FROM exampleTable");

    /// Executing a query with parameters
    sqlite::Result res2 = sqlite::Query(connection, "SELECT * "
                                                    "FROM exampleTable "
                                                    "WHERE id = ?",
                                                    3);

    /// Iterate through the results rows
    while(res.Next())
    {
        std::string textData = res.Get(0);
        int intData = res.Get(1);
        float floatData = res.Get(2);

        std::cout << textData << " " << intData << " " << floatData << std::endl;
    }

    /// Exceptions
    try
    {
        /// Deliberate mistake here           ↓
        (void)sqlite::Query(connection, "SELECCT textData FROM exampleTable WHERE intData=1234");
    }
    catch(const sqlite::Error& e)
    {
        std::cout << e.what() << std::endl;
    }

    /// Copy data from connection to backup.db
    sqlite::Backup(connection, "backup.db");

    return 0;
}
