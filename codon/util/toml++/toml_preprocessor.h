//# This file is a part of toml++ and is subject to the the terms of the MIT license.
//# Copyright (c) Mark Gillard <mark.gillard@outlook.com.au>
//# See https://github.com/marzer/tomlplusplus/blob/master/LICENSE for the full license
// text.
// SPDX-License-Identifier: MIT

// clang-format off

#pragma once
#ifndef __cplusplus
	#error toml++ is a C++ library.
#endif

//#====================================================================================================================
//# COMPILER DETECTION
//#====================================================================================================================

#ifdef __INTELLISENSE__
	#define TOML_INTELLISENSE		1
#else
	#define TOML_INTELLISENSE		0
#endif
#ifdef __clang__
	#define TOML_CLANG				__clang_major__
#else
	#define TOML_CLANG				0
#endif
#ifdef __INTEL_COMPILER
	#define TOML_ICC				__INTEL_COMPILER
	#ifdef __ICL
		#define TOML_ICC_CL			TOML_ICC
	#else
		#define TOML_ICC_CL			0
	#endif
#else
	#define TOML_ICC				0
	#define TOML_ICC_CL				0
#endif
#if defined(_MSC_VER) && !TOML_CLANG && !TOML_ICC
	#define TOML_MSVC				_MSC_VER
#else
	#define TOML_MSVC				0
#endif
#if defined(__GNUC__) && !TOML_CLANG && !TOML_ICC
	#define TOML_GCC				__GNUC__
#else
	#define TOML_GCC				0
#endif

#ifdef __has_include
	#define TOML_HAS_INCLUDE(header)		__has_include(header)
#else
	#define TOML_HAS_INCLUDE(header)		0
#endif

//#====================================================================================================================
//# CLANG
//#====================================================================================================================

#if TOML_CLANG

	#define TOML_PUSH_WARNINGS \
		_Pragma("clang diagnostic push") \
		static_assert(true)

	#define TOML_DISABLE_SWITCH_WARNINGS \
		_Pragma("clang diagnostic ignored \"-Wswitch\"") \
		static_assert(true)

	#define TOML_DISABLE_INIT_WARNINGS \
		_Pragma("clang diagnostic ignored \"-Wmissing-field-initializers\"") \
		static_assert(true)

	#define TOML_DISABLE_ARITHMETIC_WARNINGS \
		_Pragma("clang diagnostic ignored \"-Wfloat-equal\"") \
		_Pragma("clang diagnostic ignored \"-Wdouble-promotion\"") \
		_Pragma("clang diagnostic ignored \"-Wchar-subscripts\"") \
		_Pragma("clang diagnostic ignored \"-Wshift-sign-overflow\"") \
		static_assert(true)

	#define TOML_DISABLE_SHADOW_WARNINGS \
		_Pragma("clang diagnostic ignored \"-Wshadow\"") \
		static_assert(true)

	#define TOML_DISABLE_SPAM_WARNINGS \
		_Pragma("clang diagnostic ignored \"-Wweak-vtables\"")	\
		_Pragma("clang diagnostic ignored \"-Wweak-template-vtables\"") \
		_Pragma("clang diagnostic ignored \"-Wpadded\"") \
		static_assert(true)

	#define TOML_POP_WARNINGS \
		_Pragma("clang diagnostic pop") \
		static_assert(true)

	#define TOML_DISABLE_WARNINGS \
		TOML_PUSH_WARNINGS; \
		_Pragma("clang diagnostic ignored \"-Weverything\"") \
		static_assert(true)

	#define TOML_ENABLE_WARNINGS				TOML_POP_WARNINGS

	#define TOML_ASSUME(cond)					__builtin_assume(cond)
	#define TOML_UNREACHABLE					__builtin_unreachable()
	#define TOML_ATTR(...)						__attribute__((__VA_ARGS__))
	#if defined(_MSC_VER) // msvc compat mode
		#ifdef __has_declspec_attribute
			#if __has_declspec_attribute(novtable)
				#define TOML_ABSTRACT_BASE		__declspec(novtable)
			#endif
			#if __has_declspec_attribute(empty_bases)
				#define TOML_EMPTY_BASES	__declspec(empty_bases)
			#endif
			#ifndef TOML_ALWAYS_INLINE
				#define TOML_ALWAYS_INLINE	__forceinline
			#endif
			#if __has_declspec_attribute(noinline)
				#define TOML_NEVER_INLINE	__declspec(noinline)
			#endif
		#endif
	#endif
	#ifdef __has_attribute
		#if !defined(TOML_ALWAYS_INLINE) && __has_attribute(always_inline)
			#define TOML_ALWAYS_INLINE		__attribute__((__always_inline__)) inline
		#endif
		#if !defined(TOML_NEVER_INLINE) && __has_attribute(noinline)
			#define TOML_NEVER_INLINE		__attribute__((__noinline__))
		#endif
		#if !defined(TOML_TRIVIAL_ABI) && __has_attribute(trivial_abi)
			#define TOML_TRIVIAL_ABI		__attribute__((__trivial_abi__))
		#endif
	#endif
	#define TOML_LIKELY(...)				(__builtin_expect(!!(__VA_ARGS__), 1) )
	#define TOML_UNLIKELY(...)				(__builtin_expect(!!(__VA_ARGS__), 0) )

	#define TOML_SIMPLE_STATIC_ASSERT_MESSAGES	1

