/**************************************************************/
/* CS/COE 1541				 			
   just compile with gcc -o pipeline pipeline.c			
   and execute using							
   ./pipeline  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0  
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <arpa/inet.h> // change to original
#include "CPU.h"
#include "cache.h"

enum stage_type
{
    IF1 = 0,
    IF2,
    ID,
    EX,
    MEM1,
    MEM2,
    WB
};

enum hazard_type
{
    hz_STRUCT = 1,
    hz_DATA1,
    hz_DATA2,
    hz_CTRL
};

// Used as an empty stage type
unsigned char ti_EMPTY = 10;

struct trace_item push_stages_from(struct trace_item stages[], int pushFrom, struct trace_item out)
{
    out = stages[WB];
    //printf("%d\n", out.type);
    for (int i = WB; i > pushFrom; i--)
    {
        stages[i] = stages[i - 1]; // push non-stalled instrs down pipeline
    }
    stages[pushFrom].type = ti_NOP; // insert nop to stall pipeline
    return out;
}

int branch_check(struct trace_item stages[], int mode);

int hazard_detect(struct trace_item stages[], int prediction_mode, struct trace_item *out)
{
    int detected = 0;

    // check for control hazard (if hazard while branch/jump in EX, flush first 3 instrs)
    if (stages[EX].type == ti_BRANCH)
    {
        //printf("Control hazard\n");
        detected = branch_check(stages, prediction_mode);
    }

    // check for structural hazard (instr trying to WB while decoding)
    else if ((stages[WB].type >= ti_RTYPE) && (stages[WB].type <= ti_LOAD))
    {
        if (stages[ID].type > ti_NOP && stages[ID].type < ti_EMPTY)
        {
            //printf("struct hazard detected\n");
            *out = push_stages_from(stages, EX, *out);
            //printf("%d\n", out->type);
            detected = hz_STRUCT;
        }
    }

    // check for data hazard (LW in EX/MEM1 or MEM1/MEM2 w/ data dependency in ID/EX)
    else if (stages[EX].type == ti_LOAD)
    {   // which stage to check from depends on how we load the pipeline
        //FIX!!!!****************************************
        if (stages[EX].dReg == stages[ID].sReg_a || stages[EX].dReg == stages[ID].sReg_b)
        {
            //printf("data hazard detected\n");
            *out = push_stages_from(stages, EX, *out);
            detected = hz_DATA1;
        }
    }
    else if (stages[MEM1].type == ti_LOAD)
    {
        //FIX!!!!****************************************
        if (stages[EX].dReg == stages[ID].sReg_a || stages[EX].dReg == stages[ID].sReg_b)
        {
            //printf("data hazard detected\n");
            *out = push_stages_from(stages, EX, *out);
            detected = hz_DATA2;
        }
    }

    return detected;
}

/** BRANCH PREDICTION **/

enum branch_result_1bit
{
    br1_NOT_TAKEN = 0,
    br1_TAKEN
};

enum branch_result_2bit
{
    br2_NOT_TAKEN = 0,
    br2_NOT_TAKEN_FAIL,
    br2_TAKEN_FAIL,
    br2_TAKEN
};

static int *prediction_table;
// THIS MUST BE CHANGED BY A METHOD PARAMETER IN A FUNCTION THE MAIN METHOD
static int table_size = 64;

int flush_stages(struct trace_item stages[]);
int check_branch(struct trace_item stages[]);
int get_prediction(struct trace_item stages[], int mode);
int check_store_prediction(struct trace_item stages[], int branch_result, int mode);

void set_up_table(int size)
{
    table_size = size;
    prediction_table = (int *)malloc(sizeof(int) * size);
}

// predicts whether a branch will be taken, affecting what gets loaded into the pipeline
// 0 . not taken, 1 . taken
int branch_predict(struct trace_item stages[], int mode)
{
    if (stages[0].type == ti_BRANCH)
        return get_prediction(stages, mode);
    return 1;
}

