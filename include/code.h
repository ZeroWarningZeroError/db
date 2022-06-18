#pragma once
#ifndef CODE_H
#define CODE_H

enum class ResultCode {
  SUCCESS = 0,
  FAIL = 1,
  ERROR_KEY_DUPLICATE = 2,
  ERROR_KEY_EXIST = 3,
  ERROR_KEY_NOT_EXIST = 4,
  ERROR_PAGE_FULL = 5,
  ERROR_SPACE_NOT_ENOUGH = 6,
  ERROR_NOT_MATCH_CONSTRAINT = 7
};

#endif