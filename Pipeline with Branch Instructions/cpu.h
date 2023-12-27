#ifndef _CPU_H_
#define _CPU_H_
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define NUM_REGS 16
// #define MEM_SIZE 65536
#define MAX_CPU_CYCLES 1000000
#define DEBUG_PIPELINE 0  // Macro to enable pipeline debug messages
#define DEBUG_STATS 1   // For debugging stats

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
	fmt_st,       // opcode, src1, dest
    fmt_st_imm,   // opcode, imm1, dest 
    fmt_bez_imm,   // opcode, src1, imm1
    fmt_bgez_imm,  // opcode, src1, imm1
    fmt_blez_imm,  // opcode, src1, imm1
    fmt_bgtz_imm,  // opcode, src1, imm1
    fmt_bltz_imm,  // opcode, src1, imm1
    fmt_ret       // opcode
};

// Debug string macro for stages
#define FETCH "IF"
#define DECODE "ID"
#define INSTRUCTION_ANALYSER "IA"
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
	int last_reg_update_cycle;   // stores the recent cycle where this reg was written back
	int reg_in_process_cnt;   // stores total instructions with this reg in cycle
	int regNum; 
	bool has_value;  
} Register;


typedef struct Stage
{
    int pc;
    int dest;
    int src1;
	int src2;
    int imm1;
    int src1_value;
	int src2_value;
    int dest_value;
    bool dest_written;
    int opcode;
    int ld_flag;    // to check in memory if ld is there
    int imm_flag;    // to determine if its const instruction 
	int st_flag;   // to determine if the instruction is store instruction
    int br_flag;  // to determine if the instruction is branch instruction
    int instruction_line;
	bool ia_data_hazard_found;
    int read_st;    // to store read memory address during STORE instruction
    int write_st;    // to store write memory address during STORE instruction
    int addr;
    bool is_branch_instr;   // To check if it is a branch instruction
    int curr_pc;       // Stores program counter for current instruction in multiple of 7
    enum stageStatus_enum status;
} Stage;

// BTB table entry to store instruction tag and target instruction address
typedef struct BTBEntry{
    int tag;
    int target;
} BTBEntry;

// Prediction Table entry to store counter values used to predict branching
typedef struct PredTableEntry{
    int counter;
} PredTableEntry;

/* Model of CPU */
typedef struct CPU
{
	/* Integer register file */
	Register *regs;
	Register forward_regs[NUM_REGS];
    int pc; // Program Counter
    
    int *instruction_memory;      // file parser instructions stored here
    char* instruction_line[100];      // for printing purpose
    int tot_instructions;
    char lines[150][50] ;

    int clock;   // to track clock cycles
    int *memory;    // Used to store memory map
    int memoryLen;
    bool cpu_stalled;   
    bool cpu_halted;     // set this flag when ret is encountered
    bool cpu_read_stall;
    int cpu_stalled_cnt;
    int tot_instructions_done;
    bool ia_data_hazard_found;

    BTBEntry BTBTable[NUM_REGS];
    PredTableEntry PredTable[NUM_REGS];

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

int * file_parser(const char *filename, CPU* cpu);

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
void output_memory_map_file(CPU * cpu);


#endif