// checks the prediction stored and flushes IF1/2 and ID
int branch_check(struct trace_item stages[], int mode)
{
    if (stages[3].type == ti_BRANCH)
    {
        int branch_result = check_branch(stages);
        if (!check_store_prediction(stages, branch_result, mode))
            return hz_CTRL;
    }
    return 0;
}

// Returns 1 if to be taken, 0 if not to be taken
int check_branch(struct trace_item stages[])
{
    int target_addr = stages[3].Addr;
    int next_addr = stages[2].PC;

    if (target_addr == next_addr)
        return -1;
    return 0;
}

//Hashes address for table
int hash_for_table(int addr)
{
    //int temp = addr >> 3;
    //return temp % table_size;
    return (addr >> 3) % table_size;
}

// Gets the prediction for this branch or stores a new one
int get_prediction(struct trace_item stages[], int mode)
{
    int addr;
    if (mode != 0)
        addr = hash_for_table(stages[0].PC);

    switch (mode)
    {
    case 0:
        return 0;
    case 1:
        return prediction_table[addr];
    case 2:
        return prediction_table[addr] > 1;
    }
    return 0;
}

// The prediction in order to determine flushing
// Returns -1 if prediction was correct, 0 if not
int check_store_prediction(struct trace_item stages[], int branch_result, int mode)
{
    int addr = hash_for_table(stages[0].PC);

    if (mode == 0)
    {
        return branch_result == 0;
    }
    if (mode == 1)
    {
        int prediction = prediction_table[addr];
        if (branch_result == prediction)
            return -1;
        else
        {
            prediction_table[addr] = branch_result;
            return 0;
        }
    }
    if (mode == 2)
    {
        int prediction = prediction_table[addr];
        int prediction_outcome = (prediction > 1);
        int predict_check = (branch_result == prediction_outcome);

        if (predict_check)
        {
            if (prediction_outcome)
                prediction_table[addr] = br2_TAKEN;
            else
                prediction_table[addr] = br2_NOT_TAKEN;
        }
        else
        {
            switch (prediction)
            {
            case br2_NOT_TAKEN:
                prediction_table[addr] = br2_NOT_TAKEN_FAIL;
                break;
            case br2_NOT_TAKEN_FAIL:
                prediction_table[addr] = br1_TAKEN;
                break;
            case br2_TAKEN_FAIL:
                prediction_table[addr] = br2_NOT_TAKEN;
                break;
            case br2_TAKEN:
                prediction_table[addr] = br2_TAKEN_FAIL;
                break;
            }
        }
        return predict_check;
    }
    else
        return branch_result == 0;
}

//***********************************************************************

