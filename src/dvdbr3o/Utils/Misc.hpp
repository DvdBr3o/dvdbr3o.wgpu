#pragma once

#include <spdlog/spdlog.h>
#include <exec/task.hpp>
#include <exec/asio/use_sender.hpp>
#include <exec/asio/asio_thread_pool.hpp>
#include <asio.hpp>
#include <exec/asio/use_sender.hpp>

#include <format>
#include <filesystem>
#include <fstream>

namespace dvdbr3o {
template<typename... Ts>
inline auto panic(const std::format_string<Ts...>& fmt, Ts&&... args) {
	spdlog::critical("{}", std::format(fmt, std::forward<Ts>(args)...));
	std::terminate();
}

inline auto async_read_text_from(std::filesystem::path path, auto executor)
	-> exec::task<std::string> {
	asio::stream_file file { executor };
	file.open(path.string(), asio::stream_file::read_only);

	std::string			   out;
	std::array<char, 4096> buf {};

	while (true) {
		std::size_t n = 0;
		try {
			n = co_await file.async_read_some(asio::buffer(buf), exec::asio::use_sender);
		} catch (const std::system_error& e) {
			if (e.code() == asio::error::eof)
				break;
			throw;
		}
		out.append(buf.data(), n);
	}

	co_return out;
}

inline auto read_text_from(const std::filesystem::path& path) -> std::optional<std::string> {
	std::ifstream in { path };

	if (!in.is_open())
		return std::nullopt;

	in.seekg(0);
	std::string content { std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };
	in.close();

	return content;
}

inline constexpr auto catch_exception_and_log = stdexec::upon_error([](auto&& e) {
	try {
		if (e)
			std::rethrow_exception(e);
	} catch (const std::exception& e) { spdlog::error("async failed: {}", e.what()); } catch (...) {
		spdlog::error("async failed: unknown exception");
	}
});

template<typename T>
concept Hashable = requires(T t) { std::hash<T>()(t); };

template<typename T, template<typename...> class TempT>
struct is_like : std::false_type {};

template<typename... Ts, template<typename...> class TempT>
struct is_like<TempT<Ts...>, TempT> : std::true_type {};

template<typename T, template<typename...> class TempT>
inline constexpr auto is_like_v = is_like<T, TempT>::value;

template<typename T, template<typename...> class TempT>
concept Like = is_like<T, TempT>::value;

template<typename T>
concept FatPointerAlike = requires(T t) {
	t.data();
	t.size();
};

template<auto f>
concept ConstEvaluated = requires { typename std::bool_constant<(f, true)>; };

template<class T>
inline constexpr void hash_combine(std::size_t& s, const T& v) {
	std::hash<T> h;
	s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

class UnlikelyExcption : public std::exception {
public:
	UnlikelyExcption() = default;

	UnlikelyExcption(std::string_view reason) : _reason(reason) {}

	template<typename... Args>
	UnlikelyExcption(const std::format_string<Args...>& fmt, Args&&... args) :
		_reason(std::format(fmt, std::forward<Args>(args)...)) {}

	[[nodiscard]] auto what() const noexcept -> const char* override { return _reason.c_str(); }

private:
	std::string _reason;
};

template<typename T>
inline constexpr auto to_span(const std::vector<T>& vec) -> std::span<const T> {
	return std::span(vec.data(), vec.size());
}

}  // namespace dvdbr3o
