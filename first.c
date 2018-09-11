#include<stdlib.h>
#include<stdio.h>
#include "first.h"

int main(int argc, char const *argv[]){
  FILE *fr;
  const char* fileName;

  if (argc!=6){
    printf("Invalid number of arguments!\n");
    return 0;
  }
  //getting the data from the command line and opening file
  cache_size=atoi(argv[1]);
  size_of_blocks=atoi(argv[4]);
  if (powerOfTwo(cache_size)==FALSE || powerOfTwo(size_of_blocks)==FALSE){
    printf("Cache size and size of the block must be a power of 2\n");
    return 0;
  }
  number_of_blocks = cache_size/size_of_blocks;
  int assoc = setAssoc(argv[2]);
  if (assoc!=0){
    printf("Invalid Associtivity!\n");
    return 0;
  }
  int setRP = setReplacementP(argv[3]);
  if(setRP!=0){
    printf("Invalid replacement policy!\n");
    return 0;
  }


  /*
      initializing the cache. cache is a 2D array where cache[i] represents a set while cache[i][j] represents a block
  */

  cache = (cacheBlock **)malloc(number_of_sets*sizeof(cacheBlock*));
  int i;
  for (i=0;i<number_of_sets;i++){
    cache[i]=(cacheBlock*)malloc(set_size*sizeof(cacheBlock));
  }
  for (i=0;i<number_of_sets;i++){
    int j;
    for (j=0;j<set_size;j++){
      cache[i][j].valid=FALSE;
      cache[i][j].next=NULL;
      cache[i][j].nruBit=1;

    }
  }
  /*
  this is the list of head nodes for each linkedlist(used by fifo or lru).
  head[i] denotes the pointer to the head of the linkList used by set i.
  */
  head = (cacheBlock **)malloc(number_of_sets*sizeof(cacheBlock*));
  int k;
  for (k=0;k<number_of_sets;k++){
    head[k]=NULL;
  }
  fileName = argv[5];
  fr = fopen(fileName,"r");
  if (!fr){
    printf("error\n");
    return 0;
  }

  while (!feof(fr)){
   int count = fscanf(fr,"%s\t%c\t%s\n",pc,&mode,memoryHexValue);
  if (count==3){
    //printf("%s %c %s\n",pc,mode,memoryHexValue);
    unsigned long memoryValue=0;
    //converts the hex memory address into unsigned long
    memoryValue = strtoul(memoryHexValue, (char**)0, 0);
    //printf("strtoul unsigned = %lu\n", memoryValue);
    //grabs the number of offSet, index, and tag bits
    int numOfOffsetBits = 0;
    int numOfIndexBits = 0;
    int numOfTagBits = 0;
    getNumOfBits(&numOfOffsetBits,&numOfTagBits,&numOfIndexBits,cache_type);
    printf("tag: %d index :%d offset %d num of sets %d\n",numOfTagBits,numOfIndexBits,numOfOffsetBits,number_of_sets );
    unsigned long tag =0;
    unsigned long index = 0;
    getBitValues(&tag,&index,cache_type, numOfOffsetBits, numOfIndexBits, numOfTagBits,memoryValue);
  //  printf(" memory: %s dec: %lu tag: %d index :%d offset %d actual index: %lu num of sets: %d \n",memoryHexValue,tag,numOfTagBits,numOfIndexBits,numOfOffsetBits,index,number_of_sets);
  //  printf(" %s  %lu \n",memoryHexValue,tag);

    if (strcmp(cache_type,"direct")==0)
      directMappedCache(cache,tag,mode,index);
    else if (strcmp(cache_type,"assoc")==0)
      fullAssociativeCache(cache,tag,mode);
    else if (strcmp(cache_type,"assoc:N")==0)
      NwayAssociativeCache(cache,tag,mode,index);



  }else{
      //do nothing

  }


}
//printf("%s\n",replacement_policy);
printf("Memory reads: %d\n",memoryRead );
printf("Memory writes: %d\n",memoryWrite );
printf("Cache hits: %d\n",cacheHits );
printf("Cache misses: %d\n",cacheMiss );
fclose(fr);
return 0;
}

