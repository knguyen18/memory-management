// Name Katelynn Nguyen
// Class (CECS 325-02)
// Project Name (Proj 7 - Memory Manager)
// Due Date (12/08/2022)
// I certify that this program is my own original work. I did not copy any part of this program from
// any other source. I further certify that I typed each and every line of code in this program.
#include "MemoryManager.h"

#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <cstring>
#include <climits>
using namespace std;

namespace MemoryManager
{
	// IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT
	//
	// This is the only static memory that you may use, no other global variables may be
	// created, if you need to save data make it fit in MM_pool
	//
	// IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT

	const int MM_POOL_SIZE = 65536;
	char MM_pool[MM_POOL_SIZE];

	// I have provided this tool for your use
	void memView(int start, int end)
	{

		const unsigned int SHIFT = 8;
		const unsigned int MASK = 1 << (SHIFT - 1);

		unsigned int value; // used to facilitate bit shifting and masking

		cout << "               Pool                     Unsignd  Unsigned " << endl;
		cout << "Mem Add        indx   bits   chr ASCII#  short      int    " << endl;
		cout << "-------------- ---- -------- --- ------ ------- ------------" << endl;

		for (int i = start; i <= end; i++)
		{
			cout << (long *)(MM_pool + i) << ':'; // the actual address in hexadecimal
			cout << '[' << setw(2) << i << ']';	  // the index into MM_pool

			value = MM_pool[i];
			cout << " ";
			for (int j = 1; j <= SHIFT; j++) // the bit sequence for this byte (8 bits)
			{
				cout << ((value & MASK) ? '1' : '0');
				value <<= 1;
			}
			cout << " ";

			if (MM_pool[i] < 32) // non-printable character so print a blank
				cout << '|' << ' ' << "| (";
			else												// characger is printable so print it
				cout << '|' << *(char *)(MM_pool + i) << "| ("; // the ASCII character of the 8 bits (1 byte)

			cout << setw(4) << ((int)(*((unsigned char *)(MM_pool + i)))) << ")"; // the ASCII number of the character

			cout << " (" << setw(5) << (*(unsigned short *)(MM_pool + i)) << ")"; // the unsigned short value of 16 bits (2 bytes)
			cout << " (" << setw(10) << (*(unsigned int *)(MM_pool + i)) << ")";  // the unsigned int value of 32 bits (4 bytes)

			//  cout << (*(unsigned int*)(MM_pool+i)) << "|";	// the unsigned int value of 32 bits (4 bytes)

			cout << endl;
		}
	}

	// Initialize set up any data needed to manage the memory pool
	void initializeMemoryManager(void)
	{

		int freeHead = 0;  // starting index of the freelist
		int inUseHead = 2; // starting index of the inUselist
		int usedHead = 4;  // starting index for the used list - deallocated memory

		int nextLink = 2; // offset index of the next link
		int prevLink = 4; // offset index for the prev link

		*(unsigned short *)(MM_pool + freeHead) = 6;  // freelist starts at byte 6
		*(unsigned short *)(MM_pool + inUseHead) = 0; // nothing in the inUse list yet
		*(unsigned short *)(MM_pool + usedHead) = 0;  // nothing in the used list yet
	}

	// return a pointer inside the memory pool
	// If no chunk can accommodate aSize call onOutOfMemory() - still
	// aSize = size of chunk of mem allocating mem for

