#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cpu.h"

#define REG_COUNT 128

void print_pipeline(char *stage, char* stage_instruction)
{
    if(DEBUG_PIPELINE)
    {
        printf("%s \t\t %s\n", stage, stage_instruction);
    }
}

int * load_memory(const char* filename)
{
    FILE *file;
    int *arr, size=100000, i;

    // Open file for reading
    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    // Allocate memory for array
    arr = (int*) malloc(size * sizeof(int));

    // Read integers from file and store in array
    for (i = 0; i < size; i++) {
        fscanf(file, "%d", &arr[i]);
    }

    // Close file
    fclose(file);

    // Print array
    // printf("Array contents:\n");
    // for (i = 0; i < size; i++) {
    //     printf("%d ", arr[i]);
    // }

    // Free memory
    return arr;
}

void get_lines(const char* filename, CPU* cpu)
{
    FILE *file_ptr;
    //char lines[150][50];
    int line_count = 0;

    file_ptr = fopen(filename, "r");
    if (file_ptr == NULL) {
        printf("Error opening file\n");
        exit(1);
    }

    while (fgets(cpu->lines[line_count], 50, file_ptr) != NULL) {
        cpu->lines[line_count][strcspn(cpu->lines[line_count], "\n")] = '\0'; // remove trailing newline
        line_count++;
        if (line_count >= 150) {
            printf("Maximum number of lines reached\n");
            break;
        }
    }

    fclose(file_ptr);

    // now the lines are stored in the `lines` array
    // for (int i = 0; i < line_count; i++) {
    //     printf("%s\n", cpu->lines[i]);
    // }
}

CPU* CPU_init(const char* filename)
{
     CPU* cpu = (CPU*)malloc(sizeof(CPU));

    if (!cpu) {
        return NULL;
    }

    /* Create register files */
    get_lines(filename, cpu);
    cpu->regs= create_registers(REG_COUNT);
    cpu->memory = load_memory("./memory_map.txt");
    cpu->instruction_memory = file_parser(filename);

    cpu->pc = 0;   
    cpu->clock = 1;
    cpu->tot_instructions_done = 0;

    cpu->fetch.status = stage_action;
    cpu->decode.status = stage_noAction;
    cpu->instruction_analyse.status = stage_noAction;
    cpu->register_read.status = stage_noAction;
    cpu->adder.status = stage_noAction;
    cpu->divider.status = stage_noAction;
    cpu->multipler.status = stage_noAction;
    cpu->branch.status = stage_noAction;
    cpu->memory_first.status = stage_noAction;
    cpu->memory_second.status = stage_noAction;
    cpu->writeback.status = stage_noAction;
    

    return cpu;
}

/*
 * This function de-allocates CPU cpu.
 */
void CPU_stop(CPU* cpu)
{
    free(cpu);
}

/*
 * This function prints the content of the registers.
 */
void print_registers(CPU *cpu){
    
    
    printf("--------------------------------\n");
    for (int reg=0; reg<REG_COUNT; reg++) {
        printf("REG[%2d]   |   Value=%d  \n",reg,cpu->regs[reg].value);
        printf("--------------------------------\n");
    }
    printf("================================\n\n");
}

/*
 *  CPU CPU simulation loop
 */
int CPU_run(CPU* cpu)
{
    for(int i=0; i < MAX_CPU_CYCLES;i++)
    {
        if(DEBUG_PIPELINE)
        {
            printf("================================\n");
            printf("Clock Cycle #: %d \n", cpu->clock);
            printf("--------------------------------\n");
        }
        
        if(writeback_stage(cpu))
            break;

        memory_second_stage(cpu);
        memory_first_stage(cpu);
        branch_stage(cpu);
        divider_stage(cpu);
        multipler_stage(cpu);
        adder_stage(cpu);
        register_read_stage(cpu);
        instruction_analyse_stage(cpu);
        decode_stage(cpu);
        fetch_stage(cpu);

        //printf("=============== STATE OF ARCHITECTURAL REGISTER FILE ==========\n");
        //print_registers(cpu);

        cpu->clock++;
    } 
  
    print_registers(cpu);

    if(DEBUG_STATS)
    {
        printf("Stalled cycles due to structural hazard: %d \n", cpu->cpu_stalled_cnt);
        printf("Total execution cycles: %d \n", cpu->clock);
        printf("Total instruction simulated: %d\n", cpu->tot_instructions_done);
        printf("IPC: %f \n", (float)cpu->tot_instructions_done/cpu->clock);
    }
   
    return 0;
}

