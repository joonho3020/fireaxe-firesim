#ifndef __HARDWARESAMPLERS_H_
#define __HARDWARESAMPLERS_H_

#include "generictrace_worker.h"

#include <map>
#include <random>
#include <stdexcept>
#include <unordered_map>

struct robState {
  bool     committing   : 1;
  bool     valid        : 1;
  bool     ready        : 1;
  bool     empty        : 1;
  bool     dispatching  : 1;
  bool     rollingback  : 1;
  bool     exception    : 1;
  bool     csrstall     : 1;
  uint8_t  instr_comm   : 4;
  uint8_t  instr_valid  : 4;
  uint16_t head_idx     : 16;
  uint16_t pnr_idx      : 16;
  uint16_t tail_idx     : 16;
};

struct instr {
  bool     committing    : 1;
  bool     valid         : 1;
  bool     misspeculated : 1;
  bool     flushes       : 1;
  uint8_t  padding       : 4;
  uint64_t address       : 56;
};

struct genericTraceToken512 {
  uint64_t cycle;
  uint64_t cpu_cycle;
  struct robState rob;
  struct instr instr_0;
  struct instr instr_1;
  struct instr instr_2;
  struct instr instr_3;
  struct instr instr_4;
};

struct genericTraceToken1024 {
  uint64_t cycle;
  uint64_t cpu_cycle;
  struct robState rob;
  struct instr instr_0;
  struct instr instr_1;
  struct instr instr_2;
  struct instr instr_3;
  struct instr instr_4;
  struct instr instr_5;
  struct instr instr_6;
  struct instr instr_7;
  struct instr instr_8;
  struct instr instr_9;
  struct instr instr_10;
  struct instr instr_11;
  struct instr instr_12;
};


struct flatSample {
  uint64_t tCycles;
  uint64_t tCommitting;
  uint64_t tStalling;
  uint64_t tDeferred;
  uint64_t tRollingback;
  uint64_t tException;
  uint64_t tMisspeculated;
  uint64_t tFlushed;
  uint64_t cCommitted;
  uint64_t cMisspeculated;
  uint64_t cFlushed;
  uint64_t cStalled;
};

class base_profiler : public generictrace_worker {
protected:
  enum profiler_state : unsigned int
    {
     off     = 0,
     next    = 1 << 0,
     armed   = 1 << 1,
     tagging = 1 << 2
    };
  std::unordered_map<uint64_t, struct flatSample> result;

  FILE *csvFile = stdout;
  bool fileOpened = false;
  std::string csvFileName = "stdout";
  std::string profilerName = "Base Profiler";
  bool customResult = false;
  uint64_t samplingPeriod = 0;
  uint64_t triggerCounter = 0;
  uint64_t lastTokenCycle = 0;
  uint64_t lastFlushCycle = 0;
  uint64_t randomStartOffset = 0;
  uint64_t randomOffset = 0;

  unsigned int state = off;
  uint64_t sampleCounter = 0;
  uint64_t attributeCycles = 0;
  uint64_t attributeSamples = 0;
  uint64_t nextPeriodCycle = 0;
  uint64_t nextSamplingCycle = 0;
  uint64_t lastSamplingCycle = 0;

  // TODO: Disabled due to unknown impact on simulation performance
  // those are trigger addreses that if seen will cause a flush of
  // the results
  std::vector<uint64_t> softTriggers;
  // This gap in TSC will cause a trigger point
  // Currently defaulted to 0.1 seconds (target clock)
  uint64_t triggerCycleGap = 0;
  // Every so many target cycles the results are dumped to the file
  // Currently defaulted to every 50 seconds (target clock)
  uint64_t flushCycleThreshold = 0;


  std::mt19937 randomGenerator;
  std::uniform_int_distribution<unsigned long> randomRange;
public:
  base_profiler(std::vector<std::string> &args, struct traceInfo info, std::string name, bool customResult = false);
  bool triggerDetection(struct genericTraceToken512 const &token);
  bool triggerDetection2(struct genericTraceToken1024 const &trace);
  virtual void flushResult();
  virtual void flushHeader();
  ~base_profiler();
};

class oracle_profiler : public base_profiler {
private:
  // Use some number magic (prime factorization with elimination) to avoid floating point calculus
  // by using those numbers for n instructions committed and dividing at the end
  // we get the correct weights, needs to be extended with the max coreWidth
  unsigned int const aggregate_magic[5] = {60,  30,  20,  15, 12};
  struct genericTraceToken512 lastRetired = {};
  bool deferred = false;
  bool stalled = false;
public:
  oracle_profiler(std::vector<std::string> &args, struct traceInfo info);
  ~oracle_profiler();
  void flushResult();
  void tick(char const * const data, unsigned int tokens);
};


class interval_plus_profiler : public base_profiler {
private:
  unsigned int const aggregate_magic[5] = {60,  30,  20,  15, 12};
  struct genericTraceToken512 lastRetired = {};
  bool deferred = false;
public:
  interval_plus_profiler(std::vector<std::string> &args, struct traceInfo info);
  ~interval_plus_profiler();
  void flushResult();
  void tick(char const * const data, unsigned int tokens);
};


class interval_profiler : public base_profiler {
private:
  struct genericTraceToken512 lastRetired = {};
  bool deferred = false;
public:
  interval_profiler(std::vector<std::string> &args, struct traceInfo info);
  void tick(char const * const data, unsigned int tokens);
};

class lynsyn_profiler : public base_profiler {
private:
  struct genericTraceToken512 lastRetired;
  uint64_t attributeSamples;
public:
  lynsyn_profiler(std::vector<std::string> &args, struct traceInfo info);
  void tick(char const * const data, unsigned int tokens);
};

class pebs_plus_profiler : public base_profiler {
private:
  unsigned int const aggregate_magic[5] = {60,  30,  20,  15, 12};
public:
  pebs_plus_profiler(std::vector<std::string> &args, struct traceInfo info);
  ~pebs_plus_profiler();
  void flushResult();
  void tick(char const * const data, unsigned int tokens);
};

class pebs_profiler : public base_profiler {
public:
  pebs_profiler(std::vector<std::string> &args, struct traceInfo info);
  void tick(char const * const data, unsigned int tokens);
};

class ibs_profiler : public base_profiler {
private:
  uint16_t tag;
  uint16_t post_tag;
  uint64_t evicted;
  uint64_t overrunning;
public:
  ibs_profiler(std::vector<std::string> &args, struct traceInfo info);
  ~ibs_profiler();
  void tick(char const * const data, unsigned int tokens);
};

class oracle_profiler_2 : public base_profiler {
private:
  // Use some number magic (prime factorization with elimination) to avoid floating point calculus
  // by using those numbers for n instructions committed and dividing at the end
  // we get the correct weights, needs to be extended with the max coreWidth
  unsigned int const aggregate_magic[8] = {840, 420, 280, 210, 168, 140, 120, 105};
  struct genericTraceToken1024 lastRetired = {};
  bool deferred = false;
  bool stalled = false;
public:
  oracle_profiler_2(std::vector<std::string> &args, struct traceInfo info);
  ~oracle_profiler_2();
  void flushResult();
  void tick(char const * const data, unsigned int traces);
};

#endif // __HARDWARESAMPLERS_H_