//eviction policy is irrelevent for this
void directMappedCache(cacheBlock **cache, unsigned long tag, char mode,unsigned long index){
  if (mode=='R'){
    //mode is to read
    if (cache[index][0].valid==FALSE){
      //the valid bit is false. Cache miss
      cacheMiss++;
      //put the data in
      cache[index][0].valid=TRUE;
      cache[index][0].tag=tag;
      memoryRead++;
    }else{
      if (cache[index][0].tag==tag){
        ///this is when the tag matches our tag
        cacheHits++;
      }else{
        //bit is valid but is occupied by some other block
        cacheMiss++;
        cache[index][0].tag=tag;
        memoryRead++;
      }
    }
  }else{
    //mode is to write
    if (cache[index][0].valid==FALSE){
      //the valid bit is false. Cache miss
      cache[index][0].valid=TRUE;
      cache[index][0].tag=tag;
      cacheMiss++;
      memoryRead++;
      memoryWrite++;
    }else{
      if (cache[index][0].tag==tag){
        ///this is when the tag matches our tag
        cacheHits++;
        memoryWrite++;
      }else{
        //bit is valid but is occupied by some other block
        cacheMiss++;
        cache[index][0].tag=tag;
        memoryRead++;
        memoryWrite++;
      }
    }

  }
}


//this function reads and writes for a fully assoc cache
void fullAssociativeCache(cacheBlock **cache, unsigned long tag, char mode){

  if (mode=='R'){
    //mode is to read

    int i;
    //this loops through the entire cache, and checks if our tag matches any other tag. If it does it a hit
    for (i=0;i<number_of_blocks;i++){

        if (cache[0][i].valid==TRUE && cache[0][i].tag==tag){
          if (strcmp(replacement_policy,"lru")==0){
            changeLruLinkedList(cache,0,i);
          }

          cache[0][i].nruBit=0;
          cacheHits++;
          return;
        }
      }
      //if you have made it here, that means its a cache miss and we need to probe to find a spot to fit new info in
      cacheMiss++;
      memoryRead++;
      //-1 is the default index to insert (means no space was found )
      int indexToInsert=-1;
      //loops through the entire cache and looks for an empty spot
      for (i=0;i<number_of_blocks;i++){
        if (cache[0][i].valid==FALSE){
          //if a valid spot is found, the index to insert is set to that spot
          indexToInsert=i;
          break;
        }
      }
      if (indexToInsert!=-1){
        //if the index to insert is not -1 (no spot found), the new block is inserted!
        insertIntoCache(cache,tag,0,indexToInsert);
      }else{
        //if the index to insert is -1, this means no spot was found and eviction is neccesary
        if (strcmp(replacement_policy,"fifo")==0)
          fifoEviction(cache,0);
        if (strcmp(replacement_policy,"lru")==0)
          lruEviction(cache,0);
        if (strcmp(replacement_policy,"nru")==0)
          nruEviction(cache,0);



        //the eviction has now happened according the specified policy, there is now a spot open and this looks for that spot
        int i;
        for (i=0;i<number_of_blocks;i++){
          if (cache[0][i].valid==FALSE){
            indexToInsert=i;
            break;
          }
        }
        //the new block is finally inserted
        insertIntoCache(cache,tag,0,indexToInsert);

      }

  }else{

    //write mode
    int i;
    //this loops through the entire cache, and checks if our tag matches any other tag. If it does it a hit
    for (i=0;i<number_of_blocks;i++){
        if (cache[0][i].valid==TRUE && cache[0][i].tag==tag){
          if (strcmp(replacement_policy,"lru")==0)
            changeLruLinkedList(cache,0,i);

          cache[0][i].nruBit=0;
          cacheHits++;
          memoryWrite++;
          return;
        }
      }
      //if you have made it here, that means its a cache miss and we need to probe a way to fit new info in
      cacheMiss++;
      memoryRead++;
      memoryWrite++;
      //-1 is the default index to insert (means no space was found )

      int indexToInsert=-1;
      for (i=0;i<number_of_blocks;i++){
        if (cache[0][i].valid==FALSE){
          //if a valid spot is found, the index to insert is set to that spot
          indexToInsert=i;
          break;
        }
      }
      if (indexToInsert!=-1){
        //if the index to insert is not -1 (no spot found), the new block is inserted!
        insertIntoCache(cache,tag,0,indexToInsert);
      }else{
        //if the index to insert is -1, this means no spot was found and evictions is neccesary
        if (strcmp(replacement_policy,"fifo")==0)
          fifoEviction(cache,0);
        if (strcmp(replacement_policy,"lru")==0)
          lruEviction(cache,0);
        if (strcmp(replacement_policy,"nru")==0)
          nruEviction(cache,0);

        //the eviction has now happened according the specified policy, there is now a spot open and this looks for that spot
        int i;
        for (i=0;i<number_of_blocks;i++){
          if (cache[0][i].valid==FALSE){
            indexToInsert=i;
            break;
          }
        }
        //the new block is finally inserted
        insertIntoCache(cache,tag,0,indexToInsert);
      }
  }


}

