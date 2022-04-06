#include "allocator.hh"

namespace Tailslide {

ScriptAllocator::ScriptAllocator() {}
ScriptAllocator::~ScriptAllocator() {
  for(auto &obj_ptr : _tracked_objects) {
    delete obj_ptr;
  }
  for(auto &obj_ptr : _mallocs) {
    free(obj_ptr);
  }
}

}
