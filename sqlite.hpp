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
#include <unordered_map>
#include <stdexcept>
#include <cstdint>

#define CPP_SQLITE_VERSION 10000

#if __cplusplus >= 201703L
    #include <string_view>
    #include <filesystem>
    #define CPP_SQLITE_NODISCARD [[nodiscard]]
#else
    #define CPP_SQLITE_NODISCARD
#endif

#if !defined(__EXCEPTIONS) && !defined(_CPPUNWIND)
    #define CPP_SQLITE_NOTHROW
#endif

#if defined(CPP_SQLITE_NOTHROW)
    #define CPP_SQLITE_THROW(...) return false
#else
    #define CPP_SQLITE_THROW(...) throw sqlite::Exception(__VA_ARGS__)
#endif

namespace sqlite
{
    using byte = unsigned char;

    struct NonCopyable
    {
        NonCopyable() = default;
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable(NonCopyable&&) = default;
        ~NonCopyable() = default;
        NonCopyable& operator=(const NonCopyable&) = delete;
        NonCopyable& operator=(NonCopyable&&) = default;
    };

    class Exception : public std::runtime_error
    {
    public:
        explicit Exception(const char* message, std::int32_t error_code = SQLITE_ERROR)
        : std::runtime_error(message), m_error_code(error_code)
        {

        }

        explicit Exception(const std::string& message, std::int32_t error_code = SQLITE_ERROR)
        : std::runtime_error(message), m_error_code(error_code)
        {

        }

        CPP_SQLITE_NODISCARD
        std::int32_t get_code() const
        {
            return m_error_code;
        }

    private:
        std::int32_t m_error_code;
    };

    namespace priv
    {
        inline bool check_error(sqlite3* db, std::int32_t code)
        {
            if(code != SQLITE_OK && code != SQLITE_DONE)
            {
                const std::int32_t extendedCode = sqlite3_extended_errcode(db);
                std::string errstr = sqlite3_errstr(extendedCode);
                std::string errmsg = sqlite3_errmsg(db);

                CPP_SQLITE_THROW(errstr + ": " + errmsg, extendedCode);
            }

            return true;
        }

        inline bool check_error(std::int32_t code)
        {
            if(code != SQLITE_OK && code != SQLITE_DONE)
            {
                std::string errstr = std::string("SQL error: ") + sqlite3_errstr(code);
                CPP_SQLITE_THROW(errstr, code);
            }

            return true;
        }
    }

    struct Blob
    {
        Blob(const sqlite::byte* data, std::size_t bytes)
        {
            m_data.resize(bytes);
            std::memcpy(m_data.data(), data, bytes);
        }

        Blob(const std::vector<sqlite::byte>& data)
        : m_data(data)
        {

        }

        Blob(const Blob&) = default;

        Blob(Blob&&) = default;

        Blob& operator=(const Blob& other) = default;

        Blob& operator=(Blob&& other) = default;

        CPP_SQLITE_NODISCARD
        std::size_t get_size() const noexcept
        {
            return m_data.size();
        }

        CPP_SQLITE_NODISCARD
        sqlite::byte* get_data() noexcept
        {
            return m_data.data();
        }

        CPP_SQLITE_NODISCARD
        const sqlite::byte* get_data() const noexcept
        {
            return m_data.data();
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        T to() const
        {
            return T(m_data.data(), m_data.size());
        }

        CPP_SQLITE_NODISCARD
        std::string to_string() const
        {
            return std::string(
                reinterpret_cast<const char*>(m_data.data()), 
                m_data.size());
        }

#if __cplusplus >= 201703L
        CPP_SQLITE_NODISCARD
        std::string_view to_string_view() const noexcept
        {
            return std::string_view(
                reinterpret_cast<const char*>(m_data.data()), 
                m_data.size()
            );
        }
#endif

    private:
        std::vector<sqlite::byte> m_data;
    };

    /** Non-owning blob*/
    struct BlobView
    {
        BlobView() = delete;

        BlobView(const sqlite::byte* data, std::size_t number_of_bytes) noexcept
        : m_ptr(data), m_bytes(number_of_bytes)
        {

        }

        BlobView(const BlobView& other) noexcept
        : m_ptr(other.m_ptr), m_bytes(other.m_bytes)
        {

        }

        BlobView(BlobView&& other) noexcept
        : m_ptr(other.m_ptr), m_bytes(other.m_bytes)
        {
            other.m_ptr = nullptr;
            other.m_bytes = 0;
        }

