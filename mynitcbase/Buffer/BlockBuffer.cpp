#include "BlockBuffer.h"
#include <iostream>
#include <cstdlib>
#include <cstring>


BlockBuffer::BlockBuffer(int blockNum) {
  // initialise this.blockNum with the argument
  this->blockNum=blockNum;
}

BlockBuffer::BlockBuffer(char blocktype){
    // allocate a block on the disk and a buffer in memory to hold the new block of
    //int blockType = blocktype == 'R' ? REC : UNUSED_BLK; 
    // given type using getFreeBlock function and get the return error codes if any.
    int blockNum = getFreeBlock(blocktype);
    /*if(blockNum<0 || blockNum>DISK_BLOCKS){
    	std::cout << E_OUTOFBOUND;
    	this->blockNum=E_OUTOFBOUND;
    	return;
    }*/
    this->blockNum=blockNum;
    	
    // set the blockNum field of the object to that of the allocated block
    // number if the method returned a valid block number,
    // otherwise set the error code returned as the block number.

    // (The caller must check if the constructor allocatted block successfully
    // by checking the value of block number field.)
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}
RecBuffer::RecBuffer() : BlockBuffer('R'){}
// call parent non-default constructor with 'R' denoting record block.


// load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head) {
  //unsigned char buffer[BLOCK_SIZE];
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;   // return any errors that might have occured in the process
  }
  // read the block at this.blockNum into the buffer
  //Disk::readBlock(buffer, this->blockNum);
  // populate the numEntries, numAttrs and numSlots fields in *head
  memcpy(&head->numSlots, bufferPtr + 24, 4);
  memcpy(&head->numEntries, bufferPtr + 16, 4);
  memcpy(&head->numAttrs, bufferPtr + 20, 4);
  memcpy(&head->rblock, bufferPtr + 12, 4);
  memcpy(&head->lblock, bufferPtr + 8, 4);

  return SUCCESS;
}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  struct HeadInfo head;

  // get the header using this.getHeader() function
  this->getHeader(&head);
  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  // read the block at this.blockNum into a buffer
  //unsigned char buffer[BLOCK_SIZE];
  //Disk::readBlock(buffer, this->blockNum);
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }
  /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
     - each record will have size attrCount * ATTR_SIZE
     - slotMap will be of size slotCount
  */
  int recordSize = attrCount * ATTR_SIZE;
  int offset = HEADER_SIZE + slotCount + (recordSize * slotNum);
  //unsigned char *slotPointer = bufferPtr + offset/* calculate buffer + offset */;

  // load the record into the rec data structure
  memcpy(rec, bufferPtr + offset, recordSize);

  return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;
    int ret=BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret!=SUCCESS){
    	return ret;
    }
    struct HeadInfo head;

    this->getHeader(&head);

    int attrCount = head.numAttrs;   
    int slotCount = head.numSlots;  
    if(slotNum>slotCount or slotNum<0){
      return E_OUTOFBOUND;
    }
    //unsigned char buffer[BLOCK_SIZE];
    //Disk::readBlock(buffer, this->blockNum);

    int recordSize = attrCount * ATTR_SIZE;
    int offset = HEADER_SIZE + slotCount + (recordSize * slotNum);

    memcpy(bufferPtr + offset, rec, recordSize);
    StaticBuffer::setDirtyBit(this->blockNum);
    //Disk::writeBlock(buffer, this->blockNum);

    return SUCCESS; 
}

int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;

  // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  struct HeadInfo head;
  this->getHeader(&head);
  // get the header of the block using getHeader() function

  int slotCount = head.numSlots/* number of slots in block from header */;

  // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

  // copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
  memcpy(slotMap, slotMapInBuffer, slotCount);

  return SUCCESS;
}


int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr){
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if (bufferNum != E_BLOCKNOTINBUFFER) {
    for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
    if(StaticBuffer::metainfo[bufferIndex].free==false)
      StaticBuffer::metainfo[bufferIndex].timeStamp++;
    }
    StaticBuffer::metainfo[bufferNum].timeStamp = 0;
  }
  else{
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND) {
      return E_OUTOFBOUND;
    }

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }
  // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
  *buffPtr = StaticBuffer::blocks[bufferNum];

  return SUCCESS;
}

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {

    double diff;
    if (attrType == STRING)
    	diff = strcmp(attr1.sVal, attr2.sVal);

    else
        diff = attr1.nVal - attr2.nVal;

    if (diff > 0) 
    	return 1;
    if (diff < 0) 
    	return -1;
    if (diff == 0)  
    	return 0;
}

