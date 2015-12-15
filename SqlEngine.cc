/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include "Bruinbase.h"
#include "SqlEngine.h"
#include "BTreeIndex.h"
using namespace std;

// external functions and variables for load file and sql command parsing 
extern FILE* sqlin;
int sqlparse(void);

RC SqlEngine::run(FILE* commandline)
{
  fprintf(stdout, "Bruinbase> ");

  // set the command line input and start parsing user input
  sqlin = commandline;
  sqlparse();  // sqlparse() is defined in SqlParser.tab.c generated from
               // SqlParser.y by bison (bison is GNU equivalent of yacc)

  return 0;
}
RC SqlEngine::TreeSelection( int attr, bool& keyconstraints, bool& valueconstraints, const vector<SelCond>& cond){
  if (attr == 1 || attr ==4 ){
    if (cond.size() == 0){
      keyconstraints=true;
    }
    else {
      for (int i=0; i< cond.size();i++){
        if (cond[i].attr==1){
          if (cond[i].comp!=SelCond::NE){
            keyconstraints=true;
          }
        }
        else{
          valueconstraints=true;
        }
      }
    }
  }
  else{ // attr = 3 attr =4 
    for (int i=0; i< cond.size();i++){
      if (cond[i].attr==1){
        if (cond[i].comp!=SelCond::NE){
          keyconstraints=true;
        }
      }
      else{
        valueconstraints=true;
      }
    }
  }
  return 0;
}

