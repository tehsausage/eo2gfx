#ifndef TIDY_PTR_HPP
#define TIDY_PTR_HPP

// Observing smart pointer which is set to nullptr after move
// Based on redi::tidy_ptr and N4282's std::observer_ptr proposal

#include <iterator>
#include <type_traits>
#include <utility>

template <class T> struct tidy_ptr
{
	using element_type = T;
	using pointer = std::add_pointer_t<T>;
	using reference = std::add_lvalue_reference_t<T>;

	pointer m_ptr;

	constexpr tidy_ptr() noexcept
		: m_ptr(nullptr)
	{ }

	constexpr tidy_ptr(std::nullptr_t) noexcept
		: m_ptr(nullptr)
	{ }

	constexpr explicit tidy_ptr(pointer other) noexcept
		: m_ptr(other)
	{ }

	template <class U> constexpr tidy_ptr(
		std::enable_if_t<std::is_convertible<T*, U*>::value, const tidy_ptr<U>&> other
	) noexcept
		: m_ptr(dynamic_cast<pointer>(other.m_ptr))
	{
	}
	
	constexpr tidy_ptr(tidy_ptr&& other) noexcept
		: m_ptr(other.m_ptr)
	{
		other.m_ptr = nullptr;
	}
	
	template <class U> constexpr tidy_ptr(
		std::enable_if_t<std::is_convertible<T*, U*>::value, tidy_ptr<U>&&> other
	) noexcept
		: m_ptr(dynamic_cast<pointer>(other.m_ptr))
	{
		other.m_ptr = nullptr;
	}

	tidy_ptr& operator =(tidy_ptr&& other) noexcept
	{
		m_ptr = other.m_ptr;
		other.m_ptr = nullptr;
		return *this;
	}

	template <class U> tidy_ptr& operator =(
		std::enable_if_t<std::is_convertible<T*, U*>::value, tidy_ptr<U>&&> other
	) noexcept
	{
		m_ptr = other.m_ptr;
		other.m_ptr = nullptr;
		return *this;
	}

	constexpr pointer get() const noexcept
	{
		return m_ptr;
	}

	constexpr reference operator *() const noexcept
	{
		return *m_ptr;
	}

	constexpr pointer operator->() const noexcept
	{
		return m_ptr;
	}

	constexpr explicit operator bool() const noexcept
	{
		return m_ptr != nullptr;
	}

	constexpr explicit operator pointer() noexcept
	{
		return m_ptr;
	}

	constexpr pointer release() noexcept
	{
		m_ptr = nullptr;
	}

	constexpr void reset(pointer p = nullptr) noexcept
	{
		m_ptr = p;
	}

	constexpr void swap(tidy_ptr& other) noexcept
	{
		std::swap(m_ptr, other.m_ptr);
	}
};

// Comparisons

// Swaps

// Hash


#endif // TIDY_PTR_HPP
