#include "stdafx.h"
#include "NaturalLanguageProcessor.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>

// ==================== HELPER FUNCTIONS FOR DSL BLOCK HANDLING ====================

static int countIndent(const std::string& s) {
    int spaces = 0;
    for (char c : s) {
        if (c == ' ') spaces++;
        else if (c == '\t') spaces += 4;
        else break;
    }
    return spaces / 4;
}

static std::string trim_copy(std::string s) {
    auto issp = [](unsigned char ch) {return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; };
    while (!s.empty() && issp((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && issp((unsigned char)s.back()))  s.pop_back();
    return s;
}

static std::string lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {return (char)std::tolower(c); });
    return s;
}

// Escapar string para C++
static std::string escapeForCxxString(const std::string& s) {
    std::string result;
    result.reserve(s.size() + 10);
    for (char c : s) {
        if (c == '\\') result += "\\\\";
        else if (c == '"') result += "\\\"";
        else result += c;
    }
    return result;
}

// Reemplazo seguro de subcadenas (palabras completas)
static void replace_word(std::string& s, const std::string& from, const std::string& to) {
    std::string lower = lower_copy(s);
    for (size_t pos = 0;;) {
        pos = lower.find(from, pos);
        if (pos == std::string::npos) break;
        s.replace(pos, from.size(), to);
        lower.replace(pos, from.size(), to);
        pos += to.size();
    }
}

static std::string toCppCondition(std::string cond) {
    // normaliza espacios y minúsculas, pero mantenemos nombres/valores
    std::string w = " " + cond + " ";
    replace_word(w, " mayor o igual que ", " >= ");
    replace_word(w, " menor o igual que ", " <= ");
    replace_word(w, " distinto de ", " != ");
    replace_word(w, " diferente de ", " != ");
    replace_word(w, " igual a ", " == ");
    replace_word(w, " mayor que ", " > ");
    replace_word(w, " menor que ", " < ");
    replace_word(w, " y ", " && ");
    replace_word(w, " o ", " || ");
    // ' no X' ? '!X'
    for (size_t pos = w.find(" no "); pos != std::string::npos; pos = w.find(" no ", pos + 1)) {
        w.replace(pos, 4, " !");
    }
    // compacta espacios
    std::string out; out.reserve(w.size());
    bool prevSpace = false;
    for (char c : w) { bool sp = (c == ' ' || c == '\t'); if (!(sp && prevSpace)) out.push_back(sp ? ' ' : c); prevSpace = sp; }
    if (!out.empty() && out.front() == ' ') out.erase(out.begin());
    if (!out.empty() && out.back() == ' ')  out.pop_back();
    return out;
}

enum class BlockType { If, Else, While, DoUntil, For, Foreach };
struct Block { BlockType type; int indent; std::string extra; };

// ==================== END HELPER FUNCTIONS ====================

NaturalLanguageProcessor::NaturalLanguageProcessor()
    : instructionCount(0), lastDSLok(false), lastCppOk(false)
{
}

NaturalLanguageProcessor::~NaturalLanguageProcessor()
{
}

bool NaturalLanguageProcessor::loadFile(const std::string& filePath)
{
    try {
        clearErrors();
        generatedCode.clear();
        lists.reset();

        std::ifstream file(filePath);
        if (!file.is_open()) {
            addError("No se pudo abrir el archivo: " + filePath);
            return false;
        }

        inputContent.clear();
        std::string line;
        while (std::getline(file, line)) {
            inputContent += line + "\n";
        }
        file.close();

        if (inputContent.empty()) {
            addError("El archivo esta vacio");
            return false;
        }

        try { parseInstructions(inputContent); }
        catch (const std::exception& e) {
            addError("Error al procesar instrucciones: " + std::string(e.what()));
            return false;
        }
        catch (...) {
            addError("Error desconocido al procesar instrucciones");
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        addError("Error al cargar archivo: " + std::string(e.what()));
        return false;
    }
    catch (...) {
        addError("Error desconocido al cargar archivo");
        return false;
    }
}

bool NaturalLanguageProcessor::saveToFile(const std::string& filePath)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        addError("No se pudo crear el archivo: " + filePath);
        return false;
    }
    file << generatedCode;
    file.close();
    return true;
}

std::string NaturalLanguageProcessor::getInputContent() const { return inputContent; }
void NaturalLanguageProcessor::setInputContent(const std::string& content) { inputContent = content; }

bool NaturalLanguageProcessor::validateDSLOnly(const std::string& content)
{
    std::string oldContent = inputContent;
    inputContent = content;
    errors.clear();
    bool result = validateInputSyntax();
    inputContent = oldContent;
    return result;
}

bool NaturalLanguageProcessor::processText(const std::string& input)
{
    try {
        if (input.empty()) { addError("El texto de entrada esta vacio"); return false; }

        clearErrors();
        generatedCode.clear();
        lists.reset();
        inputContent = input;

        try {
            if (!validateInputSyntax()) { lastDSLok = false; return false; }
            lastDSLok = true;

            try { parseInstructions(input); }
            catch (const std::exception& e) { addError("Error al parsear instrucciones: " + std::string(e.what())); lastDSLok = false; return false; }
            catch (...) { addError("Error desconocido al parsear instrucciones"); lastDSLok = false; return false; }

            try { generateCode(); }
            catch (const std::exception& e) { addError("Error al generar codigo: " + std::string(e.what())); return false; }
            catch (...) { addError("Error desconocido al generar codigo"); return false; }

            try { validateCode(); }
            catch (const std::exception& e) { addError("Error al validar codigo: " + std::string(e.what())); return false; }
            catch (...) { addError("Error desconocido al validar codigo"); return false; }

            return errors.getSize() == 0;
        }
        catch (const std::exception& e) { addError("Error durante validacion DSL: " + std::string(e.what())); lastDSLok = false; return false; }
        catch (...) { addError("Error desconocido durante validacion DSL"); lastDSLok = false; return false; }
    }
    catch (const std::exception& e) { addError("Error interno durante el procesamiento: " + std::string(e.what())); lastDSLok = false; return false; }
    catch (...) { addError("Error interno desconocido durante el procesamiento"); lastDSLok = false; return false; }
}

void NaturalLanguageProcessor::processInstructions()
{
    try {
        clearErrors();
        lastDSLok = validateInputSyntax();
        if (!lastDSLok) { generatedCode.clear(); return; }
        generateCode();
        lastCppOk = validateSyntax(generatedCode);
    }
    catch (...) {
        addError("Error interno durante el procesamiento de instrucciones");
        lastDSLok = false; lastCppOk = false; generatedCode.clear();
    }
}

