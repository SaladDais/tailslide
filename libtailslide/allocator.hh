#ifndef ALLOCATOR_HH
#define ALLOCATOR_HH

#include <functional>
#include <vector>
#include <cstring>
#include <cstdlib>

namespace Tailslide {

struct ScriptContext;

class TrackableObject {
public:
  explicit TrackableObject(ScriptContext *ctx) { mContext = ctx;};
  virtual ~TrackableObject() = default;
  ScriptContext *mContext = nullptr;
};

class ScriptAllocator {
public:
    ScriptAllocator() = default;
    virtual ~ScriptAllocator();

    void setContext(ScriptContext *context) { _mContext = context;};

    template<typename TClazz, typename... Args>
    inline TClazz * newTracked(Args&&... args) {
      static_assert(std::is_base_of<TrackableObject, TClazz>::value, "Must be based on LLTrackableObject");
      auto* val = new TClazz(_mContext, std::forward<Args>(args)...);
      _mTrackedObjects.emplace_back(val);
      return val;
    }

    char *alloc(size_t size) {
      char *val = (char *)malloc(size);
      _mMallocs.emplace_back(val);
      return val;
    }

    char *copyStr(const char* old_str) {
      char *new_str = (char *)malloc(strlen(old_str) + 1);
      if (new_str) {
        strcpy(new_str, old_str);
        trackMalloc(new_str);
      }
      return new_str;
    }

    void trackMalloc(void *alloced_data) {
      _mMallocs.emplace_back(alloced_data);
    }
private:
    std::vector<TrackableObject *> _mTrackedObjects {};
    std::vector<void *> _mMallocs {};
    ScriptContext *_mContext = nullptr;
};

}

#endif
