## Single file header only sqlite wrapper for C++

## Example
```cpp
#include "sqlite.hpp"
#include <iostream>

int main()
{
    sqlite::Connection connection("example.db");

    connection.Statement("CREATE TABLE IF NOT EXISTS example ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "textData TEXT, "
                         "intData INTEGER, "
                         "floatData REAL)");

    connection.Statement("INSERT INTO example (textData, intData, floatData) "
                         "VALUES (?,?,?)",
                         "Hello world",
                         1,
                         1.23);

    sqlite::Result result = connection.Query("SELECT * FROM example");

    while(result.Next())
    {
        std::cout
        << result.Get<int>(0)          << " "
        << result.Get<std::string>(1)  << " "
        << result.Get<int>(2)          << " "
        << result.Get<float>(3)        << std::endl;
    }

    return 0;
}
```
