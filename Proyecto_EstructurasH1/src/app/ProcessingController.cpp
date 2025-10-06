#include "stdafx.h"
#include "ProcessingController.h"
#include "NaturalLanguageProcessor.h"
#include "LinkedList.h"
#include <QTextEdit>
#include <QProcess>
#include <QTemporaryFile>
#include <QDir>
#include <string>
#include <atomic>

using namespace std;

// BusyGuard utility
struct BusyGuard {
    atomic<bool>& flag;
    explicit BusyGuard(atomic<bool>& f) : flag(f) { flag = true; }
    ~BusyGuard() { flag = false; }
    BusyGuard(const BusyGuard&) = delete;
    BusyGuard& operator=(const BusyGuard&) = delete;
};

ProcessingController::ProcessingController(NaturalLanguageProcessor* nlp,
                                             QTextEdit* input, QTextEdit* output, QObject* parent)
    : QObject(parent), proc(nlp), in(input), out(output)
{
}

void ProcessingController::process()
{
    if (isProcessing.load()) {
        emit status("Ya hay un procesamiento en curso", true);
        return;
    }
    
    BusyGuard guard(isProcessing);
    
    try {
        if (!proc) {
            emit status("Error: Procesador no disponible", true);
            return;
        }
        
        if (!in) {
            emit status("Error: Editor de entrada no disponible", true);
            return;
        }
        
        QString qContent = in->toPlainText().trimmed();
        if (qContent.isEmpty()) {
            emit status("Error: El contenido esta vacio", true);
            return;
        }
        
        QByteArray ba = qContent.toUtf8();
        string content(ba.constData(), static_cast<size_t>(ba.size()));
        
        emit status("Procesando lenguaje natural...");
        
        proc->setInputContent(content);
        
        bool ok = proc->processText(content);
        
        if (ok) {
            string code = proc->getGeneratedCode();
            QString qCode = QString::fromUtf8(code.c_str(), static_cast<int>(code.size()));
            
            if (out) {
                out->setPlainText(qCode);
            }
            
            emit status("Conversion completada exitosamente!");
            emit instructionCountChanged(proc->getInstructionCount());
            emit hasInputChanged(true);
            emit hasGeneratedChanged(true);
            emit isValidatedChanged(false);
            
        } else {
            emit status("Error durante la conversion", true);
            
            const LinkedList<string>& errs = proc->getErrors();
            if (errs.getSize() > 0) {
                emit status("--- ERRORES ---", true);
                for (int i = 0; i < errs.getSize(); i++) {
                    string err = errs.get(i);
                    emit status("[" + QString::number(i+1) + "] " + QString::fromStdString(err), true);
                }
                emit status("--- FIN ERRORES ---", true);
            }
            
            emit hasInputChanged(true);
            emit hasGeneratedChanged(false);
            emit isValidatedChanged(false);
        }
        
    } catch (const exception& e) {
        emit status("Excepcion durante procesamiento: " + QString::fromStdString(e.what()), true);
        emit hasGeneratedChanged(false);
        emit isValidatedChanged(false);
    } catch (...) {
        emit status("Excepcion desconocida durante procesamiento", true);
        emit hasGeneratedChanged(false);
        emit isValidatedChanged(false);
    }
}

