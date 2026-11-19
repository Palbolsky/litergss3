#include "Agent.h"

#include <cstdlib>
#include <cstring>
#include <string>

#include "LiteRGSS.h"
#include "LiteCGSS/Agent/Channel.h"
#include "LiteCGSS/Inputs/Injection.h"

// The runtime's Ruby source (agent_runtime.rb), embedded at build time.
#include "AgentRuntime.h"
#include "../Profiling.h"

namespace {
	VALUE rb_mAgent = Qnil;
	bool g_runtime_loaded = false;

	struct HandleCall {
		const char* verb;
		const std::string* payload;
	};

	VALUE call_handle(VALUE arg)
	{
		auto* call = reinterpret_cast<HandleCall*>(arg);
		VALUE verb = rb_utf8_str_new_cstr(call->verb);
		VALUE payload = rb_utf8_str_new(call->payload->data(), static_cast<long>(call->payload->size()));
		return rb_funcall(rb_mAgent, rb_intern("handle"), 2, verb, payload);
	}

	// Runs LiteRGSS::Agent.handle(verb, payload): never lets a Ruby exception
	// unwind through the engine's C++ frames.
	std::string handle(const char* verb, const std::string& payload)
	{
		HandleCall call{ verb, &payload };
		int state = 0;
		VALUE reply = rb_protect(call_handle, reinterpret_cast<VALUE>(&call), &state);
		if (state != 0 || !RB_TYPE_P(reply, T_STRING)) {
			rb_set_errinfo(Qnil);
			return R"({"ok":false,"error":"the agent runtime failed to answer"})";
		}
		return std::string{ RSTRING_PTR(reply), static_cast<std::size_t>(RSTRING_LEN(reply)) };
	}

	VALUE call_simple(VALUE name)
	{
		return rb_funcall(rb_mAgent, SYM2ID(name), 0);
	}

	VALUE protected_call(const char* method)
	{
		int state = 0;
		VALUE result = rb_protect(call_simple, ID2SYM(rb_intern(method)), &state);
		if (state != 0) {
			rb_set_errinfo(Qnil);
			return Qnil;
		}
		return result;
	}

	VALUE eval_runtime(VALUE)
	{
		return rb_eval_string(kAgentRuntime);
	}

	void close_at_exit(VALUE)
	{
		cgss::agent::Close();
	}

	VALUE rb_Agent_available(VALUE) { return cgss::agent::Available() ? Qtrue : Qfalse; }
	VALUE rb_Agent_open(VALUE) { return cgss::agent::IsOpen() ? Qtrue : Qfalse; }
	VALUE rb_Agent_backend(VALUE) { return rb_utf8_str_new_cstr(cgss::agent::BackendName()); }
	// inject_key(key_code, scancode, down): Unknown scancode (-1) derives it from the key.
	VALUE rb_Agent_inject_key(VALUE, VALUE key, VALUE scancode, VALUE down)
	{
		cgss::input::injectKey(static_cast<cgss::KeyCode>(NUM2INT(key)), static_cast<cgss::Scancode>(NUM2INT(scancode)), RTEST(down));
		return Qnil;
	}

	VALUE rb_Agent_release_keys(VALUE)
	{
		cgss::input::releaseInjectedKeys();
		return Qnil;
	}
}

void Init_Agent()
{
	rb_mAgent = rb_define_module_under(rb_mLiteRGSS, "Agent");
	rb_define_module_function(rb_mAgent, "available?", RUBY_METHOD_FUNC(rb_Agent_available), 0);
	rb_define_module_function(rb_mAgent, "open?", RUBY_METHOD_FUNC(rb_Agent_open), 0);
	rb_define_module_function(rb_mAgent, "backend", RUBY_METHOD_FUNC(rb_Agent_backend), 0);
	rb_define_module_function(rb_mAgent, "inject_key", RUBY_METHOD_FUNC(rb_Agent_inject_key), 3);
	rb_define_module_function(rb_mAgent, "release_injected_keys", RUBY_METHOD_FUNC(rb_Agent_release_keys), 0);

	// Gate: the channel opens only when asked for, on the socket named.
	const char* socket = std::getenv("LITERGSS_AGENT");
	if (socket == nullptr || *socket == '\0' || std::strcmp(socket, "0") == 0) return;

	int state = 0;
	rb_protect(eval_runtime, Qnil, &state);
	if (state != 0) {
		rb_set_errinfo(Qnil);
		rb_warn("LiteRGSS agent: the runtime failed to load, channel not opened");
		return;
	}
	g_runtime_loaded = true;

	const std::string error = cgss::agent::Open(socket);
	if (!error.empty()) {
		rb_warn("LiteRGSS agent: channel not opened: %s", error.c_str());
		return;
	}
	for (const char* verb : { "eval", "run", "scripts" }) {
		cgss::agent::Register(verb, [verb](const std::string& payload) { return handle(verb, payload); });
	}
	rb_set_end_proc(close_at_exit, Qnil);
}

void agent_drain(void (*redraw)(VALUE), VALUE self)
{
	if (!cgss::agent::IsOpen()) return;
	LITERGSS_PROFILE_ZONE("LiteRGSS::agent_drain");
	protected_call("tick");
	if (RTEST(protected_call("stepping?"))) return;
	cgss::agent::Drain();
	while (RTEST(protected_call("hold?"))) {
		cgss::agent::Drain();
		if (!RTEST(protected_call("hold?"))) break; // resumed, stepping or quitting: this frame runs
		redraw(self);
	}
}

bool agent_take_quit_request()
{
	if (!g_runtime_loaded || !cgss::agent::IsOpen()) return false;
	return RTEST(protected_call("take_quit_request"));
}
