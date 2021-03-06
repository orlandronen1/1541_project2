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

int main(int argc, char **argv)
{
    const struct trace_item empty = {
        .type = ti_EMPTY,
        .sReg_a = 255,
        .sReg_b = 255,
        .dReg = 255,
        .PC = 0,
        .Addr = 0
    };
    
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