std::string NaturalLanguageProcessor::convertToCode(const std::string& naturalLanguage)
{
    std::string code;
    if (naturalLanguage.find("crear variable") != std::string::npos) code += "int variable;\n";
    if (naturalLanguage.find("mostrar") != std::string::npos) code += "std::cout << \"Hola Mundo\" << std::endl;\n";
    generatedCode = code; return code;
}

bool NaturalLanguageProcessor::validateSyntax() { return validateSyntax(generatedCode); }

bool NaturalLanguageProcessor::validateSyntax(const std::string& code)
{
    try {
        if (code.empty()) { addError("El codigo esta vacio"); lastCppOk = false; return false; }

        int brace = 0, paren = 0; bool inString = false, inChar = false, escape = false;
        std::istringstream iss(code); std::string line;
        while (std::getline(iss, line)) {
            for (size_t i = 0; i < line.size(); ++i) {
                char c = line[i];
                if (escape) { escape = false; continue; }
                if (inString) { if (c == '\\') escape = true; else if (c == '"') inString = false; continue; }
                if (inChar) { if (c == '\\') escape = true; else if (c == '\'') inChar = false; continue; }
                if (c == '"') { inString = true; continue; }
                if (c == '\'') { inChar = true; continue; }
                if (c == '{') ++brace; else if (c == '}') --brace;
                else if (c == '(') ++paren; else if (c == ')') --paren;
            }
        }
        if (brace != 0) addError("Llaves no balanceadas");
        if (paren != 0) addError("Parentesis no balanceados");
        if (inString) addError("Comillas dobles no cerradas");
        if (inChar) addError("Comillas simples no cerradas");
        lastCppOk = (errors.getSize() == 0);
        return lastCppOk;
    }
    catch (...) { addError("Error interno durante la validacion de sintaxis C++"); lastCppOk = false; return false; }
}

bool NaturalLanguageProcessor::validateInputSyntax()
{
    try {
        if (inputContent.empty()) { addError("El archivo esta vacio"); lastDSLok = false; return false; }

        auto to_lower_copy = [](const std::string& s) {
            std::string r = s;
            std::transform(r.begin(), r.end(), r.begin(), [](unsigned char ch) { return std::tolower(ch); });
            return r;
            };
        auto trim = [](std::string& s) {
            size_t a = s.find_first_not_of(" \t\r\n");
            size_t b = s.find_last_not_of(" \t\r\n");
            if (a == std::string::npos) { s.clear(); return; }
            s = s.substr(a, b - a + 1);
            };
        auto is_identifier = [](const std::string& s) {
            if (s.empty()) return false;
            if (!(std::isalpha((unsigned char)s[0]) || s[0] == '_')) return false;
            for (char c : s) if (!(std::isalnum((unsigned char)c) || c == '_')) return false;
            return true;
            };
        auto read_identifier_after = [](const std::string& lower, const std::string& original, const std::string& keyword) {
            size_t pos = lower.find(keyword);
            if (pos == std::string::npos) return std::string();
            pos += keyword.size();
            while (pos < original.size() && (original[pos] == ' ' || original[pos] == '\t')) pos++;
            size_t start = pos;
            while (pos < original.size() && (std::isalnum((unsigned char)original[pos]) || original[pos] == '_')) pos++;
            if (pos > start) return original.substr(start, pos - start);
            return std::string();
            };
        auto read_value_after = [](const std::string& lower, const std::string& original, const std::string& keyword) {
            size_t pos = lower.find(keyword);
            if (pos == std::string::npos) return std::string();
            pos += keyword.size();
            while (pos < original.size() && (original[pos] == ' ' || original[pos] == '\t')) pos++;
            std::string value = original.substr(pos);
            size_t last = value.find_last_not_of(" \t\r\n");
            if (last != std::string::npos) value.erase(last + 1);
            return value;
            };
        auto extract_ints = [](const std::string& str, int out[], int maxn) {
            std::string cleaned = str;
            for (char& c : cleaned) if (!std::isdigit((unsigned char)c) && c != ' ' && c != '\t' && c != '-' && c != '+') c = ' ';
            std::istringstream iss(cleaned);
            int count = 0, num;
            while (count < maxn && (iss >> num)) out[count++] = num;
            return count;
            };

        std::istringstream iss(inputContent);
        std::string line; int lineNo = 0;
        while (std::getline(iss, line)) {
            lineNo++;
            std::string raw = line;
            trim(raw);
            if (raw.empty() || raw[0] == '#') continue;

            std::string lower = to_lower_copy(raw);
            bool recognized = false;

            // Comentario
            if (lower.rfind("comentario ", 0) == 0) { recognized = true; }

            // CREAR VARIABLE ...
            else if (lower.find("crear variable") != std::string::npos) {
                recognized = true;
                bool hasType = (lower.find("entero") != std::string::npos ||
                    lower.find("decimal") != std::string::npos ||
                    lower.find("texto") != std::string::npos ||
                    lower.find("caracter") != std::string::npos ||
                    lower.find("booleano") != std::string::npos);
                if (!hasType) addError("L" + std::to_string(lineNo) + ": falta el tipo en 'crear variable'.");

                std::string name = read_identifier_after(lower, raw, "llamada ");
                if (!is_identifier(name)) addError("L" + std::to_string(lineNo) + ": identificador invalido o ausente en 'llamada'.");

                if (lower.find("entero") != std::string::npos) {
                    std::string v = read_value_after(lower, raw, "valor ");
                    if (!v.empty()) { try { (void)std::stoi(v); } catch (...) { addError("L" + std::to_string(lineNo) + ": valor entero invalido."); } }
                }
                else if (lower.find("decimal") != std::string::npos) {
                    std::string v = read_value_after(lower, raw, "valor ");
                    if (!v.empty()) { try { (void)std::stod(v); } catch (...) { addError("L" + std::to_string(lineNo) + ": valor decimal invalido."); } }
                }
                else if (lower.find("booleano") != std::string::npos) {
                    std::string v = to_lower_copy(read_value_after(lower, raw, "valor "));
                    if (!v.empty() && !(v == "true" || v == "false")) addError("L" + std::to_string(lineNo) + ": booleano debe ser true/false.");
                }
                else if (lower.find("caracter") != std::string::npos) {
                    std::string v = read_value_after(lower, raw, "valor ");
                    if (!v.empty() && v.size() != 1) addError("L" + std::to_string(lineNo) + ": caracter debe ser un unico simbolo.");
                }
            }
            // CREAR LISTA/ARREGLO ...
            else if (lower.find("crear lista") != std::string::npos || lower.find("crear arreglo") != std::string::npos) {
                recognized = true;
                int nums[5]; int c = extract_ints(raw, nums, 5);
                if (c == 0 || nums[0] <= 0) addError("L" + std::to_string(lineNo) + ": tamano de lista/arreglo invalido o ausente.");
                std::string name = read_identifier_after(lower, raw, "llamada ");
                if (!name.empty() && !is_identifier(name)) addError("L" + std::to_string(lineNo) + ": identificador de lista invalido.");
                bool typed = (lower.find("enteros") != std::string::npos ||
                    lower.find("decimales") != std::string::npos ||
                    lower.find("texto") != std::string::npos ||
                    lower.find("caracteres") != std::string::npos);
                if (!typed) addError("L" + std::to_string(lineNo) + ": especifique el tipo de la lista (enteros/decimales/texto/caracteres).");
            }
            // OPERACIONES con "mostrar resultado"
            else if ((lower.find("sumar") != std::string::npos ||
                lower.find("restar") != std::string::npos ||
                lower.find("multiplicar") != std::string::npos ||
                lower.find("dividir") != std::string::npos)) {
                recognized = true; // ahora aceptamos también sin "mostrar resultado"
            }
            // ASIGNAR (con o sin "valor")
            else if (lower.rfind("asignar ", 0) == 0) {
                recognized = true;
                size_t startVal = 8;
                if (lower.find("valor ", startVal) == startVal) startVal += 6;
                size_t aPos = lower.find(" a ", startVal);
                if (aPos == std::string::npos) addError("L" + std::to_string(lineNo) + ": falta ' a ' en asignacion.");
                else {
                    std::string varName = raw.substr(aPos + 3);
                    trim(varName);
                    if (varName.empty()) addError("L" + std::to_string(lineNo) + ": destino de asignacion vacio.");
                    // Permitimos "en posicion <expr>" (expr puede ser numero o variable)
                    // Si no trae "en posicion" validamos como identificador simple
                    std::string varNameLower = to_lower_copy(varName);
                    if (varNameLower.find(" en posicion ") == std::string::npos) {
                        if (!is_identifier(varName)) addError("L" + std::to_string(lineNo) + ": identificador invalido en asignacion.");
                    }
                }
            }
            // FOR/ PARA ... DESDE ... HASTA ...
            else if ((lower.rfind("for ", 0) == 0 || lower.rfind("para ", 0) == 0) &&
                lower.find(" desde ") != std::string::npos && lower.find(" hasta ") != std::string::npos) {
                recognized = true;
                size_t desdePos = lower.find(" desde ");
                size_t startPos = (lower.rfind("for ", 0) == 0) ? 4 : 5;
                std::string id = raw.substr(startPos, desdePos - startPos);
                trim(id);
                if (!std::isalpha((unsigned char)id[0]) && id[0] != '_') addError("L" + std::to_string(lineNo) + ": identificador invalido en 'para/for'.");
            }
            // REPETIR HASTA
            else if (lower.find("repetir hasta ") == 0) { recognized = true; }
            // E/S y otros
            else if (lower.find("leer") != std::string::npos ||
                lower.find("ingresar") != std::string::npos ||
                lower.find("mostrar") != std::string::npos ||
                lower.find("imprimir") != std::string::npos ||
                lower.find("si ") != std::string::npos ||
                lower.find("sino") != std::string::npos ||
                lower.find("mientras") != std::string::npos ||
                lower.find("para cada") != std::string::npos ||
                lower.find("comenzar programa") != std::string::npos ||
                lower.find("terminar programa") != std::string::npos ||
                lower.find("definir funcion") != std::string::npos) {
                recognized = true;
            }

            if (!recognized) addError("L" + std::to_string(lineNo) + ": instruccion no reconocida.");
        }

        lastDSLok = (errors.getSize() == 0);
        return lastDSLok;
    }
    catch (...) { addError("Error interno durante la validacion de sintaxis DSL"); lastDSLok = false; return false; }
}

