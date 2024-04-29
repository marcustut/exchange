#include "limit.h"

limit limit_default() {
  return (limit){};
}

bool limit_not_found(limit result) {
  return result.price == limit_default().price;
}