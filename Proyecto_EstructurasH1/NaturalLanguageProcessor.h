#pragma once

#include <string>
#include "LinkedList.h"
#include "DslList.h"
#include "Stack.h"
#include "SymbolTable.h"

class NaturalLanguageProcessor 
{
public:
    NaturalLanguageProcessor();
    ~NaturalLanguageProcessor();
    
    // Métodos para manejo de archivos
    bool loadFile(const std::string& filePath);
    bool saveToFile(const std::string& filePath);
    std::string getInputContent() const;
    void setInputContent(const std::string& content);
    
    // Métodos para procesar lenguaje natural
    bool processText(const std::string& input);
    void processInstructions();
    std::string convertToCode(const std::string& naturalLanguage);
    bool validateSyntax();
    bool validateSyntax(const std::string& code);
    
    // Validación DSL
    bool validateInputSyntax();
    bool isValidToSave() const;
    
    // Getters para resultados
    std::string getGeneratedCode() const;
    const LinkedList<std::string>& getErrors() const;
    int getInstructionCount() const;
    
    // Métodos públicos de control de errores
    void clearErrors();
    void addError(const std::string& error);
    bool validateDSLOnly(const std::string& content);
    
    // Reset completo del procesador
    void reset();
    
    // Helpers seguros para parsing
    bool tryParseInt(const std::string& str, int& result) noexcept;
    bool tryParseDouble(const std::string& str, double& result) noexcept;
    bool safeSubstr(const std::string& str, size_t start, size_t len, std::string& result) noexcept;
    
    // Métodos auxiliares para procesamiento de listas DSL (no intrusivos)
    void processDslList(const std::string& line, const std::string& lower);
    void processDslAssignment(const std::string& line, const std::string& lower);
    void processDslForEach(const std::string& line, const std::string& lower);
    
private:
    std::string inputContent;
    std::string generatedCode;
    LinkedList<std::string> errors;
    int instructionCount;
    bool lastDSLok = false;
    bool lastCppOk = false;
    
    // Capa de listas para cumplir requisito académico
    DslList lists;
    
    // Métodos auxiliares privados
    void parseInstructions(const std::string& input);
    void generateCode();
    void validateCode();
};