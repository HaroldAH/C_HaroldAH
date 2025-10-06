#pragma once
#include <QObject>
#include <atomic>
#include <string>

class QTextEdit;
class NaturalLanguageProcessor;

class ProcessingController : public QObject {
    Q_OBJECT
public:
    ProcessingController(NaturalLanguageProcessor* nlp,
                         QTextEdit* input, QTextEdit* output, QObject* parent = nullptr);

public slots:
    void process();
    void validate();

signals:
    void status(const QString& msg, bool error = false);
    void instructionCountChanged(int count);
    void hasInputChanged(bool state);
    void hasGeneratedChanged(bool state);
    void isValidatedChanged(bool state);

private:
    NaturalLanguageProcessor* proc{};
    QTextEdit* in{};
    QTextEdit* out{};
    std::atomic<bool> isProcessing{false};
};
