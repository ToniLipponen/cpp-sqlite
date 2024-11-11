/**
    MIT License

    Copyright (c) 2023 Toni Lipponen

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
 */

#pragma once
#include <sqlite3.h>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdint>

#if __cplusplus > 201402L
    #define CPP_SQLITE_NODISCARD [[nodiscard]]
#else
    #define CPP_SQLITE_NODISCARD
#endif

#if defined(CPP_SQLITE_NOTHROW)
    #define CPP_SQLITE_THROW(...) return false
#else
    #define CPP_SQLITE_THROW(...) throw sqlite::Error(__VA_ARGS__)
#endif

namespace sqlite
{
    class Error : public std::runtime_error
    {
    public:
        explicit Error(const char* message, int errorCode = SQLITE_ERROR)
        : std::runtime_error(message), m_errorCode(errorCode)
        {

        }

        explicit Error(const std::string& message, int errorCode = SQLITE_ERROR)
        : std::runtime_error(message), m_errorCode(errorCode)
        {

        }

        CPP_SQLITE_NODISCARD
        int GetCode() const
        {
            return m_errorCode;
        }

    private:
        int m_errorCode;
    };

    namespace Priv
    {
        inline bool CheckError(sqlite3* db, int code)
        {
            if(code != SQLITE_OK && code != SQLITE_DONE)
            {
                const int extendedCode = sqlite3_extended_errcode(db);
                std::string errstr = sqlite3_errstr(extendedCode);
                std::string errmsg = sqlite3_errmsg(db);

                CPP_SQLITE_THROW(errstr + ": " + errmsg, extendedCode);
            }

            return true;
        }

        inline bool CheckError(int code)
        {
            if(code != SQLITE_OK && code != SQLITE_DONE)
            {
                std::string errstr = std::string("SQL error: ") + sqlite3_errstr(code);
                CPP_SQLITE_THROW(errstr, code);
            }

            return true;
        }
    }

