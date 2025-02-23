        .ORIG x3000

        ; Prompt the user for input
        LEA R0, PROMPT
        PUTS

        ; Get character input
        GETC
        OUT

        ; Check if input is 'y'
        LD R1, CHAR_Y
        ADD R2, R0, R1
        BRz PRINT_YES

        ; Check if input is 'n'
        LD R1, CHAR_N
        ADD R2, R0, R1
        BRz PRINT_NO

        ; If input is neither 'y' nor 'n', print "INVALID"
        LEA R0, INVALID
        PUTS
        BR END

PRINT_YES
        LEA R0, YES_MSG
        PUTS
        BR END

PRINT_NO
        LEA R0, NO_MSG
        PUTS
        BR END

END     HALT

; Data Section
PROMPT  .STRINGZ "Enter y or n: "
YES_MSG .STRINGZ "You entered yes.\n"
NO_MSG  .STRINGZ "You entered no.\n"
INVALID .STRINGZ "Invalid input.\n"
CHAR_Y  .FILL xFF87  ; -'y' in two's complement (x0067 -> xFF87)
CHAR_N  .FILL xFF92  ; -'n' in two's complement (x006E -> xFF92)

        .END
