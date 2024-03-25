#include "generictrace_worker.h"

void strReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

generictrace_worker::generictrace_worker(struct traceInfo info) : info(info) {}

void generictrace_worker::tick(char const * const data, unsigned int tokens) {
  (void) data; (void) tokens;
}

void generictrace_worker::flushResult() {}

generictrace_worker::~generictrace_worker() {}

generictrace_filedumper::generictrace_filedumper(std::vector<std::string> args, struct traceInfo info)
    : generictrace_worker(info), file(NULL), byteCount(0), compressed(false), raw(false)
{
    std::map<std::string, std::pair<std::string, bool>> const compressionMap = {
        {".gz",  {"gzip", false}},
        {".bz2", {"bzip2", false}},
        {".xz",  {"xz -T", true}},
        {".zst", {"zstd -T", true}},
    };

    if (args.empty()) {
        fprintf(stderr, "FileDumper: no filename provided");
        abort();
    }
    filename = args.front();
    strReplaceAll(filename, std::string("%id"), std::to_string(info.tracerId));
    args.erase(args.begin());

    std::pair<std::string, bool> compress_app;
    std::string compress_level = "1";
    std::string compress_threads = "0";

    unsigned int n = 0;
    for (auto &a: args) {
        if (a == "raw") {
            raw = true;
            continue;
        }
        switch (n) {
            case 0:
                compress_level = a; n++;
                break;
            case 1:
                compress_threads = a; n++;
                break;
        }
    }
    for (const auto &c: compressionMap) {
        if (c.first.size() <= filename.size() && std::equal(c.first.rbegin(), c.first.rend(), filename.rbegin())) {
            compressed = true;
            compress_app = c.second;
            break;
        }

    }

    if (compressed) {
        std::string cmd = compress_app.first;
        if (compress_app.second) {
            cmd += compress_threads;
        }
        cmd += std::string(" -") + compress_level + std::string(" - >") + filename;
        file = popen(cmd.c_str(), "w");
    } else {
        compress_level = "0";
        compress_threads = "0";
        file = fopen(filename.c_str(), "w");
    }
    if (!file) {
        fprintf(stderr, "FileDumper: could not open %s\n", filename.c_str());
        abort();
    }

    if (info.traceBytes == info.tokenBytes) {
        raw = true;
    }

    // TODO: currently the file dumper does not support non-raw tracing if data is transferred over multiple tokens
    // this would require a slightly different algorithm that keeps track of partial data over tick boundaries
    assert(raw || info.traceBytes <= info.tokenBytes);

    fprintf(stdout, "FileDumper: file(%s), compression_level(%s), compression_threads(%s), raw(%d)\n", filename.c_str(), compress_level.c_str(), compress_threads.c_str(), raw);
}

generictrace_filedumper::~generictrace_filedumper() {
    fprintf(stdout, "FileDumper: file(%s), bytes_stored(%ld)\n", filename.c_str(), byteCount);
    if (compressed) {
        pclose(file);
    } else {
        fclose(file);
    }
}

void generictrace_filedumper::flushResult() {}

void generictrace_filedumper::tick(char const * const data, unsigned int tokens) {
   if (raw) {
       fwrite(data, 1, tokens * info.tokenBytes, file);
       byteCount += tokens * info.tokenBytes;
    } else {
       for (unsigned int i = 0; i < tokens; i++) {
           fwrite(&data[i * info.tokenBytes], 1, info.traceBytes, file);
       }
       byteCount += tokens * info.traceBytes;
   }
}