RC SqlEngine::select(int attr, const string& table, const vector<SelCond>& cond)
{
  RecordFile rf;   // RecordFile containing the table
  RecordId   rid;  // record cursor for table scanning
  BTreeIndex MyTree;
  RC     rc;
  int    key;     
  string value;
  int    count;
  int    diff;
  bool keyconstraints=false;
  bool valueconstraints=false;
  rc = TreeSelection(attr, keyconstraints, valueconstraints,  cond);
  // two conditions we don't need to use the BTreeindex file
  // first one is that we don't have any indextree that has been stored
  // second is some conditions we must use the indextree
  //    if the attr is not count(*) we use the indextree if there is a NON-"<>" constraints on key 
  //    if the attr is count(*), we use the indextree if there is no constraints or if there is a NON-"<>" constraints on key 
  if ((rc = rf.open(table + ".tbl", 'r')) < 0) {
        fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
        return rc;
  }
  if (MyTree.open(table + ".idx", 'r')<0 || !keyconstraints){

      // scan the table file from the beginning
      rid.pid = rid.sid = 0;
      count = 0;
      while (rid < rf.endRid()) {
        // read the tuple
        if ((rc = rf.read(rid, key, value)) < 0) {
          fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
          goto exit_select;
        }

        // check the conditions on the tuple
        for (unsigned i = 0; i < cond.size(); i++) {
          // compute the difference between the tuple value and the condition value
              switch (cond[i].attr) {
              case 1:
              	diff = key - atoi(cond[i].value);
              	break;
              case 2:
              	diff = strcmp(value.c_str(), cond[i].value);
              	break;
              }

          // skip the tuple if any condition is not met
              switch (cond[i].comp) {
              case SelCond::EQ:
                	if (diff != 0) goto next_tuple;
                	break;
              case SelCond::NE:
                	if (diff == 0) goto next_tuple;
                	break;
              case SelCond::GT:
                	if (diff <= 0) goto next_tuple;
                	break;
              case SelCond::LT:
                	if (diff >= 0) goto next_tuple;
                	break;
              case SelCond::GE:
                	if (diff < 0) goto next_tuple;
                	break;
              case SelCond::LE:
                	if (diff > 0) goto next_tuple;
                	break;
              }
        }

        // the condition is met for the tuple. 
        // increase matching tuple counter
        count++;

        // print the tuple 
        switch (attr) {
        case 1:  // SELECT key
          fprintf(stdout, "%d\n", key);
          break;
        case 2:  // SELECT value
          fprintf(stdout, "%s\n", value.c_str());
          break;
        case 3:  // SELECT *
          fprintf(stdout, "%d '%s'\n", key, value.c_str());
          break;
        }

        // move to the next tuple
        next_tuple:
        ++rid;
      }

      // print matching tuple count if "select count(*)"
      if (attr == 4) {
        fprintf(stdout, "%d\n", count);
      }
      rc = 0;

      // close the table file and return
      exit_select:
      rf.close();
      return rc;
  }
  else { 
  // valueconstraints = true means we must read the underlying recordfile and valueconstraints == false means we don't need the file
    // first find the key max and min constraints 
    IndexCursor cursor;count=0;
    int count=0;
    std::set<int> keynotequal;
    std::set<int> keyequal;
    vector<SelCond> valuecon;
    MAX1 MAXKEY;
    MIN1 MINKEY;
    MAXKEY.max=INT_MAX;MAXKEY.equalornot=0;
    MINKEY.min=INT_MIN;MINKEY.equalornot=0;
    bool valuecheck=false;
    for (int i =0; i< cond.size(); i++){
       if (cond[i].attr==1){
         checkconstraints(cond, i, keyequal, keynotequal, &MAXKEY,&MINKEY);
       }
       else{
        valuecon.push_back(cond[i]);
        valuecheck=true;
       }
    }
    if (keyequal.size()!=0){
      for (std::set<int>::iterator it=keyequal.begin();it!=keyequal.end();++it){
        if (((*it<MAXKEY.max && MAXKEY.equalornot==0) || (*it<=MAXKEY.max && MAXKEY.equalornot==1)) 
             && ((*it>MINKEY.min && MINKEY.equalornot==0 )|| (*it>=MINKEY.min && MINKEY.equalornot==1))
             && (keynotequal.find(*it)==keynotequal.end())){
            rc=MyTree.locate(*it, cursor);
            if (rc==0){
              MyTree.readForward(cursor, key, rid);
              if(key == *it){
                if (valuecheck ){
                  if ((rc = rf.read(rid, key, value)) < 0)
                    fprintf(stderr, "Error: while reading a tuple from table 203 %s\n", table.c_str());
                  if (valueconstraintscheck(valuecon, value)){
                    print_the_value(attr,key,value);
                    count++;
                  }
                }
                else{
                  if(attr==1){
                    print_the_value(attr,key,"");
                  }
                  else if (attr!=4){
                    if ((rc = rf.read(rid, key, value)) < 0)
                      fprintf(stderr, "Error: while reading a tuple from table 215 %s\n", table.c_str());
                    print_the_value(attr,key,value);
                  }
                  count++; 
                }                
              }
            }
        }
      }
      if (attr == 4) {
        fprintf(stdout, "%d\n", count);
      }
    }
    else{
      RC temprc=0;
      rc=MyTree.locate(MINKEY.min, cursor);
      if (cursor.eid==-1){
        cursor.eid =0;
      }
      if (rc!=0 && rc != RC_END_OF_TREE && rc!=RC_NO_SUCH_RECORD)
        fprintf(stderr, "Error: while reading a tuple from table 231 %s\n", table.c_str());
      MyTree.readForward(cursor, key, rid);
      if (!(key>MINKEY.min && MINKEY.equalornot==0) && !(key>=MINKEY.min && MINKEY.equalornot==1) ){
         MyTree.readForward(cursor, key, rid);
      }
      int rc2;
      while((key<MAXKEY.max && MAXKEY.equalornot==0) || (key<=MAXKEY.max && MAXKEY.equalornot==1)){
        if (keynotequal.find(key) == keynotequal.end()){
          if (valuecheck){
            if ((rc = rf.read(rid, key, value)) < 0){
              fprintf(stderr, "Error: while reading a tuple from table 240 %s\n", table.c_str());}
            if (valueconstraintscheck(valuecon, value)){
              print_the_value(attr,key,value);
              count++;
            }
          }
          else{
            if(attr==1){
              print_the_value(attr,key,"");
            }
            else if (attr!=4){
              if ((rc = rf.read(rid, key, value)) < 0){
                fprintf(stderr, "Error: while reading a tuple from table 252 %s\n", table.c_str());}
              print_the_value(attr,key,value);
            }
            count++; 
          }
        }
        rc=MyTree.readForward(cursor, key, rid);
        if (rc == RC_END_OF_TREE){
          if((key<MAXKEY.max && MAXKEY.equalornot==0) || (key<=MAXKEY.max && MAXKEY.equalornot==1)) {
            if (keynotequal.find(key) == keynotequal.end()){
              if (valuecheck){
                if ((rc = rf.read(rid, key, value)) < 0){
                  fprintf(stderr, "Error: while reading a tuple from table 240 %s\n", table.c_str());}
                if (valueconstraintscheck(valuecon, value)){
                  print_the_value(attr,key,value);
                  count++;
                }
              }
              else{
                if(attr==1){
                  print_the_value(attr,key,"");
                }
                else if (attr!=4){
                  if ((rc = rf.read(rid, key, value)) < 0){
                    fprintf(stderr, "Error: while reading a tuple from table 252 %s\n", table.c_str());}
                  print_the_value(attr,key,value);
                } 
		            count++;
              }
            }  
          }
          break;
        }
      }
      if (attr == 4) {
        fprintf(stdout, "%d\n", count);
      }
    }
    rc = 0;
    rf.close();
    return rc;
  }
}

