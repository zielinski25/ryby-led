// Atrapa LittleFS na katalogu hosta (ROOT z $TSPOOL_FS_ROOT). Odwzorowuje
// minimalne API Arduino File/FS, którego używa telemetry_spool.cpp.
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <memory>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

struct FileImpl {
  FILE* fp = nullptr;
  DIR* dp = nullptr;
  std::string full;   // pełna ścieżka hostowa
  std::string name;   // nazwa (dla wpisów katalogu: basename)
  bool isDir = false;
  bool valid = false;
};

class File {
public:
  File() : p(std::make_shared<FileImpl>()) {}
  explicit operator bool() const { return p->valid; }
  bool isDirectory() const { return p->isDir; }
  const char* name() const { return p->name.c_str(); }
  size_t size() const {
    if (p->fp) {
      long cur = ftell(p->fp);
      fseek(p->fp, 0, SEEK_END);
      long n = ftell(p->fp);
      fseek(p->fp, cur, SEEK_SET);
      return (size_t)n;
    }
    struct stat st{};
    if (stat(p->full.c_str(), &st) == 0) return (size_t)st.st_size;
    return 0;
  }
  size_t read(uint8_t* buf, size_t n) { return p->fp ? fread(buf, 1, n, p->fp) : 0; }
  size_t write(const uint8_t* buf, size_t n) { return p->fp ? fwrite(buf, 1, n, p->fp) : 0; }
  bool seek(size_t pos) { return p->fp ? fseek(p->fp, (long)pos, SEEK_SET) == 0 : false; }
  void flush() { if (p->fp) fflush(p->fp); }
  void close() {
    if (p->fp) { fclose(p->fp); p->fp = nullptr; }
    if (p->dp) { closedir(p->dp); p->dp = nullptr; }
    p->valid = false;
  }
  File openNextFile() {
    File out;
    if (!p->dp) return out;
    while (struct dirent* de = readdir(p->dp)) {
      if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
      out.p->name = de->d_name;
      out.p->full = p->full + "/" + de->d_name;
      struct stat st{};
      out.p->isDir = (stat(out.p->full.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
      out.p->valid = true;
      return out;
    }
    return out;
  }
  std::shared_ptr<FileImpl> p;
};

class FS {
public:
  std::string root() const {
    const char* r = getenv("TSPOOL_FS_ROOT");
    return r ? r : "/tmp/tspool-fakefs";
  }
  std::string map(const char* path) const { return root() + path; }
  bool exists(const char* path) const { struct stat st{}; return stat(map(path).c_str(), &st) == 0; }
  bool remove(const char* path) const { return ::unlink(map(path).c_str()) == 0; }
  bool rename(const char* a, const char* b) const { return ::rename(map(a).c_str(), map(b).c_str()) == 0; }
  bool mkdir(const char* path) const { return ::mkdir(map(path).c_str(), 0777) == 0; }
  size_t totalBytes() const { return 1500000; }
  size_t usedBytes() const { return 400000; }
  File open(const char* path, const char* mode) const {
    File f;
    const std::string full = map(path);
    struct stat st{};
    f.p->full = full;
    f.p->name = path;
    if (mode[0] == 'r' && stat(full.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
      f.p->dp = opendir(full.c_str());
      f.p->isDir = true;
      f.p->valid = (f.p->dp != nullptr);
      return f;
    }
    f.p->fp = fopen(full.c_str(), mode[0] == 'r' ? "rb" : (mode[0] == 'a' ? "ab" : "wb"));
    f.p->valid = (f.p->fp != nullptr);
    return f;
  }
};

inline FS LittleFS;
