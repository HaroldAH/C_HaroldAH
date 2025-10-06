#pragma once
#include <string>
#include "LinkedList.h"

struct Symbol {
    std::string name;
    std::string type; // "int","double","string","char","bool"
    bool isArray = false;
    int  size    = 0;
};

class SymbolTable {
    LinkedList<Symbol> items;
public:
    bool add(const Symbol& s) {
        if (find(s.name)) return false;
        items.add(s);
        return true;
    }
    Symbol* find(const std::string& name) {
        for (int i=0;i<items.getSize();++i) if (items.get(i).name==name) return &items.getRef(i);
        return nullptr;
    }
    const Symbol* find(const std::string& name) const {
        for (int i=0;i<items.getSize();++i) if (items.get(i).name==name) return &items.get(i);
        return nullptr;
    }
};
