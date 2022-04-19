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
  _mErrorsSeen.clear();
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
  char buf[1024];
  char *bp = buf;
  const char *fmt;
  const char *fp;
  bool seen_before;
  LogLevel level = (error < W_WARNING) ? LOG_ERROR : LOG_WARN;
  va_list args;

  *bp = 0;

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


  // see if we've seen this error code before
  if (std::find(_mErrorsSeen.begin(), _mErrorsSeen.end(), error) != _mErrorsSeen.end()) {
    seen_before = true;
  } else {
    seen_before = false;
    _mErrorsSeen.push_back((ErrorCode)error);
  }


  if (_mShowErrorCodes) {
    bp += snprintf(bp, LOG_BUF_LEFT, "[E%d] ", (int) error);
  }

  for (fp = fmt; *fp; ++fp) {
    if (*fp == '\n')
      break;
    else
      *bp++ = *fp;
  }
  *bp = 0;

  va_start(args, error);
  logv(level, yylloc, buf, args, error);
  va_end(args);

  // extra info we haven't seen before
  if (!seen_before && *fp != 0) {
    while (*fp) {
      bp = buf;
      for (++fp; *fp; ++fp) {
        if (*fp == '\n')
          break;
        else
          *bp++ = *fp;
      }
      *bp = 0;
      log(LOG_CONTINUE, nullptr, buf);
    }
  }
}


void Logger::logv(LogLevel level, YYLTYPE *yylloc, const char *fmt, va_list args, int error) {
  const char *type = nullptr;
  char buf[1024];
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
    case LOG_CONTINUE:
      vsnprintf(bp, LOG_BUF_LEFT, fmt, args);
      if (_mLastMessage)
        _mLastMessage->cont(buf);

      return;
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

  _mLastMessage = _mAllocator->newTracked<LogMessage>(level, yylloc, buf, (ErrorCode) error);
  //  fprintf(stderr, "%p\n", _mLastMessage);
  _mMessages.push_back(_mLastMessage);
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

void Logger::report() {
  finalize();
  if (_mSort)
    std::sort(_mMessages.begin(), _mMessages.end(), LogMessageSort());

  std::vector<LogMessage *>::iterator i;
  for (i = _mMessages.begin(); i != _mMessages.end(); ++i)
    (*i)->print(stderr);

  fprintf(stderr, "TOTAL:: Errors: %d  Warnings: %d\n", _mErrors, _mWarnings);
}

LogMessage::LogMessage(ScriptContext *ctx, LogLevel type, YYLTYPE *loc, char *message, ErrorCode error)
    : TrackableObject(ctx), _mLogType(type), _mErrorCode(error), _mLoc({}) {
  char *np = mContext->allocator->alloc(strlen(message) + 1);
  if (loc) _mLoc = *loc;
  if (np != nullptr) {
    strcpy(np, message);
    _mMessages.push_back(np);
  }
}

void LogMessage::cont(char *message) {
  char *np = mContext->allocator->alloc(strlen(message) + 1);
  if (np != nullptr) {
    strcpy(np, message);
    _mMessages.push_back(np);
  }
}


void LogMessage::print(FILE *fp) {
  std::vector<char *>::const_iterator i;
  for (i = _mMessages.begin(); i != _mMessages.end(); ++i) {
    if (i != _mMessages.begin())
      fprintf(fp, "%20s", "");
    fprintf(fp, "%s\n", *i);
  }
}


/// ERROR MESSAGE

const char *Logger::_sErrorMessages[] = {
        "ERROR",
        "Duplicate declaration of `%s'; previously declared at (%d, %d).",
        "Invalid operator: %s %s %s.",
        "`%s' is deprecated.",
        "`%s' is deprecated, use %s instead.",
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
        "Expression and constant without operator.\n"
          "Are you doing `foo-2`? Separate operators with spaces.\n"
          "See: http://secondlife.com/badgeo/wakka.php?wakka=knownbugs",
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
};

const char *Logger::_sWarningMessages[] = {
        "WARN",
        "Declaration of `%s' in this scope shadows previous declaration at (%d, %d)",
        "Suggest parentheses around assignment used as truth value.",
        "Changing state to current state acts the same as return. (SL1.8.3)\n"
        "If this is what you intended, consider using return instead.",
        "Changing state in a list or string function will corrupt the stack.\n"
        "Using the return value from this function will cause a run-time bounds check error.\n"
          "See: http://secondlife.com/badgeo/wakka.php?wakka=FunctionStateChangeHack",
        "Using an if statement to change state in a function is a hack and may have unintended side-effects.\n"
          "See: http://secondlife.com/badgeo/wakka.php?wakka=FunctionStateChangeHack",
        "",
        "Empty if statement.",
        "`%s' treated as %d; this is probably not what you wanted.\n"
        "Make sure you separate opeartors with spaces.\n"
          "See: http://forums.secondlife.com/showthread.php?t=60257",
        "%s `%s' declared but never used.",
        "Unused event parameter `%s'.",
        "Using == on lists only compares lengths.\n"
          "See: http://secondlife.com/badgeo/wakka.php?wakka=annoyances",
        "Condition is always true.",
        "Condition is always false.",
        "Empty loop body.",
        "`i_val *= f_val' can have unpredictable runtime behavior, prefer `i_val = (integer)(i_val * f_val)'",
        "`jump %s;' may jump to the wrong label due to label name clashes within function",
        "label `@%s' is declared multiple times in the same function, which may cause undesired behavior",
        "== comparison used as a statement"
};

}
