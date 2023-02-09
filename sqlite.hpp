/**
    Copyright (C) 2023 Toni Lipponen

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
 */

#pragma once
#include <sqlite3.h>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

#if __cplusplus > 201402L
    #define CPP_SQLITE_NODISCARD [[nodiscard]]
#else
    #define CPP_SQLITE_NODISCARD
#endif

namespace sqlite
{
    class Error : public std::runtime_error
    {
    public:
        explicit Error(const char* message)
        : std::runtime_error(message)
        {

        }

        explicit Error(const std::string& message)
        : std::runtime_error(message)
        {

        }
    };

    namespace Priv
    {
        inline bool CheckError(sqlite3* db, int code)
        {
            if(code != SQLITE_OK)
            {
                const int extendedCode = sqlite3_extended_errcode(db);
                std::string errstr = sqlite3_errstr(extendedCode);
                std::string errmsg = sqlite3_errmsg(db);

                throw Error(errstr + ": " + errmsg);
            }

            return true;
        }

        inline bool CheckError(int code)
        {
            if(code != SQLITE_OK)
            {
                std::string errstr = std::string("SQL error: ") + sqlite3_errstr(code);
                throw Error(errstr);
            }

            return true;
        }
    }

    class Connection
    {
    public:
        Connection() : m_connection(nullptr) {}

        explicit Connection(const std::string& filename)
        {
            Open(filename);
        }

        Connection(const Connection&) = delete;

        Connection(Connection&& other) noexcept
        {
            this->m_connection = other.m_connection;
            other.m_connection = nullptr;
        }

        virtual ~Connection()
        {
            Close();
        }

        Connection& operator=(const Connection&) = delete;

        Connection& operator=(Connection&& other) noexcept
        {
            if(&other != this)
            {
                this->m_connection = other.m_connection;
                other.m_connection = nullptr;
            }

            return *this;
        }

        bool Open(const std::string& filename)
        {
            return Priv::CheckError(sqlite3_open(filename.data(), &m_connection));
        }

        bool Close()
        {
            const auto result = Priv::CheckError(sqlite3_close(m_connection));
            m_connection = nullptr;

            return result;
        }

        CPP_SQLITE_NODISCARD
        sqlite3* GetPtr()
        {
            return m_connection;
        }

    private:
        sqlite3* m_connection = nullptr;
    };

    class Blob
    {
    public:
        Blob(const unsigned char* data, int32_t bytes)
        : m_data(data, data+bytes)
        {
            
        }

        explicit Blob(std::vector<unsigned char> data)
        : m_data(std::move(data))
        {

        }

        CPP_SQLITE_NODISCARD
        uint32_t GetSize() const
        {
            return m_data.size();
        }

        CPP_SQLITE_NODISCARD
        unsigned char* GetData()
        {
            return m_data.data();
        }

        CPP_SQLITE_NODISCARD
        const unsigned char* GetData() const
        {
            return m_data.data();
        }

    private:
        std::vector<unsigned char> m_data;
    };

    /** Non-owning blob*/
    class NOBlob
    {
    public:
        NOBlob(const void* ptr, uint32_t bytes)
        : m_ptr(ptr), m_bytes(bytes)
        {

        }

        CPP_SQLITE_NODISCARD
        uint32_t GetSize() const
        {
            return m_bytes;
        }

        const void* GetData()
        {
            return m_ptr;
        }

        CPP_SQLITE_NODISCARD
        const void* GetData() const
        {
            return m_ptr;
        }

    private:
        const void* m_ptr;
        uint32_t m_bytes;
    };

    namespace Priv
    {
        inline void Append(sqlite3_stmt* statement, int index, const int32_t& data)
        {
            CheckError(sqlite3_bind_int(statement, index, data));
        }

        inline void Append(sqlite3_stmt* statement, int index, const int64_t& data)
        {
            CheckError(sqlite3_bind_int64(statement, index, data));
        }

        inline void Append(sqlite3_stmt* statement, int index, const float& data)
        {
            CheckError(sqlite3_bind_double(statement, index, static_cast<double>(data)));
        }

        inline void Append(sqlite3_stmt* statement, int index, const double& data)
        {
            CheckError(sqlite3_bind_double(statement, index, data));
        }

        inline void Append(sqlite3_stmt* statement, int index, const std::string& data)
        {
            CheckError(sqlite3_bind_text(statement, index, data.data(), static_cast<int>(data.size()), nullptr));
        }

        inline void Append(sqlite3_stmt* statement, int index, const char* data)
        {
            CheckError(sqlite3_bind_text(statement, index, data, static_cast<int>(std::strlen(data)), nullptr));
        }

        inline void Append(sqlite3_stmt* statement, int index, const Blob& blob)
        {
            CheckError(sqlite3_bind_blob(statement, index, blob.GetData(), static_cast<int>(blob.GetSize()), nullptr));
        }

