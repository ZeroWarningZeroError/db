#pragma once
#ifndef CODE_H
#define CODE_H

enum class ResultCode {
  SUCCESS = 0,
  FAIL = 1,
  ERROR_KEY_DUPLICATE = 2,
  ERROR_KEY_EXIST = 3,
  ERROR_PAGE_FULL = 4,
  ERROR_SPACE_NOT_ENOUGH = 5,
  ERROR_NOT_MATCH_CONSTRAINT = 6
};

#endif