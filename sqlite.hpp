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
#include <type_traits>
#include <string_view>
#include <string>
#include <vector>
#include <exception>
#include <cstring>
#include <cstdint>
#include <sstream>

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

        explicit Connection(const std::string_view& filename)
        {
            Open(filename);
        }

        virtual ~Connection()
        {
            Close();
        }

        bool Open(const std::string_view& filename)
        {
            return Priv::CheckError(sqlite3_open(filename.data(), &m_connection));
        }

        bool Close()
        {
            const auto result = Priv::CheckError(sqlite3_close(m_connection));
            m_connection = nullptr;

            return result;
        }

        [[nodiscard]]
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
        explicit Blob(const unsigned char* data, uint32_t bytes)
        {
            m_data.resize(bytes);
            std::memcpy(m_data.data(), data, bytes);
        }

        explicit Blob(std::vector<unsigned char> data)
        : m_data(std::move(data))
        {

        }

        [[nodiscard]]
        uint32_t GetSize() const
        {
            return m_data.size();
        }

        [[nodiscard]]
        unsigned char* GetData()
        {
            return m_data.data();
        }

        [[nodiscard]]
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

        [[nodiscard]]
        uint32_t GetSize() const
        {
            return m_bytes;
        }

        const void* GetData()
        {
            return m_ptr;
        }

        [[nodiscard]]
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

        inline void Append(sqlite3_stmt* statement, int index, const std::string_view& data)
        {
            CheckError(sqlite3_bind_text(statement, index, data.data(), static_cast<int>(data.size()), nullptr));
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

            Statement(Connection& connection, const std::string_view& command)
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
            [[nodiscard]]
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

            return Blob(reinterpret_cast<const unsigned char*>(sqlite3_column_blob(handle, col)), size);
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
        operator T() const
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

        friend void Statement(Connection&, const std::string_view&);

        template<typename First, typename ... Args>
        friend Result Query(Connection& connection, const std::string_view& command, const First& first, const Args... args);
        friend Result Query(Connection& connection, const std::string_view& command);

    private:
        Priv::Statement m_statement;
        int m_currentColumn = 0;
    };

    template<typename First, typename ... Args>
    void Statement(Connection& connection, const std::string_view& command, const First& first, const Args... args)
    {
        Priv::Statement statement(connection, command);
        Priv::AppendToQuery<First, Args...>(statement.handle, 1, first, args...);

        statement.Advance();
    }

    void Statement(Connection& connection, const std::string_view& command)
    {
        Priv::Statement statement(connection, command);

        statement.Advance();
    }

    template<typename First, typename ... Args>
    [[nodiscard]]
    Result Query(Connection& connection, const std::string_view& command, const First& first, const Args... args)
    {
        Priv::Statement statement(connection, command);
        Priv::AppendToQuery<First, Args...>(statement.handle, 1, first, args...);

        return Result(std::move(statement));
    }

    [[nodiscard]]
    Result Query(Connection& connection, const std::string_view& command)
    {
        Priv::Statement statement(connection, command);

        return Result(std::move(statement));
    }
}