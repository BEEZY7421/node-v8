// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_CPPGC_GARBAGE_COLLECTED_H_
#define INCLUDE_CPPGC_GARBAGE_COLLECTED_H_

#include <type_traits>

#include "include/cppgc/internal/api-constants.h"
#include "include/cppgc/platform.h"
#include "include/cppgc/trace-trait.h"
#include "include/cppgc/type-traits.h"

namespace cppgc {

class Visitor;

namespace internal {

class GarbageCollectedBase {
 public:
  // Must use MakeGarbageCollected.
  void* operator new(size_t) = delete;
  void* operator new[](size_t) = delete;
  // The garbage collector is taking care of reclaiming the object. Also,
  // virtual destructor requires an unambiguous, accessible 'operator delete'.
  void operator delete(void*) {
#ifdef V8_ENABLE_CHECKS
    internal::Abort();
#endif  // V8_ENABLE_CHECKS
  }
  void operator delete[](void*) = delete;

 protected:
  GarbageCollectedBase() = default;
};

}  // namespace internal

template <typename>
class GarbageCollected : public internal::GarbageCollectedBase {
 public:
  using IsGarbageCollectedTypeMarker = void;

 protected:
  GarbageCollected() = default;
};

class GarbageCollectedMixin : public internal::GarbageCollectedBase {
 public:
  using IsGarbageCollectedMixinTypeMarker = void;

  // Sentinel used to mark not-fully-constructed mixins.
  static constexpr void* kNotFullyConstructedObject = nullptr;

  virtual void Trace(cppgc::Visitor*) {}

  // Provide default implementation that indicate that the vtable is not yet
  // set up properly. This is used to to get GCInfo objects for mixins so that
  // these objects can be processed later on.
  virtual TraceDescriptor GetTraceDescriptor() const {
    return {kNotFullyConstructedObject, nullptr};
  }
};

namespace internal {

class __thisIsHereToForceASemicolonAfterThisMacro {};

}  // namespace internal

// The USING_GARBAGE_COLLECTED_MIXIN macro defines all methods and markers
// needed for handling mixins. HasUsingGarbageCollectedMixinMacro is used
// by the clang GC plugin to check for proper usages of the
// USING_GARBAGE_COLLECTED_MIXIN macro.
#define USING_GARBAGE_COLLECTED_MIXIN()                                      \
 public:                                                                     \
  typedef int HasUsingGarbageCollectedMixinMacro;                            \
                                                                             \
  TraceDescriptor GetTraceDescriptor() const override {                      \
    static_assert(                                                           \
        internal::IsSubclassOfTemplate<                                      \
            std::remove_const_t<std::remove_pointer_t<decltype(this)>>,      \
            cppgc::GarbageCollected>::value,                                 \
        "Only garbage collected objects can have garbage collected mixins"); \
    return {this, TraceTrait<std::remove_const_t<                            \
                      std::remove_pointer_t<decltype(this)>>>::Trace};       \
  }                                                                          \
                                                                             \
 private:                                                                    \
  friend class internal::__thisIsHereToForceASemicolonAfterThisMacro

// Merge two or more Mixins into one:
//
//  class A : public GarbageCollectedMixin {};
//  class B : public GarbageCollectedMixin {};
//  class C : public A, public B {
//    // C::GetTraceDescriptor is now ambiguous because there are two
//    // candidates: A::GetTraceDescriptor and B::GetTraceDescriptor.  Ditto for
//    // other functions.
//
//    MERGE_GARBAGE_COLLECTED_MIXINS();
//    // The macro defines C::GetTraceDescriptor, similar to
//    // GarbageCollectedMixin, so that they are no longer ambiguous.
//    // USING_GARBAGE_COLLECTED_MIXIN() overrides them later and provides the
//    // implementations.
//  };
#define MERGE_GARBAGE_COLLECTED_MIXINS()                \
 public:                                                \
  TraceDescriptor GetTraceDescriptor() const override { \
    return {kNotFullyConstructedObject, nullptr};       \
  }                                                     \
                                                        \
 private:                                               \
  friend class internal::__thisIsHereToForceASemicolonAfterThisMacro

}  // namespace cppgc

#endif  // INCLUDE_CPPGC_GARBAGE_COLLECTED_H_
