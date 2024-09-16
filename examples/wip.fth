\ https://github.com/raysan5/raylib/blob/master/examples/textures/textures_image_loading.c
\ https://www.raylib.com/examples.html

\ Define screen dimensions and target FPS
800 constant screen-width
450 constant screen-height
60  constant target-fps

\ Initialize the window
screen-width screen-height s" raylib [textures] example - image loading" init-window

\ Load the image and create texture
s" ../../examples/resources/raylib_logo.png" load-image constant image
\ image set-window-icon

\ Create the texture from the image
variable texture
image load-texture-from-image texture !
\ image unload-image  \ Unload image from RAM after uploading to VRAM

\ Set target FPS
target-fps set-target-fps

\ Main game loop
: game-loop ( -- )
    BEGIN
        window-should-close 0=  \ Continue looping as long as the window should not close
    WHILE
        \ Draw
        begin-drawing
            RAYWHITE clear-background

            \ ." Stack should be empty" cr .s cr

            \ Calculate texture drawing position to center it
            \ texture @ texture-get-width  2/ screen-width  2/ - dup >r  \ x position (save to return stack)
            \ texture @ texture-get-height 2/ screen-height 2/ -          \ y position

            \ WHITE texture @ over r> draw-texture  \ Draw texture at (x, y) position
            \ texture @ 300 100 RAYWHITE draw-texture

            \ s" Put Texture here" 300 170 25 GRAY draw-text

            \ s" this IS a texture loaded from an image!" 300 370 10 GRAY draw-text
        end-drawing
    REPEAT

    \ De-Initialization
    texture @ unload-texture  \ Unload texture from VRAM
    close-window
;

\ Start the game
\ game-loop
