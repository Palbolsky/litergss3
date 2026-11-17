# Runs exactly N updates of a held game, then holds it again; the next request is answered after them.
# Usage: litergss-agent run step [N]   (N default 1; holds the game first if it runs)
count = Integer(args[0] || 1)
LiteRGSS::Agent.step!(count)
{ stepping: count, from_frame: LiteRGSS::Agent.frame, until_frame: LiteRGSS::Agent.frame + count }
