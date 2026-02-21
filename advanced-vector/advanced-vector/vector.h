#pragma once
#include <cassert>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <utility>
#include <algorithm>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {}

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    Vector() noexcept = default;

    explicit Vector(size_t size)
        : data_(size), size_(size) {
        std::uninitialized_value_construct_n(data_.GetAddress(), size_);
    }

    Vector(const Vector& other)
        : data_(other.size_), size_(other.size_) {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    Vector(Vector&& other) noexcept
        : data_(std::exchange(other.data_, {})),
          size_(std::exchange(other.size_, 0)) {}

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector tmp(rhs);
                Swap(tmp);
            } else {
                const size_t min_size = std::min(size_, rhs.size_);
                
                std::copy_n(rhs.data_.GetAddress(), min_size, data_.GetAddress());
                
                if (rhs.size_ > size_) {
                    std::uninitialized_copy_n(
                        rhs.data_.GetAddress() + size_,
                        rhs.size_ - size_,
                        data_.GetAddress() + size_
                    );
                } 
                else if (rhs.size_ < size_) {
                    std::destroy_n(
                        data_.GetAddress() + rhs.size_,
                        size_ - rhs.size_
                    );
                }
                
                size_ = rhs.size_;
            }
        }
        return *this;
    }
    
    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;   
    }

    using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() noexcept {
        return data_.GetAddress();
    }
    
    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }
    
    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }
    
    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }
    
    const_iterator cbegin() const noexcept {
        return begin();
    }
    
    const_iterator cend() const noexcept {
        return end();
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        assert(pos >= begin() && pos <= end() && "Position out of range");
        const size_t offset = pos - begin();
        if (size_ == Capacity()) {
            return EmplaceWithRealloc(offset, std::forward<Args>(args)...);
        }
        return EmplaceWithoutRealloc(offset, std::forward<Args>(args)...);
    }

    iterator Erase(const_iterator pos) {
        const size_t offset = pos - begin();
        std::move(begin() + offset + 1, end(), begin() + offset);
        PopBack();
        return begin() + offset;
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    void Reserve(size_t capacity) {
        if (capacity <= data_.Capacity()) {
            return;
        }

        RawMemory<T> new_data(capacity);
        MoveOrCopyToNewMemory(new_data);
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        } else if (new_size > size_) {
            Reserve(new_size);
            try {
                std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
            } catch (...) {
                throw;
            }
        }
        size_ = new_size;
    }

    void PushBack(const T& value) {
        EmplaceBack(value);
    }

    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }

    void PopBack() noexcept {
        assert(size_ > 0 && "PopBack() called on empty vector");
        --size_;
        data_[size_].~T();
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        return *Emplace(end(), std::forward<Args>(args)...);
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

private:
    void MoveOrCopyToNewMemory(RawMemory<T>& new_data) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            try {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            } catch (...) {
                throw;
            }
        }
    }

    template <typename... Args>
    iterator EmplaceWithRealloc(size_t offset, Args&&... args) {
        const size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
        RawMemory<T> new_data(new_capacity);

        new (new_data + offset) T(std::forward<Args>(args)...);

        try {
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(begin(), offset, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(begin(), offset, new_data.GetAddress());
            }
        } catch (...) {
            new_data[offset].~T();
            throw;
        }

        try {
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(begin() + offset, size_ - offset, new_data.GetAddress() + offset + 1);
            } else {
                std::uninitialized_copy_n(begin() + offset, size_ - offset, new_data.GetAddress() + offset + 1);
            }
        } catch (...) {
            std::destroy_n(new_data.GetAddress(), offset + 1);
            throw;
        }

        std::destroy_n(begin(), size_);
        data_.Swap(new_data);
        ++size_;
        return begin() + offset;
    }

    template <typename... Args>
    iterator EmplaceWithoutRealloc(size_t offset, Args&&... args) {
        if (offset == size_) {
            new (data_ + size_) T(std::forward<Args>(args)...);
            ++size_;
            return end() - 1;
        }

        T temp(std::forward<Args>(args)...);
        new (data_ + size_) T(std::move(data_[size_ - 1]));
        std::move_backward(begin() + offset, end() - 1, end());
        data_[offset] = std::move(temp);
        ++size_;
        return begin() + offset;
    }

    RawMemory<T> data_;
    size_t size_ = 0;
};
