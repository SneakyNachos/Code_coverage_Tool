#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <pin.H>
using namespace std;



int main(int argc, char *argv[]){

	//Check if there's a argv[1]
	if(argc < 2){
		cout << "Usage: test <program>" << endl;
		exit(-1);
	}
	
	//Use argv[1] as the file name
	ifstream fp;
	fp.open(argv[1]);

	//Check if file was found
	if(!fp.is_open()){
		//Not found
		cout << "Unable to locate <program>" << endl;
		exit(-1);			
	}

	char elfArch;
	//Move to elfs header to determine arch 
	fp.seekg(0x4);	
	if(!fp.get(elfArch)){
		//Failed to read the char
		cout << "Unable to get Architecture of the file" << endl;
		exit(-1);
	}
	
	//Move to Entry Point location
	unsigned long long e_entry;	
	unsigned long long e_shoff;
	unsigned short e_shstrndx;
	unsigned short e_shnum;
	unsigned short e_shentsize;
	fp.seekg(0x18);

	//Check Architecture for read size
	if(elfArch == 0x1){
		//32 bit
		cout << "32 ELF detected" << endl;

		//Read 4 bytes, and save to entryPoint
		char *ep = new char [4];
		fp.read(ep,4);
		e_entry = *((unsigned long*)ep);
		
		fp.seekg(0x20);
		char *eshoff = new char [4];
		fp.read(eshoff,4);
		e_shoff = *((unsigned long*)eshoff);

		fp.seekg(0x32);
		char *eshstr = new char [2];
		fp.read(eshstr,2);
		e_shstrndx = *((unsigned short*)eshstr);

		fp.seekg(0x30);
		char *eshnum = new char [2];
		fp.read(eshnum,2);
		e_shnum = *((unsigned short *)eshnum);

		fp.seekg(0x2e);
		char *eshentsize = new char [2];
		fp.read(eshentsize,2);
		e_shentsize = *((unsigned short *)eshentsize);

		//Clean Up
		delete[] ep;
		delete[] eshoff;
		delete[] eshstr;
		delete[] eshnum;
		delete[] eshentsize;
	}
	else if(elfArch == 0x2){
		//64 bit
		cout << "64 ELF detected" << endl;

		//Read 8 bytes, and save to entryPoint
		char *ep = new char [8];
		fp.read(ep,8);
		e_entry = *((unsigned long long*)ep); 
		
		fp.seekg(0x28);
		char *eshoff = new char [8];
                fp.read(eshoff,8);
                e_shoff = *((unsigned long*)eshoff);

		fp.seekg(0x3e);
                char *eshstr = new char [2];
                fp.read(eshstr,2);
                e_shstrndx = *((unsigned short*)eshstr);

                fp.seekg(0x3c);
                char *eshnum = new char [2];
                fp.read(eshnum,2);
                e_shnum = *((unsigned short *)eshnum);

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
		printf("section_location_shstrndx: 0x%llx\n",section_shstrndx);
		unsigned long long offset_shstrndx;
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
				unsigned long long maxTextAddress = e_entry+sizeText;
				unsigned long long minTextAddress = e_entry;
				printf(".text Range 0x%llx - 0x%llx\n",minTextAddress,maxTextAddress);
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
	else{
		//Unexpected byte
		cout << "The byte was not either 0x1 or 0x2(32 or 64)";
		exit(-1);
	}
	
	
	
	return 0;
}


