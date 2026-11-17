# Presses a key the way the keyboard would (events + polled state), released N frames later.
# Usage: litergss-agent run press <Key> [frames]   e.g. press Enter · press Down 6 · press LControl 30
# <Key>: a LiteRGSS::Inputs::Keyboard (Sf::Keyboard) constant.
keyboard = LiteRGSS::Inputs::Keyboard
name = args[0] or raise ArgumentError, 'press: key name missing (e.g. Enter, Down, A, Escape)'
frames = Integer(args[1] || 2)
raise ArgumentError, "press: unknown key '#{name}'" unless keyboard.const_defined?(name, false)

code = keyboard.const_get(name, false)
scancodes = keyboard.const_defined?(:Scancode, false) ? keyboard::Scancode : nil
scancode = scancodes&.const_defined?(name, false) ? scancodes.const_get(name, false) : -1
LiteRGSS::Agent.inject_key(code, scancode, true)
LiteRGSS::Agent.after_frames(frames) { LiteRGSS::Agent.inject_key(code, scancode, false) }
{ injected: name, down_from: LiteRGSS::Agent.frame + 1, released_at: LiteRGSS::Agent.frame + frames }
