#pragma once
#include "LinkedList.h"

template<typename T>
class Stack {
    LinkedList<T> data;
public:
    bool empty() const { return data.getSize()==0; }
    int  size()  const { return data.getSize(); }
    void push(const T& v) { data.add(v); }
    // pop: devuelve false si vacío
    bool pop(T& out) {
        int n = data.getSize();
        if (n==0) return false;
        out = data.get(n-1);
        data.removeAt(n-1);
        return true;
    }
    // top: nullptr si vacío
    const T* top() const {
        int n = data.getSize();
        return (n==0) ? nullptr : &data.get(n-1);
    }
};