bool NaturalLanguageProcessor::isValidToSave() const { return (errors.getSize() == 0) && !generatedCode.empty(); }
std::string NaturalLanguageProcessor::getGeneratedCode() const { return generatedCode; }
int NaturalLanguageProcessor::getInstructionCount() const { return instructionCount; }

void NaturalLanguageProcessor::parseInstructions(const std::string& input)
{
    try {
        instructionCount = 0;
        std::istringstream iss(input);
        std::string line;

        auto to_lower_copy = [](const std::string& str)->std::string {
            std::string r = str;
            std::transform(r.begin(), r.end(), r.begin(), [](unsigned char ch) { return std::tolower(ch); });
            return r;
            };

        while (std::getline(iss, line)) {
            if (!line.empty() && line[0] != '#') {
                instructionCount++;
                std::string lower = to_lower_copy(line);
                processDslList(line, lower);
                processDslAssignment(line, lower);
                processDslForEach(line, lower);
            }
        }
    }
    catch (...) { instructionCount = 0; }
}

void NaturalLanguageProcessor::generateCode()
{
    try {
        generatedCode.clear();

        auto to_lower_copy = [this](const std::string& str)->std::string {
            try {
                std::string r = str;
                std::transform(r.begin(), r.end(), r.begin(), [](unsigned char ch) { return std::tolower(ch); });
                return r;
            }
            catch (...) { addError("Error al convertir a minusculas"); return str; }
            };

        auto extract_ints = [this](const std::string& str, int out[], int maxn)->int {
            try {
                std::string cleaned = str;
                for (char& c : cleaned) if (!std::isdigit((unsigned char)c) && c != ' ' && c != '\t' && c != '-' && c != '+') c = ' ';
                std::istringstream iss(cleaned);
                int count = 0; std::string token;
                while (count < maxn && iss >> token) { int num; if (tryParseInt(token, num)) out[count++] = num; }
                return count;
            }
            catch (...) { addError("Error al extraer enteros"); return 0; }
            };

        auto read_identifier_after = [this](const std::string& lower, const std::string& original, const std::string& keyword)->std::string {
            try {
                size_t pos = lower.find(keyword);
                if (pos == std::string::npos) return "";
                pos += keyword.length();
                while (pos < original.length() && (original[pos] == ' ' || original[pos] == '\t')) pos++;
                size_t start = pos;
                while (pos < original.length() && (std::isalnum((unsigned char)original[pos]) || original[pos] == '_')) pos++;
                std::string result;
                if (start < pos && safeSubstr(original, start, pos - start, result)) return result;
                return "";
            }
            catch (...) { return ""; }
            };

        auto read_value_after = [this](const std::string& lower, const std::string& original, const std::string& keyword)->std::string {
            try {
                size_t pos = lower.find(keyword);
                if (pos == std::string::npos) return "";
                pos += keyword.length();
                while (pos < original.length() && (original[pos] == ' ' || original[pos] == '\t')) pos++;
                size_t start = pos; size_t end = original.length();
                std::string result;
                if (start < end && safeSubstr(original, start, end - start, result)) {
                    size_t r = result.find_last_not_of(" \t\r\n");
                    if (r != std::string::npos) result.erase(r + 1);
                    return result;
                }
                return "";
            }
            catch (...) { return ""; }
            };

        SymbolTable symbols;
        int sumaIdx = 1, restaIdx = 1, productoIdx = 1, divisionIdx = 1, listaIdx = 1, foreachIdxCounter = 0;

        struct LineInfo { int indent; std::string raw; std::string lower; };
        LinkedList<LineInfo> L;
        {
            std::istringstream iss2(inputContent);
            std::string ln;
            while (std::getline(iss2, ln)) {
                if (ln.empty() || ln[0] == '#') continue;
                int ind = countIndent(ln);
                std::string raw = trim_copy(ln);
                if (raw.empty()) continue;
                std::string low = lower_copy(raw);
                LineInfo info{ ind, raw, low };
                L.add(info);
            }
        }

        // Header: SOLO iostream
        generatedCode += "#include <iostream>\n";
        generatedCode += "using namespace std;\n\n";

        // Helpers para texto (copia segura) - evita incluir <cstring>
        generatedCode += "void __copia(char* dst, const char* src, int cap=127){\n";
        generatedCode += "    if(!dst||!src) return; int i=0; for(; src[i] && i<cap; ++i) dst[i]=src[i]; dst[i]='\\0'; }\n\n";

        generatedCode += "int main() {\n";

        Stack<Block> stack;
        auto closeBlocksTo = [&](int targetIndent) {
            Block b;
            while (!stack.empty()) {
                const Block* top = stack.top();
                if (top && top->indent < targetIndent) break;
                if (stack.pop(b)) {
                    if (b.type == BlockType::DoUntil) {
                        generatedCode += "    " + std::string(b.indent * 4, ' ') + "} while (" + b.extra + ");\n";
                    }
                    else {
                        generatedCode += "    " + std::string(b.indent * 4, ' ') + "}\n";
                    }
                }
            }
            };

        // Helpers para operaciones sin "mostrar resultado"
        auto isIdent = [](const std::string& s) {
            if (s.empty()) return false;
            if (!(std::isalpha((unsigned char)s[0]) || s[0] == '_')) return false;
            for (char c : s) if (!(std::isalnum((unsigned char)c) || c == '_')) return false;
            return true;
            };
        auto buildExpr = [](std::string rest, const std::string& op) {
            rest = trim_copy(rest);
            std::string expr; std::string low = lower_copy(rest);
            size_t start = 0;
            while (true) {
                size_t pos = low.find(" y ", start);
                std::string term = trim_copy(rest.substr(start, (pos == std::string::npos ? rest.size() : pos) - start));
                if (!expr.empty()) expr += " " + op + " ";
                expr += term;
                if (pos == std::string::npos) break;
                start = pos + 3;
            }
            return expr;
            };

        for (int i = 0; i < L.getSize(); i++) {
            try {
                LineInfo info = L.get(i);
                int ind = info.indent;
                std::string raw = info.raw;
                std::string low = info.lower;

                closeBlocksTo(ind);

                if (low.find("comenzar programa") != std::string::npos ||
                    low.find("terminar programa") != std::string::npos ||
                    low.find("definir funcion") != std::string::npos) {
                    continue;
                }

                // COMENTARIO
                if (low.rfind("comentario ", 0) == 0) {
                    std::string resto = trim_copy(raw.substr(11));
                    generatedCode += "    " + std::string(ind * 4, ' ') + "// " + resto + "\n";
                    continue;
                }

                // SI
                if (low.rfind("si ", 0) == 0) {
                    std::string cond = trim_copy(raw.substr(3));
                    generatedCode += "    " + std::string(ind * 4, ' ') + "if (" + toCppCondition(cond) + ") {\n";
                    Block b{ BlockType::If, ind, "" };
                    stack.push(b);
                    continue;
                }

                // SINO
                if (low == "sino" || low.rfind("sino", 0) == 0) {
                    Block b;
                    // Primero verificar si hay un bloque If/Else en el stack sin hacer pop
                    const Block* top = stack.top();
                    if (top && (top->type == BlockType::If || top->type == BlockType::Else) && top->indent == ind) {
                        // Ahora sí hacer pop porque sabemos que es válido
                        stack.pop(b);
                        generatedCode += "    " + std::string(ind * 4, ' ') + "} else {\n";
                        b.type = BlockType::Else; 
                        stack.push(b);
                    }
                    else {
                        addError("Bloque 'sino' sin 'si' correspondiente o con indentacion incorrecta.");
                        generatedCode += "    " + std::string(ind * 4, ' ') + "} else {\n";
                        Block newB{ BlockType::Else, ind, "" }; 
                        stack.push(newB);
                    }
                    continue;
                }

                // MIENTRAS
                if (low.rfind("mientras ", 0) == 0) {
                    std::string cond = trim_copy(raw.substr(9));
                    generatedCode += "    " + std::string(ind * 4, ' ') + "while (" + toCppCondition(cond) + ") {\n";
                    Block b{ BlockType::While, ind, "" };
                    stack.push(b);
                    continue;
                }

                // REPETIR HASTA
                if (low.rfind("repetir hasta ", 0) == 0) {
                    std::string cond = trim_copy(raw.substr(14));
                    generatedCode += "    " + std::string(ind * 4, ' ') + "do {\n";
                    Block b{ BlockType::DoUntil, ind, toCppCondition(cond) };
                    stack.push(b);
                    continue;
                }

                // PARA CADA
                if (low.rfind("para cada ", 0) == 0 && low.find(" en ") != std::string::npos) {
                    size_t enPos = low.find(" en ");
                    std::string item = trim_copy(raw.substr(10, enPos - 10));
                    std::string list = trim_copy(raw.substr(enPos + 4));

                    int arrSize = 0;
                    const Symbol* sym = symbols.find(list);
                    if (sym && sym->isArray) arrSize = sym->size;

                    std::string idx = "__i" + std::to_string(foreachIdxCounter++);
                    std::string bound;
                    if (arrSize > 0) {
                        bound = std::to_string(arrSize);
                    } else {
                        // Usar sizeof que funciona en todos los compiladores
                        bound = "(int)(sizeof(" + list + ")/sizeof(" + list + "[0]))";
                    }
                    generatedCode += "    " + std::string(ind * 4, ' ') + "for (int " + idx + " = 0; " + idx + " < " + bound + "; ++" + idx + ") {\n";
                    generatedCode += "    " + std::string((ind + 1) * 4, ' ') + "auto " + item + " = " + list + "[" + idx + "];\n";
                    // registrar item en símbolos (tipo del arreglo)
                    if (sym) { Symbol si; si.name = item; si.type = sym->type; si.isArray = false; si.size = 0; symbols.add(si); }
                    Block b{ BlockType::Foreach, ind, "" };
                    stack.push(b);
                    continue;
                }

                // FOR/ PARA ... DESDE ... HASTA ...
                if ((low.rfind("for ", 0) == 0 || low.rfind("para ", 0) == 0) && low.find(" desde ") != std::string::npos && low.find(" hasta ") != std::string::npos) {
                    size_t desdePos = low.find(" desde ");
                    size_t hastaPos = low.find(" hasta ");
                    size_t startPos = (low.rfind("for ", 0) == 0) ? 4 : 5;
                    std::string id = trim_copy(raw.substr(startPos, desdePos - startPos));
                    std::string ini = trim_copy(raw.substr(desdePos + 7, hastaPos - (desdePos + 7)));
                    std::string rest = raw.substr(hastaPos + 7);
                    std::string paso;
                    size_t pasoPos = lower_copy(rest).find(" paso ");
                    std::string fin = rest;
                    if (pasoPos != std::string::npos) { fin = trim_copy(rest.substr(0, pasoPos)); paso = trim_copy(rest.substr(pasoPos + 6)); }
                    else { fin = trim_copy(fin); }
                    std::string stepExpr = !paso.empty() ? paso : ("((" + ini + ")<=(" + fin + ")?1:-1)");
                    generatedCode += "    " + std::string(ind * 4, ' ') + "for (int " + id + " = " + ini + "; ((" + stepExpr + ")>0 ? " + id + " <= " + fin + " : " + id + " >= " + fin + "); " + id + " += " + stepExpr + ") {\n";
                    Block b{ BlockType::For, ind, "" };
                    stack.push(b);
                    continue;
                }

                // CREAR VARIABLE
                if (low.find("crear variable") != std::string::npos) {
                    std::string name = read_identifier_after(low, raw, "llamada ");
                    std::string value = read_value_after(low, raw, "valor ");

                    if (low.find("entero") != std::string::npos) {
                        if (name.empty()) name = "numero";
                        int val = 0; if (!value.empty()) { if (!tryParseInt(value, val)) { addError("Valor entero invalido en linea: " + raw); val = 0; } }
                        generatedCode += "    " + std::string(ind * 4, ' ') + "int " + name + " = " + std::to_string(val) + ";\n";
                        Symbol s{ name,"int",false,0 }; symbols.add(s);
                    }
                    else if (low.find("decimal") != std::string::npos) {
                        if (name.empty()) name = "promedio";
                        double val = 0.0; if (!value.empty()) { if (!tryParseDouble(value, val)) { addError("Valor decimal invalido en linea: " + raw); val = 0.0; } }
                        generatedCode += "    " + std::string(ind * 4, ' ') + "double " + name + " = " + std::to_string(val) + ";\n";
                        Symbol s{ name,"double",false,0 }; symbols.add(s);
                    }
                    else if (low.find("texto") != std::string::npos) {
                        if (name.empty()) name = "nombre";
                        std::string val = ""; if (!value.empty()) val = value;
                        generatedCode += "    " + std::string(ind * 4, ' ') + "char " + name + "[128] = \"" + escapeForCxxString(val) + "\";\n";
                        Symbol s{ name,"char[]",false,128 }; symbols.add(s);
                    }
                    else if (low.find("caracter") != std::string::npos) {
                        if (name.empty()) name = "inicial";
                        char val = '\0'; if (!value.empty()) val = value[0];
                        generatedCode += "    " + std::string(ind * 4, ' ') + "char " + name + " = '" + std::string(1, val) + "';\n";
                        Symbol s{ name,"char",false,0 }; symbols.add(s);
                    }
                    else if (low.find("booleano") != std::string::npos) {
                        if (name.empty()) name = "activo";
                        bool val = (!value.empty() && value == "true");
                        generatedCode += "    " + std::string(ind * 4, ' ') + "bool " + name + " = " + (val ? "true" : "false") + ";\n";
                        Symbol s{ name,"bool",false,0 }; symbols.add(s);
                    }
                    continue;
                }

                // CREAR LISTA/ARREGLO
                if (low.find("crear lista") != std::string::npos || low.find("crear arreglo") != std::string::npos) {
                    std::string name = read_identifier_after(low, raw, "llamada ");
                    int nums[5]; int count = extract_ints(raw, nums, 5);
                    int size = (count > 0) ? nums[0] : 5; if (size <= 0) { addError("Tamaño de lista invalido en linea: " + raw); size = 1; }

                    if (low.find("enteros") != std::string::npos) {
                        if (name.empty()) name = "lista" + std::to_string(listaIdx++);
                        generatedCode += "    " + std::string(ind * 4, ' ') + "int " + name + "[" + std::to_string(size) + "] = {0};\n";
                        Symbol s{ name,"int",true,size }; symbols.add(s);
                    }
                    else if (low.find("decimales") != std::string::npos) {
                        if (name.empty()) name = "promedios";
                        generatedCode += "    " + std::string(ind * 4, ' ') + "double " + name + "[" + std::to_string(size) + "] = {0.0};\n";
                        Symbol s{ name,"double",true,size }; symbols.add(s);
                    }
                    else if (low.find("texto") != std::string::npos) {
                        if (name.empty()) name = "nombres";
                        generatedCode += "    " + std::string(ind * 4, ' ') + "char " + name + "[" + std::to_string(size) + "][128] = {{0}};\n";
                        Symbol s{ name,"char[128]",true,size }; symbols.add(s);
                    }
                    else if (low.find("caracteres") != std::string::npos) {
                        if (name.empty()) name = "iniciales";
                        generatedCode += "    " + std::string(ind * 4, ' ') + "char " + name + "[" + std::to_string(size) + "] = {0};\n";
                        Symbol s{ name,"char",true,size }; symbols.add(s);
                    }
                    continue;
                }

                // OPERACIONES ARITMETICAS - con "mostrar resultado"
                if (low.find("sumar") != std::string::npos && low.find("mostrar resultado") != std::string::npos) {
                    int nums[10]; int count = extract_ints(raw, nums, 10);
                    if (count >= 2) {
                        std::string varName = "suma" + std::to_string(sumaIdx++);
                        generatedCode += "    " + std::string(ind * 4, ' ') + "int " + varName + " = ";
                        for (int j = 0; j < count; j++) { if (j > 0) generatedCode += " + "; generatedCode += std::to_string(nums[j]); }
                        generatedCode += ";\n";
                        generatedCode += "    " + std::string(ind * 4, ' ') + "cout << \"El resultado es: \" << " + varName + " << endl;\n";
                        Symbol s{ varName,"int",false,0 }; symbols.add(s);
                    }
                    else { addError("Operacion suma requiere al menos 2 numeros en linea: " + raw); }
                    continue;
                }
                if (low.find("restar") != std::string::npos && low.find("mostrar resultado") != std::string::npos) {
                    int nums[10]; int count = extract_ints(raw, nums, 10);
                    if (count >= 2) {
                        std::string varName = "resta" + std::to_string(restaIdx++);
                        generatedCode += "    " + std::string(ind * 4, ' ') + "int " + varName + " = " + std::to_string(nums[0]) + ";\n";
                        for (int j = 1; j < count; j++) generatedCode += "    " + std::string(ind * 4, ' ') + varName + " -= " + std::to_string(nums[j]) + ";\n";
                        generatedCode += "    " + std::string(ind * 4, ' ') + "cout << \"El resultado es: \" << " + varName + " << endl;\n";
                        Symbol s{ varName,"int",false,0 }; symbols.add(s);
                    }
                    else { addError("Operacion resta requiere al menos 2 numeros en linea: " + raw); }
                    continue;
                }
                if (low.find("multiplicar") != std::string::npos && low.find("mostrar resultado") != std::string::npos) {
                    int nums[10]; int count = extract_ints(raw, nums, 10);
                    if (count >= 2) {
                        std::string varName = "producto" + std::to_string(productoIdx++);
                        generatedCode += "    " + std::string(ind * 4, ' ') + "int " + varName + " = 1;\n";
                        for (int j = 0; j < count; j++) generatedCode += "    " + std::string(ind * 4, ' ') + varName + " *= " + std::to_string(nums[j]) + ";\n";
                        generatedCode += "    " + std::string(ind * 4, ' ') + "cout << \"El resultado es: \" << " + varName + " << endl;\n";
                        Symbol s{ varName,"int",false,0 }; symbols.add(s);
                    }
                    else { addError("Operacion multiplicar requiere al menos 2 numeros en linea: " + raw); }
                    continue;
                }
                if (low.find("dividir") != std::string::npos && low.find("mostrar resultado") != std::string::npos) {
                    int nums[10]; int count = extract_ints(raw, nums, 10);
                    if (count >= 2) {
                        std::string varName = "division" + std::to_string(divisionIdx++);
                        generatedCode += "    " + std::string(ind * 4, ' ') + "int " + varName + " = " + std::to_string(nums[0]) + ";\n";
                        generatedCode += "    " + std::string(ind * 4, ' ') + "bool divisionValida = true;\n";
                        for (int j = 1; j < count; j++) {
                            if (nums[j] == 0) {
                                generatedCode += "    " + std::string(ind * 4, ' ') + "divisionValida = false; cout << \"Error: division por cero\" << endl;\n";
                            }
                            else {
                                generatedCode += "    " + std::string(ind * 4, ' ') + "if (divisionValida) " + varName + " /= " + std::to_string(nums[j]) + ";\n";
                            }
                        }
                        generatedCode += "    " + std::string(ind * 4, ' ') + "if (divisionValida) cout << \"El resultado es: \" << " + varName + " << endl;\n";
                        Symbol s{ varName,"int",false,0 }; symbols.add(s);
                    }
                    else { addError("Operacion dividir requiere al menos 2 numeros en linea: " + raw); }
                    continue;
                }

                // Operaciones SIN "mostrar resultado"
                if (low.rfind("sumar ", 0) == 0 && low.find("mostrar resultado") == std::string::npos) {
                    std::string rest = raw.substr(6);
                    std::string expr = buildExpr(rest, "+");
                    size_t p = lower_copy(rest).find(" y ");
                    std::string first = trim_copy(p == std::string::npos ? rest : rest.substr(0, p));
                    const Symbol* s0 = symbols.find(first);
                    if (s0 && !s0->isArray) generatedCode += "    " + std::string(ind * 4, ' ') + first + " = " + expr + ";\n";
                    else {
                        std::string varName = "suma" + std::to_string(sumaIdx++);
                        generatedCode += "    " + std::string(ind * 4, ' ') + "int " + varName + " = " + expr + ";\n";
                        Symbol s{ varName,"int",false,0 }; symbols.add(s);
                    }
                    continue;
                }
                if (low.rfind("restar ", 0) == 0 && low.find("mostrar resultado") == std::string::npos) {
                    std::string rest = raw.substr(7);
                    std::string expr = buildExpr(rest, "-");
                    size_t p = lower_copy(rest).find(" y ");
                    std::string first = trim_copy(p == std::string::npos ? rest : rest.substr(0, p));
                    const Symbol* s0 = symbols.find(first);
                    if (s0 && !s0->isArray) generatedCode += "    " + std::string(ind * 4, ' ') + first + " = " + expr + ";\n";
                    else {
                        std::string varName = "resta" + std::to_string(restaIdx++);
                        generatedCode += "    " + std::string(ind * 4, ' ') + "int " + varName + " = " + expr + ";\n";
                        Symbol s{ varName,"int",false,0 }; symbols.add(s);
                    }
                    continue;
                }
                if (low.rfind("multiplicar ", 0) == 0 && low.find("mostrar resultado") == std::string::npos) {
                    std::string rest = raw.substr(12);
                    std::string expr = buildExpr(rest, "*");
                    std::string varName = "producto" + std::to_string(productoIdx++);
                    generatedCode += "    " + std::string(ind * 4, ' ') + "int " + varName + " = " + expr + ";\n";
                    Symbol s{ varName,"int",false,0 }; symbols.add(s);
                    continue;
                }
                if (low.rfind("dividir ", 0) == 0 && low.find("mostrar resultado") == std::string::npos) {
                    std::string rest = raw.substr(8);
                    std::string expr = buildExpr(rest, "/");
                    std::string varName = "division" + std::to_string(divisionIdx++);
                    generatedCode += "    " + std::string(ind * 4, ' ') + "int " + varName + " = " + expr + ";\n";
                    Symbol s{ varName,"int",false,0 }; symbols.add(s);
                    continue;
                }

                // E/S
                if (low.find("leer") != std::string::npos || low.find("ingresar") != std::string::npos) {
                    std::istringstream issTokens(raw);
                    std::string token; bool found = false;
                    while (issTokens >> token && !found) {
                        const Symbol* sym = symbols.find(token);
                        if (sym && !sym->isArray) {
                            if (sym->type == "char[]") {
                                generatedCode += "    " + std::string(ind * 4, ' ') + "cin.ignore(10000, '\\n');\n";
                                generatedCode += "    " + std::string(ind * 4, ' ') + "cin.getline(" + sym->name + ", 128);\n";
                            }
                            else {
                                generatedCode += "    " + std::string(ind * 4, ' ') + "cin >> " + sym->name + ";\n";
                            }
                            found = true;
                        }
                    }
                    continue;
                }

                if (low.find("mostrar") != std::string::npos || low.find("imprimir") != std::string::npos) {
                    bool foundVar = false;
                    std::istringstream issTokens(raw);
                    std::string token;
                    while (issTokens >> token && !foundVar) {
                        const Symbol* sym = symbols.find(token);
                        if (sym && !sym->isArray) {
                            generatedCode += "    " + std::string(ind * 4, ' ') + "cout << " + sym->name + " << endl;\n";
                            foundVar = true;
                        }
                    }
                    if (!foundVar) {
                        size_t pos = low.find("mostrar"); if (pos == std::string::npos) pos = low.find("imprimir");
                        pos += (low.find("mostrar") != std::string::npos) ? 7 : 8;
                        std::string message;
                        if (safeSubstr(raw, pos, raw.length() - pos, message)) {
                            message.erase(0, message.find_first_not_of(" \t"));
                            message.erase(message.find_last_not_of(" \t") + 1);
                            if (!message.empty() && message[0] == ':') { std::string temp; if (safeSubstr(message, 1, message.length() - 1, temp)) { message = trim_copy(temp); } }
                            if (!message.empty()) generatedCode += "    " + std::string(ind * 4, ' ') + "cout << \"" + escapeForCxxString(message) + "\" << endl;\n";
                        }
                    }
                    continue;
                }

                // ASIGNACIONES (con o sin "valor")
                if (lower.rfind("asignar ", 0) == 0) {
                    std::string lowerRaw = lower_copy(raw);
                    size_t startVal = 8;
                    if (lowerRaw.find("valor ", startVal) == startVal) startVal += 6;
                    size_t aPos = lowerRaw.find(" a ", startVal);
                    if (aPos != std::string::npos) {
                        std::string value, varName;
                        if (safeSubstr(raw, startVal, aPos - startVal, value) &&
                            safeSubstr(raw, aPos + 3, raw.length() - (aPos + 3), varName)) {

                            value = trim_copy(value);
                            varName = trim_copy(varName);

                            size_t posPos = lower_copy(varName).find(" en posicion ");
                            if posPos != std::string::npos) {
                                std::string arrName = trim_copy(varName.substr(0, posPos));
                                std::string indexStr = trim_copy(varName.substr(posPos + 13));
                                // Para listas de texto (char[128]) usamos __copia
                                const Symbol* sarr = symbols.find(arrName);
                                if (sarr && sarr->isArray && sarr->type == "char[128]") {
                                    // si value no es variable conocida, trátalo como literal
                                    const Symbol* v = symbols.find(value);
                                    if (v && v->type == "char[]") {
                                        generatedCode += "    " + std::string(ind * 4, ' ') + "__copia(" + arrName + "[" + indexStr + "], " + value + ");\n";
                                    }
                                    else {
                                        generatedCode += "    " + std::string(ind * 4, ' ') + "__copia(" + arrName + "[" + indexStr + "], \"" + escapeForCxxString(value) + "\");\n";
                                    }
                                }
                                else {
                                    generatedCode += "    " + std::string(ind * 4, ' ') + arrName + "[" + indexStr + "] = " + value + ";\n";
                                }
                            }
                            else {
                                const Symbol* sv = symbols.find(varName);
                                if (sv && sv->type == "char[]") {
                                    const Symbol* v = symbols.find(value);
                                    if (v && v->type == "char[]") {
                                        generatedCode += "    " + std::string(ind * 4, ' ') + "__copia(" + varName + ", " + value + ");\n";
                                    }
                                    else {
                                        generatedCode += "    " + std::string(ind * 4, ' ') + "__copia(" + varName + ", \"" + escapeForCxxString(value) + "\");\n";
                                    }
                                }
                                else {
                                    generatedCode += "    " + std::string(ind * 4, ' ') + varName + " = " + value + ";\n";
                                }
                            }
                        }
                        else { addError("Error de sintaxis en asignacion: " + raw); }
                    }
                    continue;
                }

                // Si nada aplicó, deja la línea como comentario
                generatedCode += "    " + std::string(ind * 4, ' ') + "// " + raw + "\n";
            }
            catch (...) { addError("Error interno procesando linea."); }
        }

        closeBlocksTo(0);
        generatedCode += "    return 0;\n";
        generatedCode += "}\n";
    }
    catch (...) { addError("Error interno durante la generacion de codigo."); }
}