#endif // clang

//#====================================================================================================================
//# MSVC
//#====================================================================================================================

#if TOML_MSVC || TOML_ICC_CL

	#define TOML_CPP_VERSION					_MSVC_LANG
	#if TOML_MSVC // !intel-cl

		#define TOML_PUSH_WARNINGS \
			__pragma(warning(push)) \
			static_assert(true)

		#if TOML_HAS_INCLUDE(<CodeAnalysis\Warnings.h>)
			#pragma warning(push, 0)
			#include <CodeAnalysis\Warnings.h>
			#pragma warning(pop)
			#define TOML_DISABLE_CODE_ANALYSIS_WARNINGS \
				__pragma(warning(disable: ALL_CODE_ANALYSIS_WARNINGS)) \
				static_assert(true)
		#else
			#define TOML_DISABLE_CODE_ANALYSIS_WARNINGS
				static_assert(true)
		#endif

		#define TOML_DISABLE_SWITCH_WARNINGS \
			__pragma(warning(disable: 4061)) \
			__pragma(warning(disable: 4062)) \
			__pragma(warning(disable: 4063)) \
			__pragma(warning(disable: 26819)) \
			static_assert(true)

		#define TOML_DISABLE_SPAM_WARNINGS \
			__pragma(warning(disable: 4127)) /* conditional expr is constant */ \
			__pragma(warning(disable: 4324)) /* structure was padded due to alignment specifier */  \
			__pragma(warning(disable: 4348)) \
			__pragma(warning(disable: 4464)) /* relative include path contains '..' */ \
			__pragma(warning(disable: 4505)) /* unreferenced local function removed */  \
			__pragma(warning(disable: 4514)) /* unreferenced inline function has been removed */ \
			__pragma(warning(disable: 4582)) /* constructor is not implicitly called */ \
			__pragma(warning(disable: 4623)) /* default constructor was implicitly defined as deleted		*/ \
			__pragma(warning(disable: 4625)) /* copy constructor was implicitly defined as deleted			*/ \
			__pragma(warning(disable: 4626)) /* assignment operator was implicitly defined as deleted		*/ \
			__pragma(warning(disable: 4710)) /* function not inlined */ \
			__pragma(warning(disable: 4711)) /* function selected for automatic expansion */ \
			__pragma(warning(disable: 4820)) /* N bytes padding added */  \
			__pragma(warning(disable: 4946)) /* reinterpret_cast used between related classes */ \
			__pragma(warning(disable: 5026)) /* move constructor was implicitly defined as deleted	*/ \
			__pragma(warning(disable: 5027)) /* move assignment operator was implicitly defined as deleted	*/ \
			__pragma(warning(disable: 5039)) /* potentially throwing function passed to 'extern "C"' function */ \
			__pragma(warning(disable: 5045)) /* Compiler will insert Spectre mitigation */ \
			__pragma(warning(disable: 26451)) \
			__pragma(warning(disable: 26490)) \
			__pragma(warning(disable: 26495)) \
			__pragma(warning(disable: 26812)) \
			__pragma(warning(disable: 26819)) \
			static_assert(true)

		#define TOML_DISABLE_ARITHMETIC_WARNINGS \
			__pragma(warning(disable: 4365)) /* argument signed/unsigned mismatch */ \
			__pragma(warning(disable: 4738)) /* storing 32-bit float result in memory */ \
			__pragma(warning(disable: 5219)) /* implicit conversion from integral to float */ \
			static_assert(true)

		#define TOML_POP_WARNINGS \
			__pragma(warning(pop)) \
			static_assert(true)

		#define TOML_DISABLE_WARNINGS \
			__pragma(warning(push, 0))			\
			__pragma(warning(disable: 4348))	\
			__pragma(warning(disable: 4668))	\
			__pragma(warning(disable: 5105))	\
			TOML_DISABLE_CODE_ANALYSIS_WARNINGS;\
			TOML_DISABLE_SWITCH_WARNINGS;		\
			TOML_DISABLE_SPAM_WARNINGS;			\
			TOML_DISABLE_ARITHMETIC_WARNINGS;	\
			static_assert(true)

		#define TOML_ENABLE_WARNINGS			TOML_POP_WARNINGS

	#endif
	#ifndef TOML_ALWAYS_INLINE
		#define TOML_ALWAYS_INLINE				__forceinline
	#endif
	#define TOML_NEVER_INLINE					__declspec(noinline)
	#define TOML_ASSUME(cond)					__assume(cond)
	#define TOML_UNREACHABLE					__assume(0)
	#define TOML_ABSTRACT_BASE					__declspec(novtable)
	#define TOML_EMPTY_BASES					__declspec(empty_bases)
	#ifdef _CPPUNWIND
		#define TOML_COMPILER_EXCEPTIONS 1
	#else
		#define TOML_COMPILER_EXCEPTIONS 0
	#endif

