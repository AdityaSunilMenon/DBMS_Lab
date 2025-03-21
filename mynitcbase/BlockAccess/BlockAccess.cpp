#include "BlockAccess.h"
#include <cstdlib>
#include <cstring>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId,&prevRecId);
    int block=-1,slot=-1;

    // let block and slot denote the record id of the record being currently checked

    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)
	RelCatEntry relCatEntry;
	RelCacheTable::getRelCatEntry(relId, &relCatEntry);
        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
	block=relCatEntry.firstBlk;
	slot=0;
        // block = first record block of the relation
        // slot = 0
    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)
	block=prevRecId.block;
	slot=prevRecId.slot + 1;
        // block = search index's block
        // slot = search index's slot + 1
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */
           RecBuffer recBuffer(block);
           HeadInfo head;
           recBuffer.getHeader(&head);
           Attribute rec[ATTRCAT_NO_ATTRS];
           recBuffer.getRecord(rec,slot);
           unsigned char slotMap[head.numSlots];
           recBuffer.getSlotMap(slotMap);

        // get the record with id (block, slot) using RecBuffer::getRecord()
        // get header of the block using RecBuffer::getHeader() function
        // get slot map of the block using RecBuffer::getSlotMap() function

        // If slot >= the number of slots per block(i.e. no more slots in this block)
        if(slot>=head.numSlots)
        {
            // update block = right block of block
            // update slot = 0
            block=head.rblock;
            slot=0;
            continue;  // continue to the beginning of this while loop
        }

        // if slot is free skip the loop
        // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
        if(slotMap[slot] == SLOT_UNOCCUPIED)
        {
            // increment slot and continue to the next record slot
            slot++;
            continue;
        }

        // compare record's attribute value to the the given attrVal as below:
        /*
            firstly get the attribute offset for the attrName attribute
            from the attribute cache entry of the relation using
            AttrCacheTable::getAttrCatEntry()
        */
        AttrCatEntry attrcatbuffer;
        AttrCacheTable::getAttrCatEntry(relId,attrName,&attrcatbuffer);
        /* use the attribute offset to get the value of the attribute from
           current record */
        if(attrcatbuffer.attrType == NUMBER)
		int val=rec[attrcatbuffer.offset].nVal;
	else
		const char * val2=rec[attrcatbuffer.offset].sVal;
        //Attribute rec2[ATTRCAT_NO_ATTRS];
        //AttrCacheTable::attrCatEntryToRecord(&attrcatbuffer,rec2);
        int cmpVal=compareAttrs(rec[attrcatbuffer.offset],attrVal,attrcatbuffer.attrType);  // will store the difference between the attributes
        // set cmpVal using compareAttrs()

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            */
            RecId recid={block,slot};
	    RelCacheTable::setSearchIndex(relId,&recid);
            return RecId{block, slot};
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;    // set newRelationName with newName
    strcpy(newRelationName.sVal,newName);
    // search the relation catalog for an entry with "RelName" = newRelationName
    RecId recid=BlockAccess::linearSearch(RELCAT_RELID,RELCAT_ATTR_RELNAME,newRelationName,EQ);
    if(recid.block!=-1 && recid.slot!=-1){
    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    	return E_RELEXIST;
    }

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute oldRelationName;    // set oldRelationName with oldName
    strcpy(oldRelationName.sVal,oldName);
    // search the relation catalog for an entry with "RelName" = oldRelationName
    RecId recidold=BlockAccess::linearSearch(RELCAT_RELID,RELCAT_ATTR_RELNAME,oldRelationName,EQ);
    if(recidold.block==-1 && recidold.slot==-1){
    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    return E_RELNOTEXIST;
    }
    
    RecBuffer relcatblock(RELCAT_BLOCK);
    Attribute rec[RELCAT_NO_ATTRS];
    relcatblock.getRecord(rec,recidold.slot);
    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    strcpy(rec[RELCAT_REL_NAME_INDEX].sVal,newName);
    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    // set back the record value using RecBuffer.setRecord
    relcatblock.setRecord(rec,recidold.slot);
    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    for (int i = 0; i < rec[RELCAT_NO_ATTRIBUTES_INDEX].nVal; i++) {
    	RecId recidattr=BlockAccess::linearSearch(ATTRCAT_RELID,ATTRCAT_ATTR_RELNAME,oldRelationName,EQ);
    	RecBuffer attrcatblock(recidattr.block);
    	Attribute recattr[ATTRCAT_NO_ATTRS];
    	attrcatblock.getRecord(recattr,recidattr.slot);
    	strcpy(recattr[ATTRCAT_REL_NAME_INDEX].sVal,newName);
    	attrcatblock.setRecord(recattr,recidattr.slot);
    }
    //for i = 0 to numberOfAttributes :
    //    linearSearch on the attribute catalog for relName = oldRelationName
    //    get the record using RecBuffer.getRecord
    //
    //    update the relName field in the record to newName
    //    set back the record using RecBuffer.setRecord

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;    // set relNameAttr to relName
    strcpy(relNameAttr.sVal,relName);
    RecId recid=BlockAccess::linearSearch(RELCAT_RELID,RELCAT_ATTR_RELNAME,relNameAttr,EQ);
    if(recid.block==-1 && recid.slot==-1){
    // Search for the relation with name relName in relation catalog using linearSearch()
    // If relation with name relName does not exist (search returns {-1,-1})
        return E_RELNOTEXIST;
    }
    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    /* declare variable attrToRenameRecId used to store the attr-cat recId
    of the attribute to rename */
    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    /* iterate over all Attribute Catalog Entry record corresponding to the
       relation to find the required attribute */
    while (true) {
        // linear search on the attribute catalog for RelName = relNameAttr
	RecId recidattr=BlockAccess::linearSearch(ATTRCAT_RELID,ATTRCAT_ATTR_RELNAME,relNameAttr,EQ);
        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        //     break;
	if(recidattr.block==-1 && recidattr.slot==-1)
		break;
        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */
	RecBuffer attrcatblock(recidattr.block);
	attrcatblock.getRecord(attrCatEntryRecord,recidattr.slot);
	if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,oldName)==0){
		attrToRenameRecId.block=recidattr.block;
		attrToRenameRecId.slot=recidattr.slot;
        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record
	}
	if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName)==0){
        // if attrCatEntryRecord.attrName = newName
             return E_ATTREXIST;
        }
    }

    // if attrToRenameRecId == {-1, -1}
    //     return E_ATTRNOTEXIST;
    if(attrToRenameRecId.block==-1 && attrToRenameRecId.slot==-1){
    	return E_ATTRNOTEXIST;
    }
    RecBuffer attrcatblock(attrToRenameRecId.block);
    Attribute attrcatrec[ATTRCAT_NO_ATTRS];;
    attrcatblock.getRecord(attrcatrec,attrToRenameRecId.slot);
    strcpy(attrcatrec[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);
    attrcatblock.setRecord(attrcatrec,attrToRenameRecId.slot);
    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    //   update the AttrName of the record with newName
    //   set back the record with RecBuffer.setRecord

    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record) {
    // get the relation catalog entry from relation cache
    // ( use RelCacheTable::getRelCatEntry() of Cache Layer)
    RelCatEntry relCatEntry;
    int ret=RelCacheTable::getRelCatEntry(relId,&relCatEntry);
    if(ret!=SUCCESS)
    	return ret;
    int blockNum = relCatEntry.firstBlk/* first record block of the relation (from the rel-cat entry)*/;

    // rec_id will be used to store where the new record will be inserted
    RecId rec_id = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk/* number of slots per record block */;
    int numOfAttributes = relCatEntry.numAttrs/* number of attributes of the relation */;

    int prevBlockNum = -1/* block number of the last element in the linked list = -1 */;

    /*
        Traversing the linked list of existing record blocks of the relation
        until a free slot is found OR
        until the end of the list is reached
    */
    while (blockNum != -1) {
        
        RecBuffer recbuffer(blockNum);
        // create a RecBuffer object for blockNum (using appropriate constructor!)
        struct HeadInfo head;
        // get header of block(blockNum) using RecBuffer::getHeader() function
        recbuffer.getHeader(&head);
        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function
        unsigned char slotMap2[head.numSlots];
    recbuffer.getSlotMap(slotMap2);
        // search for free slot in the block 'blockNum' and store it's rec-id in rec_id
        // (Free slot can be found by iterating over the slot map of the block)
        /* slot map stores SLOT_UNOCCUPIED if slot is free and
           SLOT_OCCUPIED if slot is occupied) */
        for(int j=0;j<head.numSlots;j++){
            if(slotMap2[j]==SLOT_UNOCCUPIED){
            	rec_id.block=blockNum;
            	rec_id.slot = j;
            	break;
           }
        }
        if(rec_id.slot!=-1 && rec_id.block!=-1){
        	break;
        }
        
        prevBlockNum = blockNum;
        blockNum=head.rblock;
        /* if a free slot is found, set rec_id and discontinue the traversal
           of the linked list of record blocks (break from the loop) */

        /* otherwise, continue to check the next block by updating the
           block numbers as follows:
              update prevBlockNum = blockNum
              update blockNum = header.rblock (next element in the linked
                                               list of record blocks)
        */
    }

    //  if no free slot is found in existing record blocks (rec_id = {-1, -1})
    if(rec_id.block == -1 || rec_id.slot==-1)
    {
        // if relation is RELCAT, do not allocate any more blocks
        //     return E_MAXRELATIONS;
        if (relId == RELCAT_RELID) {
            return E_MAXRELATIONS;
         }
        // Otherwise,
        // get a new record block (using the appropriate RecBuffer constructor!)
        // get the block number of the newly allocated block
        // (use BlockBuffer::getBlockNum() function)
        // let ret be the return value of getBlockNum() function call
        RecBuffer blockBuffer;
        blockNum = blockBuffer.getBlockNum();
        if (blockNum == E_DISKFULL) {
            return E_DISKFULL;
        }

        // Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
        rec_id.block = blockNum;
        rec_id.slot = 0;
        /*
            set the header of the new record block such that it links with
            existing record blocks of the relation
            set the block's header as follows:
            blockType: REC, pblock: -1
            lblock
                  = -1 (if linked list of existing record blocks was empty
                         i.e this is the first insertion into the relation)
                  = prevBlockNum (otherwise),
            rblock: -1, numEntries: 0,
            numSlots: numOfSlots, numAttrs: numOfAttributes
            (use BlockBuffer::setHeader() function)
        */
        struct HeadInfo blockheader;
        blockheader.pblock=-1;
        blockheader.rblock=-1;
        blockheader.lblock=prevBlockNum;
        blockheader.blockType = REC;
        blockheader.numAttrs = relCatEntry.numAttrs;
        blockheader.numEntries=0;
        blockheader.numSlots = relCatEntry.numSlotsPerBlk;
        blockBuffer.setHeader(&blockheader);
        blockBuffer.getHeader(&blockheader);
        /*
            set block's slot map with all slots marked as free
            (i.e. store SLOT_UNOCCUPIED for all the entries)
            (use RecBuffer::setSlotMap() function)
        */
        unsigned char slotMap1[numOfSlots];
    for (int k = 0; k < relCatEntry.numSlotsPerBlk; k++) {
      slotMap1[k] = SLOT_UNOCCUPIED;
    }
    blockBuffer.setSlotMap(slotMap1);

        // if prevBlockNum != -1
        if(prevBlockNum!=-1) 
        {
            RecBuffer prevBuffer(prevBlockNum);
            struct HeadInfo prevhead;
            prevBuffer.getHeader(&prevhead);
            prevhead.rblock=rec_id.block;
            prevBuffer.setHeader(&prevhead);
            // create a RecBuffer object for prevBlockNum
            // get the header of the block prevBlockNum and
            // update the rblock field of the header to the new block
            // number i.e. rec_id.block
            // (use BlockBuffer::setHeader() function)
        }
        else
        {
            // update first block field in the relation catalog entry to the
            relCatEntry.firstBlk=rec_id.block;
            RelCacheTable::setRelCatEntry(relId,&relCatEntry);
            // new block (using RelCacheTable::setRelCatEntry() function)
        }
        relCatEntry.lastBlk = rec_id.block;
        RelCacheTable::setRelCatEntry(relId,&relCatEntry);

        // update last block field in the relation catalog entry to the
        // new block (using RelCacheTable::setRelCatEntry() function)
    }
    RecBuffer blockBuffer(rec_id.block);
    blockBuffer.setRecord(record,rec_id.slot);
    
    // create a RecBuffer object for rec_id.block
    // insert the record into rec_id'th slot using RecBuffer.setRecord())
    unsigned char slotMap[numOfSlots];
  blockBuffer.getSlotMap(slotMap);
  slotMap[rec_id.slot]=SLOT_OCCUPIED;
  blockBuffer.setSlotMap(slotMap);
    /* update the slot map of the block by marking entry of the slot to
       which record was inserted as occupied) */
    // (ie store SLOT_OCCUPIED in free_slot'th entry of slot map)
    // (use RecBuffer::getSlotMap() and RecBuffer::setSlotMap() functions)

    // increment the numEntries field in the header of the block to
    // which record was inserted
    // (use BlockBuffer::getHeader() and BlockBuffer::setHeader() functions)
    struct HeadInfo head;
    blockBuffer.getHeader(&head);
    head.numEntries++;
    blockBuffer.setHeader(&head);
    // Increment the number of records field in the relation cache entry for
    // the relation. (use RelCacheTable::setRelCatEntry function)
    relCatEntry.numRecs++;

    RelCacheTable::setRelCatEntry(relId,&relCatEntry);

    return SUCCESS;
}