	void *allocate(int aSize)
	{
		if ((int)(*(unsigned short *)(MM_pool)) + aSize + 6 > 65536) // add 6 because free head, inuse, and used
			onOutOfMemory();

		// allocate memory for overhead (6) then actual mem needed
		// need to cast as unsigned short bwcause accessing 2 bytes at a time
		int freeHead = ((int)*(unsigned short *)(MM_pool));
		int inUseHead = ((int)*(unsigned short *)&MM_pool[2]);

		// starts at freehead each time allocating memory
		// this is where the free memory starts at
		*(unsigned short *)(MM_pool + freeHead) = aSize;

		// next ptr
		if (freeHead == 6)
		{
			*(unsigned short *)(MM_pool + freeHead + 2) = 0;
		}
		else
		{
			*(unsigned short *)(MM_pool + freeHead + 2) = inUseHead;
			// update prev pointer for LEFT block
			*(unsigned short *)(MM_pool + inUseHead + 4) = freeHead;
		}

		// prev pointer
		*(unsigned short *)(MM_pool + freeHead + 4) = 0;

		// update inUseHead (should be what free used to be)
		*(unsigned short *)(MM_pool + 2) = freeHead;
		// updating free head after allocation: freehead + overhead of node + alloc size
		*(unsigned short *)(MM_pool) = (freeHead + aSize + 6);
		// MM_pool[0] = (unsigned short)(freeHead + aSize);

		// returning pointer where data starts: after 6 bytes overhead
		int sum = (freeHead + 6);
		return (void *)(MM_pool + sum);
	}

	// Free up a chunk previously allocated
	void deallocate(void *aPointer)
	// points to where data starts, after 6 bytes overhead
	{
		int usedHead = ((int)*(unsigned short *)&MM_pool[4]);
		int inUsedHead = ((int)*(unsigned short *)&MM_pool[2]);
		char *ptr = (char *)(aPointer);
		unsigned short currPrev = (unsigned short)(*(ptr - 2));
		unsigned short currNext = (unsigned short)(*(ptr - 4));

		int curr = 6;
		if (currNext != 0)
		{

			curr = ((int)*(unsigned short *)&MM_pool[currNext + 4]);
		}

		// ****** FOR USED PARTS ****
		if (usedHead == 0)
		{
			*(unsigned short *)(ptr - 2) = 0;
			*(unsigned short *)(ptr - 4) = 0;
			unsigned short stored_curr_next = (unsigned short)(*(ptr - 2));
			unsigned short stored_curr_prev = (unsigned short)(*(ptr - 4));
		}
		else
		{
			// update usedHead's prev
			*(unsigned short *)(MM_pool + usedHead + 4) = curr;
			// update current dealloc block's next
			*(unsigned short *)(MM_pool + curr + 2) = usedHead;
			// current dealloc block's prev = 0
			*(unsigned short *)(MM_pool + curr + 4) = 0;
		}

		// ************ FOR INUSE PARTS ******************
		// usedHead = where dealloc block starts aka dealloc block's left block's prev ptr
		// but need to make sure a left block exists aka we're not deallocing the 1st block
		if (currNext == 0)
		{
			// if we're dealloc the 1st block, usedHead points at 6 (according to example)
			*(unsigned short *)(MM_pool + 4) = 6;
		}
		else
		{
			int left_block_prev = ((int)*(unsigned short *)&MM_pool[currNext + 4]);
			*(unsigned short *)(MM_pool + 4) = left_block_prev;
		}
		int stored_usedHead = ((int)*(unsigned short *)&MM_pool[4]);

		// if dealloc the last block, does inUsed point to its left block?
		// case: what if there is only 1 block and we dealloc it.. what happens to the inUsed? does it point to 0?
		if (currPrev == 0)
		{
			*(unsigned short *)(MM_pool + 2) = currNext;
		}
		int stored_inUsedHead((int)*(unsigned short *)&MM_pool[2]);

		// if there is a left block, update its prev
		if (currNext != 0)
		{
			*(unsigned short *)(MM_pool + currNext + 4) = currPrev;
		}

		// if there is a right block, update its next
		if (currPrev != 0)
		{
			*(unsigned short *)(MM_pool + currPrev + 2) = currNext;
		}

		int stored_inUse_left_prev = ((int)*(unsigned short *)&MM_pool[currNext + 4]);
		int stored__inUse_right_next = ((int)*(unsigned short *)&MM_pool[currPrev + 2]);
	}
	// returns the number of bytes allocated by ptr
	int size(void *ptr)
	{
		// ptr holds mem add and go back 6 bc that's where node starts
		// no need for MM_pool because ptr already points to somewhere in MM_pool
		char *node = (char *)(ptr);
		return (int)(*(node - 6));
	}

