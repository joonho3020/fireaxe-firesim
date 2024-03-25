#include "core/config.h"
#include "qsfp.h"

#include <errno.h>
#include <stdio.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <queue>
#include <vector>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <string>
#include <string.h>
#include <iomanip>
#include <array>
#include <fcntl.h> 
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

//#define DEBUG_QSFP


qsfp_t::qsfp_t(const FPGATopQSFPConfig &conf) : conf(conf) {
  SHMEM_NUMBYTES = (int)(conf.data_bits / 8);
  SHMEM_BITSBY64 = (int)(conf.data_bits / 64);
  for (int i = 0; i < SHMEM_BITSBY64; i++) {
    rx_bits_saved.push_back(0LL);
  }
}

bool qsfp_t::setup_shmem(char *owned_name,
                         char *other_name) {

  if (shmem_setup_complete)
    return true;

  // create shared memory regions
  if (shmemname_owned == nullptr) {
    shmemname_owned = (char *) malloc(SHMEM_NAME_SIZE * sizeof(char));
    strcpy(shmemname_owned, owned_name);
    strcat(shmemname_owned, "shmem");
  }
  if (shmemname_other == nullptr) {
    shmemname_other = (char *) malloc(SHMEM_NAME_SIZE * sizeof(char));
    strcpy(shmemname_other, other_name);
    strcat(shmemname_other, "shmem");
  }
  if (mutexname_owned == nullptr) {
    mutexname_owned = (char *) malloc(SHMEM_NAME_SIZE * sizeof(char));
    strcpy(mutexname_owned, owned_name);
    strcat(mutexname_owned, "mutex");
  }
  if (mutexname_other == nullptr) {
    mutexname_other = (char *) malloc(SHMEM_NAME_SIZE * sizeof(char));
    strcpy(mutexname_other, other_name);
    strcat(mutexname_other, "mutex");
  }

  int ftresult;
  int shm_flags = O_RDWR | O_CREAT;
  int usr_flags = S_IRWXU; // RWX access for other processes

  // Creating the shared memory space that "this" metasim owns
  if (!owned_shmem_opened) {
    std::cout << "creating owned shmem region: " << shmemname_owned << std::endl;
    shmemfd_owned = shm_open(shmemname_owned, shm_flags, usr_flags);
    if (shmemfd_owned == -1) {
      std::cout << "shm_open for owned failed, retrying in 1s..." << std::endl;
      usleep(1000000);
      return false;
    }
    owned_shmem_opened = true;
  }

  // Making sharedmem large enough for 256 bits + flag
  if (!owned_shmem_ftruncated) {
    std::cout << "ftruncating space for owned data and flag" << std::endl;
    ftresult = ftruncate(shmemfd_owned, SHMEM_NUMBYTES + SHMEM_EXTRABYTES);
    if (ftresult == -1) {
      std::cout << "ftruncate for owned failed, retrying in 1s..." << std::endl;
      usleep(1000000);
      return false;
    }
    owned_shmem_ftruncated = true;
  }

  // Opening the shared memory space that the "other" metasim owns
  if (!other_shmem_opened) {
    std::cout << "opening other shmem region: " << shmemname_other << std::endl;
    shmemfd_other = shm_open(shmemname_other, shm_flags, usr_flags);
    if (shmemfd_other == -1) {
      std::cout << "shm_open for other failed, retrying in 1s..." << std::endl;
      usleep(1000000);
      return false;
    }
    other_shmem_opened = true;
  }

  // Mapping the owned shared memory space
  if (!owned_shmem_mapped) {
    std::cout << "mapping owned shmem region: " << shmemname_owned << std::endl;
    ownedbuf = (uint8_t *) mmap(NULL,
                                SHMEM_NUMBYTES + SHMEM_EXTRABYTES,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED,
                                shmemfd_owned,
                                0);
    if (ownedbuf == MAP_FAILED) {
      std::cout << "mapping for owned failed, retrying in 1s..." << std::endl;
      usleep(1000000);
      return false;
    }
    owned_shmem_mapped = true;
    memset((void *) ownedbuf, 0, SHMEM_NUMBYTES + SHMEM_EXTRABYTES);
  }

  // Mapping the other shared memory space
  if (!other_shmem_mapped) {
    std::cout << "mapping other shmem region: " << shmemname_other << std::endl;
    otherbuf = (uint8_t *) mmap(NULL,
                                SHMEM_NUMBYTES + SHMEM_EXTRABYTES,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED,
                                shmemfd_other,
                                0);
    if (otherbuf == MAP_FAILED) {
      std::cout << "mapping for other failed, retrying in 1s..." << std::endl;
      usleep(1000000);
      return false;
    }
    other_shmem_mapped = true;
  }

  // At this point...
  //   1) This metasim's shared mem region has been created and
  //      mapped to this process's virtual mem.
  //   2) The other metasim's shared mem region has been created
  //      and mapped to this process's virtual mem.

  // Setting up owned write mutex
  if (!owned_mutex_opened) {
    std::cout << "creating owned mutex: " << mutexname_owned << std::endl;
    ownedsem = sem_open(mutexname_owned,
                        O_CREAT,
                        usr_flags,
                        0);
    if (ownedsem == SEM_FAILED) {
      std::cout << "sem_open for owned failed, retrying in 1s..." << std::endl;
      usleep(1000000);
      return false;
    }
    owned_mutex_opened = true;
  }

  // Connecting to other write mutex
  // If able to connect to other write mutex -> other write mutex has been created -> other shmem region has been ftruncated + mapped + zeroed
  if (!other_mutex_opened) {
    std::cout << "opening other mutex: " << mutexname_other << std::endl;
    othersem = sem_open(mutexname_other, 0);
    if (othersem == SEM_FAILED) {
      std::cout << "sem_open for " << mutexname_other << " failed, retrying in 1s..." << std::endl;
      usleep(1000000);
      return false;
    }
    other_mutex_opened = true;
    free(shmemname_owned);
    free(shmemname_other);
    free(mutexname_owned);
    free(mutexname_other);
  }

  //volatile uint64_t otherbuf_flag;
  //if (!other_flag_zeroed) {
  //  std::cout << "checking if other shmem flag is cleared" << std::endl;
  //  otherbuf_flag = *((uint64_t *) otherbuf);
  //  if (otherbuf_flag != 0) {
  //    std::cout << "other shmem flag is not cleared, retrying in 1s..." << std::endl;
  //    usleep(1000000);
  //    return false;
  //  }
  //  other_flag_zeroed = true;
  //  free(shmemname_owned);
  //  free(shmemname_other);
  //  free(mutexname_owned);
  //  free(mutexname_other);
  //}

  std::cout << "SHARED MEMORY AND MUTEX SETUP COMPLETE!!!" << std::endl;
  shmem_setup_complete = true;
  return true;
}

