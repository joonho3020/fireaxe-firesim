//See LICENSE for license details

#include "generictrace.h"
#include "hardwaresamplers.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>

std::vector<std::string> split(std::string str,std::string sep){
    char* cstr=const_cast<char*>(str.c_str());
    char* current;
    std::vector<std::string> arr;
    current=strtok(cstr,sep.c_str());
    while(current!=NULL){
        arr.push_back(current);
        current=strtok(NULL,sep.c_str());
    }
    return arr;
}

char generictrace_t::KIND;

generictrace_t::generictrace_t(
    simif_t &sim,
    StreamEngine &stream,
    const GENERICTRACEBRIDGEMODULE_struct &mmioAddrs,
    int tracerId,
    std::vector<std::string> &args,
    int stream_idx,
    int stream_depth,
    const unsigned int tokenWidth,
    const unsigned int traceWidth,
    const ClockInfo &clock_info
  ) : streaming_bridge_driver_t(sim, stream, &KIND),
      mmioAddrs(mmioAddrs),
      clock_info(clock_info),
      stream_idx(stream_idx),
      dmaQueueDepth(stream_depth),
      tokenWidth(tokenWidth),
      traceWidth(traceWidth)
{
    info.tracerId = tracerId;
    info.tokenBytes = (tokenWidth + 7) / 8;
    info.traceBytes = (traceWidth + 7) / 8;

    tokensPerTrace = std::max((int)((traceWidth + tokenWidth - 1) / tokenWidth), 1);
    maxTokensToPull = (int)(dmaQueueDepth / tokensPerTrace) * tokensPerTrace;

    fprintf(stdout, "generictrace_t: tokensPerTrace %u, maxTokensToPull: %u\n", tokensPerTrace, maxTokensToPull);

    std::string suffix = std::string("=");
    std::string tracetrigger_arg = std::string("+generictrace-trigger") + suffix;
    std::string tracethreads_arg = std::string("+generictrace-threads") + suffix;
    std::string tracecore_arg    = std::string("+generictrace-core") + suffix;
    std::string tracedma_arg     = std::string("+generictrace-tdma") + suffix;
    std::string tracebuffers_arg = std::string("+generictrace-buffers") + suffix;
    std::string traceworker_arg  = std::string("+generictrace-worker") + suffix;


    for (auto &arg: args) {
        if (arg.find(tracetrigger_arg) == 0) {
            std::string const sarg = arg.substr(tracetrigger_arg.length());
            this->traceTrigger = std::stol(sarg);
        }
        if (arg.find(tracebuffers_arg) == 0) {
            auto bufferargs = split(arg.substr(tracebuffers_arg.length()), ",");
            if (bufferargs.size() >= 1) {
                bufferGrouping = std::stoi(bufferargs[0]);
                bufferGrouping = (bufferGrouping <= 0) ? 1 : bufferGrouping;
            }
            if (bufferargs.size() >= 2) {
                bufferDepth = std::stoi(bufferargs[1]);
                bufferDepth = (bufferDepth <= 0) ? 1 : bufferDepth;
            }
        }
        if (arg.find(tracedma_arg) == 0) {
            std::string const sarg = arg.substr(tracedma_arg.length());
            dmaThreshold = std::stod(sarg);
        }
        if (arg.find(tracethreads_arg) == 0) {
            std::string const sarg = arg.substr(tracethreads_arg.length());
            traceThreads = std::stol(sarg);
        }
        if (arg.find(tracecore_arg) == 0) {
            auto coreargs = split(arg.substr(tracecore_arg.length()), ",");
            if (coreargs.size() < 2) {
                fprintf(stderr, "GenericTracer %d: invalid core arguments, expected 'core_width,rob_depth'\n", info.tracerId);
                abort();
            }
            try {
                info.coreWidth = std::stoi(coreargs[0]);
                info.robDepth = std::stoi(coreargs[1]);
            } catch(...) {
                fprintf(stderr, "GenericTracer %d: could not parse core arguments\n", info.tracerId);
                abort();
            }
        }
    }
    for (auto &arg: args) {
        if (arg.find(traceworker_arg) == 0) {
            auto workerargs = split(std::string(arg.c_str() + traceworker_arg.length()), ",");
            std::string workername;
            if (workerargs.empty()) {
                fprintf(stderr, "GenericTracer %d: invalid worker argument\n", info.tracerId);
                abort();
            } else {
                workername = workerargs.front();
                workerargs.erase(workerargs.begin());
            }
            auto reg = workerRegister.find(workername);
            if (reg == workerRegister.end()) {
                fprintf(stderr, "GenericTracer %d: unknown worker '%s'\n", info.tracerId, workername.c_str());
                abort();
            }
            fprintf(stdout, "GenericTracer %d: adding worker '%s' with args '", info.tracerId, workername.c_str());
            for (auto &a: workerargs) {
                fprintf(stdout, "%s%s", a.c_str(), (&a == &workerargs.back()) ? "" : ", ");
            }
            fprintf(stdout, "'\n");


            std::shared_ptr<protectedWorker> worker = std::make_shared<protectedWorker>();
            worker->worker = reg->second(workerargs, info);
            workers.push_back(worker);
        }
    }

    if (workers.size() == 0) {
        fprintf(stdout, "GenericTrace %d: no workers selected, disable tracing\n", info.tracerId);
        traceEnabled = false;
    } else {
        traceEnabled = true;
    }

    // How many tokens are fitting into our buffers
    bufferTokenCapacity = bufferGrouping * dmaQueueDepth;
    // At which point the buffer cannot fit another drain (worst case)
    bufferTokenThreshold = bufferTokenCapacity - dmaQueueDepth;
    // DMA token threshold to start pull
    dmaTokenThreshold = std::max<unsigned int>(std::min<unsigned int>(dmaQueueDepth * dmaThreshold, dmaQueueDepth), 1);

    fprintf(stdout, "bufferTokenCap: %d, bufferTokenThresh: %d, dmaTokenThresh: %d\n", bufferTokenCapacity, bufferTokenThreshold, dmaThreshold);

    if (traceEnabled) {
        if (traceThreads == 0) {
            fprintf(stdout, "GenericTrace %d: multithreading disabled, reduce to single buffer\n", info.tracerId);
            bufferDepth = 1;
        } else if (traceThreads > workers.size()) {
            traceThreads = workers.size();
            fprintf(stdout, "GenericTrace %d: unbalanced thread number, reducing to %d threads\n", info.tracerId, traceThreads);
        }
        for (unsigned int i = 0; i < bufferDepth; i++) {
            std::shared_ptr<referencedBuffer> buffer = std::make_shared<referencedBuffer>();
            buffer->data = (char *) aligned_alloc(sysconf(_SC_PAGESIZE), bufferTokenCapacity * info.tokenBytes);
            if (!buffer->data) {
                fprintf(stdout, "GenericTrace %d: could not allocate memory buffer\n", info.tracerId);
                abort();
            }
            buffers.push_back(buffer);
        }

        if (traceThreads > 0) {
            fprintf(stdout, "GenericTrace %d: spawning %u worker threads\n", info.tracerId, traceThreads);
            for (unsigned int i = 0; i < traceThreads; i++) {
                workerThreads.emplace_back(std::move(std::thread(&generictrace_t::work, this, i)));
            }
        }
    }
}