        inline void Append(sqlite3_stmt* statement, int index, const NOBlob& blob)
        {
            CheckError(sqlite3_bind_blob(statement, index, blob.GetData(), static_cast<int>(blob.GetSize()), nullptr));
        }

        template<typename Arg>
        inline void AppendToQuery(sqlite3_stmt* statement, int index, const Arg& arg)
        {
            Append(statement, index, arg);
        }

        template<typename First, typename ... Args>
        inline void AppendToQuery(sqlite3_stmt* statement, int index, const First& first, const Args&... args)
        {
            Append(statement, index, first);
            AppendToQuery(statement, ++index, args...);
        }

        struct Statement
        {
            Statement() : handle(nullptr) {}

            Statement(Connection& connection, const std::string& command)
            {
                auto* db = connection.GetPtr();

                const int code = sqlite3_prepare(
                        db,
                        command.data(),
                        static_cast<int>(command.size()),
                        &handle,
                        nullptr);

                Priv::CheckError(db, code);
            }

            Statement(Statement&& other) noexcept
            {
                std::swap(handle, other.handle);
            }

            ~Statement()
            {
                sqlite3_finalize(handle);
            }

            Statement& operator=(Statement&& other) noexcept
            {
                handle = other.handle;
                other.handle = nullptr;

                return *this;
            }

            bool Advance() const
            {
                const int code = sqlite3_step(handle);

                // No more rows
                if(code == 101)
                {
                    return false;
                }
                else if(code == SQLITE_ROW)
                {
                    return true;
                }

                return CheckError(code);
            }

            template<typename T>
            CPP_SQLITE_NODISCARD
            T Get(int) const
            {
                throw Error("SQL error: invalid column data type");
            }

            sqlite3_stmt* handle = nullptr;
        };

        template<>
        inline float Statement::Get(int col) const
        {
            return static_cast<float>(sqlite3_column_double(handle, col));
        }

        template<>
        inline double Statement::Get(int col) const
        {
            return sqlite3_column_double(handle, col);
        }

        template<>
        inline int32_t Statement::Get(int col) const
        {
            return sqlite3_column_int(handle, col);
        }

        template<>
        inline int64_t Statement::Get(int col) const
        {
            return sqlite3_column_int64(handle, col);
        }

        template<>
        inline std::string Statement::Get(int col) const
        {
            return reinterpret_cast<const char*>(sqlite3_column_text(handle, col));
        }

        template<>
        inline Blob Statement::Get(int col) const
        {
            const int size = sqlite3_column_bytes(handle, col);

            return {reinterpret_cast<const unsigned char*>(sqlite3_column_blob(handle, col)), size};
        }
    }

    class Type
    {
    private:
        Type(const Priv::Statement& statement, int col)
        : m_statement(statement), m_columnIndex(col)
        {

        }
    public:
        template<typename T>
        explicit operator T() const
        {
            return m_statement.Get<T>(m_columnIndex);
        }

        friend class Result;

    private:
        const Priv::Statement& m_statement;
        const int m_columnIndex;
    };

    class Result
    {
        explicit Result(Priv::Statement&& statement)
        : m_statement(std::move(statement))
        {

        }

    public:
        Result() = default;

        Result(Result&& other) noexcept
        {
            m_statement = std::move(other.m_statement);
            m_currentColumn = other.m_currentColumn;
        }

        Result& operator=(Result&& other) noexcept
        {
            m_statement = std::move(other.m_statement);
            m_currentColumn = other.m_currentColumn;

            return *this;
        }

        bool Next()
        {
            m_currentColumn = 0;
            return m_statement.Advance();
        }

        Type Get()
        {
            return {m_statement, m_currentColumn++};
        }

        friend void Statement(Connection&, const std::string&);

        template<typename First, typename ... Args>
        friend Result Query(Connection& connection, const std::string& command, const First& first, const Args... args);
        friend Result Query(Connection& connection, const std::string& command);

    private:
        Priv::Statement m_statement;
        int m_currentColumn = 0;
    };

    template<typename First, typename ... Args>
    inline void Statement(Connection& connection, const std::string& command, const First& first, const Args... args)
    {
        Priv::Statement statement(connection, command);
        Priv::AppendToQuery<First, Args...>(statement.handle, 1, first, args...);

        statement.Advance();
    }

    inline void Statement(Connection& connection, const std::string& command)
    {
        Priv::Statement statement(connection, command);

        statement.Advance();
    }

    template<typename First, typename ... Args>
    CPP_SQLITE_NODISCARD
    inline Result Query(Connection& connection, const std::string& command, const First& first, const Args... args)
    {
        Priv::Statement statement(connection, command);
        Priv::AppendToQuery<First, Args...>(statement.handle, 1, first, args...);

        return Result(std::move(statement));
    }

    CPP_SQLITE_NODISCARD
    inline Result Query(Connection& connection, const std::string& command)
    {
        Priv::Statement statement(connection, command);

        return Result(std::move(statement));
    }
}
