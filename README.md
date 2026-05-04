## Single file header only sqlite wrapper for C++

## Example
```cpp
#include "sqlite.hpp"
#include <iostream>

int main()
{
    sqlite::Connection connection("example.db");

    connection.statement("CREATE TABLE IF NOT EXISTS example ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "textData TEXT, "
                         "intData INTEGER, "
                         "floatData REAL)");

    connection.statement("INSERT INTO example (textData, intData, floatData) "
                         "VALUES (?,?,?)",
                         "Hello world",
                         1,
                         1.23);

    sqlite::Result result = connection.query("SELECT * FROM example");

    while(result.next())
    {
        std::cout
        << result.get<int>(0)          << " "
        << result.get<std::string>(1)  << " "
        << result.get<int>(2)          << " "
        << result.get<float>(3)        << std::endl;
    }

    return 0;
}
```