RC SqlEngine::load(const string& table, const string& loadfile, bool index)
{
  /* your code here */
  
  string inputline;
  string val;
  int key;
  RecordFile fp;
  RC rc;
  ifstream input_stream;
  input_stream.open(loadfile);
  int counter=0;
  BTreeIndex MyTree;
  RecordId rid;
  if (input_stream.fail()){
    cerr<<"not able to open file!, please check!";
    exit(1);
  }
  rc=fp.open(table+".tbl",'w');
  if (rc !=0){
    cerr<<"unable to create or open a table file! ";
    exit(rc);
  }
  if (!index){
    while(getline(input_stream,inputline )){
      parseLoadLine(inputline,key,val);
      rc=fp.append(key,val,rid);
      //printf("new append \n");
      if (rc!=0){
        cout<<"line number is :"<<counter<<endl;
        cerr<<"unable to write this tuple!";
        exit(rc);
      }
      counter++;
    }
  }
  else{
    //printf("good here 1\n");
    MyTree.open(table + ".idx", 'w');
    //printf("good here 2\n");
    while(getline(input_stream,inputline )){
        parseLoadLine(inputline,key,val);
        rc=fp.append(key,val,rid);
        if (rc!=0){
          cout<<"line number is :"<<counter<<endl;
          cerr<<"unable to write this tuple!";
          exit(rc);
        }
        rc=MyTree.insert(key, rid);
        if (rc!=0){
          cout<<"line number is :"<<counter<<endl;
          cerr<<"unable to write this tuple into the tree!";
          exit(rc);
        }
        counter++;
    }
    //printf("successfully insert %d \n", counter);
    MyTree.close();
  }
  fp.close();
  input_stream.close();
  return rc;
}

RC SqlEngine::parseLoadLine(const string& line, int& key, string& value)
{
    const char *s;
    char        c;
    string::size_type loc;
    
    // ignore beginning white spaces
    c = *(s = line.c_str());
    while (c == ' ' || c == '\t') { c = *++s; }

    // get the integer key value
    key = atoi(s);

    // look for comma
    s = strchr(s, ',');
    if (s == NULL) { return RC_INVALID_FILE_FORMAT; }

    // ignore white spaces
    do { c = *++s; } while (c == ' ' || c == '\t');
    
    // if there is nothing left, set the value to empty string
    if (c == 0) { 
        value.erase();
        return 0;
    }

    // is the value field delimited by ' or "?
    if (c == '\'' || c == '"') {
        s++;
    } else {
        c = '\n';
    }

    // get the value string
    value.assign(s);
    loc = value.find(c, 0);
    if (loc != string::npos) { value.erase(loc); }

    return 0;
}
// only parse the key since the the key is guaranteed the integer
void SqlEngine::checkconstraints(const vector<SelCond>& cond, int i, set<int> &equal, set<int> &notequal, MAX1 *MAX,
   MIN1 *MIN){
    if (cond[i].comp==SelCond::NE){
      notequal.insert( atoi(cond[i].value));
    }
    else if(cond[i].comp==SelCond::EQ){
      equal.insert( atoi(cond[i].value));
    }
    else if (cond[i].comp==SelCond::LT ){
      if (atoi(cond[i].value)<=MAX->max){
        MAX->max=atoi(cond[i].value);
        MAX->equalornot=0;
      }
    }
    else if (cond[i].comp==SelCond::LE){
      if (atoi(cond[i].value)<MAX->max){
        MAX->max=atoi(cond[i].value);
        MAX->equalornot=1;
      } 
    }
    else if (cond[i].comp==SelCond::GT ){
      if (atoi(cond[i].value)>=MIN->min){
        MIN->min=atoi(cond[i].value);
        MIN->equalornot=0;
      }
    }
    else{
      if (atoi(cond[i].value)>MIN->min){
        MIN->min=atoi(cond[i].value);
        MIN->equalornot=1;

      }
    }
}




bool SqlEngine::valueconstraintscheck(const vector<SelCond> cond, string value){
  int diff;
  for (int i =0 ; i< cond.size(); i++){
    diff = strcmp(value.c_str(), cond[i].value); 
    switch (cond[i].comp) {
      case SelCond::EQ:
          if (diff != 0) return false;
          else continue;
          break;
      case SelCond::NE:
          if (diff == 0) return false;
          else continue;
          break;
      case SelCond::GT:
          if (diff <= 0) return false;
          else continue;
          break;
      case SelCond::LT:
          if (diff >= 0) return false;
          else continue;
          break;
      case SelCond::GE:
          if (diff < 0) return false;
          else continue;
          break;
      case SelCond::LE:
          if (diff > 0) return false;
          else continue;
          break;
    }
  }
  return true;
}

void SqlEngine::print_the_value(int attr,int key, string value){
  switch (attr) {
        case 1:  // SELECT key
          fprintf(stdout, "%d\n", key);
          //printf(" select key" );
          break;
        case 2:  // SELECT value
          fprintf(stdout, "%s\n", value.c_str());
          //printf(" select value" );
          break;
        case 3:  // SELECT *
          fprintf(stdout, "%d '%s'\n", key, value.c_str());
          //printf(" select all" );
          break;
        }
}