Register*  create_registers(int size){
    Register* regs = malloc(sizeof(*regs) * size);
    if (!regs) {
        return NULL;
    }
    for (int i=0; i<size; i++){
        regs[i].value = 0;
        regs[i].is_writing = false;
    }
    return regs;
}


/*
 * This function fetches the instruction and halts the cpu when ret instruction is fetched.
 */
void fetch_stage(CPU* cpu)
{
    if(cpu->fetch.status == stage_action && !cpu->cpu_stalled && !cpu->cpu_halted)
    {

        //     cpu->instruction_memory stores the instruction in followings format
        //     int opcode; -> current_instruction
        //     int dst; -> current_instruction + 1
        //     int src1; -> current_instruction + 2
        //     int imm1; -> current_instruction + 3
        //     int imm2; -> current_instruction + 4
        //     int imm1_flag  -> current_instruction + 5
        //     int line -> current_instruction + 6
        cpu->fetch.opcode = cpu->instruction_memory[cpu->pc];
        cpu->fetch.dest = cpu->instruction_memory[cpu->pc+1];
        cpu->fetch.src1 = cpu->instruction_memory[cpu->pc+2];
        cpu->fetch.imm1 = cpu->instruction_memory[cpu->pc+3];
        cpu->fetch.imm2 = cpu->instruction_memory[cpu->pc+4];
        cpu->fetch.imm_flag = cpu->instruction_memory[cpu->pc+5];

        if( cpu->fetch.opcode==fmt_ld_imm) // Setting the flag in 
        //curr stage if load instruction in encountered
            cpu->fetch.ld_flag = 1;

        cpu->fetch.instruction_line = cpu->instruction_memory[cpu->pc+6];
        
        // Incrementing PC by 7 as for every instruction 
        // cpu->instruction_memory stores 7 decoded values 
        cpu->pc+=7; 

        // Halt the cpu when ret instruction is fetched
        if(cpu->fetch.opcode == fmt_ret)
        {
            cpu->cpu_halted = true;
            //cpu->fetch.status = stage_noAction;
        }

        cpu->decode = cpu->fetch;

        print_pipeline(FETCH, cpu->lines[cpu->fetch.instruction_line]);       
    }
}


/*
 * This function is for the next stage i.e decode and it passes the instruction from Decode to Instruction Analyse.
 */
void decode_stage(CPU* cpu)
{
    if(cpu->decode.status == stage_action)
    {
        cpu->instruction_analyse = cpu->decode;
        cpu->decode.status = stage_noAction;

        print_pipeline(DECODE, cpu->lines[cpu->decode.instruction_line]);
    }
}


/*
 Instruction Analyse stage
*/
void instruction_analyse_stage(CPU* cpu)
{
    if(cpu->instruction_analyse.status == stage_action)
    {
        cpu->register_read = cpu->instruction_analyse;
        cpu->instruction_analyse.status = stage_noAction;
        print_pipeline(INSTRUCTION_ANALYSER, cpu->lines[cpu->instruction_analyse.instruction_line]);
    }
}


/*
 This function performs register read and checks for opcodes of cpu registers and stores those accordingly
*/
void register_read_stage(CPU* cpu)
{
    if(cpu->register_read.status == stage_action)
    {
        if(cpu->register_read.imm_flag==0)
        {
            if(cpu->register_read.opcode == fmt_add || cpu->register_read.opcode == fmt_add_imm)
            {
                if(cpu->register_read.imm_flag==0)
                {
                    cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                }
            }
            else if(cpu->register_read.opcode == fmt_div ||cpu->register_read.opcode == fmt_div_imm){
                if(cpu->register_read.imm_flag==0)
                {
                    cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                }
            }
            else if(cpu->register_read.opcode == fmt_ld || cpu->register_read.opcode == fmt_ld_imm){
                if(cpu->register_read.imm_flag==0)
                {
                    cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                }
            }
            else if(cpu->register_read.opcode == fmt_sub || cpu->register_read.opcode == fmt_sub_imm){
                if(cpu->register_read.imm_flag==0)
                {
                    cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                }
            }
            else{
                if(cpu->register_read.imm_flag==0)
                {
                    cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                }
            }
        }
        cpu->adder = cpu->register_read;
        cpu->register_read.status = stage_noAction;
        print_pipeline(REGISTER_READ, cpu->lines[cpu->register_read.instruction_line]);
    }
}


