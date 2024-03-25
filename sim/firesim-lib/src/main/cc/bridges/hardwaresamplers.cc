#include "hardwaresamplers.h"


base_profiler::base_profiler(std::vector<std::string> &args, struct traceInfo info, std::string name, bool customResult)
  : generictrace_worker(info), profilerName(name), customResult(customResult) {
  if (info.tokenBytes != 512 / 8) {
    throw std::invalid_argument("profiling workers are optimized towards 512 bit trace tokens coming from the DMA interface");
  }

  if (info.coreWidth <= 0 || info.coreWidth > 8) {
    throw std::invalid_argument(std::string("Invalid core configuration detected. Please set core width and rob depth via core parameters. (Supported core width <= 8)"));
  }

  std::random_device rd;
  randomGenerator = std::mt19937(rd());

  unsigned int n = 0;
  for (auto &a: args) {
    try {
      // The first argument is the filename, after that any argument that starts with an 't'
      // is considered a address trigger and 'f' is considered a flush threshold
      if (n > 0) {
        if (a.at(0) == 'g') {
          triggerCycleGap = std::stoul(a.substr(1), nullptr, 0);
          continue;
        }
        if (a.at(0) == 'f') {
          flushCycleThreshold = std::stoul(a.substr(1), nullptr, 0);
          continue;
        }
        if (a.at(0) == 't') {
          fprintf(stderr, "%s: soft triggers are currently disabled, argument %s will have no impact\n", profilerName.c_str(), a.c_str());
          softTriggers.push_back(std::stoul(a.substr(1), nullptr, 0));
          continue;
        }
      }

      switch (n) {
      case 3:
        randomOffset = std::stol(a); n++; break;
      case 2:
        randomStartOffset = std::stol(a); n++; break;
      case 1:
        samplingPeriod = std::stol(a); n++; break;
      case 0:
        csvFileName = a;
        strReplaceAll(csvFileName, std::string("%id"), std::to_string(info.tracerId));
        csvFile = fopen(csvFileName.c_str(), "w");
        if (!csvFile)
          throw;
        fileOpened = true;
        n++; break;
      }
    } catch(...) {
      throw std::invalid_argument(std::string("inavlid profiler argument: ") + a);
    }
  }

  if (randomStartOffset > 0) {
    std::uniform_int_distribution<unsigned long> randomRangeStart(0, randomStartOffset);
    randomStartOffset = randomRangeStart(randomGenerator);
  }
  result.clear();

  if (samplingPeriod > 0 && randomOffset >= samplingPeriod) {
    fprintf(stdout, "%s: random offset cannot be bigger than the sampling period, reducing to %lu\n", profilerName.c_str(), samplingPeriod - 1);
    randomOffset = samplingPeriod - 1;
  }

  randomRange = std::uniform_int_distribution<unsigned long>(0, randomOffset);

  // Current implementation requires an output file!
  if (!csvFile) {
    throw std::invalid_argument("output file required");
  }

  fprintf(stdout, "%s: file(%s), sampling_period(%lu), random_start(%lu), random_offset(%lu), trigger_cycle_gap(%lu), flush_cycles(%lu)\n",
          profilerName.c_str(), csvFileName.c_str(), samplingPeriod, randomStartOffset, randomOffset, triggerCycleGap, flushCycleThreshold);

  // Offer derived classes handling the file output themselves
  if (!customResult) {
    flushHeader();
  }
}

/*
 * triggerDetection
 * - returns true if timing must be restarted
 * - flushes the results when triggered
  */
bool base_profiler::triggerDetection(struct genericTraceToken512 const &token) {
  bool gapTrigger = triggerCycleGap && (token.cycle - lastTokenCycle >= triggerCycleGap);
  bool flush = gapTrigger || (flushCycleThreshold && (token.cycle - lastFlushCycle >= flushCycleThreshold));

  // TODO: experimental soft triggers currently disabled
  // They will trigger when certain addresses are committed
  // Add soft triggers via the arguments e.g.: t0x10000
  /*
  if (!flush && !softTriggers.empty() && token.rob.committing) {
    for (auto &t : softTriggers) {
      if ((token.instr_0.committing && token.instr_0.address == t) ||
          (token.instr_1.committing && token.instr_1.address == t) ||
          (token.instr_2.committing && token.instr_2.address == t) ||
          (token.instr_3.committing && token.instr_3.address == t)) {
        flush = true;
        break;
      }
    }
  }
  */

  if (flush) {
    lastFlushCycle = token.cycle;
    triggerCounter++;
/* fprintf(csvFile, "# flush at cycle %lu\n", token.cycle); */
    flushResult();
  }

  // this method returns a bool whether timing must be restarted from this token
  // this is true only if a gap trigger was detected (tacing bridge didn't trace)
  // because of hardware triggers which are only detectable through unseen cycles
  // or if this is the first sample at all
  // soft triggers are flushing the results anyway no timing restart is required
  if (!lastTokenCycle)
    gapTrigger = true;
  lastTokenCycle = token.cycle;
  return gapTrigger;
}

bool base_profiler::triggerDetection2(struct genericTraceToken1024 const &trace) {
  bool gapTrigger = triggerCycleGap && (trace.cycle - lastTokenCycle >= triggerCycleGap);
  bool flush = gapTrigger || (flushCycleThreshold && (trace.cycle - lastFlushCycle >= flushCycleThreshold));

  if (flush) {
    lastFlushCycle = trace.cycle;
    triggerCounter++;
/* fprintf(csvFile, "# flush at cycle %lu\n", trace.cycle); */
    flushResult();
  }

  // this method returns a bool whether timing must be restarted from this token
  // this is true only if a gap trigger was detected (tacing bridge didn't trace)
  // because of hardware triggers which are only detectable through unseen cycles
  // or if this is the first sample at all
  // soft triggers are flushing the results anyway no timing restart is required
  if (!lastTokenCycle)
    gapTrigger = true;
  lastTokenCycle = trace.cycle;
  return gapTrigger;
}

void base_profiler::flushHeader() {
  fprintf(csvFile, "pc0;time;committing;stalling;deferred;rollingback;exception;misspeculated;flushes;ccommitting;cmisspeculated;cflushed;cstalled\n");
}

void base_profiler::flushResult() {
  for (auto &r: result) {
    fprintf(csvFile, "0x%lx;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld\n",
            r.first,
            r.second.tCycles,
            r.second.tCommitting,
            r.second.tStalling,
            r.second.tDeferred,
            r.second.tRollingback,
            r.second.tException,
            r.second.tMisspeculated,
            r.second.tFlushed,
            r.second.cCommitted,
            r.second.cMisspeculated,
            r.second.cFlushed,
            r.second.cStalled);
  }
  result.clear();
}

