## Single file header only sqlite wrapper for C++

## Example
```cpp
#include "sqlite.hpp"
#include <iostream>

int main()
{
    sqlite::Connection connection;
    connection.open("example.db");

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

## Prepared statements/queries
```cpp
sqlite::Statement statement = connection.prepare_statement(
    "INSERT INTO example (textData, intData, floatData)"
    "VALUES (?,?,?)"
);

// Bind values to (?,?,?)
statement.bind("Hello, world", 123, 1.23);
// Execute
statement.execute();

// Binding new values
statement.bind("Something else", 42, 3.14);
statement.execute();

sqlite::Query query = connection.prepare_query("SELECT * FROM example WHERE id = ?");

query.bind(1);
sqlite::ResultView result = query.execute();

query.bind(42);
result = query.execute();
```

## Accessing result columns
```cpp
sqlite::Result result = connection.query("SELECT * FROM example");

// Next row
result.next();

// By column id
int my_int = result.get<int>(2); 

// By column name
std::string my_string = result.get<std::string>("textData"); 

// By column name using implicit type conversion
float my_float = result.get_value("floatData");
```