        BlobView(const Blob& owningBlob)
        : m_ptr(owningBlob.get_data()), m_bytes(owningBlob.get_size())
        {

        }

        BlobView& operator=(const BlobView& other) noexcept
        {
            m_ptr = other.m_ptr;
            m_bytes = other.m_bytes;
            return *this;
        }

        BlobView& operator=(BlobView&& other) noexcept
        {
            m_ptr = other.m_ptr;
            m_bytes = other.m_bytes;
            other.m_ptr = nullptr;
            other.m_bytes = 0;
            return *this;
        }

        BlobView& operator=(const Blob& owning_blob)
        {
            m_ptr = owning_blob.get_data();
            m_bytes = owning_blob.get_size();
            return *this;
        }

        CPP_SQLITE_NODISCARD
        std::size_t get_size() const noexcept
        {
            return m_bytes;
        }

        CPP_SQLITE_NODISCARD
        const sqlite::byte* get_data() const noexcept
        {
            return m_ptr;
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        T to() const
        {
            return T(m_ptr, m_bytes);
        }

        CPP_SQLITE_NODISCARD
        std::string to_string() const
        {
            return std::string(
                reinterpret_cast<const char*>(m_ptr), 
                m_bytes
            );
        }

#if __cplusplus >= 201703L
        CPP_SQLITE_NODISCARD
        std::string_view to_string_view() const noexcept
        {
            return std::string_view(
                reinterpret_cast<const char*>(m_ptr), 
                m_bytes
            );
        }
#endif
    private:
        const sqlite::byte* m_ptr = nullptr;
        std::size_t m_bytes = 0;
    };

    struct Statement : NonCopyable
    {
        friend struct Connection;

        Statement() = delete;

        Statement(Statement&& other) noexcept
        {
            std::swap(m_handle, other.m_handle);
        }

        ~Statement()
        {
            if(m_handle)
            {
                sqlite::priv::check_error(sqlite3_finalize(m_handle));
            }
        }

        Statement& operator=(Statement&& other) noexcept
        {
            m_handle = other.m_handle;
            other.m_handle = nullptr;

            return *this;
        }

        template<typename First, typename ... Args>
        bool bind(const First& first, const Args&... args)
        {
            return reset() && bind(1, first, args...);
        }

        bool reset() const
        {
            return sqlite::priv::check_error(sqlite3_reset(m_handle));
        }

        bool evaluate()
        {
            const std::int32_t code = sqlite3_step(m_handle);

            if(code == SQLITE_ROW)
            {
                return true;
            }

            sqlite::priv::check_error(code);
            reset();

            return false;
        }

        bool bind(std::int32_t index, bool data)
        {
            return sqlite::priv::check_error(sqlite3_bind_int(m_handle, index, static_cast<std::int32_t>(data)));
        }

        bool bind(std::int32_t index, std::int32_t data)
        {
            return sqlite::priv::check_error(sqlite3_bind_int(m_handle, index, data));
        }

        bool bind(std::int32_t index, std::int64_t data)
        {
            return sqlite::priv::check_error(sqlite3_bind_int64(m_handle, index, data));
        }

        bool bind(std::int32_t index, float data)
        {
            return sqlite::priv::check_error(sqlite3_bind_double(m_handle, index, static_cast<double>(data)));
        }

        bool bind(std::int32_t index, double data)
        {
            return sqlite::priv::check_error(sqlite3_bind_double(m_handle, index, data));
        }

        bool bind(std::int32_t index, const std::string& data)
        {
            return sqlite::priv::check_error(sqlite3_bind_text(m_handle, index, data.data(), static_cast<std::int32_t>(data.size()), nullptr));
        }

        bool bind(std::int32_t index, const char* data)
        {
            return sqlite::priv::check_error(sqlite3_bind_text(m_handle, index, data, static_cast<std::int32_t>(std::strlen(data)), nullptr));
        }

        bool bind(std::int32_t index, const sqlite::Blob& blob)
        {
            return sqlite::priv::check_error(sqlite3_bind_blob(m_handle, index, blob.get_data(), static_cast<std::int32_t>(blob.get_size()), nullptr));
        }

        bool bind(std::int32_t index, const sqlite::BlobView& blob)
        {
            return sqlite::priv::check_error(sqlite3_bind_blob(m_handle, index, blob.get_data(), static_cast<std::int32_t>(blob.get_size()), nullptr));
        }

