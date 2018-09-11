#include <string.h>
#include <math.h>
#define FALSE 0
#define TRUE 1

typedef struct cacheBlock{
  int valid;
  unsigned long tag;
  int nruBit;
  struct cacheBlock* next;
}cacheBlock;

int setAssoc(const char*);
int setReplacementP(const char*);
void getBinaryFromHex(char[], char*);
int powerOfTwo(int);
void directMappedCache(cacheBlock**, unsigned long, char,unsigned long);
void getNumOfBits(int*, int*, int*, const char* );
void getBitValues(unsigned long*,unsigned long*, const char* , int, int,int, unsigned long);
void insertIntoCache(cacheBlock**, unsigned long, int, int);
void fullAssociativeCache(cacheBlock**, unsigned long, char);
void NwayAssociativeCache(cacheBlock**, unsigned long, char,unsigned long);
void fifoEviction(cacheBlock**, int);
void lruEviction(cacheBlock**, int);
void nruEviction(cacheBlock**, int);
void allNruBitsZero(cacheBlock**, int);
void changeLruLinkedList(cacheBlock**, int, int);
void printHead();
void printCache();
unsigned int l2( unsigned int);
/*cache spec*/
int cache_size;
int number_of_blocks;
int size_of_blocks;
int set_size;
int number_of_sets;
char* replacement_policy;
char mode;
char memoryHexValue[15];
char pc[15];
char* cache_type;
/*cache spec end*/
cacheBlock **cache;
//cacheBlock *head=NULL;
cacheBlock **head;
int cacheHits=0;
int cacheMiss=0;
int memoryRead=0;
int memoryWrite=0;
