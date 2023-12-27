#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cpu.h"

#define REG_COUNT 16

void print_pipeline(char *stage, char* stage_instruction)
{
    if(DEBUG_PIPELINE)
    {
        printf("%s \t\t: %s\n", stage, stage_instruction);
    }
}

void load_memory(const char* filename, CPU* cpu)
{
    FILE *file;
    int *arr, size=0, i;

    // Open file for reading
    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    while (fscanf(file, "%d", &i) == 1) {
        size++;
    }

    cpu->memoryLen = size;

    rewind(file);
    // Create dynamic memory for array
    arr = (int*) malloc(size * sizeof(int));

    // Read integers from file and store in array
    for (i = 0; i < size; i++) {
        fscanf(file, "%d", &arr[i]);
    }
    cpu->memory =  arr;

    // Close file
    fclose(file);    
}

void get_lines(const char* filename, CPU* cpu)
{
    FILE *file_ptr;
    //char lines[150][50];
    int line_count = 0;

    file_ptr = fopen(filename, "r");
    if (file_ptr == NULL) {
        printf("Error opening file input\n");
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
    load_memory("./memory_map.txt", cpu);
    cpu->instruction_memory = file_parser(filename, cpu);

    cpu->pc = 0;   
    cpu->clock = 1;
    cpu->tot_instructions_done = 0;


    // Destination register nums are initialized to -1 to avoid rewriting 
    cpu->fetch.status = stage_action;
    cpu->fetch.dest = -1;

    cpu->decode.status = stage_noAction;
    cpu->decode.dest = -1;

    cpu->instruction_analyse.status = stage_noAction;
    cpu->instruction_analyse.dest = -1;

    cpu->register_read.status = stage_noAction;
    cpu->register_read.dest = -1;

    cpu->adder.status = stage_noAction;
    cpu->adder.dest = -1;

    cpu->divider.status = stage_noAction;
    cpu->divider.dest = -1;

    cpu->multipler.status = stage_noAction;
    cpu->multipler.dest = -1;

    cpu->branch.status = stage_noAction;
    cpu->branch.dest = -1;

    cpu->memory_first.status = stage_noAction;
    cpu->memory_first.dest = -1;

    cpu->memory_second.status = stage_noAction;
    cpu->memory_second.dest = -1;

    cpu->writeback.status = stage_noAction;
    cpu->writeback.dest = -1;


    // initialize forward registers value to -1
    for(int i=0;i<NUM_REGS;i++)
    {
        cpu->forward_regs[i].value = -9999;
        cpu->forward_regs[i].regNum = i + 1;
        cpu->forward_regs[i].has_value = false;
    }

    // Initializing BTBtable and PredTable entry
    for(int i = 0;i < REG_COUNT;i++)
    {
        cpu->BTBTable[i].tag = -1;
        cpu->BTBTable[i].target = -1;
        cpu->PredTable[i].counter = 3;
    }

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
    for (int reg=0; reg<NUM_REGS; reg++) {
        printf("REG[%2d]   |   Value=%d  \n",reg,cpu->regs[reg].value);
        printf("--------------------------------\n");
    }
    printf("================================\n\n");
}

/*
 * This function prints the content of the btb table used for debugging.
 */
void print_btb_table(CPU *cpu){
    
    //printf("================================\n");
    printf("\n============ BTB =================================\n\n");
    for (int reg=0; reg<REG_COUNT; reg++) {
        printf("|	 BTB[%2d]	|	TAG=%d   |   TARGET=%d   |\n",reg,cpu->BTBTable[reg].tag,cpu->BTBTable[reg].target);
        //printf("--------------------------------\n");
    }
    //printf("================================\n\n");
}

/*
 * This function prints the content of the pred table used for debugging.
 */
void print_pred_table(CPU *cpu){
    
    //printf("================================\n");
    printf("\n============ Prediction Table  ==================\n\n");
    for (int reg=0; reg<REG_COUNT; reg++) {
        printf("|	 PT[%2d] |  Pattern=%d   |\n",reg,cpu->PredTable[reg].counter);
        //printf("--------------------------------\n");
    }
    printf("\n");

    //printf("================================\n\n");
}


/*
 *  CPU CPU simulation loop
 */
int CPU_run(CPU* cpu)
{
    for(int i=0; i < MAX_CPU_CYCLES;i++)
    {
        if(1)
        {
            printf("================================\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            //printf("--------------------------------\n");
        }
        
        bool done = writeback_stage(cpu);
    
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

        if(done)
        {
            print_registers(cpu);
            // print_btb_table(cpu);
            // print_pred_table(cpu);
            break;
        }

        //printf("=============== STATE OF ARCHITECTURAL REGISTER FILE ==========\n");

        print_registers(cpu);
        // print_btb_table(cpu);
        // print_pred_table(cpu);

        cpu->clock++;
    } 

    printf("================================\n");

    printf("\n");
    printf("=============== STATE OF ARCHITECTURAL REGISTER FILE ==========\n");
    printf("\n");

    print_registers(cpu);

    if(DEBUG_STATS)
    {
        printf("Stalled cycles due to data hazard: %d \n", cpu->cpu_stalled_cnt);
        printf("Total execution cycles: %d\n", cpu->clock);
        printf("Total instruction simulated: %d\n", cpu->tot_instructions_done);
        printf("IPC: %f\n", (float)cpu->tot_instructions_done/cpu->clock);
    }

    output_memory_map_file(cpu);

    return 0;
}

void output_memory_map_file(CPU * cpu)
{
    FILE *fout = fopen("memory_output.txt", "w");
    if (fout == NULL) {
        printf("Error: failed to open output file\n");
        return;
    }
    
    // Print array values to output file
    for (int i = 0; i < cpu->memoryLen; i++) {
        fprintf(fout, "%d ", cpu->memory[i]);
    }
    
    // Close output file
    fclose(fout);
}

Register*  create_registers(int size){
    Register* regs = malloc(sizeof(*regs) * size);
    if (!regs) {
        return NULL;
    }
    for (int i=0; i<size; i++){
        regs[i].value = 0;
        regs[i].is_writing = false;
        regs[i].last_reg_update_cycle = -1;
        regs[i].reg_in_process_cnt = 0;
    }
    return regs;
}

int get_tag_by_pc(int pc)
{
    return pc >> 6;
}

/*
 * This function fetches the instruction and halts the cpu when ret instruction is fetched.
 */
void fetch_stage(CPU* cpu)
{
    if(cpu->fetch.status == stage_action && !cpu->cpu_halted && cpu->pc<cpu->tot_instructions*7)
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
        cpu->fetch.src2 = cpu->instruction_memory[cpu->pc+4];
        cpu->fetch.imm_flag = cpu->instruction_memory[cpu->pc+5];
        
        cpu->fetch.is_branch_instr = false;
        cpu->fetch.st_flag = 0;
        cpu->fetch.ld_flag = 0;
        cpu->fetch.curr_pc = cpu->pc;    // Storing curr pc
        // if(cpu->fetch.imm_flag)
        // {
        //     cpu->fetch.src2 = -1;
        // }

        if(cpu->fetch.opcode == fmt_bez_imm || 
        cpu->fetch.opcode == fmt_bgez_imm || 
        cpu->fetch.opcode == fmt_bgtz_imm || 
        cpu->fetch.opcode == fmt_blez_imm ||
        cpu->fetch.opcode == fmt_bltz_imm)
            cpu->fetch.is_branch_instr = true;   // Tags the curr instruction as branch instruction

        if( cpu->fetch.opcode==fmt_ld_imm || cpu->fetch.opcode==fmt_ld) // Setting the flag in 
        //curr stage if load instruction in encountered
            cpu->fetch.ld_flag = 1;

        if( cpu->fetch.opcode==fmt_st_imm || cpu->fetch.opcode==fmt_st) // Setting the flag in 
        //curr stage if load instruction in encountered
            cpu->fetch.st_flag = 1;

        cpu->fetch.instruction_line = cpu->instruction_memory[cpu->pc+6];
        
        // Incrementing PC by 7 as for every instruction 
        // cpu->instruction_memory stores 7 decoded values
       

        // Halt the cpu when ret instruction is fetched
        // if(cpu->fetch.opcode == fmt_ret)
        // {
        //     if(!cpu->cpu_stalled && !cpu->cpu_read_stall)
        //         cpu->cpu_halted = true;
        //     //cpu->fetch.status = stage_noAction;
        // }

        if(!cpu->cpu_stalled && !cpu->cpu_read_stall)
        {
            //cpu->pc+=7; 
            cpu->decode = cpu->fetch;

            // If its a branch instruction we are checking if the instruction is present in
            // BTB table using TAG and based on its presence determining the program counter
            if(cpu->fetch.is_branch_instr)
            {
                unsigned int predicted_pc = 0;
                unsigned int index = ((cpu->pc/7 * 4) >> 2) % REG_COUNT;
                if (cpu->BTBTable[index].tag == get_tag_by_pc(cpu->fetch.curr_pc/7 * 4)) {
                    // We found in the BTB, use the predicted target address
                    predicted_pc = cpu->BTBTable[index].target;
                    // Checking prediction table for a counter value
                    unsigned int counter = cpu->PredTable[index].counter;

                    // Use the prediction to update the PC
                    if (counter >= 4) {
                        // We predict the branch will be taken
                        cpu->pc = cpu->BTBTable[index].target/4 * 7;
                    } else {
                        // We predict the branch will not be taken
                        cpu->pc = cpu->pc + 7;
                    }
                } else {
                    // We have a miss in the BTB, predict the next instruction address
                    cpu->pc = cpu->pc + 7;
                }
            }
            else
            {             
                cpu->pc = cpu->pc + 7;
            }

        }
        print_pipeline(FETCH, cpu->lines[cpu->fetch.instruction_line]);       
    }
    else
        cpu->fetch.status=stage_action;
}


/*
 * This function is for the next stage i.e decode and it passes the instruction from Decode to Instruction Analyse.
 */
void decode_stage(CPU* cpu)
{
    if(cpu->decode.status == stage_action)
    {
        if(!cpu->cpu_stalled && !cpu->cpu_read_stall)
        {
            cpu->instruction_analyse = cpu->decode;
            cpu->decode.status = stage_noAction;
        }

        print_pipeline(DECODE, cpu->lines[cpu->decode.instruction_line]);
    }
}

bool analyse_data_dependency(CPU* cpu)
{
    if(cpu->instruction_analyse.opcode == fmt_ld_imm)
        return false;

    int reg1_in_process_cnt = cpu->regs[cpu->instruction_analyse.src1].reg_in_process_cnt;
    int reg2_in_process_cnt = 0;
    int dest_in_process_cnt = cpu->regs[cpu->register_read.dest].reg_in_process_cnt;
    if(!cpu->instruction_analyse.imm_flag)
        reg2_in_process_cnt = cpu->regs[cpu->instruction_analyse.src2].reg_in_process_cnt;

    switch (cpu->instruction_analyse.opcode )
    {
    case fmt_ret:
        return false; 
        break;
    case fmt_set:
        return false;
        break;
    case (fmt_st):
        return reg1_in_process_cnt > 0 || dest_in_process_cnt>0;
        break;
    case (fmt_ld):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_st_imm):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_ld_imm):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_bez_imm):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_bgez_imm):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_blez_imm):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_bgtz_imm):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_bltz_imm):
        return reg1_in_process_cnt > 0;
        break;

    default:
        return reg1_in_process_cnt > 0 || reg2_in_process_cnt > 0;
        break;
    }

    return false;
}