void NaturalLanguageProcessor::validateCode()
{
    try {
        if (generatedCode.empty()) { addError("El codigo generado esta vacio"); return; }
        int braceCount = 0;
        for (char c : generatedCode) { if (c == '{') braceCount++; if (c == '}') braceCount--; }
        if (braceCount != 0) addError("Llaves no balanceadas en el codigo generado");
    }
    catch (...) { addError("Error interno durante la validacion de codigo"); }
}

const LinkedList<std::string>& NaturalLanguageProcessor::getErrors() const { return errors; }

void NaturalLanguageProcessor::clearErrors() { try { errors.clear(); } catch (...) {} }
void NaturalLanguageProcessor::addError(const std::string& error) { try { errors.add(error); } catch (...) {} }

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
    catch (...) {}
}

// Helpers seguros para parsing
bool NaturalLanguageProcessor::tryParseInt(const std::string& str, int& result) noexcept
{
    try { if (str.empty()) return false; size_t pos; result = std::stoi(str, &pos); return pos == str.length(); }
    catch (...) { return false; }
}

bool NaturalLanguageProcessor::tryParseDouble(const std::string& str, double& result) noexcept
{
    try { if (str.empty()) return false; size_t pos; result = std::stod(str, &pos); return pos == str.length(); }
    catch (...) { return false; }
}

