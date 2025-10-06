# C_HaroldA - Convertidor de Lenguaje Natural a C++

![Version](https://img.shields.io/badge/version-1.0-blue.svg)
![Language](https://img.shields.io/badge/language-C%2B%2B14-orange.svg)
![Framework](https://img.shields.io/badge/framework-Qt6-green.svg)

**Desarrollado por:** Harold Avia Hernandez  
**Tema Visual:** Inspirado en Zenitsu (Kimetsu no Yaiba)

---

## Descripcion General

**C_HaroldA** es un convertidor que traduce instrucciones escritas en **lenguaje natural español** a codigo **C++ funcional**. El programa permite escribir algoritmos usando palabras cotidianas y genera automaticamente codigo C++ que compila y ejecuta correctamente.

### Ejemplo Rapido

**Entrada (Lenguaje Natural):**
```
comenzar programa
crear variable entero llamada edad valor 25
si edad mayor que 18
    mostrar Es mayor de edad
sino
    mostrar Es menor de edad
terminar programa
```

**Salida (C++ Generado):**
```cpp
#include <iostream>
using namespace std;

int main() {
    int edad = 25;
    if (edad > 18) {
        cout << "Es mayor de edad" << endl;
    } else {
        cout << "Es menor de edad" << endl;
    }
    return 0;
}
```

---

## Caracteristicas Principales

- **Conversion DSL a C++**: Traduce español a codigo funcional
- **Validacion Dual**: Sintaxis interna + compilacion externa (opcional)
- **27 Plantillas DSL**: Snippets predefinidos organizados en 6 categorias
- **Autocompletado**: 45+ palabras clave con sugerencias inteligentes
- **Recarga Automatica**: Detecta cambios en archivos y recarga
- **Tema Zenitsu**: Interfaz inspirada en el anime con colores thunder

### Estructuras de Datos Propias
- **LinkedList<T>**: Lista enlazada generica
- **Stack<T>**: Pila con metodos push/pop
- **Queue<T>**: Cola FIFO
- **SymbolTable**: Tabla de simbolos para variables
- **DslList**: Manejo de listas tipadas

### Tipos de Datos Soportados
- **Entero**: Variables y listas de numeros enteros
- **Decimal**: Variables y listas de numeros decimales
- **Texto**: Cadenas de caracteres con escape
- **Caracter**: Caracteres individuales
- **Booleano**: Valores true/false

---

## Instalacion

### Requisitos
- Windows 10/11 (64-bit)
- Qt 6.9.3 o superior
- Visual Studio 2022 (MSVC)
- RAM: 4GB minimo, 8GB recomendado

### Pasos
1. Clonar repositorio
2. Abrir en Qt Creator: `Proyecto_EstructurasH1.vcxproj`
3. Compilar: `Ctrl+B` o `qmake && make`
4. Ejecutar: `./release/Proyecto_EstructurasH1.exe`

---

## Uso del Programa

### Atajos de Teclado
| Atajo | Accion |
|-------|--------|
| `Ctrl+N` | Nuevo archivo |
| `Ctrl+O` | Cargar archivo |
| `Ctrl+P` | Procesar DSL |
| `Ctrl+V` | Validar codigo |
| `Ctrl+S` | Guardar C++ |
| `F1` | Ayuda completa |

### Flujo de Trabajo
1. **Escribir DSL**: Algoritmo en español
2. **Procesar**: Convertir a C++ (`Ctrl+P`)
3. **Validar**: Verificar sintaxis (`Ctrl+V`)
4. **Guardar**: Exportar codigo C++ (`Ctrl+S`)

---

## Sintaxis DSL

### Estructura Basica
```dsl
comenzar programa
    # Tu codigo aqui
terminar programa
```

### Variables
```dsl
crear variable entero llamada edad valor 25
crear variable decimal llamada altura valor 1.75
crear variable texto llamada nombre valor Harold
crear variable caracter llamada inicial valor H
crear variable booleano llamada activo valor true
```

### Listas
```dsl
crear lista de enteros con 5 elementos llamada numeros
asignar 100 a numeros en posicion 0
para cada num en numeros
    mostrar num
```

### Operaciones
```dsl
sumar 10 y 20 y mostrar resultado
restar 100 y 30
multiplicar precio y 2
dividir total y cantidad
```

### Control de Flujo
```dsl
si edad mayor que 18
    mostrar Mayor de edad
sino
    mostrar Menor de edad

mientras contador menor que 10
    sumar contador y 1

for i desde 0 hasta 9 paso 1
    mostrar i

PORT 5 veces
    mostrar Hola
```

### Comparadores
- `igual a` → `==`
- `distinto de` → `!=`
- `mayor que` → `>`
- `menor que` → `<`
- `mayor o igual que` → `>=`
- `menor o igual que` → `<=`

### Operadores Logicos
- `y` → `&&`
- `o` → `||`
- `no` → `!`

---

## Ejemplos

La carpeta `ejemplos/` contiene 11 archivos que demuestran todas las funcionalidades:

1. **00_test_simple.txt** - Prueba basica de funcionamiento
2. **01_basico.txt** - Variables y salida basica
3. **02_listas.txt** - Manejo de listas/arreglos
4. **03_operaciones.txt** - Operaciones aritmeticas
5. **04_control.txt** - Control de flujo completo
6. **05_completo.txt** - Sistema academico completo
7. **06_port.txt** - Bucles PORT en 3 variantes
8. **07_entrada_salida.txt** - Interaccion con usuario
9. **08_tipos_datos.txt** - Todos los tipos de datos
10. **09_algoritmos.txt** - Algoritmos comunes
11. **10_casos_uso.txt** - Aplicaciones empresariales

### Cargar Ejemplo
1. Abrir C_HaroldA
2. `Ctrl+O` → Navegar a `ejemplos/`
3. Seleccionar archivo (ej: `01_basico.txt`)
4. `Ctrl+P` para procesar

---

## Validacion de Codigo

### Validacion Dual
1. **Interna**: Sintaxis C++ basica (siempre disponible)
2. **Externa**: Compilacion real con g++/clang++ (opcional)

### Configurar Compilador Externo
```bash
# Instalar MSYS2
pacman -S mingw-w64-x86_64-gcc

# Verificar instalacion
g++ --version
```

---

## Troubleshooting

### Errores Comunes Corregidos ✅
1. **"valor decimal invalido"** → Usar punto, no coma (1.5 no 1,5)
2. **"identificador invalido"** → Solo letras, numeros, _ sin espacios
3. **"instruccion no reconocida"** → Verificar sintaxis exacta
4. **"El archivo esta vacio"** → Incluir comenzar/terminar programa
5. **Comillas problemáticas** → Evitar comillas en mostrar y asignar
6. **Operador 'entre' en divisiones** → Usar 'y' en lugar de 'entre'
7. **Caracteres especiales** → Evitar acentos y simbolos especiales
8. **PORT no reconocido** → Agregado soporte completo
9. **comentario no reconocido** → Agregado soporte
10. **Asignaciones de texto** → Corregido manejo sin comillas

### Sintaxis Actualizada
- ✅ `dividir 100 y 4` (correcto)
- ❌ `dividir 100 entre 4` (incorrecto)
- ✅ `mostrar Hola mundo` (correcto)
- ❌ `mostrar "Hola mundo"` (problemático)
- ✅ `asignar Maria a nombre` (correcto)
- ❌ `asignar "Maria" a nombre` (problemático)

---

## Arquitectura

### Componentes Principales
- **Proyecto_EstructurasH1**: Ventana principal con tema Zenitsu
- **NaturalLanguageProcessor**: Convertidor DSL a C++
- **EditorController**: Manejo de archivos
- **ProcessingController**: Orquestacion de procesamiento
- **DSLAssistantPanel**: Snippets y autocompletado

### Estructuras Propias
- **LinkedList**: Lista enlazada thread-safe
- **Stack**: Pila LIFO
- **Queue**: Cola FIFO
- **SymbolTable**: Tabla hash para variables

---

## Creditos

**Desarrollador:** Harold Avia Hernandez
- Diseño e implementacion completa
- Arquitectura modular
- Interfaz de usuario con tema Zenitsu

**Tecnologias:**
- Qt 6.9.3
- C++14
- MSVC 2022

**Proposito Academico:**
- Curso de Estructuras de Datos
- Enseñanza de programacion en español
- Demostracion de arquitectura modular

---

## Estado de Ejemplos

### ✅ **Ejemplos Funcionando (12/12)**
- `00_test_simple.txt` - ✅ Funcionando
- `01_basico.txt` - ✅ Funcionando
- `02_listas.txt` - ✅ Corregido y funcionando
- `03_operaciones.txt` - ✅ Corregido y funcionando
- `04_control.txt` - ✅ Corregido y funcionando
- `05_completo.txt` - ✅ Corregido y funcionando
- `06_port.txt` - ✅ Corregido y funcionando
- `07_entrada_salida.txt` - ✅ Corregido y funcionando
- `08_tipos_datos.txt` - ✅ Corregido y funcionando
- `09_algoritmos.txt` - ✅ Corregido y funcionando
- `10_casos_uso.txt` - ✅ Corregido y funcionando
- `11_test_listas_fix.txt` - ✅ Funcionando

### 🔧 **Correcciones Realizadas**
- Eliminadas comillas problemáticas en mostrar/asignar
- Cambiado 'entre' por 'y' en operaciones de división
- Corregido error de duplicación en terminar programa
- Agregado soporte para PORT y comentario en validador
- Eliminados caracteres especiales y acentos
- Simplificada sintaxis de asignación de texto
- **🆕 Eliminados comentarios inline (#)** - Causaban errores de validación
- **🆕 Corregida función extract_ints** - Para detectar correctamente tamaños de listas

### 🐛 **Problemas Específicos Solucionados**

#### **Problema 1: Comentarios inline con #**
```
❌ asignar 10 a lista en posicion 0  # comentario
✅ comentario Asignar valor
    asignar 10 a lista en posicion 0
```

#### **Problema 2: Tamaños de listas no detectados**
```
❌ L5: tamano de lista/arreglo invalido o ausente
✅ Función extract_ints corregida - detecta números correctamente
```

#### **Problema 3: Identificadores con espacios/símbolos**
```
❌ Monitor 24 pulgadas  # Con espacio y números
✅ Monitor_24_pulgadas  # Solo letras, números y _