void NwayAssociativeCache(cacheBlock **cache, unsigned long tag, char mode,unsigned long index){

  if (mode=='R'){
    //mode is to read
    int i;
    for (i=0;i<set_size;i++){
        if (cache[index][i].valid==TRUE && cache[index][i].tag==tag){
          if (strcmp(replacement_policy,"lru")==0)
            changeLruLinkedList(cache,index,i);
          cache[index][i].nruBit=0;

          cacheHits++;
          return;
        }
      }
    cacheMiss++;
    memoryRead++;
    //-1 is the default index to insert (means no space was found )
    int indexToInsert=-1;
    //loops through the entire cache and looks for an empty spot
    for (i=0;i<set_size;i++){
      if (cache[index][i].valid==FALSE){
        //if a valid spot is found, the index to insert is set to that spot
        indexToInsert=i;
        break;
      }
    }
    if (indexToInsert!=-1){
      //if the index to insert is not -1 (no spot found), the new block is inserted!
      insertIntoCache(cache,tag,index,indexToInsert);
    }else{
      //if the index to insert is -1, this means no spot was found and eviction is neccesary

      if (strcmp(replacement_policy,"fifo")==0)
        fifoEviction(cache,index);
       if(strcmp(replacement_policy,"lru")==0)
        lruEviction(cache,index);
      if (strcmp(replacement_policy,"nru")==0)
        nruEviction(cache,index);

      //the eviction has now happened according the specified policy, there is now a spot open and this looks for that spot
      int i;
      for (i=0;i<set_size;i++){
        if (cache[index][i].valid==FALSE){
          indexToInsert=i;
          break;
        }
      }
      //the new block is finally inserted
      insertIntoCache(cache,tag,index,indexToInsert);
    }

  }else{
    //write mode
    int i;

    //this loops through the entire cache, and checks if our tag matches any other tag. If it does it a hit
    for (i=0;i<set_size;i++){
        if (cache[index][i].valid==TRUE && cache[index][i].tag==tag){
          //if the replacement policy is lru, then the cacheblock that was a hit is brought to the front of the linked list
          if (strcmp(replacement_policy,"lru")==0)
            changeLruLinkedList(cache,index,i);

          cache[index][i].nruBit=0;
          cacheHits++;
          memoryWrite++;
          return;
        }
      }
      //if you have made it here, that means its a cache miss and we need to probe a way to fit new info in
      cacheMiss++;
      memoryRead++;
      memoryWrite++;
      //-1 is the default index to insert (means no space was found )

      int indexToInsert=-1;
      for (i=0;i<set_size;i++){
        if (cache[index][i].valid==FALSE){
          //if a valid spot is found, the index to insert is set to that spot
          indexToInsert=i;
          break;
        }
      }
      if (indexToInsert!=-1){
        //if the index to insert is not -1 (no spot found), the new block is inserted!
        insertIntoCache(cache,tag,index,indexToInsert);
      }else{

        //if the index to insert is -1, this means no spot was found and evictions is neccesary
        if (strcmp(replacement_policy,"fifo")==0)
          fifoEviction(cache,index);
        if (strcmp(replacement_policy,"lru")==0)
          lruEviction(cache,index);
        if (strcmp(replacement_policy,"nru")==0)
          nruEviction(cache,index);

        //the eviction has now happened according the specified policy, there is now a spot open and this looks for that spot
        int i;
        for (i=0;i<set_size;i++){
          if (cache[index][i].valid==FALSE){
            indexToInsert=i;
            break;
          }
        }
        //the new block is finally inserted
        insertIntoCache(cache,tag,index,indexToInsert);

      }
  }





}
///this functions is used by fully assoc and N way assoc to insert their data
///setIndex and blockIndex correspond to cache[s][b]
void insertIntoCache(cacheBlock **cache, unsigned long tag, int setIndex, int blockIndex){
    ///inserts the tag into the cache
    cache[setIndex][blockIndex].valid=TRUE;
    cache[setIndex][blockIndex].tag=tag;
    cache[setIndex][blockIndex].nruBit=0;
    //this is used to modify the linked list according the replacement policy (FIFO OR LRU)
    if (strcmp(replacement_policy,"fifo")==0){
    if (head[setIndex]==NULL){
      //if head is null then the block is at the top of the list
      head[setIndex] = &cache[setIndex][blockIndex];
    }else{
      //otherwise it will be at the bottom of the list.
      cacheBlock* counter = head[setIndex];
      while(counter->next!=NULL){
        counter=counter->next;
      }
        counter->next=&cache[setIndex][blockIndex];
    }
  }
  if (strcmp(replacement_policy,"lru")==0){
    if (head[setIndex]==NULL){
      //this is the first node, make it the head of the list
      head[setIndex] = &cache[setIndex][blockIndex];
      //printf("head is %lu \n", head[setIndex]->tag);
    }else{
      //the node needs to be the new head
      cache[setIndex][blockIndex].next = head[setIndex];
      head[setIndex] = &cache[setIndex][blockIndex];
      //printf("head is %lu\n", head[setIndex]->tag);
      //printf("next is %lu \n", (head[setIndex]->next)->tag);
    }


  }
}

