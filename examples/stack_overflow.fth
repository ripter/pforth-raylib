
800 600 s" stack_overflow.fth" init-window

: should-close ( -- flag )
    key? ;



: check-stack-empty ( -- )
    depth dup 0<> if
        cr ." Warning: Stack is not empty!" cr .s cr
    else
        drop
    then ;


: TEST-STACK-BUG ( -- )
  BEGIN
    window-should-close 0=  
  WHILE
    \ Draw
    begin-drawing
        DARKBROWN 2drop 2drop 
        check-stack-empty
    end-drawing
  REPEAT
  \ De-Initialization
  close-window
;


TEST-STACK-BUG






