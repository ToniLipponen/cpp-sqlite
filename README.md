## Single file header only sqlite wrapper for C++

## Example
```cpp
#include "sqlite.hpp"
#include <iostream>

int main()
{
    sqlite::Connection connection("example.db");

    sqlite::Statement(connection, "CREATE TABLE IF NOT EXISTS exampleTable ("
                                  "textData TEXT, "
                                  "intData INTEGER, "
                                  "floatData REAL)");

    sqlite::Statement(connection,
                      "INSERT INTO exampleTable VALUES (?, ?, ?)",
                      "Hello world",
                      1234,
                      5.6789);

    sqlite::Result res = sqlite::Query(connection, "SELECT textData, intData, floatData "
                                                   "FROM exampleTable");

    while(res.Next())
    {
        std::string textData = res.Get();
        int intData = res.Get();
        float floatData = res.Get();

        std::cout << textData << " " << intData << " " << floatData << std::endl;
    }
    
    return 0;
}

```