#endif // msvc

//#====================================================================================================================
//# ICC
//#====================================================================================================================

#if TOML_ICC
	
	#define TOML_PUSH_WARNINGS \
		__pragma(warning(push)) \
		static_assert(true)

	#define TOML_DISABLE_SPAM_WARNINGS \
		__pragma(warning(disable: 82))	/* storage class is not first */ \
		__pragma(warning(disable: 111))	/* statement unreachable (false-positive) */ \
		__pragma(warning(disable: 869)) /* unreferenced parameter */ \
		__pragma(warning(disable: 1011)) /* missing return (false-positive) */ \
		__pragma(warning(disable: 2261)) /* assume expr side-effects discarded */  \
		static_assert(true)

	#define TOML_POP_WARNINGS \
		__pragma(warning(pop)) \
		static_assert(true)

	#define TOML_DISABLE_WARNINGS \
		__pragma(warning(push, 0)) \
		static_assert(true)

	#define TOML_ENABLE_WARNINGS \
		TOML_POP_WARNINGS

#endif // icc

//#====================================================================================================================
//# GCC
//#====================================================================================================================

#if TOML_GCC

	#define TOML_PUSH_WARNINGS \
		_Pragma("GCC diagnostic push") \
		static_assert(true)

	#define TOML_DISABLE_SWITCH_WARNINGS \
		_Pragma("GCC diagnostic ignored \"-Wswitch\"")						\
		_Pragma("GCC diagnostic ignored \"-Wswitch-enum\"")					\
		_Pragma("GCC diagnostic ignored \"-Wswitch-default\"") \
		static_assert(true)

	#define TOML_DISABLE_INIT_WARNINGS \
		_Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"")	\
		_Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")			\
		_Pragma("GCC diagnostic ignored \"-Wuninitialized\"") \
		static_assert(true)

	#define TOML_DISABLE_ARITHMETIC_WARNINGS \
		_Pragma("GCC diagnostic ignored \"-Wfloat-equal\"")					\
		_Pragma("GCC diagnostic ignored \"-Wsign-conversion\"")				\
		_Pragma("GCC diagnostic ignored \"-Wchar-subscripts\"") \
		static_assert(true)

	#define TOML_DISABLE_SHADOW_WARNINGS \
		_Pragma("GCC diagnostic ignored \"-Wshadow\"") \
		static_assert(true)

	#define TOML_DISABLE_SPAM_WARNINGS \
		_Pragma("GCC diagnostic ignored \"-Wpadded\"")						\
		_Pragma("GCC diagnostic ignored \"-Wcast-align\"")					\
		_Pragma("GCC diagnostic ignored \"-Wcomment\"")						\
		_Pragma("GCC diagnostic ignored \"-Wtype-limits\"")					\
		_Pragma("GCC diagnostic ignored \"-Wuseless-cast\"")				\
		_Pragma("GCC diagnostic ignored \"-Wsuggest-attribute=const\"")		\
		_Pragma("GCC diagnostic ignored \"-Wsuggest-attribute=pure\"") \
		static_assert(true)

	#define TOML_POP_WARNINGS \
		_Pragma("GCC diagnostic pop") \
		static_assert(true)

	#define TOML_DISABLE_WARNINGS \
		TOML_PUSH_WARNINGS;													\
		_Pragma("GCC diagnostic ignored \"-Wall\"")							\
		_Pragma("GCC diagnostic ignored \"-Wextra\"")						\
		_Pragma("GCC diagnostic ignored \"-Wpedantic\"")					\
		TOML_DISABLE_SWITCH_WARNINGS;										\
		TOML_DISABLE_INIT_WARNINGS;											\
		TOML_DISABLE_ARITHMETIC_WARNINGS;									\
		TOML_DISABLE_SHADOW_WARNINGS;										\
		TOML_DISABLE_SPAM_WARNINGS;											\
		static_assert(true)


	#define TOML_ENABLE_WARNINGS \
		TOML_POP_WARNINGS

	#define TOML_ATTR(...)						__attribute__((__VA_ARGS__))
	#ifndef TOML_ALWAYS_INLINE
		#define TOML_ALWAYS_INLINE				__attribute__((__always_inline__)) inline
	#endif
	#define TOML_NEVER_INLINE					__attribute__((__noinline__))
	#define TOML_UNREACHABLE					__builtin_unreachable()
	#define TOML_LIKELY(...)					(__builtin_expect(!!(__VA_ARGS__), 1) )
	#define TOML_UNLIKELY(...)					(__builtin_expect(!!(__VA_ARGS__), 0) )

