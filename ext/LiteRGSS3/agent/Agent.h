#ifndef L_AGENT_H
#define L_AGENT_H

#include "RubyValue.h"

// LiteRGSS::Agent — the Ruby side of LiteCGSS' agent channel
// (<LiteCGSS/Agent/Channel.h>): the `eval`, `run` and `scripts` verbs, served
// by agent_runtime.rb. Opened at load time when LITERGSS_AGENT names a socket
// path, and only in a build with LITECGSS_AGENT ON.

void Init_Agent();

// Serves the agent requests waiting. DisplayWindow#update calls it each
// frame, GVL and GL context held, before the frame is drawn. While a client
// holds the game (`pause`), it does not return: it keeps serving requests and
// calls redraw(self) to keep the window alive — no game code runs. While a
// `step N` runs, the N updates are not served (a request waits for the end).
void agent_drain(void (*redraw)(VALUE), VALUE self);

// A client asked the game to quit (the `quit` script) since the last call.
bool agent_take_quit_request();

#endif
