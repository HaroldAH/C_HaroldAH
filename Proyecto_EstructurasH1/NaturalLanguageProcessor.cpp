#include "stdafx.h"
#include "NaturalLanguageProcessor.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <limits>

using namespace std;


static int countIndent(const string& s) {
    int spaces = 0;
    for (char c : s) {
        if (c == ' ') spaces++;
        else if (c == '\t') spaces += 4;
        else break;
    }
    return spaces / 4;
}

static string trim_copy(string s) {
    auto issp = [](unsigned char ch){return ch==' '||ch=='\t'||ch=='\r'||ch=='\n';};
    while(!s.empty() && issp((unsigned char)s.front())) s.erase(s.begin());
    while(!s.empty() && issp((unsigned char)s.back()))  s.pop_back();
    return s;
}

static string lower_copy(string s){
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c){return (char)tolower(c);});
    return s;
}

// Escapar string para C++
static string escapeForCxxString(const string& s) {
    string result;
    result.reserve(s.size() + 10);
    for (char c : s) {
        if (c == '\\') result += "\\\\";
        else if (c == '"') result += "\\\"";
        else result += c;
    }
    return result;
}

// Reemplazo seguro de subcadenas (palabras completas)
static void replace_word(string& s, const string& from, const string& to) {
    string lower = lower_copy(s);
    for (size_t pos = 0;;) {
        pos = lower.find(from, pos);
        if (pos == string::npos) break;
        s.replace(pos, from.size(), to);
        lower.replace(pos, from.size(), to);
        pos += to.size();
    }
}

static string toCppCondition(string cond) {
    // normaliza espacios y minúsculas, pero mantenemos nombres/valores
    // Reemplazos ordenados (de más largos a más cortos)
    string w = " " + cond + " ";
    replace_word(w, " mayor o igual que ", " >= ");
    replace_word(w, " menor o igual que ", " <= ");
    replace_word(w, " distinto de ",       " != ");
    replace_word(w, " diferente de ",      " != ");
    replace_word(w, " igual a ",           " == ");
    replace_word(w, " mayor que ",         " > ");
    replace_word(w, " menor que ",         " < ");
    replace_word(w, " y ",                 " && ");
    replace_word(w, " o ",                 " || ");
    // ' no X' ? '!X'
    for (size_t pos = w.find(" no "); pos != string::npos; pos = w.find(" no ", pos+1)) {
        // elimina ' no ' y pone ' !'
        w.replace(pos, 4, " !");
    }
    // compacta espacios
    string out; out.reserve(w.size());
    bool prevSpace=false;
    for(char c: w){ bool sp=(c==' '||c=='\t'); if(!(sp && prevSpace)) out.push_back(sp?' ':c); prevSpace=sp; }
    // quita espacios extremos
    if(!out.empty() && out.front()==' ') out.erase(out.begin());
    if(!out.empty() && out.back()==' ')  out.pop_back();
    return out;
}

enum class BlockType { If, Else, While, DoUntil, For, Foreach };
struct Block { BlockType type; int indent; string extra; };



NaturalLanguageProcessor::NaturalLanguageProcessor() 
    : instructionCount(0), lastDSLok(false), lastCppOk(false) 
{
}

NaturalLanguageProcessor::~NaturalLanguageProcessor() 
{
}

bool NaturalLanguageProcessor::loadFile(const string& filePath)
{
    try {
        // Limpiar estado anterior al inicio
        clearErrors();
        generatedCode.clear();
        lists.reset();
        
        ifstream file(filePath);
        if (!file.is_open()) {
            addError("No se pudo abrir el archivo: " + filePath);
            return false;
        }
        
        inputContent.clear();
        string line;
        while (getline(file, line)) {
            inputContent += line + "\n";
        }
        file.close();
        
        if (inputContent.empty()) {
            addError("El archivo esta vacio");
            return false;
        }
        
        // Parsing protegido
        try {
            parseInstructions(inputContent);
        }
        catch (const exception& e) {
            addError("Error al procesar instrucciones: " + string(e.what()));
            return false;
        }
        catch (...) {
            addError("Error desconocido al procesar instrucciones");
            return false;
        }
        
        return true;
    }
    catch (const exception& e) {
        addError("Error al cargar archivo: " + string(e.what()));
        return false;
    }
    catch (...) {
        addError("Error desconocido al cargar archivo");
        return false;
    }
}

bool NaturalLanguageProcessor::saveToFile(const string& filePath)
{
    ofstream file(filePath);
    if (!file.is_open()) {
        addError("No se pudo crear el archivo: " + filePath);
        return false;
    }
    
    file << generatedCode;
    file.close();
    return true;
}

string NaturalLanguageProcessor::getInputContent() const
{
    return inputContent;
}

void NaturalLanguageProcessor::setInputContent(const string& content) 
{
    inputContent = content;
}

bool NaturalLanguageProcessor::validateDSLOnly(const string& content)
{
    // Guarda el estado anterior
    string oldContent = inputContent;
    
    // Establece el nuevo contenido temporalmente
    inputContent = content;
    errors.clear();
    
    // Valida solo DSL
    bool result = validateInputSyntax();
    
    // Restaura contenido (pero NO errors - queremos mantener los errores encontrados)
    inputContent = oldContent;
    
    return result;
}

bool NaturalLanguageProcessor::processText(const string& input) 
{
    try {
        if (input.empty()) {
            addError("El texto de entrada esta vacio");
            return false;
        }
        
        // Limpiar estado anterior al inicio
        clearErrors();
        generatedCode.clear();
        lists.reset();
        
        inputContent = input;
        
        // VALIDAR DSL ANTES DE PROCESAR - MANEJO SEGURO SIN CRASH
        try {
            if (!validateInputSyntax()) {
                lastDSLok = false;
                return false;  // NO CRASH - solo retorna false
            }
            
            lastDSLok = true;
            
            // Parsing protegido
            try {
                parseInstructions(input);
            }
            catch (const exception& e) {
                addError("Error al parsear instrucciones: " + string(e.what()));
                lastDSLok = false;
                return false;
            }
            catch (...) {
                addError("Error desconocido al parsear instrucciones");
                lastDSLok = false;
                return false;
            }
            
            // Generación protegida
            try {
                generateCode();
            }
            catch (const exception& e) {
                addError("Error al generar codigo: " + string(e.what()));
                return false;
            }
            catch (...) {
                addError("Error desconocido al generar codigo");
                return false;
            }
            
            // Validación protegida
            try {
                validateCode();
            }
            catch (const exception& e) {
                addError("Error al validar codigo: " + string(e.what()));
                return false;
            }
            catch (...) {
                addError("Error desconocido al validar codigo");
                return false;
            }
            
            return errors.getSize() == 0;
        }
        catch (const exception& e) {
            addError("Error durante validacion DSL: " + string(e.what()));
            lastDSLok = false;
            return false;
        }
        catch (...) {
            addError("Error desconocido durante validacion DSL");
            lastDSLok = false;
            return false;
        }
    }
    catch (const exception& e) {
        addError("Error interno durante el procesamiento: " + string(e.what()));
        lastDSLok = false;
        return false;
    }
    catch (...) {
        addError("Error interno desconocido durante el procesamiento");
        lastDSLok = false;
        return false;
    }
}

void NaturalLanguageProcessor::processInstructions()
{
    try {
        clearErrors();                 // limpia errores anteriores
        lastDSLok = validateInputSyntax();  // 1) valida el TXT/DSL primero

        if (!lastDSLok) {
            generatedCode.clear();     // no generes nada si el DSL es inválido
            // NO HACER THROW O CRASH - solo retornar graciosamente
            return;
        }

        generateCode();                // 2) genera C++
        // 3) valida C++ generado
        lastCppOk = validateSyntax(generatedCode);
    }
    catch (...) {
        addError("Error interno durante el procesamiento de instrucciones");
        lastDSLok = false;
        lastCppOk = false;
        generatedCode.clear();
    }
}

string NaturalLanguageProcessor::convertToCode(const string& naturalLanguage) 
{
    string code;
    
    if (naturalLanguage.find("crear variable") != string::npos) {
        code += "int variable;\n";
    }
    if (naturalLanguage.find("mostrar") != string::npos) {
        code += "cout << \"Hola Mundo\" << endl;\n";
    }
    
    generatedCode = code;
    return code;
}

bool NaturalLanguageProcessor::validateSyntax()
{
    return validateSyntax(generatedCode);
}