bool analyse_rr_data_dependency(CPU* cpu)
{
    if(cpu->register_read.opcode == fmt_ld_imm)
        return false;

    int reg1_in_process_cnt = cpu->regs[cpu->register_read.src1].reg_in_process_cnt;
    int reg2_in_process_cnt = 0;
    int dest_in_process_cnt = cpu->regs[cpu->register_read.dest].reg_in_process_cnt;

    if(!cpu->register_read.imm_flag)
        reg2_in_process_cnt = cpu->regs[cpu->register_read.src2].reg_in_process_cnt;

    switch (cpu->register_read.opcode )
    {
    case fmt_ret:
        return false; 
        break;
    case fmt_set:
        return false;
        break;  
    case (fmt_st):
        return reg1_in_process_cnt > 0 || dest_in_process_cnt>0;
        break;
    case (fmt_ld):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_st_imm):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_ld_imm):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_bez_imm):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_bgez_imm):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_blez_imm):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_bgtz_imm):
        return reg1_in_process_cnt > 0;
        break;
    case (fmt_bltz_imm):
        return reg1_in_process_cnt > 0;
        break;
    default:
        break;
    }

    return reg1_in_process_cnt > 0 || reg2_in_process_cnt > 0;
}


/*
 Instruction Analyse stage
*/
void instruction_analyse_stage(CPU* cpu)
{
    if(cpu->instruction_analyse.status == stage_action)
    {
        cpu->instruction_analyse.ia_data_hazard_found = analyse_data_dependency(cpu);

        if(!cpu->cpu_stalled && !cpu->cpu_read_stall)
        {
            cpu->register_read = cpu->instruction_analyse;
            cpu->instruction_analyse.status = stage_noAction;
        }
        print_pipeline(INSTRUCTION_ANALYSER, cpu->lines[cpu->instruction_analyse.instruction_line]);
    }
}


