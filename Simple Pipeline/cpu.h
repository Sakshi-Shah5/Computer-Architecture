#ifndef _CPU_H_
#define _CPU_H_
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define NUM_REGS 128
// #define MEM_SIZE 65536
#define MAX_CPU_CYCLES 10000
#define DEBUG_PIPELINE 0
#define DEBUG_STATS 1

#define MAX_LINE_SIZE 1000 // maximum size of a line in the file
#define MAX_TOKENS 10

#define TESTING_MODE_ENABLED 1

enum opcodeFmt_enum {
	fmt_set,      // opcode, dest, imm1
	fmt_add,      // opcode, dest, sr1, imm1
	fmt_add_imm,  // opcode, dest, imm1, imm2
	fmt_sub,      // opcode, dest, sr1, imm1
	fmt_sub_imm,  // opcode, dest, imm1, imm2
	fmt_div,      // opcode, dest, sr1, imm
	fmt_div_imm,  // opcode, dest, imm1, imm2
    fmt_mul,      // opcode, dest, sr1, imm1
    fmt_mul_imm,  // opcode, dest, imm1, imm2
    fmt_ld,       // opcode, dest, imm1
    fmt_ld_imm,   // opcode, dest, sr1
    fmt_ret       // opcode
};

// Debug string macro for stages
#define FETCH "IF"
#define DECODE "ID"
#define INSTRUCTION_ANALYSER "IF"
#define REGISTER_READ "RR"
#define ADDER "ADD"
#define MULTIPLIER "MUL"
#define DIVIDER "DIV"
#define BRANCH "BR"
#define MEMORY_FIRST "Mem1"
#define MEMORY_SECOND "Mem2"
#define WRITEBACK "WB"

// To track status of each stage
enum stageStatus_enum {
	stage_stalled,
	stage_noAction,
    stage_action,
};

typedef struct Register
{
	int reg[NUM_REGS];
    int value;          // contains register value
    bool is_writing;    // indicate that the register is current being written
	                    // True: register is not ready
						// False: register is ready
} Register;

typedef struct Stage
{
    int pc;
    int dest;
    int src1;
    int imm1;
    int imm2;
    int src1_value;
    int dest_value;
    bool dest_written;
    int opcode;
    int ld_flag;    // to check in memory if ld is there
    int imm_flag;    // to determine if its const instruction 
    int instruction_line;

    int addr;
    enum stageStatus_enum status;
} Stage;

/* Model of CPU */
typedef struct CPU
{
	/* Integer register file */
	Register *regs;
    int pc; // Program Counter
    
    int *instruction_memory;      // file parser instructions stored here
    char* instruction_line[100];      // for printing purpose
    char lines[150][50] ;

    int clock;   // to track clock cycles
    int *memory;    // Used to store memory map
    bool cpu_stalled;   
    bool cpu_halted;     // set this flag when ret is encountered
    int cpu_stalled_cnt;
    int tot_instructions_done;

    Stage fetch;
    Stage decode;
    Stage instruction_analyse;
    Stage register_read;
    Stage adder;
    Stage divider;
    Stage multipler;
    Stage branch;
    Stage memory_first;
    Stage memory_second;
    Stage writeback;
} CPU;

CPU* CPU_init(const char* filename);

Register* create_registers(int size);

int CPU_run(CPU* cpu);

void CPU_stop(CPU* cpu);

int get_instruction_count_in_input_file(FILE* file);

int * file_parser(const char *filename);

void instruction_analyse_stage(CPU* cpu);
void register_read_stage(CPU* cpu);
void adder_stage(CPU* cpu);
void divider_stage(CPU* cpu);
void multipler_stage(CPU* cpu);
void branch_stage(CPU* cpu);
void memory_first_stage(CPU* cpu);
void memory_second_stage(CPU* cpu);
bool writeback_stage(CPU* cpu);
void fetch_stage(CPU* cpu);
void decode_stage(CPU* cpu);

#endif
