#ifndef _ASMLAI_COPY_PTR_H
#define _ASMLAI_COPY_PTR_H

#include <memory>

template <typename T> class CopyPtr {
public:
  CopyPtr() : ptr_(nullptr) {}
  CopyPtr(const T &value) : ptr_(new T{value}) {}

  CopyPtr(const CopyPtr &other) : ptr_(nullptr) {
    if (other) {
      ptr_ = std::unique_ptr<T>{new T{*other}};
    }
  }

  CopyPtr(CopyPtr &&other) noexcept : ptr_(nullptr) {
    if (other) {
      ptr_ = std::unique_ptr<T>{new T{*other}};
    }
  }

  CopyPtr &operator=(const CopyPtr &other) {
    CopyPtr temp{other};
    swap(*this, temp);
    return *this;
  }

  CopyPtr &operator=(CopyPtr &&other) noexcept {
    swap(*this, other);
    return *this;
  }

  void swap(CopyPtr &other) noexcept {
    using std::swap;
    swap(*this->ptr_, other.ptr_);
  }

  T &operator*() { return *ptr_; }
  const T &operator*() const { return *ptr_; }
  T *const operator->() { return ptr_.operator->(); }
  const T *const operator->() const { return ptr_.operator->(); }
  const T *const get() const { return ptr_.get(); }
  explicit operator bool() const { return (bool)ptr_; }

private:
  std::unique_ptr<T> ptr_;
};

#endif
