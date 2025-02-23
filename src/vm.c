#include <stdint.h> //required for uin16_t

// MARK: hardware components

// MARK: includes
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
/* unix only */
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

//MARK:regs
enum
{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,   // program counter
    R_COND, // conditional
    R_COUNT
};

//MARK: condition flags
enum
{
    FL_POS = 1 << 0,/* P */ // 1
    FL_ZRO = 1 << 1,/* Z */ // 2
    FL_NEG = 1 << 2,/* N */ // 4
};

//MARK: opcodes
enum
{
    OP_BR = 0, /* branch */
    OP_ADD,    /* add  */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap */
};

// -------------------------------------------------------------


//MARK: memory mapped regs
enum
{
    MR_KBSR = 0xFE00, /* keyboard status */
    MR_KBDR = 0xFE02  /* keyboard data */
};

//MARK: trap codes
enum
{
    TRAP_GETC = 0x20,  /* get character from keyboard, not echoed onto the terminal */
    TRAP_OUT = 0x21,   /* output a character */
    TRAP_PUTS = 0x22,  /* output a word string */
    TRAP_IN = 0x23,    /* get character from keyboard, echoed onto the terminal */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT = 0x25   /* halt the program */
};





//MARK: memory storage
#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX]; // 65536 locations



//MARK: reg storage
uint16_t reg[R_COUNT];













// MARK: helper functions

// input buffering
struct termios original_tio;
void disable_input_buffering()
{
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}
void restore_input_buffering()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}
uint16_t check_key()
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
}


//handle interrupt
void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}


//sign extend
uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1)
    {
        x |= (0xFFFF << bit_count);
    }
    return x;
}


//swap
uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}


//update flags
void update_flags(uint16_t r)
{
    if (reg[r] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15)
    { /* 1 in left most bit means negetive*/
        reg[R_COND] = FL_NEG;
    }
    else
    {
        reg[R_COND] = FL_POS;
    }
}


//Read Image File
void read_image_file(FILE* file)
{
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    while(read-- > 0)
    {
        *p = swap16(*p);
        ++p;
    }
}


//read image
int read_image(const char* image_path)
{
    FILE* file = fopen(image_path, "rb");
    if (!file)
    {
        return 0;
    }
    read_image_file(file);
    fclose(file);
    return 1;
}


//memory access
void mem_write(uint16_t address, uint16_t val)
{
    memory[address] = val;
}