    class Blob
    {
    public:
        Blob(const void* data, int32_t bytes)
        {
            m_data.resize(bytes);
            std::memcpy(&m_data.at(0), data, bytes);
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

    struct Statement
    {
    private:
        Statement(sqlite3* connectionHandle, const std::string& command)
        {
            const int code = sqlite3_prepare_v2(
                    connectionHandle,
                    command.data(),
                    static_cast<int>(command.size()),
                    &m_handle,
                    nullptr);

            Priv::CheckError(connectionHandle, code);
        }

    public:
        friend class Connection;
        Statement(Statement&& other) noexcept
        {
            std::swap(m_handle, other.m_handle);
        }

        ~Statement()
        {
            if(m_handle)
            {
                sqlite::Priv::CheckError(sqlite3_finalize(m_handle));
            }
        }

        Statement& operator=(Statement&& other) noexcept
        {
            m_handle = other.m_handle;
            other.m_handle = nullptr;

            return *this;
        }

        template<typename First, typename ... Args>
        bool Bind(const First& first, const Args&... args)
        {
            return Reset() && Bind(1, first, args...);
        }

        bool Reset() const
        {
            return sqlite::Priv::CheckError(sqlite3_reset(m_handle));
        }

        bool Evaluate()
        {
            const int code = sqlite3_step(m_handle);

            if(code == SQLITE_ROW)
            {
                return true;
            }

            sqlite::Priv::CheckError(code);
            Reset();

            return false;
        }

        bool Bind(int index, const int32_t& data)
        {
            return sqlite::Priv::CheckError(sqlite3_bind_int(m_handle, index, data));
        }

        bool Bind(int index, const int64_t& data)
        {
            return sqlite::Priv::CheckError(sqlite3_bind_int64(m_handle, index, data));
        }

        bool Bind(int index, const float& data)
        {
            return sqlite::Priv::CheckError(sqlite3_bind_double(m_handle, index, static_cast<double>(data)));
        }

        bool Bind(int index, const double& data)
        {
            return sqlite::Priv::CheckError(sqlite3_bind_double(m_handle, index, data));
        }

        bool Bind(int index, const std::string& data)
        {
            return sqlite::Priv::CheckError(sqlite3_bind_text(m_handle, index, data.data(), static_cast<int>(data.size()), nullptr));
        }

        bool Bind(int index, const char* data)
        {
            return sqlite::Priv::CheckError(sqlite3_bind_text(m_handle, index, data, static_cast<int>(std::strlen(data)), nullptr));
        }

        bool Bind(int index, const sqlite::Blob& blob)
        {
            return sqlite::Priv::CheckError(sqlite3_bind_blob(m_handle, index, blob.GetData(), static_cast<int>(blob.GetSize()), nullptr));
        }

        bool Bind(int index, const sqlite::NOBlob& blob)
        {
            return sqlite::Priv::CheckError(sqlite3_bind_blob(m_handle, index, blob.GetData(), static_cast<int>(blob.GetSize()), nullptr));
        }

    private:
        template<typename First, typename ... Args>
        bool Bind(int index, const First& first, const Args&... args)
        {
            return Bind(index, first) && Bind(++index, args...);
        }

    protected:
        sqlite3_stmt* m_handle = nullptr;
    };

    class Result
    {
        explicit Result(sqlite3_stmt*& statement, bool own = false)
        : m_statement(nullptr), m_owning(own)
        {
            if(own)
            {
                std::swap(m_statement, statement);
            }
            else
            {
                m_statement = statement;
            }
        }

    public:
        friend class Query;
        friend class Connection;

        Result(Result&& other) noexcept
        : m_statement(nullptr), m_owning(other.m_owning)
        {
            std::swap(m_statement, other.m_statement);
        }

        ~Result()
        {
            if(m_owning)
            {
                sqlite3_finalize(m_statement);
            }
        }
        Result& operator=(Result&& other) noexcept
        {
            std::swap(m_statement, other.m_statement);
            other.m_statement = nullptr;

            return *this;
        }

        CPP_SQLITE_NODISCARD
        bool HasData() const
        {
            return ColumnCount() > 0;
        }

        bool Reset() const
        {
            return sqlite::Priv::CheckError(sqlite3_reset(m_statement));
        }

        CPP_SQLITE_NODISCARD
        bool Next() const
        {
            const int code = sqlite3_step(m_statement);

            if(code == SQLITE_ROW)
            {
                return true;
            }

            sqlite::Priv::CheckError(code);
            Reset();

            return false;
        }

        CPP_SQLITE_NODISCARD
        int ColumnCount() const
        {
            Reset();

            if(!Next())
            {
                return 0;
            }

            const int count = sqlite3_column_count(m_statement);
            Reset();

            return count;
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        T Get(int columnIndex) const
        {
            static_assert(sizeof(T) == -1, "SQL error: invalid column data type");
        }

    private:
        sqlite3_stmt* m_statement;
        bool m_owning;
    };

    template<>
    inline float Result::Get(int col) const
    {
        return static_cast<float>(sqlite3_column_double(m_statement, col));
    }

    template<>
    inline double Result::Get(int col) const
    {
        return sqlite3_column_double(m_statement, col);
    }

    template<>
    inline int32_t Result::Get(int col) const
    {
        return sqlite3_column_int(m_statement, col);
    }

    template<>
    inline int64_t Result::Get(int col) const
    {
        return sqlite3_column_int64(m_statement, col);
    }

    template<>
    inline std::string Result::Get(int col) const
    {
        const unsigned char* bytes = sqlite3_column_text(m_statement, col);
        const int size = sqlite3_column_bytes(m_statement, col);

        if(size == 0)
        {
            return "";
        }

        return {reinterpret_cast<const char*>(bytes), static_cast<std::string::size_type>(size)};
    }

    template<>
    inline sqlite::Blob Result::Get(int col) const
    {
        const void* bytes = sqlite3_column_blob(m_statement, col);
        const int size = sqlite3_column_bytes(m_statement, col);

        return {bytes, size};
    }

    class Query : public Statement
    {
    public:
        using Statement::Statement;
        friend class Connection;

        Result Execute()
        {
            return Result(m_handle);
        }
    };

    class Connection
    {
    public:
        Connection() : m_connection(nullptr) {}

        explicit Connection(const std::string& filename)
        {
            this->Open(filename);
        }

        Connection(const Connection&) = delete;

        Connection(Connection&& other) noexcept
        {
            this->m_connection = other.m_connection;
            other.m_connection = nullptr;
        }

        virtual ~Connection() noexcept
        {
            try
            {
                this->Close();
            }
            catch(...)
            {

            }
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

        bool Backup(const std::string& filename) const
        {
            Connection backup(filename);

            return Backup(backup);
        }

        bool Backup(Connection& backup) const
        {
            sqlite3_backup* backupHandle = sqlite3_backup_init(backup.GetHandle(),
                                                               "main",
                                                               GetHandle(),
                                                               "main");
            if(!backupHandle)
            {
                CPP_SQLITE_THROW("SQL error: Failed to initialize backup");
            }

            if(!Priv::CheckError(sqlite3_backup_step(backupHandle, -1)))
            {
                CPP_SQLITE_THROW("SQL error: Could not execute backup");
            }

            if(!Priv::CheckError(GetHandle(), sqlite3_backup_finish(backupHandle)))
            {
                CPP_SQLITE_THROW("SQL error: Could not finish backup");
            }

            return true;
        }

        bool Statement(const std::string& command) const
        {
            sqlite::Statement statement(GetHandle(), command);

            return statement.Evaluate();
        }

        template<typename First, typename ... Args>
        bool Statement(const std::string& command, const First& first, const Args&... args)
        {
            sqlite::Statement statement(GetHandle(), command);
            statement.Bind(first, args...);

            return statement.Evaluate();
        }

        CPP_SQLITE_NODISCARD
        Result Query(const std::string& command) const
        {
            sqlite::Query query(GetHandle(), command);

            return Result(query.m_handle, true);
        }

        template<typename First, typename ... Args>
        CPP_SQLITE_NODISCARD
        Result Query(const std::string& command, const First& first, const Args&... args) const
        {
            sqlite::Query query(GetHandle(), command);
            query.Bind(first, args...);

            return Result(query.m_handle, true);
        }

        CPP_SQLITE_NODISCARD
        sqlite::Statement PrepareStatement(const std::string& command) const
        {
            sqlite::Statement statement(GetHandle(), command);

            return statement;
        }

        CPP_SQLITE_NODISCARD
        sqlite::Query PrepareQuery(const std::string& command) const
        {
            sqlite::Query query(GetHandle(), command);

            return query;
        }

        bool Open(const std::string& filename)
        {
            return sqlite::Priv::CheckError(sqlite3_open(filename.c_str(), &m_connection));
        }

        bool Close()
        {
            const auto result = Priv::CheckError(sqlite3_close(m_connection));
            m_connection = nullptr;

            return result;
        }

        CPP_SQLITE_NODISCARD
        int GetExtendedResult() const
        {
            return sqlite3_extended_errcode(m_connection);
        }

        CPP_SQLITE_NODISCARD
        sqlite3* GetHandle() const
        {
            return m_connection;
        }

    private:
        sqlite3* m_connection = nullptr;
    };
}