bool can_read_reg_in_curr_cycle(CPU* cpu)
{
    int last_reg1_update_cycle = cpu->regs[cpu->register_read.src1].last_reg_update_cycle;
    int last_reg2_update_cycle = cpu->regs[cpu->register_read.src2].last_reg_update_cycle;

    int reg1_in_process_cnt = cpu->regs[cpu->register_read.src1].reg_in_process_cnt>0;
    int reg2_in_process_cnt = cpu->regs[cpu->register_read.src2].reg_in_process_cnt>0;

    int reg1 = cpu->register_read.src1;
    int reg2 = cpu->register_read.src2;

    if(cpu->register_read.opcode == fmt_st)
    {
        reg2 = cpu->register_read.dest;
        last_reg2_update_cycle = cpu->regs[cpu->register_read.dest].last_reg_update_cycle;
    }

    bool allow_read = !(last_reg1_update_cycle == cpu->clock);
    if(!allow_read)
    {
        return false;
    }
    
    if(cpu->register_read.imm_flag)
    {
        return true;
    }

    if(cpu->register_read.opcode ==fmt_st)
        allow_read =  !(last_reg2_update_cycle == cpu->clock);

    if(!cpu->register_read.imm_flag && cpu->register_read.opcode !=fmt_st)
        allow_read =  !(last_reg2_update_cycle == cpu->clock);

    return allow_read;
}

