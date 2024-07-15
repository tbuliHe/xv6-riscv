// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
//buddy system
#define M_BLOCK 1024*1024
#define BYTE_SIZE 8
uint64 tree[1024*1024*2-1];
struct spinlock memlock;

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

int judge(uint64 n){
  return !(n & (n-1));
}

int mem_modify(int i){
  int temp = 2;
  for(;;temp*=2){
    if(i>temp && i<temp*2){
      return temp * 2;
    }
  }
}
int left_leaf(int i){
  return 2*i+1;
}
int right_leaf(int i){
  return 2*i+2;
}
int parent(int child){
  return (child+1)/2-1;
}
uint64 max(uint64 x,uint64 y){
  if(x>=y)return x;
  else return y;
}
void *mem_malloc(int n){
  acquire(&memlock);
  uint64 size = (n + BYTE_SIZE - 1) / BYTE_SIZE;  // 向上取整
  uint64 temp;
  uint64 index = 0;

  if(size == 0){
    size = 1;
  }else if(!judge(size)){
    size = mem_modify(size);  // 确保 size 是 2 的幂次方
  }

  if(tree[index] < size){
    printf("Memory is not enough, please free in time!\n");
    release(&memlock);  // 释放锁
    return (void*)0;
  }

  for(temp = M_BLOCK; temp != size; temp /= 2){
    if(tree[left_leaf(index)] >= size){
      index = left_leaf(index);
    }else if(tree[right_leaf(index)] >= size){
      index = right_leaf(index);
    }else{
      printf("Your Memory is short for allocating %d Byte", n);
      release(&memlock);  // 释放锁
      return (void*)0;
    }
  }

  tree[index] = 0;
  uint64 heap_offset = (index + 1) * temp - M_BLOCK;
  
  // 添加调试打印语句
 //printf("temp: %d, size:%d, index: %d, heap_offset: %d, heap_offset * BYTE_SIZE: %d\n", temp, size, index, heap_offset, heap_offset * BYTE_SIZE);

  while(index != 0){
    index = parent(index);
    tree[index] = max(tree[left_leaf(index)], tree[right_leaf(index)]);
  }

  void *allocated_memory = (void *)(HEAPSTART + heap_offset * BYTE_SIZE);
  memset(allocated_memory, -1, size * BYTE_SIZE);  // 确保不会越界访问
  release(&memlock);

  printf("Successfully allocate %d B \n", n);
  printf("The allocated memory is %d B from 0x%p to 0x%p\n", size * BYTE_SIZE, allocated_memory, allocated_memory + size * BYTE_SIZE);
  return allocated_memory;
}

void mem_free(void *pointer,int flag){
  acquire(&memlock);
  uint64 temp;
  uint64 index = 0;
  uint64 left_num;
  uint64 right_num;
  if(flag==1&&pointer==(void*)HEAPSTART){
    memset(pointer,0,M_BLOCK*BYTE_SIZE);
    release(&memlock);
    printf("Initialized memory: %dB\n",M_BLOCK*BYTE_SIZE);
    return;
  }
  if(pointer==(void*)0){
    release(&memlock);
    return;
  }
  if(((uint64)pointer%BYTE_SIZE)!=0 || (uint64)pointer>=HEAPSTOP){
    panic("mfree");
  }
  temp = 1;
  index = ((uint64)pointer-HEAPSTART)/BYTE_SIZE + M_BLOCK -1;
  for(;tree[index]!=0;index = parent(index)){
    temp*=2;
    if(index==0){
      release(&memlock);
      return;
    }
  }
  tree[index] = temp;
  memset(pointer,0,temp*BYTE_SIZE);
  printf("%d B memory has been released,",temp*BYTE_SIZE);
  printf("from 0x%p to 0x%p \n",(uint64)pointer,(uint64)pointer+temp*BYTE_SIZE);
  while(index){
    index = parent(index);
    temp *= 2;
    left_num=tree[left_leaf(index)];
    right_num=tree[right_leaf(index)];
    if(left_num+right_num==temp){
      tree[index] = temp;
    }else{
      tree[index] = max(left_num,right_num);
    }
  }
  release(&memlock);
  return;
  }
  void mem_init(){
  initlock(&memlock, "heaplock");
  mem_free((void*)HEAPSTART,1);
  uint64 temp_size;
  int i;
  temp_size = 2*M_BLOCK;
  for(i = 0;i < 2*M_BLOCK-1;i++){
    if(judge(i+1)){
      temp_size /= 2;
    }
    tree[i] = temp_size;
  }
  printf("Memory allocation system is ready！\n");
}
