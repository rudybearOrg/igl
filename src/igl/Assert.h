/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#if !defined(IGL_COMMON_H) && !defined(IGL_COMMON_SKIP_CHECK)
#error "Please, include <igl/Common.h> instead"
#endif

#include <igl/IGLFolly.h>
#include <igl/Macros.h>

///--------------------------------------
/// MARK: - Assert

// When a runtime assert fails, it will:
//
// * trap in the debugger
// * logs to console and/or debugger console: failing expression, function signature, file/line
// * allows you to continue execution during debugging after a failing assert, instead of exiting
//
// ## IGL_DEBUG_ASSERT vs IGL_DEBUG_VERIFY/IGL_DEBUG_VERIFY_NOT
//
// Use IGL_DEBUG_ASSERT for debug-only assertions. On release builds, the expressions
// expand to no-op's, so no perf penalty. IGL_DEBUG_ASSERT logs the failed expression to
// console. To customize, provide a format argument with printf semantics:
//
//   int i = 42;
//   auto p = std::make_shared<int>(i);
//   IGL_DEBUG_ASSERT(p);
//   IGL_DEBUG_ASSERT(*p == i, "*p is wrong value. Got %d. Expected %d.", *p, i);
//
// Use IGL_DEBUG_VERIFY and IGL_DEBUG_VERIFY_NOT to evaluate expressions and catch asserts on debug
// builds. Typically, you'd wrap an expressions inside an `if` statement with IGL_DEBUG_VERIFY.
// IGL_DEBUG_VERIFY_NOT is for `if` statements that check if an error condition is true. That way,
// you can catch assertions on debug builds. On release builds, there's no overhead; they simply
// expand to the original expression:
//
//   FILE* fp = std::fopen("test.txt", "r");
//   if (IGL_DEBUG_VERIFY(fp)) {
//     printf("Success!\n");
//   } else {
//     printf("Failure!\n");
//   }
//
//   void Foo::initialize() {
//     if (IGL_DEBUG_VERIFY_NOT(initialized_)) {
//       // Initialize should only be called once!
//       return;
//     }
//   }
//

#include <igl/Log.h>

#define IGL_ERROR_CATEGORY "IGL"

using IGLErrorHandlerFunc = void (*)(const char* IGL_NONNULL category,
                                     const char* IGL_NONNULL reason,
                                     const char* IGL_NONNULL file,
                                     const char* IGL_NONNULL func,
                                     int line,
                                     const char* IGL_NONNULL format,
                                     va_list ap);

IGL_API void _IGLDebugBreak();

IGL_API void IGLSetDebugAbortListener(IGLErrorHandlerFunc IGL_NULLABLE listener);
IGL_API IGLErrorHandlerFunc IGL_NULLABLE IGLGetDebugAbortListener(void);

namespace igl {
bool isDebugBreakEnabled();
void setDebugBreakEnabled(bool enabled);

[[nodiscard]] inline bool _IGLEnsureNoDiscard(bool cond) {
  return cond;
}

inline void _IGLDebugAbortV([[maybe_unused]] const char* IGL_NONNULL category,
                            [[maybe_unused]] const char* IGL_NONNULL reason,
                            [[maybe_unused]] const char* IGL_NONNULL func,
                            [[maybe_unused]] const char* IGL_NONNULL file,
                            [[maybe_unused]] int line,
                            [[maybe_unused]] const char* IGL_NONNULL format,
                            [[maybe_unused]] va_list ap) {
#if IGL_DEBUG_ABORT_ENABLED
  va_list apCopy;
  va_copy(apCopy, ap);
  auto listener = IGLGetDebugAbortListener();
  if (listener) {
    listener(category, reason, file, func, line, format, apCopy);
  }
  va_end(apCopy);

  IGLLog(IGLLogError, "[%s] %s in '%s' (%s:%d): ", category, reason, func, file, line);
  IGLLogV(IGLLogError, format, ap);
  IGLLog(IGLLogError, IGL_NEWLINE);
  _IGLDebugBreak();
#endif // IGL_DEBUG_ABORT_ENABLED
}

[[nodiscard]] inline bool _IGLDebugAbort(const char* IGL_NONNULL category,
                                         const char* IGL_NONNULL reason,
                                         const char* IGL_NONNULL func,
                                         const char* IGL_NONNULL file,
                                         int line,
                                         const char* IGL_NONNULL format,
                                         ...) {
  va_list ap;
  va_start(ap, format);
  _IGLDebugAbortV(category, reason, func, file, line, format, ap);
  va_end(ap);

  return false;
}
} // namespace igl

#if IGL_DEBUG_ABORT_ENABLED

