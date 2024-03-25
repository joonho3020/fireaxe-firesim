#ifndef __GENERICTRACE_WORKER_H_
#define __GENERICTRACE_WORKER_H_

#include <vector>
#include <string>
#include <map>
#include <cassert>
#include <inttypes.h>

struct traceInfo {
    unsigned int tracerId;
    unsigned int tokenBytes;
    unsigned int traceBytes;
    unsigned int coreWidth;
    unsigned int robDepth;
};

void strReplaceAll(std::string& str, const std::string& from, const std::string& to);

class generictrace_worker {
protected:
    struct traceInfo info;
public:
    generictrace_worker(struct traceInfo info);
    virtual void tick(char const * const data, unsigned int tokens);
    virtual void flushResult();
    ~generictrace_worker();
};

class generictrace_filedumper : public generictrace_worker {
private:
    FILE *file;
    std::string filename;
    uint64_t byteCount;
    bool compressed;
    bool raw;
public:
    generictrace_filedumper(std::vector<std::string> args,
                            struct traceInfo info);
    ~generictrace_filedumper();
    void tick(char const * const data, unsigned int tokens);
    void flushResult();

};

#endif // __GENERICTRACE_WORKER_H_