	// Will scan the memory pool and return the total free space remaining
	int freeMemory(void)
	{
		return (MM_POOL_SIZE - ((int)(*(unsigned short *)MM_pool)));
	}

	// Will scan the memory pool and return the total used memory - memory that has been deallocated
	int usedMemory(void)
	{
		// traverse through
		int curr = *(unsigned short *)&MM_pool[4];
		int size = *(unsigned short *)&MM_pool[curr];
		int next = *(unsigned short *)&MM_pool[curr + 2];
		int usedMem = 0;
		int numNode = 0;
		while (curr != 0)
		{
			// update current to next 'pointer'
			curr = next;
			usedMem = size + usedMem;
			size = *(unsigned short *)&MM_pool[curr];
			next = *(unsigned short *)&MM_pool[curr + 2];
			++numNode;
		}
		usedMem = usedMem + (numNode * 6);
		return usedMem;
	}

	// Will scan the memory pool and return the total in use memory - memory being currently used
	int inUseMemory(void)
	{
		// 1.traverse through the inUse list in memory
		// 2.get the index value inside of the 2 bytes of inUse
		int curr = *(unsigned short *)&MM_pool[2];
		int size = *(unsigned short *)&MM_pool[curr];
		int next = *(unsigned short *)&MM_pool[curr + 2];
		int numNode = 0;
		int inUseMem = 0;
		while (curr != 0)
		{
			// update current to next 'pointer'
			curr = next;
			inUseMem = size + inUseMem;
			size = *(unsigned short *)&MM_pool[curr];
			next = *(unsigned short *)&MM_pool[curr + 2];
			++numNode;
		}
		// multiply numNode by 6 to take account of each overhead
		inUseMem = inUseMem + (numNode * 6);
		return inUseMem;
	}

	// helper function to see the InUse list in memory
	void traverseInUse()
	{
		int cur = *(unsigned short *)&MM_pool[2];
		int size = *(unsigned short *)&MM_pool[cur];
		int next = *(unsigned short *)&MM_pool[cur + 2];
		int prev = *(unsigned short *)&MM_pool[cur + 4];
		cout << "\nTraversing InUse Memory....";
		cout << "\n      InUse Head:" << cur;
		while (cur != 0)
		{
			cout << "\n        Index:" << cur << " Size:" << size << " Next:" << next << " Prev:" << prev;
			cur = next;
			size = *(unsigned short *)&MM_pool[cur];
			next = *(unsigned short *)&MM_pool[cur + 2];
			prev = *(unsigned short *)&MM_pool[cur + 4];
		}
	}

	// Helper function to see the Used list in memory
	void traverseUsed()
	{
		int cur = *(unsigned short *)&MM_pool[4];
		int size = *(unsigned short *)&MM_pool[cur];
		int next = *(unsigned short *)&MM_pool[cur + 2];
		int prev = *(unsigned short *)&MM_pool[cur + 4];
		cout << "\nTraversing Used Memory....";
		cout << "\n      Used Head:" << cur;
		while (cur != 0)
		{
			cout << "\n        Index:" << cur << " Size:" << size << " Next:" << next << " Prev:" << prev;
			cur = next;
			size = *(unsigned short *)&MM_pool[cur];
			next = *(unsigned short *)&MM_pool[cur + 2];
			prev = *(unsigned short *)&MM_pool[cur + 4];
		}
	}

	// this is called from Allocate if there is no more memory availalbee
	void onOutOfMemory(void)
	{
		std::cout << "\nMemory pool out of memory\n"
				  << std::endl;
		std::cout << "\n---Press \"Enter\" key to end program---:";
		cin.get();
		exit(1); // exit main
	}
}