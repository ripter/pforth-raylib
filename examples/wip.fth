800 constant screen-width
450 constant screen-height
60 constant target-fps

screen-width screen-height s" Hello Raylib from Forth!" init-window
target-fps set-target-fps

: interpret-loop ( -- )
  BEGIN
    \ Check for a line of input
    tib 80 accept  \ Accept input from the user
    evaluate       \ Evaluate the input
    s" exit" compare 0=  \ Compare input to "exit"
  UNTIL
;

: check-input-and-evaluate ( -- )
  key? IF
    key dup emit
    [CHAR] p = IF   \ Enter interpret mode when 'p' is pressed
      cr ." Entering interactive mode. Type 'exit' to return to game loop." cr
      interpret-loop
      cr ." Resuming game loop." cr
    THEN
  THEN
;

: DRAW-WINDOW! ( -- )
  RAYWHITE clear-background
  s" Congrats! You opened a window from Forth!" 190 200 20 ORANGE draw-text
;

: GAME-LOOP ( -- )
    BEGIN
        window-should-close 0=  \ Continue looping as long as the window should not close
    WHILE
        begin-drawing
        draw-window!
        end-drawing
        check-input-and-evaluate 
    REPEAT
    close-window
;

game-loop
