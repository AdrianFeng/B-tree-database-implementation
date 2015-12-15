// this the temp file that store the previous deleted code 


/*/////////////////////////first version of the treeindex insert//////////////////////////////////////*/

RC BTreeIndex::insert(int key, const RecordId& rid)
{
    RC rc;PageId previousid;
    PageId sibiling=-1; int sibkey=-1; PageId parent =-1;
    rc =this->newinsert(key,rid,rootPid,sibiling,sibkey,parent);
    if (sibiling!=-1){
      printf("here 5\n");
      BTNonLeafNode root; root.read(rootPid,pf);
      rc = root.insert(sibkey,sibiling);
      printf("here 6\n");
      if (rc == RC_NODE_FULL){
        printf("here 9\n");
        int midKey;
         BTNonLeafNode secondone;
         PageId newsec = pf.endPid();
         printf("%d\n", newsec);
         printf("here 10\n");
         secondone.read(newsec,pf);
         root.insertAndSplit(sibkey,sibiling,secondone,midKey);
         secondone.setself(newsec);secondone.setasnon();
        previousid=pf.endPid();
        printf("here 11\n");
        BTNonLeafNode previous; previous.read(previousid,pf);
        previous.initializeRoot(rootPid,midKey,newsec);
        previous.setasnon(); previous.setParent(previousid);
        previous.setself(previousid);
        root.setParent(previousid);secondone.setParent(previousid);
        previous.write(previousid,pf);
        root.setParent(previousid);secondone.setParent(previousid);
        secondone.write(newsec,pf);
        rootPid=previousid;
      }
      printf("here 7\n");
      root.write(root.getself(),pf); 
      printf("here 8\n");
    }
    return 0;
}

RC BTreeIndex::newinsert(int key, const RecordId rid, PageId currentid, PageId& sibiling, int& sibkey, PageId& parent){
  RC rc;
  BTNonLeafNode current;
  PageId newpage;
  PageId previousid;
  int newkey;
  current.read(currentid, pf);
  printf("here 2\n");
  if (current.isLeaf()){
     printf("here 3\n");
     BTLeafNode firstone;
     firstone.read(currentid, pf);
     rc=firstone.insert(key,rid);
     firstone.setself(currentid);
     if (parent!=-1){
      firstone.setParent(parent);
     }
     if (rc==RC_NODE_FULL){
      newpage=pf.endPid();
      int siblingKey;
       BTLeafNode secondone;secondone.read(newpage,pf);
       firstone.insertAndSplit(key,rid,secondone,siblingKey);
       secondone.setself(newpage);secondone.setNextNodePtr(firstone.getNextNodePtr());
       firstone.setNextNodePtr(newpage);
       secondone.write(newpage,pf);
       sibiling=newpage;sibkey=siblingKey;
     }
     firstone.write(currentid,pf);
     return 0;
  }
  else{
      printf("here 4\n");
      previousid=currentid;
      PageId next = current.locateChildPtr( key, currentid);
      newpage=-1;sibkey=-1;
      this->newinsert(key,rid,currentid,newpage,sibkey,previousid);
    if (newpage!=-1){
      rc=current.insert(sibkey,newpage);
      if (rc==RC_NODE_FULL){
        int midKey;
        PageId nextnew = pf.endPid();
        BTNonLeafNode newsec; newsec.read(nextnew,pf);
        current.insertAndSplit(sibkey,newpage, newsec, midKey);
        newsec.setself(nextnew);newsec.setasnon();newsec.setParent(current.getParent());
        newsec.write(nextnew,pf);sibiling=nextnew;sibkey=midKey;
      }
      current.write(previousid,pf);
    }
    else{
      sibiling=-1;sibkey=-1;
    }
    return 0;
  }
     
}
/*//////////////////////////////end here ///////////////////////////////////////////////*/

/*/////////////////////////second version of the treeindex insert//////////////////////////////////////*/
if (Leaf1.isroot()){
      PageId parent_pid=pf.endPid();
      BTNonLeafNode parent_leaf;
      parent_leaf.read(parent_pid,pf);
      parent_leaf.initializeRoot(pid, FirKeyInSib, sibling_pid);
      parent_leaf.setasnon();
      Leaf1.setParent(parent_pid);sibling_leaf.setParent(parent_pid);
      sibling_leaf.setNextNodePtr(Leaf1.getNextNodePtr()); Leaf1.setNextNodePtr(sibling_pid);
      sibling_leaf.setself(sibling_pid);
      rootPid=parent_pid;
      Leaf1.write(pid,pf);sibling_leaf.write(sibling_pid,pf);parent_leaf.write(parent_pid,pf);
      return 0;
}
else{
  PageId parent_pid = Leaf1.getParent();
  BTNonLeafNode parent_leaf;
  parent_leaf.read(parent_pid,pf);
  parent_leaf.insert(FirKeyInSib,sibling_pid);
  if (rc ==RC_NODE_FULL){
    int non_leaf_mid_key;
    BTNonLeafNode parent_sibling;
    PageId parent_sibling_pid=pf.endPid();
    parent_sibling.read(parent_sibling_pid,pf);parent_sibling.setasnon();
    parent_leaf.insertAndSplit(FirKeyInSib, sibling_pid, parent_sibling, non_leaf_mid_key);
    sibling_leaf.setNextNodePtr(Leaf1.getNextNodePtr()); Leaf1.setNextNodePtr(sibling_pid);sibling_leaf.setself(sibling_pid);
    if (FirKeyInSib<non_leaf_mid_key){
      sibling_leaf.setParent(parent_pid);
    }else{
      sibling_leaf.setParent(parent_sibling_pid);
    }
    Leaf1.write(pid,pf);sibling_leaf.write(sibling_pid,pf);
    while(!parent_leaf.isroot()){
      BTNonLeafNode grand_parent;
      PageId grand_parent_id=parent_leaf.getParent();
      grand_parent.read(grand_parent_id.pf);
      int ano_non_leaf_mid_key;
      grand_parent.insert(non_leaf_mid_key,parent_sibling_pid);
      if (rc ==RC_NODE_FULL){
        BTNonLeafNode grand_parent_sibling;
        PageId grand_parent_sibling_pid=pf.endPid();
        grand_parent_sibling.read(grand_parent_sibling_pid,pf);grand_parent_sibling.setasnon();
        /////////////////////repeat////////////////////
        grand_parent.insertAndSplit(non_leaf_mid_key, parent_sibling_pid, grand_parent_sibling, ano_non_leaf_mid_key);
        parent_sibling.setNextNodePtr(parent_leaf.getNextNodePtr()); 
        parent_leaf.setNextNodePtr(parent_sibling_pid);parent_sibling.setself(parent_sibling_pid);
        if (non_leaf_mid_key<ano_non_leaf_mid_key){
          parent_sibling.setParent(grand_parent_id);
        }else{
          parent_sibling.setParent(grand_parent_sibling_pid);
        }
        parent_leaf.write(parent_pid,pf);parent_sibling.write(parent_sibling_pid,pf);
        //////////////////////end/////////////////////// 
      }

    }
    if (initflag){

    }

  }
  else {
    sibling_leaf.setParent(parent_pid);
    sibling_leaf.setNextNodePtr(Leaf1.getNextNodePtr()); Leaf1.setNextNodePtr(sibling_pid);
    sibling_leaf.setself(sibling_pid);
    rootPid=parent_pid;
    Leaf1.write(pid,pf);sibling_leaf.write(sibling_pid,pf);parent_leaf.write(parent_pid,pf);
    return 0;
  }
}
/*//////////////////////////////end here ///////////////////////////////////////////////*/


