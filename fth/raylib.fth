\ Custom raylib color palette for amazing visuals on WHITE background
: LIGHTGRAY 200 200 200 255 ;
: GRAY 130 130 130 255 ;
: DARKGRAY 80 80 80 255 ;
: YELLOW 253 249 0 255 ;
: GOLD 255 203 0 255 ;
: ORANGE 255 161 0 255 ;
: PINK 255 109 194 255 ;
: RED 230 41 55 255 ;
: MAROON 190 33 55 255 ;
: GREEN 0 228 48 255 ;
: LIME 0 158 47 255 ;
: DARKGREEN 0 117 44 255 ;
: SKYBLUE 102 191 255 255 ;
: BLUE 0 121 241 255 ;
: DARKBLUE 0 82 172 255 ;
: PURPLE 200 122 255 255 ;
: VIOLET 135 60 190 255 ;
: DARKPURPLE 112 31 126 255 ;
: BEIGE 211 176 131 255 ;
: BROWN 127 106 79 255 ;
: DARKBROWN 76 63 47 255 ;

: WHITE 255 255 255 255 ;
: BLACK 0 0 0 255 ;
: BLANK 0 0 0 0 ;
: MAGENTA 255 0 255 255 ;
: RAYWHITE 245 245 245 255 ;


\ System/Window config flags
HEX  \ Switch to hexadecimal mode
40 CONSTANT FLAG_VSYNC_HINT         \ Set to try enabling V-Sync on GPU
02 CONSTANT FLAG_FULLSCREEN_MODE    \ Set to run program in fullscreen
04 CONSTANT FLAG_WINDOW_RESIZABLE   \ Set to allow resizable window
08 CONSTANT FLAG_WINDOW_UNDECORATED \ Set to disable window decoration (frame and buttons)
80 CONSTANT FLAG_WINDOW_HIDDEN      \ Set to hide window
200 CONSTANT FLAG_WINDOW_MINIMIZED  \ Set to minimize window (iconify)
400 CONSTANT FLAG_WINDOW_MAXIMIZED  \ Set to maximize window (expanded to monitor)
800 CONSTANT FLAG_WINDOW_UNFOCUSED  \ Set to window non focused
1000 CONSTANT FLAG_WINDOW_TOPMOST   \ Set to window always on top
100 CONSTANT FLAG_WINDOW_ALWAYS_RUN \ Set to allow windows running while minimized
10 CONSTANT FLAG_WINDOW_TRANSPARENT \ Set to allow transparent framebuffer
2000 CONSTANT FLAG_WINDOW_HIGHDPI   \ Set to support HighDPI
4000 CONSTANT FLAG_WINDOW_MOUSE_PASSTHROUGH \ Set to support mouse passthrough
8000 CONSTANT FLAG_BORDERLESS_WINDOWED_MODE \ Set to run program in borderless windowed mode
20 CONSTANT FLAG_MSAA_4X_HINT       \ Set to try enabling MSAA 4X
10000 CONSTANT FLAG_INTERLACED_HINT \ Set to try enabling interlaced video format (for V3D)
DECIMAL  \ Switch back to decimal mode (optional)


