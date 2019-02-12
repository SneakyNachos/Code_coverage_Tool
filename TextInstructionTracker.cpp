#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string.h>
#include "pin.H"

FILE * trace;
unsigned long long e_entry;	
unsigned long long e_shoff;
unsigned short e_shstrndx;
unsigned short e_shnum;
unsigned short e_shentsize;

unsigned long long maxTextAddress;
unsigned long long minTextAddress;

VOID RecordMemRead(ADDRINT ip, VOID * addr){
	if(ip >= (ADDRINT)minTextAddress && ip <= (ADDRINT)maxTextAddress){
		
		fprintf(trace, ": R %p \n",addr);
	}
}
VOID RecordMemWrite(ADDRINT ip, VOID * addr){
	if(ip >= (ADDRINT)minTextAddress && ip <= (ADDRINT)maxTextAddress){
		
		fprintf(trace, ": W %p \n",addr);
	}
}


VOID Instruction(INS ins, VOID *v){
	if(INS_Address(ins) >= (ADDRINT)minTextAddress && INS_Address(ins) <= (ADDRINT)maxTextAddress){
		fprintf(trace, "%lx\n : %s\n",INS_Address(ins),INS_Disassemble(ins).c_str());
	}
	//INS_InsertCall(ins,IPOINT_BEFORE,(AFUNPTR)writeIp,IARG_INST_PTR,IARG_END);

	UINT32 memOperands = INS_MemoryOperandCount(ins);
	
	for(UINT32 memOp = 0; memOp < memOperands; ++memOp){
		if(INS_MemoryOperandIsRead(ins,memOp)){
			INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
				IARG_INST_PTR, IARG_MEMORYOP_EA,memOp, IARG_END);
		}
		if(INS_MemoryOperandIsWritten(ins,memOp)){
			INS_InsertPredicatedCall(
				ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
				IARG_INST_PTR, IARG_MEMORYOP_EA,memOp, IARG_END);
		}
	}
}

VOID Fini(INT32 code, VOID *v){
	fprintf(trace, "#eof\n");
	fclose(trace);
}

INT32 Usage(){
	PIN_ERROR(".text instruction tracker\n" + KNOB_BASE::StringKnobSummary() + "\n");
	return -1;
}




int main(int argc, char * argv[]){
	trace = fopen("ttrace.out","wb");
	//Constants used for determining the offset and size of the .text section
	

	if(PIN_Init(argc,argv)){
		return Usage();
	}

	//Check if there's a argv[1]
	if(argc < 2){
		cout << "Usage: test <program>" << endl;
		exit(-1);
	}
	
	ifstream fp;
	//Huge issue between versions, they change the command line args
	fp.open(argv[6]);

	//Check if file was found
	if(!fp.is_open()){
		//Not found
		cout << "Unable to locate <program>" << endl;
		exit(-1);			
	} else {
		cout << "Opened " << argv[12] << endl;
	}

	char elfArch = '\x00';
	//Move to elfs header to determine arch 
	fp.seekg(0x4);	
	if(!fp.get(elfArch)){
		//Failed to read the char
		cout << "Unable to get Architecture of the file" << endl;
		exit(-1);
	}

	if(elfArch == 0x1){
		//32 bit - elf setup
		cout << "32 ELF detected\n Not Supported yet." << endl;
		exit(-1);
	} 
	else if(elfArch == 0x2){
		//64 bit - elf setup
		cout << "64 ELF detected" << endl;

		//Read 8 bytes, and save to entryPoint
		fp.seekg(0x18);
		char *ep = new char [8];
		fp.read(ep,8);
		e_entry = *((unsigned long long*)ep); 
		
		//Section header offset
		fp.seekg(0x28);
		char *eshoff = new char [8];
                fp.read(eshoff,8);
                e_shoff = *((unsigned long*)eshoff);

		//The section id number for shstrndx
		fp.seekg(0x3e);
                char *eshstr = new char [2];
                fp.read(eshstr,2);
                e_shstrndx = *((unsigned short*)eshstr);

		//The number of sections
                fp.seekg(0x3c);
                char *eshnum = new char [2];
                fp.read(eshnum,2);
                e_shnum = *((unsigned short *)eshnum);

		//The size of each section
                fp.seekg(0x3a);
                char *eshentsize = new char [2];
                fp.read(eshentsize,2);
                e_shentsize = *((unsigned short *)eshentsize);

		//Clean Up
                delete[] ep;
                delete[] eshoff;
                delete[] eshstr;
		delete[] eshnum;
		delete[] eshentsize;

		//Debug
		printf("Entry Point: 0x%llx\n",e_entry);
		printf("Section Header: 0x%llx\n",e_shoff);
		printf("Section Name Index: 0x%hx\n",e_shstrndx);
		printf("e_shnum : 0x%hx\n",e_shnum);
		printf("e_shentsize: 0x%hx\n",e_shentsize);

		//Calculate  location of section names in string
		unsigned long long section_shstrndx = (e_shstrndx*e_shentsize)+e_shoff;
		unsigned long long offset_shstrndx;
		printf("section_location_shstrndx: 0x%llx\n",section_shstrndx);		
		char * off = new char [8];
		fp.seekg(section_shstrndx+0x18);
		fp.read(off,8);
		offset_shstrndx = *((unsigned long*)off);
		delete[] off;

		printf("Offset of shstrndx: 0x%llx\n",offset_shstrndx);

		unsigned long long offsetName = 0x0;
		unsigned long long sizeText = 0x0;
		//Loop through the sections
		for(int i = 0; i < e_shnum;++i){
			string Name;

			fp.seekg((e_shoff+(i*e_shentsize )));
			char * off = new char [4];
			fp.read(off,4);

			offsetName = *((unsigned long*)off);
			//printf("Offset Name for section %d: 0x%llx\n",i,offsetName);	
			fp.seekg(offsetName+offset_shstrndx);
			getline(fp,Name,'\0');
			//cout << "Name: " << Name << endl;
			delete[] off;

			if(Name.compare(".text") == 0){
				cout << "Found .text!" << endl;
				fp.seekg((e_shoff+(i*e_shentsize))+0x20);
				char * off = new char [8];
				fp.read(off,8);
				sizeText = *((unsigned long long *)off);
				printf(".text Size : 0x%llx\n",sizeText);
				minTextAddress = e_entry;
				maxTextAddress = e_entry+sizeText;
				break;
			}

			
		
		}
		//Check if .text was found
		if(sizeText == 0x0){
			//Not Found
			cout << "Unable to find the .text section, or it is zero" << endl;
			exit(-1);
		}
	}
	else {
		cout << "Unable to determine the architecture" << endl;
		exit(-1);
	}

	printf("Range .text : 0x%llx - 0x%llx\n",minTextAddress,maxTextAddress);
	
	
	INS_AddInstrumentFunction(Instruction,0);
	PIN_AddFiniFunction(Fini,0);
	PIN_StartProgram();
	return 0;
	
	
	
}