//used to get the number of tag/bitoffset/index bits
void getNumOfBits(int *numOfOffsetBits, int *numOfTagBits, int *numOfIndexBits, const char* cacheType){
    if (strcmp(cacheType,"direct")==0){
     *numOfOffsetBits = l2(size_of_blocks);
     *numOfIndexBits = l2(number_of_blocks);
     *numOfTagBits = 48-(*numOfOffsetBits)-(*numOfIndexBits);
   }
   if (strcmp(cacheType,"assoc")==0){
     *numOfOffsetBits=l2(size_of_blocks);
     *numOfIndexBits=0;
     *numOfTagBits = 48-(*numOfOffsetBits)-(*numOfIndexBits);
   }
   if (strcmp(cacheType,"assoc:N")==0){
     *numOfOffsetBits = l2(size_of_blocks);
     *numOfIndexBits = l2(number_of_sets);
     *numOfTagBits = 48-(*numOfOffsetBits)-(*numOfIndexBits);
   }
}

//used to extract bit values from the inputted 48 bit string
void getBitValues(unsigned long *tag,unsigned long *index, const char* cacheType, int numOfOffsetBits, int numOfIndexBits,int numOfTagBits, unsigned long memoryValue){
  if (strcmp(cacheType,"direct")==0){
     *tag = (memoryValue>>(numOfIndexBits+numOfOffsetBits));
    //this pushes the index bits to the end of the bit stream
    *index = (memoryValue>>numOfOffsetBits);
    //mask is used to extract the end N bits
    unsigned long indexMask = (1<<numOfIndexBits)-1;
    *index = (*index)&indexMask;
  }
  if (strcmp(cacheType,"assoc")==0){
    *tag = (memoryValue>>(numOfIndexBits+numOfOffsetBits));
    *index=2;
  }
  if (strcmp(cacheType,"assoc:N")==0){
     *tag = (memoryValue>>(numOfIndexBits+numOfOffsetBits));
    //this pushes the index bits to the end of the bit stream
    *index = (memoryValue>>numOfOffsetBits);
    //mask is used to extract the end N bits
    unsigned long indexMask = (1<<numOfIndexBits)-1;
    *index = (*index)&indexMask;
  }


}
//used to evict based on LRU policy
void lruEviction(cacheBlock **cache, int evictionSet){
  cacheBlock* current = head[evictionSet];
  cacheBlock* prev = NULL;
  while(current->next!=NULL){
    prev=current;
    current=current->next;
  }
  prev->next = NULL;
  current->next = NULL;
  unsigned long tagToBeRemoved = current->tag;
  //printf("To be evicted %lu \n",tagToBeRemoved);
  int i;
  for (i=0;i<set_size;i++){
    if (cache[evictionSet][i].tag==tagToBeRemoved){
      cache[evictionSet][i].valid=FALSE;
    }
  }
}
//used to evict based on FIFO policy

