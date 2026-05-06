#include "sqlite.hpp"
#include <iostream>

int main()
{
    // Opening a new connection
    sqlite::Connection connection;
    connection.open("example.db");

    // Executing a statement
    connection.statement("CREATE TABLE IF NOT EXISTS example ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "textData TEXT, "
                         "intData INTEGER, "
                         "floatData REAL)");

    // Executing a statement with parameters
    connection.statement("INSERT INTO example (textData, intData, floatData) "
                         "VALUES (?,?,?)",
                         "Hello world",
                         1,
                         1.23);

    // Executing a query
    sqlite::Result result = connection.query("SELECT * FROM example");

    // Iterating through the result rows
    while(result.next())
    {
        std::cout 
            << result.get<int>(0)           << " "
            << result.get<std::string>(1)   << " "
            << result.get<int>(2)           << " "
            << result.get<float>(3)         
            << std::endl;
    }

    // Exceptions
    try
    {
        // Deliberate mistake here  ↓
        (void)connection.query("SELECCT textData FROM example");
    }
    catch(const sqlite::Exception& e)
    {
        std::cout << e.what() << std::endl;
    }

    // Copy data into backup.db
    connection.backup("backup.db");

    return 0;
}
