/////////////////////////////////////////////////////////////////////////////// 
// 
// Copyright (c) 2015 Microsoft Corporation. All rights reserved. 
// 
// This code is licensed under the MIT License (MIT). 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
// THE SOFTWARE. 
// 
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef GSL_GSL_H
#define GSL_GSL_H

#include "gsl_assert.h"  // Ensures/Expects
#include <memory>

#ifdef _MSC_VER

// No MSVC does constexpr fully yet
#pragma push_macro("constexpr")
#define constexpr 

// MSVC 2013 workarounds
#if _MSC_VER <= 1800
// noexcept is not understood 
#pragma push_macro("noexcept")
#define noexcept  

// turn off some misguided warnings
#pragma warning(push)
#pragma warning(disable: 4351) // warns about newly introduced aggregate initializer behavior

#endif // _MSC_VER <= 1800

#endif // _MSC_VER

namespace std {
    template<class T>
    using add_lvalue_reference_t = add_lvalue_reference<T>;
    template<class T>
    using add_const_t = add_const<T>;
    template< bool B, class T = void >
    using enable_if_t = typename enable_if<B,T>::type;
    template< bool B, class T, class F >
    using conditional_t = typename conditional<B,T,F>::type;
    template< class T >
    using remove_const_t    = typename remove_const<T>::type;
    template< class T >
    using remove_cv_t       = typename remove_cv<T>::type;
    template< class T >
    using remove_reference_t = typename remove_reference<T>::type;
    template< class T >
    using add_pointer_t = typename add_pointer<T>::type;
    template< class T >
    using decay_t = typename decay<T>::type;
    template< class T >
    using remove_all_extents_t = typename remove_all_extents<T>::type;
}


#include <type_traits>
#include <vector>
namespace gsl
{

//
// GSL.owner: ownership pointers 
//
using std::unique_ptr;
using std::shared_ptr;

template <class T>
using owner = T;


//
// not_null
//
// Restricts a pointer or smart pointer to only hold non-null values.
// 
// Has zero size overhead over T.
//
// If T is a pointer (i.e. T == U*) then 
// - allow construction from U* or U& 
// - disallow construction from nullptr_t
// - disallow default construction
// - ensure construction from U* fails with nullptr
// - allow implicit conversion to U*
//
template<class T>
class not_null
{
    static_assert(std::is_assignable<T&, std::nullptr_t>::value, "T cannot be assigned nullptr.");
    
    template<typename Arg, typename... Args>
    struct check_args : std::conditional<std::is_constructible<not_null<Arg>, Arg>::value, check_args<Arg, Args...>, std::false_type>{};
    
    template<typename Arg>
    struct check_args<Arg> : std::is_constructible<not_null<Arg>, Arg>{};
    
public:
    template<class... U>
    not_null(U... args):ptr_(std::forward<U>(args)...){
        check_args<U...> arg_check;
    }
    not_null(T t) : ptr_(t) { ensure_invariant(); }
    not_null& operator=(const T& t) { ptr_ = t; ensure_invariant(); return *this; }

    not_null(const not_null &other) = default;
    not_null& operator=(const not_null &other) = default;

    template <typename U, typename Dummy = std::enable_if_t<std::is_convertible<U, T>::value>>
    not_null(const not_null<U> &other)
    {
        *this = other;
    }

    template <typename U, typename Dummy = std::enable_if_t<std::is_convertible<U, T>::value>>
    not_null& operator=(const not_null<U> &other)
    {
        ptr_ = other.get();
        return *this;
    }

    // prevents compilation when someone attempts to assign a nullptr 
    not_null(std::nullptr_t) = delete;
    not_null(int) = delete;
    not_null<T>& operator=(std::nullptr_t) = delete;
	not_null<T>& operator=(int) = delete;
    
    T get() const {
#ifdef _MSC_VER
        __assume(ptr_ != nullptr);
#endif
        return ptr_;
    } // the assume() should help the optimizer

    operator T() const {  return get(); }
    T operator->() const { return get(); }

	bool operator==(const T& rhs) const { return ptr_ == rhs; }
	bool operator!=(const T& rhs) const { return !(*this == rhs); }
private:
    T ptr_;

    // we assume that the compiler can hoist/prove away most of the checks inlined from this function
    // if not, we could make them optional via conditional compilation
    void ensure_invariant() const { Expects(ptr_ != nullptr); }

    // unwanted operators...pointers only point to single objects!
    // TODO ensure all arithmetic ops on this type are unavailable
    not_null<T>& operator++() = delete;
    not_null<T>& operator--() = delete;
    not_null<T> operator++(int) = delete;
    not_null<T> operator--(int) = delete;
    not_null<T>& operator+(size_t) = delete;
    not_null<T>& operator+=(size_t) = delete;
    not_null<T>& operator-(size_t) = delete;
    not_null<T>& operator-=(size_t) = delete;
};
    
} // namespace gsl

namespace std
{
    template<class T>
    struct hash<gsl::not_null<T>>
    {
        size_t operator()(const gsl::not_null<T> & value) const
        {
            return hash<T>{}(value);
        }
    };

} // namespace std

#ifdef _MSC_VER

#undef constexpr
#pragma pop_macro("constexpr")

#if _MSC_VER <= 1800

#undef noexcept
#pragma pop_macro("noexcept")
 
#pragma warning(pop)

#endif // _MSC_VER <= 1800

#endif // _MSC_VER

#endif // GSL_GSL_H