bool NaturalLanguageProcessor::safeSubstr(const std::string& str, size_t start, size_t len, std::string& result) noexcept
{
    try { if (start >= str.length()) return false; size_t maxLen = str.length() - start; if (len > maxLen) len = maxLen; result = str.substr(start, len); return true; }
    catch (...) { return false; }
}

// Métodos auxiliares para procesamiento de listas DSL (no intrusivos)
void NaturalLanguageProcessor::processDslList(const std::string& line, const std::string& lower)
{
    try {
        if (lower.find("crear lista") != std::string::npos || lower.find("crear arreglo") != std::string::npos) {
            auto extract_ints_safe = [this](const std::string& str, int out[], int maxn)->int {
                try {
                    std::string cleaned = str;
                    for (char& c : cleaned) if (!std::isdigit((unsigned char)c) && c != ' ' && c != '\t') c = ' ';
                    std::istringstream iss(cleaned);
                    int count = 0; std::string token;
                    while (count < maxn && iss >> token) { int num; if (tryParseInt(token, num)) out[count++] = num; }
                    return count;
                }
                catch (...) { return 0; }
                };
            auto read_identifier_after_safe = [this](const std::string& lower, const std::string& original, const std::string& keyword)->std::string {
                try {
                    size_t pos = lower.find(keyword); if (pos == std::string::npos) return "";
                    pos += keyword.length();
                    while (pos < original.length() && (original[pos] == ' ' || original[pos] == '\t')) pos++;
                    size_t start = pos;
                    while (pos < original.length() && (std::isalnum((unsigned char)original[pos]) || original[pos] == '_')) pos++;
                    std::string result; if (start < pos && safeSubstr(original, start, pos - start, result)) return result;
                    return "";
                }
                catch (...) { return ""; }
                };

            int nums[5]; int count = extract_ints_safe(line, nums, 5);
            int size = (count > 0) ? nums[0] : 5;

            std::string name = read_identifier_after_safe(lower, line, "llamada ");
            if (name.empty()) name = "lista_auto";

            DslType dslType = DslType::Entero;
            if (lower.find("enteros") != std::string::npos) dslType = DslType::Entero;
            else if (lower.find("decimales") != std::string::npos) dslType = DslType::Decimal;
            else if (lower.find("texto") != std::string::npos) dslType = DslType::Texto;
            else if (lower.find("caracteres") != std::string::npos) dslType = DslType::Caracter;

            lists.createList(dslType, name, size);
        }
    }
    catch (...) {}
}

