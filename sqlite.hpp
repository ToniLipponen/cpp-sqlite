#pragma once
#include <sqlite3.h>
#include <type_traits>
#include <string_view>
#include <string>
#include <vector>
#include <exception>
#include <cstring>
#include <sstream>

namespace sqlite
{
    class [[maybe_unused]] Error : public std::runtime_error
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

    class [[maybe_unused]] Blob
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
    struct [[maybe_unused]] NOBlob
    {
    public:
        explicit NOBlob(const void* ptr, uint32_t bytes)
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

    namespace Impl
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
                std::string errstr = sqlite3_errstr(code);
                throw Error(errstr);
            }

            return true;
        }

        [[maybe_unused]]
        inline void Append(sqlite3_stmt* statement, int index, const int32_t& data)
        {
            CheckError(sqlite3_bind_int(statement, index, data));
        }

        [[maybe_unused]]
        inline void Append(sqlite3_stmt* statement, int index, const int64_t& data)
        {
            CheckError(sqlite3_bind_int64(statement, index, data));
        }

        [[maybe_unused]]
        inline void Append(sqlite3_stmt* statement, int index, const float& data)
        {
            CheckError(sqlite3_bind_double(statement, index, static_cast<double>(data)));
        }

        [[maybe_unused]]
        inline void Append(sqlite3_stmt* statement, int index, const double& data)
        {
            CheckError(sqlite3_bind_double(statement, index, data));
        }

        [[maybe_unused]]
        inline void Append(sqlite3_stmt* statement, int index, const std::string_view& data)
        {
            CheckError(sqlite3_bind_text(statement, index, data.data(), static_cast<int>(data.size()), nullptr));
        }

        [[maybe_unused]]
        inline void Append(sqlite3_stmt* statement, int index, const Blob& blob)
        {
            CheckError(sqlite3_bind_blob(statement, index, blob.GetData(), static_cast<int>(blob.GetSize()), nullptr));
        }

        [[maybe_unused]]
        inline void Append(sqlite3_stmt* statement, int index, const NOBlob& blob)
        {
            CheckError(sqlite3_bind_blob(statement, index, blob.GetData(), static_cast<int>(blob.GetSize()), nullptr));
        }

        template<typename Arg>
        [[maybe_unused]]
        inline void AppendToQuery(sqlite3_stmt* statement, int index, const Arg& arg)
        {
            Append(statement, index, arg);
        }

        template<typename First, typename ... Args>
        [[maybe_unused]]
        inline void AppendToQuery(sqlite3_stmt* statement, int index, const First& first, const Args&... args)
        {
            Append(statement, index, first);
            AppendToQuery(statement, ++index, args...);
        }

        struct [[maybe_unused]] Statement
        {
            Statement() : handle(nullptr) {}

            explicit Statement(sqlite3_stmt* sqlite3Stmt)
            : handle(sqlite3Stmt)
            {

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

                CheckError(code);
            }

            template<typename T>
            [[nodiscard]]
            T Get(int col)
            {
                if constexpr(std::is_same<T, int32_t>::value)
                {
                    T result = sqlite3_column_int(handle, col);

                    return result;
                }
                else if constexpr(std::is_same<T, std::string>::value)
                {
                    auto str = (const char*)sqlite3_column_text(handle, col);

                    if(!str)
                        return "";

                    return str;
                }
            }

            sqlite3_stmt* handle = nullptr;
        };
    }

    class [[maybe_unused]] Connection
    {
    public:
        Connection() : m_connection(nullptr) {}

        Connection(const std::string_view& filename)
        {
            Open(filename);
        }

        virtual ~Connection()
        {
            Close();
        }

        bool Open(const std::string_view& filename)
        {
            return Impl::CheckError(sqlite3_open(filename.data(), &m_connection));
        }

        bool Close()
        {
            const auto result = Impl::CheckError(sqlite3_close(m_connection));
            m_connection = nullptr;

            return result;
        }

        sqlite3* GetPtr()
        {
            return m_connection;
        }

    private:
        sqlite3* m_connection;
    };

    class [[maybe_unused]] Result
    {
        explicit Result(Impl::Statement&& statement)
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

        template<typename T>
        T Get()
        {
            T value = m_statement.Get<T>(m_currentColumn++);

            return value;
        }

        friend void Statement(Connection&, const std::string_view&);

        template<typename First, typename ... Args>
        friend Result Query(Connection& connection, const std::string_view& command, const First& first, const Args... args);
        friend Result Query(Connection& connection, const std::string_view& command);

    private:
        Impl::Statement m_statement;
        int m_currentColumn = 0, m_columnCount = 0;
    };

    template<typename First, typename ... Args>
    [[maybe_unused]]
    void Statement(Connection& connection, const std::string_view& command, const First& first, const Args... args)
    {
        auto* db = connection.GetPtr();
        Impl::Statement statement;

        const int code = sqlite3_prepare(
                db,
                command.data(),
                static_cast<int>(command.size()),
                &statement.handle,
                nullptr);

        Impl::AppendToQuery<First, Args...>(statement.handle, 1, first, args...);
        Impl::CheckError(db, code);

        statement.Advance();
    }

    [[maybe_unused]]
    void Statement(Connection& connection, const std::string_view& command)
    {
        auto* db = connection.GetPtr();
        Impl::Statement statement;

        const int code = sqlite3_prepare(
                db,
                command.data(),
                static_cast<int>(command.size()),
                &statement.handle,
                nullptr);

        Impl::CheckError(db, code);

        statement.Advance();
    }

    template<typename First, typename ... Args>
    [[nodiscard, maybe_unused]]
    Result Query(Connection& connection, const std::string_view& command, const First& first, const Args... args)
    {
        auto* db = connection.GetPtr();
        Impl::Statement statement;

        const int code = sqlite3_prepare(
                db,
                command.data(),
                static_cast<int>(command.size()),
                &statement.handle,
                nullptr);

        Impl::CheckError(db, code);
        Impl::AppendToQuery<First, Args...>(statement.handle, 1, first, args...);

        return Result(std::move(statement));
    }

    Result Query(Connection& connection, const std::string_view& command)
    {
        auto* db = connection.GetPtr();
        Impl::Statement statement;

        const int code = sqlite3_prepare(
                connection.GetPtr(),
                command.data(),
                static_cast<int>(command.size()),
                &statement.handle,
                nullptr);

        Impl::CheckError(db, code);

        return Result(std::move(statement));
    }
}
