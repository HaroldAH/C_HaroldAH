#include <iostream>
#include <string>
using namespace std;

int calcularPromedio(int num1, int num2) {
    int resultado = num1 + num2;
    return resultado / 2;
}
void validarDatos(int numero) {
    if (numero > 0) cout << "Dato valido" << endl;
    else            cout << "Dato invalido" << endl;
}
void procesarLista(int* lista, int n) {
    for (int i = 0; i < n; ++i) cout << lista[i] << endl;
}

int main() {
    int numero = 10;
    double altura = 1.750000;
    string nombre = "Harold";
    char inicial = 'H';
    bool activo = true;
    int sumaBasica = 15 + 30 + 45;
    int restaBasica = 100 - 25;
    int productoBasico = 8 * 9 * 2;
    double divisionBasica = 144.0 / 12.0;
    int datos[5] = {0};
    int suma1 = 10 + 20;
    cout << "El resultado es: " << suma1 << endl;
    cin >> numero;
    cout << numero << endl;
    cout << "Hola mundo" << endl;
    numero = 25;
    return 0;
}