void ProcessingController::validate()
{
    if (isProcessing.load()) {
        emit status("Ya hay un procesamiento en curso", true);
        return;
    }
    
    BusyGuard guard(isProcessing);
    
    try {
        if (!proc) {
            emit status("Error: Procesador no disponible", true);
            return;
        }
        
        string code = proc->getGeneratedCode();
        if (code.empty()) {
            emit status("Error: No hay codigo para validar", true);
            return;
        }
        
        emit status("Validando sintaxis C++...");
        
        bool ok = proc->validateSyntax();
        
        if (ok) {
            emit status("Validacion interna exitosa! El codigo es valido");
            emit isValidatedChanged(true);
            
            // NUEVO: Intentar compilación con g++ o clang++
            emit status("Intentando compilacion externa...");
            
            // Verificar rápidamente si hay algún compilador antes de crear archivos
            QProcess testCompiler;
            testCompiler.start("g++", QStringList() << "--version");
            bool hasGpp = testCompiler.waitForStarted(1000);
            if (hasGpp) {
                testCompiler.waitForFinished(1000);
                testCompiler.close();
            }
            
            if (!hasGpp) {
                testCompiler.start("clang++", QStringList() << "--version");
                bool hasClang = testCompiler.waitForStarted(1000);
                if (hasClang) {
                    testCompiler.waitForFinished(1000);
                    testCompiler.close();
                }
                
                if (!hasClang) {
                    emit status("No se encontro g++/clang++ en el PATH. Se realizo la validacion interna pero no se pudo compilar externamente.", false);
                    emit status("  Sugerencia: Instala MinGW-w64 o MSYS2 para habilitar compilacion externa.", false);
                    return;
                }
            }
            
            // Crear archivo temporal para el código C++
            QString tempPath = QDir::tempPath();
            QString cppFile = tempPath + "/C_HaroldA_tmp.cpp";
            QString exeFile = tempPath + "/C_HaroldA_tmp.exe";
            
            // Escribir código al archivo temporal
            QFile file(cppFile);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << QString::fromStdString(code);
                file.close();
                
                // Intentar compilar con g++
                QProcess compiler;
                compiler.setProgram("g++");
                compiler.setArguments({
                    "-std=c++17",
                    "-O0",
                    "-Wall",
                    "-Wextra",
                    "-pedantic",
                    cppFile,
                    "-o", exeFile
                });
                
                compiler.setProcessChannelMode(QProcess::MergedChannels);
                compiler.start();
                
                if (!compiler.waitForStarted(3000)) {
                    // g++ no disponible, intentar con clang++
                    compiler.setProgram("clang++");
                    compiler.start();
                    
                    if (!compiler.waitForStarted(3000)) {
                        // Ningún compilador disponible
                        emit status("No se encontro g++/clang++ en el PATH. Se realizo la validacion interna pero no se pudo compilar externamente.", false);
                        emit status("  Sugerencia: Instala MinGW-w64 o MSYS2 para habilitar compilacion externa.", false);
                        QFile::remove(cppFile);
                        return;
                    }
                }
                
                // Esperar a que termine la compilación
                if (compiler.waitForFinished(10000)) {
                    int exitCode = compiler.exitCode();
                    QString output = compiler.readAll();
                    
                    if (exitCode == 0) {
                        emit status("Compilacion exitosa (validacion externa OK)", false);
                        // Limpiar archivos temporales
                        QFile::remove(cppFile);
                        QFile::remove(exeFile);
                    } else {
                        emit status("La compilacion externa encontro errores:", true);
                        
                        // Emitir cada línea del output del compilador
                        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
                        for (const QString& line : lines) {
                            bool isError = line.contains("error", Qt::CaseInsensitive) || 
                                         line.contains("fatal", Qt::CaseInsensitive);
                            emit status("  " + line, isError);
                        }
                        
                        // Limpiar archivo temporal
                        QFile::remove(cppFile);
                    }
                } else {
                    emit status("Timeout esperando compilacion externa", true);
                    compiler.kill();
                    QFile::remove(cppFile);
                }
            } else {
                emit status("No se pudo crear archivo temporal para compilacion", true);
            }
            
        } else {
            emit status("El codigo contiene errores de sintaxis", true);
            
            const LinkedList<string>& errs = proc->getErrors();
            if (errs.getSize() > 0) {
                emit status("--- ERRORES ---", true);
                for (int i = 0; i < errs.getSize(); i++) {
                    string err = errs.get(i);
                    emit status("[" + QString::number(i+1) + "] " + QString::fromStdString(err), true);
                }
                emit status("--- FIN ERRORES ---", true);
            }
            
            emit isValidatedChanged(false);
        }
        
    } catch (const exception& e) {
        emit status("Excepcion durante validacion: " + QString::fromStdString(e.what()), true);
        emit isValidatedChanged(false);
    } catch (...) {
        emit status("Excepcion desconocida durante validacion", true);
        emit isValidatedChanged(false);
    }
}
