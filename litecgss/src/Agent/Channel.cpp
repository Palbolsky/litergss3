#include "LiteCGSS/Agent/Channel.h"

#include <atomic>
#include <unordered_map>

#include "LiteCGSS/Backend/ActiveBackendFwd.h"

const char* cgss::agent::BackendName() {
	return cgss::backend::ActiveBackend::name;
}

namespace {
	std::atomic<std::uint64_t> g_drains { 0 };
}

std::uint64_t cgss::agent::DrainCount() {
	return g_drains.load();
}

#if defined(LITECGSS_AGENT_ENABLED) && !defined(_WIN32) && !defined(__ANDROID__)

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

namespace {
	std::string json_escape(const std::string& text) {
		std::string out;
		out.reserve(text.size());
		for (const char c : text) {
			switch (c) {
				case '"':  out += "\\\""; break;
				case '\\': out += "\\\\"; break;
				case '\n': out += "\\n"; break;
				case '\r': out += "\\r"; break;
				case '\t': out += "\\t"; break;
				default:
					if (static_cast<unsigned char>(c) < 0x20) {
						static const char* hex = "0123456789abcdef";
						out += "\\u00";
						out += hex[(c >> 4) & 0xF];
						out += hex[c & 0xF];
					} else {
						out += c;
					}
			}
		}
		return out;
	}

	std::string error_reply(const std::string& message) {
		return "{\"ok\":false,\"error\":\"" + json_escape(message) + "\"}";
	}

	// Bounds what one Drain() costs the frame: a few requests, a short read.
	constexpr unsigned kMaxRequestsPerDrain = 4;
	constexpr std::size_t kMaxRequestBytes = 16u << 20;
	constexpr int kReadTimeoutMs = 2000;

	int g_listenFd = -1;
	std::string g_socketPath;
	std::atomic<bool> g_draining { false };
	std::unordered_map<std::string, cgss::agent::Handler> g_handlers;

	bool fill_address(const std::string& path, sockaddr_un& address) {
		std::memset(&address, 0, sizeof(address));
		address.sun_family = AF_UNIX;
		if (path.size() >= sizeof(address.sun_path)) return false;
		std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
		return true;
	}

	// Another live process answers on `path`.
	bool socket_in_use(const sockaddr_un& address) {
		const int probe = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
		if (probe < 0) return false;
		const bool live = ::connect(probe, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0;
		::close(probe);
		return live;
	}

	bool same_user(int fd) {
#ifdef SO_PEERCRED
		ucred credentials{};
		socklen_t length = sizeof(credentials);
		if (::getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &credentials, &length) != 0) return false;
		return credentials.uid == ::getuid();
#else
		uid_t uid; gid_t gid;
		if (::getpeereid(fd, &uid, &gid) != 0) return false;
		return uid == ::getuid();
#endif
	}

	bool read_request(int fd, std::string& request) {
		timeval timeout{ kReadTimeoutMs / 1000, (kReadTimeoutMs % 1000) * 1000 };
		::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		char buffer[8192];
		for (;;) {
			const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
			if (n == 0) return true;
			if (n < 0) {
				if (errno == EINTR) continue;
				return false;
			}
			request.append(buffer, static_cast<std::size_t>(n));
			if (request.size() > kMaxRequestBytes) return false;
		}
	}

	void write_reply(int fd, const std::string& reply) {
		std::size_t sent = 0;
		while (sent < reply.size()) {
#ifdef MSG_NOSIGNAL
			const ssize_t n = ::send(fd, reply.data() + sent, reply.size() - sent, MSG_NOSIGNAL);
#else
			const ssize_t n = ::send(fd, reply.data() + sent, reply.size() - sent, 0);
#endif
			if (n < 0) {
				if (errno == EINTR) continue;
				return;
			}
			sent += static_cast<std::size_t>(n);
		}
	}

