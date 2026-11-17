#ifndef CGSS_AGENT_CHANNEL_H
#define CGSS_AGENT_CHANNEL_H

#include <cstdint>
#include <functional>
#include <string>

// The agent channel: a local command socket a development tool (an agent's
// `litergss-agent` CLI) uses to query and drive a running game.
//
// The engine owns the transport and a verb registry; the host (a binding)
// registers its verbs and calls Drain() at a point of its frame loop where
// those verbs may safely run (LiteRGSS: DisplayWindow#update, before the
// frame is drawn). Nothing runs on another thread.
//
// Protocol, one request per connection: the client sends `<verb>\n<payload>`
// and shuts its writing side; the reply is the verb handler's string, then
// the connection closes. Engine errors reply `{"ok":false,"error":"..."}`.
//
// Development only: compiled out unless LITECGSS_AGENT is ON (never in a
// release preset); Unix socket only (no TCP), owner-only (0600), callers of
// another user refused. Unavailable on Windows and Android.
namespace cgss::agent {

	// Payload in, reply out (sent as is).
	using Handler = std::function<std::string(const std::string& payload)>;

	// Whether the channel was compiled in (LITECGSS_AGENT).
	bool Available();

	// The graphics backend this engine was built with ("raylib", "sfml"...),
	// for a tool to report what it is talking to.
	const char* BackendName();

	// Opens the channel on a Unix socket at `socketPath` (its folder must
	// exist). Returns an empty string on success, else why it did not open
	// (unavailable, path too long, another live process on that socket...).
	// A stale socket file left by a dead process is replaced.
	std::string Open(const std::string& socketPath);
	bool IsOpen();
	// Closes the channel and removes its socket file.
	void Close();

	// Registers (or replaces) the handler of `verb`. Built-in: `ping`.
	void Register(const std::string& verb, Handler handler);

	// Serves the requests waiting, on the caller's thread; never blocks for a
	// client that has not connected. Re-entrant calls (a handler's code
	// reaching Drain() again) serve nothing. Returns the number served.
	unsigned Drain();

	// Drain() calls so far: the host's frame count when it drains once a frame.
	std::uint64_t DrainCount();
}

#endif