bool NaturalLanguageProcessor::validateSyntax(const string& code) 
{
    try {
        if (code.empty()) {
            addError("El codigo esta vacio");
            lastCppOk = false;
            return false;
        }

        int brace=0, paren=0;
        bool inString=false, inChar=false, escape=false;

        istringstream iss(code);
        string line;

        while (getline(iss, line)) {
            for (size_t i=0; i<line.size(); ++i) {
                char c = line[i];
                if (escape) { escape=false; continue; }

                if (inString) { 
                    if (c=='\\') escape=true; 
                    else if (c=='"') inString=false; 
                    continue; 
                }
                if (inChar) { 
                    if (c=='\\') escape=true; 
                    else if (c=='\'') inChar=false; 
                    continue; 
                }

                if (c=='"') { inString=true; continue; }
                if (c=='\'') { inChar=true; continue; }

                if (c=='{') ++brace;
                else if (c=='}') --brace;
                else if (c=='(') ++paren;
                else if (c==')') --paren;
            }
        }

        if (brace!=0) addError("Llaves no balanceadas");
        if (paren!=0) addError("Parentesis no balanceados");
        if (inString) addError("Comillas dobles no cerradas");
        if (inChar) addError("Comillas simples nocerradas");

        lastCppOk = (errors.getSize()==0);
        return lastCppOk;
    }
    catch (...) {
        addError("Error interno durante la validacion de sintaxis C++");
        lastCppOk = false;
        return false;
    }
}

