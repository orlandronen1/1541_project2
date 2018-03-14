/**************************************************************/
/* CS/COE 1541				 			
AThis program allows the user to input the instructions for a trace 
and produce a trace file with these instructions readable by CPU.c.
The program takes the name of the file to be generated as an argument.
***************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h" 

int main(int argc, char **argv)
{
  struct trace_item *tr_entry=malloc(sizeof(struct trace_item));
  size_t size;
  char *trace_file_name;
  int trace_view_on = 1;
  
  unsigned char t_type ;
  unsigned char t_sReg_a;
  unsigned char t_sReg_b;
  unsigned char t_dReg;
  unsigned int t_PC ;
  unsigned int t_Addr ;

  unsigned int cycle_number = 0;

  if (argc < 3) {
    fprintf(stdout, "\nMissing argument: the name of the file to be generated\n");
    exit(0);
  }
  trace_file_name = argv[1];

	int check;
int trcount, i, repeat;
char itype ;
int sReg_a, sReg_b, dReg;

trcount = atoi(argv[2]);

for (i = 0 ; i < trcount ; i++) {
  printf("Enter the fields for instruction %d (PC itype(R|L|S|B) sReg_a sReg_b dReg addr):\n", i);
  fflush(stdin);
  printf("PC: ");
  scanf("%x",  &tr_entry->PC);
  fflush(stdin);
  printf("itype(R|L|S|B): ");
  scanf(" %c",  &itype);
  printf("sReg_a: ");
  scanf("%d",  &sReg_a);
  tr_entry->sReg_a = sReg_a;
  printf("sReg_b: ");
  scanf("%d",  &sReg_b);
  tr_entry->sReg_b = sReg_b;
  printf("dReg: ");
  scanf("%d",  &dReg);
  tr_entry->dReg = dReg;
  printf("addr: ");
  scanf("%x",  &tr_entry->Addr);
  fflush(stdin);
  repeat = 0 ;
  if(itype == 'R') {tr_entry->type = ti_RTYPE ;} 
    else if (itype == 'L') {tr_entry->type = ti_LOAD;} 
    else if (itype == 'S') {tr_entry->type = ti_STORE;} 
    else if (itype == 'B') {tr_entry->type = ti_BRANCH ;} 
    else if (itype == 'N') {tr_entry->type = ti_NOP;}
    else {printf("unrecognized instruction type\n") ; repeat = 1;  i-- ; }
  if (repeat == 0) {
    write_trace(*tr_entry, trace_file_name);
  }
} 
trace_fd = fopen(trace_file_name, "rb");
trace_init();
  while(1) {
    size = trace_get_item(&tr_entry);
   
    if (!size) {       /* no more instructions (trace_items) to simulate */
      printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      break;
    }
    else{              /* parse the next instruction to simulate */
      cycle_number++;
      t_type = tr_entry->type;
      t_sReg_a = tr_entry->sReg_a;
      t_sReg_b = tr_entry->sReg_b;
      t_dReg = tr_entry->dReg;
      t_PC = tr_entry->PC;
      t_Addr = tr_entry->Addr;
    }  

// Display the generated trace 

    if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
      switch(tr_entry->type) {
        case ti_NOP:
          printf("[cycle %d] NOP:",cycle_number) ;
          break;
        case ti_RTYPE:
          printf("[cycle %d] RTYPE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->dReg);
          break;
        case ti_LOAD:
          printf("[cycle %d] LOAD:",cycle_number) ;      
          printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
          break;
        case ti_STORE:
          printf("[cycle %d] STORE:",cycle_number) ;      
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
          break;
        case ti_BRANCH:
          printf("[cycle %d] BRANCH:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
          break;
      }
    }
  }

  trace_uninit();

  exit(0);
}


