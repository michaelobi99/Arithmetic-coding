#pragma once
#include <iostream>
#include <memory>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <format>
#include <ranges>
#include "BitIO.h"
#include "model.h"
#include <bitset>

const char* compressionName = "Adaptive order-0 model with arithmetic coding\n";
const char* usage = "inputFile outputFile\n";

//****************************************************************************************************
//buildModel function and all its dependencies	

void scaleCounts(unsigned char counts[]) {
	for (unsigned char index : std::ranges::iota_view(0, 256)) {
		if (counts[index] > 0)
			counts[index] = (counts[index] + 1) / 2;
	}
}
																						
void countBytes(std::fstream& input, unsigned char counts[]) {	
	unsigned total{ 1 };
	USHORT scale{};
	char ch{};
	for (int i = 0; i < 256; ++i)
		counts[i] = 0;																															
	while (input.get(ch)) {
		counts[int(ch)]++;
		if (counts[int(ch)] == 255)
			scaleCounts(counts);
	}						
	//input.seekg(0);
	input.close();
	input.open(R"(..\ArithmeticCoding\testFile.txt)", std::ios_base::in | std::ios_base::binary);
	//make sure that the total counts is less than 16384.
	//total was initialized to 1 instead of 0 because there will be an additional 1 	
	//added for the END_OF_STREAM symbol												
	total += std::accumulate(counts, counts + 256, 0);
	if (total > (32767 - 256))
		scale = 4;
	else if (total > 16383)
		scale = 2;
	else 
		return;
	for (int i = 0; i < 256; ++i)
		counts[i] /= scale;
}																						
	
void outputCounts(std::fstream& output, unsigned char counts[]) {
	for (unsigned char index : std::ranges::iota_view(0, 256)) {
		if (counts[index] > 0) {
			output.put(counts[index]);
			output.put(index);
		}
	}
	output.put(END_OF_COUNT);
}
																						
void buildModel(std::fstream& input, std::fstream& output) {							
	unsigned char counts[256];															
	countBytes(input, counts);																												
	buildTotals(counts);		
	outputCounts(output, counts);
}																						
//****************************************************************************************************


void convertIntToSymbol(int c, Symbol& s) {
	s.scale = totals[END_OF_STREAM + 1];
	s.low_count = totals[c];
	s.high_count = totals[c + 1];
}

void encodeSymbol(std::unique_ptr<stl::BitFile>& output, Symbol& s, USHORT& low, USHORT& high, USHORT& underflowBits) {
	unsigned long range = (high - low) + 1;
	high = low + static_cast<USHORT>((range * s.high_count) / s.scale - 1);
	low = low + static_cast<USHORT>((range * s.low_count) / s.scale);
	//the following loop churns out new bits until high and low are far enough apart to have stabilized
	for (;;) {
		//if their MSBs are the same
		if ((high & 0x8000) == (low & 0x8000)) {
			stl::outputBit(output, high & 0x8000);
			while (underflowBits > 0){
				stl::outputBit(output, (~high) & 0x8000);
				underflowBits--;
			}
		}
		//if low first and second MSBs are 01 and high first and second MSBs are 10, and underflow is about to occur
		else if ((low & 0x4000) && !(high & 0x4000)) {
			underflowBits++;
			//toggle the second MSB in both low and high.
			//the shifting operation at the end of the loop will set things right
			high |= (1 << 14); 
			low &=  ~(1 << 14);
		}
		else {
			return;
		}
		low <<= 1;
		high <<= 1;
		high |= 1;
	}
}

void flushArithmeticEncoder(std::unique_ptr<stl::BitFile>& output, USHORT high, USHORT& underflowBits) {
	stl::outputBit(output, high & 0x8000);
	++underflowBits;
	while (underflowBits > 0) {
		stl::outputBit(output, ~high & 0x8000);
		underflowBits--;
	}
}

void compressFile(std::fstream& input, std::unique_ptr<stl::BitFile>& output) {
	int c{};
	USHORT low{0}, high{0xffff}, underflowBits{ 0 };
	Symbol s;
	buildModel(input, output->file);
	while ((c = input.get()) != EOF) {
		convertIntToSymbol(c, s);
		encodeSymbol(output, s, low, high, underflowBits);
	}
	convertIntToSymbol(END_OF_STREAM, s);
	encodeSymbol(output, s, low, high, underflowBits);
	flushArithmeticEncoder(output, high, underflowBits);
	stl::outputBits(output, 0L, 16);
}