bool NaturalLanguageProcessor::validateInputSyntax()
{
    try {
        // NO borrar errores aquí si processInstructions() ya llamó clearErrors().
        if (inputContent.empty()) {
            addError("El archivo esta vacio");
            lastDSLok = false;
            return false;
        }

        auto to_lower_copy = [](const string& s){
            string r = s;
            transform(r.begin(), r.end(), r.begin(),
                           [](unsigned char ch){ return tolower(ch); });
            return r;
        };
        auto trim = [](string& s){
            size_t a = s.find_first_not_of(" \t\r\n");
            size_t b = s.find_last_not_of(" \t\r\n");
            if (a==string::npos) { s.clear(); return; }
            s = s.substr(a, b-a+1);
        };
        auto is_identifier = [](const string& s){
            if (s.empty()) return false;
            if (!(isalpha((unsigned char)s[0]) || s[0]=='_')) return false;
            for (char c: s) {
                if (!(isalnum((unsigned char)c) || c=='_')) return false;
            }
            return true;
        };
        auto read_identifier_after = [](const string& lower, const string& original, const string& keyword){
            size_t pos = lower.find(keyword);
            if (pos == string::npos) return string();
            pos += keyword.size();
            while (pos < original.size() && (original[pos]==' ' || original[pos]=='\t')) pos++;
            size_t start = pos;
            while (pos < original.size() && (isalnum((unsigned char)original[pos]) || original[pos]=='_')) pos++;
            if (pos>start) return original.substr(start, pos-start);
            return string();
        };
        auto read_value_after = [](const string& lower, const string& original, const string& keyword){
            size_t pos = lower.find(keyword);
            if (pos == string::npos) return string();
            pos += keyword.size();
            while (pos < original.size() && (original[pos]==' ' || original[pos]=='\t')) pos++;
            string value = original.substr(pos);
            // recorta trailing
            size_t last = value.find_last_not_of(" \t\r\n");
            if (last != string::npos) value.erase(last+1);
            return value;
        };
        auto extract_ints = [](const string& str, int out[], int maxn){
            // Buscar específicamente números enteros en la cadena
            istringstream iss(str);
            string token;
            int count = 0;
            
            while (count < maxn && iss >> token) {
                // Verificar si el token es un número entero válido
                auto isInt = [](const string& s) -> bool {
                    if (s.empty()) return false;
                    size_t i = 0;
                    if (s[0] == '+' || s[0] == '-') i = 1;
                    if (i >= s.size()) return false;
                    for (; i < s.size(); ++i) {
                        if (!isdigit((unsigned char)s[i])) return false;
                    }
                    return true;
                };
                
                if (isInt(token)) {
                    try {
                        int num = stoi(token);
                        out[count++] = num;
                    } catch (...) {
                        // Si no se puede convertir, continuar
                        continue;
                    }
                }
            }
            
            return count;
        };

        istringstream iss(inputContent);
        string line;
        int lineNo = 0;
        while (getline(iss, line)) {
            lineNo++;
            string raw = line;
            trim(raw);
            if (raw.empty() || raw[0]=='#') continue;

            string lower = to_lower_copy(raw);
            bool recognized = false;

            // CREAR VARIABLE ...
            if (lower.find("crear variable") != string::npos) {
                recognized = true;
                bool hasType = (lower.find("entero")!=string::npos ||
                                lower.find("decimal")!=string::npos ||
                                lower.find("texto")!=string::npos ||
                                lower.find("caracter")!=string::npos ||
                                lower.find("booleano")!=string::npos);
                if (!hasType) addError("L" + to_string(lineNo) + ": falta el tipo en 'crear variable'.");

                string name = read_identifier_after(lower, raw, "llamada ");
                if (!is_identifier(name)) addError("L" + to_string(lineNo) + ": identificador invalido o ausente en 'llamada'.");

                if (lower.find("entero")!=string::npos) {
                    string v = read_value_after(lower, raw, "valor ");
                    if (!v.empty()) {
                        try { (void)stoi(v); } catch(...) {
                            addError("L" + to_string(lineNo) + ": valor entero invalido.");
                        }
                    }
                } else if (lower.find("decimal")!=string::npos) {
                    string v = read_value_after(lower, raw, "valor ");
                    if (!v.empty()) {
                        try { (void)stod(v); } catch(...) {
                            addError("L" + to_string(lineNo) + ": valor decimal invalido.");
                        }
                    }
                } else if (lower.find("booleano")!=string::npos) {
                    string v = to_lower_copy(read_value_after(lower, raw, "valor "));

                    if (!v.empty() && !(v=="true" || v=="false")) {
                        addError("L" + to_string(lineNo) + ": booleano debe ser true/false.");
                    }
                } else if (lower.find("caracter")!=string::npos) {
                    string v = read_value_after(lower, raw, "valor ");
                    if (!v.empty() && v.size()!=1) {
                        addError("L" + to_string(lineNo) + ": caracter debe ser un unico simbolo.");
                    }
                }
            }
            // CREAR LISTA/ARREGLO ...
            else if (lower.find("crear lista") != string::npos || lower.find("crear arreglo") != string::npos) {
                recognized = true;
                int nums[5]; int c = extract_ints(raw, nums, 5);
                if (c==0 || nums[0] <= 0) {
                    addError("L" + to_string(lineNo) + ": tamano de lista/arreglo invalido o ausente.");
                }
                string name = read_identifier_after(lower, raw, "llamada ");
                if (!name.empty() && !is_identifier(name)) {
                    addError("L" + to_string(lineNo) + ": identificador de lista invalido.");
                }
                bool typed = (lower.find("enteros")!=string::npos ||
                              lower.find("decimales")!=string::npos ||
                              lower.find("texto")!=string::npos ||
                              lower.find("caracteres")!=string::npos);
                if (!typed) addError("L" + to_string(lineNo) + ": especifique el tipo de la lista (enteros/decimales/texto/caracteres).");
            }
            // OPERACIONES + "mostrar resultado" - Validación estricta de números
            else if ((lower.find("sumar")!=string::npos ||
                      lower.find("restar")!=string::npos ||
                      lower.find("multiplicar")!=string::npos ||
                      lower.find("dividir")!=string::npos) &&
                     lower.find("mostrar resultado")!=string::npos) {

                recognized = true;

                auto isPureInt = [](const string& tok)->bool {
                    if (tok.empty()) return false;
                    size_t i = 0;
                    if (tok[0]=='+' || tok[0]=='-') i = 1;
                    if (i >= tok.size()) return false;
                    for (; i<tok.size(); ++i) {
                        unsigned char ch = (unsigned char)tok[i];
                        if (!isdigit(ch)) return false;
                    }
                    return true;
                };
                auto lowerTok = [](string s){
                    transform(s.begin(), s.end(), s.begin(),
                                   [](unsigned char c){ return (char)tolower(c); });
                    return s;
                };

                istringstream issTokens(raw);
                string tok;
                int numsFound = 0;
                bool badNumericToken = false;

                while (issTokens >> tok) {
                    string lt = lowerTok(tok);
                    if (lt == "y") continue; // conector permitido

                    bool hasDigit = false;
                    for (char c: tok) if (isdigit((unsigned char)c)) { hasDigit = true; break; }

                    if (hasDigit) {
                        if (!isPureInt(tok)) { // ej. "1d0", "10," etc.
                            badNumericToken = true;
                            break;
                        }
                        // es entero puro
                        numsFound++;
                    }
                }

                if (badNumericToken) {
                    addError("L" + to_string(lineNo) + ": numero invalido en la operacion (use enteros puros: 10 20 30).");
                } else if (numsFound < 2) {
                    addError("L" + to_string(lineNo) + ": se requieren al menos 2 numeros para la operacion.");
                }
            }
            // OPERACIONES sin "mostrar resultado": aceptadas como instrucciones válidas
            else if ((lower.rfind("sumar ",0)==0 ||
                      lower.rfind("restar ",0)==0 ||
                      lower.rfind("multiplicar ",0)==0 ||
                      lower.rfind("dividir ",0)==0) &&
                     lower.find("mostrar resultado")==string::npos) {
                recognized = true;
            }
            // ASIGNAR VALOR X A Y
            else if (lower.find("asignar valor") != string::npos) {
                recognized = true;
                size_t valorPos = lower.find("valor");
                size_t aPos = lower.find(" a ");
                if (valorPos==string::npos || aPos==string::npos || aPos <= valorPos) {
                    addError("L" + to_string(lineNo) + ": formato invalido en 'asignar valor ... a ...'.");
                } else {
                    string varName = raw.substr(aPos+3);
                    trim(varName);
                    
                    // NUEVO: Verificar si es asignación a array (contiene "en posicion")
                    string varNameLower = to_lower_copy(varName);
                    if (varNameLower.find("en posicion") != string::npos) {
                        // Es asignación a array: "asignar valor X a lista en posicion Y"
                        size_t enPos = varNameLower.find(" en posicion");
                        string arrName = varName.substr(0, enPos);
                        trim(arrName);
                        
                        if (!is_identifier(arrName)) {
                            addError("L" + to_string(lineNo) + ": nombre de lista invalido en asignacion.");
                        }
                        
                        // Validar índice - ACEPTA NÚMERO O IDENTIFICADOR
                        string indexPart = varName.substr(enPos + 12);
                        trim(indexPart);
                        if (indexPart.empty()) {
                            addError("L" + to_string(lineNo) + ": falta indice en asignacion a lista.");
                        } else {
                            auto isInt = [](const string& s){
                                if (s.empty()) return false;
                                size_t i = (s[0]=='+'||s[0]=='-')?1:0;
                                if (i>=s.size()) return false;
                                for (; i<s.size(); ++i) if(!isdigit((unsigned char)s[i])) return false;
                                return true;
                            };
                            if (!(isInt(indexPart) || is_identifier(indexPart))) {
                                addError("L" + to_string(lineNo) + ": indice debe ser entero o identificador.");
                            }
                        }
                    } else {
                        // Es asignación normal: "asignar valor X a variable"
                        if (!is_identifier(varName)) {
                            addError("L" + to_string(lineNo) + ": identificador invalido en asignacion.");
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
                if (aPos == string::npos) {
                    addError("L" + to_string(lineNo) + ": formato invalido en 'asignar ... a ...'.");
                } else {
                    string value = raw.substr(startVal, aPos - startVal);
                    trim(value);

                    string target = raw.substr(aPos + 3);
                    trim(target);

                    string targetLower = to_lower_copy(target);
                    if (targetLower.find("en posicion") != string::npos) {
                        size_t enPos = targetLower.find(" en posicion");
                        string arrName = target.substr(0, enPos);
                        trim(arrName);
                        if (!is_identifier(arrName)) {
                            addError("L" + to_string(lineNo) + ": nombre de lista invalido en asignacion.");
                        }
                        string indexPart = target.substr(enPos + 12);
                        trim(indexPart);
                        if (indexPart.empty()) {
                            addError("L" + to_string(lineNo) + ": falta indice en asignacion a lista.");
                        } else {
                            auto isInt = [](const string& s){
                                if (s.empty()) return false;
                                size_t i = (s[0]=='+'||s[0]=='-')?1:0;
                                if (i>=s.size()) return false;
                                for (; i<s.size(); ++i) if(!isdigit((unsigned char)s[i])) return false;
                                return true;
                            };
                            if (!(isInt(indexPart) || is_identifier(indexPart))) {
                                addError("L" + to_string(lineNo) + ": indice debe ser entero o identificador.");
                            }
                        }
                    } else {
                        if (!is_identifier(target)) {
                            addError("L" + to_string(lineNo) + ": identificador invalido en asignacion.");
                        }
                    }
                }
            }
            // FOR/PARA clasico: for i desde 0 hasta 10 paso 2 O para i desde 0 hasta 10 paso 2
            else if ((lower.find("for ") == 0 || lower.find("para ") == 0) && lower.find(" desde ") != string::npos && lower.find(" hasta ") != string::npos) {
                recognized = true;
                // valida identificador
                size_t forPos = (lower.find("for ") == 0) ? 4 : 5;
                size_t desdePos = lower.find(" desde ");
                string id = raw.substr(forPos, desdePos - forPos);
                trim(id);
                if (!is_identifier(id)) {
                    addError("L" + to_string(lineNo) + ": identificador invalido en 'for/para'.");
                }
                // valida enteros
                size_t hastaPos = lower.find(" hasta ");
                if (hastaPos == string::npos || hastaPos <= desdePos) {
                    addError("L" + to_string(lineNo) + ": falta 'hasta' en for/para.");
                } else {
                    // ini y fin
                    string ini = raw.substr(desdePos + 7, hastaPos - (desdePos + 7));
                    trim(ini);
                    string rest = raw.substr(hastaPos + 7);
                    string fin = rest;
                    string paso;
                    size_t pasoPos = lower.find(" paso ");
                    if (pasoPos != string::npos && pasoPos > hastaPos) {
                        fin = raw.substr(hastaPos + 7, pasoPos - (hastaPos + 7));
                        paso = raw.substr(pasoPos + 6);
                        trim(fin); trim(paso);
                    } else { trim(fin); }
                    auto isInt = [&](const string& s){
                        if (s.empty()) return false;
                        size_t i = (s[0]=='+'||s[0]=='-')?1:0;
                        if (i>=s.size()) return false;
                        for (; i<s.size(); ++i) if(!isdigit((unsigned char)s[i])) return false;
                        return true;
                    };
                    if (!isInt(ini) || !isInt(fin) || (!paso.empty() && !isInt(paso))) {
                        addError("L" + to_string(lineNo) + ": for/para requiere enteros puros en desde/hasta/paso.");
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
                
                if (lower.find(" veces") != string::npos) {
                    // PORT N veces
                    int nums[5];
                    int c = extract_ints(raw, nums, 5);
                    if (c == 0 || nums[0] <= 0) {
                        addError("L" + to_string(lineNo) + ": PORT veces requiere numero positivo.");
                    }
                } else if (lower.find("cada ") != string::npos && lower.find(" en ") != string::npos) {
                    // PORT cada item en lista
                    size_t cadaPos = lower.find("cada ");
                    size_t enPos = lower.find(" en ");
                    if (cadaPos == string::npos || enPos == string::npos || enPos <= cadaPos) {
                        addError("L" + to_string(lineNo) + ": formato invalido en 'PORT cada ... en ...'.");
                    } else {
                        string item = raw.substr(cadaPos + 5, enPos - (cadaPos + 5));
                        trim(item);
                        string list = raw.substr(enPos + 4);
                        trim(list);
                        
                        if (!is_identifier(item)) {
                            addError("L" + to_string(lineNo) + ": identificador de item invalido en PORT.");
                        }
                        if (!is_identifier(list)) {
                            addError("L" + to_string(lineNo) + ": identificador de lista invalido en PORT.");
                        }
                    }
                } else {
                    addError("L" + to_string(lineNo) + ": formato PORT invalido. Use 'PORT N veces' o 'PORT cada item en lista'.");
                }
            }
            // COMENTARIO
            else if (lower.rfind("comentario ", 0) == 0) {
                recognized = true;
            }
            // E/S y control de flujo básicos: aceptados (validaciones mas finas luego)
            else if (lower.find("leer")!=string::npos ||
                     lower.find("ingresar")!=string::npos ||
                     lower.find("mostrar")!=string::npos ||
                     lower.find("imprimir")!=string::npos ||
                     lower.find("si ")!=string::npos ||
                     lower.find("sino")!=string::npos ||
                     lower.find("mientras")!=string::npos ||
                     lower.find("para cada")!=string::npos ||
                     lower.find("comenzar programa")!=string::npos ||
                     lower.find("terminar programa")!=string::npos ||
                     lower.find("definir funcion")!=string::npos) {
                recognized = true;
                // (Opcional) aquí puedes agregar checks por comparadores, etc.
            }

            if (!recognized) {
                addError("L" + to_string(lineNo) + ": instruccion no reconocida.");
            }
        }

        lastDSLok = (errors.getSize() == 0);
        return lastDSLok;
    }
    catch (...) {
        addError("Error interno durante la validacion de sintaxis DSL");
        lastDSLok = false;
        return false;
    }
}

bool NaturalLanguageProcessor::isValidToSave() const
{
    return (errors.getSize()==0) && !generatedCode.empty();
}

string NaturalLanguageProcessor::getGeneratedCode() const 
{
    return generatedCode;
}



int NaturalLanguageProcessor::getInstructionCount() const 
{
    return instructionCount;
}

void NaturalLanguageProcessor::parseInstructions(const string& input) 
{
    try {
        instructionCount = 0;
        istringstream iss(input);
        string line;
        
        // Helper para convertir a minúsculas
        auto to_lower_copy = [](const string& str) -> string {
            string result = str;
            transform(result.begin(), result.end(), result.begin(),
                           [](unsigned char ch){ return tolower(ch); });
            return result;
        };
        
        while (getline(iss, line)) {
            if (!line.empty() && line[0] != '#') {
                instructionCount++;
                
                // Procesar instrucciones de listas DSL (tras bambalinas)
                string lower = to_lower_copy(line);
                processDslList(line, lower);
                processDslAssignment(line, lower);
                processDslForEach(line, lower);
            }
        }
    }
    catch (...) {
        instructionCount = 0;
    }
}

void NaturalLanguageProcessor::generateCode() 
{
    try {
        generatedCode.clear();
        
        // Helper functions con manejo seguro
        auto to_lower_copy = [this](const string& str) -> string {
            try {
                string result = str;
                transform(result.begin(), result.end(), result.begin(),
                               [](unsigned char ch){ return tolower(ch); });
                return result;
            }
            catch (...) {
                addError("Error al convertir a minusculas");
                return str;
            }
        };
        
        auto extract_ints = [this](const string& str, int out[], int maxn) -> int {
            try {
                string cleaned = str;
                for (char& c : cleaned) {
                    if (!isdigit((unsigned char)c) && c != ' ' && c != '\t' && c != '-' && c != '+') {
                        c = ' ';
                    }
                }
                istringstream iss(cleaned);
                int count = 0;
                string token;
                while (count < maxn && iss >> token) {
                    int num;
                    if (tryParseInt(token, num)) {
                        out[count++] = num;
                    }
                }
                return count;
            }
            catch (...) {
                return 0;
            }
        };
        
        auto read_identifier_after = [this](const string& low_str, const string& original, const string& keyword) -> string {
            try {
                size_t pos = low_str.find(keyword);
                if (pos == string::npos) return "";
                pos += keyword.length();
                
                while (pos < original.length() && (original[pos] == ' ' || original[pos] == '\t')) {
                    pos++;
                }
                
                size_t start = pos;
                while (pos < original.length() && (isalnum((unsigned char)original[pos]) || original[pos] == '_')) {
                    pos++;
                }
                
                string result;
                if (start < pos && safeSubstr(original, start, pos - start, result)) {
                    return result;
                }
                return "";
            }
            catch (...) {
                return "";
            }
        };
        
        auto read_value_after = [this](const string& low_str, const string& original, const string& keyword) -> string {
            try {
                size_t pos = low_str.find(keyword);
                if (pos == string::npos) return "";
                pos += keyword.length();
                
                while (pos < original.length() && (original[pos] == ' ' || original[pos] == '\t')) {
                    pos++;
                }
                
                size_t start = pos;
                size_t end = original.length();
                
                string result;
                if (start < end && safeSubstr(original, start, end - start, result)) {
                    result.erase(result.find_last_not_of(" \t\r\n") + 1);
                    return result;
                }
                return "";
            }
            catch (...) {
                return "";
            }
        };
        
        //  USA NUEVA TABLA DE SÍMBOLOS
        SymbolTable symbols;
        
    
        int sumaIdx = 1;
        int restaIdx = 1;
        int productoIdx = 1;
        int divisionIdx = 1;
        int listaIdx = 1;
        int foreachIdxCounter = 0;
        
     
        struct LineInfo { int indent; string raw; string low; };
        LinkedList<LineInfo> L;
        {
            istringstream iss2(inputContent);
            string ln;
            while (getline(iss2, ln)) {
                if (ln.empty() || ln[0]=='#') continue;
                int ind = countIndent(ln);
                string raw = trim_copy(ln);
                if (raw.empty()) continue;
                string low = to_lower_copy(raw);
                LineInfo info;
                info.indent = ind;
                info.raw = raw;
                info.low = low;
                L.add(info);
            }
        }
        
        // Generate header
        generatedCode += "#include <iostream>\n";
        generatedCode += "using namespace std;\n\n";
        
        generatedCode += "int main() {\n";
        
        // Block stack - USA NUESTRO STACK
        Stack<Block> stack;
        
        // FUNCIÓN CLOSEBLOCKSTO MEJORADA CON DEFENSAS
        auto closeBlocksTo = [&](int targetIndent) {
            Block b;
            int closedBlocks = 0;
            const int MAX_CLOSE_PROTECTION = 100; // Protección contra bucles infinitos
            
            while (!stack.empty() && closedBlocks < MAX_CLOSE_PROTECTION) {
                const Block* top = stack.top();
                // DEFENSA: Si top() es nulo, salir inmediatamente
                if (!top) {
                    break;
                }
                
                // CONDICIÓN CORRECTA: Solo cerrar si el indent del bloque >= targetIndent
                if (top->indent < targetIndent) {
                    break;
                }
                
                // DEFENSA: Si pop() falla, salir del bucle
                if (!stack.pop(b)) {
                    break;
                }
                
                // Generar cierre correcto según el tipo
                if (b.type == BlockType::DoUntil) {
                    // CORRIGIDO: do-until se cierra con "} while (condición);"
                    generatedCode += "    " + string(b.indent*4, ' ') + "} while (!(" + b.extra + "));\n";
                } else {
                    // Otros bloques se cierran con "}"
                    generatedCode += "    " + string(b.indent*4, ' ') + "}\n";
                }
                
                closedBlocks++;
            }
        };
        
        for (int i = 0; i < L.getSize(); i++) {
            LineInfo info = L.get(i);
            int ind = info.indent;
            string raw = info.raw;
            string low = info.low; 

            // MANEJO CORRECTO DE ELSE/SINO: No cerrar el 'if' del mismo nivel cuando la línea es 'sino'
            bool isElseLine = (low == "sino" || low.rfind("sino", 0) == 0);
            if (isElseLine) {
                // CORREGIDO: Cierra solo bloques más profundos; conserva el 'if' en este indent
                closeBlocksTo(ind + 1);
            } else {
                // Para todas las demás líneas, cierra bloques del mismo nivel o más profundos
                closeBlocksTo(ind);
            }

            
            if (low.find("comenzar programa") != string::npos ||
                low.find("terminar programa") != string::npos ||
                low.find("definir funcion") != string::npos) {
                continue;
            }

            // COMENTARIO
            if (low.rfind("comentario ",0)==0) {
                string resto = trim_copy(raw.substr(11));
                generatedCode += "    " + string(ind*4, ' ') + "// " + resto + "\n";
                continue;
            }

            // SI
            if (low.rfind("si ",0)==0) {
                string cond = trim_copy(raw.substr(3));
                generatedCode += "    " + string(ind*4, ' ') + "if (" + toCppCondition(cond) + ") {\n";
                Block b; b.type = BlockType::If; b.indent = ind; b.extra = "";
                stack.push(b);
                continue;
            }
            
            // SINO - MANEJO MEJORADO
            if (low=="sino" || low.rfind("sino",0)==0) {
                Block b;
                if (!stack.empty() && stack.pop(b)) {
                    // VERIFICAR que el bloque anterior sea If o Else del mismo nivel
                    if ((b.type==BlockType::If || b.type==BlockType::Else) && b.indent == ind) {
                        generatedCode += "    " + string(ind*4, ' ') + "} else {\n";
                        b.type = BlockType::Else;
                        b.indent = ind; // Asegurar el indent correcto
                        stack.push(b);
                    } else {
                        // Si no coincide, agregar error pero continuar
                        addError("Bloque 'sino' sin 'si' correspondiente del mismo nivel.");
                        generatedCode += "    " + string(ind*4, ' ') + "} else {\n";
                        Block newB; newB.type = BlockType::Else; newB.indent = ind; newB.extra = "";
                        stack.push(newB);
                    }
                } else {
                    addError("Bloque 'sino' sin 'si' correspondiente.");
                    generatedCode += "    " + string(ind*4, ' ') + "else {\n";
                    Block newB; newB.type = BlockType::Else; newB.indent = ind; newB.extra = "";
                    stack.push(newB);
                }
                continue;
            }
            
            // MIENTRAS
            if (low.rfind("mientras ",0)==0) {
                string cond = trim_copy(raw.substr(9));
                generatedCode += "    " + string(ind*4, ' ') + "while (" + toCppCondition(cond) + ") {\n";
                Block b; b.type = BlockType::While; b.indent = ind; b.extra = "";
                stack.push(b);
                continue;
            }
            
            // REPETIR HASTA - CORREGIDO
            if (low.rfind("repetir hasta ",0)==0) {
                string cond = trim_copy(raw.substr(14));
                generatedCode += "    " + string(ind*4, ' ') + "do {\n";
                // IMPORTANTE: Guardamos la condición NEGADA para el cierre
                Block b; b.type = BlockType::DoUntil; b.indent = ind; b.extra = cond;
                stack.push(b);
                continue;
            }
            
            // PARA CADA
            if (low.rfind("para cada ",0)==0 && low.find(" en ")!=string::npos) {
                size_t enPos = low.find(" en ");
                string item = trim_copy(raw.substr(10, enPos-10));
                string list = trim_copy(raw.substr(enPos+4));
                // busca tamaño conocido
                int arrSize = 0;
                const Symbol* sym = symbols.find(list);
                if (sym && sym->isArray) arrSize = sym->size;
                
                string idx = "__i" + to_string(foreachIdxCounter++);
                string bound;
                if (arrSize > 0) {
                    bound = to_string(arrSize);
                } else {
                
                    bound = "(int)(sizeof(" + list + ")/sizeof(" + list + "[0]))";
                }
                generatedCode += "    " + string(ind*4, ' ') + "for (int " + idx + " = 0; " + idx + " < " + bound + "; ++" + idx + ") {\n";
                generatedCode += "    " + string((ind+1)*4, ' ') + "auto " + item + " = " + list + "[" + idx + "];\n";
                Block b; b.type = BlockType::Foreach; b.indent = ind; b.extra = "";
                stack.push(b);
                continue;
            }
            
            // FOR / PARA ... DESDE ... HASTA ... PASO ...
            if ((low.rfind("for ",0)==0 || low.rfind("para ",0)==0) && low.find(" desde ")!=string::npos && low.find(" hasta ")!=string::npos) {
                size_t desdePos = low.find(" desde ");
                size_t hastaPos = low.find(" hasta ");
                size_t startPos = (low.rfind("for ",0)==0) ? 4 : 5;

                string id = trim_copy(raw.substr(startPos, desdePos-startPos));
                string ini = trim_copy(raw.substr(desdePos+7, hastaPos-(desdePos+7)));
                string rest = raw.substr(hastaPos+7);
                string fin = rest;
                string paso;
                size_t pasoPos = low.find(" paso ");
                if (pasoPos != string::npos && pasoPos > hastaPos) {
                    fin  = trim_copy(rest.substr(0, pasoPos-(hastaPos+7)));
                    paso = trim_copy(rest.substr(pasoPos+6-(hastaPos+7)));
                } else {
                    fin = trim_copy(fin);
                }

                auto isInt = [](const string& s){
                    if (s.empty()) return false;
                    size_t i = (s[0]=='+'||s[0]=='-')?1:0;
                    if (i>=s.size()) return false;
                    for (; i<s.size(); ++i) if(!isdigit((unsigned char)s[i])) return false;
                    return true;
                };

                string forLine;
                if (!paso.empty() && isInt(paso) && paso != "0") {
                    int pasoInt = 1;
                    try { pasoInt = stoi(paso); } catch(...) { pasoInt = 1; }
                    string comp = (pasoInt > 0) ? " <= " : " >= ";
                    forLine = "for (int " + id + " = " + ini + "; " + id + comp + fin + "; " + id + " += " + to_string(pasoInt) + ") {";
                } else {
                    string stepExpr = !paso.empty() ? paso : ("((" + ini + ")<=(" + fin + ")?1:-1)");
                    forLine = "for (int " + id + " = " + ini + "; ((" + stepExpr + ")>0 ? " + id + " <= " + fin + " : " + id + " >= " + fin + "); " + id + " += " + stepExpr + ") {";
                }

                generatedCode += "    " + string(ind*4, ' ') + forLine + "\n";
                
                // Registrar la variable del bucle en la tabla de símbolos
                Symbol s; s.name = id; s.type = "int"; s.isArray = false; s.size = 0;
                symbols.add(s);
                
                Block b; b.type = BlockType::For; b.indent = ind; b.extra = "";
                stack.push(b);
                continue;
            }
            
            // CREAR VARIABLE con parsing seguro
            if (low.find("crear variable") != string::npos) {
                string name = read_identifier_after(low, raw, "llamada ");
                string value = read_value_after(low, raw, "valor ");
                
                if (low.find("entero") != string::npos) {
                    if (name.empty()) name = "numero";
                    int val = 0;
                    if (!value.empty()) {
                        if (!tryParseInt(value, val)) {
                            addError("Valor entero invalido en linea: " + raw);
                            val = 0;
                        }
                    }
                    generatedCode += "    " + string(ind*4, ' ') + "int " + name + " = " + to_string(val) + ";\n";
                    Symbol s; s.name = name; s.type = "int"; s.isArray = false; s.size = 0;
                    symbols.add(s);
                }
                else if (low.find("decimal") != string::npos) {
                    if (name.empty()) name = "promedio";
                    double val = 0.0;
                    if (!value.empty()) {
                        if (!tryParseDouble(value, val)) {
                            addError("Valor decimal invalido en linea: " + raw);
                            val = 0.0;
                        }
                    }
                    generatedCode += "    " + string(ind*4, ' ') + "double " + name + " = " + to_string(val) + ";\n";
                    Symbol s; s.name = name; s.type = "double"; s.isArray = false; s.size = 0;
                    symbols.add(s);
                }
                else if (low.find("texto") != string::npos) {
                    if (name.empty()) name = "nombre";
                    string val = "";
                    if (!value.empty()) val = value;
                    generatedCode += "    " + string(ind*4, ' ') + "string " + name + " = \"" + escapeForCxxString(val) + "\";\n";
                    Symbol s; s.name = name; s.type = "string"; s.isArray = false; s.size = 0;
                    symbols.add(s);
                }
                else if (low.find("caracter") != string::npos) {
                    if (name.empty()) name = "inicial";
                    char val = '\0';
                    if (!value.empty()) val = value[0];
                    generatedCode += "    " + string(ind*4, ' ') + "char " + name + " = '" + string(1, val) + "';\n";
                    Symbol s; s.name = name; s.type = "char"; s.isArray = false; s.size = 0;
                    symbols.add(s);
                }
                else if (low.find("booleano") != string::npos) {
                    if (name.empty()) name = "activo";
                    bool val = false;
                    if (!value.empty() && value == "true") val = true;
                    generatedCode += "    " + string(ind*4, ' ') + "bool " + name + " = " + (val ? "true" : "false") + ";\n";
                    Symbol s; s.name = name; s.type = "bool"; s.isArray = false; s.size = 0;
                    symbols.add(s);
                }
                continue;
            }
            
            // CREAR LISTA/ARREGLO - Con inicialización para evitar basura
            if (low.find("crear lista") != string::npos || low.find("crear arreglo") != string::npos) {
                string name = read_identifier_after(low, raw, "llamada ");
                int nums[5];
                int count = extract_ints(raw, nums, 5);
                int size = (count > 0) ? nums[0] : 5;
                
                if (size <= 0) {
                    addError("Tamaño de lista invalido en linea: " + raw);
                    size = 1;
                }
                
                if (low.find("enteros") != string::npos) {
                    if (name.empty()) name = "lista" + to_string(listaIdx++);
                    generatedCode += "    " + string(ind*4, ' ') + "int " + name + " [" + to_string(size) + "] = {0};\n";
                    Symbol s; s.name = name; s.type = "int"; s.isArray = true; s.size = size;
                    symbols.add(s);
                }
                else if (low.find("decimales") != string::npos) {
                    if (name.empty()) name = "promedios";
                    generatedCode += "    " + string(ind*4, ' ') + "double " + name + " [" + to_string(size) + "] = {0.0};\n";
                    Symbol s; s.name = name; s.type = "double"; s.isArray = true; s.size = size;
                    symbols.add(s);
                }
                else if (low.find("texto") != string::npos) {
                    if (name.empty()) name = "nombres";
                    generatedCode += "    " + string(ind*4, ' ') + "string " + name + " [" + to_string(size) + "];\n";
                    Symbol s; s.name = name; s.type = "string"; s.isArray = true; s.size = size;
                    symbols.add(s);
                }
                else if (low.find("caracteres") != string::npos) {
                    if (name.empty()) name = "iniciales";
                    generatedCode += "    " + string(ind*4, ' ') + "char " + name + " [" + to_string(size) + "] = {0};\n";
                    Symbol s; s.name = name; s.type = "char"; s.isArray = true; s.size = size;
                    symbols.add(s);
                }
                continue;
            }
            
            // SUMAR silencioso: "sumar var y 1 y 2"
            if (low.rfind("sumar ",0)==0 && low.find("mostrar resultado")==string::npos) {
                string tail = trim_copy(raw.substr(6));
                istringstream ts(tail);
                LinkedList<string> terms; string tk;
                while (ts >> tk) if (to_lower_copy(tk)!="y") terms.add(tk);
                if (terms.getSize() >= 2) {
                    string lhs = terms.get(0); string rhs;
                    for (int j=1;j<terms.getSize();++j){ if(j>1) rhs+=" + "; rhs+=terms.get(j); }
                    generatedCode += "    " + string(ind*4, ' ') + lhs + " += (" + rhs + ");\n";
                } else { addError("Operacion 'sumar' requiere al menos 2 terminos: " + raw); }
                continue;
            }

            // RESTAR silencioso
            if (low.rfind("restar ",0)==0 && low.find("mostrar resultado")==string::npos) {
                string tail = trim_copy(raw.substr(7));
                istringstream ts(tail);
                LinkedList<string> terms; string tk;
                while (ts >> tk) if (to_lower_copy(tk)!="y") terms.add(tk);
                if (terms.getSize() >= 2) {
                    string lhs = terms.get(0); string rhs;
                    for (int j=1;j<terms.getSize();++j){ if(j>1) rhs+=" + "; rhs+=terms.get(j); }
                    generatedCode += "    " + string(ind*4, ' ') + lhs + " -= (" + rhs + ");\n";
                } else { addError("Operacion 'restar' requiere al menos 2 terminos: " + raw); }
                continue;
            }

            // MULTIPLICAR silencioso
            if (low.rfind("multiplicar ",0)==0 && low.find("mostrar resultado")==string::npos) {
                string tail = trim_copy(raw.substr(12));
                istringstream ts(tail);
                LinkedList<string> terms; string tk;
                while (ts >> tk) if (to_lower_copy(tk)!="y") terms.add(tk);
                if (terms.getSize() >= 2) {
                    string lhs = terms.get(0); string rhs;
                    for (int j=1;j<terms.getSize();++j){ if(j>1) rhs+=" * "; rhs+=terms.get(j); }
                    generatedCode += "    " + string(ind*4, ' ') + lhs + " *= (" + rhs + ");\n";
                } else { addError("Operacion 'multiplicar' requiere al menos 2 terminos: " + raw); }
                continue;
            }

            // DIVIDIR silencioso
            if (low.rfind("dividir ",0)==0 && low.find("mostrar resultado")==string::npos) {
                string tail = trim_copy(raw.substr(8));
                istringstream ts(tail);
                LinkedList<string> terms; string tk;
                while (ts >> tk) if (to_lower_copy(tk)!="y") terms.add(tk);
                if (terms.getSize() >= 2) {
                    string lhs = terms.get(0);
                    for (int j=1;j<terms.getSize();++j) {
                        generatedCode += "    " + string(ind*4, ' ') + lhs + " /= " + terms.get(j) + ";\n";
                    }
                } else { addError("Operacion 'dividir' requiere al menos 2 terminos: " + raw); }
                continue;
            }
            
            // OPERACIONES ARITMETICAS - Solo cuando DSL lo pide explícitamente y con mostrar resultado
            if (low.find("sumar") != string::npos && low.find("mostrar resultado") != string::npos) {
                int nums[10];
                int count = extract_ints(raw, nums, 10);
                
                if (count >= 2) {
                    string varName = "suma" + to_string(sumaIdx++);
                    generatedCode += "    " + string(ind*4, ' ') + "int " + varName + " = ";
                    for (int j = 0; j < count; j++) {
                        if (j > 0) generatedCode += " + ";
                        generatedCode += to_string(nums[j]);
                    }
                    generatedCode += ";\n";
                    generatedCode += "    " + string(ind*4, ' ') + "cout << \"El resultado es: \" << " + varName + " << endl;\n";
                    Symbol s; s.name = varName; s.type = "int"; s.isArray = false; s.size = 0;
                    symbols.add(s);
                } else {
                    addError("Operacion suma requiere al menos 2 numeros en linea: " + raw);
                }
                continue;
            }
            
            // NUEVO: Operación RESTAR con mostrar resultado
            if (low.find("restar") != string::npos && low.find("mostrar resultado") != string::npos) {
                int nums[10];
                int count = extract_ints(raw, nums, 10);
                
                if (count >= 2) {
                    string varName = "resta" + to_string(restaIdx++);
                    generatedCode += "    " + string(ind*4, ' ') + "int " + varName + " = " + to_string(nums[0]) + ";\n";
                    for (int j = 1; j < count; j++) {
                        generatedCode += "    " + string(ind*4, ' ') + varName + " -= " + to_string(nums[j]) + ";\n";
                    }
                    generatedCode += "    " + string(ind*4, ' ') + "cout << \"El resultado es: \" << " + varName + " << endl;\n";
                    Symbol s; s.name = varName; s.type = "int"; s.isArray = false; s.size = 0;
                    symbols.add(s);
                } else {
                    addError("Operacion resta requiere al menos 2 numeros en linea: " + raw);
                }
                continue;
            }
            
            // NUEVO: Operación MULTIPLICAR con mostrar resultado
            if (low.find("multiplicar") != string::npos && low.find("mostrar resultado") != string::npos) {
                int nums[10];
                int count = extract_ints(raw, nums, 10);
                
                if (count >= 2) {
                    string varName = "producto" + to_string(productoIdx++);
                    generatedCode += "    " + string(ind*4, ' ') + "int " + varName + " = 1;\n";
                    for (int j = 0; j < count; j++) {
                        generatedCode += "    " + string(ind*4, ' ') + varName + " *= " + to_string(nums[j]) + ";\n";
                    }
                    generatedCode += "    " + string(ind*4, ' ') + "cout << \"El resultado es: \" << " + varName + " << endl;\n";
                    Symbol s; s.name = varName; s.type = "int"; s.isArray = false; s.size = 0;
                    symbols.add(s);
                } else {
                    addError("Operacion multiplicar requiere al menos 2 numeros en linea: " + raw);
                }
                continue;
            }
            
            // NUEVO: Operación DIVIDIR con mostrar resultado (con protección división por cero)
            if (low.find("dividir") != string::npos && low.find("mostrar resultado") != string::npos) {
                int nums[10];
                int count = extract_ints(raw, nums, 10);
                
                if (count >= 2) {
                    string varName = "division" + to_string(divisionIdx++);
                    generatedCode += "    " + string(ind*4, ' ') + "int " + varName + " = " + to_string(nums[0]) + ";\n";
                    generatedCode += "    " + string(ind*4, ' ') + "bool divisionValida = true;\n";
                    for (int j = 1; j < count; j++) {
                        if (nums[j] == 0) {
                            // División por cero detectada en tiempo de compilación
                            generatedCode += "    " + string(ind*4, ' ') + "// ADVERTENCIA: Division por cero detectada\n";
                            generatedCode += "    " + string(ind*4, ' ') + "divisionValida = false;\n";
                            generatedCode += "    " + string(ind*4, ' ') + "cout << \"Error: division por cero\" << endl;\n";
                        } else {
                            generatedCode += "    " + string(ind*4, ' ') + "if (divisionValida) " + varName + " /= " + to_string(nums[j]) + ";\n";
                        }
                    }
                    generatedCode += "    " + string(ind*4, ' ') + "if (divisionValida) cout << \"El resultado es: \" << " + varName + " << endl;\n";
                    Symbol s; s.name = varName; s.type = "int"; s.isArray = false; s.size = 0;
                    symbols.add(s);
                } else {
                    addError("Operacion dividir requiere al menos 2 numeros en linea: " + raw);
                }
                continue;
            }
            
            // E/S
            if (low.find("leer") != string::npos || low.find("ingresar") != string::npos) {
                // Buscar qué variable se está intentando leer
                istringstream issTokens(raw);
                string token;
                bool found = false;
                while (issTokens >> token && !found) {
                    const Symbol* sym = symbols.find(token);
                    if (sym && !sym->isArray) {
                        if (sym->type == "string") {
                            generatedCode += "    " + string(ind*4, ' ') + "cin >> ws;\n";
                            generatedCode += "    " + string(ind*4, ' ') + "getline(cin, " + sym->name + ");\n";
                        } else {
                            generatedCode += "    " + string(ind*4, ' ') + "cin >> " + sym->name + ";\n";
                        }
                        found = true;
                    }
                }
                continue;
            }
            
            if (low.find("mostrar") != string::npos || low.find("imprimir") != string::npos) {
                bool foundVar = false;
                // Simplificación: buscar tokens en la línea que coincidan con variables conocidas
                istringstream issTokens(raw);
                string token;
                while (issTokens >> token && !foundVar) {
                    const Symbol* sym = symbols.find(token);
                    if (sym && !sym->isArray) {
                        generatedCode += "    " + string(ind*4, ' ') + "cout << " + sym->name + " << endl;\n";
                        foundVar = true;
                    }
                }
                
                if (!foundVar) {
                    size_t pos = low.find("mostrar");
                    if (pos == string::npos) pos = low.find("imprimir");
                    pos += (low.find("mostrar") != string::npos) ? 7 : 8;
                    
                    string message;
                    if (safeSubstr(raw, pos, raw.length() - pos, message)) {
                        message.erase(0, message.find_first_not_of(" \t"));
                        message.erase(message.find_last_not_of(" \t") + 1);
                        
                        // Remove leading colon if present
                        if (!message.empty() && message[0] == ':') {
                            string temp;
                            if (safeSubstr(message, 1, message.length() - 1, temp)) {
                                message = temp;
                                message.erase(0, message.find_first_not_of(" \t"));
                            }
                        }
                        
                        if (!message.empty()) {
                            generatedCode += "    " + string(ind*4, ' ') + "cout << \"" + escapeForCxxString(message) + "\" << endl;\n";
                        }
                    }
                }
                continue;
            }

            // ASIGNACIONES con manejo seguro
            if (low.find("asignar valor") != string::npos) {
                size_t valorPos = low.find("valor") + 5;
                size_t aPos = low.find(" a ");
                if (valorPos < aPos) {
                    string value, varName;
                    if (safeSubstr(raw, valorPos, aPos - valorPos, value) &&
                        safeSubstr(raw, aPos + 3, raw.length() - (aPos + 3), varName)) {
                        
                        value.erase(0, value.find_first_not_of(" \t"));
                        value.erase(value.find_last_not_of(" \t") + 1);
                        
                        varName.erase(0, varName.find_first_not_of(" \t"));
                        varName.erase(varName.find_last_not_of(" \t") + 1);
                        
                        // Check if it's an array assignment
                        if (varName.find("en posicion") != string::npos) {
                            size_t enPos = varName.find(" en posicion");
                            string arrName = varName.substr(0, enPos);
                            string indexStr = varName.substr(enPos + 13);
                            // trims
                            arrName.erase(arrName.find_last_not_of(" \t") + 1);
                            indexStr.erase(0, indexStr.find_first_not_of(" \t"));

                            generatedCode += "    " + string(ind*4, ' ') + arrName + "[" + indexStr + "] = " + value + ";\n";
                        } else {
                            generatedCode += "    " + string(ind*4, ' ') + varName + " = " + value + ";\n";
                        }
                    } else {
                        addError("Error de sintaxis en asignacion: " + raw);
                    }
                } else {
                    addError("Error de sintaxis en asignacion (falta ' a '): " + raw);
                }
                continue;
            }
            
            // Asignar (acepta con o sin la palabra "valor")
            if (low.rfind("asignar ", 0) == 0) {
                size_t startVal = 8; // tras "asignar "
                string lowRaw = to_lower_copy(raw);
                if (lowRaw.find("valor ", startVal) == startVal) startVal += 6;

                size_t aPos = lowRaw.find(" a ", startVal);
                if (aPos != string::npos) {
                    string value, varName;
                    if (safeSubstr(raw, startVal, aPos - startVal, value) &&
                        safeSubstr(raw, aPos + 3, raw.length() - (aPos + 3), varName)) {

                        // trims
                        value.erase(0, value.find_first_not_of(" \t"));
                        value.erase(value.find_last_not_of(" \t") + 1);
                        varName.erase(0, varName.find_first_not_of(" \t"));
                        varName.erase(varName.find_last_not_of(" \t") + 1);

                        if (varName.find(" en posicion ") != string::npos) {
                            size_t enPos = varName.find(" en posicion ");
                            string arrName = varName.substr(0, enPos);
                            string indexStr = varName.substr(enPos + 13);
                            // trims
                            arrName.erase(arrName.find_last_not_of(" \t") + 1);
                            indexStr.erase(0, indexStr.find_first_not_of(" \t"));

                            generatedCode += "    " + string(ind*4, ' ') + arrName + "[" + indexStr + "] = " + value + ";\n";
                        } else {
                            generatedCode += "    " + string(ind*4, ' ') + varName + " = " + value + ";\n";
                        }
                    } else {
                        addError("Error de sintaxis en asignacion: " + raw);
                    }
                } else {
                    addError("Error de sintaxis en asignacion (falta ' a '): " + raw);
                }
                continue;
            }
        }
        
        // CLOSE ALL REMAINING BLOCKS - closeBlocksTo(0) CON PROTECCIÓN
        closeBlocksTo(0);
        
        // VERIFICACIÓN FINAL: Si la pila no está vacía, hay un problema
        if (!stack.empty()) {
            addError("ADVERTENCIA: Pila de bloques no vacia al final - puede haber llaves no balanceadas");
            // Forzar cierre de todos los bloques restantes
            Block b;
            while (!stack.empty() && stack.pop(b)) {
                generatedCode += "    " + string(b.indent*4, ' ') + "}\n";
            }
        }
        
        generatedCode += "    return 0;\n";
        generatedCode += "}\n";
    }
    catch (...) {
        addError("Error interno durante la generacion de codigo.");
    }
}

void NaturalLanguageProcessor::validateCode() 
{
    try {
        // Implementación básica de validación
        if (generatedCode.empty()) {
            addError("El codigo generado esta vacio");
            return;
        }
        
        // Validar balanceado de llaves
        int braceCount = 0;
        for (char c : generatedCode) {
            if (c == '{') braceCount++;
            if (c == '}') braceCount--;
        }
        
        if (braceCount != 0) {
            addError("Llaves no balanceadas en el codigo generado");
        }
    }
    catch (...) {
        addError("Error interno durante la validacion de codigo");
    }
}

const LinkedList<string>& NaturalLanguageProcessor::getErrors() const
{
    return errors;
}

void NaturalLanguageProcessor::clearErrors()
{
    try {
        errors.clear();
    }
    catch (...) {
        // Si no puede limpiar errores, al menos no crash
    }
}

void NaturalLanguageProcessor::addError(const string& error)
{
    try {
        errors.add(error);
    }
    catch (...) {
        // Si no puede agregar error, al menos no crash
    }
}

void NaturalLanguageProcessor::reset()
{
    try {
        inputContent.clear();
        generatedCode.clear();
        errors.clear();
        instructionCount = 0;
        lastDSLok = false;
        lastCppOk = false;
        lists.reset();
    }
    catch (...) {
        // Reset seguro - no crash
    }
}

// Helpers seguros para parsing
bool NaturalLanguageProcessor::tryParseInt(const string& str, int& result) noexcept
{
    try {
        if (str.empty()) return false;
        size_t pos;
        result = stoi(str, &pos);
        return pos == str.length(); // Toda la cadena debe ser válida
    }
    catch (...) {
        return false;
    }
}

bool NaturalLanguageProcessor::tryParseDouble(const string& str, double& result) noexcept
{
    try {
        if (str.empty()) return false;
        size_t pos;
        result = stod(str, &pos);
        return pos == str.length(); // Toda la cadena debe ser válida
    }
    catch (...) {
        return false;
    }
}

bool NaturalLanguageProcessor::safeSubstr(const string& str, size_t start, size_t len, string& result) noexcept
{
    try {
        if (start >= str.length()) return false;
        size_t maxLen = str.length() - start;
        if (len > maxLen) len = maxLen;
        result = str.substr(start, len);
        return true;
    }
    catch (...) {
        return false;
    }
}

// Métodos auxiliares para procesamiento de listas DSL (no intrusivos)
void NaturalLanguageProcessor::processDslList(const string& line, const string& lower)
{
    try {
        // Detectar: crear lista de <tipo> con <N> elementos llamada <nombre>
        if (lower.find("crear lista") != string::npos || lower.find("crear arreglo") != string::npos) {
            
            // Extraer número de elementos de forma segura
            auto extract_ints_safe = [this](const string& str, int out[], int maxn) -> int {
                try {
                    string cleaned = str;
                    for (char& c : cleaned) {
                        if (!isdigit((unsigned char)c) && c != ' ' && c != '\t') {
                            c = ' ';
                        }
                    }
                    istringstream iss(cleaned);
                    int count = 0;
                    string token;
                    while (count < maxn && iss >> token) {
                        int num;
                        if (tryParseInt(token, num)) {
                            out[count++] = num;
                        }
                    }
                    return count;
                }
                catch (...) {
                    return 0;
                }
            };
            
            auto read_identifier_after_safe = [this](const string& low_str, const string& original, const string& keyword) -> string {
                try {
                    size_t pos = low_str.find(keyword);
                    if (pos == string::npos) return "";
                    pos += keyword.length();
                    
                    while (pos < original.length() && (original[pos] == ' ' || original[pos] == '\t')) {
                        pos++;
                    }
                    
                    size_t start = pos;
                    while (pos < original.length() && (isalnum((unsigned char)original[pos]) || original[pos] == '_')) {
                        pos++;
                    }
                    
                    string result;
                    if (start < pos && safeSubstr(original, start, pos - start, result)) {
                        return result;
                    }
                    return "";
                }
                catch (...) {
                    return "";
                }
            };
            
            int nums[5];
            int c = extract_ints_safe(line, nums, 5);
            int size = (c > 0) ? nums[0] : 5;
            
            string name = read_identifier_after_safe(lower, line, "llamada ");
            if (name.empty()) name = "lista_auto";
            
            DslType dslType = DslType::Entero; // default
            if (lower.find("enteros") != string::npos) {
                dslType = DslType::Entero;
            } else if (lower.find("decimales") != string::npos) {
                dslType = DslType::Decimal;
            } else if (lower.find("texto") != string::npos) {
                dslType = DslType::Texto;
            } else if (lower.find("caracteres") != string::npos) {
                dslType = DslType::Caracter;
            }
            
            // Crear lista en capa DSL (tras bambalinas)
            lists.createList(dslType, name, size);
        }
    } catch (...) {
        // Error silencioso - no romper el flujo principal
    }
}

void NaturalLanguageProcessor::processDslForEach(const string& line, const string& lower)
{
    try {
        // Detectar: para cada <item> en <lista>
        if (lower.find("para cada") != string::npos && lower.find(" en ") != string::npos) {
            
            size_t paraPos = lower.find("para cada");
            size_t enPos = lower.find(" en ");
            if (paraPos == string::npos || enPos == string::npos || enPos <= paraPos) {
                return;
            }
            
            // Extraer nombre de la lista de forma segura
            string listName;
            if (safeSubstr(line, enPos + 4, line.length() - (enPos + 4), listName)) {
                listName.erase(0, listName.find_first_not_of(" \t"));
                listName.erase(listName.find_last_not_of(" \t") + 1);
                
                // Si la lista existe, iterar para efectos internos
                if (lists.exists(listName)) {
                    lists.forEach(listName, [](const Value& v) {
                        // Log interno opcional - no cambiar salida C++
                    });
                }
            }
 }
    } catch (...) {
        // Error silencioso - no romper el flujo principal
    }
}

void NaturalLanguageProcessor::processDslAssignment(const string& line, const string& lower)
{
    try {
        // Detectar: asignar <valor/variable> a <lista> en posicion <i>
        if (lower.find("asignar") != string::npos && lower.find(" a ") != string::npos && lower.find("posicion") != string::npos) {
            
            // Extraer valor de forma segura
            size_t asignarPos = lower.find("asignar");
            size_t aPos = lower.find(" a ");
            if (asignarPos == string::npos || aPos == string::npos || aPos <= asignarPos) {
                return;
            }
            
            string valueStr;
            if (safeSubstr(line, asignarPos + 7, aPos - (asignarPos + 7), valueStr)) {
                valueStr.erase(0, valueStr.find_first_not_of(" \t"));
                valueStr.erase(valueStr.find_last_not_of(" \t") + 1);
                
                // Si empieza con "valor", quitarlo
                if (valueStr.find("valor") == 0) {
                    string temp;
                    if (safeSubstr(valueStr, 5, valueStr.length() - 5, temp)) {
                        valueStr = temp;
                        valueStr.erase(0, valueStr.find_first_not_of(" \t"));
                    }
                }
                
                // Extraer nombre de la lista e índice de forma segura
                string listAndPos;
                if (safeSubstr(line, aPos + 3, line.length() - (aPos + 3), listAndPos)) {
                    size_t posicionPos = listAndPos.find("posicion");
                    if (posicionPos == string::npos) return;
                    
                    string listName;
                    if (safeSubstr(listAndPos, 0, posicionPos, listName)) {
                        listName.erase(listName.find_last_not_of(" \t") + 1);
                        if (listName.find(" en") != string::npos) {
                            size_t enPos = listName.find(" en");
                            string temp;
                            if (safeSubstr(listName, 0, enPos, temp)) {
                                listName = temp;
                            }
                        }
                    }
                    
                    string indexStr;
                    if (safeSubstr(listAndPos, posicionPos + 8, listAndPos.length() - (posicionPos + 8), indexStr)) {
                        indexStr.erase(0, indexStr.find_first_not_of(" \t"));
                        int index = 0;
                        if (tryParseInt(indexStr, index)) {
                            // Si la lista existe, intentar asignar
                            if (lists.exists(listName)) {
                                DslType listType = lists.typeOf(listName);
                                Value val(listType); // valor por defecto
                                 
                                // Intentar convertir el valor según el tipo de forma segura
                                try {
                                    switch (listType) {
                                        case DslType::Entero: {
                                            int intVal;
                                            if (tryParseInt(valueStr, intVal)) {
                                                val = Value(intVal);
                                            }
                                            break;
                                        }
                                        case DslType::Decimal: {
                                            double doubleVal;
                                            if (tryParseDouble(valueStr, doubleVal)) {
                                                val = Value(doubleVal);
                                            }
                                            break;
                                        }
                                        case DslType::Texto:
                                            val = Value(valueStr);
                                            break;
                                        case DslType::Caracter:
                                            val = Value(valueStr.empty() ? '\0' : valueStr[0]);
                                            break;
                                        case DslType::Booleano:
                                            val = Value(valueStr == "true" || valueStr == "1");
                                            break;
                                    }
                                } catch (...) {
                                    // Usar valor por defecto si hay error de conversión
                                }
                                
                                // Asignar en la lista DSL (tras bambalinas)
                                lists.setAt(listName, index, val);
                            }
                        }
                    }
                }
            }
        }
    } catch (...) {
        // Error silencioso - no romper el flujo principal
    }
}