	std::string dispatch(const std::string& request) {
		const auto newline = request.find('\n');
		const std::string verb = request.substr(0, newline);
		const std::string payload = newline == std::string::npos ? std::string{} : request.substr(newline + 1);
		if (verb == "ping") {
			return "{\"ok\":true,\"value\":\"pong\",\"backend\":\"" + std::string{ cgss::agent::BackendName() }
				+ "\",\"frame\":" + std::to_string(g_drains.load()) + "}";
		}
		const auto handler = g_handlers.find(verb);
		if (handler == g_handlers.end()) return error_reply("unknown verb '" + verb + "'");
		try {
			return handler->second(payload);
		} catch (const std::exception& e) {
			return error_reply(std::string{ "verb '" } + verb + "' failed: " + e.what());
		} catch (...) {
			return error_reply("verb '" + verb + "' failed");
		}
	}

	void serve(int fd) {
		std::string request;
		if (!same_user(fd)) {
			write_reply(fd, error_reply("refused: caller is another user"));
		} else if (!read_request(fd, request)) {
			write_reply(fd, error_reply("bad request (timed out, or larger than 16 MB)"));
		} else {
			write_reply(fd, dispatch(request));
		}
		::close(fd);
	}
}

bool cgss::agent::Available() {
	return true;
}

std::string cgss::agent::Open(const std::string& socketPath) {
	if (g_listenFd >= 0) return "already open on " + g_socketPath;
	sockaddr_un address{};
	if (!fill_address(socketPath, address)) {
		return "socket path too long (" + std::to_string(socketPath.size()) + " bytes, at most "
			+ std::to_string(sizeof(address.sun_path) - 1) + "): " + socketPath;
	}
	if (::access(socketPath.c_str(), F_OK) == 0) {
		if (socket_in_use(address)) return "another process serves " + socketPath;
		::unlink(socketPath.c_str());
	}
	const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0) return std::string{ "socket(): " } + std::strerror(errno);
	// Owner-only from the start: the socket file is created 0600.
	const mode_t previousMask = ::umask(0177);
	const int bound = ::bind(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
	const int bindErrno = errno;
	::umask(previousMask);
	if (bound != 0) {
		::close(fd);
		return std::string{ "bind(" } + socketPath + "): " + std::strerror(bindErrno);
	}
	if (::listen(fd, 8) != 0) {
		const int listenErrno = errno;
		::close(fd);
		::unlink(socketPath.c_str());
		return std::string{ "listen(): " } + std::strerror(listenErrno);
	}
	::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
	g_listenFd = fd;
	g_socketPath = socketPath;
	return {};
}

bool cgss::agent::IsOpen() {
	return g_listenFd >= 0;
}

void cgss::agent::Close() {
	if (g_listenFd < 0) return;
	::close(g_listenFd);
	::unlink(g_socketPath.c_str());
	g_listenFd = -1;
	g_socketPath.clear();
}

void cgss::agent::Register(const std::string& verb, Handler handler) {
	g_handlers[verb] = std::move(handler);
}

unsigned cgss::agent::Drain() {
	if (g_draining.exchange(true)) return 0;
	g_drains.fetch_add(1);
	unsigned served = 0;
	while (g_listenFd >= 0 && served < kMaxRequestsPerDrain) {
		const int client = ::accept(g_listenFd, nullptr, nullptr);
		if (client < 0) break; // EAGAIN: nobody waiting
		::fcntl(client, F_SETFD, FD_CLOEXEC);
		// accepted sockets don't inherit O_NONBLOCK on Linux; make sure
		::fcntl(client, F_SETFL, ::fcntl(client, F_GETFL, 0) & ~O_NONBLOCK);
		serve(client);
		++served;
	}
	g_draining.store(false);
	return served;
}

#else // channel compiled out (LITECGSS_AGENT OFF, Windows, Android)

bool cgss::agent::Available() {
	return false;
}

std::string cgss::agent::Open(const std::string&) {
	return "the agent channel is not compiled in (LITECGSS_AGENT is OFF in this build)";
}

bool cgss::agent::IsOpen() {
	return false;
}

void cgss::agent::Close() {}

void cgss::agent::Register(const std::string&, Handler) {}

unsigned cgss::agent::Drain() {
	g_drains.fetch_add(1);
	return 0;
}

#endif
