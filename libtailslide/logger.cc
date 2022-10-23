#include <cstdarg>
#include <vector>
#include <algorithm>
#include "logger.hh"
#include "lslmini.hh"

namespace Tailslide {

#define LOG_BUF_LEFT (1024 - (bp - buf))

void Logger::reset() {
  _mMessages.clear();
  _mAssertions.clear();
  _mErrors = 0;
  _mWarnings = 0;
  _mFinalized = false;
  _mCheckAssertions = false;
}

void Logger::log(LogLevel level, YYLTYPE *yylloc, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logv(level, yylloc, fmt, args);
  va_end(args);
}

void Logger::error(YYLTYPE *yylloc, int error, ...) {
  // FIXME: magic numbers
  char buf[1025] = {0};
  char *bp = buf;
  const char *fmt;
  const char *fp;
  LogLevel level = (error < W_WARNING) ? LOG_ERROR : LOG_WARN;
  va_list args;

  // pick out the format string to tack onto the end of the message
  if (level == LOG_ERROR) {
    fmt = _sErrorMessages[(int) (error - E_ERROR)];
  } else {
    fmt = _sWarningMessages[(int) (error - W_WARNING)];
  }

  if (_mCheckAssertions) {
    // if we're checking assertions, messages will be removed by matching assertions,
    // so we just add "Unexpected" to all of them.
    bp += snprintf(bp, LOG_BUF_LEFT, level == LOG_ERROR ? "Unexpected error: " : "Unexpected warning: ");

    // upgrade everything to error
    level = LOG_ERROR;
  }

  if (_mShowErrorCodes) {
    bp += snprintf(bp, LOG_BUF_LEFT, "[E%d] ", (int) error);
  }

  // copy the format string for the error itself to the end
  for (fp = fmt; *fp && LOG_BUF_LEFT; ++fp) {
    *bp++ = *fp;
  }

  va_start(args, error);
  logv(level, yylloc, buf, args, error);
  va_end(args);
}


void Logger::logv(LogLevel level, YYLTYPE *yylloc, const char *fmt, va_list args, int error) {
  const char *type = nullptr;
  char buf[1025] = {0};
  char *bp = buf;
  switch (level) {
    case LOG_ERROR:
      type = "ERROR";
      ++_mErrors;
      break;
    case LOG_WARN:
      type = "WARN";
      ++_mWarnings;
      break;
    case LOG_INFO:
      if (!_mShowInfo) return;
      type = "INFO";
      break;
    case LOG_DEBUG:
    case LOG_DEBUG_MINOR:
    case LOG_DEBUG_SPAM:
#ifdef DEBUG_LEVEL
      if ( DEBUG_LEVEL < level ) return;
      type = "DEBUG";
#else /* not DEBUG_LEVEL */
      return;
#endif /* not DEBUG_LEVEL */
      break;
    default:
      type = "OTHER";
      break;
  }

  bp += snprintf(bp, LOG_BUF_LEFT, "%5s:: ", type);
  if (yylloc != nullptr) {
    bp += snprintf(bp, LOG_BUF_LEFT, "(%3d,%3d)", yylloc->first_line, yylloc->first_column);
    if (_mShowEnd)
      bp += snprintf(bp, LOG_BUF_LEFT, "-(%3d,%3d)", yylloc->last_line, yylloc->last_column);
    bp += snprintf(bp, LOG_BUF_LEFT, ": ");
  }
  bp += vsnprintf(bp, LOG_BUF_LEFT, fmt, args);
  _mMessages.push_back(_mAllocator->newTracked<LogMessage>(level, yylloc, buf, (ErrorCode) error));
}

struct LogMessageSort {
    bool operator()(LogMessage *const &left, LogMessage *const &right) {
      if (left->getType() < right->getType())
        return true;
      else if (left->getType() > right->getType())
        return false;

      if (left->getLoc()->first_line < right->getLoc()->first_line)
        return true;
      else if (left->getLoc()->first_line > right->getLoc()->first_line)
        return false;

      if (left->getLoc()->first_column < right->getLoc()->first_column)
        return true;

      return false;
    }
};

void Logger::filterAssertErrors() {
  std::vector<std::pair<int, ErrorCode>>::iterator ai;
  std::vector<std::pair<int, ErrorCode>> failed_asserts;
  for (ai = _mAssertions.begin(); ai != _mAssertions.end(); ++ai) {
    bool suppressed_by_assert = false;
    for (auto i = _mMessages.begin(); i != _mMessages.end(); ++i) {
      if ((ai)->first == (*i)->getLoc()->first_line && (ai)->second == (*i)->getError()) {
        --_mErrors; // when check assertions, warnings are treated as errors.
        _mMessages.erase(i);
        suppressed_by_assert = true;
        break;
      }
    }
    if (!suppressed_by_assert)
      failed_asserts.push_back(*ai);
  }
  for (auto failed_assert : failed_asserts) {
      log(LOG_ERROR, nullptr, "Assertion failed: error %d on line %d.", failed_assert.second, failed_assert.first);
  }
}

void Logger::finalize() {
  if (_mFinalized)
    return;
  _mFinalized = true;
  if (_mCheckAssertions)
    filterAssertErrors();
}

void Logger::printReport() {
  finalize();
  if (_mSort)
    std::sort(_mMessages.begin(), _mMessages.end(), LogMessageSort());

  std::vector<LogMessage *>::iterator i;
  for (i = _mMessages.begin(); i != _mMessages.end(); ++i)
    fprintf(stderr, "%s\n", (*i)->getMessage().c_str());

  fprintf(stderr, "TOTAL:: Errors: %d  Warnings: %d\n", _mErrors, _mWarnings);
}

LogMessage::LogMessage(ScriptContext *ctx, LogLevel type, YYLTYPE *loc, const char *message, ErrorCode error)
    : TrackableObject(ctx), _mLogType(type), _mLoc({}), _mErrorCode(error) {
  if (loc) _mLoc = *loc;
  assert (message != nullptr);
  _mMessage = message;
}


/// ERROR MESSAGE

const char *Logger::_sErrorMessages[] = {
        "ERROR",
        "Duplicate declaration of `%s'; previously declared at (%d, %d).",
        "Invalid operator: %s %s %s.",
        "",
        "",
        "Attempting to use `%s' as a %s, but it is a %s.",
        "`%s' is undeclared.",
        "`%s' is undeclared; did you mean %s?",
        "Invalid member: `%s.%s'.",
        "Trying to access `%s.%s', but `%1$s' is a %3$s",
        "Attempting to access `%s.%s', but `%1$s' is not a vector or rotation.",
        "Passing %s as argument %d of `%s' which is declared as `%s %s'.",
        "Too many arguments to function `%s'.",
        "Too few arguments to function `%s'.",
        "Functions cannot change state.",
        "`%s %s' assigned a %s value.",
        "%s member assigned %s value (must be float or integer).",
        "Event handlers cannot return a value.",
        "Returning a %s value from a %s function.",
        "Not all code paths return a value.",
        "%s", // Syntax error, bison includes all the info.
        "Global initializer must be constant.",
        "", // deprecated
        "State must have at least one event handler.",
        "Parser stack depth exceeded; SL will throw a syntax error here.",
        "`%s' is a constant and cannot be used as an lvalue.",
        "`%s' is a constant and cannot be used in a variable declaration.",
        "Declaring `%s' as parameter %d of `%s' which should be `%s %s'.",
        "Too many parameters for event `%s'.",
        "Too few parameters for event `%s'.",
        "`%s' is not a valid event name.",
        "`%s' is an event name, and cannot be used as an identifier.",
        "`%s' may not be declared here, create a new scope with { }.",
        "Multiple handlers for event `%s'",
        "Lists may not contain other lists",
        "May not cast %s to %s",
        "Lists may not contain nulls",
        "Stack-heap collision"
};

const char *Logger::_sWarningMessages[] = {
        "WARN",
        "Declaration of `%s' in this scope shadows previous declaration at (%d, %d)",
        "Suggest parentheses around assignment used as truth value.",
        "Changing state to current state acts the same as return, use return instead.",
        "Changing state in a list or string function will corrupt the stack",
        "Using an if statement to change state in a function is a hack and may have unintended side-effects.",
        "", // deprecated
        "Empty if statement.",
        "", // deprecated
        "%s `%s' declared but never used.",
        "Unused event parameter `%s'.",
        "Using == on lists only compares lengths.",
        "Condition is always true.",
        "Condition is always false.",
        "Empty loop body.",
        "`i_val *= f_val' can have unpredictable runtime behavior, prefer `i_val = (integer)(i_val * f_val)'",
        "`jump %s;' may jump to the wrong label due to label name clashes within function",
        "label `@%s' is declared multiple times in the same function, which may cause undesired behavior",
        "== comparison used as a statement",
        "`%s' is deprecated.",
        "`%s' is deprecated, use %s instead.",
};

}