    private:
        template<typename First, typename ... Args>
        bool bind(std::int32_t index, const First& first, const Args&... args)
        {
            return bind(index, first) && bind(++index, args...);
        }

        Statement(sqlite3* connection_handle, const char* command)
        {
            const std::size_t command_len = std::strlen(command);
            const std::int32_t code = sqlite3_prepare_v2(
                    connection_handle,
                    command,
                    static_cast<std::int32_t>(command_len),
                    &m_handle,
                    nullptr);

            priv::check_error(connection_handle, code);
        }
    protected:
        sqlite3_stmt* m_handle = nullptr;
    };

    struct ResultBase
    {
        friend struct Query;
        friend struct Connection;

        struct Value : NonCopyable
        {
            Value(ResultBase& result, std::int32_t column_index)
            : m_result_ref(result), m_column_index(column_index)
            {

            }

            template<typename T>
            operator T() const
            {
                return m_result_ref.get<T>(m_column_index);
            }

        private:
            ResultBase& m_result_ref;
            std::int32_t m_column_index;
        };

        CPP_SQLITE_NODISCARD
        bool has_data() const
        {
            return column_count() > 0;
        }

        bool reset() const
        {
            return sqlite::priv::check_error(sqlite3_reset(m_statement));
        }

        CPP_SQLITE_NODISCARD
        bool next() const
        {
            const std::int32_t code = sqlite3_step(m_statement);

            if(code == SQLITE_ROW)
            {
                return true;
            }

            sqlite::priv::check_error(code);
            reset();

            return false;
        }

        CPP_SQLITE_NODISCARD
        std::int32_t column_count() const
        {
            reset();

            if(!next())
            {
                return 0;
            }

            const std::int32_t count = sqlite3_column_count(m_statement);
            reset();

            return count;
        }

        CPP_SQLITE_NODISCARD
        Value get_value(std::int32_t column_index)
        {
            return Value(*this, column_index);
        }

