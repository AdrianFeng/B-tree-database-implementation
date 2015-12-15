/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "BTreeIndex.h"
#include "BTreeNode.h"
#include <vector>
using namespace std;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
    rootPid = -1;
}

/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::open(const string& indexname, char mode)
{
  RC rc;
  int count=0;
  char page[PageFile::PAGE_SIZE];
  if((rc=pf.open(indexname,mode))<0) return rc;
  PageId pid=pf.endPid();
  if(pid==0) {rootPid=0;} // empty
  else 
  {
      pid--;
      BTNonLeafNode tempNonLeafNode;
      tempNonLeafNode.read(pid,pf);
      while(!tempNonLeafNode.isroot())
	     {
	       pid=tempNonLeafNode.getParent();
	       tempNonLeafNode.read(pid,pf);
	     }
      rootPid=pid;
  }
  return 0;
}


/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
  rootPid = -1;  
  return pf.close();
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid){
  IndexCursor current_cursor; RC rc;
  rc= this->locate(key, current_cursor); // find  the pid of that leaf node which the key pair should be inserted 
  if (rc==0){return 0;}
  PageId pid =current_cursor.pid; // get that pid 
  BTLeafNode Leaf1;
  Leaf1.read(pid,pf);
  bool initflag=false;
  rc=Leaf1.insert(key,rid);
  if (rc == RC_NODE_FULL){
    PageId sibling_pid=pf.endPid();
    BTLeafNode sibling_leaf; int FirKeyInSib;
    sibling_leaf.read(sibling_pid,pf);sibling_leaf.write(sibling_pid,pf); // write back immediatelly 
    Leaf1.insertAndSplit(key, rid, sibling_leaf, FirKeyInSib);
    PageId parent_pid = Leaf1.getParent();
    bool rootornot= false;
    if (Leaf1.getParent() == Leaf1.getself()){rootornot=true;}
    rc =this->newinsert(FirKeyInSib,pid, sibling_pid, rootornot, parent_pid);
    if (rc==1){
      Leaf1.setParent(parent_pid);
    }
    sibling_leaf.setParent(parent_pid);
    if(Leaf1.end()){
      sibling_leaf.setNextNodePtr(sibling_leaf.getself());Leaf1.setNextNodePtr(sibling_pid);}
    else{
    sibling_leaf.setNextNodePtr(Leaf1.getNextNodePtr()); Leaf1.setNextNodePtr(sibling_pid);}
    sibling_leaf.setself(sibling_pid);
    Leaf1.write(pid,pf);sibling_leaf.write(sibling_pid,pf);
    return 0;
  }
  else{
    Leaf1.write(pid,pf);
    return 0;
  }
}

RC BTreeIndex::newinsert(int FirKeyInSib, PageId self_pid,PageId sibling_pid, bool rootornot, PageId & parent_id){
  RC rc;
  if (rootornot){
      PageId parent_pid=pf.endPid();
      BTNonLeafNode parent_leaf;
      parent_leaf.read(parent_pid,pf);parent_leaf.write(parent_pid,pf);parent_leaf.setself(parent_pid);
      parent_leaf.initializeRoot(self_pid, FirKeyInSib, sibling_pid);
      parent_leaf.setasnon();
      rootPid=parent_pid;
      parent_id=parent_pid;
      parent_leaf.setParent(parent_pid);
      parent_leaf.write(parent_pid,pf);
      return 1;
  }
  else {
    PageId parent_pid=parent_id;
    PageId copy = parent_id;
    BTNonLeafNode parent_leaf;
    parent_leaf.read(parent_pid,pf);
    rc=parent_leaf.insert(FirKeyInSib,sibling_pid);
    if (rc ==RC_NODE_FULL){
      int ano_FirKeyInSib;
      PageId current_sibling_id=pf.endPid();
      BTNonLeafNode current_sibling;
      current_sibling.read(current_sibling_id,pf);current_sibling.setasnon();current_sibling.write(current_sibling_id,pf);// write back immediatelly 
      parent_leaf.insertAndSplit(FirKeyInSib,sibling_pid,current_sibling,ano_FirKeyInSib);
      PageId grand_parent_id = parent_leaf.getParent();
      bool isroot= parent_leaf.isroot();
      rc=this->newinsert(ano_FirKeyInSib,parent_pid,current_sibling_id,isroot,grand_parent_id);
      if (rc==1){
        parent_leaf.setParent(grand_parent_id);
      }
      current_sibling.setParent(grand_parent_id);
      current_sibling.setself(current_sibling_id);
      parent_leaf.write(parent_pid,pf);current_sibling.write(current_sibling_id,pf);
      if (FirKeyInSib>=ano_FirKeyInSib){
        parent_id=current_sibling_id;
      }
      else {
        parent_id=copy;
      }
      return 2;
    }
    else{
      parent_leaf.write(parent_pid,pf);
      return 3;
    }
  }
}




/**
 * Run the standard B+Tree key search algorithm and identify the
 * leaf node where searchKey may exist. If an index entry with
 * searchKey exists in the leaf node, set IndexCursor to its location
 * (i.e., IndexCursor.pid = PageId of the leaf node, and
 * IndexCursor.eid = the searchKey index entry number.) and return 0.
 * If not, set IndexCursor.pid = PageId of the leaf node and
 * IndexCursor.eid = the index entry immediately after the largest
 * index key that is smaller than searchKey, and return the error
 * code RC_NO_SUCH_RECORD.
 * Using the returned "IndexCursor", you will have to call readForward()
 * to retrieve the actual (key, rid) pair from the index.
 * @param key[IN] the key to find
 * @param cursor[OUT] the cursor pointing to the index entry with
 *                    searchKey or immediately behind the largest key
 *                    smaller than searchKey.
 * @return 0 if searchKey is found. Othewise an error code
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor)
{
  PageId current=rootPid;
  BTNonLeafNode tempBTNonLeafNode;
  tempBTNonLeafNode.read(rootPid,pf);
  while(!tempBTNonLeafNode.isLeaf())
    {
      tempBTNonLeafNode.locateChildPtr(searchKey,current);
      tempBTNonLeafNode.read(current,pf);
    }
  //after while root now is leaf node;
  BTLeafNode tempBTLeafNode;
  tempBTLeafNode.read(current,pf);
  cursor.pid=current;
  RC rc;
  rc=tempBTLeafNode.locate(searchKey,cursor.eid);
  if(rc == RC_NO_SUCH_RECORD) {
   // when this is no such record, cursor.eid == -1 if the node is empty 

  }
    return rc;
}


/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid)
{ //done
  BTLeafNode tempBTLeafNode;
  tempBTLeafNode.read(cursor.pid,pf);
  int totalkey=tempBTLeafNode.getKeyCount();
  tempBTLeafNode.readEntry(cursor.eid,key,rid);
  if(cursor.eid+1==totalkey) //cursor need to move to next leafnode;
    {
      cursor.pid=tempBTLeafNode.getNextNodePtr();
      if (tempBTLeafNode.end())
      {
        return RC_END_OF_TREE;
      }
      cursor.eid=0;
    }
  else //dont need to move;
    {
      cursor.eid=cursor.eid+1; 
    }
    return 0;
}


 