bool can_use_forwarding(CPU* cpu)
{
    if(cpu->register_read.opcode == fmt_ld_imm)
        return false;

    bool reg1_in_process = false;
    bool reg2_in_process = false;

    reg1_in_process = cpu->regs[cpu->register_read.src1].reg_in_process_cnt > 0;
    if(cpu->register_read.imm_flag==0)
        reg2_in_process = cpu->regs[cpu->register_read.src2].reg_in_process_cnt > 0;

    int reg1 = cpu->register_read.src1;
    int reg2 = cpu->register_read.src2;

    bool allow_forwarding = false;

    if(reg1_in_process)
    {
        if(cpu->forward_regs[reg1].has_value)
        {
            if(reg2_in_process)
            {
                if(cpu->forward_regs[reg2].has_value)
                {
                    allow_forwarding = true;   
                }
            }
            else
                allow_forwarding = true; 
        }
    }
    else
    {
        if(reg2_in_process)
        {
            if(cpu->forward_regs[reg2].has_value)
            {
                allow_forwarding = true;   
            }
        }
    }

    return allow_forwarding;
}



/*
 This function performs register read and checks for opcodes of cpu registers and stores those accordingly
*/
void register_read_stage(CPU* cpu)
{
    if(cpu->register_read.status == stage_action)
    {
        cpu->cpu_read_stall = false;

        print_pipeline(REGISTER_READ, cpu->lines[cpu->register_read.instruction_line]);

        if(cpu->register_read.ia_data_hazard_found)
        {
            cpu->register_read.ia_data_hazard_found = false;
            cpu->cpu_stalled = true;
        }

        cpu->cpu_stalled = analyse_rr_data_dependency(cpu);

        bool use_forward_values = can_use_forwarding(cpu);

        if(cpu->cpu_stalled && use_forward_values)
        {
            cpu->cpu_stalled = false;
        }
 
        if(!cpu->cpu_stalled && !can_read_reg_in_curr_cycle(cpu))
            cpu->cpu_read_stall = true;

        if(!cpu->cpu_stalled && (can_read_reg_in_curr_cycle(cpu) || (cpu->register_read.imm_flag && use_forward_values)))
        {
            cpu->cpu_read_stall = false;
            bool reg1 = false;
            bool reg2 = false;
            reg1 = cpu->regs[cpu->register_read.src1].reg_in_process_cnt>0;

            if(cpu->register_read.imm_flag==0)
                reg2 = cpu->regs[cpu->register_read.src2].reg_in_process_cnt>0;
            
            if(cpu->register_read.opcode == fmt_add || cpu->register_read.opcode == fmt_add_imm)
            {
                if(cpu->register_read.imm_flag==0)
                {
                    if(use_forward_values)
                    {
                        if(reg1)
                            cpu->register_read.src1_value = cpu->forward_regs[cpu->register_read.src1].value;
                        else
                            cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;

                        if(reg2)
                            cpu->register_read.src2_value = cpu->forward_regs[cpu->register_read.src2].value;
                        else
                            cpu->register_read.src2_value = cpu->regs[cpu->register_read.src2].value;
                    }
                    else
                    {
                        cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                        cpu->register_read.src2_value = cpu->regs[cpu->register_read.src2].value;
                    }
                    
                    cpu->regs[cpu->register_read.dest].is_writing = true;
                    cpu->regs[cpu->register_read.dest].reg_in_process_cnt++;
                }
                else
                {
                    if(use_forward_values)
                        cpu->register_read.src1_value = cpu->forward_regs[cpu->register_read.src1].value;
                    else
                        cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;

                    cpu->regs[cpu->register_read.dest].is_writing = true;
                    cpu->regs[cpu->register_read.dest].reg_in_process_cnt++;
                }
            }
            else if(cpu->register_read.opcode == fmt_div ||cpu->register_read.opcode == fmt_div_imm){
                if(cpu->register_read.imm_flag==0)
                {
                    if(use_forward_values)
                    {
                        if(reg1)
                            cpu->register_read.src1_value = cpu->forward_regs[cpu->register_read.src1].value;
                        else
                            cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;

                        if(reg2)
                            cpu->register_read.src2_value = cpu->forward_regs[cpu->register_read.src2].value;
                        else
                            cpu->register_read.src2_value = cpu->regs[cpu->register_read.src2].value;
                    }
                    else
                    {
                        cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                        cpu->register_read.src2_value = cpu->regs[cpu->register_read.src2].value;
                    }
                    cpu->regs[cpu->register_read.dest].is_writing = true;
                    cpu->regs[cpu->register_read.dest].reg_in_process_cnt++;                   
                }
                else
                {
                    if(use_forward_values)
                    {
                        cpu->register_read.src1_value = cpu->forward_regs[cpu->register_read.src1].value;
                    }
                    else
                    {
                        cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                    }
                    cpu->regs[cpu->register_read.dest].is_writing = true;
                    cpu->regs[cpu->register_read.dest].reg_in_process_cnt++;
                }
            }
            else if(cpu->register_read.opcode == fmt_ld || cpu->register_read.opcode == fmt_ld_imm){
                if(cpu->register_read.imm_flag==0)
                {
                    if(use_forward_values)
                    {
                        cpu->register_read.src1_value = cpu->forward_regs[cpu->register_read.src1].value;

                    }
                    else
                        cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                    cpu->regs[cpu->register_read.dest].is_writing = true;
                    cpu->regs[cpu->register_read.dest].reg_in_process_cnt++;
                }
                else
                {
                    cpu->regs[cpu->register_read.dest].is_writing = true;
                    cpu->regs[cpu->register_read.dest].reg_in_process_cnt++;
                }
            }
            else if(cpu->register_read.opcode == fmt_sub || cpu->register_read.opcode == fmt_sub_imm){
                if(cpu->register_read.imm_flag==0)
                {
                    if(use_forward_values)
                    {
                        if(reg1)
                            cpu->register_read.src1_value = cpu->forward_regs[cpu->register_read.src1].value;
                        else
                            cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;

                        if(reg2)
                            cpu->register_read.src2_value = cpu->forward_regs[cpu->register_read.src2].value;
                        else
                            cpu->register_read.src2_value = cpu->regs[cpu->register_read.src2].value;
                    }
                    else
                    {
                        cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                        cpu->register_read.src2_value = cpu->regs[cpu->register_read.src2].value;
                    }
                    cpu->regs[cpu->register_read.dest].is_writing = true;
                    cpu->regs[cpu->register_read.dest].reg_in_process_cnt++;
                }
                else
                {   
                    if(use_forward_values)
                    {
                        cpu->register_read.src1_value = cpu->forward_regs[cpu->register_read.src1].value;
                    }
                    else
                    {
                        cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                    }
                    cpu->regs[cpu->register_read.dest].is_writing = true;
                    cpu->regs[cpu->register_read.dest].reg_in_process_cnt++;
                }
            }
            else if(cpu->register_read.opcode == fmt_mul || cpu->register_read.opcode == fmt_mul_imm){
                if(cpu->register_read.imm_flag==0)
                {
                    if(use_forward_values)
                    {
                        if(reg1)
                            cpu->register_read.src1_value = cpu->forward_regs[cpu->register_read.src1].value;
                        else
                            cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;

                        if(reg2)
                            cpu->register_read.src2_value = cpu->forward_regs[cpu->register_read.src2].value;
                        else
                            cpu->register_read.src2_value = cpu->regs[cpu->register_read.src2].value;
                    }
                    else
                    {
                        cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                        cpu->register_read.src2_value = cpu->regs[cpu->register_read.src2].value;
                    }
                    cpu->regs[cpu->register_read.dest].is_writing = true;
                    cpu->regs[cpu->register_read.dest].reg_in_process_cnt++;
                }
                else
                {   if(use_forward_values)
                    {
                        cpu->register_read.src1_value = cpu->forward_regs[cpu->register_read.src1].value;
                    }
                    else
                    {
                        cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                    }
                    cpu->regs[cpu->register_read.dest].is_writing = true;
                    cpu->regs[cpu->register_read.dest].reg_in_process_cnt++;
                }
            }
            else if(cpu->register_read.opcode == fmt_st || cpu->register_read.opcode == fmt_st_imm)
            {
                if(use_forward_values)
                {
                    if(cpu->register_read.imm_flag==0)
                    {
                        reg2 = cpu->regs[cpu->register_read.dest].reg_in_process_cnt>0;
                        if(reg1)
                            cpu->register_read.src1_value = cpu->forward_regs[cpu->register_read.src1].value;
                        else
                            cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;

                        if(reg2)
                            cpu->register_read.dest_value = cpu->forward_regs[cpu->register_read.dest].value;
                        else
                            cpu->register_read.dest_value = cpu->regs[cpu->register_read.dest].value;

                    }
                    else
                    {
                        cpu->register_read.src1_value = cpu->forward_regs[cpu->register_read.src1].value;
                    }
                }
                else
                {
                    if(cpu->register_read.imm_flag==0)
                    {
                        cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                        cpu->register_read.dest_value = cpu->regs[cpu->register_read.dest].value;
                    }
                    else
                    {
                        cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                    }
                }

                // cpu->regs[cpu->register_read.dest].is_writing = true;
                // cpu->regs[cpu->register_read.dest].reg_in_process_cnt++;
                
            }
            else if(cpu->register_read.opcode == fmt_set){
                if(use_forward_values)
                {
                    cpu->register_read.src1_value = cpu->forward_regs[cpu->register_read.src1].value;

                }
                else
                    cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
                    
                cpu->regs[cpu->register_read.dest].is_writing = true;
                cpu->regs[cpu->register_read.dest].reg_in_process_cnt++;
                
            }
            else if(cpu->register_read.is_branch_instr){
                if(use_forward_values)
                {
                    cpu->register_read.src1_value = cpu->forward_regs[cpu->register_read.src1].value;

                }
                else
                    cpu->register_read.src1_value = cpu->regs[cpu->register_read.src1].value;
            }
              cpu->adder = cpu->register_read;
            cpu->register_read.status = stage_noAction;
        }
        else
            cpu->cpu_stalled_cnt++;
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
                cpu->adder.dest_value = cpu->adder.src1_value + cpu->adder.src2_value;
            }
            else{
                cpu->adder.dest_value = cpu->adder.imm1 + cpu->adder.src1_value;
            }
            cpu->forward_regs[cpu->adder.dest].has_value = true;
            cpu->forward_regs[cpu->adder.dest].value = cpu->adder.dest_value;
        }
        if(cpu->adder.opcode == fmt_sub || cpu->adder.opcode == fmt_sub_imm)
        {
            if(cpu->adder.imm_flag==0)
            {
                cpu->adder.dest_value = cpu->adder.src1_value - cpu->adder.src2_value;
            }
            else{
                cpu->adder.dest_value = cpu->adder.src1_value - cpu->adder.imm1;
            }
            cpu->forward_regs[cpu->adder.dest].has_value = true;
            cpu->forward_regs[cpu->adder.dest].value = cpu->adder.dest_value;
        }
        if(cpu->adder.opcode == fmt_set)
        {
            cpu->adder.dest_value = cpu->adder.imm1;
            cpu->forward_regs[cpu->adder.dest].has_value = true;
            cpu->forward_regs[cpu->adder.dest].value = cpu->adder.dest_value;
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
        // cpu->forward_regs[cpu->multipler.dest].has_value = false;
        // cpu->forward_regs[cpu->multipler.dest].value = false;

        if(cpu->multipler.opcode == fmt_mul || cpu->multipler.opcode == fmt_mul_imm)
        {
            if(cpu->multipler.imm_flag==0)
            {
                cpu->multipler.dest_value = cpu->multipler.src1_value * cpu->multipler.src2_value;
            }
            else{
                cpu->multipler.dest_value = cpu->multipler.imm1 * cpu->multipler.src1_value;
            }
            cpu->forward_regs[cpu->multipler.dest].has_value = true;
            cpu->forward_regs[cpu->multipler.dest].value = cpu->multipler.dest_value;
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
        // cpu->forward_regs[cpu->divider.dest].has_value = false;
        // cpu->forward_regs[cpu->divider.dest].value = false;

        if(cpu->divider.opcode == fmt_div || cpu->divider.opcode == fmt_div_imm)
        {
            if(cpu->divider.imm_flag==0)
            {
                cpu->divider.dest_value = (int)cpu->divider.src1_value /cpu->divider.src2_value;
            }
            else{
                cpu->divider.dest_value = (int)cpu->divider.src1_value / cpu->divider.imm1;
            }
            cpu->forward_regs[cpu->divider.dest].has_value = true;
            cpu->forward_regs[cpu->divider.dest].value = cpu->divider.dest_value;
           
        }
        
        cpu->branch = cpu->divider;
        cpu->divider.status = stage_noAction;
        print_pipeline(DIVIDER, cpu->lines[cpu->divider.instruction_line]);
    }
    

}

