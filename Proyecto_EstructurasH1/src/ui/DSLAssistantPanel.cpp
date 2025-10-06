#include "stdafx.h"
#include "DSLAssistantPanel.h"
#include <QDockWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCompleter>
#include <QStringList>
#include <QTextCursor>
#include <QTextEdit>
#include <QLabel>
#include <QMainWindow>

DSLAssistantPanel::DSLAssistantPanel(QMainWindow* owner, QTextEdit* input, QObject* parent)
    : QObject(parent), inputEdit(input)
{
    buildUi(owner);
}

QDockWidget* DSLAssistantPanel::dock() const
{
    return dockWidget;
}

void DSLAssistantPanel::buildUi(QMainWindow* owner)
{
    dockWidget = new QDockWidget("Asistente DSL", owner);
    dockWidget->setStyleSheet(
        "QDockWidget { background-color: #111317; color: #fde68a; border: 1px solid #f59e0b; }"
        "QDockWidget::title { background-color: #111317; color: #fbbf24; font-weight: bold; padding: 4px; }"
    );

    QWidget* dockContent = new QWidget();
    dockContent->setStyleSheet("QWidget { background-color: #111317; color: #fde68a; }");

    QVBoxLayout* dockLayout = new QVBoxLayout(dockContent);
    dockLayout->setContentsMargins(8, 8, 8, 8);
    dockLayout->setSpacing(8);

    QLabel* titleLabel = new QLabel("Plantillas DSL");
    titleLabel->setStyleSheet("color: #fbbf24; font-weight: bold;");
    dockLayout->addWidget(titleLabel);

    tree = new QTreeWidget();
    tree->setHeaderHidden(true);
    tree->setRootIsDecorated(true);
    tree->setStyleSheet(
        "QTreeWidget { background-color: #0b0f14; color: #fde68a; border: 1px solid #f59e0b; border-radius: 4px; }"
        "QTreeWidget::item { padding: 4px; }"
        "QTreeWidget::item:hover { background-color: #1a1d22; }"
        "QTreeWidget::item:selected { background-color: #1a1d22; color: #fbbf24; }"
    );

    connect(tree, &QTreeWidget::itemDoubleClicked, this, &DSLAssistantPanel::onSnippetDoubleClick);

    dockLayout->addWidget(tree);

    QPushButton* insertBtn = new QPushButton("Insertar");
    insertBtn->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fbbf24, stop:1 #ea580c); "
        "color: #0b0f14; border: 1px solid #f59e0b; border-radius: 4px; padding: 4px 8px; font-weight: bold; }"
    );

    connect(insertBtn, &QPushButton::clicked, [this]() {
        QTreeWidgetItem* current = tree->currentItem();
        if (current && current->data(0, Qt::UserRole).isValid()) {
            QString snippetId = current->data(0, Qt::UserRole).toString();
            insertSnippet(snippetId, false);
        }
    });

    dockLayout->addWidget(insertBtn);

    dockWidget->setWidget(dockContent);
    owner->addDockWidget(Qt::RightDockWidgetArea, dockWidget);

    setupSnippets();
    setupAutoCompleter();
}

void DSLAssistantPanel::setupSnippets()
{
    tree->clear();

    // Categorías
    auto* catVars  = new QTreeWidgetItem(tree);  catVars->setText(0, "Variables");  catVars->setExpanded(true);  catVars->setFlags(Qt::ItemIsEnabled);
    auto* catOps   = new QTreeWidgetItem(tree);  catOps->setText(0, "Operaciones"); catOps->setExpanded(false);  catOps->setFlags(Qt::ItemIsEnabled);
    auto* catIO    = new QTreeWidgetItem(tree);  catIO->setText(0, "Entrada / Salida"); catIO->setExpanded(false); catIO->setFlags(Qt::ItemIsEnabled);
    auto* catList  = new QTreeWidgetItem(tree);  catList->setText(0, "Listas / Arreglos"); catList->setExpanded(false); catList->setFlags(Qt::ItemIsEnabled);
    auto* catCtrl  = new QTreeWidgetItem(tree);  catCtrl->setText(0, "Control");    catCtrl->setExpanded(true);  catCtrl->setFlags(Qt::ItemIsEnabled);
    auto* catTempl = new QTreeWidgetItem(tree);  catTempl->setText(0, "Plantillas"); catTempl->setExpanded(true); catTempl->setFlags(Qt::ItemIsEnabled);

    // Helper function to add items
    auto add = [](QTreeWidgetItem* parent, const char* text, const char* id){
        auto* it = new QTreeWidgetItem(parent);
        it->setText(0, text);
        it->setData(0, Qt::UserRole, QString::fromUtf8(id));
        it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        return it;
    };

    // Variables
    add(catVars,  "Variable Entero",   "var_int");
    add(catVars,  "Variable Decimal",  "var_dec");
    add(catVars,  "Variable Texto",    "var_text");
    add(catVars,  "Variable Caracter", "var_char");
    add(catVars,  "Variable Booleana", "var_bool");

    // Operaciones
    add(catOps, "Sumar y Mostrar",   "op_sum_print");
    add(catOps, "Sumar",             "op_sum");
    add(catOps, "Restar",            "op_sub");
    add(catOps, "Multiplicar",       "op_mul");
    add(catOps, "Dividir",           "op_div");
    add(catOps, "Asignar valor a variable", "op_assign");

    // Entrada / Salida
    add(catIO, "Leer variable",          "io_leer");
    add(catIO, "Ingresar variable",      "io_ingresar");
    add(catIO, "Mostrar texto",          "io_mostrar_texto");
    add(catIO, "Mostrar variable",       "io_mostrar_var");

    // Listas / Arreglos
    add(catList, "Crear lista enteros",     "list_int");
    add(catList, "Crear lista decimales",   "list_dec");
    add(catList, "Crear lista texto",       "list_text");
    add(catList, "Crear lista caracteres",  "list_char");
    add(catList, "Asignar a lista (posicion)", "list_assign_pos");

    // Control
    add(catCtrl, "Si - Sino",         "ctrl_if_else");
    add(catCtrl, "Mientras",          "ctrl_while");
    add(catCtrl, "Repetir hasta",     "ctrl_repeat");
    add(catCtrl, "Para",              "ctrl_for");  // Renombrado de "For clasico" a "Para"

    // Plantillas
    add(catTempl, "Programa Basico",        "templ_basic");
    add(catTempl, "Carga de lista con contador", "templ_load_list");
}