base_profiler::~base_profiler() {
  fprintf(stdout, "base_profiler destructor\n");
  fprintf(stdout, "%s: triggers(%lu), samples(%lu)\n", profilerName.c_str(), triggerCounter, sampleCounter);
  if (!customResult) {
    flushResult();
  }
  if (fileOpened) {
    fclose(csvFile);
  }
}

oracle_profiler::oracle_profiler(std::vector<std::string> &args, struct traceInfo info)
  : base_profiler(args, info, "OracleProfiler@" + std::to_string(info.tracerId), true) {
  fprintf(stdout, "oracle_profiler constructor\n");
  // We told the base profiler that we take care of the file output ourselves
  flushHeader();
}

void oracle_profiler::flushResult() {
/* fprintf(stdout, "oracle_profiler flushResult\n"); */
  for (auto &r: result) {
    sampleCounter +=
      r.second.tCommitting +
      r.second.tStalling +
      r.second.tDeferred +
      r.second.tRollingback +
      r.second.tException +
      r.second.tMisspeculated +
      r.second.tFlushed;

    fprintf(csvFile, "0x%lx;%f;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld\n",
            r.first,
            (double) r.second.tCycles / aggregate_magic[0],
            r.second.tCommitting,
            r.second.tStalling,
            r.second.tDeferred,
            r.second.tRollingback,
            r.second.tException,
            r.second.tMisspeculated,
            r.second.tFlushed,
            r.second.cCommitted,
            r.second.cMisspeculated,
            r.second.cFlushed,
            r.second.cStalled);
  }
  result.clear();
}

oracle_profiler::~oracle_profiler() {
  fprintf(stdout, "oracle_profiler destructor\n");
  // We need to call this method from the destructor of the derived class
  // because we have overwritten it and base_profiler would just call
  // its own as we are already destructed when base_profiler destructs
  flushResult();
}

