#include "stdafx.h"
#include "DslList.h"
#include <sstream>

std::string Value::toString() const {
    switch(type) {
        case DslType::Entero:
            return std::to_string(intVal);
        case DslType::Decimal:
            return std::to_string(doubleVal);
        case DslType::Texto:
            return stringVal;
        case DslType::Caracter:
            return std::string(1, charVal);
        case DslType::Booleano:
            return boolVal ? "true" : "false";
        default:
            return "";
    }
}

DslList::DslList() {
}

DslList::~DslList() {
}

void DslList::reset() {
    try {
        tables.clear();
    } catch (...) {
        // No crash en reset
    }
}

int DslList::indexOf(const std::string& name) const {
    for (int i = 0; i < tables.getSize(); ++i) {
        if (tables.get(i).name == name) {
            return i;
        }
    }
    return -1;
}

bool DslList::createList(DslType type, const std::string& name, int n) {
    try {
        if (name.empty() || n <= 0) {
            return false;
        }
        
        // Si ya existe, la reemplazamos
        int idx = indexOf(name);
        
        // Crear nueva NamedList
        NamedList newEntry;
        newEntry.name = name;
        newEntry.type = type;
        
        // Llenar con valores por defecto
        for (int i = 0; i < n; i++) {
            Value defaultVal(type);
            newEntry.list.add(defaultVal);
        }
        
        if (idx >= 0) {
            // Reemplazar existente
            tables.setAt(idx, newEntry);
        } else {
            // Agregar nuevo
            tables.add(newEntry);
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool DslList::setAt(const std::string& name, int index, const Value& v) {
    try {
        int idx = indexOf(name);
        if (idx < 0) {
            return false;
        }
        
        NamedList& entry = tables.getRef(idx);
        
        if (index < 0 || index >= entry.list.getSize()) {
            return false;
        }
        
        // Verificar que el tipo coincida
        if (v.type != entry.type) {
            return false;
        }
        
        // Actualizar in-place
        return entry.list.setAt(index, v);
        
    } catch (...) {
        return false;
    }
}

bool DslList::getAt(const std::string& name, int index, Value& out) const {
    try {
        int idx = indexOf(name);
        if (idx < 0) {
            return false;
        }
        
        const NamedList& entry = tables.get(idx);
        
        if (index < 0 || index >= entry.list.getSize()) {
            return false;
        }
        
        out = entry.list.get(index);
        return true;
    } catch (...) {
        return false;
    }
}

bool DslList::exists(const std::string& name) const {
    return indexOf(name) >= 0;
}

DslType DslList::typeOf(const std::string& name) const {
    int idx = indexOf(name);
    if (idx >= 0) {
        return tables.get(idx).type;
    }
    return DslType::Entero; // Default
}

bool DslList::forEach(const std::string& name, std::function<void(const Value&)> fn) const {
    try {
        if (!fn) {
            return false;
        }
        
        int idx = indexOf(name);
        if (idx < 0) {
            return false;
        }
        
        const NamedList& entry = tables.get(idx);
        for (int i = 0; i < entry.list.getSize(); i++) {
            fn(entry.list.get(i));
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

void DslList::debugDump(const std::string& name, LinkedList<std::string>& out) const {
    try {
        int idx = indexOf(name);
        if (idx < 0) {
            out.add("Lista '" + name + "' no existe");
            return;
        }
        
        const NamedList& entry = tables.get(idx);
        std::string typeStr;
        switch (entry.type) {
            case DslType::Entero: typeStr = "enteros"; break;
            case DslType::Decimal: typeStr = "decimales"; break;
            case DslType::Texto: typeStr = "texto"; break;
            case DslType::Caracter: typeStr = "caracteres"; break;
            case DslType::Booleano: typeStr = "booleanos"; break;
        }
        
        out.add("Lista '" + name + "' (tipo: " + typeStr + ", tamaño: " + std::to_string(entry.list.getSize()) + "):");
        
        for (int i = 0; i < entry.list.getSize(); i++) {
            out.add("  [" + std::to_string(i) + "] = " + entry.list.get(i).toString());
        }
        
    } catch (...) {
        out.add("Error durante debugDump");
    }
}

int DslList::getSize(const std::string& name) const {
    try {
        int idx = indexOf(name);
        if (idx < 0) {
            return 0;
        }
        
        return tables.get(idx).list.getSize();
    } catch (...) {
        return 0;
    }
}