void NaturalLanguageProcessor::processDslAssignment(const std::string& line, const std::string& lower)
{
    try {
        if (lower.find("asignar") != std::string::npos && lower.find(" a ") != std::string::npos && lower.find("posicion") != std::string::npos) {
            size_t asignarPos = lower.find("asignar");
            size_t aPos = lower.find(" a ");
            if (asignarPos == std::string::npos || aPos == std::string::npos || aPos <= asignarPos) return;

            std::string valueStr;
            if (safeSubstr(line, asignarPos + 7, aPos - (asignarPos + 7), valueStr)) {
                valueStr = trim_copy(valueStr);
                if (valueStr.rfind("valor", 0) == 0) {
                    std::string temp; if (safeSubstr(valueStr, 5, valueStr.length() - 5, temp)) valueStr = trim_copy(temp);
                }
                std::string listAndPos; if (!safeSubstr(line, aPos + 3, line.length() - (aPos + 3), listAndPos)) return;
                size_t posicionPos = listAndPos.find("posicion"); if (posicionPos == std::string::npos) return;

                std::string listName; if (!safeSubstr(listAndPos, 0, posicionPos, listName)) return;
                listName.erase(listName.find_last_not_of(" \t") + 1);
                if (listName.find(" en") != std::string::npos) {
                    size_t enPos = listName.find(" en");
                    std::string temp; if (safeSubstr(listName, 0, enPos, temp)) listName = temp;
                }

                std::string indexStr; if (!safeSubstr(listAndPos, posicionPos + 8, listAndPos.length() - (posicionPos + 8), indexStr)) return;
                indexStr = trim_copy(indexStr);

                if (lists.exists(listName)) {
                    DslType listType = lists.typeOf(listName);
                    Value val(listType);
                    try {
                        switch (listType) {
                        case DslType::Entero: { int iv; if (tryParseInt(valueStr, iv)) val = Value(iv); break; }
                        case DslType::Decimal: { double dv; if (tryParseDouble(valueStr, dv)) val = Value(dv); break; }
                        case DslType::Texto: { val = Value(valueStr); break; }
                        case DslType::Caracter: { val = Value(valueStr.empty() ? '\0' : valueStr[0]); break; }
                        case DslType::Booleano: { val = Value(valueStr == "true" || valueStr == "1"); break; }
                        }
                    }
                    catch (...) {}
                    int idx = 0; if (tryParseInt(indexStr, idx)) lists.setAt(listName, idx, val);
                }
            }
        }
    }
    catch (...) {}
}

void NaturalLanguageProcessor::processDslForEach(const std::string& line, const std::string& lower)
{
    try {
        if (lower.find("para cada") != std::string::npos && lower.find(" en ") != std::string::npos) {
            size_t enPos = lower.find(" en ");
            std::string listName;
            if (safeSubstr(line, enPos + 4, line.length() - (enPos + 4), listName)) {
                listName = trim_copy(listName);
                if (lists.exists(listName)) {
                    lists.forEach(listName, [](const Value& v) {});
                }
            }
        }
    }
    catch (...) {}
}