generictrace_t::~generictrace_t() {
    // Cleanup
    if (traceEnabled) {
        exit = true;
        workerQueueCond.notify_all();
        // Join in the worker threads, they will finish processing the last tokens
        for (auto &t : workerThreads) {
            t.join();
        }
        // Explicitly destruct our workers here
        workers.clear();
        for (auto &b: buffers) {
            free(b->data);
        }
        fprintf(stdout, "GenericTrace %d: tick_time(%f), dma_time(%f)\n", info.tracerId, tickTime.count(), dmaTime.count());
        fprintf(stdout, "GenericTrace %d: traced_tokens(%ld), traced_bytes(%ld)\n", info.tracerId, totalTokens, totalTokens * info.traceBytes);
    }
}

void generictrace_t::init() {
    if (!traceEnabled) {
      write(mmioAddrs.traceEnable, 0);
      write(mmioAddrs.triggerSelector, 0);
      fprintf(stdout, "GenericTrace %d: collection disabled\n", info.tracerId);
    } else {
      write(mmioAddrs.traceEnable, 1);
      write(mmioAddrs.triggerSelector, traceTrigger);
      fprintf(stdout, "GenericTrace %d: trigger(%d), queue_depth(%d), dma_threshold(%d), token_width(%d), trace_width(%d),\n",
              info.tracerId, traceTrigger, dmaQueueDepth, dmaTokenThreshold, tokenWidth, traceWidth);
      fprintf(stdout, "GenericTrace %d: buffers(%d, %d), workers(%ld), threads(%d),  core_width(%d), rob_depth(%d)\n", info.tracerId, bufferGrouping, bufferDepth, workers.size(), traceThreads, info.coreWidth, info.robDepth);
    }
    write(mmioAddrs.initDone, true);
}