/*
NOTE: This function will copy the result of the search to the `record` argument.
      The caller should ensure that space is allocated for `record` array
      based on the number of attributes in the relation.
*/
int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    // Declare a variable called recid to store the searched record
    RecId recId= BlockAccess::linearSearch(relId,attrName,attrVal,op);

    /* search for the record id (recid) corresponding to the attribute with
    attribute name attrName, with value attrval and satisfying the condition op
    using linearSearch() */
    if(recId.block==-1 && recId.slot==-1)
    	return E_NOTFOUND;
    // if there's no record satisfying the given condition (recId = {-1, -1})
    //    return E_NOTFOUND;
    RecBuffer recblock(recId.block);
    int ret= recblock.getRecord(record,recId.slot);
    if(ret!=SUCCESS)
    	return ret;
    /* Copy the record with record id (recId) to the record buffer (record)
       For this Instantiate a RecBuffer class object using recId and
       call the appropriate method to fetch the record
    */

    return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE]) {
    // if the relation to delete is either Relation Catalog or Attribute Catalog,
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
         return E_NOTPERMITTED;
        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_NAME and ATTRCAT_NAME)

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr; // (stores relName as type union Attribute)
    // assign relNameAttr.sVal = relName
    strcpy((char *)relNameAttr.sVal,(const char *)relName);
    //  linearSearch on the relation catalog for RelName = relNameAttr
    
    // if the relation does not exist (linearSearch returned {-1, -1})
    //     return E_RELNOTEXIST
    RecId relCatRecId = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAttr ,EQ);
    if(relCatRecId.block==-1 && relCatRecId.slot==-1)
    	return E_RELNOTEXIST;
    RecBuffer relcatblock(relCatRecId.block);
    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    relcatblock.getRecord(relCatEntryRecord,relCatRecId.slot);
    /* store the relation catalog record corresponding to the relation in
       relCatEntryRecord using RecBuffer.getRecord */
    int firstBlock=relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    int numAttrs=relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    /* get the first record block of the relation (firstBlock) using the
       relation catalog entry record */
    /* get the number of attributes corresponding to the relation (numAttrs)
       using the relation catalog entry record */
    int nextblock=firstBlock;
    while(nextblock!=-1){
    	RecBuffer block(nextblock);
    	HeadInfo head;
    	block.getHeader(&head);
    	nextblock=head.rblock;
    	block.releaseBlock();
    }
    /*
     Delete all the record blocks of the relation
    */
    // for each record block of the relation:
    //     get block header using BlockBuffer.getHeader
    //     get the next block from the header (rblock)
    //     release the block using BlockBuffer.releaseBlock
    //
    //     Hint: to know if we reached the end, check if nextBlock = -1


    /***
        Deleting attribute catalog entries corresponding the relation and index
        blocks corresponding to the relation with relName on its attributes
    ***/

    // reset the searchIndex of the attribute catalog
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    int numberOfAttributesDeleted = 0;

    while(true) {
        RecId attrCatRecId=BlockAccess::linearSearch(ATTRCAT_RELID,ATTRCAT_ATTR_RELNAME,relNameAttr,EQ);
        // attrCatRecId = linearSearch on attribute catalog for RelName = relNameAttr
        if(attrCatRecId.block==-1 && attrCatRecId.slot==-1)
        	break;

        // if no more attributes to iterate over (attrCatRecId == {-1, -1})
        //     break;

        numberOfAttributesDeleted++;

        // create a RecBuffer for attrCatRecId.block
        // get the header of the block
        // get the record corresponding to attrCatRecId.slot
        RecBuffer attrcatblock(attrCatRecId.block);
        HeadInfo attrcathead;
        attrcatblock.getHeader(&attrcathead);
        Attribute attrcatrecord[ATTRCAT_NO_ATTRS];
        attrcatblock.getRecord(attrcatrecord,attrCatRecId.slot);
        // declare variable rootBlock which will be used to store the root
        // block field from the attribute catalog record.
        int rootBlock =  attrcatrecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
        /* get root block from the record */
        // (This will be used later to delete any indexes if it exists)
	unsigned char slotmap[attrcathead.numSlots];
	attrcatblock.getSlotMap(slotmap);
	slotmap[attrCatRecId.slot]= SLOT_UNOCCUPIED;
	attrcatblock.setSlotMap(slotmap);
        // Update the Slotmap for the block by setting the slot as SLOT_UNOCCUPIED
        // Hint: use RecBuffer.getSlotMap and RecBuffer.setSlotMap
	attrcathead.numEntries--;
	attrcatblock.setHeader(&attrcathead);
        /* Decrement the numEntries in the header of the block corresponding to
           the attribute catalog entry and then set back the header
           using RecBuffer.setHeader */

        /* If number of entries become 0, releaseBlock is called after fixing
           the linked list.
        */
        if (attrcathead.numEntries==0/*   header.numEntries == 0  */) {
            RecBuffer leftblock(attrcathead.lblock);
            HeadInfo lefthead;
            leftblock.getHeader(&lefthead);
            lefthead.rblock=attrcathead.rblock;
            leftblock.setHeader(&lefthead);
            
            /* Standard Linked List Delete for a Block
               Get the header of the left block and set it's rblock to this
               block's rblock
            */

            // create a RecBuffer for lblock and call appropriate methods

            if (attrcathead.rblock!=-1/* header.rblock != -1 */) {
                /* Get the header of the right block and set it's lblock to
                   this block's lblock */
                // create a RecBuffer for rblock and call appropriate methods
		RecBuffer rightblock(attrcathead.rblock);
            	HeadInfo righthead;
            	rightblock.getHeader(&righthead);
            	righthead.lblock=attrcathead.lblock;
            	rightblock.setHeader(&righthead);
            } else {
                // (the block being released is the "Last Block" of the relation.)
                RelCatEntry relCatEntry;
                RelCacheTable::getRelCatEntry(ATTRCAT_RELID,&relCatEntry);
                relCatEntry.lastBlk=attrcathead.lblock;
                /* update the Relation Catalog entry's LastBlock field for this
                   relation with the block number of the previous block. */
            }

            // (Since the attribute catalog will never be empty(why?), we do not
            //  need to handle the case of the linked list becoming empty - i.e
            //  every block of the attribute catalog gets released.)

            // call releaseBlock()
            attrcatblock.releaseBlock();
        }

        // (the following part is only relevant once indexing has been implemented)
        // if index exists for the attribute (rootBlock != -1), call bplus destroy
        if (rootBlock != -1) {
            // delete the bplus tree rooted at rootBlock using BPlusTree::bPlusDestroy()
        }
    }

    /*** Delete the entry corresponding to the relation from relation catalog ***/
    // Fetch the header of Relcat block
    HeadInfo relcathead;
    relcatblock.getHeader(&relcathead);
    relcathead.numEntries--;
    relcatblock.setHeader(&relcathead);
    /* Decrement the numEntries in the header of the block corresponding to the
       relation catalog entry and set it back */

    /* Get the slotmap in relation catalog, update it by marking the slot as
       free(SLOT_UNOCCUPIED) and set it back. */
    unsigned char slotmap[relcathead.numSlots];
    relcatblock.getSlotMap(slotmap);
    slotmap[relCatRecId.slot] = SLOT_UNOCCUPIED;
    relcatblock.setSlotMap(slotmap);
    /*** Updating the Relation Cache Table ***/
    /** Update relation catalog record entry (number of records in relation
        catalog is decreased by 1) **/
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);
    relCatEntry.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);
    // Get the entry corresponding to relation catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)
    RelCatEntry attrCatEntry;
    /** Update attribute catalog entry (number of records in attribute catalog
        is decreased by numberOfAttributesDeleted) **/
    // i.e., #Records = #Records - numberOfAttributesDeleted
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatEntry);
    attrCatEntry.numRecs -= numberOfAttributesDeleted;    
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatEntry);
    // Get the entry corresponding to attribute catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)

    return SUCCESS;
}