#endif

//#====================================================================================================================
//# USER CONFIGURATION
//#====================================================================================================================

#ifdef TOML_CONFIG_HEADER
	#include TOML_CONFIG_HEADER
#endif
 
#ifdef DOXYGEN
	#define TOML_HEADER_ONLY 0
	#define TOML_WINDOWS_COMPAT 1
#endif

#if defined(TOML_ALL_INLINE) && !defined(TOML_HEADER_ONLY)
	#define TOML_HEADER_ONLY	TOML_ALL_INLINE
#endif

#if !defined(TOML_HEADER_ONLY) || (defined(TOML_HEADER_ONLY) && TOML_HEADER_ONLY) || TOML_INTELLISENSE
	#undef TOML_HEADER_ONLY
	#define TOML_HEADER_ONLY 1
#endif

#if defined(TOML_IMPLEMENTATION) || TOML_HEADER_ONLY
	#undef TOML_IMPLEMENTATION
	#define TOML_IMPLEMENTATION 1
#else
	#define TOML_IMPLEMENTATION 0
#endif

#ifndef TOML_API
	#define TOML_API
#endif

#ifndef TOML_UNRELEASED_FEATURES
	#define TOML_UNRELEASED_FEATURES 0
#endif

#ifndef TOML_LARGE_FILES
	#define TOML_LARGE_FILES 0
#endif

#ifndef TOML_UNDEF_MACROS
	#define TOML_UNDEF_MACROS 1
#endif

#ifndef TOML_PARSER
	#define TOML_PARSER 1
#endif

#ifndef TOML_MAX_NESTED_VALUES
	#define TOML_MAX_NESTED_VALUES 256
	// this refers to the depth of nested values, e.g. inline tables and arrays.
	// 256 is crazy high! if you're hitting this limit with real input, TOML is probably the wrong tool for the job...
#endif

#ifndef DOXYGEN
	#ifdef _WIN32
		#ifndef TOML_WINDOWS_COMPAT
			#define TOML_WINDOWS_COMPAT 1
		#endif
		#if TOML_WINDOWS_COMPAT && !defined(TOML_INCLUDE_WINDOWS_H)
			#define TOML_INCLUDE_WINDOWS_H 0
		#endif
	#endif
	#if !defined(_WIN32) || !defined(TOML_WINDOWS_COMPAT)
		#undef TOML_WINDOWS_COMPAT
		#define TOML_WINDOWS_COMPAT		0
	#endif
	#if !TOML_WINDOWS_COMPAT
		#undef TOML_INCLUDE_WINDOWS_H
		#define TOML_INCLUDE_WINDOWS_H	0
	#endif
#endif

#ifdef TOML_OPTIONAL_TYPE
	#define TOML_HAS_CUSTOM_OPTIONAL_TYPE 1
#else
	#define TOML_HAS_CUSTOM_OPTIONAL_TYPE 0
#endif

#ifdef TOML_CHAR_8_STRINGS
	#if TOML_CHAR_8_STRINGS
		#error TOML_CHAR_8_STRINGS was removed in toml++ 2.0.0; \
all value setters and getters can now work with char8_t strings implicitly so changing the underlying string type \
is no longer necessary.
	#endif
#endif

//#====================================================================================================================
//# ATTRIBUTES, UTILITY MACROS ETC
//#====================================================================================================================

#ifndef TOML_CPP_VERSION
	#define TOML_CPP_VERSION __cplusplus
#endif
#if TOML_CPP_VERSION < 201103L
	#error toml++ requires C++17 or higher. For a TOML library supporting pre-C++11 see https://github.com/ToruNiina/Boost.toml
#elif TOML_CPP_VERSION < 201703L
	#error toml++ requires C++17 or higher. For a TOML library supporting C++11 see https://github.com/ToruNiina/toml11
#elif TOML_CPP_VERSION >= 202600L
	#define TOML_CPP 26
#elif TOML_CPP_VERSION >= 202300L
	#define TOML_CPP 23
#elif TOML_CPP_VERSION >= 202002L
	#define TOML_CPP 20
#elif TOML_CPP_VERSION >= 201703L
	#define TOML_CPP 17
#endif
#undef TOML_CPP_VERSION

#ifndef TOML_COMPILER_EXCEPTIONS
	#if defined(__EXCEPTIONS) || defined(__cpp_exceptions)
		#define TOML_COMPILER_EXCEPTIONS 1
	#else
		#define TOML_COMPILER_EXCEPTIONS 0
	#endif
