/****************************************************************************
** Meta object code from reading C++ file 'qsciscintilla.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../build/_deps/qscintilla/src/ep_qscintilla/src/Qsci/qsciscintilla.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qsciscintilla.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN13QsciScintillaE_t {};
} // unnamed namespace

template <> constexpr inline auto QsciScintilla::qt_create_metaobjectdata<qt_meta_tag_ZN13QsciScintillaE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "QsciScintilla",
        "cursorPositionChanged",
        "",
        "line",
        "index",
        "copyAvailable",
        "yes",
        "indicatorClicked",
        "Qt::KeyboardModifiers",
        "state",
        "indicatorReleased",
        "linesChanged",
        "marginClicked",
        "margin",
        "marginRightClicked",
        "modificationAttempted",
        "modificationChanged",
        "m",
        "selectionChanged",
        "textChanged",
        "userListActivated",
        "id",
        "string",
        "append",
        "text",
        "autoCompleteFromAll",
        "autoCompleteFromAPIs",
        "autoCompleteFromDocument",
        "callTip",
        "clear",
        "copy",
        "cut",
        "ensureCursorVisible",
        "ensureLineVisible",
        "foldAll",
        "children",
        "foldLine",
        "indent",
        "insert",
        "insertAt",
        "moveToMatchingBrace",
        "paste",
        "redo",
        "removeSelectedText",
        "replaceSelectedText",
        "resetSelectionBackgroundColor",
        "resetSelectionForegroundColor",
        "selectAll",
        "select",
        "selectToMatchingBrace",
        "setAutoCompletionCaseSensitivity",
        "cs",
        "setAutoCompletionReplaceWord",
        "replace",
        "setAutoCompletionShowSingle",
        "single",
        "setAutoCompletionSource",
        "AutoCompletionSource",
        "source",
        "setAutoCompletionThreshold",
        "thresh",
        "setAutoCompletionUseSingle",
        "AutoCompletionUseSingle",
        "setAutoIndent",
        "autoindent",
        "setBraceMatching",
        "BraceMatch",
        "bm",
        "setBackspaceUnindents",
        "unindent",
        "setCaretForegroundColor",
        "QColor",
        "col",
        "setCaretLineBackgroundColor",
        "setCaretLineFrameWidth",
        "width",
        "setCaretLineVisible",
        "enable",
        "setCaretWidth",
        "setColor",
        "c",
        "setCursorPosition",
        "setEolMode",
        "EolMode",
        "mode",
        "setEolVisibility",
        "visible",
        "setFolding",
        "FoldStyle",
        "fold",
        "setIndentation",
        "indentation",
        "setIndentationGuides",
        "setIndentationGuidesBackgroundColor",
        "setIndentationGuidesForegroundColor",
        "setIndentationsUseTabs",
        "tabs",
        "setIndentationWidth",
        "setLexer",
        "QsciLexer*",
        "lexer",
        "setMarginsBackgroundColor",
        "setMarginsFont",
        "QFont",
        "f",
        "setMarginsForegroundColor",
        "setMarginLineNumbers",
        "lnrs",
        "setMarginMarkerMask",
        "mask",
        "setMarginSensitivity",
        "sens",
        "setMarginWidth",
        "s",
        "setModified",
        "setPaper",
        "setReadOnly",
        "ro",
        "setSelection",
        "lineFrom",
        "indexFrom",
        "lineTo",
        "indexTo",
        "setSelectionBackgroundColor",
        "setSelectionForegroundColor",
        "setTabIndents",
        "setTabWidth",
        "setText",
        "setUtf8",
        "cp",
        "setWhitespaceVisibility",
        "WhitespaceVisibility",
        "setWrapMode",
        "WrapMode",
        "undo",
        "zoomIn",
        "range",
        "zoomOut",
        "zoomTo",
        "size",
        "handleCallTipClick",
        "dir",
        "handleCharAdded",
        "charadded",
        "handleIndicatorClick",
        "pos",
        "modifiers",
        "handleIndicatorRelease",
        "handleMarginClick",
        "handleMarginRightClick",
        "handleModified",
        "mtype",
        "const char*",
        "len",
        "added",
        "foldNow",
        "foldPrev",
        "token",
        "annotationLinesAdded",
        "handlePropertyChange",
        "prop",
        "val",
        "handleSavePointReached",
        "handleSavePointLeft",
        "handleSelectionChanged",
        "handleAutoCompletionSelection",
        "handleUserListSelection",
        "handleStyleColorChange",
        "style",
        "handleStyleEolFillChange",
        "eolfill",
        "handleStyleFontChange",
        "handleStylePaperChange",
        "handleUpdateUI",
        "updated",
        "delete_selection"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'cursorPositionChanged'
        QtMocHelpers::SignalData<void(int, int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::Int, 4 },
        }}),
        // Signal 'copyAvailable'
        QtMocHelpers::SignalData<void(bool)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Signal 'indicatorClicked'
        QtMocHelpers::SignalData<void(int, int, Qt::KeyboardModifiers)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::Int, 4 }, { 0x80000000 | 8, 9 },
        }}),
        // Signal 'indicatorReleased'
        QtMocHelpers::SignalData<void(int, int, Qt::KeyboardModifiers)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::Int, 4 }, { 0x80000000 | 8, 9 },
        }}),
        // Signal 'linesChanged'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'marginClicked'
        QtMocHelpers::SignalData<void(int, int, Qt::KeyboardModifiers)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 }, { QMetaType::Int, 3 }, { 0x80000000 | 8, 9 },
        }}),
        // Signal 'marginRightClicked'
        QtMocHelpers::SignalData<void(int, int, Qt::KeyboardModifiers)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 }, { QMetaType::Int, 3 }, { 0x80000000 | 8, 9 },
        }}),
        // Signal 'modificationAttempted'
        QtMocHelpers::SignalData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'modificationChanged'
        QtMocHelpers::SignalData<void(bool)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 17 },
        }}),
        // Signal 'selectionChanged'
        QtMocHelpers::SignalData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'textChanged'
        QtMocHelpers::SignalData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'userListActivated'
        QtMocHelpers::SignalData<void(int, const QString &)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 21 }, { QMetaType::QString, 22 },
        }}),
        // Slot 'append'
        QtMocHelpers::SlotData<void(const QString &)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 24 },
        }}),
        // Slot 'autoCompleteFromAll'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'autoCompleteFromAPIs'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'autoCompleteFromDocument'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'callTip'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'clear'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'copy'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'cut'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'ensureCursorVisible'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'ensureLineVisible'
        QtMocHelpers::SlotData<void(int)>(33, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'foldAll'
        QtMocHelpers::SlotData<void(bool)>(34, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 35 },
        }}),
        // Slot 'foldAll'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void),
        // Slot 'foldLine'
        QtMocHelpers::SlotData<void(int)>(36, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'indent'
        QtMocHelpers::SlotData<void(int)>(37, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'insert'
        QtMocHelpers::SlotData<void(const QString &)>(38, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 24 },
        }}),
        // Slot 'insertAt'
        QtMocHelpers::SlotData<void(const QString &, int, int)>(39, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 24 }, { QMetaType::Int, 3 }, { QMetaType::Int, 4 },
        }}),
        // Slot 'moveToMatchingBrace'
        QtMocHelpers::SlotData<void()>(40, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'paste'
        QtMocHelpers::SlotData<void()>(41, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'redo'
        QtMocHelpers::SlotData<void()>(42, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'removeSelectedText'
        QtMocHelpers::SlotData<void()>(43, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'replaceSelectedText'
        QtMocHelpers::SlotData<void(const QString &)>(44, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 24 },
        }}),
        // Slot 'resetSelectionBackgroundColor'
        QtMocHelpers::SlotData<void()>(45, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'resetSelectionForegroundColor'
        QtMocHelpers::SlotData<void()>(46, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'selectAll'
        QtMocHelpers::SlotData<void(bool)>(47, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 48 },
        }}),
        // Slot 'selectAll'
        QtMocHelpers::SlotData<void()>(47, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void),
        // Slot 'selectToMatchingBrace'
        QtMocHelpers::SlotData<void()>(49, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setAutoCompletionCaseSensitivity'
        QtMocHelpers::SlotData<void(bool)>(50, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 51 },
        }}),
        // Slot 'setAutoCompletionReplaceWord'
        QtMocHelpers::SlotData<void(bool)>(52, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 53 },
        }}),
        // Slot 'setAutoCompletionShowSingle'
        QtMocHelpers::SlotData<void(bool)>(54, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 55 },
        }}),
        // Slot 'setAutoCompletionSource'
        QtMocHelpers::SlotData<void(enum AutoCompletionSource)>(56, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 57, 58 },
        }}),
        // Slot 'setAutoCompletionThreshold'
        QtMocHelpers::SlotData<void(int)>(59, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 60 },
        }}),
        // Slot 'setAutoCompletionUseSingle'
        QtMocHelpers::SlotData<void(enum AutoCompletionUseSingle)>(61, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 62, 55 },
        }}),
        // Slot 'setAutoIndent'
        QtMocHelpers::SlotData<void(bool)>(63, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 64 },
        }}),
        // Slot 'setBraceMatching'
        QtMocHelpers::SlotData<void(enum BraceMatch)>(65, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 66, 67 },
        }}),
        // Slot 'setBackspaceUnindents'
        QtMocHelpers::SlotData<void(bool)>(68, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 69 },
        }}),
        // Slot 'setCaretForegroundColor'
        QtMocHelpers::SlotData<void(const QColor &)>(70, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 71, 72 },
        }}),
        // Slot 'setCaretLineBackgroundColor'
        QtMocHelpers::SlotData<void(const QColor &)>(73, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 71, 72 },
        }}),
        // Slot 'setCaretLineFrameWidth'
        QtMocHelpers::SlotData<void(int)>(74, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 75 },
        }}),
        // Slot 'setCaretLineVisible'
        QtMocHelpers::SlotData<void(bool)>(76, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 77 },
        }}),
        // Slot 'setCaretWidth'
        QtMocHelpers::SlotData<void(int)>(78, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 75 },
        }}),
        // Slot 'setColor'
        QtMocHelpers::SlotData<void(const QColor &)>(79, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 71, 80 },
        }}),
        // Slot 'setCursorPosition'
        QtMocHelpers::SlotData<void(int, int)>(81, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::Int, 4 },
        }}),
        // Slot 'setEolMode'
        QtMocHelpers::SlotData<void(enum EolMode)>(82, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 83, 84 },
        }}),
        // Slot 'setEolVisibility'
        QtMocHelpers::SlotData<void(bool)>(85, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 86 },
        }}),
        // Slot 'setFolding'
        QtMocHelpers::SlotData<void(enum FoldStyle, int)>(87, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 88, 89 }, { QMetaType::Int, 13 },
        }}),
        // Slot 'setFolding'
        QtMocHelpers::SlotData<void(enum FoldStyle)>(87, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { 0x80000000 | 88, 89 },
        }}),
        // Slot 'setIndentation'
        QtMocHelpers::SlotData<void(int, int)>(90, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::Int, 91 },
        }}),
        // Slot 'setIndentationGuides'
        QtMocHelpers::SlotData<void(bool)>(92, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 77 },
        }}),
        // Slot 'setIndentationGuidesBackgroundColor'
        QtMocHelpers::SlotData<void(const QColor &)>(93, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 71, 72 },
        }}),
        // Slot 'setIndentationGuidesForegroundColor'
        QtMocHelpers::SlotData<void(const QColor &)>(94, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 71, 72 },
        }}),
        // Slot 'setIndentationsUseTabs'
        QtMocHelpers::SlotData<void(bool)>(95, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 96 },
        }}),
        // Slot 'setIndentationWidth'
        QtMocHelpers::SlotData<void(int)>(97, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 75 },
        }}),
        // Slot 'setLexer'
        QtMocHelpers::SlotData<void(QsciLexer *)>(98, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 99, 100 },
        }}),
        // Slot 'setLexer'
        QtMocHelpers::SlotData<void()>(98, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void),
        // Slot 'setMarginsBackgroundColor'
        QtMocHelpers::SlotData<void(const QColor &)>(101, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 71, 72 },
        }}),
        // Slot 'setMarginsFont'
        QtMocHelpers::SlotData<void(const QFont &)>(102, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 103, 104 },
        }}),
        // Slot 'setMarginsForegroundColor'
        QtMocHelpers::SlotData<void(const QColor &)>(105, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 71, 72 },
        }}),
        // Slot 'setMarginLineNumbers'
        QtMocHelpers::SlotData<void(int, bool)>(106, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 }, { QMetaType::Bool, 107 },
        }}),
        // Slot 'setMarginMarkerMask'
        QtMocHelpers::SlotData<void(int, int)>(108, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 }, { QMetaType::Int, 109 },
        }}),
        // Slot 'setMarginSensitivity'
        QtMocHelpers::SlotData<void(int, bool)>(110, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 }, { QMetaType::Bool, 111 },
        }}),
        // Slot 'setMarginWidth'
        QtMocHelpers::SlotData<void(int, int)>(112, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 }, { QMetaType::Int, 75 },
        }}),
        // Slot 'setMarginWidth'
        QtMocHelpers::SlotData<void(int, const QString &)>(112, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 }, { QMetaType::QString, 113 },
        }}),
        // Slot 'setModified'
        QtMocHelpers::SlotData<void(bool)>(114, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 17 },
        }}),
        // Slot 'setPaper'
        QtMocHelpers::SlotData<void(const QColor &)>(115, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 71, 80 },
        }}),
        // Slot 'setReadOnly'
        QtMocHelpers::SlotData<void(bool)>(116, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 117 },
        }}),
        // Slot 'setSelection'
        QtMocHelpers::SlotData<void(int, int, int, int)>(118, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 119 }, { QMetaType::Int, 120 }, { QMetaType::Int, 121 }, { QMetaType::Int, 122 },
        }}),
        // Slot 'setSelectionBackgroundColor'
        QtMocHelpers::SlotData<void(const QColor &)>(123, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 71, 72 },
        }}),
        // Slot 'setSelectionForegroundColor'
        QtMocHelpers::SlotData<void(const QColor &)>(124, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 71, 72 },
        }}),
        // Slot 'setTabIndents'
        QtMocHelpers::SlotData<void(bool)>(125, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 37 },
        }}),
        // Slot 'setTabWidth'
        QtMocHelpers::SlotData<void(int)>(126, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 75 },
        }}),
        // Slot 'setText'
        QtMocHelpers::SlotData<void(const QString &)>(127, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 24 },
        }}),
        // Slot 'setUtf8'
        QtMocHelpers::SlotData<void(bool)>(128, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 129 },
        }}),
        // Slot 'setWhitespaceVisibility'
        QtMocHelpers::SlotData<void(enum WhitespaceVisibility)>(130, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 131, 84 },
        }}),
        // Slot 'setWrapMode'
        QtMocHelpers::SlotData<void(enum WrapMode)>(132, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 133, 84 },
        }}),
        // Slot 'undo'
        QtMocHelpers::SlotData<void()>(134, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'unindent'
        QtMocHelpers::SlotData<void(int)>(69, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'zoomIn'
        QtMocHelpers::SlotData<void(int)>(135, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 136 },
        }}),
        // Slot 'zoomIn'
        QtMocHelpers::SlotData<void()>(135, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomOut'
        QtMocHelpers::SlotData<void(int)>(137, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 136 },
        }}),
        // Slot 'zoomOut'
        QtMocHelpers::SlotData<void()>(137, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'zoomTo'
        QtMocHelpers::SlotData<void(int)>(138, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 139 },
        }}),
        // Slot 'handleCallTipClick'
        QtMocHelpers::SlotData<void(int)>(140, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 141 },
        }}),
        // Slot 'handleCharAdded'
        QtMocHelpers::SlotData<void(int)>(142, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 143 },
        }}),
        // Slot 'handleIndicatorClick'
        QtMocHelpers::SlotData<void(int, int)>(144, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 145 }, { QMetaType::Int, 146 },
        }}),
        // Slot 'handleIndicatorRelease'
        QtMocHelpers::SlotData<void(int, int)>(147, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 145 }, { QMetaType::Int, 146 },
        }}),
        // Slot 'handleMarginClick'
        QtMocHelpers::SlotData<void(int, int, int)>(148, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 145 }, { QMetaType::Int, 13 }, { QMetaType::Int, 146 },
        }}),
        // Slot 'handleMarginRightClick'
        QtMocHelpers::SlotData<void(int, int, int)>(149, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 145 }, { QMetaType::Int, 13 }, { QMetaType::Int, 146 },
        }}),
        // Slot 'handleModified'
        QtMocHelpers::SlotData<void(int, int, const char *, int, int, int, int, int, int, int)>(150, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 145 }, { QMetaType::Int, 151 }, { 0x80000000 | 152, 24 }, { QMetaType::Int, 153 },
            { QMetaType::Int, 154 }, { QMetaType::Int, 3 }, { QMetaType::Int, 155 }, { QMetaType::Int, 156 },
            { QMetaType::Int, 157 }, { QMetaType::Int, 158 },
        }}),
        // Slot 'handlePropertyChange'
        QtMocHelpers::SlotData<void(const char *, const char *)>(159, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 152, 160 }, { 0x80000000 | 152, 161 },
        }}),
        // Slot 'handleSavePointReached'
        QtMocHelpers::SlotData<void()>(162, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleSavePointLeft'
        QtMocHelpers::SlotData<void()>(163, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleSelectionChanged'
        QtMocHelpers::SlotData<void(bool)>(164, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Slot 'handleAutoCompletionSelection'
        QtMocHelpers::SlotData<void()>(165, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleUserListSelection'
        QtMocHelpers::SlotData<void(const char *, int)>(166, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 152, 24 }, { QMetaType::Int, 21 },
        }}),
        // Slot 'handleStyleColorChange'
        QtMocHelpers::SlotData<void(const QColor &, int)>(167, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 71, 80 }, { QMetaType::Int, 168 },
        }}),
        // Slot 'handleStyleEolFillChange'
        QtMocHelpers::SlotData<void(bool, int)>(169, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 170 }, { QMetaType::Int, 168 },
        }}),
        // Slot 'handleStyleFontChange'
        QtMocHelpers::SlotData<void(const QFont &, int)>(171, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 103, 104 }, { QMetaType::Int, 168 },
        }}),
        // Slot 'handleStylePaperChange'
        QtMocHelpers::SlotData<void(const QColor &, int)>(172, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 71, 80 }, { QMetaType::Int, 168 },
        }}),
        // Slot 'handleUpdateUI'
        QtMocHelpers::SlotData<void(int)>(173, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 174 },
        }}),
        // Slot 'delete_selection'
        QtMocHelpers::SlotData<void()>(175, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<QsciScintilla, qt_meta_tag_ZN13QsciScintillaE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject QsciScintilla::staticMetaObject = { {
    QMetaObject::SuperData::link<QsciScintillaBase::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13QsciScintillaE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13QsciScintillaE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13QsciScintillaE_t>.metaTypes,
    nullptr
} };

void QsciScintilla::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<QsciScintilla *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->cursorPositionChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 1: _t->copyAvailable((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 2: _t->indicatorClicked((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<Qt::KeyboardModifiers>>(_a[3]))); break;
        case 3: _t->indicatorReleased((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<Qt::KeyboardModifiers>>(_a[3]))); break;
        case 4: _t->linesChanged(); break;
        case 5: _t->marginClicked((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<Qt::KeyboardModifiers>>(_a[3]))); break;
        case 6: _t->marginRightClicked((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<Qt::KeyboardModifiers>>(_a[3]))); break;
        case 7: _t->modificationAttempted(); break;
        case 8: _t->modificationChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 9: _t->selectionChanged(); break;
        case 10: _t->textChanged(); break;
        case 11: _t->userListActivated((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 12: _t->append((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->autoCompleteFromAll(); break;
        case 14: _t->autoCompleteFromAPIs(); break;
        case 15: _t->autoCompleteFromDocument(); break;
        case 16: _t->callTip(); break;
        case 17: _t->clear(); break;
        case 18: _t->copy(); break;
        case 19: _t->cut(); break;
        case 20: _t->ensureCursorVisible(); break;
        case 21: _t->ensureLineVisible((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 22: _t->foldAll((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 23: _t->foldAll(); break;
        case 24: _t->foldLine((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 25: _t->indent((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 26: _t->insert((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 27: _t->insertAt((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 28: _t->moveToMatchingBrace(); break;
        case 29: _t->paste(); break;
        case 30: _t->redo(); break;
        case 31: _t->removeSelectedText(); break;
        case 32: _t->replaceSelectedText((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 33: _t->resetSelectionBackgroundColor(); break;
        case 34: _t->resetSelectionForegroundColor(); break;
        case 35: _t->selectAll((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 36: _t->selectAll(); break;
        case 37: _t->selectToMatchingBrace(); break;
        case 38: _t->setAutoCompletionCaseSensitivity((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 39: _t->setAutoCompletionReplaceWord((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 40: _t->setAutoCompletionShowSingle((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 41: _t->setAutoCompletionSource((*reinterpret_cast<std::add_pointer_t<enum AutoCompletionSource>>(_a[1]))); break;
        case 42: _t->setAutoCompletionThreshold((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 43: _t->setAutoCompletionUseSingle((*reinterpret_cast<std::add_pointer_t<enum AutoCompletionUseSingle>>(_a[1]))); break;
        case 44: _t->setAutoIndent((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 45: _t->setBraceMatching((*reinterpret_cast<std::add_pointer_t<enum BraceMatch>>(_a[1]))); break;
        case 46: _t->setBackspaceUnindents((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 47: _t->setCaretForegroundColor((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 48: _t->setCaretLineBackgroundColor((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 49: _t->setCaretLineFrameWidth((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 50: _t->setCaretLineVisible((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 51: _t->setCaretWidth((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 52: _t->setColor((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 53: _t->setCursorPosition((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 54: _t->setEolMode((*reinterpret_cast<std::add_pointer_t<enum EolMode>>(_a[1]))); break;
        case 55: _t->setEolVisibility((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 56: _t->setFolding((*reinterpret_cast<std::add_pointer_t<enum FoldStyle>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 57: _t->setFolding((*reinterpret_cast<std::add_pointer_t<enum FoldStyle>>(_a[1]))); break;
        case 58: _t->setIndentation((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 59: _t->setIndentationGuides((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 60: _t->setIndentationGuidesBackgroundColor((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 61: _t->setIndentationGuidesForegroundColor((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 62: _t->setIndentationsUseTabs((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 63: _t->setIndentationWidth((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 64: _t->setLexer((*reinterpret_cast<std::add_pointer_t<QsciLexer*>>(_a[1]))); break;
        case 65: _t->setLexer(); break;
        case 66: _t->setMarginsBackgroundColor((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 67: _t->setMarginsFont((*reinterpret_cast<std::add_pointer_t<QFont>>(_a[1]))); break;
        case 68: _t->setMarginsForegroundColor((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 69: _t->setMarginLineNumbers((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 70: _t->setMarginMarkerMask((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 71: _t->setMarginSensitivity((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 72: _t->setMarginWidth((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 73: _t->setMarginWidth((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 74: _t->setModified((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 75: _t->setPaper((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 76: _t->setReadOnly((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 77: _t->setSelection((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[4]))); break;
        case 78: _t->setSelectionBackgroundColor((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 79: _t->setSelectionForegroundColor((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 80: _t->setTabIndents((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 81: _t->setTabWidth((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 82: _t->setText((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 83: _t->setUtf8((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 84: _t->setWhitespaceVisibility((*reinterpret_cast<std::add_pointer_t<enum WhitespaceVisibility>>(_a[1]))); break;
        case 85: _t->setWrapMode((*reinterpret_cast<std::add_pointer_t<enum WrapMode>>(_a[1]))); break;
        case 86: _t->undo(); break;
        case 87: _t->unindent((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 88: _t->zoomIn((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 89: _t->zoomIn(); break;
        case 90: _t->zoomOut((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 91: _t->zoomOut(); break;
        case 92: _t->zoomTo((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 93: _t->handleCallTipClick((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 94: _t->handleCharAdded((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 95: _t->handleIndicatorClick((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 96: _t->handleIndicatorRelease((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 97: _t->handleMarginClick((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 98: _t->handleMarginRightClick((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 99: _t->handleModified((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<const char*>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[5])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[6])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[7])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[8])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[9])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[10]))); break;
        case 100: _t->handlePropertyChange((*reinterpret_cast<std::add_pointer_t<const char*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<const char*>>(_a[2]))); break;
        case 101: _t->handleSavePointReached(); break;
        case 102: _t->handleSavePointLeft(); break;
        case 103: _t->handleSelectionChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 104: _t->handleAutoCompletionSelection(); break;
        case 105: _t->handleUserListSelection((*reinterpret_cast<std::add_pointer_t<const char*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 106: _t->handleStyleColorChange((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 107: _t->handleStyleEolFillChange((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 108: _t->handleStyleFontChange((*reinterpret_cast<std::add_pointer_t<QFont>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 109: _t->handleStylePaperChange((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 110: _t->handleUpdateUI((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 111: _t->delete_selection(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (QsciScintilla::*)(int , int )>(_a, &QsciScintilla::cursorPositionChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintilla::*)(bool )>(_a, &QsciScintilla::copyAvailable, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintilla::*)(int , int , Qt::KeyboardModifiers )>(_a, &QsciScintilla::indicatorClicked, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintilla::*)(int , int , Qt::KeyboardModifiers )>(_a, &QsciScintilla::indicatorReleased, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintilla::*)()>(_a, &QsciScintilla::linesChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintilla::*)(int , int , Qt::KeyboardModifiers )>(_a, &QsciScintilla::marginClicked, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintilla::*)(int , int , Qt::KeyboardModifiers )>(_a, &QsciScintilla::marginRightClicked, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintilla::*)()>(_a, &QsciScintilla::modificationAttempted, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintilla::*)(bool )>(_a, &QsciScintilla::modificationChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintilla::*)()>(_a, &QsciScintilla::selectionChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintilla::*)()>(_a, &QsciScintilla::textChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintilla::*)(int , const QString & )>(_a, &QsciScintilla::userListActivated, 11))
            return;
    }
}

const QMetaObject *QsciScintilla::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QsciScintilla::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13QsciScintillaE_t>.strings))
        return static_cast<void*>(this);
    return QsciScintillaBase::qt_metacast(_clname);
}

int QsciScintilla::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QsciScintillaBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 112)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 112;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 112)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 112;
    }
    return _id;
}

// SIGNAL 0
void QsciScintilla::cursorPositionChanged(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void QsciScintilla::copyAvailable(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void QsciScintilla::indicatorClicked(int _t1, int _t2, Qt::KeyboardModifiers _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2, _t3);
}

// SIGNAL 3
void QsciScintilla::indicatorReleased(int _t1, int _t2, Qt::KeyboardModifiers _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2, _t3);
}

// SIGNAL 4
void QsciScintilla::linesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void QsciScintilla::marginClicked(int _t1, int _t2, Qt::KeyboardModifiers _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2, _t3);
}

// SIGNAL 6
void QsciScintilla::marginRightClicked(int _t1, int _t2, Qt::KeyboardModifiers _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2, _t3);
}

// SIGNAL 7
void QsciScintilla::modificationAttempted()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void QsciScintilla::modificationChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void QsciScintilla::selectionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void QsciScintilla::textChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void QsciScintilla::userListActivated(int _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1, _t2);
}
QT_WARNING_POP