void generictrace_t::work(unsigned int thread_index) {
    while (true) {
        // We need this to synchronize the worker threads taking and processing
        // the jobs in order in regard to every worker
        Lock2<locktype_t, locktype_t> workerSync(workerQueueLock, workerSyncLock);
        workerQueueCond.wait(workerSync, [this](){return exit || !workerQueue.empty();});
        if (workerQueue.empty()) {
            return;
        }
        // Pick up a job and its worker
        auto job = workerQueue.front();
        workerQueue.pop();
        // Unlock the workerQueueLock, the main thread might now
        // put new jobs into the queue, however no other thread
        // is yet allowed to take another job out of it
        workerSync.unlock_1();

        // Pick up the worker and takes its ownership
        auto &worker = job.first;
        worker->lock.lock();
        // From here on it is save to release the other threads
        // as no other thread can now execute a job on this worker
        workerSync.unlock_2();

        auto &buffer = job.second;
        worker->worker->tick(buffer->data, buffer->tokens);
        // Release the ownership of this worker so that the next buffer
        // can be given to it by the next worker thread
        worker->lock.unlock();

        // Decrement the buffer reference and if it is not referenced anymore
        // put it back to the spare buffers
        buffer->refs--;
    }
}



size_t generictrace_t::process_tokens(
    unsigned int const max_tokens,
    unsigned int const min_tokens,
    bool flush)
{
/* fprintf(stdout, "generictrace_t::process_tokens: max: %u min: %d flush: %d\n", max_tokens, min_tokens, flush); */
    if (max_tokens == 0 && !flush)
        return 0;

    auto &buffer = buffers[bufferIndex];

    // Only if multi threading is enabled, check if buffer is still used
    while (traceThreads && buffer->refs > 0);

    // This buffer must have been processed as refs is 0
    // and it holds tokens that exceed the bufferTokenThreshold
    // Reset it and reuse it
    if (buffer->tokens > bufferTokenThreshold) {
        buffer->tokens = 0;
    }

    // Drain the tokens from the DMA
    uint32_t token_bytes_from_fpga = 0;
    uint32_t tokens = 0;
    {
        auto start = std::chrono::high_resolution_clock::now();
        token_bytes_from_fpga = pull(
            this->stream_idx,
            buffer->data + (buffer->tokens * info.tokenBytes),
            max_tokens * info.tokenBytes,
            min_tokens * info.tokenBytes);
        tokens = token_bytes_from_fpga / info.tokenBytes;
        dmaTime += std::chrono::high_resolution_clock::now() - start;
    }

/* fprintf(stdout, "tokens: %u token_bytes: %u\n", tokens, token_bytes_from_fpga); */
    assert(tokens % tokensPerTrace == 0);

    buffer->tokens += tokens;
    uint32_t traces = buffer->tokens / tokensPerTrace;

    // If we have exceeded the bufferTokenThreshold we cannot fit another drain
    // into this buffer and we should process it (normal usage that means it is full)
    if (buffer->tokens > bufferTokenThreshold || flush) {
        if (traceThreads) {
            buffer->refs = workers.size();

            workerQueueLock.lock();
            for (auto &worker : workers) {
                workerQueue.push(std::make_pair(worker, buffer));
            }
            workerQueueLock.unlock();
            workerQueueCond.notify_all();

            // Only for multithreading it makes sense to use multiple buffers
            bufferIndex = (bufferIndex + 1) % buffers.size();
            assert(false);
        } else {
            // No threads are launched, processing the workers here
            for (auto &worker: workers) {
                worker->worker->tick(buffer->data, traces);
            }
        }
    }

    totalTokens += tokens;
    return token_bytes_from_fpga;
}


void generictrace_t::tick() {
/* fprintf(stdout, "generictrace_t::tick\n"); */
    auto start = std::chrono::high_resolution_clock::now();
    if (traceEnabled) {
      // Always pull out maxTokensToPull to avoid trailing tokens when a
      // trace consists of multiple tokens
      process_tokens(maxTokensToPull, maxTokensToPull, false);
    }
    tickTime += std::chrono::high_resolution_clock::now() - start;
}


void generictrace_t::flush() {
/* fprintf(stdout, "generictrace_t::flush\n"); */

  pull_flush(stream_idx);
  // Just pull out tokensPerTrace amount of tokens when flushing to avoid trailing tokens
  // when a trace consists of multiple tokens
  while (this->traceEnabled && (process_tokens(tokensPerTrace, tokensPerTrace, true) > 0)) ;

  for (auto &worker: workers) {
      worker->worker->flushResult();
  }
}

