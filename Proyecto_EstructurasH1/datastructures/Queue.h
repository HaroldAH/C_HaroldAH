#pragma once
// ============================================================================
// Queue.h - Cola simple basada en punteros (sin STL)
// Implementación para cumplir requisito académico de estructuras de datos
// ============================================================================

#include <stdexcept>

template<typename T>
class Queue {
private:
    struct Node {
        T value;
        Node* next;
        explicit Node(const T& v) : value(v), next(nullptr) {}
    };
    
    Node* frontPtr;
    Node* rearPtr;
    int count;

public:
    Queue() : frontPtr(nullptr), rearPtr(nullptr), count(0) {}
    
    ~Queue() {
        clear();
    }
    
    // Constructor de copia
    Queue(const Queue& other) : frontPtr(nullptr), rearPtr(nullptr), count(0) {
        Node* current = other.frontPtr;
        while (current) {
            enqueue(current->value);
            current = current->next;
        }
    }
    
    // Operador de asignación
    Queue& operator=(const Queue& other) {
        if (this != &other) {
            clear();
            Node* current = other.frontPtr;
            while (current) {
                enqueue(current->value);
                current = current->next;
            }
        }
        return *this;
    }
    
    // Agregar elemento al final
    void enqueue(const T& value) {
        Node* newNode = new Node(value);
        if (empty()) {
            frontPtr = rearPtr = newNode;
        } else {
            rearPtr->next = newNode;
            rearPtr = newNode;
        }
        ++count;
    }
    
    // Remover elemento del frente
    bool dequeue(T& out) {
        if (empty()) {
            return false;
        }
        
        Node* temp = frontPtr;
        out = frontPtr->value;
        frontPtr = frontPtr->next;
        
        if (frontPtr == nullptr) {
            rearPtr = nullptr;
        }
        
        delete temp;
        --count;
        return true;
    }
    
    // Obtener elemento del frente sin remover
    const T* front() const {
        return empty() ? nullptr : &frontPtr->value;
    }
    
    // Verificar si está vacía
    bool empty() const {
        return frontPtr == nullptr;
    }
    
    // Obtener tamaño
    int size() const {
        return count;
    }
    
    // Limpiar todos los elementos
    void clear() {
        while (frontPtr) {
            Node* temp = frontPtr;
            frontPtr = frontPtr->next;
            delete temp;
        }
        rearPtr = nullptr;
        count = 0;
    }
};
