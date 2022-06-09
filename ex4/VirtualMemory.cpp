#include "VirtualMemory.h"
#include "MemoryConstants.h"
#include "PhysicalMemory.h"

#define MASK ((1LL << OFFSET_WIDTH) -1)

//----------------------------------------------
//--------------- Helper Functions -------------
//----------------------------------------------

// min function
int min (int a, int b){
  if (a > b){
    return b;
  }
  return a;
}

// abs function
int abs (int a){
  if (a < 0){
    return -a;
  }
  return a;
}

// checks if a frame has zeros in all places
bool isFrameAvailable(uint64_t frame_address){
  for (int i = 0; i < PAGE_SIZE; ++i)
  {
    word_t word = -1;
    PMread ((frame_address * PAGE_SIZE) + i, &word);
    if (word != 0){
      return false;
    }
  }
  return true;
}

/**
 * inits all the rows in the frame to zeros
 * @param frame_num
 */
void initFrame(unsigned int frame) {
    for (int i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frame + i, 0);
    }
}

// calculates the cyclic distance
unsigned int caculateCyclicDistance(word_t page_swapped_in, word_t p){
  return min(NUM_PAGES - abs(page_swapped_in - p), abs(page_swapped_in - p));
}

/**
 * performs a DFS algorithm on the current tree
 */
void DFS(word_t father_table, unsigned int depth, word_t curr_frame, uint64_t target_page, word_t curr_page, uint64_t parent,
         unsigned int* best_cyc_dist, uint64_t *page_to_evict, uint64_t* frame_to_evict,
         uint64_t* frame_to_evict_parent, bool* found_empty_frame, word_t* empty_frame_addr,
         uint64_t* empty_page_addr, unsigned int* max_frame_reached, uint64_t* empty_page_parent){

  if (*found_empty_frame){ // if we have already found an empty frame, stop the DFS
    return;
  }

  // check if we've reached the leaves
  if (depth == TABLES_DEPTH){
    // calculate the cyclical distance and return
    unsigned int cyc_dist = caculateCyclicDistance(target_page, curr_page);
    if (cyc_dist > *best_cyc_dist) {// update the frame to evict, and save its parent
      *best_cyc_dist = cyc_dist;  //save the calculated distance
      *page_to_evict = curr_page; // save the page intended to evict
      *frame_to_evict = curr_frame; // save the frame this page is in
      *frame_to_evict_parent = parent; // save its parent
    }
    return;
  }

  // check if next table is empty
  if( depth != 0 && curr_frame != father_table && isFrameAvailable (curr_frame) ){
    *found_empty_frame = true;
    *empty_frame_addr = curr_frame;
    *empty_page_addr = curr_page;
    *empty_page_parent = parent;
  }

  // start traversing the child nodes
  for (int i = 0; i < PAGE_SIZE; ++i)
    {
      // get next node in the tree
      word_t next_addr = 0; // will hold the address written in the next tree node
      word_t next_node = (curr_frame * PAGE_SIZE) + i; // the next tree node
      PMread (next_node, &next_addr);

      // if the child in not zero, go in
      if (next_addr != 0){
        (*max_frame_reached)++; // update num of frames reached
        word_t next_page = (curr_page << OFFSET_WIDTH) + i; // next virtual address

        DFS(father_table, depth + 1, next_addr, target_page, next_page, next_node,
            best_cyc_dist, page_to_evict, frame_to_evict,
            frame_to_evict_parent, found_empty_frame, empty_frame_addr,
            empty_page_addr, max_frame_reached, empty_page_parent);
      }
    }
}


/**
 * extracts the next table index from the address, according to the current depth
 * then depth == TABLES_DEPTH, the offset is returned
 * @param address
 * @param depth
 * @return
 */
unsigned int getNextTable(uint64_t address, uint64_t depth){
  return (address >> ((TABLES_DEPTH - depth) * OFFSET_WIDTH)) & MASK;
}

/**
 * handles page fault
 * @return the address of the new frame
 */