int BlockBuffer::setHeader(struct HeadInfo *head){

    unsigned char *bufferPtr;
    // get the starting address of the buffer containing the block using
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret!=SUCCESS){
    	return ret;
    }
    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.

    // cast bufferPtr to type HeadInfo*
    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

    // copy the fields of the HeadInfo pointed to by head (except reserved) to
    // the header of the block (pointed to by bufferHeader)
    //(hint: bufferHeader->numSlots = head->numSlots )
    bufferHeader->numSlots = head->numSlots;
    bufferHeader->lblock = head->lblock;
    bufferHeader->numEntries = head->numEntries;
    bufferHeader->pblock = head->pblock;
    bufferHeader->rblock = head->rblock;
    bufferHeader->blockType = head->blockType;
    bufferHeader->numAttrs=head->numAttrs;
    // update dirty bit by calling StaticBuffer::setDirtyBit()
    // if setDirtyBit() failed, return the error code
    int dirty = StaticBuffer::setDirtyBit(this->blockNum); 
    
    return dirty;
    // return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType){

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block using*/ 		    
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret!=SUCCESS){
    	return ret;
    }
    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.

    // store the input block type in the first 4 bytes of the buffer.
    // (hint: cast bufferPtr to int32_t* and then assign it)
    *((int32_t *)bufferPtr) = blockType;

    // update the StaticBuffer::blockAllocMap entry corresponding to the
    // object's block number to `blockType`.
    StaticBuffer::blockAllocMap[this->blockNum] = blockType;
    // update dirty bit by calling StaticBuffer::setDirtyBit()
    // if setDirtyBit() failed
        // return the returned value from the call
    int dirty= StaticBuffer::setDirtyBit(this->blockNum);
    return dirty;
    // return SUCCESS
}

int BlockBuffer::getFreeBlock(int blockType){

    // iterate through the StaticBuffer::blockAllocMap and find the block number
    // of a free block in the disk.
    int blockNum;
    for (blockNum = 0; blockNum < DISK_BLOCKS; blockNum++) {
      if (StaticBuffer::blockAllocMap[blockNum] == UNUSED_BLK) {
        break;
      }
    }
    // if no block is free, return E_DISKFULL.
    if (blockNum == DISK_BLOCKS)
        return E_DISKFULL;
    // set the object's blockNum to the block number of the free block.
    this->blockNum=blockNum;
    // find a free buffer using StaticBuffer::getFreeBuffer() .
    int bufferNum=StaticBuffer::getFreeBuffer(blockNum);
    struct HeadInfo head;
    head.pblock=-1;
    head.lblock=-1;
    head.rblock=-1;
    head.numEntries = 0;
    head.numAttrs = 0;
    head.numSlots = 0;
    setHeader(&head); 
    // initialize the header of the block passing a struct HeadInfo with values
    // pblock: -1, lblock: -1, rblock: -1, numEntries: 0, numAttrs: 0, numSlots: 0
    // to the setHeader() function.
    setBlockType(blockType);
    // update the block type of the block to the input block type using setBlockType().
   
    // return block number of the free block.
    return blockNum;
}

int RecBuffer::setSlotMap(unsigned char *slotMap) {
    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block using*/
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);

    if(ret!=SUCCESS)
    	return ret;
    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.

    // get the header of the block using the getHeader() function
    HeadInfo head;
    this->getHeader(&head);
    int numSlots = head.numSlots;/* the number of slots in the block */;
    memcpy(bufferPtr + HEADER_SIZE, slotMap, numSlots);
    // the slotmap starts at bufferPtr + HEADER_SIZE. Copy the contents of the
    // argument `slotMap` to the buffer replacing the existing slotmap.
    // Note that size of slotmap is `numSlots`

    // update dirty bit using StaticBuffer::setDirtyBit
    // if setDirtyBit failed, return the value returned by the call
    int dirty=StaticBuffer::setDirtyBit(this->blockNum);
    if(dirty!=SUCCESS)
    	return dirty;
    return SUCCESS;
}

int BlockBuffer::getBlockNum(){

    return this->blockNum;
    //return corresponding block number.
}

void BlockBuffer::releaseBlock(){

    // if blockNum is INVALID_BLOCKNUM (-1), or it is invalidated already, do nothing
    if(this->blockNum == INVALID_BLOCKNUM /*|| StaticBuffer::blockAllocMap[this->blockNum] == UNUSED_BLK*/)
    	return;

    else{
    	int bufferNum= StaticBuffer::getBufferNum(this->blockNum);
    	if(bufferNum>=0 && bufferNum< BUFFER_CAPACITY)
    		StaticBuffer::metainfo[bufferNum].free=true;
    	StaticBuffer::blockAllocMap[blockNum] = UNUSED_BLK;
    	this->blockNum=INVALID_BLOCKNUM;
    	}
        /* get the buffer number of the buffer assigned to the block
           using StaticBuffer::getBufferNum().
           (this function return E_BLOCKNOTINBUFFER if the block is not
           currently loaded in the buffer)
            */

        // if the block is present in the buffer, free the buffer
        // by setting the free flag of its StaticBuffer::tableMetaInfo entry
        // to true.

        // free the block in disk by setting the data type of the entry
        // corresponding to the block number in StaticBuffer::blockAllocMap
        // to UNUSED_BLK.

        // set the object's blockNum to INVALID_BLOCK (-1)
}