/*
NOTE: the caller is expected to allocate space for the argument `record` based
      on the size of the relation. This function will only copy the result of
      the projection onto the array pointed to by the argument.
*/
int BlockAccess::project(int relId, Attribute *record) {
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId,&prevRecId);
    // get the previous search index of the relation relId from the relation
    // cache (use RelCacheTable::getSearchIndex() function)

    // declare block and slot which will be used to store the record id of the
    // slot we need to check.
    int block, slot;

    /* if the current search index record is invalid(i.e. = {-1, -1})
       (this only happens when the caller reset the search index)
    */
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (new project operation. start from beginning)
	RelCatEntry relCatEntry;
	RelCacheTable::getRelCatEntry(relId,&relCatEntry);
        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
	block=relCatEntry.firstBlk;
	slot=0;
        // block = first record block of the relation
        // slot = 0
    }
    else
    {
        // (a project/search operation is already in progress)
	block=prevRecId.block;
	slot=prevRecId.slot+1;
        // block = previous search index's block
        // slot = previous search index's slot + 1
    }


    // The following code finds the next record of the relation
    /* Start from the record id (block, slot) and iterate over the remaining
       records of the relation */
    while (block != -1)
    {
        RecBuffer recblock(block);
        HeadInfo head;
        recblock.getHeader(&head);
        unsigned char slotmap[head.numSlots];
        recblock.getSlotMap(slotmap);
        // create a RecBuffer object for block (using appropriate constructor!)

        // get header of the block using RecBuffer::getHeader() function
        // get slot map of the block using RecBuffer::getSlotMap() function

        if(slot>=head.numSlots/* slot >= the number of slots per block*/)
        {
            block=head.rblock;
            slot=0;
            // (no more slots in this block)
            // update block = right block of block
            // update slot = 0
            // (NOTE: if this is the last block, rblock would be -1. this would
            //        set block = -1 and fail the loop condition )
        }
        else if (slotmap[slot]==SLOT_UNOCCUPIED/* slot is free */)
        { // (i.e slot-th entry in slotMap contains SLOT_UNOCCUPIED)
	    slot++;
            // increment slot
        }
        else {
            // (the next occupied slot / record has been found)
            break;
        }
    }

    if (block == -1){
        // (a record was not found. all records exhausted)
        return E_NOTFOUND;
    }

    // declare nextRecId to store the RecId of the record found
    RecId nextRecId{block, slot};
    RelCacheTable::setSearchIndex(relId,&nextRecId);
    // set the search index to nextRecId using RelCacheTable::setSearchIndex
    RecBuffer blockbuffer(block);
    blockbuffer.getRecord(record,slot);
    /* Copy the record with record id (nextRecId) to the record buffer (record)
       For this Instantiate a RecBuffer class object by passing the recId and
       call the appropriate method to fetch the record
    */

    return SUCCESS;
}
