# Holds the game at its next update: no game code runs, the window stays alive, requests are served.
# Usage: litergss-agent run pause   (then: run step N · run shot · run resume)
LiteRGSS::Agent.pause!
{ paused: true, frame: LiteRGSS::Agent.frame }
