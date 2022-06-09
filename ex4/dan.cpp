

#include "PhysicalMemory.h"
#define ABS(n) ((n<0)?(-n):(n))
#define MIN(n, m) ((n < m)?(n):(m))

uint64_t getRelevantPhysicalAddress (uint64_t address);
word_t get_table_index (uint64_t address, long long i);
bool is_frame_empty (word_t frame_index, long long for_amount);
word_t find_frame (word_t  (&branch)[TABLES_DEPTH], uint64_t target_page);
word_t create_table (word_t (&branch)[TABLES_DEPTH], uint64_t target_page);
void evict_frame (uint64_t target_page_to_evict_frame_index, uint64_t target_page_to_evict_index);

uint64_t calc_targ_page (uint64_t address);
uint64_t cyclic_dist_calc (uint64_t page, uint64_t candidate);
bool not_a_parent (word_t  (&branch)[TABLES_DEPTH], word_t index);
uint64_t calc_root_table_bits_in_virtual_address ();

uint64_t fix_page_number (uint64_t number, long long int i, int depth);

uint64_t get_the_offset_of_page_number (uint64_t number);

uint64_t calc_root_table_bits_in_virtual_address ()
{
  return VIRTUAL_ADDRESS_WIDTH - (OFFSET_WIDTH * TABLES_DEPTH);
}

uint64_t getRelevantPhysicalAddress (uint64_t virtualAddress)
{
  word_t address1 = 0;
  word_t table_entry;
  word_t address2;
  word_t table_index;
  uint64_t target_page = calc_targ_page (virtualAddress);
  word_t branch[TABLES_DEPTH] = {0};
  for (long long i = 0; i < TABLES_DEPTH - 1; ++i)
    {
      branch[i] = address1;
      table_index = get_table_index (virtualAddress, i);
      PMread (uint64_t (PAGE_SIZE * address1 + table_index), &table_entry);
      if (!table_entry)
        {
          word_t temp_frame_index = create_table (branch, target_page);
          PMwrite (uint64_t (PAGE_SIZE * address1 + table_index), temp_frame_index);
//          printRam();
          address1 = temp_frame_index;
        }
      else
        {
          address1 = table_entry;
        }
    }

  branch[TABLES_DEPTH - 1] = address1;
  table_index = get_table_index (virtualAddress, TABLES_DEPTH - 1);
  PMread (uint64_t (address1 * PAGE_SIZE + table_index), &address2);

  if (!address2)
    {
      uint64_t empty_frame = find_frame (branch, target_page);
      PMrestore (empty_frame, target_page);
      PMwrite (address1 * PAGE_SIZE + table_index, empty_frame);
      address2 = empty_frame;
    }
  return address2;
}

uint64_t calc_targ_page (uint64_t address)
{
  return (address >> OFFSET_WIDTH);
}
void
find_empty_frame (word_t frame_index, word_t *max_frame_index, bool *is_successful,
                  uint64_t target_page, uint64_t *target_page_to_evict_index, int depth,
                  word_t (&branch)[TABLES_DEPTH], word_t *res, uint64_t page_number,
                  uint64_t *evict_frame_index_page, word_t *frame_index_to_zero_evict, word_t parent_frame_index,
                  uint64_t *frame_index_parent_to_empty_table_unused, uint64_t *table_index_unused)
{
  word_t val;

  if (*max_frame_index < frame_index)
    { *max_frame_index = frame_index; }

  long long for_amount = depth != 1 ?
                         PAGE_SIZE : 1LL
                             << calc_root_table_bits_in_virtual_address ();

  if (is_frame_empty (frame_index, for_amount))
    {

      if (not_a_parent (branch, frame_index))
        {
          *is_successful = true;
          *res = frame_index;
          *frame_index_parent_to_empty_table_unused = parent_frame_index;
          *table_index_unused = get_the_offset_of_page_number (page_number);
        }
      return;
    }

  for (long long i = 0; i < for_amount; ++i)
    {
      if (*is_successful)
        {
          break;
        }
      PMread (frame_index * PAGE_SIZE + i, &val);

      if (val)
        {
          if (TABLES_DEPTH > depth)
            {
              find_empty_frame (
                  val, max_frame_index, is_successful,
                  target_page, target_page_to_evict_index, depth + 1,
                  branch, res, fix_page_number (page_number, i, depth),
                  evict_frame_index_page, frame_index_to_zero_evict, frame_index,
                  frame_index_parent_to_empty_table_unused, table_index_unused);
            }

            //here we calculate the cyclic distance
          else
            {
              if (*max_frame_index < val)
                { *max_frame_index = val; }

              uint64_t curr_page_num = (page_number << OFFSET_WIDTH) + i;

              if (cyclic_dist_calc (target_page, *target_page_to_evict_index)
                  < cyclic_dist_calc (target_page, curr_page_num)
                  && not_a_parent (branch, val))
                {
                  *target_page_to_evict_index = curr_page_num;
                  *evict_frame_index_page = val;
                  *frame_index_to_zero_evict = frame_index;
                }
            }
        }
    }
}

