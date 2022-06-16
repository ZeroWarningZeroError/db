#pragma once
#ifndef ITERATOR_H
#define ITERATOR_H

template <typename T>
class Iterator {
 public:
  /**
   * @brief 是否有下一个元素
   *
   * @return true
   * @return false
   */
  virtual bool hasNext() = 0;

  /**
   * @brief 下一个元素
   *
   * @return Iterator
   */
  virtual Iterator Next() = 0;

  /**
   * @brief 重载箭头运算符
   *
   * @return T*
   */
  virtual T* operator->() = 0;

  /**
   * @brief 重载解引用
   *
   * @return T
   */
  virtual T operator*() = 0;
};

#endif