void fifoEviction(cacheBlock **cache, int evictionSet){
  unsigned long tag = head[evictionSet]->tag;
  int i;
  for (i=0; i<set_size;i++){
    if (cache[evictionSet][i].tag==tag){
      //block for eviction found! throw it out
      head[evictionSet]=head[evictionSet]->next;
      cache[evictionSet][i].valid=FALSE;
      cache[evictionSet][i].next=NULL;
      return;
    }
  }
}
//used to evict based on nru policy

void nruEviction(cacheBlock **cache, int evictionSet){
  //used the function to check if all bits are zero and flips them to one
  allNruBitsZero(cache,evictionSet);
  //evicts the first block with nru bit 1
  int i;
  for (i=0;i<set_size;i++){
    if (cache[evictionSet][i].nruBit==1){
      cache[evictionSet][i].valid=0;
      return;
    }
  }

}

//helper function that checks if all the nru bits are zero and if they are, they are all flipped to one!
void allNruBitsZero(cacheBlock **cache, int evictionSet){
  int i;
  for (i=0;i<set_size;i++){
    if (cache[evictionSet][i].nruBit==1){
      return;
    }
  }
  //if you have made it this far, the set has all bits with NRU zero
  //sets all the bits to one!
  for (i=0;i<set_size;i++){
    cache[evictionSet][i].nruBit=1;
  }

}

//helper function used to bring the cacheBlock the was most recently used to the front of the list
void changeLruLinkedList(cacheBlock **cache, int set, int block){
  unsigned long tag = cache[set][block].tag;
  if (head[set]->tag==tag){
    //do nothing as the block is already in the proper place
    return;
  }
  cacheBlock* current = head[set];
  cacheBlock* prev = NULL;

  while(current!=NULL && current->tag!=tag){
    prev=current;
    current=current->next;
  }
  //now current is the block that needs to be moved to the front of the list.
  prev->next = current->next;
  current->next=head[set];
  head[set] = current;
}






int setReplacementP(const char* input){
  int bad=0;
  if (strcmp(input,"fifo")==0){
    replacement_policy="fifo\0";
  }else if (strcmp(input,"lru")==0){
    replacement_policy="lru\0";
  }else if (strcmp(input,"nru")==0){
    replacement_policy="nru\0";
  }else {
    bad=1;
  }

  return bad;
}
/*sets the "set size" of the cache depending on what the input was*/
int setAssoc(const char* input){
  int bad=0;
  if (strcmp(input,"direct")==0){
    set_size=1;
    number_of_sets=number_of_blocks;
    cache_type="direct\0";
  }else if (strcmp(input,"assoc")==0){
    set_size=number_of_blocks;
    number_of_sets=1;
    cache_type="assoc\0";
  }else{
    char *firstOcc = strstr(input,"assoc:");
    if (firstOcc!=NULL){

      int offsetToN=strlen("assoc:");
      set_size=atoi(input+offsetToN);
      if (powerOfTwo(set_size)==FALSE){
        bad=1;
        return bad;
      }
      number_of_sets=number_of_blocks/set_size;
      cache_type="assoc:N\0";
    }else{
      bad=1;
    }
  }
  return bad;
}



//my math.h was not linking. This is (supposedly) the implementation of the log2 function
unsigned int l2( unsigned int x )
{
  unsigned int ans = 0 ;
  while( x>>=1 ) ans++;
  return ans ;
}
//some clever bitwise to quickly check if it is power of 2!
int powerOfTwo(int x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}
void printHead(){
  int k;
  printf("------Start List----------\n" );
for (k=0;k<number_of_sets;k++){
    cacheBlock* counter = head[k];
    printf("%d -> ",k );
    while(counter!=NULL){
      printf("%lu ", counter->tag);
      counter=counter->next;
    }
    printf("\n");
}
printf("------end List----------\n" );
}

void printCache(){
int i;
printf("---start cache----\n");
for (i=0;i<number_of_sets;i++){
  int j;
  printf("set %d\n",i);
  for ( j=0;j<set_size;j++){
      if (cache[i][j].valid==TRUE)
        printf("%lu ",cache[i][j].tag);
  }
  printf("\n");
}
printf("---end cache----\n");
}