bool check_dest(Stage* cpu_stage)
{
    if(cpu_stage->opcode == fmt_ret)
        return false;
    if(cpu_stage->is_branch_instr)
        return false;
    return true;
}

void flush_pipeline(CPU *cpu)
{
    if(cpu->divider.status == stage_action)
        if(check_dest(&cpu->divider))
            cpu->regs[cpu->divider.dest].reg_in_process_cnt--;
    cpu->divider.status = stage_noAction;

    if(cpu->multipler.status == stage_action)
        if(check_dest(&cpu->multipler))
            cpu->regs[cpu->multipler.dest].reg_in_process_cnt--;
    cpu->multipler.status = stage_noAction;

    if(cpu->adder.status == stage_action)
        if(check_dest(&cpu->adder))
            cpu->regs[cpu->adder.dest].reg_in_process_cnt--;
    cpu->adder.status = stage_noAction;


    cpu->register_read.status = stage_noAction;
    cpu->instruction_analyse.status = stage_noAction;
    cpu->decode.status = stage_noAction;
    cpu->fetch.status = stage_noAction;

    cpu->cpu_stalled = false;
}


bool check_predicted_pc(CPU* cpu, int pc)
{
    if(cpu->divider.status == stage_action)
    {
        return cpu->divider.curr_pc == pc;
    }
    if(cpu->multipler.status == stage_action)
    {
        return cpu->multipler.curr_pc == pc;
    }
    if(cpu->adder.status == stage_action)
    {
        return cpu->adder.curr_pc == pc;
    }
    if(cpu->register_read.status == stage_action)
    {
        return cpu->register_read.curr_pc == pc;
    }
    return false;
}

