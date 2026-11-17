# Closes the game the way the player closing its window does (on_closed, then ClosedWindowError).
# Usage: litergss-agent run quit   (envs stop uses it; never stop a game with a signal)
LiteRGSS::Agent.request_quit!
{ closing: true }
