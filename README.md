# HVNC
A basic performant HVNC implementation. Based very loosely on Tinynuke HVNC, does not include WM. Server is written for browsers, not Windows, so the input functionality heavy lifting must be done manually, hence only support for left click and character inputs is implemented. Note that this is NOT intended to be a full production quality HVNC implementation, just a simple implementation to allow for effective understanding of how HVNC works.

## Differences from Tinynuke
1. Written in pure C, not C++
2. Cross-platform server module, written in python and HTML/JS
3. Significantly decreased latency, improved performance
4. WM not included, but WM code can be easily pasted from tinynuke into this implementation

## Compilation
Compiles with mingw gcc, use command `i686-w64-mingw32-gcc hvnc.c -lgdi32 -lws2_32 -lole32`
