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
#include <vector>
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

#if !defined(__EXCEPTIONS) && !defined(_CPPUNWIND) && !defined(CPP_SQLITE_NO_EXCEPTIONS)
    #define CPP_SQLITE_NO_EXCEPTIONS
#endif

#if defined(CPP_SQLITE_NO_EXCEPTIONS)
    #define CPP_SQLITE_THROW_MSG_OR_ABORT(condition_, exception_, msg_) if (condition_) std::abort() 
    #define CPP_SQLITE_THROW_OR_ABORT(condition_, exception_) if (condition_) std::abort()
    #define CPP_SQLITE_THROW(...) return false
#else
    #define CPP_SQLITE_THROW_MSG_OR_ABORT(condition_, exception_, msg_) if (condition_) throw exception_(msg_ #condition_)
    #define CPP_SQLITE_THROW_OR_ABORT(condition_, exception_) if (condition_) throw exception_()
    #define CPP_SQLITE_THROW(...) throw sqlite::Exception(__VA_ARGS__)
#endif

namespace sqlite
{
    using byte = unsigned char;

    /// @brief Null type that can be used in parameter binding in statements
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
    /// @brief Partial reimplementation of std::string_view
    /// mean to serve as a fallback for C++11 and C++14.
    /// @attention Don't expect this to be at feature parity with the standard one.
    /// @note 
    /// This is only included when C++17 is not available, 
    /// and on C++17 and up the standard string_view is used 
    /// and aliased to sqlite::string_view.
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
        string_view() noexcept = default; 

        CPP_SQLITE_CONSTEXPR
        string_view(const string_view&) noexcept = default;

        CPP_SQLITE_CONSTEXPR
        string_view(string_view&&) noexcept = default;

        CPP_SQLITE_CONSTEXPR17
        string_view(const_pointer cstr) noexcept 
        : m_str(cstr), m_len(traits_type::length(cstr)) {}

        CPP_SQLITE_CONSTEXPR
        string_view(const_pointer str, size_type len) noexcept
        : m_str(str), m_len(len){}

        CPP_SQLITE_CONSTEXPR17
        string_view(const std::string& str) noexcept
        : m_str(str.data()), m_len(str.size()) {}

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const_reference operator[](size_type pos) const noexcept 
        { 
            assert(m_len > pos);
            return *(m_str + pos); 
        }

        CPP_SQLITE_CONSTEXPR14
        string_view& operator=(const string_view&) noexcept = default;

