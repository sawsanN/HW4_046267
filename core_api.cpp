/* 046267 Computer Architecture - Spring 2021 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <vector>
#include <iostream>

#define FG 0
#define BLOCKED 1

tcontext* FG_context;
tcontext* Blocked_context;

int* NumOfCyclesBlocked;
int* NumOfCyclesFG;
int* NumOfInstBlocked;
int* NumOfInstFg;
int* Threads_Num;
class Thread{

	public:
		bool halted;
		int id;
		int pc;
		int cyclesLeft;
		Thread():
			halted(false),id(-1),pc(-1),cyclesLeft(-1){}
		Thread(int tid):
			halted(false),id(tid),pc(0),cyclesLeft(-1){
				regFile_BLOCKED.resize(REGS_COUNT);
				regFile_FG.resize(REGS_COUNT);
			}
		void Execute(int cycles , std::vector<int>& regFile);

		bool IsAccessingMemory(){ return cyclesLeft>0;}

		std::vector<int> regFile_BLOCKED;
		std::vector<int> regFile_FG;

		
};


class ProgramManager{

	public:
		std::vector<Thread> Threads;
		int NumOfCycles_Blocked;
		int NumOfCycles_FG;
		int ThreadsNum;
		int NumActive;
		int NumOfInst_blocked;
		int NumOfInst_fg;

		ProgramManager():
			NumOfCycles_Blocked(0),NumOfCycles_FG(0),NumOfInst_blocked(0),NumOfInst_fg(0){

			ThreadsNum = SIM_GetThreadsNum();
			//Threads.resize(ThreadsNum);
			for(int i=0 ; i<ThreadsNum ;i++){
				Thread t(i);
				Threads.push_back(t);
			}
			NumActive = ThreadsNum;
		}
		void init(){
			NumOfCycles_Blocked = 0;
			NumOfCycles_FG = 0;
			NumOfInst_blocked = 0;
			NumOfInst_fg = 0;
			ThreadsNum = SIM_GetThreadsNum();
			Threads.resize(ThreadsNum);
			for(int i=0 ; i<ThreadsNum ;i++){
				Thread t = Thread(i);
				Threads.push_back(t);
			}
			NumActive = ThreadsNum;			
		}
		int GetNextActive(int tid){
			int id = tid + 1;
			if(id == ThreadsNum)
				id = 0;
			while(id != tid){
				if(!Threads[id].halted)
					return id;
				id++;
				if(id == ThreadsNum)
					id = 0;
			}

			return tid;


		}
		void ExecuteThreads_BLOCKED(){ 

			int tid = 0;
			int CtrActive = NumActive;
			while(CtrActive){
				std::cout<<"id is "<<tid<<std::endl;
				Threads[tid].Execute(1 , Threads[tid].regFile_BLOCKED);
				NumOfCycles_Blocked++;

				if(Threads[tid].halted){
					NumOfInst_blocked++;
					CtrActive--;
				} 

				if(Threads[tid].IsAccessingMemory() || Threads[tid].halted ){
					NumOfCycles_Blocked += SIM_GetSwitchCycles();
					tid = GetNextActive(tid);

				}
				else
					NumOfInst_blocked++;
			}
			
		}

		void ExecuteThreads_FineGrained(){ 

			int tid = 0;


			int CtrActive = NumActive;
			while(CtrActive){

				Threads[tid].Execute(1,Threads[tid].regFile_FG);
				NumOfCycles_FG++;

				if(!Threads[tid].IsAccessingMemory()) NumOfCycles_FG++;
				if(Threads[tid].halted){
					CtrActive--;
				} 

				tid = GetNextActive(tid);
			}
			
		}



};

void Thread::Execute(int cycles , std::vector<int>& regFile){

	int cyclesCtr = cycles;

	Instruction Inst;

	while(cyclesCtr){
		//std::cout<<"in exec pc is"<<pc<<std::endl;
		SIM_MemInstRead(pc,&Inst ,id );
		//std::cout<<"in exec"<<std::endl;
		if(Inst.opcode == CMD_ADD){
			regFile[Inst.dst_index] = regFile[Inst.src1_index] + regFile[Inst.src2_index_imm];
			pc+=1;
			
		}
		if(Inst.opcode == CMD_SUB){
			regFile[Inst.dst_index] = regFile[Inst.src1_index] - regFile[Inst.src2_index_imm];
			pc+=1;
			
		}
		if(Inst.opcode == CMD_ADDI){
			regFile[Inst.dst_index] = regFile[Inst.src1_index] + Inst.src2_index_imm;
			pc+=1;
			
		}
		if(Inst.opcode == CMD_SUBI){
			regFile[Inst.dst_index] = regFile[Inst.src1_index] - Inst.src2_index_imm;
			pc+=1;
			
		}
		if(Inst.opcode == CMD_LOAD){
			int adrs;
			if(!Inst.isSrc2Imm)
				adrs = regFile[Inst.src1_index] + regFile[Inst.src2_index_imm];
			else
				adrs = regFile[Inst.src1_index] + Inst.src2_index_imm;
			SIM_MemDataRead(adrs, &(regFile[Inst.dst_index]));
			if(cyclesLeft <0){
				cyclesLeft = SIM_GetLoadLat() - 1;
			}
			else{
				cyclesLeft --;
				if(cyclesLeft == 0){
					pc+=1;
					cyclesLeft = -1;
				}

			}
		}

		if(Inst.opcode == CMD_STORE){
			int adrs;
			if(!Inst.isSrc2Imm)
				adrs = Inst.dst_index + regFile[Inst.src2_index_imm];
			else
				adrs = Inst.dst_index + Inst.src2_index_imm;
			SIM_MemDataWrite(adrs,Inst.src1_index);

			if(cyclesLeft <0){
				cyclesLeft = SIM_GetStoreLat() - 1;
			}
			else{
				cyclesLeft --;
				if(cyclesLeft == 0){
					pc+=1;
					cyclesLeft = -1;
				}

			}

		}
		if(Inst.opcode == CMD_HALT){
			halted = true;
			break;
		}
		if(Inst.opcode == CMD_NOP){
			break;
		}

		cyclesCtr--;
	}



}



void CORE_BlockedMT() {
	//OurProg.init();
	ProgramManager OurProg = ProgramManager();
	//std::cout<<"hi "<<OurProg.Threads[0].id<<std::endl;
	OurProg.ExecuteThreads_BLOCKED();
	//std::cout<<"it is"<<OurProg.Threads.size()<<std::endl;
	Blocked_context = new tcontext[OurProg.ThreadsNum];
	NumOfCyclesBlocked = new int ;
	NumOfInstBlocked= new int;
	Threads_Num = new int ;
	*Threads_Num = OurProg.ThreadsNum;
	*NumOfCyclesBlocked = OurProg.NumOfCycles_Blocked;
	*NumOfInstBlocked = OurProg.NumOfInst_blocked;

	for(int i=0 ; i<OurProg.ThreadsNum; i++){
		for(int j=0 ; j<REGS_COUNT;j++)
			Blocked_context[i].reg[j] = OurProg.Threads[i].regFile_BLOCKED[j];
	}
		

}

void CORE_FinegrainedMT() {
	ProgramManager OurProg = ProgramManager();
	//std::cout<<"hi "<<OurProg.Threads[0].id<<std::endl;
	OurProg.ExecuteThreads_FineGrained();
	//std::cout<<"it is"<<OurProg.Threads.size()<<std::endl;
	FG_context = new tcontext[OurProg.ThreadsNum];
	NumOfCyclesFG = new int ;
	NumOfInstFg = new int;
	*NumOfCyclesFG = OurProg.NumOfCycles_FG;
	*NumOfInstFg = OurProg.NumOfInst_fg;
	for(int i=0 ; i<OurProg.ThreadsNum; i++){
		for(int j=0 ; j<REGS_COUNT;j++)
			FG_context[i].reg[j] = OurProg.Threads[i].regFile_FG[j];
	//OurProg.ExecuteThreads_FineGrained();
}
}

double CORE_BlockedMT_CPI(){

	return (double)*NumOfCyclesBlocked/(*NumOfInstBlocked ); 

}

double CORE_FinegrainedMT_CPI(){

		return (double)*NumOfCyclesFG/(*NumOfInstFg); 


}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	//std::cout<<"hi"<<std::endl;
	//std::cout<<OurProg.Threads.size()<<std::endl;
	//std::cout<<"size is: "<<OurProg.Threads[threadid].regFile_BLOCKED.size()<<std::endl;
	for(int i=0 ; i<(*Threads_Num) ; i++){
		for(int j=0 ;j<REGS_COUNT ;j++)
		context[i].reg[i] = Blocked_context[i].reg[j];
	}
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	//std::cout<<"size is: "<<OurProg.Threads[threadid].regFile_BLOCKED.size()<<std::endl;
	for(int i=0 ; i<(*Threads_Num) ; i++){
		for(int j=0 ;j<REGS_COUNT ;j++)
		context[i].reg[i] = FG_context[i].reg[j];
	}
}