uint16_t mem_read(uint16_t address)
{
    if (address == MR_KBSR)
    {
        if (check_key())
        {
            memory[MR_KBSR] = (1 << 15);
            printf("waiting for input...\n");
            memory[MR_KBDR] = getchar();
            printf("got input: %c\n", memory[MR_KBDR]);
        } else
        {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}




// MARK: main loop
int main(int argc, const char *argv[])
{

    // Load Arguments
    if (argc < 2)
    {
        printf("lc3 [image-file1] ...\n");
        printf("no image file was provided");
        exit(2); // exit code 2 indicates incorrent usage
    }

    for (int j = 1; j < argc; j++)
    {
        if (!read_image(argv[j]))
        {
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    //@Setup
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();


    reg[R_COND] = FL_ZRO;

    enum
    {
        PC_START = 0x3000
    };
    reg[R_PC] = PC_START;

    int running = 1;
    while (running)
    {

        // fetchin instruction
        uint16_t instr = mem_read(reg[R_PC]++);
        uint16_t op = instr >> 12;

        switch (op)
        {
            case OP_ADD:
            {
                uint16_t r0 = (instr >> 9) & 0x7;       /*DR*/
                uint16_t r1 = (instr >> 6) & 0x7;       /*SR1*/
                uint16_t imm_flag = (instr >> 5) & 0x1; /*immediate or not*/

                if (imm_flag)
                {
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    reg[r0] = reg[r1] + imm5;
                }
                else
                {
                    uint16_t r2 = instr & 0x7;
                    reg[r0] = reg[r1] + reg[r2];
                }

                update_flags(r0);
            }
            break;

            case OP_AND:
            {
                uint16_t r0 = (instr >> 9) & 0x7; /*DR*/
                uint16_t r1 = (instr >> 6) & 0x7; /*SR1*/
                uint16_t imm_flag = (instr >> 5) & 0x1;

                if (imm_flag)
                {
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    reg[r0] = reg[r1] & imm5;
                }
                else
                {
                    uint16_t r2 = instr & 0x7;
                    reg[r0] = reg[r1] & reg[r2];
                }
                update_flags(r0);
            }
            break;

            case OP_NOT:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t r1 = (instr >> 6) & 0x7;

                reg[r0] = ~reg[r1];

                update_flags(r0);
            }
            break;

            case OP_BR:
            {
                uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                uint16_t cond_flag = (instr >> 9) & 0x7;

                if (cond_flag & reg[R_COND])
                {
                    reg[R_PC] += pc_offset;
                }
            }
            break;

            case OP_JMP:
            {
                uint16_t baseR = (instr >> 6) & 0x7;
                reg[R_PC] = reg[baseR];
            }
            break;

            case OP_JSR:
            {
                uint16_t use_PCoffset = (instr >> 11) & 0x1;
                reg[R_R7] = reg[R_PC];

                if (use_PCoffset)
                {
                    uint16_t PCoffset11 = sign_extend(instr & 0x7FF, 11);
                    reg[R_PC] +=  PCoffset11;
                }
                else
                {
                    uint16_t baseR = (instr >> 6) & 0x7;
                    reg[R_PC] = reg[baseR];
                }
            }
            break;

            case OP_LD:
            {
                uint16_t DR = (instr >> 9) & 0x7;
                uint16_t PCoffset9 = sign_extend(instr & 0x1FF, 9);
                reg[DR] = mem_read(reg[R_PC] + PCoffset9);
                update_flags(DR);
            }
            break;

            case OP_LDI:
            {
                uint16_t r0 = (instr >> 9) & 0x7; /*DR*/
                uint16_t pc_offset = sign_extend(instr & 0x01FF, 9);

                reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
                update_flags(r0);
            }
            break;

            case OP_LDR:
            {
                uint16_t DR = (instr >> 9) & 0x7;
                uint16_t baseR = (instr >> 6) & 0x7;
                uint16_t offset6 = sign_extend(instr & 0x3F, 6);
                reg[DR] = mem_read(reg[baseR] + offset6);
                update_flags(DR);
            }
            break;

            case OP_LEA:
            {
                uint16_t DR = (instr >> 9) & 0x7;
                uint16_t PCoffset9 = sign_extend(instr & 0x1FF, 9);
                reg[DR] = reg[R_PC] + PCoffset9;
                update_flags(DR);
            }
            break;

            case OP_ST:
            {
                uint16_t SR = (instr >> 9) & 0x7;
                uint16_t PCoffset9 = sign_extend(instr & 0x1FF, 9);

                mem_write(reg[R_PC] + PCoffset9, reg[SR]); //stores value from register to memory
            }
            break;

            case OP_STI:
            {
                uint16_t SR = (instr >> 9) & 0x7;
                uint16_t PCoffset9 = sign_extend(instr & 0x1FF, 9);

                mem_write(mem_read(reg[R_PC] + PCoffset9), reg[SR]);
            }
            break;

            case OP_STR:
            {
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t r1 = (instr >> 6) & 0x7;
                uint16_t offset = sign_extend(instr & 0x3F, 6);
                mem_write(reg[r1] + offset, reg[r0]);
            }
            break;

            case OP_TRAP:
            {
                reg[R_R7] = reg[R_PC];
                switch (instr & 0xFF)
                {
                    case TRAP_GETC:
                    {
                        reg[R_R0] = (uint16_t)getchar();
                        update_flags(R_R0);
                    }
                    break;

                    case TRAP_OUT:
                    {
                        putc((char)reg[R_R0], stdout);
                        fflush(stdout);
                    }
                    break;

                    case TRAP_PUTS:
                    {
                        uint16_t* c = memory + reg[R_R0];
                        while (*c)
                        {
                            putc((char)*c, stdout);
                            ++c;
                        }
                        fflush(stdout);
                    }
                    break;

                    case TRAP_IN:
                    {
                        printf("Enter a char: ");
                        char c = getchar();
                        putc(c, stdout);
                        fflush(stdout);
                        reg[R_R0] = (uint16_t)c;
                        update_flags(R_R0);
                    }
                    break;

                    case TRAP_PUTSP:
                    {
                        uint16_t* c = memory + reg[R_R0];
                        while(*c)
                        {
                            char c1 = (*c) * 0xFF;
                            putc(c1, stdout);

                            char c2 = (*c) >> 8;
                            if (c2)
                            {
                                putc(c2, stdout);
                            }

                            ++c;
                        }
                        fflush(stdout);
                    }
                    break;

                    case TRAP_HALT:
                    {
                        puts("HALTT");
                        fflush(stdout);
                        running = 0;
                    }
                    break;
                }
            }
            break;

            case OP_RES:
            case OP_RTI:
            default:
                abort();
                break;
        }
    }

    //@shudown
    restore_input_buffering();
}
