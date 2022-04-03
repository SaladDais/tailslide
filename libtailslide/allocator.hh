#ifndef ALLOCATOR_HH
#define ALLOCATOR_HH

#include <functional>
#include <vector>
#include <cstring>
#include <cstdlib>

namespace Tailslide {

class TrackableObject {
public:
  virtual ~TrackableObject() = default;
};

class ScriptAllocationManager {
public:
    ScriptAllocationManager();
    virtual ~ScriptAllocationManager();

    template<typename T, typename... Args>
    inline T* new_tracked(Args&&... args) {
      static_assert(std::is_base_of<TrackableObject, T>::value, "Must be based on LLTrackableObject");
      T* val = new T(std::forward<Args>(args)...);
      _tracked_objects.emplace_back(val);
      return val;
    }

    char *alloc(size_t size) {
      char *val = (char *)malloc(size);
      _mallocs.emplace_back(val);
      return val;
    }

    char *copy_str(const char* old_str) {
      char *new_str = strdup(old_str);
      if (new_str)
        track_malloc(new_str);
      return new_str;
    }

    void track_malloc(void *alloced_data) {
      _mallocs.emplace_back(alloced_data);
    }
private:
    std::vector<TrackableObject *> _tracked_objects {};
    std::vector<void *> _mallocs {};
};

extern thread_local ScriptAllocationManager *gAllocationManager;

}

#endif
