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

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <type_traits>
#include <limits>

#define CPP_SQLITE_VERSION        "1.0.0"
#define CPP_SQLITE_VERSION_NUMBER 1000000

#if __cplusplus >= 201103L
    #define CPP_SQLITE_CONSTEXPR constexpr
#else
    #define CPP_SQLITE_CONSTEXPR
#endif

#if __cplusplus >= 201402L
    #define CPP_SQLITE_CONSTEXPR14 constexpr
#else
    #define CPP_SQLITE_CONSTEXPR14
#endif

#if __cplusplus >= 201703L
    #define CPP_SQLITE_CONSTEXPR17 constexpr
#else
    #define CPP_SQLITE_CONSTEXPR17
#endif

#if __cplusplus >= 201703L
    #include <optional>
    #include <string_view>
    #define CPP_SQLITE_UNUSED [[maybe_unused]]
    #define CPP_SQLITE_NODISCARD [[nodiscard]]
#else
    #if defined(CPP_SQLITE_IOSTREAM)
        #include <ostream>
    #endif
    #if defined(__GNUC__) || defined(__clang__)
        #define CPP_SQLITE_UNUSED __attribute__((unused))
    #else
        #define CPP_SQLITE_UNUSED
    #endif
    #define CPP_SQLITE_NODISCARD
#endif

#if !defined(__EXCEPTIONS) && !defined(_CPPUNWIND) && !defined(CPP_SQLITE_NOTHROW)
    #define CPP_SQLITE_NOTHROW
#endif

#if defined(CPP_SQLITE_NOTHROW)
    #define CPP_SQLITE_ASSERT_THROW(condition, ex, msg) if (condition) std::abort() 
    #define CPP_SQLITE_THROW(...) return false
#else
    #define CPP_SQLITE_ASSERT_THROW(condition, ex, msg) if (condition) throw ex(msg #condition)
    #define CPP_SQLITE_THROW(...) throw sqlite::Exception(__VA_ARGS__)
#endif

namespace sqlite
{
    using byte = unsigned char;

    struct null_t {};
#if __cplusplus >= 201703L
    inline constexpr null_t null{};
#else
    static const null_t null{};
#endif

#if __cplusplus >= 201703L
    using string_view = std::string_view;

    template<typename T>
    using optional = std::optional<T>;
    using nullopt_t = std::nullopt_t;
    using bad_optional_access = std::bad_optional_access;

    inline constexpr std::nullopt_t nullopt = std::nullopt;

#else
    class string_view
    {
    public:
        using traits_type                   = std::char_traits<char>;
        using value_type		            = char;
        using pointer		                = value_type*;
        using const_pointer	                = const value_type*;
        using reference		                = value_type&;
        using const_reference	            = const value_type&;
        using const_iterator	            = const value_type*;
        using iterator		                = const_iterator;
        using const_reverse_iterator        = std::reverse_iterator<const_iterator>;
        using reverse_iterator	            = const_reverse_iterator;
        using size_type		                = std::size_t;
        using difference_type               = std::ptrdiff_t;
        static constexpr size_type npos     = size_type(-1);
    public:
        CPP_SQLITE_CONSTEXPR
        string_view() noexcept 
        : m_len(0), m_str(nullptr) {}

        CPP_SQLITE_CONSTEXPR17
        string_view(const_pointer cstr) noexcept 
        : m_len(traits_type::length(cstr)), m_str(cstr) {}

        CPP_SQLITE_CONSTEXPR
        string_view(const_pointer cstr, size_type len) noexcept 
        : m_len(len), m_str(cstr) {}

        CPP_SQLITE_CONSTEXPR
        string_view(const string_view& other) noexcept
        : m_len(other.m_len), m_str(other.m_str) {}

        CPP_SQLITE_CONSTEXPR17
        explicit string_view(const std::string& str) noexcept
        : m_len(str.size()), m_str(str.data()) {}

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const_reference operator[](size_type pos) const noexcept 
        { 
            assert(m_len > pos);
            return *(m_str + pos); 
        }

        CPP_SQLITE_CONSTEXPR17
        string_view& operator=(const_pointer rhs) noexcept { *this = string_view{rhs}; return *this; }