#endif
#if TOML_COMPILER_EXCEPTIONS
	#if !defined(TOML_EXCEPTIONS) || (defined(TOML_EXCEPTIONS) && TOML_EXCEPTIONS)
		#undef TOML_EXCEPTIONS
		#define TOML_EXCEPTIONS 1
	#endif
#else
	#if defined(TOML_EXCEPTIONS) && TOML_EXCEPTIONS
		#error TOML_EXCEPTIONS was explicitly enabled but exceptions are disabled/unsupported by the compiler.
	#endif
	#undef TOML_EXCEPTIONS
	#define TOML_EXCEPTIONS	0
#endif

#if defined(DOXYGEN) || TOML_EXCEPTIONS
	#define TOML_MAY_THROW
#else
	#define TOML_MAY_THROW				noexcept
#endif

#if TOML_GCC || TOML_CLANG || (TOML_ICC && !TOML_ICC_CL)
	// not supported by any version of GCC or Clang as of 26/11/2020
	// not supported by any version of ICC on Linux as of 11/01/2021
	#define TOML_FLOAT_CHARCONV 0
#endif
#if defined(__EMSCRIPTEN__) || defined(__APPLE__)
	// causes link errors on emscripten
	// causes Mac OS SDK version errors on some versions of Apple Clang
	#define TOML_INT_CHARCONV 0
#endif
#ifndef TOML_INT_CHARCONV
	#define TOML_INT_CHARCONV 1
#endif
#ifndef TOML_FLOAT_CHARCONV
	#define TOML_FLOAT_CHARCONV 1
#endif
#if (TOML_INT_CHARCONV || TOML_FLOAT_CHARCONV) && !TOML_HAS_INCLUDE(<charconv>)
	#undef TOML_INT_CHARCONV
	#undef TOML_FLOAT_CHARCONV
	#define TOML_INT_CHARCONV 0
	#define TOML_FLOAT_CHARCONV 0
#endif

#ifndef TOML_PUSH_WARNINGS
	#define TOML_PUSH_WARNINGS static_assert(true)
#endif
#ifndef TOML_DISABLE_CODE_ANALYSIS_WARNINGS
	#define	TOML_DISABLE_CODE_ANALYSIS_WARNINGS static_assert(true)
#endif
#ifndef TOML_DISABLE_SWITCH_WARNINGS
	#define	TOML_DISABLE_SWITCH_WARNINGS static_assert(true)
#endif
#ifndef TOML_DISABLE_INIT_WARNINGS
	#define	TOML_DISABLE_INIT_WARNINGS static_assert(true)
#endif
#ifndef TOML_DISABLE_SPAM_WARNINGS
	#define TOML_DISABLE_SPAM_WARNINGS static_assert(true)
#endif
#ifndef TOML_DISABLE_ARITHMETIC_WARNINGS
	#define TOML_DISABLE_ARITHMETIC_WARNINGS static_assert(true)
#endif
#ifndef TOML_DISABLE_SHADOW_WARNINGS
	#define TOML_DISABLE_SHADOW_WARNINGS static_assert(true)
#endif
#ifndef TOML_POP_WARNINGS
	#define TOML_POP_WARNINGS static_assert(true)
#endif
#ifndef TOML_DISABLE_WARNINGS
	#define TOML_DISABLE_WARNINGS static_assert(true)
#endif
#ifndef TOML_ENABLE_WARNINGS
	#define TOML_ENABLE_WARNINGS static_assert(true)
#endif

#ifndef TOML_ATTR
	#define TOML_ATTR(...)
#endif

#ifndef TOML_ABSTRACT_BASE
	#define TOML_ABSTRACT_BASE
#endif

#ifndef TOML_EMPTY_BASES
	#define TOML_EMPTY_BASES
#endif

#ifndef TOML_ALWAYS_INLINE
	#define TOML_ALWAYS_INLINE	inline
#endif
#ifndef TOML_NEVER_INLINE
	#define TOML_NEVER_INLINE
#endif

#ifndef TOML_ASSUME
	#define TOML_ASSUME(cond)	(void)0
#endif

#ifndef TOML_UNREACHABLE
	#define TOML_UNREACHABLE	TOML_ASSERT(false)
#endif

#define TOML_NO_DEFAULT_CASE	default: TOML_UNREACHABLE

#if defined(__cpp_consteval) && __cpp_consteval >= 201811 && !defined(_MSC_VER)
	// https://developercommunity.visualstudio.com/t/Erroneous-C7595-error-with-consteval-in/1404234
	#define TOML_CONSTEVAL		consteval
#else
	#define TOML_CONSTEVAL		constexpr
#endif

#ifdef __has_cpp_attribute
	#define TOML_HAS_ATTR(...)	__has_cpp_attribute(__VA_ARGS__)
#else
	#define TOML_HAS_ATTR(...)	0
#endif

