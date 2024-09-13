\ https://github.com/raysan5/raylib/blob/master/examples/core/core_input_keys.c
\ https://www.raylib.com/examples.html
\ Define screen dimensions and target FPS
800 constant screen-width
450 constant screen-height
60 constant target-fps

\ Initialize the window
screen-width screen-height s" raylib [core] example - keyboard input" init-window

\ Set target FPS
target-fps set-target-fps

\ Define variables for ball position
variable ball-x
variable ball-y

\ Initialize ball position to the center of the screen
screen-width  s>f 2e f/ ball-x f!
screen-height s>f 2e f/ ball-y f!

\ Define a helper word for adding to a float variable
: f+! ( r addr -- )
    over f@ f+ swap f! ;

\ Main game loop
: game-loop ( -- )
    BEGIN
        window-should-close 0=  \ Continue looping as long as the window should not close
    WHILE
        \ Update ball position based on key input
        KEY_RIGHT is-key-down IF 2e   ball-x f+! THEN
        KEY_LEFT  is-key-down IF -2e  ball-x f+! THEN
        KEY_UP    is-key-down IF -2e  ball-y f+! THEN
        KEY_DOWN  is-key-down IF 2e   ball-y f+! THEN

        \ Start drawing
        begin-drawing
            RAYWHITE clear-background
            s" move the ball with arrow keys" 10 10 20 DARKGRAY draw-text

            \ Draw the ball at the current position
            ball-x f@ ball-y f@  \ Retrieve ball position
            50e MAROON draw-circle-v
        end-drawing
    REPEAT

    \ Close the window and clean up resources
    close-window ;

\ Start the game
game-loop