word_t findNewFrame(uint64_t v_address, word_t father_table) {
  // DFS
  /** recursion variables -----------------------------------------------------------------------------*/
  unsigned int depth = 0; // the current depth we are in
  word_t curr_frame = 0; // current frame we are in
  uint64_t target_page = v_address >> OFFSET_WIDTH; // address without offset
  word_t curr_page = 0; // current page we are in
  uint64_t parent = 0; // the physical address of the parent of the current node, that means, a specific row in a frame
  unsigned int best_cyc_dist = 0; // the maximum cyclic distance of all leaves
  uint64_t page_to_evict= 0; // virtual address of the page to evict
  uint64_t frame_to_evict= 0; // the physical address of the page to evict
  uint64_t frame_to_evict_parent= 0; // the physical address of the parent of the page to evict
  bool found_empty_frame= false; // did we find a table with only zeros?
  word_t empty_frame_addr = 0; // physical address of only zero frame
  uint64_t empty_page_addr = 0; // virtual address of only zero frame
  unsigned int max_frame_reached = 0; // number of frames we've visited in the DFS
  uint64_t empty_page_parent = 0; // the physical address of the parent of the all zero frame
  /**-------------------------------------------------------------------------------------------------*/

  DFS(father_table, depth, curr_frame, target_page, curr_page, parent,
      &best_cyc_dist, &page_to_evict, &frame_to_evict,
      &frame_to_evict_parent, &found_empty_frame, &empty_frame_addr,
      &empty_page_addr, &max_frame_reached, &empty_page_parent);

  // 1) look for a frame with only zeros
  if (found_empty_frame){
      PMwrite (empty_page_parent, 0);
      return empty_frame_addr;
  }
  // 2) else, take the next available frame in root table
  else if (!found_empty_frame && max_frame_reached + 1 < NUM_FRAMES){
    return max_frame_reached + 1;
  }
  // 3) else, evict
  else {
      PMwrite (frame_to_evict_parent , 0);
      PMevict (frame_to_evict, page_to_evict);
      return frame_to_evict;
  }
}

/**
 * the main function that translates the virtual address to the physical address
 * will be used by VMwrite / VMread
 * @param v_address the address to translate
 * @return
 */
word_t translateAddress(uint64_t v_address){

  // if the address is bigger than virtual memory size, return failure
  if (v_address >= VIRTUAL_MEMORY_SIZE){
    return -1;
  }

  word_t old_addr = 0;
  for (int depth = 0; depth < TABLES_DEPTH; ++depth)
    {
      unsigned int next_table = getNextTable (v_address, depth);
      int new_addr = -1; // init with something different from zero
      PMread((old_addr * PAGE_SIZE) + next_table, &new_addr);

      if (new_addr == 0) {
        // a) page fault!
        word_t new_frame = findNewFrame(v_address, old_addr);

        // if this is not the leaf (leaf is in TABLE_DEPTH - 1)
        if (depth < (TABLES_DEPTH-1) ){

          // b) initialize the new frame
          initFrame ((new_frame*PAGE_SIZE));

          // c) write the address of the new frame in the father table
          PMwrite ((old_addr * PAGE_SIZE) + next_table, new_frame);

          // d)
          old_addr = new_frame;
        }
        // if this is the leaf
        else {
            // b) restore
            PMrestore (new_frame, (v_address >> OFFSET_WIDTH));

            // c)
            PMwrite ((old_addr * PAGE_SIZE) + next_table, new_frame);

            // d)
            old_addr = new_frame;
        }

      } else {
        old_addr = new_addr;
      }
    }
  return old_addr;
}

//----------------------------------------------
//----------------- API Functions --------------
//----------------------------------------------

/*
 * Initialize the virtual memory.
 */
void VMinitialize() {
  initFrame(0);
}

/* Reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t *value) {
  if (value == nullptr) {
    return 0;
  }
  uint64_t address = translateAddress(virtualAddress);
  if ( (int)address == -1) {
    return 0;
  }
  unsigned int offset = virtualAddress & MASK;
  PMread((address * PAGE_SIZE) + offset, value);
  return 1;
}

/* Writes a word to the given virtual address.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value) {
  uint64_t address = translateAddress(virtualAddress);
  if ((int)address == -1) {
    return 0;
  }
  unsigned int offset = virtualAddress & MASK;
  PMwrite((address * PAGE_SIZE) + offset, value);
  return 1;
}