#if !defined(DOXYGEN) && !TOML_INTELLISENSE
	#if !defined(TOML_LIKELY) && TOML_HAS_ATTR(likely) >= 201803
		#define TOML_LIKELY(...)	(__VA_ARGS__) [[likely]]
	#endif
	#if !defined(TOML_UNLIKELY) && TOML_HAS_ATTR(unlikely) >= 201803
		#define TOML_UNLIKELY(...)	(__VA_ARGS__) [[unlikely]]
	#endif
	#if TOML_HAS_ATTR(nodiscard) >= 201907
		#define TOML_NODISCARD_CTOR [[nodiscard]]
	#endif
#endif

#ifndef TOML_LIKELY
	#define TOML_LIKELY(...)	(__VA_ARGS__)
#endif
#ifndef TOML_UNLIKELY
	#define TOML_UNLIKELY(...)	(__VA_ARGS__)
#endif
#ifndef TOML_NODISCARD_CTOR
	#define TOML_NODISCARD_CTOR
#endif

#ifndef TOML_TRIVIAL_ABI
	#define TOML_TRIVIAL_ABI
#endif

#define TOML_ASYMMETRICAL_EQUALITY_OPS(LHS, RHS, ...)														\
	__VA_ARGS__ [[nodiscard]] friend bool operator == (RHS rhs, LHS lhs) noexcept { return lhs == rhs; }	\
	__VA_ARGS__ [[nodiscard]] friend bool operator != (LHS lhs, RHS rhs) noexcept { return !(lhs == rhs); }	\
	__VA_ARGS__ [[nodiscard]] friend bool operator != (RHS rhs, LHS lhs) noexcept { return !(lhs == rhs); } \
	static_assert(true)

#ifndef TOML_SIMPLE_STATIC_ASSERT_MESSAGES
	#define TOML_SIMPLE_STATIC_ASSERT_MESSAGES	0
#endif

#define TOML_CONCAT_1(x, y) x##y
#define TOML_CONCAT(x, y) TOML_CONCAT_1(x, y)

#define TOML_EVAL_BOOL_1(T, F)	T
#define TOML_EVAL_BOOL_0(T, F)	F

#if defined(__aarch64__) || defined(__ARM_ARCH_ISA_A64) || defined(_M_ARM64) || defined(__ARM_64BIT_STATE)	\
		|| defined(__arm__) || defined(_M_ARM) || defined(__ARM_32BIT_STATE)
	#define TOML_ARM 1
#else
	#define TOML_ARM 0
#endif

#define TOML_MAKE_FLAGS_(name, op)																	\
	[[nodiscard]]																					\
	TOML_ALWAYS_INLINE																				\
	TOML_ATTR(const)																				\
	constexpr name operator op(name lhs, name rhs) noexcept											\
	{																								\
		using under = std::underlying_type_t<name>;													\
		return static_cast<name>(static_cast<under>(lhs) op static_cast<under>(rhs));				\
	}																								\
	constexpr name& operator TOML_CONCAT(op, =)(name & lhs, name rhs) noexcept						\
	{																								\
		return lhs = (lhs op rhs);																	\
	}																								\
	static_assert(true, "")

#define TOML_MAKE_FLAGS(name)																		\
	TOML_MAKE_FLAGS_(name, &);																		\
	TOML_MAKE_FLAGS_(name, |);																		\
	TOML_MAKE_FLAGS_(name, ^);																		\
	[[nodiscard]]																					\
	TOML_ALWAYS_INLINE																				\
	TOML_ATTR(const)																				\
	constexpr name operator~(name val) noexcept														\
	{																								\
		using under = std::underlying_type_t<name>;													\
		return static_cast<name>(~static_cast<under>(val));											\
	}																								\
	[[nodiscard]]																					\
	TOML_ALWAYS_INLINE																				\
	TOML_ATTR(const)																				\
	constexpr bool operator!(name val) noexcept														\
	{																								\
		using under = std::underlying_type_t<name>;													\
		return !static_cast<under>(val);															\
	}																								\
	static_assert(true, "")


#ifndef TOML_LIFETIME_HOOKS
	#define TOML_LIFETIME_HOOKS 0
#endif

//#====================================================================================================================
//# EXTENDED INT AND FLOAT TYPES
//#====================================================================================================================

#ifdef __FLT16_MANT_DIG__
	#if __FLT_RADIX__ == 2					\
			&& __FLT16_MANT_DIG__ == 11		\
			&& __FLT16_DIG__ == 3			\
			&& __FLT16_MIN_EXP__ == -13		\
			&& __FLT16_MIN_10_EXP__ == -4	\
			&& __FLT16_MAX_EXP__ == 16		\
			&& __FLT16_MAX_10_EXP__ == 4
		#if TOML_ARM && (TOML_GCC || TOML_CLANG)
			#define TOML_FP16 __fp16
		#endif
		#if TOML_ARM && TOML_CLANG // not present in g++
			#define TOML_FLOAT16 _Float16
		#endif
	#endif