uint64_t get_the_offset_of_page_number (uint64_t number)
{
  return number & ((1LL << OFFSET_WIDTH) - 1);
}

uint64_t fix_page_number (uint64_t number, long long int i, int depth)
{
  if (depth == 1)
    { return i; }
  number = number << OFFSET_WIDTH;
  return number + i;
}

bool not_a_parent (word_t (&branch)[TABLES_DEPTH], word_t index)
{
  for (const auto i : branch)
    {
      if (index == i)
        { return false; }
    }
  return true;
}
uint64_t cyclic_dist_calc (uint64_t page_swapped_in, uint64_t candidate)
{
  return MIN((NUM_PAGES - ABS (((int64_t) page_swapped_in - (int64_t) candidate))), (ABS (
      ((int64_t) page_swapped_in - (int64_t) candidate))));
}
word_t create_table (word_t (&branch)[TABLES_DEPTH], uint64_t target_page)
{
  word_t frame_index = find_frame (branch, target_page);
  for (long long i = 0; i < PAGE_SIZE; ++i)
    {
      PMwrite (frame_index * PAGE_SIZE + i, 0);
    }
  return frame_index;

}

word_t find_frame (word_t (&branch)[TABLES_DEPTH], uint64_t target_page)
{
  bool is_successful = false;
  word_t max_frame_index = 0;
  word_t res;
  word_t frame_to_zero_evict = -1;
  uint64_t frame_index_page_evict = -1;
  uint64_t target_page_to_evict_index = target_page;
  uint64_t frame_index_parent_to_empty_table_unused = -1;
  uint64_t table_index_unused = -1;

  find_empty_frame (0, &max_frame_index, &is_successful,
                    target_page, &target_page_to_evict_index, 1,
                    branch, &res, 0,
                    &frame_index_page_evict, &frame_to_zero_evict, -1,
                    &frame_index_parent_to_empty_table_unused, &table_index_unused);

  if (!is_successful)
    {
      if (max_frame_index + 1 < NUM_FRAMES)
        {
          res = max_frame_index + 1;
        }
      else
        {
          evict_frame (frame_index_page_evict, target_page_to_evict_index);
          for (long long i = 0; i < PAGE_SIZE; ++i)
            {
              PMwrite (frame_index_page_evict * PAGE_SIZE + i, 0);
            }
          PMwrite ((frame_to_zero_evict * PAGE_SIZE + (get_the_offset_of_page_number (target_page_to_evict_index))), 0);
          res = word_t (frame_index_page_evict);
        }
    }
  else
    {
      PMwrite ((frame_index_parent_to_empty_table_unused * PAGE_SIZE + table_index_unused), 0);
    }
  return res;
}

void
evict_frame (uint64_t target_page_to_evict_frame_index, uint64_t target_page_to_evict_index)
{
//  printRam();
  PMevict (target_page_to_evict_frame_index, target_page_to_evict_index);
}

bool is_frame_empty (word_t frame_index, long long for_amount)
{
  int val;
  for (long long i = 0; i < for_amount; ++i)
    {
      PMread (frame_index * PAGE_SIZE + i, &val);
      if (val)
        { return false; }
    }
  return true;
}
word_t get_table_index (uint64_t address, long long i)
{
  if (!i)
    {
      return word_t (address >> (VIRTUAL_ADDRESS_WIDTH
                                 - calc_root_table_bits_in_virtual_address ()));
    }
  return word_t (get_the_offset_of_page_number (
      address >> (VIRTUAL_ADDRESS_WIDTH - calc_root_table_bits_in_virtual_address () - i * OFFSET_WIDTH)));
}

void VMinitialize ()
{
  uint64_t root_table_size = 1LL << calc_root_table_bits_in_virtual_address ();
  for (uint64_t i = 0; i < root_table_size; ++i)
    {
      PMwrite (i, 0);
    }
}

int VMread (uint64_t virtualAddress, word_t *value)
{
  if (TABLES_DEPTH >= NUM_FRAMES || virtualAddress >= VIRTUAL_MEMORY_SIZE)
    { return 0; }
  uint64_t address2;
  address2 = getRelevantPhysicalAddress (virtualAddress);
  PMread ((address2 * PAGE_SIZE
           + get_table_index (virtualAddress, TABLES_DEPTH)), value);

  return 1;
}

int VMwrite (uint64_t virtualAddress, word_t value)
{
  if (TABLES_DEPTH >= NUM_FRAMES || virtualAddress >= VIRTUAL_MEMORY_SIZE)
    { return 0; }
  uint64_t address2;
  address2 = getRelevantPhysicalAddress (virtualAddress);
  PMwrite ((address2 * PAGE_SIZE
            + get_table_index (virtualAddress, TABLES_DEPTH)), value);
//  printRam();
  return 1;
}