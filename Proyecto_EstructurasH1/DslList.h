#pragma once

#include "LinkedList.h"
#include <string>
#include <functional>

enum class DslType { 
    Entero, 
    Decimal, 
    Texto, 
    Caracter, 
    Booleano 
};

struct Value {
    DslType type;
    union {
        int intVal;
        double doubleVal;
        char charVal;
        bool boolVal;
    };
    std::string stringVal; // Para texto
    
    // Constructores
    Value() : type(DslType::Entero), intVal(0) {}
    Value(int v) : type(DslType::Entero), intVal(v) {}
    Value(double v) : type(DslType::Decimal), doubleVal(v) {}
    Value(const std::string& v) : type(DslType::Texto), stringVal(v) {}
    Value(char v) : type(DslType::Caracter), charVal(v) {}
    Value(bool v) : type(DslType::Booleano), boolVal(v) {}
    
    // Constructor desde tipo y valor por defecto
    Value(DslType t) : type(t) {
        switch(t) {
            case DslType::Entero: intVal = 0; break;
            case DslType::Decimal: doubleVal = 0.0; break;
            case DslType::Texto: stringVal = ""; break;
            case DslType::Caracter: charVal = '\0'; break;
            case DslType::Booleano: boolVal = false; break;
        }
    }
    
    // Operador de copia
    Value(const Value& other) : type(other.type) {
        switch(type) {
            case DslType::Entero: intVal = other.intVal; break;
            case DslType::Decimal: doubleVal = other.doubleVal; break;
            case DslType::Texto: stringVal = other.stringVal; break;
            case DslType::Caracter: charVal = other.charVal; break;
            case DslType::Booleano: boolVal = other.boolVal; break;
        }
    }
    
    // Operador de asignación
    Value& operator=(const Value& other) {
        if (this != &other) {
            type = other.type;
            switch(type) {
                case DslType::Entero: intVal = other.intVal; break;
                case DslType::Decimal: doubleVal = other.doubleVal; break;
                case DslType::Texto: stringVal = other.stringVal; break;
                case DslType::Caracter: charVal = other.charVal; break;
                case DslType::Booleano: boolVal = other.boolVal; break;
            }
        }
        return *this;
    }
    
    // Helper para convertir a string
    std::string toString() const;
};

struct NamedList {
    std::string name;
    DslType type;
    LinkedList<Value> list;
};

class DslList {
public:
    DslList();
    ~DslList();
    
    // API principal
    void reset();
    bool createList(DslType type, const std::string& name, int n);
    bool setAt(const std::string& name, int index, const Value& v);
    bool getAt(const std::string& name, int index, Value& out) const;
    bool exists(const std::string& name) const;
    DslType typeOf(const std::string& name) const;
    bool forEach(const std::string& name, std::function<void(const Value&)> fn) const;
    void debugDump(const std::string& name, LinkedList<std::string>& out) const;
    
    // Helper para obtener el size de una lista
    int getSize(const std::string& name) const;
    
private:
    LinkedList<NamedList> tables;
    int indexOf(const std::string& name) const;
};