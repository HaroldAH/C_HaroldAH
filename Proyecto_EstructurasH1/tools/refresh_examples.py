import shutil, os, io

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXDIR = os.path.join(ROOT, "dsl_examples")

FILES = {
    "01_for.txt": """comenzar programa
for i desde 0 hasta 5
    mostrar i
terminar programa
""",
    "02_suma.txt": """comenzar programa
sumar 5 y 7 y mostrar resultado
sumar 10, 20, 30 y mostrar resultado
terminar programa
""",
    "03_mientras.txt": """comenzar programa
crear variable entero llamada contador valor 0
mientras contador menor que 3
    mostrar contador
    asignar valor contador+1 a contador
terminar programa
""",
    "04_repetir_hasta.txt": """comenzar programa
crear variable entero llamada edad valor 25
repetir hasta edad mayor o igual que 28
    asignar valor edad+1 a edad
    mostrar edad
terminar programa
""",
    "05_para_cada.txt": """comenzar programa
crear lista de enteros con 3 elementos llamada numeros
asignar 10 a numeros en posicion 0
asignar 20 a numeros en posicion 1
asignar 30 a numeros en posicion 2
para cada n en numeros
    mostrar n
terminar programa
""",
    "06_listas_asignacion.txt": """comenzar programa
crear lista de texto con 3 elementos llamada nombres
asignar Harold a nombres en posicion 0
asignar Ana a nombres en posicion 1
asignar Luis a nombres en posicion 2
para cada nombre en nombres
    mostrar nombre
terminar programa
""",
    "07_io_y_condiciones.txt": """comenzar programa
crear variable entero llamada edad valor 0
crear variable booleano llamada activo valor true
leer edad
si edad mayor o igual que 18 y activo igual a true
    mostrar Acceso permitido
sino
    mostrar Acceso denegado
terminar programa
""",
    "08_completo.txt": """comenzar programa
# Variables
crear variable entero llamada edad valor 17
crear variable texto llamada nombre valor Harold
crear variable booleano llamada activo valor true

# Listas
crear lista de enteros con 4 elementos llamada numeros
asignar 5 a numeros en posicion 0
asignar 10 a numeros en posicion 1
asignar 15 a numeros en posicion 2
asignar 20 a numeros en posicion 3

# Operaciones
sumar 5 y 10 y mostrar resultado

# Mientras
mientras edad menor que 20
    asignar valor edad+1 a edad

# Para cada
para cada n en numeros
    mostrar n

# For
for i desde 0 hasta 3
    mostrar i

# Condicion final
si edad mayor o igual que 18 y activo igual a true
    mostrar Bienvenido
sino
    mostrar Pendiente
terminar programa
""",
    "99_erroneo.txt": """comenzar programa
crear variable llamada edad valor 25       # falta tipo
sumar 10 y 2d y mostrar resultado          # '2d' no es entero puro
crear lista con 3 elementos llamada datos  # falta tipo de lista
si edad >> 18                              # operador invalido en DSL
terminar programa
"""
}

def ensure_dir(p):
    os.makedirs(p, exist_ok=True)

def clean_txts(dirpath):
    for name in os.listdir(dirpath):
        if name.lower().endswith(".txt"):
            try:
                os.remove(os.path.join(dirpath, name))
            except FileNotFoundError:
                pass

def write_utf8(path, content):
    with io.open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)

def main():
    ensure_dir(EXDIR)
    clean_txts(EXDIR)
    for fname, text in FILES.items():
        write_utf8(os.path.join(EXDIR, fname), text)
    print(f"Regenerados {len(FILES)} archivos en {EXDIR}")

if __name__ == "__main__":
    main()
