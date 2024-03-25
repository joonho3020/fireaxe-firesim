#ifndef __QSFP_H
#define __QSFP_H

#include "core/config.h"

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
#include <iomanip>
#include <array>
#include <fcntl.h> 
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

/**
 *  @brief Staging container for QSFP aurora transactions
 *
 *  Aurora interface transactions bound for the RTL-simulator and back are queued
 *  up in this data structure as they wait to be driven into and out of the
 *  verilator/VCS design.
 *
 *  Used for outward and inward QSFP data movement (see simif_t::to_qsfp,
 *  simit_t::from_qsfp).
 */
class qsfp_t {
public:
  qsfp_t(const FPGATopQSFPConfig &conf);
  ~qsfp_t() = default;

  uint64_t rx_bits_by_idx(int idx);
  bool rx_valid();
  bool tx_ready();
  bool channel_up();

  void tick(bool reset,
            bool tx_valid,
            std::vector<uint64_t> tx_bits,
            bool rx_ready);

  bool setup_shmem(char *owned_name,
                   char *other_name);

  uint64_t SHMEM_EXTRABYTES = 24;
  uint64_t SHMEM_NUMBYTES = 0;
  uint64_t SHMEM_BITSBY64 = 0;
  uint64_t SHMEM_NAME_SIZE = 256;

  struct qsfp_data_t {
    std::vector<uint64_t> bits;
    qsfp_data_t(const std::vector<uint64_t> bits_) {
      for (int i = 0; i < (int)bits_.size(); i++) {
        bits.push_back(bits_[i]);
      }
    }

    int len() {
      return (int)bits.size();
    }

    uint64_t bits_by_64(int idx) {
      return bits[idx];
    }
  };

private:
  const FPGATopQSFPConfig conf;
  uint8_t *ownedbuf;
  uint8_t *otherbuf;

  sem_t *ownedsem;
  sem_t *othersem;

  // shmem_setup checkpoints
  bool owned_shmem_opened = false;
  bool owned_shmem_ftruncated = false;
  bool other_shmem_opened = false;
  bool other_shmem_ftruncated = false;
  bool owned_shmem_mapped = false;
  bool other_shmem_mapped = false;
  bool owned_mutex_opened = false;
  bool other_mutex_opened = false;
  bool other_flag_zeroed = false;
  bool shmem_setup_complete = false;

  // retain reference to these for multiple shmem_setup calls
  char *shmemname_owned = nullptr;
  char *shmemname_other = nullptr;
  char *mutexname_owned = nullptr;
  char *mutexname_other = nullptr;
  int shmemfd_owned;
  int shmemfd_other;

  std::queue<qsfp_data_t> tx_queue;
  std::queue<qsfp_data_t> rx_queue;

  std::vector<uint64_t> rx_bits_saved;
  bool rx_valid_saved = false;
};

#endif // __QSFP_H
