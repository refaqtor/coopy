#ifndef COOPY_JSWRAP
#define COOPY_JSWRAP

namespace coopy {
  namespace js {
    class JsWrap;
  }
}

#include "jsapi.h"

#include <string>

/**
 *
 * Javascript interpreter.
 *
 */
class coopy::js::JsWrap {
public:
  JsWrap();

  virtual ~JsWrap();

  std::string apply(const std::string& str);

  bool send(const std::string& function_name,
	    const std::string& str);

  bool send(const std::string& function_name, jsval *val);

  JSContext *context() const;
  JSObject *global() const;

  int getId() const;
  void setCurrentId(int i) const;
  bool isCurrentId(int i) const;

};

#endif

