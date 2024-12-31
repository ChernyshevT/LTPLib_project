#ifndef _IO_STRINGS_HEADER
#define _IO_STRINGS_HEADER

#include <pybind11/pybind11.h>
namespace py = pybind11;

#include <iostream>
#include <string>
#include <sstream>
#include <string_view>
#include <memory>
#include <map>
using namespace std::string_literals;

#include "typedefs.hxx"

template <typename T>
constexpr auto _type_name() noexcept {
  std::string_view name = "Error: unsupported compiler", prefix, suffix;
#ifdef __clang__
  name = __PRETTY_FUNCTION__;
  prefix = "auto _type_name() [T = ";
  suffix = "]";
#elif defined(__GNUC__)
  name = __PRETTY_FUNCTION__;
  prefix = "constexpr auto _type_name() [with T = ";
  suffix = "]";
#elif defined(_MSC_VER)
  name = __FUNCSIG__;
  prefix = "auto __cdecl _type_name<";
  suffix = ">(void) noexcept";
#endif
  name.remove_prefix(prefix.size());
  name.remove_suffix(suffix.size());
  return name;
}
#define TYPE_STR(X) (_type_name<decltype(X)>())


#define CHECK_BIT(var,pos) (((var)>>(pos))&1)
#define SET_BIT(var,pos,x)   ((var)|(x<<pos))

/**********************************************************************/
// here was FMT_HEADER_ONLY...
#include "fmt/format.h"
#include "fmt/ranges.h"

/******************************************************************************/
template<typename... ts>
std::tuple<ts...> parse_vals (std::istringstream& istr) {
	std::tuple<ts...>  vals;

	auto done = std::apply ([&] (auto&&... val) {
		return ((istr >> val) and ...);
	}, vals);

	if (not done) throw std::invalid_argument
	(fmt::format("failed to parse \"{}\" -> {}", istr.str(), TYPE_STR(vals)));
	
	return vals;
}

template<typename... ts>
std::tuple<ts...> parse_vals (const std::string& line) {
	std::istringstream istr (line);
	return parse_vals<ts...> (istr);
}

/*******************************************************************************
 * Fowler–Noll–Vo hash functions, see ref.
 * [Eastlake, Donald; Hansen, Tony; Fowler, Glenn; Vo, Kiem-Phong; Noll, Landon
 * 29 May 2019. tools.ietf.org. The FNV Non-Cryptographic Hash Algorithm],
 * see also [https://github.com/sindresorhus/fnv1a]
 ******************************************************************************/
inline constexpr size_t _hash (const char *str, size_t n=0) {
	// FNV-1a
	size_t res{0xcbf29ce484222325}, prime{0x00000100000001b3};
	for (size_t i{0}; i < n or str[i]; ++i) {
		res = (res ^ str[i]) * prime;
	}
	return res;
}

inline constexpr size_t operator ""_hash (const char *str, size_t n) {
	return _hash (str, n);
}

inline size_t _hash (py::handle str) {
	return _hash (py::cast<std::string>(str).c_str()); 
}

/******************************************************************************/
inline std::string& ltrim(std::string &s) {
	
	s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), [] (unsigned char c) {
		return std::iscntrl(c) or std::isblank(c);
	}));
	
	return s;
}

inline std::string& rtrim(std::string &s) {
	
	s.erase(std::find_if_not(s.rbegin(), s.rend(), [] (unsigned char c) {
		return std::iscntrl(c) or std::isblank(c);
	}).base(), s.end());
	
	return s;
}

inline std::string& trim(std::string &s) {
	ltrim(s); rtrim(s);
	return s;
}

/******************************************************************************/

template<typename t, size_t n, typename = std::enable_if_t<not std::is_same<t,char>(), void>>
decltype(auto) operator <<
(std::ostream & lhs, const t (&rhs)[n]){
	for(auto [i,val] : enumerate(rhs,1)){
		lhs << (i==1?"[":"") << val << (i==n? "]":", ");
	} return lhs;
}
template<typename... ts>
decltype(auto) operator <<
(std::ostream & lhs, const std::tuple<ts...>& rhs) {
	auto func = [&lhs](auto&&... arg) {
		size_t n=sizeof...(arg), i=1;
		((lhs << (i==1? "(":"") << arg << (i==n? ")":", "), ++i), ...);
	}; std::apply(func, rhs);
	return lhs;
}

template<typename t>
decltype(auto) operator <<
(std::ostream & lhs, const std::vector<t>& rhs){
	for(auto [i,x] : enumerate(rhs,size_t(1))){
		lhs << (i==1?"[":"") << x << (i==rhs.size()? "]":", ");
	} return lhs;
}

template<typename t, size_t n>
decltype(auto) operator <<
(std::ostream & lhs, const std::array<t,n>& rhs) {
	for(auto [i,x] : enumerate(rhs,size_t(1))){
		lhs << (i==1?"[":"") << x << (i==n? "]":", ");
	} return lhs;
}

template<class x, typename y>
decltype(auto) operator <<
(std::ostream & lhs, const std::map<x,y>& rhs) {
	for(auto [i,rh] : enumerate(rhs,size_t(1))) {
		auto& [k,v] = rh;
		lhs << (i==1?"{":"") << k<<":"<<v << (i==rhs.size()?"}":", ");
	} return lhs;
}

/**********************************************************************/
decltype(auto) inline operator * (int n, const std::string& s) {
	std::string tmp{""};
	for(int i=0; i<n; ++i) tmp += s;
	return tmp;
}

decltype(auto) inline operator * (const std::string& s, int n) {
	return n*s;
}
/**********************************************************************/

decltype(auto) inline replace
(std::string str, std::string&& from, std::string&& to){
	if (from == to) {
		return str;
	} while (str.find(from) != std::string::npos) {
		str.replace(str.find(from), from.length(), to);
	} return str;
}

/**********************************************************************/
namespace logger {
	
	template<typename... ts>
	void debug (fmt::format_string<ts...> arg, ts&&... args) {
		py::module::import("logging").attr("getLogger")("_ltplib").attr("debug")
		(fmt::format(arg, std::forward<ts>(args)...).c_str());
	}

	template<typename... ts>
	void info (fmt::format_string<ts...> arg, ts&&... args) {
		py::module::import("logging").attr("getLogger")("_ltplib").attr("info")
		(fmt::format(arg, std::forward<ts>(args)...).c_str());
	}
	
	template<typename... ts>
	void warning (fmt::format_string<ts...> arg, ts&&... args) {
		py::module::import("logging").attr("getLogger")("_ltplib").attr("warning")
		(fmt::format(arg, std::forward<ts>(args)...).c_str());
	}

	template<typename... ts>
	void error (fmt::format_string<ts...> arg, ts&&... args) {
		py::module::import("logging").attr("getLogger")("_ltplib").attr("error")
		(fmt::format(arg, std::forward<ts>(args)...).c_str());
	}

	template<typename... ts>
	void critical (fmt::format_string<ts...> arg, ts&&... args) {
		py::module::import("logging").attr("getLogger")("_ltplib").attr("critical")
		(fmt::format(arg, std::forward<ts>(args)...).c_str());
	}

}

template<typename tp>
const char* datatypecode() {
	if constexpr (std::is_same<tp, u8>::value)  return "u8";
	if constexpr (std::is_same<tp, u16>::value) return "u16";
	if constexpr (std::is_same<tp, u32>::value) return "u32";
	if constexpr (std::is_same<tp, u64>::value) return "u64";
	if constexpr (std::is_same<tp, f32>::value)    return "f32";
	if constexpr (std::is_same<tp, f64>::value)   return "f64";
	else throw;
}

#endif
