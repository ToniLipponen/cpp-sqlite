#include "sqlite.hpp"
#include <iostream>

int main()
{
    /// Opening a new connection
    sqlite::Connection connection("example.db");

    /// Executing a statement
    connection.Statement("CREATE TABLE IF NOT EXISTS example ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "textData TEXT, "
                         "intData INTEGER, "
                         "floatData REAL)");

    /// Executing a statement with parameters
    connection.Statement("INSERT INTO example (textData, intData, floatData) "
                         "VALUES (?,?,?)",
                         "Hello world",
                         1,
                         1.23);

    /// Executing a query
    sqlite::Result result = connection.Query("SELECT * FROM example");

    /// Iterating through the result rows
    while(result.Next())
    {
        std::cout
        << result.Get<int>(0)          << " "
        << result.Get<std::string>(1)  << " "
        << result.Get<int>(2)          << " "
        << result.Get<float>(3)        << std::endl;
    }

    /// Exceptions
    try
    {
        /// Deliberate mistake here  ↓
        (void)connection.Query("SELECCT textData FROM example");
    }
    catch(const sqlite::Error& e)
    {
        std::cout << e.what() << std::endl;
    }

    /// Copy data into backup.db
    connection.Backup("backup.db");

    return 0;
}