int main(int argc, char **argv)
{
    const struct trace_item empty = {
        .type = ti_EMPTY,
        .sReg_a = 255,
        .sReg_b = 255,
        .dReg = 255,
        .PC = 0,
        .Addr = 0};
    struct trace_item stages[7] = {empty, empty, empty, empty, empty, empty, empty};

    struct cache_t *IL1_cache, *DL1_cache, *SL2_cache;

    struct trace_item *tr_entry;
    struct trace_item out;
    size_t size;
    char *trace_file_name;
    int trace_view_on = 0;
    int prediction_mode;

    unsigned char t_type = 0;
    unsigned char t_sReg_a = 0;
    unsigned char t_sReg_b = 0;
    unsigned char t_dReg = 0;
    unsigned int t_PC = 0;
    unsigned int t_Addr = 0;

    unsigned int cycle_number = 0;
    int delays = 0;

    if (argc == 1)
    {
        fprintf(stdout, "\nUSAGE: tv <trace_file> <prediction_mode> <switch - any character>\n");
        fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
        exit(0);
    }

    trace_file_name = argv[1];
    prediction_mode = atoi(argv[2]);
    if (argc == 4)
        trace_view_on = atoi(argv[3]);

    // here you should extract the cache parameters from the configuration file
    unsigned int I_size = 2;
    unsigned int I_assoc = 4;
    unsigned int D_size = 1;
    unsigned int D_assoc = 4;
    unsigned int S_size = 1;
    unsigned int S_assoc = 4;
    unsigned int bsize = 16;
    unsigned int S_miss_penalty = 1;
    unsigned int miss_penalty = 80;
    unsigned int latency;

    FILE *cf = fopen("cache_config.txt", "r");

    fscanf(cf, "%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d",
           &I_size, &I_assoc, &D_size, &D_assoc, &S_size, &S_assoc, &bsize, &S_miss_penalty, &miss_penalty);

    fclose(cf);

    IL1_cache = cache_create(I_size, bsize, I_assoc, 0);
    DL1_cache = cache_create(D_size, bsize, D_assoc, 0);
    SL2_cache = cache_create(D_size, bsize, D_assoc, S_miss_penalty);
    
    // THIS MUST BE CHANGED TO CHANGE THE SIZE OF THE PREDICTION TABLE
    set_up_table(128);

    fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

    trace_fd = fopen(trace_file_name, "rb");

    if (!trace_fd)
    {
        fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
        exit(0);
    }

    trace_init();

    while (1)
    {

        //printf("Cycle number: %d\n", cycle_number);
        /* hazard detection */
        int hazard_detected = hazard_detect(stages, prediction_mode, &out);
        //printf("hazard detection done\n");
        if (hazard_detected == hz_CTRL)
            delays = 3;
        /* branch prediction*/
        branch_predict(stages, prediction_mode);
        //printf("branch prediction done\n");
        if (delays > 0)
        {
            push_stages_from(stages, EX, out);
            delays--;
        }

        else if (hazard_detected == 0)
        {
            out = stages[WB];
            for (int i = WB; i > 0; i--)
            {
                stages[i] = stages[i - 1];
            }

            size = trace_get_item(&tr_entry);

            if (!size)
            {
                //printf("%d %d %d", size, stages[WB].type, out.type);
                if (stages[WB].type == ti_EMPTY && out.type == ti_EMPTY)
                {
                    printf("+ Simulation terminates at cycle : %u\n", cycle_number);
                    break;
                }
                stages[IF1].type = ti_EMPTY;
            }
            else
            {
                t_type = tr_entry->type;
                t_sReg_a = tr_entry->sReg_a;
                t_sReg_b = tr_entry->sReg_b;
                t_dReg = tr_entry->dReg;
                t_PC = tr_entry->PC;
                t_Addr = tr_entry->Addr;
                stages[IF1] = *tr_entry;
            }
        }
        cycle_number++;

        //print_stages(stages);

        if (trace_view_on && out.type != ti_EMPTY)
        { /* print the executed instruction if trace_view_on=1 */
            switch (out.type)
            {
            case ti_NOP:
                printf("[cycle %d] NOP\n:", cycle_number);
                break;
            case ti_RTYPE:
                printf("[cycle %d] RTYPE:", cycle_number);
                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", out.PC, out.sReg_a, out.sReg_b, out.dReg);
                break;
            case ti_ITYPE:
                printf("[cycle %d] ITYPE:", cycle_number);
                printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", out.PC, out.sReg_a, out.dReg, out.Addr);
                break;
            case ti_LOAD:
                printf("[cycle %d] LOAD:", cycle_number);
                printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", out.PC, out.sReg_a, out.dReg, out.Addr);
                break;
            case ti_STORE:
                printf("[cycle %d] STORE:", cycle_number);
                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", out.PC, out.sReg_a, out.sReg_b, out.Addr);
                break;
            case ti_BRANCH:
                printf("[cycle %d] BRANCH:", cycle_number);
                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", out.PC, out.sReg_a, out.sReg_b, out.Addr);
                break;
            case ti_JTYPE:
                printf("[cycle %d] JTYPE:", cycle_number);
                printf(" (PC: %x)(addr: %x)\n", out.PC, out.Addr);
                break;
            case ti_SPECIAL:
                printf("[cycle %d] SPECIAL:\n", cycle_number);
                break;
            case ti_JRTYPE:
                printf("[cycle %d] JRTYPE:", cycle_number);
                printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", out.PC, out.dReg, out.Addr);
                break;
            }
        }
    }

    trace_uninit();

    exit(0);
}