uint64_t qsfp_t::rx_bits_by_idx(int idx) {
  assert(idx < (int)rx_bits_saved.size());
  return rx_bits_saved[idx];
}

bool qsfp_t::rx_valid() {
  return rx_valid_saved;
}

bool qsfp_t::tx_ready() {
  return true; // infinite outbound queue is always ready to take data
}

bool qsfp_t::channel_up() {
  return true;
}

// RTL -> Host
void qsfp_t::tick(bool reset,
                  bool tx_valid,
                  std::vector<uint64_t> tx_bits,
                  bool rx_ready) {
  uint64_t *ownedbuf_cast = (uint64_t *) ownedbuf;
  uint64_t *otherbuf_cast = (uint64_t *) otherbuf;

  if (tx_valid && tx_ready()) {
#ifdef DEBUG_QSFP
  std::cout << "qsfp_t::tick " << "tx_valid: " << tx_valid
                               << " rx_ready: " << rx_ready
                               << " tx_ready: " << tx_ready()
                               << " rx_valid: " << rx_valid_saved << std::endl;
#endif
    assert(tx_bits.size() == SHMEM_BITSBY64);
    qsfp_data_t tx_data(tx_bits);
    tx_queue.push(tx_data);
  }

  uint64_t owned_flag = *(ownedbuf_cast+0);
  if (owned_flag == 0 && !tx_queue.empty()) {
    qsfp_data_t out_data = tx_queue.front();
    for (int i = 1; i <= out_data.len(); i++) {
      assert(out_data.len() == SHMEM_BITSBY64);
      *(ownedbuf_cast+i) = out_data.bits_by_64(i-1);
    }
    *(ownedbuf_cast+0) = 1;
    tx_queue.pop();
  }

  uint64_t other_flag = *(otherbuf_cast+0);
  if (other_flag == 1) {
    std::vector<uint64_t> rx_bits;
    for (int i = 1; i <= SHMEM_BITSBY64; i++) {
      rx_bits.push_back(*(otherbuf_cast+i));
    }
    *(otherbuf_cast+0) = 0;

    qsfp_data_t rx_data(rx_bits);
    rx_queue.push(rx_data);
  }

  if (!rx_queue.empty() && !reset) {
    qsfp_data_t rx_data = rx_queue.front();
    for (int i = 0; i < SHMEM_BITSBY64; i++) {
      rx_bits_saved[i] = rx_data.bits_by_64(i);
    }
    rx_valid_saved = true;
  } else {
    rx_valid_saved = false;
  }

  if (rx_valid() && rx_ready) {
    rx_queue.pop();
  }
}
