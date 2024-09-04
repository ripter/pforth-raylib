800 constant screen-width
450 constant screen-height
60 constant target-fps

screen-width screen-height s" Hello Raylib from Forth!" init-window
." \n init-window: " .s
target-fps set-target-fps
." \n set-target-fps: " .s

: game-loop ( -- )
    BEGIN
        window-should-close ." window should close? " .s 0=  \ Continue looping as long as the window should not close
    WHILE
        ." \n begin-drawing: " .s
        begin-drawing
        ." \n clear-background: " .s
        RAYWHITE clear-background
        ." \n draw-text: " .s
        s" Congrats! You opened a window from Forth!" 190 200 20 ORANGE draw-text
        end-drawing
        ." \n end-drawing: " .s
    REPEAT
    close-window
    ." \n close-window: " .s
;

game-loop
