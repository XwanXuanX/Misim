/*********************************************************************
 * 
 * @file   utility.hpp
 * 
 * @brief  Collection of useful utilities
 * 
 * @author Yetong Li
 * @date   September 2023
 * 
 *********************************************************************/


#include <concepts>
#include <coroutine>


namespace utility::generator
{
    template <typename V>
    concept valid_generator_value_type = requires
    {
        requires
        (std::movable<V> || std::copyable<V>);
        requires
        std::default_initializable<V>;
    };

    template <valid_generator_value_type V>
    class Generator
    {
    public:
        struct promise_type;

        using value_type  = std::remove_cvref_t<V>;
        using handle_type = std::coroutine_handle<promise_type>;

    public:
        struct promise_type
        {
        public:
            auto get_return_object() noexcept
                -> Generator
            {
                return Generator(handle_type::from_promise(*this));
            }

            auto initial_suspend() noexcept
                -> std::suspend_always
            {
                return {};
            }

            auto final_suspend() noexcept
                -> std::suspend_always
            {
                return {};
            }

            auto return_void() noexcept
                -> void
            {
                return;
            }

            auto unhandled_exception() noexcept
                -> void
            {
                return;
            }

            template <std::convertible_to<value_type> From>
            auto yield_value(From&& from)
                -> std::suspend_always
            {
                m_promise_value = std::forward<From>(from);
                return {};
            }

            auto get_promise_value() const noexcept
                -> value_type
            {
                return m_promise_value;
            }

        private:
            value_type m_promise_value;
        };

    public:
        [[nodiscard]] explicit
        Generator(handle_type handle)
            : m_handle{ handle }
        {
        }

        [[nodiscard]] explicit
        Generator() = default;

        ~Generator()
        {
            if (m_handle)
            {
                m_handle.destroy();
            }
        }

    public:
        auto finished() const noexcept
            -> bool
        {
            return m_handle.done();
        }

        auto value() const noexcept
            -> value_type
        {
            return m_handle.promise().get_promise_value();
        }

        auto resume() noexcept
            -> void
        {
            if (!finished())
            {
                m_handle.resume();
            }
        }

    private:
        struct Sentinal {};

        struct Iterator
        {
        public:
            [[nodiscard]] explicit
            Iterator(handle_type handle)
                : m_iter_handle{ handle }
            {
            }

        public:
            auto operator==(Sentinal) const noexcept
                -> bool
            {
                return m_iter_handle.done();
            }

            auto operator++() noexcept
                -> Iterator&
            {
                m_iter_handle.resume();
                
                return *this;
            }

            auto operator*() const noexcept
                -> const value_type
            {
                return m_iter_handle.promise().get_promise_value();
            }

        private:
            handle_type m_iter_handle{};
        };

    public:
        auto begin() const noexcept
            -> Iterator
        {
            return Iterator(m_handle);
        }

        auto end() const noexcept
            -> Sentinal
        {
            return {};
        }

    private:
        handle_type m_handle{};
    };
}