void DSLAssistantPanel::setupAutoCompleter()
{
    QStringList dslKeywords = {
        // estructura
        "comenzar", "programa", "terminar",
        // crear
        "crear", "variable", "lista", "arreglo",
        "entero", "decimal", "texto", "caracter", "booleano",
        "enteros", "decimales", "caracteres",
        "llamada", "valor", "elementos",
        // listas
        "posicion", "asignar",
        // io
        "leer", "ingresar", "mostrar", "imprimir",
        // operaciones
        "sumar", "restar", "multiplicar", "dividir",
        // control
        "si", "sino", "mientras", "repetir", "hasta", "para", "cada",
        "for", "desde", "paso", "PORT", "veces",
        "mayor", "menor", "igual", "distinto", "o", "y", "no"
    };
    
    QCompleter* completer = new QCompleter(dslKeywords, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    
    if (inputEdit) {
        inputEdit->installEventFilter(this);
    }
}

void DSLAssistantPanel::onSnippetDoubleClick(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if (item && item->data(0, Qt::UserRole).isValid()) {
        QString snippetId = item->data(0, Qt::UserRole).toString();
        insertSnippet(snippetId, true);
    }
}

void DSLAssistantPanel::insertSnippet(const QString& snippetId, bool useDialog)
{
    Q_UNUSED(useDialog);
    
    QString t;

    // VARIABLES
    if      (snippetId == "var_int")   t = "crear variable entero llamada edad valor 0";
    else if (snippetId == "var_dec")   t = "crear variable decimal llamada altura valor 0.0";
    else if (snippetId == "var_text")  t = "crear variable texto llamada nombre valor Harold";
    else if (snippetId == "var_char")  t = "crear variable caracter llamada inicial valor H";
    else if (snippetId == "var_bool")  t = "crear variable booleano llamada activo valor true";

    // OPERACIONES
    else if (snippetId == "op_sum_print") t = "sumar 5 y 10 y mostrar resultado";
    else if (snippetId == "op_sum")       t = "sumar 10, 20, 30";
    else if (snippetId == "op_sub")       t = "restar 100 y 25";
    else if (snippetId == "op_mul")       t = "multiplicar 8 y 9 y 2";
    else if (snippetId == "op_div")       t = "dividir 144 entre 12";
    else if (snippetId == "op_assign")    t = "asignar valor 0 a contador";

    // ENTRADA / SALIDA
    else if (snippetId == "io_leer")          t = "leer edad";
    else if (snippetId == "io_ingresar")      t = "ingresar nombre";
    else if (snippetId == "io_mostrar_texto") t = "mostrar Bienvenido a C_HaroldA";
    else if (snippetId == "io_mostrar_var")   t = "mostrar edad";

    // LISTAS
    else if (snippetId == "list_int")       t = "crear lista de enteros con 5 elementos llamada numeros";
    else if (snippetId == "list_dec")       t = "crear lista de decimales con 3 elementos llamada promedios";
    else if (snippetId == "list_text")      t = "crear lista de texto con 4 elementos llamada nombres";
    else if (snippetId == "list_char")      t = "crear lista de caracteres con 3 elementos llamada iniciales";
    else if (snippetId == "list_assign_pos")t = "asignar 99 a numeros en posicion 0";

    // CONTROL
    else if (snippetId == "ctrl_if_else")
        t = "si edad mayor que 18\n    mostrar Es mayor de edad\nsino\n    mostrar Es menor de edad";
    else if (snippetId == "ctrl_while")
        t = "mientras contador menor que 3\n    sumar contador y 1";
    else if (snippetId == "ctrl_repeat")
        t = "repetir hasta edad igual a 50\n    sumar edad y 1";
    else if (snippetId == "ctrl_for")
        t = "para i desde 0 hasta 9 paso 1\n    mostrar i";  // Cambiado a "para" en lugar de "for"

    // PLANTILLAS
    else if (snippetId == "templ_basic")
        t = "comenzar programa\ncrear variable entero llamada contador valor 0\nmostrar contador\nterminar programa";
    else if (snippetId == "templ_load_list")
        t = "comenzar programa\ncrear lista de enteros con 3 elementos llamada numeros\ncrear variable entero llamada contador valor 0\ncrear variable entero llamada numero valor 0\nmientras contador menor que 3\n    leer numero\n    asignar numero a numeros en posicion contador\n    sumar contador y 1\npara cada n en numeros\n    mostrar n\nterminar programa";
    else
        return;
    
    if (inputEdit && !t.isEmpty()) {
        QTextCursor c = inputEdit->textCursor();
        if (!inputEdit->toPlainText().isEmpty())
            c.insertText("\n");
        c.insertText(t);
        inputEdit->setTextCursor(c);
    }
}
