#include "StaticBuffer.h"
#include <string.h>
#include <stdio.h>

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer() {
  // copy blockAllocMap blocks from disk to buffer (using readblock() of disk)
  // blocks 0 to 3
  int blockmapslot=0;
  unsigned char buffer[BLOCK_SIZE];
  for(int i=0;i<4;i++)
  {
	//unsigned char buffer[BLOCK_SIZE];
	Disk::readBlock(buffer, i);
	/*for (int slot = 0; slot < BLOCK_SIZE; slot++, blockmapslot++) {
            StaticBuffer::blockAllocMap[blockmapslot] = buffer[slot];
        }*/
        memcpy(blockAllocMap + i * BLOCK_SIZE,buffer,BLOCK_SIZE);
  }
  // initialise all blocks as free
  for (int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++) {
    metainfo[bufferIndex].free = true;
    metainfo[bufferIndex].dirty = false;
    metainfo[bufferIndex].timeStamp = -1;
    metainfo[bufferIndex].blockNum = -1;
  }
}

StaticBuffer::~StaticBuffer() {

  int blockmapslot=0;
  for(int i=0;i<4;i++)
  {
	unsigned char buffer[BLOCK_SIZE];
	for (int slot = 0; slot < BLOCK_SIZE; slot++, blockmapslot++) {
            //buffer[slot] = blockAllocMap[blockmapslot];
            Disk::writeBlock(blockAllocMap + slot * BLOCK_SIZE,slot);
        }
        //Disk::writeBlock(buffer, i);
  }
  for (int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++) {
    if(metainfo[bufferIndex].free==false && metainfo[bufferIndex].dirty==true){
      Disk::writeBlock(StaticBuffer::blocks[bufferIndex],metainfo[bufferIndex].blockNum);
     }
  }

}

int StaticBuffer::getFreeBuffer(int blockNum) {
  if (blockNum < 0 || blockNum > DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }
  int bufferNum=-1;
  int timestamp=-1;
  int maxindex=0;
  for(int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++){
    if(metainfo[bufferIndex].free==false){
      metainfo[bufferIndex].timeStamp++;
    }
  }
  

  for(int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++){
    if(metainfo[bufferIndex].free==true){
      bufferNum=bufferIndex;
      break;
      }
    if(metainfo[bufferIndex].timeStamp>timestamp){
    	timestamp=metainfo[bufferIndex].timeStamp;
    	maxindex=bufferIndex;
    }
   }
   if(bufferNum==-1){
    if(metainfo[maxindex].dirty==true){
      Disk::writeBlock(StaticBuffer::blocks[maxindex],metainfo[maxindex].blockNum);
      bufferNum=maxindex;
    }
  }
     
  
  // iterate through all the blocks in the StaticBuffer
  // find the first free block in the buffer (check metainfo)
  // assign allocatedBuffer = index of the free block

  metainfo[bufferNum].free = false;
  metainfo[bufferNum].blockNum = blockNum;
  metainfo[bufferNum].dirty=false;
  metainfo[bufferNum].timeStamp=0;

  return bufferNum;
}

int StaticBuffer::getBufferNum(int blockNum) {
  // Check if blockNum is valid (between zero and DISK_BLOCKS)
  // and return E_OUTOFBOUND if not valid.
  if (blockNum < 0 || blockNum > DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }
  // find and return the bufferIndex which corresponds to blockNum (check metainfo)
  for(int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++){
    if(metainfo[bufferIndex].blockNum==blockNum){
      return bufferIndex;
      }
   }
  // if block is not in the buffer
  return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum){
    // find the buffer index corresponding to the block using getBufferNum().
    int bufferNum=getBufferNum(blockNum);
    if(bufferNum == E_BLOCKNOTINBUFFER){
    // if block is not present in the buffer (bufferNum = E_BLOCKNOTINBUFFER)
    	return E_BLOCKNOTINBUFFER;
    }
    
    if(bufferNum == E_OUTOFBOUND){
    // if blockNum is out of bound (bufferNum = E_OUTOFBOUND)
         return E_OUTOFBOUND;
    }
    else {
    //     (the bufferNum is valid)
    //     set the dirty bit of that buffer to true in metainfo
	  metainfo[bufferNum].dirty=true;
    }
    return SUCCESS;
}