        CPP_SQLITE_NODISCARD
        Value get_value(const char* column_name)
        {
            const std::int32_t column_index = get_column_index(column_name);
            return Value(*this, column_index);
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        T get(std::int32_t column_index) const
        {
            (void)column_index;
            static_assert(sizeof(T) == -1, "SQL error: invalid column data type");
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        T get(const char* column_name) const
        {
            const std::int32_t column_index = get_column_index(column_name);
            return get<T>(column_index); 
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        T get(const std::string& column_name) const
        {
            return get<T>(column_name.c_str());
        }

#if __cplusplus >= 201703L
        template<typename T>
        CPP_SQLITE_NODISCARD
        T get(std::string_view column_name) const
        {
            return get<T>(column_name.data());
        }

        CPP_SQLITE_NODISCARD
        std::int32_t get_column_index(const char* column_name) const
        {
            return get_column_index(std::string_view(column_name));
        }

        CPP_SQLITE_NODISCARD
        std::int32_t get_column_index(std::string_view column_name) const
        {
            const auto it = m_column_index.find(column_name);
            if (it == m_column_index.end())
            {
                return -1;
            }
            return it->second;
        }
#else
        CPP_SQLITE_NODISCARD
        std::int32_t get_column_index(const char* column_name) const
        {
            return get_column_index(std::string(column_name));
        }
#endif

        CPP_SQLITE_NODISCARD
        std::int32_t get_column_index(const std::string& column_name) const
        {
            const auto it = m_column_index.find(column_name);
            if (it == m_column_index.end())
            {
                return -1;
            }
            return it->second;
        }

    protected:
        sqlite3_stmt* m_statement = nullptr;
#if __cplusplus >= 201703L
        std::unordered_map<std::string_view, std::int32_t> m_column_index;
#else
        std::unordered_map<std::string, std::int32_t> m_column_index;
#endif

    };

    /* Non-owning result */
    struct ResultView : ResultBase
    {
        friend struct Query;
        friend struct Connection;

        ResultView() = delete;

        ResultView(const ResultView& other) = default;

        ~ResultView() = default;

        ResultView& operator=(const ResultView& other) = default;

    private:
        explicit ResultView(sqlite3_stmt*& statement)
        {
            m_statement = statement;
            const std::int32_t column_count = sqlite3_column_count(m_statement);

            for (std::int32_t i = 0; i < column_count; ++i)
            {
                m_column_index.emplace(
                    sqlite3_column_name(m_statement, i),
                    i
                );
            }
        }
    };

    struct Result : ResultBase, NonCopyable
    {
        friend struct Query;
        friend struct Connection;

        Result() = delete;

        Result(Result&& other) noexcept
        {
            std::swap(m_statement, other.m_statement);
        }

        Result& operator=(Result&& other)
        {
            m_statement = other.m_statement;
            other.m_statement = nullptr;
            return *this;
        }

        ~Result()
        {
            sqlite3_finalize(m_statement);
        }
    private:
        explicit Result(sqlite3_stmt*& statement)
        {
            std::swap(m_statement, statement);
            const std::int32_t column_count = sqlite3_column_count(m_statement);

            for (std::int32_t i = 0; i < column_count; ++i)
            {
                m_column_index.emplace(
                    sqlite3_column_name(m_statement, i),
                    i
                );
            }
        }
    };

    struct Query : Statement
    {
        using Statement::Statement;
        friend struct Connection;

        ResultView execute()
        {
            return ResultView(m_handle);
        }
    };

    struct Connection : NonCopyable
    {
        Connection() : m_connection(nullptr) {}

        Connection(const char* path)
        {
            this->open(path);
        }

        explicit Connection(const std::string& path)
        {
            this->open(path);
        }

#if __cplusplus >= 201703L
        explicit Connection(std::string_view path)
        {
            this->open(path);
        }

        explicit Connection(const std::filesystem::path& path)
        {
            this->open(path);
        }
#endif

        Connection(Connection&& other) noexcept
        {
            this->m_connection = other.m_connection;
            other.m_connection = nullptr;
        }

        virtual ~Connection()
        {
            this->close();
        }

        Connection& operator=(Connection&& other) noexcept
        {
            if(&other != this)
            {
                this->m_connection = other.m_connection;
                other.m_connection = nullptr;
            }

            return *this;
        }

        bool backup(const char* path) const
        {
            Connection connection(path);

            return backup(connection);
        }

        bool backup(const std::string& path) const
        {
            return backup(path.c_str());
        }

        bool backup(Connection& backup) const
        {
            sqlite3_backup* backupHandle = sqlite3_backup_init(backup.get_handle(),
                                                               "main",
                                                               get_handle(),
                                                               "main");
            if(!backupHandle)
            {
                CPP_SQLITE_THROW("SQL error: Failed to initialize backup");
            }

            if(!sqlite::priv::check_error(sqlite3_backup_step(backupHandle, -1)))
            {
                CPP_SQLITE_THROW("SQL error: Could not execute backup");
            }

            if(!sqlite::priv::check_error(get_handle(), sqlite3_backup_finish(backupHandle)))
            {
                CPP_SQLITE_THROW("SQL error: Could not finish backup");
            }

            return true;
        }

        bool statement(const char* command) const
        {
            sqlite::Statement statement(get_handle(), command);

            return statement.evaluate();
        }

        bool statement(const std::string& command) const
        {
            return statement(command.c_str());
        }

        template<typename First, typename ... Args>
        bool statement(const char* command, const First& first, const Args&... args)
        {
            sqlite::Statement statement(get_handle(), command);
            statement.bind(first, args...);

            return statement.evaluate();
        }

        template<typename First, typename ... Args>
        bool statement(const std::string& command, const First& first, const Args&... args)
        {
            return statement(command.c_str(), first, args...);
        }

        CPP_SQLITE_NODISCARD
        Result query(const char* command) const
        {
            sqlite::Query query(get_handle(), command);

            return Result(query.m_handle);
        }

        CPP_SQLITE_NODISCARD
        Result query(const std::string& command) const
        {
            return query(command.c_str());
        }

        template<typename First, typename ... Args>
        CPP_SQLITE_NODISCARD
        Result query(const char* command, const First& first, const Args&... args) const
        {
            sqlite::Query query(get_handle(), command);
            query.bind(first, args...);

            return Result(query.m_handle);
        }

        template<typename First, typename ... Args>
        CPP_SQLITE_NODISCARD
        Result query(const std::string& command, const First& first, const Args&... args) const
        {
            return query(command.c_str(), first, args...);
        }

        CPP_SQLITE_NODISCARD
        sqlite::Statement prepare_statement(const char* command) const
        {
            return sqlite::Statement(get_handle(), command);
        }

        CPP_SQLITE_NODISCARD
        sqlite::Statement prepare_statement(const std::string& command) const
        {
            return prepare_statement(command.c_str());
        }

        CPP_SQLITE_NODISCARD
        sqlite::Query prepare_query(const char* command) const
        {
            sqlite::Query query(get_handle(), command);

            return query;
        }

        CPP_SQLITE_NODISCARD
        sqlite::Query prepare_query(const std::string& command) const
        {
            return prepare_query(command.c_str());
        }

        bool open(const char* path)
        {
            return sqlite::priv::check_error(sqlite3_open(path, &m_connection));
        }

        bool open(const std::string& path)
        {
            return open(path.c_str());
        }

#if __cplusplus >= 201703L
        bool backup(std::string_view path) const
        {
            return backup(path.data());
        }

        bool backup(const std::filesystem::path& path) const
        {
            return backup(path.c_str());
        }

        bool statement(std::string_view command) const
        {
            return statement(command.data());
        }

        template<typename First, typename ... Args>
        bool statement(std::string_view command, const First& first, const Args&... args)
        {
            return statement(command.data(), first, args...);
        }

        CPP_SQLITE_NODISCARD
        Result query(std::string_view command) const
        {
            return query(command.data());
        }

        template<typename First, typename ... Args>
        CPP_SQLITE_NODISCARD
        Result query(std::string_view command, const First& first, const Args&... args) const
        {
            return query(command.data(), first, args...);
        }

        CPP_SQLITE_NODISCARD
        sqlite::Statement prepare_statement(std::string_view command) const
        {
            return prepare_statement(command.data());
        }

        CPP_SQLITE_NODISCARD
        sqlite::Query prepare_query(std::string_view command) const
        {
            return prepare_query(command.data());
        }

        bool open(std::string_view path)
        {
            return open(path.data());
        }

        bool open(const std::filesystem::path& path)
        {
            return open(path.c_str());
        }
#endif

        bool close()
        {
            if (m_connection == nullptr)
            {
                return true;
            }

            const auto result = priv::check_error(sqlite3_close(m_connection));
            m_connection = nullptr;

            return result;
        }

        CPP_SQLITE_NODISCARD
        std::int32_t get_extended_error_code() const
        {
            return sqlite3_extended_errcode(m_connection);
        }

        CPP_SQLITE_NODISCARD
        sqlite3* get_handle() const noexcept
        {
            return m_connection;
        }

    protected:
        sqlite3* m_connection = nullptr;
    };


    template<>
    inline bool ResultBase::get(std::int32_t col) const
    {
        return sqlite3_column_int(m_statement, col) != 0;
    }

    template<>
    inline float ResultBase::get(std::int32_t col) const
    {
        return static_cast<float>(sqlite3_column_double(m_statement, col));
    }

    template<>
    inline double ResultBase::get(std::int32_t col) const
    {
        return sqlite3_column_double(m_statement, col);
    }

    template<>
    inline std::int32_t ResultBase::get(std::int32_t col) const
    {
        return sqlite3_column_int(m_statement, col);
    }

    template<>
    inline std::int64_t ResultBase::get(std::int32_t col) const
    {
        return sqlite3_column_int64(m_statement, col);
    }

    template<>
    inline std::string ResultBase::get(std::int32_t col) const
    {
        const char* bytes = reinterpret_cast<const char*>(sqlite3_column_text(m_statement, col));

        const std::size_t size = static_cast<std::size_t>(
            sqlite3_column_bytes(m_statement, col)
        );

        if(size == 0)
        {
            return "";
        }

        return std::string(bytes, size);
    }

    template<>
    inline sqlite::Blob ResultBase::get(std::int32_t col) const
    {
        const sqlite::byte* bytes = static_cast<const sqlite::byte*>(
            sqlite3_column_blob(m_statement, col)
        );

        const std::size_t size = static_cast<std::size_t>(
            sqlite3_column_bytes(m_statement, col)
        );

        return sqlite::Blob(bytes, size);
    }

    template<>
    inline sqlite::BlobView ResultBase::get(std::int32_t col) const
    {
        const sqlite::byte* bytes = static_cast<const sqlite::byte*>(
            sqlite3_column_blob(m_statement, col)
        );

        const std::size_t size = static_cast<std::size_t>(
            sqlite3_column_bytes(m_statement, col)
        );

        return sqlite::BlobView(bytes, size);
    }
}