#endif

#if defined(__SIZEOF_FLOAT128__)		\
	&& defined(__FLT128_MANT_DIG__)		\
	&& defined(__LDBL_MANT_DIG__)		\
	&& __FLT128_MANT_DIG__ > __LDBL_MANT_DIG__
	#define TOML_FLOAT128	__float128
#endif

#ifdef __SIZEOF_INT128__
	#define TOML_INT128		__int128_t
	#define TOML_UINT128	__uint128_t
#endif

//#====================================================================================================================
//# VERSIONS AND NAMESPACES
//#====================================================================================================================

#include "toml_version.h"

#define TOML_LIB_SINGLE_HEADER 0

#define TOML_MAKE_VERSION(maj, min, rev)											\
		((maj) * 1000 + (min) * 25 + (rev))

#if TOML_UNRELEASED_FEATURES
	#define TOML_LANG_EFFECTIVE_VERSION												\
		TOML_MAKE_VERSION(TOML_LANG_MAJOR, TOML_LANG_MINOR, TOML_LANG_PATCH+1)
#else
	#define TOML_LANG_EFFECTIVE_VERSION												\
		TOML_MAKE_VERSION(TOML_LANG_MAJOR, TOML_LANG_MINOR, TOML_LANG_PATCH)
#endif

#define TOML_LANG_HIGHER_THAN(maj, min, rev)										\
		(TOML_LANG_EFFECTIVE_VERSION > TOML_MAKE_VERSION(maj, min, rev))

#define TOML_LANG_AT_LEAST(maj, min, rev)											\
		(TOML_LANG_EFFECTIVE_VERSION >= TOML_MAKE_VERSION(maj, min, rev))

#define TOML_LANG_UNRELEASED														\
		TOML_LANG_HIGHER_THAN(TOML_LANG_MAJOR, TOML_LANG_MINOR, TOML_LANG_PATCH)

#ifndef TOML_ABI_NAMESPACES
	#ifdef DOXYGEN
		#define TOML_ABI_NAMESPACES 0
	#else
		#define TOML_ABI_NAMESPACES 1
	#endif
#endif
#if TOML_ABI_NAMESPACES
	#define TOML_NAMESPACE_START				namespace toml { inline namespace TOML_CONCAT(v, TOML_LIB_MAJOR)
	#define TOML_NAMESPACE_END					} static_assert(true)
	#define TOML_NAMESPACE						::toml::TOML_CONCAT(v, TOML_LIB_MAJOR)
	#define TOML_ABI_NAMESPACE_START(name)		inline namespace name { static_assert(true)
	#define TOML_ABI_NAMESPACE_BOOL(cond, T, F)	TOML_ABI_NAMESPACE_START(TOML_CONCAT(TOML_EVAL_BOOL_, cond)(T, F))
	#define TOML_ABI_NAMESPACE_END				} static_assert(true)
#else
	#define TOML_NAMESPACE_START				namespace toml
	#define TOML_NAMESPACE_END					static_assert(true)
	#define TOML_NAMESPACE						toml
	#define TOML_ABI_NAMESPACE_START(...)		static_assert(true)
	#define TOML_ABI_NAMESPACE_BOOL(...)		static_assert(true)
	#define TOML_ABI_NAMESPACE_END				static_assert(true)
#endif
#define TOML_IMPL_NAMESPACE_START				TOML_NAMESPACE_START { namespace impl
#define TOML_IMPL_NAMESPACE_END					} TOML_NAMESPACE_END
#if TOML_HEADER_ONLY
	#define TOML_ANON_NAMESPACE_START			TOML_IMPL_NAMESPACE_START
	#define TOML_ANON_NAMESPACE_END				TOML_IMPL_NAMESPACE_END
	#define TOML_ANON_NAMESPACE					TOML_NAMESPACE::impl
	#define TOML_USING_ANON_NAMESPACE			using namespace TOML_ANON_NAMESPACE
	#define TOML_EXTERNAL_LINKAGE				inline
	#define TOML_INTERNAL_LINKAGE				inline
#else
	#define TOML_ANON_NAMESPACE_START			namespace
	#define TOML_ANON_NAMESPACE_END				static_assert(true)
	#define TOML_ANON_NAMESPACE
	#define TOML_USING_ANON_NAMESPACE			static_cast<void>(0)
	#define TOML_EXTERNAL_LINKAGE
	#define TOML_INTERNAL_LINKAGE				static
#endif

//#====================================================================================================================
//# ASSERT
//#====================================================================================================================

TOML_DISABLE_WARNINGS;
#ifndef TOML_ASSERT
	#if defined(NDEBUG) || !defined(_DEBUG)
		#define TOML_ASSERT(expr)	static_cast<void>(0)
	#else
		#ifndef assert
			#include <cassert>
		#endif
		#define TOML_ASSERT(expr)	assert(expr)
	#endif