        CPP_SQLITE_CONSTEXPR14
        string_view& operator=(string_view&&) noexcept = default;

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        bool operator==(string_view rhs) const noexcept 
        { return rhs.m_len == m_len && compare(rhs) == 0; }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        bool operator==(const_pointer rhs) const noexcept 
        { 
            string_view rhs_sv{rhs};
            return rhs_sv.m_len == m_len && compare(rhs_sv) == 0; 
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR17
        bool operator==(const std::string& rhs) const noexcept 
        { return rhs.size() == m_len && compare(string_view{rhs}) == 0; }

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

        size_type copy(char* str, size_type n, size_type pos = 0) const
        {
            assert(m_len > pos);
            CPP_SQLITE_THROW_MSG_OR_ABORT(pos > m_len, std::out_of_range, "string_view::copy: ");
            const size_type len = std::min<size_type>(n, m_len - pos);
            traits_type::copy(str, m_str + pos, len);
            return len;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        string_view substr(size_type pos = 0, size_type n = string_view::npos) const
        {
            assert(m_len > pos);
            CPP_SQLITE_THROW_MSG_OR_ABORT(pos > m_len, std::out_of_range, "string_view::substr: ");
            const size_type len = std::min<size_type>(n, m_len - pos);
            return string_view(m_str + pos, len);
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const_reference at(size_type pos) const
        {
            assert(m_len > pos);
            CPP_SQLITE_THROW_MSG_OR_ABORT(pos >= m_len, std::out_of_range, "string_view::at: ");
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
            const size_type len = std::min<size_type>(m_len, str.m_len); 
            int ret = traits_type::compare(m_str, str.m_str, len);
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
            const difference_type diff = n1 - n2;
            if (diff > limits::max()) return limits::max();
            if (diff < limits::min()) return limits::min();

            return static_cast<int>(diff);
        }
    private:
        const_pointer m_str = nullptr;
        size_type m_len = 0;
    };

#if defined(CPP_SQLITE_IOSTREAM) && __cplusplus < 201703L
    inline std::ostream& operator<<(std::ostream& out, string_view sv)
    {
        out.write(sv.data(), sv.size());
        return out;
    }
#endif

    struct nullopt_t{};

    constexpr static const nullopt_t nullopt{};

    class bad_optional_access : public std::exception
    {
    public:
        const char* what() const noexcept override
        {
            return "bad optional access";
        }
    };

    /// @brief Partial reimplementation of std::optional<T> 
    /// meant to serve as a fallback for C++11 and C++14. 
    /// @attention Don't expect this to be at feature parity with the standard one. 
    /// @note 
    /// This is only included when C++17 is not available, 
    /// and on C++17 and up the standard optional<T> is used 
    /// and aliased to sqlite::optional<T>.
    /// @tparam T 
    template<typename T>
    class optional
    {
    public:
        using value_type = T;

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
        optional(const optional<T>& other)
        : m_has_value(other.m_has_value)
        {
            if (m_has_value)
                new (&m_storage) T(other.get_value());
        }

        CPP_SQLITE_CONSTEXPR14
        optional(optional<T>&& other)
        : m_has_value(other.m_has_value)
        {
            if (m_has_value)
            {
                new (&m_storage) T(std::move(other.get_value()));
                other.m_has_value = false;
            }
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

        optional<T>& operator=(optional<T>&& other)
        {
            reset();
            if (other.m_has_value)
            {
                construct(other.get_value());
                other.m_has_value = false;
            }
            return *this;
        }

        optional<T>& operator=(const optional<T>& other)
        {
            reset();
            if (other.m_has_value)
                construct(other.get_value());
            return *this;
        }

        optional<T>& operator=(const T& value)
        {
            reset();
            construct(value);
            return *this;
        }

        optional<T>& operator=(T&& value)
        {
            reset();
            construct(std::move(value));
            return *this;
        }

        optional<T>& operator=(nullopt_t) noexcept
        {
            reset();
            return *this;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        bool operator==(nullopt_t) noexcept
        {
            return !m_has_value;
        }

        CPP_SQLITE_NODISCARD
        bool operator==(const T& value) noexcept
        {
            return m_has_value && get_value() == value;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        explicit operator bool() const noexcept
        {
            return m_has_value;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        T& operator*() noexcept
        {
            assert(m_has_value);
            return get_value();
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const T& operator*() const noexcept
        {
            assert(m_has_value);
            return get_value();
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        T* operator->() noexcept
        {
            assert(m_has_value);
            return get_pointer();
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const T* operator->() const noexcept
        {
            assert(m_has_value);
            return get_pointer();
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        T& value() &
        {
            assert(m_has_value);
            CPP_SQLITE_THROW_OR_ABORT(!m_has_value, bad_optional_access);
            return get_value();
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const T& value() const &
        {
            assert(m_has_value);
            CPP_SQLITE_THROW_OR_ABORT(!m_has_value, bad_optional_access);
            return get_value();
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        T&& value() &&
        {
            assert(m_has_value);
            CPP_SQLITE_THROW_OR_ABORT(!m_has_value, bad_optional_access);
            return std::move(get_value());
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        const T&& value() const &&
        {
            assert(m_has_value);
            CPP_SQLITE_THROW_OR_ABORT(!m_has_value, bad_optional_access);
            return std::move(get_value());
        }

        template<typename Arg>
        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        T value_or(Arg&& fallback) const &
        {
            if (m_has_value)
                return get_value();
            return static_cast<T>(std::forward<Arg>(fallback));
        }

        template<typename Arg>
        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        T value_or(T&& fallback) &&
        {
            if (m_has_value)
                return std::move(get_value());
            return static_cast<T>(std::forward<Arg>(fallback));
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR14
        void reset()
        {
            if (!m_has_value)
                return;
            get_pointer()->~T();
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
        void construct(const T& value)
        {
            new (&m_storage) T(value);
            m_has_value = true;
        }

        void construct(T&& value)
        {
            new (&m_storage) T(std::move(value));
            m_has_value = true;
        }

        CPP_SQLITE_NODISCARD
        T* get_pointer() noexcept
        {
            return reinterpret_cast<T*>(&m_storage);
        }

        CPP_SQLITE_NODISCARD
        const T* get_pointer() const noexcept
        {
            return reinterpret_cast<const T*>(&m_storage);
        }

        CPP_SQLITE_NODISCARD
        T& get_value() noexcept { return *get_pointer(); }

        CPP_SQLITE_NODISCARD
        const T& get_value() const noexcept { return *get_pointer(); }
    private:
        typename std::aligned_storage<sizeof(T), alignof(T)>::type m_storage;
        bool m_has_value = false;
    };
#endif

    struct Error
    {
        std::int32_t code;
        std::int32_t extended_code;
        std::string message;
    };

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
        Blob() noexcept = default;

        Blob(const sqlite::byte* data, std::size_t size)
        {
            if (size == 0 || data == nullptr)
                return;
            m_vector.assign(data, data + size);
        }

        Blob(const Blob&) = default;
        Blob(Blob&&) noexcept = default;
        Blob& operator=(const Blob&) = default;
        Blob& operator=(Blob&&) noexcept = default;

        CPP_SQLITE_NODISCARD
        bool empty() const noexcept
        {
            return m_vector.empty();
        }

        CPP_SQLITE_NODISCARD
        std::size_t size() const noexcept
        {
            return m_vector.size();
        }

        CPP_SQLITE_NODISCARD
        sqlite::byte* data() noexcept
        {
            return m_vector.data();
        }

        CPP_SQLITE_NODISCARD 
        const sqlite::byte* data() const noexcept
        {
            return m_vector.data();
        }

        CPP_SQLITE_NODISCARD
        std::string to_string() const
        {
            return std::string(
                reinterpret_cast<const char*>(m_vector.data()),
                m_vector.size() 
            );
        }

        CPP_SQLITE_NODISCARD
        string_view to_string_view() const noexcept
        {
            return string_view(
                reinterpret_cast<const char*>(m_vector.data()), 
                m_vector.size() 
            );
        }

        CPP_SQLITE_NODISCARD
        struct BlobView to_view() const noexcept;
    private:
        std::vector<sqlite::byte> m_vector;
    };

    struct BlobView
    {
        CPP_SQLITE_CONSTEXPR
        BlobView() noexcept
        : m_data(nullptr), m_size(0) {}

        CPP_SQLITE_CONSTEXPR14
        BlobView(const sqlite::byte* data, std::size_t size) noexcept
        : m_data(data), m_size(size) 
        {
            normalize();
        }

        CPP_SQLITE_CONSTEXPR
        BlobView(const BlobView&) noexcept = default;

        CPP_SQLITE_CONSTEXPR14
        BlobView(BlobView&&) noexcept = default;

        BlobView(const Blob& blob) noexcept
        : m_data(blob.data()), m_size(blob.size()) 
        {
            normalize();
        }

        CPP_SQLITE_CONSTEXPR14
        BlobView& operator=(const BlobView&) noexcept = default;

        CPP_SQLITE_CONSTEXPR14
        BlobView& operator=(BlobView&&) noexcept = default;

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        std::size_t empty() const noexcept
        {
            return m_size == 0;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        std::size_t size() const noexcept
        {
            return m_size;
        }

        CPP_SQLITE_NODISCARD CPP_SQLITE_CONSTEXPR
        const sqlite::byte* data() const noexcept
        {
            return m_data;
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
        string_view to_string_view() const noexcept
        {
            return string_view(
                reinterpret_cast<const char*>(m_data), 
                m_size
            );
        }

        /// Convert to data owning blob. 
        CPP_SQLITE_NODISCARD
        Blob to_blob() const
        {
            return Blob(m_data, m_size);
        }
    private:
        CPP_SQLITE_CONSTEXPR14
        void normalize() noexcept
        {
            if (m_size == 0)
                m_data = nullptr;
            else if (m_data == nullptr)
                m_size = 0;
        }
    private:
        const sqlite::byte* m_data = nullptr;
        std::size_t m_size = 0;
    };

    CPP_SQLITE_NODISCARD
    BlobView Blob::to_view() const noexcept
    {
        return BlobView(m_vector.data(), m_vector.size());
    }

    enum class ColumnType
    {
        Null,
        Integer,
        Real,
        Text,
        Blob,
        Unknown,
    };

    template<typename T>
    struct RowMapper;

    template<typename T>
    struct ParameterMapper;

    namespace detail
    {
        inline bool check_error(sqlite3* db, std::int32_t code)
        {
            if (code == SQLITE_OK || code == SQLITE_DONE || code == SQLITE_ROW)
                return true;
#ifndef CPP_SQLITE_NO_EXCEPTIONS
            const std::int32_t extended_code = sqlite3_extended_errcode(db);
            std::string errstr = sqlite3_errstr(extended_code);
            std::string errmsg = sqlite3_errmsg(db);
            throw Exception(errstr + ": " + errmsg, extended_code);
#else
            (void)db;
            return false;
#endif
        }

        inline bool check_error(std::int32_t code)
        {
            if (code == SQLITE_OK || code == SQLITE_DONE || code == SQLITE_ROW)
                return true;
#ifndef CPP_SQLITE_NO_EXCEPTIONS
            std::string errstr = std::string("SQL error: ") + sqlite3_errstr(code);
            throw Exception(errstr, code);
#else
            return false;
#endif
        }
    
        struct ColumnInfoCache
        {
            void reset(std::int32_t count)
            {
                column_names.clear();
                column_names.reserve(count);
                column_count = count;
            }

            CPP_SQLITE_NODISCARD
            bool is_valid_column_index(std::int32_t column_index) const noexcept
            {
                return column_index >= 0 && column_index < column_count;
            }

            CPP_SQLITE_NODISCARD
            std::int32_t get_column_index(string_view column_name) const noexcept
            {
                for (std::int32_t i = 0; i < column_count; i++)
                {
                    if (string_view(column_names[i]) == column_name)
                        return i;
                }
                return -1;
            }

            CPP_SQLITE_NODISCARD
            string_view get_column_name(std::int32_t column_index) const noexcept
            {
                if (column_index < 0 || column_index >= column_count)
                    return {};
                return column_names[column_index];
            }
            std::int32_t column_count;
            std::vector<std::string> column_names;
        };
    }

    struct Prepared : NonCopyable
    {
        Prepared(Prepared&& other) noexcept
        : m_handle(nullptr)
        {
            std::swap(m_handle, other.m_handle);
            m_named_params = std::move(other.m_named_params);
        }

        ~Prepared()
        {
            finalize();
            m_handle = nullptr;
        }

        Prepared& operator=(Prepared&& other) noexcept
        {
            if (&other == this) return *this;
            finalize();
            m_handle = nullptr;
            std::swap(m_handle, other.m_handle);
            m_named_params = std::move(other.m_named_params);
            return *this;
        }

        bool reset(bool clear = false)
        {
            bool ok = true;
            if (clear)
                ok = clear_bindings();
            return ok && detail::check_error(sqlite3_reset(m_handle));
        }

        bool clear_bindings()
        {
            return detail::check_error(sqlite3_clear_bindings(m_handle));
        }

        CPP_SQLITE_NODISCARD
        std::int32_t get_parameter_index(string_view name) const
        {
            for (const auto& param : m_named_params)
            {
                if (param.name_view == name)
                    return param.index;
            }
            return -1;
        }

        template<typename T>
        bool bind(const T& mapped)
        {
            return clear_bindings() && ParameterMapper<T>::map(*this, mapped);
        }
        
        template<typename First, typename ... Args>
        bool bind_all(const First& first, const Args&... args)
        {
            return clear_bindings() && expand_bind(1, first, args...);
        }

        template<typename T>
        bool bind(string_view param_name, const T& value)
        {
            string_view normalized = normalize_param_name(param_name);
            if (normalized.empty())
                return false;
            const std::int32_t index = get_parameter_index(normalized);
            if (index == -1)
                return false;
            return bind(index, value);
        }

        template<typename T>
        bool bind(std::int32_t index, const optional<T>& data)
        {
            if (data.has_value())
                return bind(index, data.value());
            return detail::check_error(sqlite3_bind_null(m_handle, index));
        }

        bool bind(std::int32_t index, null_t)
        {
            return detail::check_error(sqlite3_bind_null(m_handle, index));
        }
        bool bind(std::int32_t index, bool data)
        {
            return detail::check_error(sqlite3_bind_int(m_handle, index, static_cast<std::int32_t>(data)));
        }
        bool bind(std::int32_t index, std::int32_t data)
        {
            return detail::check_error(sqlite3_bind_int(m_handle, index, data));
        }
        bool bind(std::int32_t index, std::int64_t data)
        {
            return detail::check_error(sqlite3_bind_int64(m_handle, index, data));
        }
        bool bind(std::int32_t index, float data)
        {
            return detail::check_error(sqlite3_bind_double(m_handle, index, static_cast<double>(data)));
        }
        bool bind(std::int32_t index, double data)
        {
            return detail::check_error(sqlite3_bind_double(m_handle, index, data));
        }
        bool bind(std::int32_t index, const char* data)
        {
            return bind(index, string_view{data});
        }
        bool bind(std::int32_t index, const std::string& data)
        {
            return detail::check_error(sqlite3_bind_text(m_handle, index, data.data(), static_cast<std::int32_t>(data.size()), nullptr));
        }
        bool bind(std::int32_t index, string_view data)
        {
            return detail::check_error(sqlite3_bind_text(m_handle, index, data.data(), static_cast<std::int32_t>(data.size()), nullptr));
        }
        bool bind(std::int32_t index, const Blob& blob)
        {
            return detail::check_error(sqlite3_bind_blob(m_handle, index, blob.data(), static_cast<std::int32_t>(blob.size()), nullptr));
        }
        bool bind(std::int32_t index, const BlobView& blob)
        {
            return detail::check_error(sqlite3_bind_blob(m_handle, index, blob.data(), static_cast<std::int32_t>(blob.size()), nullptr));
        }
    private:
        bool finalize()
        {
            return detail::check_error(sqlite3_finalize(m_handle));
        }
        template<typename First, typename ... Args>
        bool expand_bind(std::int32_t index, const First& first, const Args&... args)
        {
            return bind(index, first) && expand_bind(++index, args...);
        }
        bool expand_bind(std::int32_t)
        {
            return true;
        }
        string_view normalize_param_name(string_view name)
        {
            if (name.empty()) return name;
            switch (name.front())
            {
                case ':':
                case '@':
                case '$':
                    return name.substr(1);
                default:
                    return name;
            }
            return name;
        }
    protected:
        Prepared(sqlite3* connection_handle, string_view sql)
        {
            const std::int32_t code = sqlite3_prepare_v2(
                    connection_handle,
                    sql.data(),
                    static_cast<std::int32_t>(sql.size()),
                    &m_handle,
                    nullptr);

            detail::check_error(connection_handle, code);

            const std::int32_t param_count = sqlite3_bind_parameter_count(m_handle);
            m_named_params.reserve(param_count);
            for (std::int32_t i = 1; i <= param_count; ++i)
            {
                const char* param_name = sqlite3_bind_parameter_name(m_handle, i);
                if (param_name != nullptr)
                {
                    ++param_name; // Trim prefix
                    m_named_params.emplace_back(i, param_name);
                }
            }
        }
    protected:
        sqlite3_stmt* m_handle = nullptr;
        struct NamedParameter 
        {
            NamedParameter(std::int32_t index, const char* name) noexcept
            : index(index), name(name) 
            {
                name_view = this->name;
            }

            std::int32_t index;
            std::string name;
            string_view name_view;
        };
        std::vector<NamedParameter> m_named_params;
    };

    struct Statement : Prepared
    {
        friend struct Connection;

        Statement() = delete;

        bool execute()
        {
            return reset(false) && step();
        }
    protected:
        bool step()
        {
            return detail::check_error(sqlite3_step(m_handle));
        }

    private:
        using Prepared::Prepared;
    };

    struct ResultRow
    {
        struct Value : NonCopyable
        {
            Value(const ResultRow* row, std::int32_t column_index) noexcept
            : m_row_ptr(row), m_column_index(column_index) {}

            template<typename T>
            operator T() const
            {
                return m_row_ptr->get<T>(m_column_index);
            }

            template<typename T>
            operator optional<T>() const
            {
                return m_row_ptr->get_nullable<T>(m_column_index);
            }

        private:
            const ResultRow* m_row_ptr;
            std::int32_t m_column_index;
        };

        CPP_SQLITE_NODISCARD
        Value operator[](std::int32_t column_index)
        {
            return get_value(column_index);
        }

        CPP_SQLITE_NODISCARD
        Value operator[](string_view column_name)
        {
            return get_value(column_name);
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        T as()
        {
            return RowMapper<T>::map(*this);
        }
        
        CPP_SQLITE_NODISCARD
        ColumnType get_column_type(std::int32_t column_index) const noexcept
        {
            const std::int32_t type_int = sqlite3_column_type(m_handle, column_index);
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
        ColumnType get_column_type(string_view column_name) const
        {
            const std::int32_t column_index = m_column_info_ptr->get_column_index(column_name);
            return get_column_type(column_index);
        }

        CPP_SQLITE_NODISCARD
        bool is_null(std::int32_t column_index) const
        {
            return get_column_type(column_index) == ColumnType::Null;
        }

        CPP_SQLITE_NODISCARD
        bool is_null(string_view column_name) const
        {
            const std::int32_t column_index = m_column_info_ptr->get_column_index(column_name);
            return is_null(column_index);
        }

        CPP_SQLITE_NODISCARD
        Value get_value(std::int32_t column_index) const
        {
            return Value(this, column_index);
        }

        CPP_SQLITE_NODISCARD
        Value get_value(string_view column_name) const
        {
            const std::int32_t column_index = m_column_info_ptr->get_column_index(column_name);
            return Value(this, column_index);
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        optional<T> get_nullable(std::int32_t column_index) const
        {
            if (is_null(column_index) || !m_column_info_ptr->is_valid_column_index(column_index))
                return nullopt;
            return get<T>(column_index);
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        optional<T> get_nullable(string_view column_name) const
        {
            const std::int32_t column_index = m_column_info_ptr->get_column_index(column_name);
            return get_nullable<T>(column_index);
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        T get(string_view column_name) const
        {
            const std::int32_t column_index = m_column_info_ptr->get_column_index(column_name);
            return get<T>(column_index); 
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        T get(std::int32_t column_index) const
        {
            (void)column_index;
            static_assert(sizeof(T) == -1, "SQL error: invalid column data type");
        }
    private:
        ResultRow(sqlite3_stmt* statement_handle, detail::ColumnInfoCache* column_info_ptr) noexcept
        : m_handle(statement_handle), m_column_info_ptr(column_info_ptr)
        {

        }
    protected:
        friend struct ResultView;
        friend struct Result;
        sqlite3_stmt* m_handle;
        detail::ColumnInfoCache* m_column_info_ptr;
    };

    struct ResultView
    {
        friend struct Query;

        ResultView(const ResultView& other) = default;
        ResultView(ResultView&& other) = default;

        ~ResultView() = default;

        ResultView& operator=(const ResultView& other) = default;
        ResultView& operator=(ResultView& other) = default;

        CPP_SQLITE_NODISCARD
        ResultRow get_row() noexcept
        {
            return ResultRow(m_handle, &m_column_info);
        }

        template<typename T>
        CPP_SQLITE_NODISCARD
        T get_row_as()
        {
            ResultRow row = get_row();
            return row.as<T>();
        }

        CPP_SQLITE_NODISCARD
        bool next()
        {
            if (m_error_code != 0)
                return false;
            const std::int32_t code = sqlite3_step(m_handle);
            if (code == SQLITE_ROW)
                return true;
            if (code == SQLITE_DONE || SQLITE_OK)
                return false;
            if (m_error_code == 0)
                m_error_code = code;
            return false;
        }

        CPP_SQLITE_NODISCARD
        std::int32_t column_count() const
        {
            return m_column_info.column_count;
        }

        CPP_SQLITE_NODISCARD
        bool has_error() const
        {
            return m_error_code != 0;
        }

        CPP_SQLITE_NODISCARD
        optional<Error> get_error() const
        {
            if (m_error_code == 0)
                return nullopt;
            Error error;
            error.code = m_error_code;
            error.message = sqlite3_errstr(error.code);
            return error;
        }

    protected:
        ResultView(){}
        void cache_column_names()
        {
            const std::int32_t column_count = sqlite3_column_count(m_handle);
            m_column_info.reset(column_count);

            for (std::int32_t i = 0; i < column_count; i++)
            {
                m_column_info.column_names.emplace_back(
                    sqlite3_column_name(m_handle, i)
                );
            }
        }
        sqlite3_stmt* m_handle = nullptr;
        detail::ColumnInfoCache m_column_info;
        std::int32_t m_error_code = 0;
    private:
        explicit ResultView(sqlite3_stmt* statement_handle)
        : m_handle(statement_handle)
        {
            cache_column_names();
        }
    };

    struct Result : ResultView 
    {
        friend struct Connection;

        Result() = delete;

        ~Result()
        {
            if (m_handle)
            {
                detail::check_error(sqlite3_finalize(m_handle));
                m_handle = nullptr;
            }
        }

    private:
        explicit Result(sqlite3_stmt*& statement)
        {
            std::swap(m_handle, statement);
            cache_column_names();
        }
    };

    struct Query : Prepared 
    {
        friend struct Connection;

        ResultView get_result()
        {
            return ResultView(m_handle);
        }
    private:
        using Prepared::Prepared;
    };

    struct Connection : NonCopyable
    {
        Connection() = default;

        Connection(Connection&& other) noexcept
        : m_handle(nullptr)
        {
            std::swap(m_handle, other.m_handle);
        }

        ~Connection()
        {
            close();
        }

        Connection& operator=(Connection&& other) noexcept
        {
            if (&other != this)
            {
                close();
                std::swap(m_handle, other.m_handle);
            }
            return *this;
        }

        bool backup(string_view path) const
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

            sqlite3* connection_handle = backup.get_handle();
            sqlite3_backup* backup_handle = sqlite3_backup_init(connection_handle,
                                                               "main",
                                                               m_handle,
                                                               "main");
            if(!backup_handle)
                CPP_SQLITE_THROW("SQL error: Failed to initialize backup");

            if(!detail::check_error(sqlite3_backup_step(backup_handle, -1)))
                CPP_SQLITE_THROW("SQL error: Could not execute backup");

            if(!detail::check_error(connection_handle, sqlite3_backup_finish(backup_handle)))
                CPP_SQLITE_THROW("SQL error: Could not finish backup");

            return true;
        }

        bool statement(string_view sql) const
        {
            Statement statement(get_handle(), sql);
            return statement.execute();
        }

        template<typename First, typename ... Args>
        bool statement(string_view sql, const First& first, const Args&... args)
        {
            Statement statement(get_handle(), sql);
            statement.bind(first, args...);
            return statement.execute();
        }

        CPP_SQLITE_NODISCARD
        Result query(string_view sql) const
        {
            Query query(get_handle(), sql);
            return Result(query.m_handle);
        }

        template<typename First, typename ... Args>
        CPP_SQLITE_NODISCARD
        Result query(string_view sql, const First& first, const Args&... args) const
        {
            Query query(get_handle(), sql);
            query.bind(first, args...);

            return Result(query.m_handle);
        }

        CPP_SQLITE_NODISCARD
        Statement prepare_statement(string_view sql) const
        {
            return Statement(get_handle(), sql);
        }

        CPP_SQLITE_NODISCARD
        Query prepare_query(string_view sql) const
        {
            return Query(get_handle(), sql);
        }

        bool open(string_view path)
        {
            return detail::check_error(sqlite3_open(path.data(), &m_handle));
        }

        bool close()
        {
            if (m_handle == nullptr)
                return true;
            const auto result = detail::check_error(sqlite3_close(m_handle));
            if (result == true)
                m_handle = nullptr;
            return result;
        }

        CPP_SQLITE_NODISCARD
        optional<Error> get_error() const noexcept
        {
            Error error{};
            error.code = sqlite3_errcode(m_handle);
            if(error.code == SQLITE_OK || error.code == SQLITE_DONE || error.code == SQLITE_ROW)
                return nullopt;
            error.extended_code = sqlite3_extended_errcode(m_handle);
            std::string errstr = sqlite3_errstr(error.extended_code);
            std::string errmsg = sqlite3_errmsg(m_handle);
            error.message = errstr + errmsg;
            return error;
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
    CPP_SQLITE_NODISCARD
    inline bool ResultRow::get(std::int32_t col) const
    {
        return sqlite3_column_int(m_handle, col) != 0;
    }

    template<>
    CPP_SQLITE_NODISCARD
    inline float ResultRow::get(std::int32_t col) const
    {
        return static_cast<float>(sqlite3_column_double(m_handle, col));
    }

    template<>
    CPP_SQLITE_NODISCARD
    inline double ResultRow::get(std::int32_t col) const
    {
        return sqlite3_column_double(m_handle, col);
    }

    template<>
    CPP_SQLITE_NODISCARD
    inline std::int32_t ResultRow::get(std::int32_t col) const
    {
        return sqlite3_column_int(m_handle, col);
    }

    template<>
    CPP_SQLITE_NODISCARD
    inline std::int64_t ResultRow::get(std::int32_t col) const
    {
        return sqlite3_column_int64(m_handle, col);
    }

    template<>
    CPP_SQLITE_NODISCARD
    inline std::string ResultRow::get(std::int32_t col) const
    {
        const unsigned char* text = sqlite3_column_text(m_handle, col);
        if (text == nullptr)
            return {};
        const std::size_t size = static_cast<std::size_t>(sqlite3_column_bytes(m_handle, col));
        return {reinterpret_cast<const char*>(text), size};
    }

    template<>
    CPP_SQLITE_NODISCARD
    inline string_view ResultRow::get(std::int32_t col) const
    {
        const unsigned char* text = sqlite3_column_text(m_handle, col);
        if (text == nullptr)
            return {};
        const std::size_t size = static_cast<std::size_t>(sqlite3_column_bytes(m_handle, col));
        return {reinterpret_cast<const char*>(text), size};
    }

    template<>
    CPP_SQLITE_NODISCARD
    inline Blob ResultRow::get(std::int32_t col) const
    {
        const void* bytes = sqlite3_column_blob(m_handle, col);
        const std::int32_t size = sqlite3_column_bytes(m_handle, col);
        if (bytes == nullptr || size <= 0)
            return {};
        return {static_cast<const sqlite::byte*>(bytes), static_cast<std::size_t>(size)};
    }

    template<>
    CPP_SQLITE_NODISCARD
    inline BlobView ResultRow::get(std::int32_t col) const
    {
        const sqlite::byte* bytes = static_cast<const sqlite::byte*>(
            sqlite3_column_blob(m_handle, col)
        );
        const std::size_t size = static_cast<std::size_t>(
            sqlite3_column_bytes(m_handle, col)
        );
        return {bytes, size};
    }
}