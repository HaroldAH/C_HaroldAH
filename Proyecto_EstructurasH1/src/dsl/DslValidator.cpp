#include "stdafx.h"
#include "DslValidator.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

std::string DslValidator::to_lower_copy(const std::string& s)
{
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char ch){ return std::tolower(ch); });
    return r;
}

void DslValidator::trim(std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) { s.clear(); return; }
    s = s.substr(a, b-a+1);
}

bool DslValidator::is_identifier(const std::string& s)
{
    if (s.empty()) return false;
    if (!(std::isalpha((unsigned char)s[0]) || s[0]=='_')) return false;
    for (char c: s) {
        if (!(std::isalnum((unsigned char)c) || c=='_')) return false;
    }
    return true;
}

std::string DslValidator::read_identifier_after(const std::string& lower, const std::string& original, const std::string& keyword)
{
    size_t pos = lower.find(keyword);
    if (pos == std::string::npos) return std::string();
    pos += keyword.size();
    while (pos < original.size() && (original[pos]==' ' || original[pos]=='\t')) pos++;
    size_t start = pos;
    while (pos < original.size() && (std::isalnum((unsigned char)original[pos]) || original[pos]=='_')) pos++;
    if (pos>start) return original.substr(start, pos-start);
    return std::string();
}

std::string DslValidator::read_value_after(const std::string& lower, const std::string& original, const std::string& keyword)
{
    size_t pos = lower.find(keyword);
    if (pos == std::string::npos) return std::string();
    pos += keyword.size();
    while (pos < original.size() && (original[pos]==' ' || original[pos]=='\t')) pos++;
    std::string value = original.substr(pos);
    // recorta trailing
    size_t last = value.find_last_not_of(" \t\r\n");
    if (last != std::string::npos) value.erase(last+1);
    return value;
}

int DslValidator::extract_ints(const std::string& str, int out[], int maxn)
{
    // Buscar específicamente números enteros en la cadena
    std::istringstream iss(str);
    std::string token;
    int count = 0;
    
    while (count < maxn && iss >> token) {
        // Verificar si el token es un número entero válido
        auto isInt = [](const std::string& s) -> bool {
            if (s.empty()) return false;
            size_t i = 0;
            if (s[0] == '+' || s[0] == '-') i = 1;
            if (i >= s.size()) return false;
            for (; i < s.size(); ++i) {
                if (!std::isdigit((unsigned char)s[i])) return false;
            }
            return true;
        };
        
        if (isInt(token)) {
            try {
                int num = std::stoi(token);
                out[count++] = num;
            } catch (...) {
                // Si no se puede convertir, continuar
                continue;
            }
        }
    }
    
    return count;
}