/*
 This function performs operations for add/add_imm, sub/sub_imm and set instructions
*/
void adder_stage(CPU* cpu)
{
    if(cpu->adder.status == stage_action)
    {
        if(cpu->adder.opcode == fmt_add || cpu->adder.opcode == fmt_add_imm)
        {
            if(cpu->adder.imm_flag==0)
            {
                cpu->adder.dest_value = cpu->adder.src1_value + cpu->adder.imm1;
            }
            else{
                cpu->adder.dest_value = cpu->adder.imm1 + cpu->adder.imm2;
            }
        }
        if(cpu->adder.opcode == fmt_sub || cpu->adder.opcode == fmt_sub_imm)
        {
            if(cpu->adder.imm_flag==0)
            {
                cpu->adder.dest_value = cpu->adder.src1_value- cpu->adder.imm1;
            }
            else{
                cpu->adder.dest_value = cpu->adder.imm1 - cpu->adder.imm2;
            }
        }
        if(cpu->adder.opcode == fmt_set)
        {
            cpu->adder.dest_value = cpu->adder.imm1;
        }
        cpu->multipler = cpu->adder;
        cpu->adder.status = stage_noAction;
        print_pipeline(ADDER, cpu->lines[cpu->adder.instruction_line]);
    }
}

/*
 This function performs operations for mul/mul_imm instructions
*/
void multipler_stage(CPU* cpu)
{
    if(cpu->multipler.status == stage_action)
    {
        if(cpu->multipler.opcode == fmt_mul || cpu->multipler.opcode == fmt_mul_imm)
        {
            if(cpu->multipler.imm_flag==0)
            {
                cpu->multipler.dest_value = cpu->multipler.imm1 * cpu->multipler.src1_value;
            }
            else{
                cpu->multipler.dest_value = cpu->multipler.imm1 * cpu->multipler.imm2;
            }
        }

        cpu->divider = cpu->multipler;
        cpu->multipler.status = stage_noAction;
        print_pipeline(MULTIPLIER, cpu->lines[cpu->multipler.instruction_line]);
    }
}

/*
 This function performs operations for div/div_imm instructions
*/
void divider_stage(CPU* cpu)
{
    if(cpu->divider.status == stage_action)
    {
        if(cpu->divider.opcode == fmt_div || cpu->divider.opcode == fmt_div_imm)
        {
            if(cpu->divider.imm_flag==0)
            {
                cpu->divider.dest_value = (int)cpu->divider.src1_value /cpu->divider.imm1;
            }
            else{
                cpu->divider.dest_value = (int)cpu->divider.imm1 / cpu->divider.imm2;
            }
        }
      
        cpu->branch = cpu->divider;
        cpu->divider.status = stage_noAction;
        print_pipeline(DIVIDER, cpu->lines[cpu->divider.instruction_line]);
    }
    

}

/*
 this function passes the instruction from branch stage to mem1 
*/
void branch_stage(CPU* cpu)
{
    if(cpu->branch.status == stage_action)
    {
        cpu->memory_first = cpu->branch;
        cpu->branch.status = stage_noAction;
        print_pipeline(BRANCH, cpu->lines[cpu->branch.instruction_line]);
    }
}