void oracle_profiler::tick(char const * const data, unsigned int tokens) {
  struct genericTraceToken512 const * const trace = (struct genericTraceToken512 *) data;
  for (unsigned int i = 0; i < tokens; i ++) {
    struct genericTraceToken512 const token = trace[i];

/* #define DEBUG */
#ifdef DEBUG
      fprintf(stderr, "%lu | %lu | %d%d%d%d%d%d%d%d, %u, %u, %u | %d, %d",
              token.cycle,
              token.cpu_cycle,
              token.rob.committing,
              token.rob.valid,
              token.rob.ready,
              token.rob.empty,
              token.rob.dispatching,
              token.rob.rollingback,
              token.rob.exception,
              token.rob.csrstall,
              token.rob.head_idx,
              token.rob.pnr_idx,
              token.rob.tail_idx,
              token.rob.instr_comm,
              token.rob.instr_valid);
      fprintf(stderr, " | %d%d%d%d 0x%lx", token.instr_0.valid, token.instr_0.committing, token.instr_0.misspeculated, token.instr_0.flushes, token.instr_0.address);
      fprintf(stderr, " | %d%d%d%d 0x%lx", token.instr_1.valid, token.instr_1.committing, token.instr_1.misspeculated, token.instr_1.flushes, token.instr_1.address);
      fprintf(stderr, " | %d%d%d%d 0x%lx", token.instr_2.valid, token.instr_2.committing, token.instr_2.misspeculated, token.instr_2.flushes, token.instr_2.address);
      fprintf(stderr, " | %d%d%d%d 0x%lx", token.instr_3.valid, token.instr_3.committing, token.instr_3.misspeculated, token.instr_3.flushes, token.instr_3.address);
      fprintf(stderr, " | %d%d%d%d 0x%lx", token.instr_4.valid, token.instr_4.committing, token.instr_4.misspeculated, token.instr_4.flushes, token.instr_4.address);
      fprintf(stderr, "\n");
#endif
    if (triggerDetection(token)) {
      lastSamplingCycle = token.cycle;
      // Reset profiling, go out of deferred, zero out lastRetired
      deferred = false;
      stalled = false;
      lastRetired = {};
      continue;
    }

    // Detect wrong core configuration. Valid instructions at rob head must be within the coreWidth
    // assert(token.rob.instr_valid <= info.coreWidth);

    uint64_t const attributeCycles = token.cycle - lastSamplingCycle;
    if (token.rob.committing) {
      // Committing instructions
      uint64_t const cycleTime = attributeCycles * aggregate_magic[token.rob.instr_comm - 1];
      uint64_t const deferredCycles = deferred * attributeCycles;
      if (token.instr_0.committing) {
        struct flatSample &s = result[token.instr_0.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.tDeferred += deferredCycles;
        s.cCommitted += 1;
        s.cMisspeculated += token.instr_0.misspeculated;
        s.cFlushed += token.instr_0.flushes;
        s.cStalled += stalled; stalled = false;
      }
      if (token.instr_1.committing) {
        struct flatSample &s = result[token.instr_1.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.tDeferred += deferredCycles;
        s.cCommitted += 1;
        s.cMisspeculated += token.instr_1.misspeculated;
        s.cFlushed += token.instr_1.flushes;
        s.cStalled += stalled; stalled = false;
      }
      if (token.instr_2.committing) {
        struct flatSample &s = result[token.instr_2.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.tDeferred += deferredCycles;
        s.cCommitted += 1;
        s.cMisspeculated += token.instr_2.misspeculated;
        s.cFlushed += token.instr_2.flushes;
        s.cStalled += stalled; stalled = false;
      }
      if (token.instr_3.committing) {
        struct flatSample &s = result[token.instr_3.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.tDeferred += deferredCycles;
        s.cCommitted += 1;
        s.cMisspeculated += token.instr_3.misspeculated;
        s.cFlushed += token.instr_3.flushes;
        s.cStalled += stalled; stalled = false;
      }

      deferred = false;
      lastSamplingCycle = token.cycle; // Update last cycle everytime we take samples
      lastRetired = token; // Save this cycle as last retired for future cycles
    } else {
      uint64_t address = 0;
      bool misspeculated = false;
      bool flushes = false;

      if (token.rob.valid) {
        // Valid instruction
        if (token.instr_0.valid)
          address = token.instr_0.address;
        else if (token.instr_1.valid)
          address = token.instr_1.address;
        else if (token.instr_2.valid)
          address = token.instr_2.address;
        else if (token.instr_3.valid)
          address = token.instr_3.address;
      } else {
        if (lastRetired.instr_0.committing && (lastRetired.instr_0.misspeculated || lastRetired.instr_0.flushes)) {
          address       = lastRetired.instr_0.address;
          misspeculated = lastRetired.instr_0.misspeculated;
          flushes       = lastRetired.instr_0.flushes;
        } else if (lastRetired.instr_1.committing && (lastRetired.instr_1.misspeculated || lastRetired.instr_1.flushes)) {
          address       = lastRetired.instr_1.address;
          misspeculated = lastRetired.instr_1.misspeculated;
          flushes       = lastRetired.instr_1.flushes;
        } else if (lastRetired.instr_2.committing && (lastRetired.instr_2.misspeculated || lastRetired.instr_2.flushes)) {
          address       = lastRetired.instr_2.address;
          misspeculated = lastRetired.instr_2.misspeculated;
          flushes       = lastRetired.instr_2.flushes;
        } else if (lastRetired.instr_3.committing && (lastRetired.instr_3.misspeculated || lastRetired.instr_3.flushes)) {
          address       = lastRetired.instr_3.address;
          misspeculated = lastRetired.instr_3.misspeculated;
          flushes       = lastRetired.instr_3.flushes;
        }
      }

      if (address) {
        // Experiments have shown that with the following attribution logic
        // all cycle attributions are exclusive, all of the boolean values
        // are distinct true.
        uint64_t const cycleTime = attributeCycles * aggregate_magic[0];

        bool const exception = token.rob.exception;
        // At the end of a rollback, the head is empty and we look at retired
        // attribute those cycles to misspeculated or flushing, but not rollback anymore
        bool const rollingback = token.rob.rollingback && !(misspeculated || flushes);
        bool const stalling = token.rob.valid && !deferred && !token.rob.rollingback && !token.rob.exception;

        auto &u = result[address];
        u.tCycles        += cycleTime;
        u.tStalling      += stalling * attributeCycles;
        u.tDeferred      += deferred * attributeCycles;
        u.tRollingback   += rollingback * attributeCycles;
        u.tException     += exception * attributeCycles;
        u.tMisspeculated += misspeculated * attributeCycles;
        u.tFlushed       += flushes * attributeCycles;

        deferred = false;
        lastSamplingCycle = token.cycle; // Update last cycle everytime we take samples

        stalled = token.rob.valid && !token.rob.exception && !token.rob.rollingback;
      } else {
        // Only normal state is considered as deferred (icache miss, function unit stalling)
        // rollingback/exception is attributed separately
        deferred = !token.rob.rollingback && !token.rob.exception;
      }

    }
  }
}


interval_plus_profiler::interval_plus_profiler(std::vector<std::string> &args, struct traceInfo info)
  : base_profiler(args, info, "IntervalPlusProfiler@" + std::to_string(info.tracerId), true)  {
  if (samplingPeriod == 0) {
    throw std::invalid_argument("sampling period missing or too low");
  }
  flushHeader();
}

void interval_plus_profiler::flushResult() {
  for (auto &r: result) {
    fprintf(csvFile, "0x%lx;%f;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld\n",
            r.first,
            (double) r.second.tCycles / aggregate_magic[0],
            r.second.tCommitting,
            r.second.tStalling,
            r.second.tDeferred,
            r.second.tRollingback,
            r.second.tException,
            r.second.tMisspeculated,
            r.second.tFlushed,
            r.second.cCommitted,
            r.second.cMisspeculated,
            r.second.cFlushed,
            r.second.cStalled);
  }
  result.clear();
}

interval_plus_profiler::~interval_plus_profiler() {
  // We need to call this method from the destructor of the derived class
  // because we have overwritten it and base_profiler would just call
  // its own as we are already destructed when base_profiler destructs
  flushResult();
}


void interval_plus_profiler::tick(char const * const data, unsigned int tokens) {
  struct genericTraceToken512 const * const trace = (struct genericTraceToken512 *) data;
  for (unsigned int i = 0; i < tokens; i ++) {
    struct genericTraceToken512 const token = trace[i];

    if (triggerDetection(token)) {
      lastSamplingCycle = token.cycle + randomStartOffset;
      nextPeriodCycle = token.cycle + samplingPeriod + randomStartOffset;
      nextSamplingCycle = token.cycle + samplingPeriod + randomStartOffset;
      if (randomOffset) {
        nextSamplingCycle -= randomRange(randomGenerator);
      }
      lastRetired = {};
      deferred = false;
      state = next;
      attributeCycles = 0;
      continue;
    }

    if ((state & next) && nextSamplingCycle <= token.cycle) {
        attributeCycles += nextSamplingCycle - lastSamplingCycle;
        lastSamplingCycle = nextSamplingCycle;
        state = armed;
    }

    if (nextPeriodCycle <= token.cycle) {
      uint64_t const missedPeriods = (token.cycle - nextPeriodCycle) / samplingPeriod;
      nextPeriodCycle += (missedPeriods + 1) * samplingPeriod;

      nextSamplingCycle = nextPeriodCycle;
      if (randomOffset) {
        nextSamplingCycle -= randomRange(randomGenerator);
      }

      if (nextSamplingCycle <= token.cycle) {
        attributeCycles += nextSamplingCycle - lastSamplingCycle;
        lastSamplingCycle = nextSamplingCycle;
        state = armed;
      } else {
        state |= next;
      }
    }

    if (state & armed) {
      if (token.rob.committing) {
        uint64_t const cycleTime = attributeCycles * aggregate_magic[token.rob.instr_comm - 1];
        uint64_t const deferredCycles = deferred * attributeCycles;
        if (token.instr_0.committing) {
          struct flatSample &s = result[token.instr_0.address];
          s.tCycles += cycleTime;
          s.tCommitting += attributeCycles;
          s.tDeferred += deferredCycles;
          s.cCommitted += 1;
          s.cMisspeculated += token.instr_0.misspeculated;
          s.cFlushed += token.instr_0.flushes;
        }
        if (token.instr_1.committing) {
          struct flatSample &s = result[token.instr_1.address];
          s.tCycles += cycleTime;
          s.tCommitting += attributeCycles;
          s.tDeferred += deferredCycles;
          s.cCommitted += 1;
          s.cMisspeculated += token.instr_1.misspeculated;
          s.cFlushed += token.instr_1.flushes;
        }
        if (token.instr_2.committing) {
          struct flatSample &s = result[token.instr_2.address];
          s.tCycles += cycleTime;
          s.tCommitting += attributeCycles;
          s.tDeferred += deferredCycles;
          s.cCommitted += 1;
          s.cMisspeculated += token.instr_2.misspeculated;
          s.cFlushed += token.instr_2.flushes;
        }
        if (token.instr_3.committing) {
          struct flatSample &s = result[token.instr_3.address];
          s.tCycles += cycleTime;
          s.tCommitting += attributeCycles;
          s.tDeferred += deferredCycles;
          s.cCommitted += 1;
          s.cMisspeculated += token.instr_3.misspeculated;
          s.cFlushed += token.instr_3.flushes;
        }
        state &= ~armed;
        deferred = false;

        attributeCycles = 0;
        sampleCounter++;
      } else {
        uint64_t address = 0;
        bool misspeculated = false;
        bool flushes = false;

        if (token.rob.valid) {
          if (token.instr_0.valid)
            address = token.instr_0.address;
          else if (token.instr_1.valid)
            address = token.instr_1.address;
          else if (token.instr_2.valid)
            address = token.instr_2.address;
          else if (token.instr_3.valid)
            address = token.instr_3.address;
        } else {
          if (lastRetired.instr_0.committing && (lastRetired.instr_0.misspeculated || lastRetired.instr_0.flushes)) {
            address       = lastRetired.instr_0.address;
            misspeculated = lastRetired.instr_0.misspeculated;
            flushes       = lastRetired.instr_0.flushes;
          } else if (lastRetired.instr_1.committing && (lastRetired.instr_1.misspeculated || lastRetired.instr_1.flushes)) {
            address       = lastRetired.instr_1.address;
            misspeculated = lastRetired.instr_1.misspeculated;
            flushes       = lastRetired.instr_1.flushes;
          } else if (lastRetired.instr_2.committing && (lastRetired.instr_2.misspeculated || lastRetired.instr_2.flushes)) {
            address       = lastRetired.instr_2.address;
            misspeculated = lastRetired.instr_2.misspeculated;
            flushes       = lastRetired.instr_2.flushes;
          } else if (lastRetired.instr_3.committing && (lastRetired.instr_3.misspeculated || lastRetired.instr_3.flushes)) {
            address       = lastRetired.instr_3.address;
            misspeculated = lastRetired.instr_3.misspeculated;
            flushes       = lastRetired.instr_3.flushes;
          }
        }

        if (address) {
          uint64_t const cycleTime = attributeCycles * aggregate_magic[0];
          bool const exception = token.rob.exception;
          bool const rollingback = token.rob.rollingback && !(misspeculated || flushes);
          bool const stalling = token.rob.valid && !deferred && !token.rob.rollingback && !token.rob.exception;

          auto &u = result[address];
          u.tCycles        += cycleTime;
          u.tStalling      += stalling * attributeCycles;
          u.tDeferred      += deferred * attributeCycles;
          u.tRollingback   += rollingback * attributeCycles;
          u.tException     += exception * attributeCycles;
          u.tMisspeculated += misspeculated * attributeCycles;
          u.tFlushed       += flushes * attributeCycles;

          u.cStalled       += token.rob.valid && !token.rob.exception && !token.rob.rollingback;

          state &= ~armed;
          deferred = false;

          attributeCycles = 0;
          sampleCounter++;
        } else {
          deferred = !token.rob.rollingback && !token.rob.exception;
        }
      }
    }


    if (token.rob.committing) {
      lastRetired = token;
      deferred = false; // Reset deferred here as well
    }
  }
}

interval_profiler::interval_profiler(std::vector<std::string> &args, struct traceInfo info)
  : base_profiler(args, info, "IntervalProfiler@" + std::to_string(info.tracerId)) {
  if (samplingPeriod == 0) {
    throw std::invalid_argument("sampling period missing or too low");
  }
}

void interval_profiler::tick(char const * const data, unsigned int tokens) {
  struct genericTraceToken512 const * const trace = (struct genericTraceToken512 *) data;
  for (unsigned int i = 0; i < tokens; i ++) {
    struct genericTraceToken512 const token = trace[i];

    if (triggerDetection(token)) {
      lastSamplingCycle = token.cycle + randomStartOffset;
      nextPeriodCycle = token.cycle + samplingPeriod + randomStartOffset;
      nextSamplingCycle = token.cycle + samplingPeriod + randomStartOffset;
      if (randomOffset) {
        nextSamplingCycle -= randomRange(randomGenerator);
      }
      lastRetired = {};
      deferred = false;
      state = next;
      attributeCycles = 0;
      continue;
    }

    if ((state & next) && nextSamplingCycle <= token.cycle) {
        attributeCycles += nextSamplingCycle - lastSamplingCycle;
        lastSamplingCycle = nextSamplingCycle;
        state = armed;
    }

    if (nextPeriodCycle <= token.cycle) {
      uint64_t const missedPeriods = (token.cycle - nextPeriodCycle) / samplingPeriod;
      nextPeriodCycle += (missedPeriods + 1) * samplingPeriod;

      nextSamplingCycle = nextPeriodCycle;
      if (randomOffset) {
        nextSamplingCycle -= randomRange(randomGenerator);
      }

      if (nextSamplingCycle <= token.cycle) {
        attributeCycles += nextSamplingCycle - lastSamplingCycle;
        lastSamplingCycle = nextSamplingCycle;
        state = armed;
      } else {
        state |= next;
      }
    }

    if (state & armed) {
      if (token.rob.committing) {
        uint64_t address;
        bool misspeculated;
        bool flushes;
        if (token.instr_0.committing) {
          address       = token.instr_0.address;
          misspeculated = token.instr_0.misspeculated;
          flushes       = token.instr_0.flushes;
        } else if (token.instr_1.committing) {
          address       = token.instr_1.address;
          misspeculated = token.instr_1.misspeculated;
          flushes       = token.instr_1.flushes;
        } else if (token.instr_2.committing) {
          address       = token.instr_2.address;
          misspeculated = token.instr_2.misspeculated;
          flushes       = token.instr_2.flushes;
        } else if (token.instr_3.committing) {
          address       = token.instr_3.address;
          misspeculated = token.instr_3.misspeculated;
          flushes       = token.instr_3.flushes;
        }
        struct flatSample &s = result[address];
        s.tCycles     += attributeCycles;
        s.tCommitting += attributeCycles;
        s.tDeferred   += deferred * attributeCycles;
        s.cCommitted     += 1;
        s.cMisspeculated += misspeculated;
        s.cFlushed       += flushes;

        state &= ~armed;
        deferred = false;

        attributeCycles = 0;
        sampleCounter++;
      } else {
        uint64_t address = 0;
        bool misspeculated = false;
        bool flushes = false;

        if (token.rob.valid) {
          if (token.instr_0.valid)
            address = token.instr_0.address;
          else if (token.instr_1.valid)
            address = token.instr_1.address;
          else if (token.instr_2.valid)
            address = token.instr_2.address;
          else if (token.instr_3.valid)
            address = token.instr_3.address;
        } else {
          if (lastRetired.instr_0.committing && (lastRetired.instr_0.misspeculated || lastRetired.instr_0.flushes)) {
            address       = lastRetired.instr_0.address;
            misspeculated = lastRetired.instr_0.misspeculated;
            flushes       = lastRetired.instr_0.flushes;
          } else if (lastRetired.instr_1.committing && (lastRetired.instr_1.misspeculated || lastRetired.instr_1.flushes)) {
            address       = lastRetired.instr_1.address;
            misspeculated = lastRetired.instr_1.misspeculated;
            flushes       = lastRetired.instr_1.flushes;
          } else if (lastRetired.instr_2.committing && (lastRetired.instr_2.misspeculated || lastRetired.instr_2.flushes)) {
            address       = lastRetired.instr_2.address;
            misspeculated = lastRetired.instr_2.misspeculated;
            flushes       = lastRetired.instr_2.flushes;
          } else if (lastRetired.instr_3.committing && (lastRetired.instr_3.misspeculated || lastRetired.instr_3.flushes)) {
            address       = lastRetired.instr_3.address;
            misspeculated = lastRetired.instr_3.misspeculated;
            flushes       = lastRetired.instr_3.flushes;
          }
        }

        if (address) {
          bool const exception = token.rob.exception;
          bool const rollingback = token.rob.rollingback && !(misspeculated || flushes);
          bool const stalling = token.rob.valid && !deferred && !token.rob.rollingback && !token.rob.exception;

          auto &u = result[address];
          u.tCycles        += attributeCycles;
          u.tStalling      += stalling * attributeCycles;
          u.tDeferred      += deferred * attributeCycles;
          u.tRollingback   += rollingback * attributeCycles;
          u.tException     += exception * attributeCycles;
          u.tMisspeculated += misspeculated * attributeCycles;
          u.tFlushed       += flushes * attributeCycles;

          u.cStalled       += token.rob.valid && !token.rob.exception && !token.rob.rollingback;

          state &= ~armed;
          deferred = false;

          attributeCycles = 0;
          sampleCounter++;
        } else {
          deferred = !token.rob.rollingback && !token.rob.exception;
        }
      }
    }
    

    if (token.rob.committing) {
      lastRetired = token;
      deferred = false; // Reset deferred here as well
    }
  }

  flushResult();
}

lynsyn_profiler::lynsyn_profiler(std::vector<std::string> &args, struct traceInfo info)
  : base_profiler(args, info, "LynsynProfiler@" + std::to_string(info.tracerId)), lastRetired({}) {
  if (samplingPeriod == 0) {
    throw std::invalid_argument("sampling period missing or too low");
  }
}

void lynsyn_profiler::tick(char const * const data, unsigned int tokens) {
  struct genericTraceToken512 const * const trace = (struct genericTraceToken512 *) data;
  for (unsigned int i = 0; i < tokens; i ++) {
    struct genericTraceToken512 const token = trace[i];

    if (triggerDetection(token)) {
      lastSamplingCycle = token.cycle + randomStartOffset;
      nextPeriodCycle = token.cycle + samplingPeriod + randomStartOffset;
      nextSamplingCycle = token.cycle + samplingPeriod + randomStartOffset;
      if (randomOffset) {
        nextSamplingCycle -= randomRange(randomGenerator);
      }
      lastRetired = {};
      state = next;
      attributeCycles = 0;
      attributeSamples = 0;
      continue;
    }

    if ((state & next) && nextSamplingCycle <= token.cycle) {
        attributeCycles += nextSamplingCycle - lastSamplingCycle;
        attributeSamples += (attributeCycles  + samplingPeriod - 1) / samplingPeriod;
        lastSamplingCycle = nextSamplingCycle;
        state = armed;
    }

    if (nextPeriodCycle <= token.cycle) {
      uint64_t const missedPeriods = (token.cycle - nextPeriodCycle) / samplingPeriod;
      nextPeriodCycle += (missedPeriods + 1) * samplingPeriod;

      nextSamplingCycle = nextPeriodCycle;
      if (randomOffset) {
        nextSamplingCycle -= randomRange(randomGenerator);
      }

      if (nextSamplingCycle <= token.cycle) {
        attributeCycles += nextSamplingCycle - lastSamplingCycle;
        attributeSamples += (attributeCycles  + samplingPeriod - 1) / samplingPeriod;
        lastSamplingCycle = nextSamplingCycle;
        state = armed;
      } else {
        state |= next;
      }
    }


    if ((state & armed) && lastRetired.rob.committing) {
      uint64_t address;
      // Reverse order -- we want the ONE last committing instruction!
      if(lastRetired.instr_3.committing) {
        address = lastRetired.instr_3.address;
      } else if(lastRetired.instr_2.committing) {
        address = lastRetired.instr_2.address;
      } else if(lastRetired.instr_1.committing) {
        address = lastRetired.instr_1.address;
      } else if(lastRetired.instr_0.committing) {
        address = lastRetired.instr_0.address;
      }
      auto &u = result[address];
      u.tCycles += attributeCycles;
      // Lynsyn blindly samples last committing instructions, thus we need
      // to accumulate samples during cycles that we do not see, because they
      // are not traced
      u.cCommitted += attributeSamples;

      state &= ~armed;
      attributeCycles = 0;
      attributeSamples = 0;
      sampleCounter++;
    }

    if (token.rob.committing) {
      lastRetired = token;
    }
  }
}


pebs_plus_profiler::pebs_plus_profiler(std::vector<std::string> &args, struct traceInfo info)
  : base_profiler(args, info, "PEBSPlusProfiler@" + std::to_string(info.tracerId), true) {
  if (samplingPeriod == 0) {
    throw std::invalid_argument("sampling period missing or too low");
  }
  flushHeader();
}

void pebs_plus_profiler::flushResult() {
  for (auto &r: result) {
    fprintf(csvFile, "0x%lx;%f;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld\n",
            r.first,
            (double) r.second.tCycles / aggregate_magic[0],
            r.second.cCommitted,
            r.second.tCommitting,
            r.second.tStalling,
            r.second.tDeferred,
            r.second.tRollingback,
            r.second.tException,
            r.second.tMisspeculated,
            r.second.tFlushed);
  }
  result.clear();
}

pebs_plus_profiler::~pebs_plus_profiler() {
  // We need to call this method from the destructor of the derived class
  // because we have overwritten it and base_profiler would just call
  // its own as we are already destructed when base_profiler destructs
  flushResult();
}


void pebs_plus_profiler::tick(char const * const data, unsigned int tokens) {
  struct genericTraceToken512 const * const trace = (struct genericTraceToken512 *) data;
  for (unsigned int i = 0; i < tokens; i ++) {
    struct genericTraceToken512 const token = trace[i];

    if (triggerDetection(token)) {
      lastSamplingCycle = token.cycle + randomStartOffset;
      nextPeriodCycle = token.cycle + samplingPeriod + randomStartOffset;
      nextSamplingCycle = token.cycle + samplingPeriod + randomStartOffset;
      if (randomOffset) {
        nextSamplingCycle -= randomRange(randomGenerator);
      }
      state = next;
      attributeCycles = 0;
      continue;
    }

    if ((state & next) && nextSamplingCycle <= token.cycle) {
        attributeCycles += nextSamplingCycle - lastSamplingCycle;
        lastSamplingCycle = nextSamplingCycle;
        state = armed;
    }

    if (nextPeriodCycle <= token.cycle) {
      uint64_t const missedPeriods = (token.cycle - nextPeriodCycle) / samplingPeriod;
      nextPeriodCycle += (missedPeriods + 1) * samplingPeriod;

      nextSamplingCycle = nextPeriodCycle;
      if (randomOffset) {
        nextSamplingCycle -= randomRange(randomGenerator);
      }

      if (nextSamplingCycle <= token.cycle) {
        attributeCycles += nextSamplingCycle - lastSamplingCycle;
        lastSamplingCycle = nextSamplingCycle;
        state = armed;
      } else {
        state |= next;
      }
    }

    if ((state & armed) && token.rob.committing) {
      uint64_t const cycleTime = attributeCycles * aggregate_magic[token.rob.instr_comm - 1];
      if (token.instr_0.committing) {
        struct flatSample &s = result[token.instr_0.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.cCommitted += 1;
      }
      if (token.instr_1.committing) {
        struct flatSample &s = result[token.instr_1.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.cCommitted += 1;
      }
      if (token.instr_2.committing) {
        struct flatSample &s = result[token.instr_2.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.cCommitted += 1;
      }
      if (token.instr_3.committing) {
        struct flatSample &s = result[token.instr_3.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.cCommitted += 1;
      }

      state &= ~armed;
      attributeCycles = 0;
      sampleCounter++;
    }
  }
}


pebs_profiler::pebs_profiler(std::vector<std::string> &args, struct traceInfo info)
  : base_profiler(args, info, "PEBSProfiler@" + std::to_string(info.tracerId)) {
  if (samplingPeriod == 0) {
    throw std::invalid_argument("sampling period missing or too low");
  }
}

void pebs_profiler::tick(char const * const data, unsigned int tokens) {
  struct genericTraceToken512 const * const trace = (struct genericTraceToken512 *) data;
  for (unsigned int i = 0; i < tokens; i ++) {
    struct genericTraceToken512 const token = trace[i];

    if (triggerDetection(token)) {
      lastSamplingCycle = token.cycle + randomStartOffset;
      nextPeriodCycle = token.cycle + samplingPeriod + randomStartOffset;
      nextSamplingCycle = token.cycle + samplingPeriod + randomStartOffset;
      if (randomOffset) {
        nextSamplingCycle -= randomRange(randomGenerator);
      }
      state = next;
      attributeCycles = 0;
      continue;
    }

    if ((state & next) && nextSamplingCycle <= token.cycle) {
        attributeCycles += nextSamplingCycle - lastSamplingCycle;
        lastSamplingCycle = nextSamplingCycle;
        state = armed;
    }

    if (nextPeriodCycle <= token.cycle) {
      uint64_t const missedPeriods = (token.cycle - nextPeriodCycle) / samplingPeriod;
      nextPeriodCycle += (missedPeriods + 1) * samplingPeriod;

      nextSamplingCycle = nextPeriodCycle;
      if (randomOffset) {
        nextSamplingCycle -= randomRange(randomGenerator);
      }

      if (nextSamplingCycle <= token.cycle) {
        attributeCycles += nextSamplingCycle - lastSamplingCycle;
        lastSamplingCycle = nextSamplingCycle;
        state = armed;
      } else {
        state |= next;
      }
    }

    if ((state & armed) && token.rob.committing) {
      uint64_t address;
      if (token.instr_0.committing) {
        address = token.instr_0.address;
      } else if(token.instr_1.committing) {
        address = token.instr_1.address;
      } else if(token.instr_2.committing) {
        address = token.instr_2.address;
      } else if(token.instr_3.committing) {
        address = token.instr_3.address;
      }

      auto &u = result[address];
      u.tCycles += attributeCycles;
      u.tCommitting += attributeCycles;
      u.cCommitted += 1;

      state &= ~armed;
      attributeCycles = 0;
      sampleCounter++;
    }
  }
}


ibs_profiler::ibs_profiler(std::vector<std::string> &args, struct traceInfo info)
  : base_profiler(args, info, "IBSProfiler@" + std::to_string(info.tracerId)), tag(0), post_tag(0), evicted(0), overrunning(0)   {
  if (samplingPeriod == 0) {
    throw std::invalid_argument("sampling period missing or too low");
  }
}

ibs_profiler::~ibs_profiler() {
  fprintf(stderr, "%s: overrunning(%ld), evicted(%ld)\n", profilerName.c_str(), overrunning, evicted);
}

void ibs_profiler::tick(char const * const data, unsigned int tokens) {
  struct genericTraceToken512 const * const trace = (struct genericTraceToken512 *) data;
  for (unsigned int i = 0; i < tokens; i ++) {
    struct genericTraceToken512 const token = trace[i];


    if (triggerDetection(token)) {
      lastSamplingCycle = token.cycle + randomStartOffset;
      nextPeriodCycle = token.cycle + samplingPeriod + randomStartOffset;
      nextSamplingCycle = token.cycle + samplingPeriod + randomStartOffset;
      if (randomOffset) {
        nextSamplingCycle -= randomRange(randomGenerator);
      }
      state = next;
      attributeCycles = 0;
      continue;
    }

    // Check for eviction and sample before we check for the next sampling period
    if (state & armed) {
      uint16_t const tail = token.rob.tail_idx;
      // Align the head to the ROB row of the BOOM (least significant bits are inaccurate)
      uint16_t const head = token.rob.head_idx - (token.rob.head_idx % info.coreWidth);
      // If tail is behind head, and tag behind tail, tail has moved back, instruction is evicted
      bool const evict_1 = tail > head && tag >= tail;
      // If tail is in front of head, and tag behind head, tail has wrapped back around, instruction is evicted
      bool const evict_2 = tail > head && tag < head;
      // If tail and tag is behind the head and tail is behind tag too, tail has moved back, instruction is evicted
      bool const evict_3 = tail < head && tag >= tail && tag < head;
      // If tail is at head and the head is not valid, ROB is empty, instruction is evicted
      bool const evict_4 = tail == head && !token.rob.valid;

      if (evict_1 || evict_2 || evict_3 || evict_4) {
        evicted++;
        state &= ~armed;
      } else if (token.rob.committing && tag >= head && tag < (head + info.coreWidth)) {
        // If still armed, and we committing and the rob head has reached a tagged row
        // grap the first committing instruction as sample
        uint64_t address;
        if (token.instr_0.committing) {
          address = token.instr_0.address;
        } else if(token.instr_1.committing) {
          address = token.instr_1.address;
        } else if(token.instr_2.committing) {
          address = token.instr_2.address;
        } else if(token.instr_3.committing) {
          address = token.instr_3.address;
        }
        auto &u = result[address];

        u.tCycles += attributeCycles;
        u.tCommitting += attributeCycles;
        u.cCommitted += 1;

        state &= ~armed;
        attributeCycles = 0;
        sampleCounter++;
      }
    }

    if ((state & next) && nextSamplingCycle <= token.cycle) {
        attributeCycles += nextSamplingCycle - lastSamplingCycle;
        lastSamplingCycle = nextSamplingCycle;
        state = tagging;
    }

    if (nextPeriodCycle <= token.cycle) {
      uint64_t const missedPeriods = (token.cycle - nextPeriodCycle) / samplingPeriod;
      nextPeriodCycle += (missedPeriods + 1) * samplingPeriod;

      nextSamplingCycle = nextPeriodCycle;
      if (randomOffset) {
        nextSamplingCycle -= randomRange(randomGenerator);
      }

      if (nextSamplingCycle <= token.cycle) {
        attributeCycles += nextSamplingCycle - lastSamplingCycle;
        lastSamplingCycle = nextSamplingCycle;
        state = tagging;
      } else {
        state |= next;
      }
    }

    if ((state & tagging) && token.rob.dispatching) {
      tag = token.rob.tail_idx;
      state = (state | armed) & ~tagging;
    }
  }
}

oracle_profiler_2::oracle_profiler_2(std::vector<std::string> &args, struct traceInfo info)
  : base_profiler(args, info, "OracleProfiler2@" + std::to_string(info.tracerId), true) {
  fprintf(stdout, "oracle_profiler_2 constructor\n");
  // We told the base profiler that we take care of the file output ourselves
  flushHeader();
}

void oracle_profiler_2::flushResult() {
/* fprintf(stdout, "oracle_profiler_2 flushResult\n"); */
  for (auto &r: result) {
    sampleCounter +=
      r.second.tCommitting +
      r.second.tStalling +
      r.second.tDeferred +
      r.second.tRollingback +
      r.second.tException +
      r.second.tMisspeculated +
      r.second.tFlushed;

    fprintf(csvFile, "0x%lx;%f;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld\n",
            r.first,
            (double) r.second.tCycles / aggregate_magic[0],
            r.second.tCommitting,
            r.second.tStalling,
            r.second.tDeferred,
            r.second.tRollingback,
            r.second.tException,
            r.second.tMisspeculated,
            r.second.tFlushed,
            r.second.cCommitted,
            r.second.cMisspeculated,
            r.second.cFlushed,
            r.second.cStalled);
  }
  result.clear();
}

oracle_profiler_2::~oracle_profiler_2() {
  fprintf(stdout, "oracle_profiler_2 destructor\n");
  // We need to call this method from the destructor of the derived class
  // because we have overwritten it and base_profiler would just call
  // its own as we are already destructed when base_profiler destructs
  flushResult();
}

void oracle_profiler_2::tick(char const * const data, unsigned int traces) {
  struct genericTraceToken1024 const * const trace = (struct genericTraceToken1024 *) data;
  for (unsigned int i = 0; i < traces; i ++) {
    struct genericTraceToken1024 const token = trace[i];

/* #define DEBUG */
#ifdef DEBUG
      fprintf(stderr, "%lu | %lu | %d%d%d%d%d%d%d%d, %u, %u, %u | %d, %d",
              token.cycle,
              token.cpu_cycle,
              token.rob.committing,
              token.rob.valid,
              token.rob.ready,
              token.rob.empty,
              token.rob.dispatching,
              token.rob.rollingback,
              token.rob.exception,
              token.rob.csrstall,
              token.rob.head_idx,
              token.rob.pnr_idx,
              token.rob.tail_idx,
              token.rob.instr_comm,
              token.rob.instr_valid);
      fprintf(stderr, " | %d%d%d%d 0x%lx", token.instr_0.valid, token.instr_0.committing, token.instr_0.misspeculated, token.instr_0.flushes, token.instr_0.address);
      fprintf(stderr, " | %d%d%d%d 0x%lx", token.instr_1.valid, token.instr_1.committing, token.instr_1.misspeculated, token.instr_1.flushes, token.instr_1.address);
      fprintf(stderr, " | %d%d%d%d 0x%lx", token.instr_2.valid, token.instr_2.committing, token.instr_2.misspeculated, token.instr_2.flushes, token.instr_2.address);
      fprintf(stderr, " | %d%d%d%d 0x%lx", token.instr_3.valid, token.instr_3.committing, token.instr_3.misspeculated, token.instr_3.flushes, token.instr_3.address);
      fprintf(stderr, " | %d%d%d%d 0x%lx", token.instr_4.valid, token.instr_4.committing, token.instr_4.misspeculated, token.instr_4.flushes, token.instr_4.address);
      fprintf(stderr, " | %d%d%d%d 0x%lx", token.instr_5.valid, token.instr_5.committing, token.instr_5.misspeculated, token.instr_5.flushes, token.instr_5.address);
      fprintf(stderr, " | %d%d%d%d 0x%lx", token.instr_6.valid, token.instr_6.committing, token.instr_6.misspeculated, token.instr_6.flushes, token.instr_6.address);
      fprintf(stderr, " | %d%d%d%d 0x%lx", token.instr_7.valid, token.instr_7.committing, token.instr_7.misspeculated, token.instr_7.flushes, token.instr_7.address);
      fprintf(stderr, "\n");
#endif
    if (triggerDetection2(token)) {
      lastSamplingCycle = token.cycle;
      // Reset profiling, go out of deferred, zero out lastRetired
      deferred = false;
      stalled = false;
      lastRetired = {};
      continue;
    }

    // Detect wrong core configuration. Valid instructions at rob head must be within the coreWidth
    // assert(token.rob.instr_valid <= info.coreWidth);

    uint64_t const attributeCycles = token.cycle - lastSamplingCycle;
    if (token.rob.committing) {
      // Committing instructions
      uint64_t const cycleTime = attributeCycles * aggregate_magic[token.rob.instr_comm - 1];
      uint64_t const deferredCycles = deferred * attributeCycles;
      if (token.instr_0.committing) {
        struct flatSample &s = result[token.instr_0.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.tDeferred += deferredCycles;
        s.cCommitted += 1;
        s.cMisspeculated += token.instr_0.misspeculated;
        s.cFlushed += token.instr_0.flushes;
        s.cStalled += stalled; stalled = false;
      }
      if (token.instr_1.committing) {
        struct flatSample &s = result[token.instr_1.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.tDeferred += deferredCycles;
        s.cCommitted += 1;
        s.cMisspeculated += token.instr_1.misspeculated;
        s.cFlushed += token.instr_1.flushes;
        s.cStalled += stalled; stalled = false;
      }
      if (token.instr_2.committing) {
        struct flatSample &s = result[token.instr_2.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.tDeferred += deferredCycles;
        s.cCommitted += 1;
        s.cMisspeculated += token.instr_2.misspeculated;
        s.cFlushed += token.instr_2.flushes;
        s.cStalled += stalled; stalled = false;
      }
      if (token.instr_3.committing) {
        struct flatSample &s = result[token.instr_3.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.tDeferred += deferredCycles;
        s.cCommitted += 1;
        s.cMisspeculated += token.instr_3.misspeculated;
        s.cFlushed += token.instr_3.flushes;
        s.cStalled += stalled; stalled = false;
      }
      if (token.instr_4.committing) {
        struct flatSample &s = result[token.instr_4.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.tDeferred += deferredCycles;
        s.cCommitted += 1;
        s.cMisspeculated += token.instr_4.misspeculated;
        s.cFlushed += token.instr_4.flushes;
        s.cStalled += stalled; stalled = false;
      }
      if (token.instr_5.committing) {
        struct flatSample &s = result[token.instr_5.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.tDeferred += deferredCycles;
        s.cCommitted += 1;
        s.cMisspeculated += token.instr_5.misspeculated;
        s.cFlushed += token.instr_5.flushes;
        s.cStalled += stalled; stalled = false;
      }
      if (token.instr_6.committing) {
        struct flatSample &s = result[token.instr_6.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.tDeferred += deferredCycles;
        s.cCommitted += 1;
        s.cMisspeculated += token.instr_6.misspeculated;
        s.cFlushed += token.instr_6.flushes;
        s.cStalled += stalled; stalled = false;
      }
      if (token.instr_7.committing) {
        struct flatSample &s = result[token.instr_7.address];
        s.tCycles += cycleTime;
        s.tCommitting += attributeCycles;
        s.tDeferred += deferredCycles;
        s.cCommitted += 1;
        s.cMisspeculated += token.instr_7.misspeculated;
        s.cFlushed += token.instr_7.flushes;
        s.cStalled += stalled; stalled = false;
      }

      deferred = false;
      lastSamplingCycle = token.cycle; // Update last cycle everytime we take samples
      lastRetired = token; // Save this cycle as last retired for future cycles
    } else {
      uint64_t address = 0;
      bool misspeculated = false;
      bool flushes = false;

      if (token.rob.valid) {
        // Valid instruction
        if (token.instr_0.valid)
          address = token.instr_0.address;
        else if (token.instr_1.valid)
          address = token.instr_1.address;
        else if (token.instr_2.valid)
          address = token.instr_2.address;
        else if (token.instr_3.valid)
          address = token.instr_3.address;
        else if (token.instr_4.valid)
          address = token.instr_4.address;
        else if (token.instr_5.valid)
          address = token.instr_5.address;
        else if (token.instr_6.valid)
          address = token.instr_6.address;
        else if (token.instr_7.valid)
          address = token.instr_7.address;
      } else {
        if (lastRetired.instr_0.committing && (lastRetired.instr_0.misspeculated || lastRetired.instr_0.flushes)) {
          address       = lastRetired.instr_0.address;
          misspeculated = lastRetired.instr_0.misspeculated;
          flushes       = lastRetired.instr_0.flushes;
        } else if (lastRetired.instr_1.committing && (lastRetired.instr_1.misspeculated || lastRetired.instr_1.flushes)) {
          address       = lastRetired.instr_1.address;
          misspeculated = lastRetired.instr_1.misspeculated;
          flushes       = lastRetired.instr_1.flushes;
        } else if (lastRetired.instr_2.committing && (lastRetired.instr_2.misspeculated || lastRetired.instr_2.flushes)) {
          address       = lastRetired.instr_2.address;
          misspeculated = lastRetired.instr_2.misspeculated;
          flushes       = lastRetired.instr_2.flushes;
        } else if (lastRetired.instr_3.committing && (lastRetired.instr_3.misspeculated || lastRetired.instr_3.flushes)) {
          address       = lastRetired.instr_3.address;
          misspeculated = lastRetired.instr_3.misspeculated;
          flushes       = lastRetired.instr_3.flushes;
        } else if (lastRetired.instr_4.committing && (lastRetired.instr_4.misspeculated || lastRetired.instr_4.flushes)) {
          address       = lastRetired.instr_4.address;
          misspeculated = lastRetired.instr_4.misspeculated;
          flushes       = lastRetired.instr_4.flushes;
        } else if (lastRetired.instr_5.committing && (lastRetired.instr_5.misspeculated || lastRetired.instr_5.flushes)) {
          address       = lastRetired.instr_5.address;
          misspeculated = lastRetired.instr_5.misspeculated;
          flushes       = lastRetired.instr_5.flushes;
        } else if (lastRetired.instr_6.committing && (lastRetired.instr_6.misspeculated || lastRetired.instr_6.flushes)) {
          address       = lastRetired.instr_6.address;
          misspeculated = lastRetired.instr_6.misspeculated;
          flushes       = lastRetired.instr_6.flushes;
        } else if (lastRetired.instr_7.committing && (lastRetired.instr_7.misspeculated || lastRetired.instr_7.flushes)) {
          address       = lastRetired.instr_7.address;
          misspeculated = lastRetired.instr_7.misspeculated;
          flushes       = lastRetired.instr_7.flushes;
        }
      }

      if (address) {
        // Experiments have shown that with the following attribution logic
        // all cycle attributions are exclusive, all of the boolean values
        // are distinct true.
        uint64_t const cycleTime = attributeCycles * aggregate_magic[0];

        bool const exception = token.rob.exception;
        // At the end of a rollback, the head is empty and we look at retired
        // attribute those cycles to misspeculated or flushing, but not rollback anymore
        bool const rollingback = token.rob.rollingback && !(misspeculated || flushes);
        bool const stalling = token.rob.valid && !deferred && !token.rob.rollingback && !token.rob.exception;

        auto &u = result[address];
        u.tCycles        += cycleTime;
        u.tStalling      += stalling * attributeCycles;
        u.tDeferred      += deferred * attributeCycles;
        u.tRollingback   += rollingback * attributeCycles;
        u.tException     += exception * attributeCycles;
        u.tMisspeculated += misspeculated * attributeCycles;
        u.tFlushed       += flushes * attributeCycles;

        deferred = false;
        lastSamplingCycle = token.cycle; // Update last cycle everytime we take samples

        stalled = token.rob.valid && !token.rob.exception && !token.rob.rollingback;
      } else {
        // Only normal state is considered as deferred (icache miss, function unit stalling)
        // rollingback/exception is attributed separately
        deferred = !token.rob.rollingback && !token.rob.exception;
      }
    }
  }
}