#define _IGL_DEBUG_ABORT_IMPL(cond, reason, format, ...) \
  (cond                                                  \
       ? ::igl::_IGLEnsureNoDiscard(true)                \
       : ::igl::_IGLDebugAbort(                          \
             IGL_ERROR_CATEGORY, reason, IGL_FUNCTION, __FILE__, __LINE__, format, ##__VA_ARGS__))

#define _IGL_DEBUG_ABORT(format, ...) \
  (void)_IGL_DEBUG_ABORT_IMPL(false, "Abort requested", (format), ##__VA_ARGS__)
#define _IGL_DEBUG_ASSERT(cond, format, ...) \
  (void)_IGL_DEBUG_ABORT_IMPL(!!(cond), "Assert failed", (format), ##__VA_ARGS__)

#define _IGL_DEBUG_VERIFY(cond, format, ...) \
  _IGL_DEBUG_ABORT_IMPL(!!(cond), "Verify failed", (format), ##__VA_ARGS__)
#define _IGL_DEBUG_VERIFY_NOT(cond, format, ...) \
  !_IGL_DEBUG_ABORT_IMPL(!(cond), "Verify failed", (format), ##__VA_ARGS__)

#else

#define _IGL_DEBUG_ABORT(format, ...) static_cast<void>(0)
#define _IGL_DEBUG_ASSERT(cond, format, ...) static_cast<void>(0)
#define _IGL_DEBUG_VERIFY(cond, format, ...) ::igl::_IGLEnsureNoDiscard(!!(cond))
#define _IGL_DEBUG_VERIFY_NOT(cond, format, ...) ::igl::_IGLEnsureNoDiscard(!!(cond))

#endif // IGL_DEBUG_ABORT_ENABLED

#define IGL_DEBUG_ABORT(format, ...) _IGL_DEBUG_ABORT((format), ##__VA_ARGS__)

#define _IGL_DEBUG_ASSERT_0(cond) _IGL_DEBUG_ASSERT(cond, #cond)
#define _IGL_DEBUG_ASSERT_1(cond, format, ...) _IGL_DEBUG_ASSERT(cond, (format), ##__VA_ARGS__)
// Supported variations:
// IGL_DEBUG_ASSERT(cond)
// IGL_DEBUG_ASSERT(cond, format, ...)
#define IGL_DEBUG_ASSERT(...) \
  _IGL_CALL(IGL_CONCAT(_IGL_DEBUG_ASSERT_, _IGL_HAS_COMMA(__VA_ARGS__)), _IGL_ECHO((__VA_ARGS__)))

#define _IGL_DEBUG_VERIFY_0(cond) _IGL_DEBUG_VERIFY(cond, #cond)
#define _IGL_DEBUG_VERIFY_1(cond, format, ...) _IGL_DEBUG_VERIFY(cond, (format), ##__VA_ARGS__)
// Supported variations:
// IGL_DEBUG_VERIFY(cond)
// IGL_DEBUG_VERIFY(cond, format, ...)
#define IGL_DEBUG_VERIFY(...) \
  _IGL_CALL(IGL_CONCAT(_IGL_DEBUG_VERIFY_, _IGL_HAS_COMMA(__VA_ARGS__)), _IGL_ECHO((__VA_ARGS__)))

#define _IGL_DEBUG_VERIFY_NOT_0(cond) _IGL_DEBUG_VERIFY_NOT(cond, "!(" #cond ")")
#define _IGL_DEBUG_VERIFY_NOT_1(cond, format, ...) \
  _IGL_DEBUG_VERIFY_NOT(cond, (format), ##__VA_ARGS__)
// Supported variations:
// IGL_DEBUG_VERIFY_NOT(cond)
// IGL_DEBUG_VERIFY_NOT(cond, format, ...)
#define IGL_DEBUG_VERIFY_NOT(...)                                            \
  _IGL_CALL(IGL_CONCAT(_IGL_DEBUG_VERIFY_NOT_, _IGL_HAS_COMMA(__VA_ARGS__)), \
            _IGL_ECHO((__VA_ARGS__)))

#define IGL_DEBUG_ASSERT_NOT_REACHED() IGL_DEBUG_ABORT("Code should NOT be reached")
#define IGL_DEBUG_ASSERT_NOT_IMPLEMENTED() IGL_DEBUG_ABORT("Code NOT implemented")

///--------------------------------------
/// MARK: - Custom

IGL_API void IGLSetSoftErrorHandler(IGLErrorHandlerFunc IGL_NULLABLE handler);
// @fb-only
IGL_API IGLErrorHandlerFunc IGL_NULLABLE IGLGetSoftErrorHandler(void);
IGL_API void IGLSoftError(const char* IGL_NONNULL category,
                          const char* IGL_NONNULL reason,
                          const char* IGL_NONNULL file,
                          const char* IGL_NONNULL func,
                          int line,
                          const char* IGL_NONNULL format,
                          ...);
namespace igl {
[[nodiscard]] inline bool _IGLSoftError(const char* IGL_NONNULL category,
                                        const char* IGL_NONNULL reason,
                                        const char* IGL_NONNULL func,
                                        const char* IGL_NONNULL file,
                                        int line,
                                        const char* IGL_NONNULL format,
                                        ...) {
  va_list ap, apCopy;
  va_start(ap, format);
  va_copy(apCopy, ap);

  _IGLDebugAbortV(category, reason, func, file, line, format, apCopy);
  va_end(apCopy);

#if IGL_SOFT_ERROR_ENABLED
  auto handler = IGLGetSoftErrorHandler();
  if (handler) {
    handler(category, reason, file, func, line, format, ap);
  }
#endif // IGL_SOFT_ERROR_ENABLED

  va_end(ap);

  return false; // Always return false
}
} // namespace igl

#if IGL_SOFT_ERROR_ENABLED

#define _IGL_SOFT_ERROR_IMPL(cond, reason, format, ...) \
  (cond                                                 \
       ? ::igl::_IGLEnsureNoDiscard(true)               \
       : ::igl::_IGLSoftError(                          \
             IGL_ERROR_CATEGORY, reason, IGL_FUNCTION, __FILE__, __LINE__, format, ##__VA_ARGS__))

#define _IGL_SOFT_ERROR(format, ...) \
  (void)_IGL_SOFT_ERROR_IMPL(false, "Soft error", (format), ##__VA_ARGS__)
#define _IGL_SOFT_ASSERT(cond, format, ...) \
  (void)_IGL_SOFT_ERROR_IMPL(!!(cond), "Soft assert failed", (format), ##__VA_ARGS__)

#define _IGL_SOFT_VERIFY(cond, format, ...) \
  _IGL_SOFT_ERROR_IMPL(!!(cond), "Soft verify failed", (format), ##__VA_ARGS__)
#define _IGL_SOFT_VERIFY_NOT(cond, format, ...) \
  !_IGL_SOFT_ERROR_IMPL(!(cond), "Soft verify failed", (format), ##__VA_ARGS__)

#else

#define _IGL_SOFT_ERROR(format, ...) static_cast<void>(0)
#define _IGL_SOFT_ASSERT(cond, format, ...) static_cast<void>(0)
#define _IGL_SOFT_VERIFY(cond, format, ...) ::igl::_IGLEnsureNoDiscard(!!(cond))
#define _IGL_SOFT_VERIFY_NOT(cond, format, ...) ::igl::_IGLEnsureNoDiscard(!!(cond))

#endif // IGL_SOFT_ERROR_ENABLED

#define IGL_SOFT_ERROR(format, ...) _IGL_SOFT_ERROR((format), ##__VA_ARGS__)

#define _IGL_SOFT_ASSERT_0(cond) _IGL_SOFT_ASSERT(cond, #cond)
#define _IGL_SOFT_ASSERT_1(cond, format, ...) _IGL_SOFT_ASSERT(cond, (format), ##__VA_ARGS__)
// Supported variations:
// IGL_SOFT_ASSERT(cond)
// IGL_SOFT_ASSERT(cond, format, ...)
#define IGL_SOFT_ASSERT(...) \
  _IGL_CALL(IGL_CONCAT(_IGL_SOFT_ASSERT_, _IGL_HAS_COMMA(__VA_ARGS__)), _IGL_ECHO((__VA_ARGS__)))

#define _IGL_SOFT_VERIFY_0(cond) _IGL_SOFT_VERIFY(cond, #cond)
#define _IGL_SOFT_VERIFY_1(cond, format, ...) _IGL_SOFT_VERIFY(cond, (format), ##__VA_ARGS__)
// Supported variations:
// IGL_SOFT_VERIFY(cond)
// IGL_SOFT_VERIFY(cond, format, ...)
#define IGL_SOFT_VERIFY(...) \
  _IGL_CALL(IGL_CONCAT(_IGL_SOFT_VERIFY_, _IGL_HAS_COMMA(__VA_ARGS__)), _IGL_ECHO((__VA_ARGS__)))

#define _IGL_SOFT_VERIFY_NOT_0(cond) _IGL_SOFT_VERIFY_NOT(cond, "!(" #cond ")")
#define _IGL_SOFT_VERIFY_NOT_1(cond, format, ...) \
  _IGL_SOFT_VERIFY_NOT(cond, (format), ##__VA_ARGS__)
// Supported variations:
// IGL_SOFT_VERIFY_NOT(cond)
// IGL_SOFT_VERIFY_NOT(cond, format, ...)
#define IGL_SOFT_VERIFY_NOT(...)                                            \
  _IGL_CALL(IGL_CONCAT(_IGL_SOFT_VERIFY_NOT_, _IGL_HAS_COMMA(__VA_ARGS__)), \
            _IGL_ECHO((__VA_ARGS__)))