/*
This function handles memory access for an instruction that has reached the "memory" stage of the CPU pipeline
*/
void memory_first_stage(CPU* cpu)
{
    if(cpu->memory_first.status == stage_action)
    {
        /*
         If the instruction is a load instruction, check if it is a register-to-memory (ld_flag=1) instruction or an immediate-to-memory (imm_flag=1) instruction. If it is a register-to-memory instruction, set the memory address to the value in the source register (src1_value), otherwise set the memory address to the immediate value (imm1)
        */
        if(cpu->memory_first.opcode == fmt_ld || cpu->memory_first.opcode == fmt_ld_imm)
        {
            if(cpu->memory_first.imm_flag==0 && cpu->memory_first.ld_flag)
            {
                cpu->memory_first.addr = cpu->memory_first.src1_value;
            }
            else
            {
                cpu->memory_first.addr = cpu->memory_first.imm1;
            }
        }

        /*
        Copy the first memory stage to the second memory stage and set the status of the first memory stage to no action.
        */
        cpu->memory_second = cpu->memory_first;
        cpu->memory_first.status = stage_noAction;
        print_pipeline(MEMORY_FIRST, cpu->lines[cpu->memory_first.instruction_line]);
    }   
}



void memory_second_stage(CPU* cpu)
{
    if(cpu->memory_second.status == stage_action)
    {
        if(cpu->memory_second.opcode == fmt_ld ||cpu->memory_second.opcode == fmt_ld_imm)
        {
            if(cpu->memory_second.imm_flag && cpu->memory_second.ld_flag)
            {
                cpu->memory_second.dest_value = cpu->memory[cpu->memory_second.addr / 4];
                cpu->cpu_stalled = true;
                cpu->cpu_stalled_cnt++;
            }
            else
            {
                cpu->memory_second.dest_value = cpu->memory[cpu->memory_second.addr / 4];;
                cpu->cpu_stalled = true;
                cpu->cpu_stalled_cnt++;

                //In either case, set the CPU's stalled flag and increment the CPU stalled count
            }
        }

        cpu->writeback = cpu->memory_second;
        cpu->memory_second.status = stage_noAction;
        print_pipeline(MEMORY_SECOND, cpu->lines[cpu->memory_second.instruction_line]);
    }
}

bool writeback_stage(CPU* cpu)
{
    if(cpu->writeback.status == stage_action)
    {
        if(cpu->writeback.opcode == fmt_ret)
        {
            print_pipeline(WRITEBACK, cpu->lines[cpu->writeback.instruction_line]);
            cpu->tot_instructions_done++;
            cpu->writeback.dest_written = true;
            cpu->cpu_halted = true;
            return true;
        }


        if(!cpu->writeback.dest_written)
        {
            //to check if the destination register for the instruction has already been written to. If it has not, write the destination value (dest_value) of the writeback stage to the appropriate register in the CPU
            cpu->regs[cpu->writeback.dest].value = cpu->writeback.dest_value;
            cpu->tot_instructions_done++;
            cpu->writeback.dest_written = true;
        }

        //to check if the CPU is stalled and the instruction in the writeback stage is a load instruction. If both conditions are true, clear the CPU's stalled flag
        if(cpu->cpu_stalled && cpu->writeback.ld_flag)
        {
            cpu->cpu_stalled = false;
        }

        print_pipeline(WRITEBACK, cpu->lines[cpu->writeback.instruction_line]);
    }

    return false;
}

// Function to map opcode names to their corresponding enum values
enum opcodeFmt_enum mapOpcode(char *opcode, int imm_flag) 
{
    if(imm_flag)
    {
        // handle immediate instructions
        if(strcmp(opcode, "add") == 0){
            return fmt_add_imm;
        } else if(strcmp(opcode, "div") == 0){
            return fmt_div_imm;
        } else if(strcmp(opcode, "mul") == 0){
            return fmt_mul_imm;
        } else if(strcmp(opcode, "ld") == 0){
            return fmt_ld_imm;
        } else if(strcmp(opcode, "sub") == 0){
            return fmt_sub_imm;
        }
    }
    else
    {
        // check opcode and return format
        if (strcmp(opcode, "set") == 0) {
            return fmt_set;
        } else if (strcmp(opcode, "add") == 0) {
            return fmt_add;
        } else if (strcmp(opcode, "sub") == 0) {
            return fmt_sub;
        } else if (strcmp(opcode, "div") == 0) {
            return fmt_div;
        } else if (strcmp(opcode, "mul") == 0) {
            return fmt_mul;
        } else if (strcmp(opcode, "ld") == 0) {
            return fmt_ld;
        } else if (strcmp(opcode, "ret\n") == 0) {
            return fmt_ret;
        }else {
            printf("Invalid opcode: %s\n", opcode);
            return -1;
        }
    }
}