bool DslValidator::validateInput(const std::string& input, LinkedList<std::string>& errors, bool& lastDslOk)
{
    try {
        if (input.empty()) {
            errors.add("El archivo esta vacio");
            lastDslOk = false;
            return false;
        }

        std::istringstream iss(input);
        std::string line;
        int lineNo = 0;
        
        while (std::getline(iss, line)) {
            lineNo++;
            std::string raw = line;
            trim(raw);
            if (raw.empty() || raw[0]=='#') continue;

            std::string lower = to_lower_copy(raw);
            bool recognized = false;

            // CREAR VARIABLE ...
            if (lower.find("crear variable") != std::string::npos) {
                recognized = true;
                bool hasType = (lower.find("entero")!=std::string::npos ||
                                lower.find("decimal")!=std::string::npos ||
                                lower.find("texto")!=std::string::npos ||
                                lower.find("caracter")!=std::string::npos ||
                                lower.find("booleano")!=std::string::npos);
                if (!hasType) errors.add("L" + std::to_string(lineNo) + ": falta el tipo en 'crear variable'.");

                std::string name = read_identifier_after(lower, raw, "llamada ");
                if (!is_identifier(name)) errors.add("L" + std::to_string(lineNo) + ": identificador invalido o ausente en 'llamada'.");

                if (lower.find("entero")!=std::string::npos) {
                    std::string v = read_value_after(lower, raw, "valor ");
                    if (!v.empty()) {
                        try { (void)std::stoi(v); } catch(...) {
                            errors.add("L" + std::to_string(lineNo) + ": valor entero invalido.");
                        }
                    }
                } else if (lower.find("decimal")!=std::string::npos) {
                    std::string v = read_value_after(lower, raw, "valor ");
                    if (!v.empty()) {
                        try { (void)std::stod(v); } catch(...) {
                            errors.add("L" + std::to_string(lineNo) + ": valor decimal invalido.");
                        }
                    }
                } else if (lower.find("booleano")!=std::string::npos) {
                    std::string v = to_lower_copy(read_value_after(lower, raw, "valor "));
                    if (!v.empty() && !(v=="true" || v=="false")) {
                        errors.add("L" + std::to_string(lineNo) + ": booleano debe ser true/false.");
                    }
                } else if (lower.find("caracter")!=std::string::npos) {
                    std::string v = read_value_after(lower, raw, "valor ");
                    if (!v.empty() && v.size()!=1) {
                        errors.add("L" + std::to_string(lineNo) + ": caracter debe ser un unico simbolo.");
                    }
                }
            }
            // CREAR LISTA/ARREGLO ...
            else if (lower.find("crear lista") != std::string::npos || lower.find("crear arreglo") != std::string::npos) {
                recognized = true;
                int nums[5]; int c = extract_ints(raw, nums, 5);
                if (c==0 || nums[0] <= 0) {
                    errors.add("L" + std::to_string(lineNo) + ": tamano de lista/arreglo invalido o ausente.");
                }
                std::string name = read_identifier_after(lower, raw, "llamada ");
                if (!name.empty() && !is_identifier(name)) {
                    errors.add("L" + std::to_string(lineNo) + ": identificador de lista invalido.");
                }
                bool typed = (lower.find("enteros")!=std::string::npos ||
                              lower.find("decimales")!=std::string::npos ||
                              lower.find("texto")!=std::string::npos ||
                              lower.find("caracteres")!=std::string::npos);
                if (!typed) errors.add("L" + std::to_string(lineNo) + ": especifique el tipo de la lista (enteros/decimales/texto/caracteres).");
            }
            // OPERACIONES + "mostrar resultado"
            else if ((lower.find("sumar")!=std::string::npos ||
                      lower.find("restar")!=std::string::npos ||
                      lower.find("multiplicar")!=std::string::npos ||
                      lower.find("dividir")!=std::string::npos) &&
                     lower.find("mostrar resultado")!=std::string::npos) {

                recognized = true;

                auto isPureInt = [](const std::string& tok)->bool {
                    if (tok.empty()) return false;
                    size_t i = 0;
                    if (tok[0]=='+' || tok[0]=='-') i = 1;
                    if (i >= tok.size()) return false;
                    for (; i<tok.size(); ++i) {
                        unsigned char ch = (unsigned char)tok[i];
                        if (!std::isdigit(ch)) return false;
                    }
                    return true;
                };
                auto lowerTok = [](std::string s){
                    std::transform(s.begin(), s.end(), s.begin(),
                                   [](unsigned char c){ return (char)std::tolower(c); });
                    return s;
                };

                std::istringstream issTokens(raw);
                std::string tok;
                int numsFound = 0;
                bool badNumericToken = false;

                while (issTokens >> tok) {
                    std::string lt = lowerTok(tok);
                    if (lt == "y") continue;

                    bool hasDigit = false;
                    for (char c: tok) if (std::isdigit((unsigned char)c)) { hasDigit = true; break; }

                    if (hasDigit) {
                        if (!isPureInt(tok)) {
                            badNumericToken = true;
                            break;
                        }
                        numsFound++;
                    }
                }

                if (badNumericToken) {
                    errors.add("L" + std::to_string(lineNo) + ": numero invalido en la operacion (use enteros puros: 10 20 30).");
                } else if (numsFound < 2) {
                    errors.add("L" + std::to_string(lineNo) + ": se requieren al menos 2 numeros para la operacion.");
                }
            }
            // OPERACIONES sin "mostrar resultado": aceptadas como instrucciones válidas
            else if ((lower.rfind("sumar ",0)==0 ||
                      lower.rfind("restar ",0)==0 ||
                      lower.rfind("multiplicar ",0)==0 ||
                      lower.rfind("dividir ",0)==0) &&
                     lower.find("mostrar resultado")==std::string::npos) {
                recognized = true;
            }
            // ASIGNAR VALOR X A Y (versión completa)
            else if (lower.find("asignar valor") != std::string::npos) {
                recognized = true;
                size_t valorPos = lower.find("valor");
                size_t aPos = lower.find(" a ");
                if (valorPos==std::string::npos || aPos==std::string::npos || aPos <= valorPos) {
                    errors.add("L" + std::to_string(lineNo) + ": formato invalido en 'asignar valor ... a ...'.");
                } else {
                    std::string varName = raw.substr(aPos+3);
                    trim(varName);
                    
                    // Verificar si es asignación a array (contiene "en posicion")
                    std::string varNameLower = to_lower_copy(varName);
                    if (varNameLower.find("en posicion") != std::string::npos) {
                        // Es asignación a array: "asignar valor X a lista en posicion Y"
                        size_t enPos = varNameLower.find(" en posicion");
                        std::string arrName = varName.substr(0, enPos);
                        trim(arrName);
                        
                        if (!is_identifier(arrName)) {
                            errors.add("L" + std::to_string(lineNo) + ": nombre de lista invalido en asignacion.");
                        }
                        
                        // Validar índice - ACEPTA NÚMERO O IDENTIFICADOR
                        std::string indexPart = varName.substr(enPos + 12);
                        trim(indexPart);
                        if (indexPart.empty()) {
                            errors.add("L" + std::to_string(lineNo) + ": falta indice en asignacion a lista.");
                        } else {
                            auto isInt = [](const std::string& s){
                                if (s.empty()) return false;
                                size_t i = (s[0]=='+'||s[0]=='-')?1:0;
                                if (i>=s.size()) return false;
                                for (; i<s.size(); ++i) if(!std::isdigit((unsigned char)s[i])) return false;
                                return true;
                            };
                            if (!(isInt(indexPart) || is_identifier(indexPart))) {
                                errors.add("L" + std::to_string(lineNo) + ": indice debe ser entero o identificador.");
                            }
                        }
                    } else {
                        // Es asignación normal: "asignar valor X a variable"
                        if (!is_identifier(varName)) {
                            errors.add("L" + std::to_string(lineNo) + ": identificador invalido en asignacion.");
                        }
                    }
                }
            }
            // Aceptar "asignar [valor] X a Y [en posicion I]"
            else if (lower.rfind("asignar ", 0) == 0) {
                recognized = true;

                size_t startVal = 8; // después de "asignar "
                // si viene la palabra "valor " justo aquí, saltarla
                if (lower.find("valor ", startVal) == startVal) startVal += 6;

                size_t aPos = lower.find(" a ", startVal);
                if (aPos == std::string::npos) {
                    errors.add("L" + std::to_string(lineNo) + ": formato invalido en 'asignar ... a ...'.");
                } else {
                    std::string value = raw.substr(startVal, aPos - startVal);
                    trim(value);

                    std::string target = raw.substr(aPos + 3);
                    trim(target);

                    std::string targetLower = to_lower_copy(target);
                    if (targetLower.find("en posicion") != std::string::npos) {
                        size_t enPos = targetLower.find(" en posicion");
                        std::string arrName = target.substr(0, enPos);
                        trim(arrName);
                        if (!is_identifier(arrName)) {
                            errors.add("L" + std::to_string(lineNo) + ": nombre de lista invalido en asignacion.");
                        }
                        std::string indexPart = target.substr(enPos + 12);
                        trim(indexPart);
                        if (indexPart.empty()) {
                            errors.add("L" + std::to_string(lineNo) + ": falta indice en asignacion a lista.");
                        } else {
                            auto isInt = [](const std::string& s){
                                if (s.empty()) return false;
                                size_t i = (s[0]=='+'||s[0]=='-')?1:0;
                                if (i>=s.size()) return false;
                                for (; i<s.size(); ++i) if(!std::isdigit((unsigned char)s[i])) return false;
                                return true;
                            };
                            if (!(isInt(indexPart) || is_identifier(indexPart))) {
                                errors.add("L" + std::to_string(lineNo) + ": indice debe ser entero o identificador.");
                            }
                        }
                    } else {
                        if (!is_identifier(target)) {
                            errors.add("L" + std::to_string(lineNo) + ": identificador invalido en asignacion.");
                        }
                    }
                }
            }
            // FOR/PARA clasico: for i desde 0 hasta 10 paso 2 O para i desde 0 hasta 10 paso 2
            else if ((lower.find("for ") == 0 || lower.find("para ") == 0) && lower.find(" desde ") != std::string::npos && lower.find(" hasta ") != std::string::npos) {
                recognized = true;
                // valida identificador
                size_t forPos = (lower.find("for ") == 0) ? 4 : 5;
                size_t desdePos = lower.find(" desde ");
                std::string id = raw.substr(forPos, desdePos - forPos);
                trim(id);
                if (!is_identifier(id)) {
                    errors.add("L" + std::to_string(lineNo) + ": identificador invalido en 'for/para'.");
                }
                // valida enteros
                size_t hastaPos = lower.find(" hasta ");
                if (hastaPos == std::string::npos || hastaPos <= desdePos) {
                    errors.add("L" + std::to_string(lineNo) + ": falta 'hasta' en for/para.");
                } else {
                    // ini y fin
                    std::string ini = raw.substr(desdePos + 7, hastaPos - (desdePos + 7));
                    trim(ini);
                    std::string rest = raw.substr(hastaPos + 7);
                    std::string fin = rest;
                    std::string paso;
                    size_t pasoPos = lower.find(" paso ");
                    if (pasoPos != std::string::npos && pasoPos > hastaPos) {
                        fin = raw.substr(hastaPos + 7, pasoPos - (hastaPos + 7));
                        paso = raw.substr(pasoPos + 6);
                        trim(fin); trim(paso);
                    } else { trim(fin); }
                    auto isInt = [&](const std::string& s){
                        if (s.empty()) return false;
                        size_t i = (s[0]=='+'||s[0]=='-')?1:0;
                        if (i>=s.size()) return false;
                        for (; i<s.size(); ++i) if(!std::isdigit((unsigned char)s[i])) return false;
                        return true;
                    };
                    if (!isInt(ini) || !isInt(fin) || (!paso.empty() && !isInt(paso))) {
                        errors.add("L" + std::to_string(lineNo) + ": for/para requiere enteros puros en desde/hasta/paso.");
                    }
                }
            }
            // REPETIR HASTA
            else if (lower.find("repetir hasta ") == 0) {
                recognized = true;
                // no validamos profundamente la condicion aquí
            }
            // PORT - NUEVAS VARIANTES AGREGADAS
            else if (lower.rfind("port ", 0) == 0) {
                recognized = true;
                
                if (lower.find(" veces") != std::string::npos) {
                    // PORT N veces
                    int nums[5];
                    int c = extract_ints(raw, nums, 5);
                    if (c == 0 || nums[0] <= 0) {
                        errors.add("L" + std::to_string(lineNo) + ": PORT veces requiere numero positivo.");
                    }
                } else if (lower.find("cada ") != std::string::npos && lower.find(" en ") != std::string::npos) {
                    // PORT cada item en lista
                    size_t cadaPos = lower.find("cada ");
                    size_t enPos = lower.find(" en ");
                    if (cadaPos == std::string::npos || enPos == std::string::npos || enPos <= cadaPos) {
                        errors.add("L" + std::to_string(lineNo) + ": formato invalido en 'PORT cada ... en ...'.");
                    } else {
                        std::string item = raw.substr(cadaPos + 5, enPos - (cadaPos + 5));
                        trim(item);
                        std::string list = raw.substr(enPos + 4);
                        trim(list);
                        
                        if (!is_identifier(item)) {
                            errors.add("L" + std::to_string(lineNo) + ": identificador de item invalido en PORT.");
                        }
                        if (!is_identifier(list)) {
                            errors.add("L" + std::to_string(lineNo) + ": identificador de lista invalido en PORT.");
                        }
                    }
                } else {
                    errors.add("L" + std::to_string(lineNo) + ": formato PORT invalido. Use 'PORT N veces' o 'PORT cada item en lista'.");
                }
            }
            // COMENTARIO
            else if (lower.rfind("comentario ", 0) == 0) {
                recognized = true;
            }
            // E/S y control básicos: aceptados (validaciones mas finas luego)
            else if (lower.find("leer")!=std::string::npos ||
                     lower.find("ingresar")!=std::string::npos ||
                     lower.find("mostrar")!=std::string::npos ||
                     lower.find("imprimir")!=std::string::npos ||
                     lower.find("si ")!=std::string::npos ||
                     lower.find("sino")!=std::string::npos ||
                     lower.find("mientras")!=std::string::npos ||
                     lower.find("para cada")!=std::string::npos ||
                     lower.find("comenzar programa")!=std::string::npos ||
                     lower.find("terminar programa")!=std::string::npos ||
                     lower.find("definir funcion")!=std::string::npos) {
                recognized = true;
                // (Opcional) aquí puedes agregar checks por comparadores, etc.
            }

            if (!recognized) {
                errors.add("L" + std::to_string(lineNo) + ": instruccion no reconocida.");
            }
        }

        lastDslOk = (errors.getSize() == 0);
        return lastDslOk;
    }
    catch (...) {
        errors.add("Error interno durante la validacion de sintaxis DSL");
        lastDslOk = false;
        return false;
    }
}
