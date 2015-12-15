#include "BTreeNode.h"
#include <vector>
#include <string.h>
using namespace std;

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf) //done
{
  RC rc; memset(buffer,0,PageFile::PAGE_SIZE);
  memcpy(buffer+1016,&pid,sizeof(PageId));
  if((rc=pf.read(pid,buffer))<0) return rc;
  return 0;
}
BTLeafNode::BTLeafNode(){memset(buffer,0,PageFile::PAGE_SIZE);}
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf) //done
{
  RC rc;
  if((rc=pf.write(pid,buffer))<0) return rc;
  return 0;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount() //done
{
  int count; //first 4 bytes decide to be the total keys.
  memcpy(&count,buffer,sizeof(int));
  return count;
}

/*
 * Insert a (key, rid) pair to the  node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid) //done
{
  int count=getKeyCount();
  if(count>=84) //means overflow
  {
    return RC_NODE_FULL;
  }
  else
  {
    int eid;
    RC rc;
    rc=locate(key,eid);
    if(rc==0){
      return -1 ; //error that nodes already exits
    }
    else if(rc==RC_NO_SUCH_RECORD){//we find the position to insert where is eid;
      int restrecord=count-eid-1;
      //moving the rest data to the right one more postion
      if(restrecord!=0) //there is rest data 
        memmove(buffer+sizeof(int)+(sizeof(int)+sizeof(RecordId))*(eid+2),buffer+sizeof(int)+(sizeof(int)+sizeof(RecordId))*(eid+1),(sizeof(int)+sizeof(RecordId))*restrecord);
      //copy the key
      memcpy(buffer+sizeof(int)+(sizeof(int)+sizeof(RecordId))*(eid+1),&key,sizeof(int));
      //copy the rid;
      memcpy(buffer+sizeof(int)*2+(sizeof(int)+sizeof(RecordId))*(eid+1),&rid,sizeof(RecordId));
      count=count+1;
      //increment the key total in page;
      memcpy(buffer,&count,sizeof(int)); 
    }
    else if(rc==RC_END_OF_TREE){ //this node is empty;
      PageId lala;
      memcpy(&lala,buffer+1016,sizeof(int));
      memcpy(buffer+sizeof(int),&key,sizeof(int));
      memcpy(buffer+sizeof(int)*2,&rid,sizeof(RecordId));
	    count=count+1;
      memcpy(buffer,&count,sizeof(int));
    }
    return 0;
  }
}

bool BTLeafNode::end(){
  int itself,last;
  memcpy(&itself,buffer+1016,sizeof(int));
  memcpy(&last,buffer+1020,sizeof(int));
  return itself==last;
}
/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, BTLeafNode& sibling, int& siblingKey)  //done
{
  int count=getKeyCount();
  int count2=sibling.getKeyCount();
  if(count2!=0) return RC_INVALID_PID;
  int eid;
  RC rc;
  rc=locate(key,eid); //count always be 84 even .
  int mid=count/2; //mid =even;
  int num=0;
  for(int i=mid+5;i<count;i++) // modify here zhen 
    {
      int key2; 
      RecordId rid2;
      readEntry(i,key2,rid2); //this read from itself and send it to his sibling.
      sibling.insert(key2,rid2); 
      num++;
    }
    count=count-num; //how many keys move to the sibling
    memcpy(buffer,&count,sizeof(int)); //update the count;
  if(eid>=mid+5) //go to his sibling. // modify here zhen 
  {   
    sibling.insert(key,rid);
  }
  else //go to itself
  {
    insert(key,rid);
  }
  RecordId uselessrid;
  sibling.readEntry(0,siblingKey,uselessrid);
  return 0;
}

/**
 * If searchKey exists in the node, set eid to the index entry
 * with searchKey and return 0. If not, set eid to the index entry
 * immediately after the largest index key that is smaller than searchKey,
 * and return the error code RC_NO_SUCH_RECORD.
 * Remember that keys inside a B+tree node are always kept sorted.
 * @param searchKey[IN] the key to search for.
 * @param eid[OUT] the index entry number with searchKey or immediately
                   behind the largest key smaller than searchKey.
 * @return 0 if searchKey is found. Otherwise return an error code.
 */
RC BTLeafNode::locate(int searchKey, int& eid) //done
{
  int count=getKeyCount();
  int key;
  eid=0;
  for(int i=0;i<count;i++)
  {
    //copy the buffer key to 
    memcpy(&key,buffer+sizeof(int)+(sizeof(int)+sizeof(RecordId))*i,sizeof(int));
    if(key==searchKey)
      {
	     eid=i;
	     return 0;
      }
    if(key>searchKey)
      {
	     eid=i-1;
	     return RC_NO_SUCH_RECORD;
      }
    eid=i;
  }
  if(count==0) return RC_END_OF_TREE; // empty node.
  return RC_NO_SUCH_RECORD; //there is not anything bigger than key.
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid) //done
{
  int count=getKeyCount();
  if(eid>count) //more then leafnode have , return error
  {
    return RC_NO_SUCH_RECORD;
  }
  else
  {
    memcpy(&key,buffer+sizeof(int)+(sizeof(int)+sizeof(RecordId))*eid,sizeof(int));
    memcpy(&rid,buffer+sizeof(int)*2+(sizeof(int)+sizeof(RecordId))*eid,sizeof(RecordId));
  }
  return 0;
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()  //last 4 bytes for next Pageid //done
{
  PageId pid; //last four byte
  memcpy(&pid,buffer+1020,sizeof(PageId));
  return pid;
}

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid) //set last 4 byte to new Pageid. //done
{
  memcpy(buffer+1020,&pid,sizeof(PageId));
  return 0;
}
void BTLeafNode::setParent(PageId pid){
  memcpy(buffer+1012,&pid,sizeof(PageId));}
PageId BTLeafNode::getParent(){
  PageId pid;
  memcpy(&pid,buffer+1012,sizeof(PageId));
  return pid;}
void BTLeafNode::setself(PageId pid){
  memcpy(buffer+1016,&pid,sizeof(PageId));}
PageId BTLeafNode::getself(){
  PageId pid;
  memcpy(&pid,buffer+1016,sizeof(PageId));
  return pid;}
/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf) //done
{
  RC rc;memset(buffer,0,PageFile::PAGE_SIZE);
  memcpy(buffer+1016,&pid,sizeof(PageId)); //copy itself pid in this node
  if((rc=pf.read(pid,buffer))<0) return rc;
  return 0;
}
BTNonLeafNode::BTNonLeafNode(){memset(buffer,0,PageFile::PAGE_SIZE);}
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)  //done
{
  RC rc;
  if((rc=pf.write(pid,buffer))<0)return rc;
  return 0;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount() //done
{
  int count; //first 4 bytes decide to be the total keys;
  memcpy(&count,buffer,sizeof(int));
  return count;
}


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{
  int count=getKeyCount();
  if(count>=84)
  {
    return RC_NODE_FULL;
  }
  else
  {
    int index=0;
    int searchkey;
    for(int i=0;i<count;i++)
    {
      memcpy(&searchkey,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*i,sizeof(int));
      if(key<searchkey)
	     {
	       index=i-1;
	       break;
	     }
      else if(key==searchkey)
	     {
	       return 0;
	     }
      else 
	   {
	     index=i;
	   }
   }
  int restrecord=count-index-1;
  if(restrecord!=0) memmove(buffer+sizeof(int)+sizeof(PageId)+(sizeof(PageId)+sizeof(int))*(index+2),buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*(index+1),(sizeof(int)+sizeof(PageId))*restrecord);
  memcpy(buffer+sizeof(int)+sizeof(PageId)+(sizeof(PageId)+sizeof(int))*(index+1),&key,sizeof(int));
  memcpy(buffer+sizeof(int)*2+(sizeof(PageId)+sizeof(int))*(index+1)+sizeof(PageId),&pid,sizeof(PageId));     
  // add 1 to the total key insize the Nonleaf node
  count=count+1;
  memcpy(buffer,&count,sizeof(int));
  }
  return 0;
}

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{ 
  int count=getKeyCount();
  int mid=count/2,index=0,searchkey,tempkey,midkey,keyaftermid;
  PageId mid_1pid,midpid,pidaftermid,temppid;
  for(int i=0;i<count;i++){ //find the postion of key
      memcpy(&searchkey,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*i,sizeof(int));
      if(key<searchkey){
	  index=i-1;
	  break;}
      else if(key>searchkey){
	index=i;}}
  if(index==mid-1){ //move up;
      midKey=key; //new key need to move up;
      memcpy(&midkey,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*mid,sizeof(int));
      memcpy(&midpid,buffer+sizeof(int)*2+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*mid,sizeof(PageId));
      sibling.initializeRoot(pid,midkey,midpid);
       for(int i=mid+1;i<count;i++){ //move the rest into sibling.
        memcpy(&tempkey,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*i,sizeof(int));
        memcpy(&temppid,buffer+sizeof(int)*2+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*i,sizeof(PageId));
        sibling.insert(tempkey,temppid);}
        count=mid;
        memcpy(buffer,&count,sizeof(int)); //update count;
    }
  else if(index<mid-1){ //pick mid-1 move up;
      memcpy(&midKey,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*(mid-1),sizeof(int));
      memcpy(&mid_1pid,buffer+sizeof(int)*2+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*(mid-1),sizeof(PageId));
      memcpy(&midkey,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*mid,sizeof(int));
      memcpy(&midpid,buffer+sizeof(int)*2+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*mid,sizeof(PageId));
      sibling.initializeRoot(mid_1pid,midkey,midpid);
      for(int i=mid+1;i<count;i++){ //move the rest into sibling.
        memcpy(&tempkey,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*i,sizeof(int));
        memcpy(&temppid,buffer+sizeof(int)*2+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*i,sizeof(PageId));
        sibling.insert(tempkey,temppid);}
        count=mid-1;
        memcpy(buffer,&count,sizeof(int)); //update count;
      insert(key,pid);
    }
  else{ //pick mid move up
      memcpy(&midKey,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*mid,sizeof(int));
      memcpy(&midpid,buffer+sizeof(int)*2+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*mid,sizeof(int));
      memcpy(&pidaftermid,buffer+sizeof(int)*2+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*(mid+1),sizeof(PageId));
      memcpy(&keyaftermid,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*(mid+1),sizeof(int));
      sibling.initializeRoot(midpid,keyaftermid,pidaftermid);
      count=mid;
      memcpy(buffer,&count,sizeof(int));
      for(int i=mid+2;i<count;i++){ //move the rest into sibling.
        memcpy(&tempkey,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*i,sizeof(int));
          memcpy(&temppid,buffer+sizeof(int)*2+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*i,sizeof(PageId));
        sibling.insert(tempkey,temppid);}
      sibling.insert(key,pid);
    }
  return 0;}

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{
  int count=getKeyCount();
  int key;
  for(int i=0;i<count;i++){
    memcpy(&key,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*i,sizeof(int));
    if(key==searchKey){ //pid is locate at after the key;
	memcpy(&pid,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*i+sizeof(int),sizeof(PageId));
	return 0;}
    else if(key>searchKey){ //pid is locate at before the key;
	memcpy(&pid,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*i-sizeof(PageId),sizeof(PageId));
	return 0;}}
  memcpy(&pid,buffer+sizeof(int)+sizeof(PageId)+(sizeof(int)+sizeof(PageId))*count-sizeof(PageId),sizeof(PageId));
  return 0;
}

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{
  memcpy(buffer+sizeof(int),&pid1,sizeof(PageId));
  memcpy(buffer+sizeof(int)+sizeof(PageId),&key,sizeof(int));
  memcpy(buffer+sizeof(int)*2+sizeof(PageId),&pid2,sizeof(PageId));
  int count=1;
  memcpy(buffer,&count,sizeof(int));
  int nonleaf=-1;
  memcpy(buffer+1020,&nonleaf,sizeof(int));
  return 0;
}
void BTNonLeafNode::setParent(PageId pid){
  memcpy(buffer+1012,&pid,sizeof(PageId));}
PageId BTNonLeafNode::getParent(){
  PageId pid;
  memcpy(&pid,buffer+1012,sizeof(PageId));
  return pid;}
void BTNonLeafNode::setself(PageId pid){
  memcpy(buffer+1016,&pid,sizeof(PageId));}
PageId BTNonLeafNode::getself(){
  PageId pid;
  memcpy(&pid,buffer+1016,sizeof(PageId));
  return pid;}
bool BTNonLeafNode::isLeaf(){
  int a;
  memcpy(&a,buffer+1020,sizeof(int));
  return a>=0;}
bool BTNonLeafNode::isroot(){
  PageId selfpid,parentpid;
  memcpy(&selfpid,buffer+1016,sizeof(PageId));
  memcpy(&parentpid,buffer+1012,sizeof(PageId));
  return selfpid==parentpid;}
void BTNonLeafNode::setasnon(){
  PageId pid=-1;
  memcpy(buffer+1020,&pid,sizeof(PageId));
}