int get_instruction_count_in_input_file(FILE* file)
{
    int line_count = 0;
    char line[1000];

    while (fgets(line, sizeof(line), file)) { // read each line of the file into the buffer
        line_count++; // increment the line counter variable
    }

 //   printf("The file contains %d lines\n", line_count);
    return line_count;
}



/*
The below function is opening a file for reading, getting the total number of instructions in the file, allocating memory for an array to store the instructions, and then parsing each line of the file to extract the opcode and operands for each instruction
*/
int * file_parser(const char *filename)
{
    FILE* fp = fopen(filename, "r"); // open the file for reading

    int tot_instructions = get_instruction_count_in_input_file(fp);

    //rewind function sets the file position to the beginning of the file
    rewind(fp);

    char line[MAX_LINE_SIZE];
    char* tokens[MAX_TOKENS];

    int *instructions;
    int inst_cnt = 0;

    instructions = (int *) malloc(tot_instructions * 7 * sizeof(int));
    
    while (fgets(line, MAX_LINE_SIZE, fp)) { // read each line of the file into the buffer
        int i = 0;
        char* token = strtok(line, " "); // split the line into tokens based on spaces

        while (token != NULL && i < MAX_TOKENS) { // iterate through each token
            tokens[i] = token;
            i++;
            token = strtok(NULL, " ");
        }

        // print the tokens as space-separated values
        // for (int j = 0; j < i; j++) {
        //     printf("%s ", tokens[j]);
        // }
        // printf("\n");

        instructions[inst_cnt + 6] = inst_cnt/7;

       
        if(strcmp(tokens[1], "ld") == 0)
        {
            if(strchr(tokens[3], '#'))
            {
                instructions[inst_cnt] =  mapOpcode(tokens[1], 1);
                instructions[inst_cnt + 1] = atoi(tokens[2]+1);
                instructions[inst_cnt + 3] = atoi(tokens[3]+1);
                instructions[inst_cnt + 5] = 1;
            }
            else{
                instructions[inst_cnt] =  mapOpcode(tokens[1], 0);
                instructions[inst_cnt + 1] = atoi(tokens[2]+1);
                instructions[inst_cnt + 2] = atoi(tokens[3]+1);
               
            }
        }
        if(strcmp(tokens[1], "mul") == 0 || strcmp(tokens[1], "div") == 0 || strcmp(tokens[1], "sub") == 0 || strcmp(tokens[1], "add") == 0)
        {

            //strchr is similar to .contains 
            if(strchr(tokens[3], '#') && strchr(tokens[4], '#'))
            {
                instructions[inst_cnt] =  mapOpcode(tokens[1], 1);
                instructions[inst_cnt + 1] = atoi(tokens[2]+1);
                instructions[inst_cnt + 3] = atoi(tokens[3]+1);
                instructions[inst_cnt + 4] = atoi(tokens[4]+1);
                instructions[inst_cnt + 5] = 1;
            }
            else
            {
                instructions[inst_cnt] =  mapOpcode(tokens[1], 0);
                instructions[inst_cnt + 1] = atoi(tokens[2]+1);
                instructions[inst_cnt + 2] = atoi(tokens[3]+1);
                instructions[inst_cnt + 3] = atoi(tokens[4]+1);
            }
        }
        if(strcmp(tokens[1], "set") == 0)
        {
            instructions[inst_cnt] =  mapOpcode(tokens[1], 0);
            instructions[inst_cnt + 1] = atoi(tokens[2]+1);
            instructions[inst_cnt + 3] = atoi(tokens[3]+1);
        }
        if(strcmp(tokens[1], "ret\n") == 0)
        {
            instructions[inst_cnt] =  mapOpcode(tokens[1], 0);
        }

        inst_cnt+=7;

        //cpu->instruction_memory stores 7 decoded values 
        //The inst_cnt variable is incremented by 7 at the end of each iteration of the while loop, since each instruction is represented by 7 integers in the instructions array. Therefore, inst_cnt/7 gives the line number of the instruction in the input file.
        
    }

    fclose(fp); // close the file

    return instructions;
}