#endif
TOML_ENABLE_WARNINGS;

//#====================================================================================================================
//# DOXYGEN SPAM
//#====================================================================================================================

//# {{
#ifdef DOXYGEN

/// \addtogroup		configuration		Library Configuration
/// \brief Preprocessor macros for configuring library functionality.
/// \detail Define these before including toml++ to alter the way it functions.
/// \remarks Some of these options have ABI implications; inline namespaces are used to prevent
/// 		 you from trying to link incompatible combinations together.
/// @{


/// \def TOML_HEADER_ONLY
/// \brief Sets whether the library is entirely inline.
/// \detail Defaults to `1`.
/// \remark Disabling this means that you must define #TOML_IMPLEMENTATION in
///	<strong><em>exactly one</em></strong> translation unit in your project:
/// \cpp
/// // global_header_that_includes_toml++.h
/// #define TOML_HEADER_ONLY 0
/// #include <toml.hpp>
/// 
/// // some_code_file.cpp
/// #define TOML_IMPLEMENTATION 
/// #include "global_header_that_includes_toml++.h"
/// \ecpp


/// \def TOML_API
/// \brief An annotation to add to public symbols.
/// \detail Not defined by default.
///	\remark You'd override this with `__declspec(dllexport)` if you were building the library
/// 		into the public API of a DLL on Windows.


/// \def TOML_ASSERT(expr)
/// \brief Sets the assert function used by the library.
/// \detail Defaults to the standard C `assert()`.


#define TOML_CONFIG_HEADER
/// \def TOML_CONFIG_HEADER
/// \brief An additional header to include before any other toml++ header files.
/// \detail Not defined by default.


/// \def TOML_EXCEPTIONS
/// \brief Sets whether the library uses exceptions to report parsing failures.
/// \detail Defaults to `1` or `0` according to your compiler's exception mode.


/// \def TOML_IMPLEMENTATION
/// \brief Enables the library's implementation when #TOML_HEADER_ONLY is disabled.
/// \detail Not defined by default. Meaningless when #TOML_HEADER_ONLY is enabled.


/// \def TOML_LARGE_FILES
/// \brief Sets whether line and column indices are 32-bit integers.
/// \detail Defaults to `0`.
/// \see toml::source_index


#define TOML_OPTIONAL_TYPE
/// \def TOML_OPTIONAL_TYPE
/// \brief Overrides the `optional<T>` type used by the library.
/// \detail Not defined by default (use std::optional).
/// \warning The library uses optionals internally in a few places; if you choose to replace the optional type
/// 		 it must be with something that is still API-compatible with std::optional
/// 		 (e.g. [tl::optional](https://github.com/TartanLlama/optional)).


/// \def TOML_PARSER
/// \brief Sets whether the parser-related parts of the library are included.
/// \detail Defaults to `1`.
/// \remarks If you don't need to parse TOML data from any strings or files (e.g. you're only using the library to
/// 		 serialize data as TOML), setting `TOML_PARSER` to `0` can yield decent compilation speed improvements.


#define TOML_SMALL_FLOAT_TYPE
/// \def TOML_SMALL_FLOAT_TYPE
/// \brief If your codebase has an additional 'small' float type (e.g. half-precision), this tells toml++ about it.
/// \detail Not defined by default.
/// \remark	If you're building for a platform that has a built-in half precision float (e.g. `_Float16`), you don't
/// 		need to use this configuration option to make toml++ aware of it; the library comes with that built-in.

#define TOML_SMALL_INT_TYPE
/// \def TOML_SMALL_INT_TYPE
/// \brief If your codebase has an additional 'small' integer type (e.g. 24-bits), this tells toml++ about it.
/// \detail Not defined by default.


/// \def TOML_UNRELEASED_FEATURES
/// \brief Enables support for unreleased TOML language features not yet part of a
///		[numbered version](https://github.com/toml-lang/toml/releases).
/// \detail Defaults to `0`.
/// \see [TOML Language Support](https://github.com/marzer/tomlplusplus/blob/master/README.md#toml-language-support)


/// \def TOML_WINDOWS_COMPAT
/// \brief Enables the use of wide strings (wchar_t, std::wstring) in various places throughout the library
/// 	   when building for Windows.
/// \detail Defaults to `1` when building for Windows, `0` otherwise. Has no effect when building for anything other
/// 		than Windows.
/// \remark	This <strong>does not</strong> change the underlying string type used to represent TOML keys and string
/// 		values; that will still be std::string. This setting simply enables some narrow &lt;=&gt; wide string
/// 		conversions when necessary at various interface boundaries.
/// 		<br><br>
/// 		If you're building for Windows and you have no need for Windows' "Pretends-to-be-unicode" wide strings,
/// 		you can safely set this to `0`.


/// @}
#endif // DOXYGEN
//# }}

// clang-format on
