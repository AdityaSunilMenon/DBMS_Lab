#include "AttrCacheTable.h"
#include <cstdlib>
#include <cstring>

AttrCacheEntry* AttrCacheTable::attrCache[MAX_OPEN];

/* returns the attrOffset-th attribute for the relation corresponding to relId
NOTE: this function expects the caller to allocate memory for `*attrCatBuf`
*/
int AttrCacheTable::getAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry* attrCatBuf) {
  // check if 0 <= relId < MAX_OPEN and return E_OUTOFBOUND otherwise
  if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }
  // check if attrCache[relId] == nullptr and return E_RELNOTOPEN if true
  if (attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }
  // traverse the linked list of attribute cache entries
  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (strcmp(entry->attrCatEntry.attrName, attrName)==0) {

      // copy entry->attrCatEntry to *attrCatBuf and return SUCCESS;
      *attrCatBuf = entry->attrCatEntry;
      return SUCCESS;
    }
  }

  // there is no attribute at this offset
  return E_ATTRNOTEXIST;
}


int AttrCacheTable::getAttrCatEntry(int relId, int attrOffset, AttrCatEntry* attrCatBuf) {
  // check if 0 <= relId < MAX_OPEN and return E_OUTOFBOUND otherwise
  if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }
  // check if attrCache[relId] == nullptr and return E_RELNOTOPEN if true
  if (attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }
  // traverse the linked list of attribute cache entries
  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (entry->attrCatEntry.offset == attrOffset) {

      // copy entry->attrCatEntry to *attrCatBuf and return SUCCESS;
      *attrCatBuf = entry->attrCatEntry;
      return SUCCESS;
    }
  }

  // there is no attribute at this offset
  return E_ATTRNOTEXIST;
}

void AttrCacheTable::recordToAttrCatEntry(union Attribute record[ATTRCAT_NO_ATTRS],
                                          AttrCatEntry* attrCatEntry) {
  strcpy(attrCatEntry->relName, record[ATTRCAT_REL_NAME_INDEX].sVal);

  // copy the rest of the fields in the record to the attrCacheEntry struct
  strcpy(attrCatEntry->attrName, record[ATTRCAT_ATTR_NAME_INDEX].sVal);
  attrCatEntry->attrType=(int)record[ATTRCAT_ATTR_TYPE_INDEX].nVal;
  attrCatEntry->primaryFlag=(bool)record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal;
  attrCatEntry->rootBlock=(int)record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
  attrCatEntry->offset=(int)record[ATTRCAT_OFFSET_INDEX].nVal;
}

void AttrCacheTable::attrCatEntryToRecord(AttrCatEntry* attrCatEntry,union Attribute record[ATTRCAT_NO_ATTRS]) {
  strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal,attrCatEntry->relName);

  // copy the rest of the fields in the record to the attrCacheEntry struct
  strcpy(record[ATTRCAT_ATTR_NAME_INDEX].sVal,attrCatEntry->attrName);
  record[ATTRCAT_ATTR_TYPE_INDEX].nVal=(int)attrCatEntry->attrType;
  record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal=(bool)attrCatEntry->primaryFlag;
  record[ATTRCAT_ROOT_BLOCK_INDEX].nVal=(int)attrCatEntry->rootBlock;
  record[ATTRCAT_OFFSET_INDEX].nVal=(int)attrCatEntry->offset;
}

int AttrCacheTable::resetSearchIndex(int relId,char attrName[ATTR_SIZE])
{
	IndexId index = {-1,-1};
	
	return AttrCacheTable::setSearchIndex(relId,attrName,&index);
}

int AttrCacheTable::setSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId *searchIndex) {

  if(relId<0 || relId>=MAX_OPEN/*relId is outside the range [0, MAX_OPEN-1]*/) {
    return E_OUTOFBOUND;
  }

  if(AttrCacheTable::attrCache[relId]==nullptr/*entry corresponding to the relId in the Attribute Cache Table is free*/) {
    return E_RELNOTOPEN;
  }
  AttrCacheEntry *curr=AttrCacheTable::attrCache[relId];
  while(curr/* each attribute corresponding to relation with relId */)
  {
    if (strcmp(curr->attrCatEntry.attrName,attrName)==0/* attrName/offset field of the AttrCatEntry
        is equal to the input attrName/attrOffset */)
    {
      // copy the input searchIndex variable to the searchIndex field of the
      //corresponding Attribute Cache entry in the Attribute Cache Table.
      curr->searchIndex=*searchIndex;

      return SUCCESS;
    }
    curr=curr->next;
  }

  return E_ATTRNOTEXIST;
}
