800 constant screen-width
450 constant screen-height
60 constant target-fps

screen-width screen-height s" Hello Raylib from Forth!" init-window
target-fps set-target-fps

: GAME-LOOP ( -- )
    BEGIN
        window-should-close 0=  \ Continue looping as long as the window should not close
    WHILE
        begin-drawing
        RAYWHITE clear-background
        s" Congrats! You opened a window from Forth!" 190 200 20 ORANGE draw-text
        end-drawing
    REPEAT
    close-window
;

19 game-loop