        CPP_SQLITE_CONSTEXPR14
        string_view& operator=(string_view rhs) noexcept 
        { 
            m_len = rhs.m_len;
            m_str = rhs.m_str;
            return *this; 
        }

        CPP_SQLITE_CONSTEXPR17
        string_view& operator=(const std::string& rhs) noexcept { *this = string_view{rhs}; return *this; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        bool operator==(string_view rhs) const noexcept { return compare(rhs) == 0; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        bool operator==(const_pointer rhs) const noexcept { return compare(string_view{rhs}) == 0; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        bool operator==(const std::string& rhs) const noexcept { return compare(string_view{rhs}) == 0; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        bool operator<(string_view rhs) const noexcept { return compare(rhs) < 0; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        bool operator>(string_view rhs) const noexcept { return compare(rhs) > 0; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        bool operator<=(string_view rhs) const noexcept { return compare(rhs) <= 0; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        bool operator>=(string_view rhs) const noexcept { return compare(rhs) >= 0; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        const_pointer data() const noexcept { return m_str; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        size_type size() const noexcept { return m_len; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        size_type length() const noexcept { return m_len; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        size_type max_size() const noexcept { return npos - sizeof(size_type) - sizeof(void*); }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        bool empty() const noexcept { return m_len == 0; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        string_view substr(size_type pos, size_type n = string_view::npos) const noexcept
        {
            if (m_len == 0 || pos >= m_len)
                return string_view();
            const size_type max_len = m_len - pos;
            const size_type len = n >= max_len ? max_len : n;
            return string_view(m_str + pos, len);
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const_reference at(size_type pos) const
        {
            CPP_SQLITE_ASSERT_THROW(pos >= m_len, std::out_of_range, "string_view::at() ");
            return m_str[pos];
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        const_iterator begin() const noexcept { return m_str; }
        
        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        const_iterator end() const noexcept { return m_str + m_len; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        const_iterator cbegin() const noexcept { return m_str; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        const_iterator cend() const noexcept { return m_str + m_len; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const_reference front() const noexcept
        {
            assert(m_len > 0);
            return *m_str;
        } 

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const_reference back() const noexcept
        {
            assert(m_len > 0);
            return *(m_str + m_len - 1);
        }

        CPP_SQLITE_CONSTEXPR14
        void remove_prefix(size_type n) noexcept
        {
            assert(m_len > n);
            m_str += n;
            m_len -= n;
        }

        CPP_SQLITE_CONSTEXPR14
        void remove_suffix(size_type n) noexcept
        {
            assert(m_len > n);
            m_len -= n;
        }

        CPP_SQLITE_CONSTEXPR14
        void swap(string_view& sv) noexcept
        {
            const_pointer _temp_str = m_str;
            size_type _temp_len = m_len;
            m_str = sv.m_str;
            m_len = sv.m_len;
            sv.m_str = _temp_str;
            sv.m_len = _temp_len;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        int compare(string_view str) const noexcept
        {
            const size_type rlen = m_len <= str.m_len ? m_len : str.m_len; 
            int ret = traits_type::compare(m_str, str.m_str, rlen);
            if (ret == 0)
                ret = _S_compare(m_len, str.m_len);
            return ret;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        int compare(size_type pos1, size_type n1, string_view str) const noexcept
        {
            return substr(pos1, n1).compare(str);
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        int compare(size_type pos1, size_type n1, string_view str, size_type pos2, size_type n2) const
        {
            return substr(pos1, n1).compare(str.substr(pos2, n2));
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        int compare(const_pointer str) const noexcept
        {
            return compare(string_view{str});
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        int compare(size_type pos1, size_type n1, const_pointer str) const
        {
            return substr(pos1, n1).compare(string_view{str});
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        int compare(size_type pos1, size_type n1, const_pointer str, size_type n2) const
        {
            return substr(pos1, n1).compare(string_view{str, n2});
        }
    private: 
        CPP_SQLITE_CONSTEXPR14
        static int _S_compare(size_type n1, size_type n2) noexcept
        {
            using limits = std::numeric_limits<int>;
            constexpr difference_type max = static_cast<difference_type>(limits::max());
            constexpr difference_type min = static_cast<difference_type>(limits::min());
            const difference_type diff = n1 - n2;
            if (diff > max) return max;
            if (diff < min) return min;

            return static_cast<int>(diff);
        }
    private:
        size_type m_len;
        const_pointer m_str;
    };

#if defined(CPP_SQLITE_IOSTREAM) && __cplusplus < 201703L
    inline std::ostream& operator<<(std::ostream& out, string_view sv)
    {
        out.write(sv.data(), sv.size());
        return out;
    }
#endif

    struct nullopt_t{};

    static const nullopt_t nullopt{};

    class bad_optional_access : public std::exception
    {
    public:
        const char* what() const noexcept override
        {
            return "bad optional access";
        }
    };

    template<typename T>
    class optional
    {
    public:
        CPP_SQLITE_CONSTEXPR
        optional() noexcept
        : m_has_value(false) {}

        CPP_SQLITE_CONSTEXPR14
        optional(nullopt_t) noexcept
        : m_has_value(false) {}

        CPP_SQLITE_CONSTEXPR14
        optional(const T& value)
        : m_has_value(true)
        {
            new (&m_storage) T(value);
        }

        CPP_SQLITE_CONSTEXPR14
        optional(T&& value)
        : m_has_value(true)
        {
            new (&m_storage) T(std::move(value));
        }

        CPP_SQLITE_CONSTEXPR14
        optional(const optional& other)
        : m_has_value(other.m_has_value)
        {
            if (m_has_value)
                new (&m_storage) T(*other.ptr());
        }

        CPP_SQLITE_CONSTEXPR14
        optional(optional&& other)
        : m_has_value(other.m_has_value)
        {
            if (m_has_value)
                new (&m_storage) T(std::move(*other.ptr()));
        }

        ~optional()
        {
            reset();
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        bool has_value() const noexcept
        {
            return m_has_value;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        explicit operator bool() const noexcept
        {
            return m_has_value;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        T& operator*() noexcept
        {
            return value();
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const T& operator*() const noexcept
        {
            return value();
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        T* operator->() noexcept
        {
            return ptr();
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const T* operator->() const noexcept
        {
            return ptr();
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        T& value()
        {
            if (!m_has_value)
            {
#ifndef CPP_SQLITE_NOTHROW
                throw bad_optional_access();
#else
                std::abort();
#endif
            }

            return *ptr();
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const T& value() const
        {
            if (!m_has_value)
            {
#ifndef CPP_SQLITE_NOTHROW
                throw bad_optional_access();
#else
                std::abort();
#endif
            }

            return *ptr();
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        T value_or(const T& fallback) const noexcept
        {
            if (m_has_value)
                return value();
            return fallback;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        void reset()
        {
            if (!m_has_value)
                return;
            ptr()->~T();
            m_has_value = false;
        }

        template<typename... Args>
        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        void emplace(Args&&... args)
        {
            reset();
            new (&m_storage) T(std::forward<Args>(args)...);
            m_has_value = false;
        }

    private:
        CPP_SQLITE_NODISCARD
        T* ptr() noexcept
        {
            return reinterpret_cast<T*>(&m_storage);
        }

        CPP_SQLITE_NODISCARD
        const T* ptr() const noexcept
        {
            return reinterpret_cast<const T*>(&m_storage);
        }
    private:
        typename std::aligned_storage<sizeof(T), alignof(T)>::type m_storage;
        bool m_has_value = false;
    };
#endif

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
        : std::runtime_error(message), m_error_code(error_code) {}

        explicit Exception(const std::string& message, std::int32_t error_code = SQLITE_ERROR)
        : std::runtime_error(message), m_error_code(error_code) {}

        CPP_SQLITE_NODISCARD
        std::int32_t get_code() const noexcept
        {
            return m_error_code;
        }

    private:
        std::int32_t m_error_code;
    };

    struct Blob
    {
        Blob(const sqlite::byte* data, std::size_t size)
        : m_data(nullptr), m_size(size)
        {
            if (m_size == 0)
                return;

            m_data = static_cast<sqlite::byte*>(std::malloc(m_size));

            if (m_data == nullptr)
            {
#ifdef CPP_SQLITE_NOTHROW
                std::abort();
#else
                throw std::bad_alloc();
#endif
            }

            std::memcpy(m_data, data, m_size);
        }

        Blob(const Blob& other)
        : m_data(nullptr), m_size(other.m_size)
        {
            if (m_size == 0)
                return;
            m_data = static_cast<sqlite::byte*>(std::malloc(m_size));
            std::memcpy(m_data, other.m_data, m_size);
        }

        Blob(Blob&& other) noexcept
        : m_data(other.m_data), m_size(other.m_size)
        {
            other.m_data = nullptr;
            other.m_size = 0;
        }

        ~Blob()
        {
            std::free(m_data);
        }

        Blob& operator=(const Blob& other)
        {
            if (&other == this)
            {
                return *this;
            }

            if (other.m_size == 0)
            {
                std::free(m_data);
                m_data = nullptr;
                m_size = 0;
                return *this;
            }

            void* new_data = std::realloc(m_data, other.m_size);

            if (new_data == nullptr)
            {
#ifdef CPP_SQLITE_NOTHROW
                std::abort();
#else
                throw std::bad_alloc();
#endif
            }

            m_size = other.m_size;
            m_data = static_cast<sqlite::byte*>(new_data);
            std::memcpy(m_data, other.m_data, m_size);

            return *this;
        }

        Blob& operator=(Blob&& other) noexcept
        {
            if (&other == this)
            {
                return *this;
            }

            std::free(m_data);
            m_data = other.m_data;
            m_size = other.m_size;
            other.m_data = nullptr;
            other.m_size = 0;

            return *this;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        bool empty() const noexcept
        {
            return m_size == 0;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        std::size_t get_size() const noexcept
        {
            return m_size;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        sqlite::byte* get_data() noexcept
        {
            return m_data;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const sqlite::byte* get_data() const noexcept
        {
            return m_data;
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        T to() const
        {
            return T(m_data, m_size);
        }

        CPP_SQLITE_NODISCARD
        std::string to_string() const
        {
            return std::string(
                reinterpret_cast<const char*>(m_data), 
                m_size
            );
        }

        CPP_SQLITE_NODISCARD
        sqlite::string_view to_string_view() const noexcept
        {
            return sqlite::string_view(
                reinterpret_cast<const char*>(m_data), 
                m_size
            );
        }
    private:
        sqlite::byte* m_data = nullptr;
        std::size_t m_size = 0;
    };

    /** Non-owning blob*/
    struct BlobView
    {
        BlobView() = delete;

        CPP_SQLITE_CONSTEXPR
        BlobView(const sqlite::byte* data, std::size_t size) noexcept
        : m_data(data), m_size(size)
        {

        }

        CPP_SQLITE_CONSTEXPR
        BlobView(const BlobView& other) noexcept
        : m_data(other.m_data), m_size(other.m_size)
        {

        }

        CPP_SQLITE_CONSTEXPR14
        BlobView(BlobView&& other) noexcept
        : m_data(other.m_data), m_size(other.m_size)
        {
            other.m_data = nullptr;
            other.m_size = 0;
        }

        CPP_SQLITE_CONSTEXPR14
        BlobView(const Blob& owning_blob) noexcept
        : m_data(owning_blob.get_data()), m_size(owning_blob.get_size())
        {

        }

        CPP_SQLITE_CONSTEXPR14
        BlobView& operator=(const BlobView& other) noexcept
        {
            m_data = other.m_data;
            m_size = other.m_size;
            return *this;
        }

        CPP_SQLITE_CONSTEXPR14
        BlobView& operator=(BlobView&& other) noexcept
        {
            if (&other == this)
                return *this;
            m_data = other.m_data;
            m_size = other.m_size;
            other.m_data = nullptr;
            other.m_size = 0;
            return *this;
        }

        CPP_SQLITE_CONSTEXPR14
        BlobView& operator=(const Blob& owning_blob) noexcept
        {
            m_data = owning_blob.get_data();
            m_size = owning_blob.get_size();
            return *this;
        }

        CPP_SQLITE_NODISCARD
        CPP_SQLITE_CONSTEXPR
        std::size_t empty() const noexcept
        {
            return m_size == 0;
        }

        CPP_SQLITE_NODISCARD
        CPP_SQLITE_CONSTEXPR
        std::size_t get_size() const noexcept
        {
            return m_size;
        }

        CPP_SQLITE_NODISCARD
        CPP_SQLITE_CONSTEXPR
        const sqlite::byte* get_data() const noexcept
        {
            return m_data;
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        T to() const
        {
            return T(m_data, m_size);
        }

        CPP_SQLITE_NODISCARD
        std::string to_string() const
        {
            return std::string(
                reinterpret_cast<const char*>(m_data), 
                m_size
            );
        }

        CPP_SQLITE_NODISCARD
        sqlite::string_view to_string_view() const noexcept
        {
            return sqlite::string_view(
                reinterpret_cast<const char*>(m_data), 
                m_size
            );
        }
    private:
        const sqlite::byte* m_data = nullptr;
        std::size_t m_size = 0;
    };

    enum class ColumnType
    {
        Null,
        Integer,
        Real,
        Text,
        Blob,
        Unknown,
    };
    
    namespace detail
    {
        inline bool check_error(CPP_SQLITE_UNUSED sqlite3* db, std::int32_t code)
        {
            if (code == SQLITE_OK || code == SQLITE_DONE)
                return true;
#ifndef CPP_SQLITE_NOTHROW
            const std::int32_t extended_code = sqlite3_extended_errcode(db);
            std::string errstr = sqlite3_errstr(extended_code);
            std::string errmsg = sqlite3_errmsg(db);
            throw sqlite::Exception(errstr + ": " + errmsg, extended_code);
#else
            return false;
#endif
        }

        inline bool check_error(std::int32_t code)
        {
            if (code == SQLITE_OK || code == SQLITE_DONE)
                return true;
#ifndef CPP_SQLITE_NOTHROW
            std::string errstr = std::string("SQL error: ") + sqlite3_errstr(code);
            throw sqlite::Exception(errstr, code);
#else
            return false;
#endif
        }
    
        struct Result
        {
            struct Value : NonCopyable
            {
                Value(Result& result, std::int32_t column_index)
                : m_result_ref(result), m_column_index(column_index)
                {

                }

                template<typename T>
                operator T() const
                {
                    return m_result_ref.get<T>(m_column_index);
                }

            private:
                Result& m_result_ref;
                std::int32_t m_column_index;
            };

            bool reset() const
            {
                return sqlite::detail::check_error(sqlite3_reset(m_statement));
            }

            CPP_SQLITE_NODISCARD
            bool next() const
            {
                const std::int32_t code = sqlite3_step(m_statement);

                if (code == SQLITE_ROW)
                    return true;
                if (!sqlite::detail::check_error(code))
                    return false;
                reset();
                return false;
            }

            CPP_SQLITE_NODISCARD
            std::int32_t column_count() const
            {
                return sqlite3_column_count(m_statement);
            }

            CPP_SQLITE_NODISCARD
            bool is_null(std::int32_t column_index) const
            {
                return sqlite3_column_type(m_statement, column_index) == SQLITE_NULL;
            }

            CPP_SQLITE_NODISCARD
            bool is_null(sqlite::string_view column_name) const
            {
                const std::int32_t column_index = get_column_index(column_name);
                return is_null(column_index);
            }

            CPP_SQLITE_NODISCARD
            Value get_value(std::int32_t column_index)
            {
                return Value(*this, column_index);
            }

            CPP_SQLITE_NODISCARD
            Value get_value(sqlite::string_view column_name)
            {
                const std::int32_t column_index = get_column_index(column_name);
                return Value(*this, column_index);
            }

            template<typename T>
            CPP_SQLITE_NODISCARD
            sqlite::optional<T> get_nullable(std::int32_t column_index) const
            {
                if (is_null(column_index))
                    return {};
                return get<T>(column_index);
            }

            template<typename T>
            CPP_SQLITE_NODISCARD
            sqlite::optional<T> get_nullable(sqlite::string_view column_name) const
            {
                const std::int32_t column_index = get_column_index(column_name);
                if (is_null(column_index))
                    return {};
                return get<T>(column_index);
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
            T get(sqlite::string_view column_name) const
            {
                const std::int32_t column_index = get_column_index(column_name);
                return get<T>(column_index); 
            }

            CPP_SQLITE_NODISCARD
            std::int32_t get_column_index(sqlite::string_view column_name) const
            {
                std::string str {column_name.data(), column_name.size()};
                const auto it = m_column_index.find(str);
                if (it == m_column_index.end())
                {
                    return -1;
                }
                return it->second;
            }

            CPP_SQLITE_NODISCARD
            ColumnType get_column_type(std::int32_t column_index) const
            {
                const std::int32_t type_int = sqlite3_column_type(m_statement, column_index);
                switch (type_int)
                {
                    case SQLITE_INTEGER:
                        return ColumnType::Integer;
                    case SQLITE_FLOAT:
                        return ColumnType::Real;
                    case SQLITE_TEXT:
                        return ColumnType::Text;
                    case SQLITE_NULL:
                        return ColumnType::Null;
                    case SQLITE_BLOB:
                        return ColumnType::Blob;
                    default:
                        return ColumnType::Unknown;
                }
                return ColumnType::Unknown;
            }

            CPP_SQLITE_NODISCARD
            ColumnType get_column_type(sqlite::string_view column_name) const
            {
                const std::int32_t column_index = get_column_index(column_name);
                return get_column_type(column_index);
            }
        protected:
            sqlite3_stmt* m_statement = nullptr;
            std::unordered_map<std::string, std::int32_t> m_column_index;
        };

        struct Prepared : NonCopyable
        {
            Prepared() = default;
            Prepared(Prepared&& other) noexcept
            : m_handle(other.m_handle)
            {
                other.m_handle = nullptr;
            }

            ~Prepared()
            {
                if (m_handle)
                    sqlite::detail::check_error(sqlite3_finalize(m_handle));
            }

            Prepared& operator=(Prepared&& other) noexcept
            {
                if (&other == this)
                    return *this;
                m_handle = nullptr;
                std::swap(m_handle, other.m_handle);
                return *this;
            }

            bool reset() const
            {
                return sqlite::detail::check_error(sqlite3_reset(m_handle));
            }

            template<typename First, typename ... Args>
            bool bind(const First& first, const Args&... args)
            {
                return reset() && expand_bind(1, first, args...);
            }

            template<typename T>
            bool bind(std::int32_t index, const sqlite::optional<T>& data)
            {
                if (data.has_value())
                    return bind(index, data.value());
                return sqlite::detail::check_error(sqlite3_bind_null(m_handle, index));
            }

            bool bind(std::int32_t index, sqlite::null_t data)
            {
                (void)data;
                return sqlite::detail::check_error(sqlite3_bind_null(m_handle, index));
            }

            bool bind(std::int32_t index, bool data)
            {
                return sqlite::detail::check_error(sqlite3_bind_int(m_handle, index, static_cast<std::int32_t>(data)));
            }

            bool bind(std::int32_t index, std::int32_t data)
            {
                return sqlite::detail::check_error(sqlite3_bind_int(m_handle, index, data));
            }

            bool bind(std::int32_t index, std::int64_t data)
            {
                return sqlite::detail::check_error(sqlite3_bind_int64(m_handle, index, data));
            }

            bool bind(std::int32_t index, float data)
            {
                return sqlite::detail::check_error(sqlite3_bind_double(m_handle, index, static_cast<double>(data)));
            }

            bool bind(std::int32_t index, double data)
            {
                return sqlite::detail::check_error(sqlite3_bind_double(m_handle, index, data));
            }

            bool bind(std::int32_t index, const char* data)
            {
                return bind(index, sqlite::string_view{data});
            }

            bool bind(std::int32_t index, sqlite::string_view data)
            {
                return sqlite::detail::check_error(sqlite3_bind_text(m_handle, index, data.data(), static_cast<std::int32_t>(data.size()), nullptr));
            }

            bool bind(std::int32_t index, const sqlite::Blob& blob)
            {
                return sqlite::detail::check_error(sqlite3_bind_blob(m_handle, index, blob.get_data(), static_cast<std::int32_t>(blob.get_size()), nullptr));
            }

            bool bind(std::int32_t index, const sqlite::BlobView& blob)
            {
                return sqlite::detail::check_error(sqlite3_bind_blob(m_handle, index, blob.get_data(), static_cast<std::int32_t>(blob.get_size()), nullptr));
            }

        private:
            template<typename First, typename ... Args>
            bool expand_bind(std::int32_t index, const First& first, const Args&... args)
            {
                return bind(index, first) && expand_bind(++index, args...);
            }
            bool expand_bind(std::int32_t)
            {
                return true;
            }

        protected:
            sqlite3_stmt* m_handle = nullptr;
        };
    }

    struct Statement : sqlite::detail::Prepared
    {
        friend struct Connection;

        Statement() = delete;

        Statement(Statement&& other) noexcept
        {
            std::swap(m_handle, other.m_handle);
        }

        Statement& operator=(Statement&& other) noexcept
        {
            m_handle = other.m_handle;
            other.m_handle = nullptr;

            return *this;
        }

        bool reset() const
        {
            return sqlite::detail::check_error(sqlite3_reset(m_handle));
        }

        bool execute()
        {
            const std::int32_t code = sqlite3_step(m_handle);

            if (code == SQLITE_ROW)
                return true;
            if (!sqlite::detail::check_error(code))
                return false;
            if (!reset())
                return false;
            return true;
        }
    private:
        Statement(sqlite3* connection_handle, sqlite::string_view command)
        {
            const std::int32_t code = sqlite3_prepare_v2(
                    connection_handle,
                    command.data(),
                    static_cast<std::int32_t>(command.size()),
                    &m_handle,
                    nullptr);

            detail::check_error(connection_handle, code);
        }
    };

    /* Non-owning result */
    struct ResultView : sqlite::detail::Result
    {
        friend struct Query;
        friend struct Connection;

        ResultView() = delete;

        ResultView(const ResultView& other) = default;

        ~ResultView() = default;

        ResultView& operator=(const ResultView& other) = default;

    private:
        explicit ResultView(sqlite3_stmt* statement)
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

    struct Result : sqlite::detail::Result, NonCopyable
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
            if (m_statement)
            {
                detail::check_error(sqlite3_finalize(m_statement));
                m_statement = nullptr;
            }
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

    struct Query : sqlite::detail::Prepared 
    {
        friend struct Connection;

        ResultView execute()
        {
            return ResultView(m_handle);
        }
    private:
        Query(sqlite3* connection_handle, sqlite::string_view command)
        {
            const std::int32_t code = sqlite3_prepare_v2(
                    connection_handle,
                    command.data(),
                    static_cast<std::int32_t>(command.size()),
                    &m_handle,
                    nullptr);

            detail::check_error(connection_handle, code);
        }
    };

    struct Connection : NonCopyable
    {
        Connection() : m_handle(nullptr) {}

        Connection(Connection&& other) noexcept
        {
            this->m_handle = other.m_handle;
            other.m_handle = nullptr;
        }

        ~Connection()
        {
            close();
        }

        Connection& operator=(Connection&& other) noexcept
        {
            if(&other != this)
            {
                this->m_handle = other.m_handle;
                other.m_handle = nullptr;
            }

            return *this;
        }

        bool backup(sqlite::string_view path) const
        {
            Connection connection;
            if (!connection.open(path))
                return false;
            return backup(connection);
        }

        bool backup(Connection& backup) const
        {
            if (&backup == this)
                CPP_SQLITE_THROW("Connection::backup(Connection& backup) backup connection cannot be the same as the source connection");

            sqlite3_backup* backupHandle = sqlite3_backup_init(backup.get_handle(),
                                                               "main",
                                                               get_handle(),
                                                               "main");
            if(!backupHandle)
                CPP_SQLITE_THROW("SQL error: Failed to initialize backup");

            if(!sqlite::detail::check_error(sqlite3_backup_step(backupHandle, -1)))
                CPP_SQLITE_THROW("SQL error: Could not execute backup");

            if(!sqlite::detail::check_error(get_handle(), sqlite3_backup_finish(backupHandle)))
                CPP_SQLITE_THROW("SQL error: Could not finish backup");

            return true;
        }

        bool statement(sqlite::string_view command) const
        {
            sqlite::Statement statement(get_handle(), command);

            return statement.execute();
        }

        template<typename First, typename ... Args>
        bool statement(sqlite::string_view command, const First& first, const Args&... args)
        {
            sqlite::Statement statement(get_handle(), command);
            statement.bind(first, args...);

            return statement.execute();
        }

        CPP_SQLITE_NODISCARD
        Result query(sqlite::string_view command) const
        {
            sqlite::Query query(get_handle(), command);

            return Result(query.m_handle);
        }

        template<typename First, typename ... Args>
        CPP_SQLITE_NODISCARD
        Result query(sqlite::string_view command, const First& first, const Args&... args) const
        {
            sqlite::Query query(get_handle(), command);
            query.bind(first, args...);

            return Result(query.m_handle);
        }

        CPP_SQLITE_NODISCARD
        sqlite::Statement prepare_statement(sqlite::string_view command) const
        {
            return sqlite::Statement(get_handle(), command);
        }

        CPP_SQLITE_NODISCARD
        sqlite::Query prepare_query(sqlite::string_view command) const
        {
            return sqlite::Query(get_handle(), command);
        }

        bool open(sqlite::string_view path)
        {
            return sqlite::detail::check_error(sqlite3_open(path.data(), &m_handle));
        }

        bool close()
        {
            if (m_handle == nullptr)
                return true;

            const auto result = detail::check_error(sqlite3_close(m_handle));
            m_handle = nullptr;

            return result;
        }

        CPP_SQLITE_NODISCARD
        std::int32_t get_error_code() const
        {
            return sqlite3_errcode(m_handle);
        }

        CPP_SQLITE_NODISCARD
        std::int32_t get_extended_error_code() const
        {
            return sqlite3_extended_errcode(m_handle);
        }

        /// Returns the last error message. 
        CPP_SQLITE_NODISCARD
        std::string get_error_message() const
        {
            std::int32_t code = get_error_code();
            if(code == SQLITE_OK || code == SQLITE_DONE)
            {
                return "";
            }

            const std::int32_t extended_code = get_extended_error_code();
            std::string errstr = sqlite3_errstr(extended_code);
            std::string errmsg = sqlite3_errmsg(m_handle);

            return errstr + ": " + errmsg;
        }

        CPP_SQLITE_NODISCARD
        sqlite3* get_handle() const noexcept
        {
            return m_handle;
        }

    protected:
        sqlite3* m_handle = nullptr;
    };


    template<>
    inline bool sqlite::detail::Result::get(std::int32_t col) const
    {
        return sqlite3_column_int(m_statement, col) != 0;
    }

    template<>
    inline float sqlite::detail::Result::get(std::int32_t col) const
    {
        return static_cast<float>(sqlite3_column_double(m_statement, col));
    }

    template<>
    inline double sqlite::detail::Result::get(std::int32_t col) const
    {
        return sqlite3_column_double(m_statement, col);
    }

    template<>
    inline std::int32_t sqlite::detail::Result::get(std::int32_t col) const
    {
        return sqlite3_column_int(m_statement, col);
    }

    template<>
    inline std::int64_t sqlite::detail::Result::get(std::int32_t col) const
    {
        return sqlite3_column_int64(m_statement, col);
    }

    template<>
    inline std::string sqlite::detail::Result::get(std::int32_t col) const
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
    inline sqlite::string_view sqlite::detail::Result::get(std::int32_t col) const
    {
        const char* bytes = reinterpret_cast<const char*>(sqlite3_column_text(m_statement, col));

        const std::size_t size = static_cast<std::size_t>(
            sqlite3_column_bytes(m_statement, col)
        );

        if(size == 0)
        {
            return {};
        }

        return sqlite::string_view{bytes, size};
    }

    template<>
    inline sqlite::Blob sqlite::detail::Result::get(std::int32_t col) const
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
    inline sqlite::BlobView sqlite::detail::Result::get(std::int32_t col) const
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