/*
 this function passes the instruction from branch stage to mem1 
*/
void branch_stage(CPU* cpu)
{
    if(cpu->branch.status == stage_action)
    {
        // Wiping forwarding values in this stage
        if(!cpu->branch.is_branch_instr)
        {
            cpu->forward_regs[cpu->branch.dest].has_value = false;
            cpu->forward_regs[cpu->branch.dest].value = -9999;
        }

        // For a branch instruction we are checking if we have predicted the instruction
        // correctly and based on that incrementing the counter values
        // If instruction is not there in the BTB table a new entry is added
        if(cpu->branch.is_branch_instr)
        {
            bool result = false;

            //Check if the branch conditions are true
            if((cpu->branch.opcode == fmt_bez_imm && cpu->branch.src1_value == 0) ||
                (cpu->branch.opcode == fmt_bgez_imm && cpu->branch.src1_value >= 0) ||
                (cpu->branch.opcode == fmt_bgtz_imm && cpu->branch.src1_value > 0) ||
                (cpu->branch.opcode == fmt_blez_imm && cpu->branch.src1_value <= 0) ||
                (cpu->branch.opcode == fmt_bltz_imm && cpu->branch.src1_value < 0))
                result = true;

            int next_predicted_pc = cpu->branch.imm1;
            bool correctly_predicted = false;

            int curr_pc_addr = cpu->branch.curr_pc/7 * 4;
                
            int index = (curr_pc_addr >> 2) % REG_COUNT;
            // Check the tag if it is found and matching the curr branch instruction
            bool entry_found = cpu->BTBTable[index].tag == get_tag_by_pc(cpu->branch.curr_pc/7 * 4);

            // If PC is found in BTB, update prediction table
            if (entry_found) {
                correctly_predicted = check_predicted_pc(cpu, next_predicted_pc/4 * 7);
                if(result == true)
                {
                    if(correctly_predicted)
                    {
                        if(cpu->PredTable[index].counter<7)
                            cpu->PredTable[index].counter++;
                    }
                    else
                    {
                        flush_pipeline(cpu);
                        cpu->pc = cpu->branch.imm1/4 * 7;

                        if(cpu->PredTable[index].counter<7)
                            cpu->PredTable[index].counter++;
                    }
                }
                else
                {
                    if(!correctly_predicted)
                    {
                        if(cpu->PredTable[index].counter>0)
                            cpu->PredTable[index].counter--;
                    }
                    else
                    {
                        flush_pipeline(cpu);
                        cpu->pc = cpu->branch.curr_pc+7;

                        if(cpu->PredTable[index].counter>0)
                            cpu->PredTable[index].counter--;
                    }   
                }
            }
            else{
                cpu->BTBTable[index].tag = get_tag_by_pc(cpu->branch.curr_pc/7 * 4);
                cpu->BTBTable[index].target = cpu->branch.imm1;
                if(result)
                {
                    flush_pipeline(cpu);
                    cpu->pc = cpu->branch.imm1/4 * 7;

                    if(cpu->PredTable[index].counter<7)
                        cpu->PredTable[index].counter++;
                }
                else
                {
                    if(cpu->PredTable[index].counter>0)
                            cpu->PredTable[index].counter--;
                }
            }
        }

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
        if(cpu->memory_first.opcode == fmt_st || cpu->memory_first.opcode == fmt_st_imm)
        {
            if(cpu->memory_first.imm_flag==0 && cpu->memory_first.st_flag)
            {
                cpu->memory_first.read_st = cpu->memory_first.src1_value;
                cpu->memory_first.write_st = cpu->memory_first.dest_value;

            }
            else
            {
                cpu->memory_first.read_st = cpu->memory_first.src1_value;
                cpu->memory_first.write_st = cpu->memory_first.imm1;
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
                //cpu->cpu_stalled_cnt++;
            }
            else
            {
                cpu->memory_second.dest_value = cpu->memory[cpu->memory_second.addr / 4];;
                //cpu->cpu_stalled_cnt++;

                //In either case, set the CPU's stalled flag and increment the CPU stalled count
            }
        }

        if(cpu->memory_second.opcode == fmt_st ||cpu->memory_second.opcode == fmt_st_imm)
        {
            if(cpu->memory_second.imm_flag && cpu->memory_second.st_flag)
            {
                cpu->memory[cpu->memory_second.write_st / 4] = cpu->memory_second.read_st;
                //cpu->cpu_stalled_cnt++;
            }
            else
            {
                cpu->memory[cpu->memory_second.write_st / 4] = cpu->memory_second.read_st;
                //cpu->cpu_stalled_cnt++;

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
        cpu->cpu_read_stall = false;
        cpu->writeback.status = stage_noAction;
        
        if(cpu->writeback.opcode == fmt_ret)
        {
            //print_pipeline(WRITEBACK, cpu->lines[cpu->writeback.instruction_line]);
            cpu->tot_instructions_done++;
            cpu->writeback.dest_written = true;
            //cpu->cpu_halted = true;
                    print_pipeline(WRITEBACK, cpu->lines[cpu->writeback.instruction_line]);
            return true;
        }

        if(!cpu->writeback.dest_written
            && cpu->writeback.st_flag != 1
            && !cpu->writeback.is_branch_instr
        )
        {
            //to check if the destination register for the instruction has already been written to. If it has not, write the destination value (dest_value) of the writeback stage to the appropriate register in the CPU
            cpu->regs[cpu->writeback.dest].value = cpu->writeback.dest_value;
            cpu->regs[cpu->writeback.dest].has_value = true;
            cpu->regs[cpu->writeback.dest].reg_in_process_cnt--;
            cpu->regs[cpu->writeback.dest].last_reg_update_cycle = cpu->clock;
            //cpu->tot_instructions_done++;
            cpu->writeback.dest_written = true;

            if(cpu->writeback.dest == cpu->register_read.dest)
            {
                cpu->cpu_stalled = false;
            }

        }

        if(cpu->register_read.st_flag == 1)
            if(cpu->writeback.dest == cpu->register_read.src1)
            {
                cpu->cpu_stalled = false;
            }

        // //to check if the CPU is stalled and the instruction in the writeback stage is a load instruction. If both conditions are true, clear the CPU's stalled flag
        // if(cpu->cpu_stalled && cpu->writeback.ld_flag)
        // {
        //     cpu->cpu_stalled = false;
        // }
        cpu->tot_instructions_done++;

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
        } else if(strcmp(opcode, "st") == 0){
            return fmt_st_imm;
        } else if(strcmp(opcode, "bgez") == 0){
            return fmt_bgez_imm;
        } else if(strcmp(opcode, "bez") == 0){
            return fmt_bez_imm;
        } else if(strcmp(opcode, "blez") == 0){
            return fmt_blez_imm;
        } else if(strcmp(opcode, "bltz") == 0){
            return fmt_bltz_imm;
        } else if(strcmp(opcode, "bgtz") == 0){
            return fmt_bgtz_imm;
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
        } else if (strcmp(opcode, "st") == 0) {
            return fmt_st;
        } else if (strcmp(opcode, "ret") == 0) {
            return fmt_ret;
        }else {
            printf("Invalid opcode: %s\n", opcode);
            return -1;
        }
    }
    return -1;
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
int * file_parser(const char *filename, CPU* cpu)
{
    FILE* fp = fopen(filename, "r"); // open the file for reading

    int tot_instructions = get_instruction_count_in_input_file(fp);

    cpu->tot_instructions = tot_instructions;

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

        // instructions[inst_cnt] =  -1;
        // instructions[inst_cnt + 1] = -1;
        //  instructions[inst_cnt + 2] = -1;
        // //instructions[inst_cnt + 3] = -1;
        // instructions[inst_cnt + 4] = -1;   
        //instructions[inst_cnt + 5] = -1;  

        instructions[inst_cnt + 6] = inst_cnt/7;

    //     cpu->instruction_memory stores the instruction in followings format
        //     int opcode; -> current_instruction
        //     int dst; -> current_instruction + 1
        //     int src1; -> current_instruction + 2
        //     int imm1; -> current_instruction + 3
        //     int src2; -> current_instruction + 4
        //     int imm1_flag  -> current_instruction + 5
        //     int line -> current_instruction + 6
       
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
        if(strcmp(tokens[1], "st") == 0)
        {
            // st R1 #123
            if(strchr(tokens[3], '#'))
            {
                instructions[inst_cnt] =  mapOpcode(tokens[1], 1);
                //instructions[inst_cnt + 1] = -1;
                instructions[inst_cnt + 2] = atoi(tokens[2]+1);
                instructions[inst_cnt + 3] = atoi(tokens[3]+1);
                instructions[inst_cnt + 5] = 1;    
            }
            else{
                instructions[inst_cnt] =  mapOpcode(tokens[1], 0);
                instructions[inst_cnt + 2] = atoi(tokens[2]+1);
                instructions[inst_cnt + 1] = atoi(tokens[3]+1);
               
            }
        }
        if(strcmp(tokens[1], "bez")==0 || strcmp(tokens[1], "bgez")==0 || strcmp(tokens[1], "blez")==0 || strcmp(tokens[1], "bgtz")==0 || strcmp(tokens[1], "bltz")==0)       
        {
            instructions[inst_cnt] =  mapOpcode(tokens[1], 1);
            instructions[inst_cnt + 2] = atoi(tokens[2]+1);
            instructions[inst_cnt + 3] = atoi(tokens[3]+1);
            instructions[inst_cnt + 5] = 1;    
        }
        if(strcmp(tokens[1], "mul") == 0 || strcmp(tokens[1], "div") == 0 || strcmp(tokens[1], "sub") == 0 || strcmp(tokens[1], "add") == 0)
        {

            //strchr is similar to .contains 
            if(strchr(tokens[4], '#'))
            {
                instructions[inst_cnt] =  mapOpcode(tokens[1], 1);
                instructions[inst_cnt + 1] = atoi(tokens[2]+1);
                instructions[inst_cnt + 2] = atoi(tokens[3]+1);
                instructions[inst_cnt + 3] = atoi(tokens[4]+1);
                instructions[inst_cnt + 5] = 1;   // imm flag
            }
            else
            {
                instructions[inst_cnt] =  mapOpcode(tokens[1], 0);
                instructions[inst_cnt + 1] = atoi(tokens[2]+1);
                instructions[inst_cnt + 2] = atoi(tokens[3]+1);
                instructions[inst_cnt + 4] = atoi(tokens[4]+1);
            }
        }
        if(strcmp(tokens[1], "set") == 0)
        {
            instructions[inst_cnt] =  mapOpcode(tokens[1], 0);
            instructions[inst_cnt + 1] = atoi(tokens[2]+1);
            instructions[inst_cnt + 3] = atoi(tokens[3]+1);
        }
        // if(strcmp(tokens[1], "ret\n") == 0)
        // {
        if(strstr(tokens[1], "ret") != NULL)
        {
            char *op = "ret";
            instructions[inst_cnt] =  mapOpcode(op, 0);
        }

        inst_cnt+=7;

        //cpu->instruction_memory stores 7 decoded values 
        //The inst_cnt variable is incremented by 7 at the end of each iteration of the while loop, since each instruction is represented by 7 integers in the instructions array. Therefore, inst_cnt/7 gives the line number of the instruction in the input file.
        
    }

    fclose(fp); // close the file

    return instructions;
}

