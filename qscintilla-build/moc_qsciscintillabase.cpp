/****************************************************************************
** Meta object code from reading C++ file 'qsciscintillabase.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../build/_deps/qscintilla/src/ep_qscintilla/src/Qsci/qsciscintillabase.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qsciscintillabase.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN17QsciScintillaBaseE_t {};
} // unnamed namespace

template <> constexpr inline auto QsciScintillaBase::qt_create_metaobjectdata<qt_meta_tag_ZN17QsciScintillaBaseE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "QsciScintillaBase",
        "QSCN_SELCHANGED",
        "",
        "yes",
        "SCN_AUTOCCANCELLED",
        "SCN_AUTOCCHARDELETED",
        "SCN_AUTOCCOMPLETED",
        "const char*",
        "selection",
        "position",
        "ch",
        "method",
        "SCN_AUTOCSELECTION",
        "SCN_AUTOCSELECTIONCHANGE",
        "id",
        "SCEN_CHANGE",
        "SCN_CALLTIPCLICK",
        "direction",
        "SCN_CHARADDED",
        "charadded",
        "SCN_DOUBLECLICK",
        "line",
        "modifiers",
        "SCN_DWELLEND",
        "x",
        "y",
        "SCN_DWELLSTART",
        "SCN_FOCUSIN",
        "SCN_FOCUSOUT",
        "SCN_HOTSPOTCLICK",
        "SCN_HOTSPOTDOUBLECLICK",
        "SCN_HOTSPOTRELEASECLICK",
        "SCN_INDICATORCLICK",
        "SCN_INDICATORRELEASE",
        "SCN_MACRORECORD",
        "SCN_MARGINCLICK",
        "margin",
        "SCN_MARGINRIGHTCLICK",
        "SCN_MODIFIED",
        "SCN_MODIFYATTEMPTRO",
        "SCN_NEEDSHOWN",
        "SCN_PAINTED",
        "SCN_SAVEPOINTLEFT",
        "SCN_SAVEPOINTREACHED",
        "SCN_STYLENEEDED",
        "SCN_URIDROPPED",
        "QUrl",
        "url",
        "SCN_UPDATEUI",
        "updated",
        "SCN_USERLISTSELECTION",
        "SCN_ZOOM",
        "handleVSb",
        "value",
        "handleHSb"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'QSCN_SELCHANGED'
        QtMocHelpers::SignalData<void(bool)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 3 },
        }}),
        // Signal 'SCN_AUTOCCANCELLED'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'SCN_AUTOCCHARDELETED'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'SCN_AUTOCCOMPLETED'
        QtMocHelpers::SignalData<void(const char *, int, int, int)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 }, { QMetaType::Int, 9 }, { QMetaType::Int, 10 }, { QMetaType::Int, 11 },
        }}),
        // Signal 'SCN_AUTOCSELECTION'
        QtMocHelpers::SignalData<void(const char *, int, int, int)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 }, { QMetaType::Int, 9 }, { QMetaType::Int, 10 }, { QMetaType::Int, 11 },
        }}),
        // Signal 'SCN_AUTOCSELECTION'
        QtMocHelpers::SignalData<void(const char *, int)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 }, { QMetaType::Int, 9 },
        }}),
        // Signal 'SCN_AUTOCSELECTIONCHANGE'
        QtMocHelpers::SignalData<void(const char *, int, int)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 }, { QMetaType::Int, 14 }, { QMetaType::Int, 9 },
        }}),
        // Signal 'SCEN_CHANGE'
        QtMocHelpers::SignalData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'SCN_CALLTIPCLICK'
        QtMocHelpers::SignalData<void(int)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 17 },
        }}),
        // Signal 'SCN_CHARADDED'
        QtMocHelpers::SignalData<void(int)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 19 },
        }}),
        // Signal 'SCN_DOUBLECLICK'
        QtMocHelpers::SignalData<void(int, int, int)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::Int, 21 }, { QMetaType::Int, 22 },
        }}),
        // Signal 'SCN_DWELLEND'
        QtMocHelpers::SignalData<void(int, int, int)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::Int, 24 }, { QMetaType::Int, 25 },
        }}),
        // Signal 'SCN_DWELLSTART'
        QtMocHelpers::SignalData<void(int, int, int)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::Int, 24 }, { QMetaType::Int, 25 },
        }}),
        // Signal 'SCN_FOCUSIN'
        QtMocHelpers::SignalData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'SCN_FOCUSOUT'
        QtMocHelpers::SignalData<void()>(28, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'SCN_HOTSPOTCLICK'
        QtMocHelpers::SignalData<void(int, int)>(29, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::Int, 22 },
        }}),
        // Signal 'SCN_HOTSPOTDOUBLECLICK'
        QtMocHelpers::SignalData<void(int, int)>(30, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::Int, 22 },
        }}),
        // Signal 'SCN_HOTSPOTRELEASECLICK'
        QtMocHelpers::SignalData<void(int, int)>(31, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::Int, 22 },
        }}),
        // Signal 'SCN_INDICATORCLICK'
        QtMocHelpers::SignalData<void(int, int)>(32, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::Int, 22 },
        }}),
        // Signal 'SCN_INDICATORRELEASE'
        QtMocHelpers::SignalData<void(int, int)>(33, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::Int, 22 },
        }}),
        // Signal 'SCN_MACRORECORD'
        QtMocHelpers::SignalData<void(unsigned int, unsigned long, void *)>(34, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 2 }, { QMetaType::ULong, 2 }, { QMetaType::VoidStar, 2 },
        }}),
        // Signal 'SCN_MARGINCLICK'
        QtMocHelpers::SignalData<void(int, int, int)>(35, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::Int, 22 }, { QMetaType::Int, 36 },
        }}),
        // Signal 'SCN_MARGINRIGHTCLICK'
        QtMocHelpers::SignalData<void(int, int, int)>(37, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::Int, 22 }, { QMetaType::Int, 36 },
        }}),
        // Signal 'SCN_MODIFIED'
        QtMocHelpers::SignalData<void(int, int, const char *, int, int, int, int, int, int, int)>(38, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Int, 2 }, { 0x80000000 | 7, 2 }, { QMetaType::Int, 2 },
            { QMetaType::Int, 2 }, { QMetaType::Int, 2 }, { QMetaType::Int, 2 }, { QMetaType::Int, 2 },
            { QMetaType::Int, 2 }, { QMetaType::Int, 2 },
        }}),
        // Signal 'SCN_MODIFYATTEMPTRO'
        QtMocHelpers::SignalData<void()>(39, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'SCN_NEEDSHOWN'
        QtMocHelpers::SignalData<void(int, int)>(40, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Int, 2 },
        }}),
        // Signal 'SCN_PAINTED'
        QtMocHelpers::SignalData<void()>(41, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'SCN_SAVEPOINTLEFT'
        QtMocHelpers::SignalData<void()>(42, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'SCN_SAVEPOINTREACHED'
        QtMocHelpers::SignalData<void()>(43, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'SCN_STYLENEEDED'
        QtMocHelpers::SignalData<void(int)>(44, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 },
        }}),
        // Signal 'SCN_URIDROPPED'
        QtMocHelpers::SignalData<void(const QUrl &)>(45, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 46, 47 },
        }}),
        // Signal 'SCN_UPDATEUI'
        QtMocHelpers::SignalData<void(int)>(48, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 49 },
        }}),
        // Signal 'SCN_USERLISTSELECTION'
        QtMocHelpers::SignalData<void(const char *, int, int, int, int)>(50, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 }, { QMetaType::Int, 14 }, { QMetaType::Int, 10 }, { QMetaType::Int, 11 },
            { QMetaType::Int, 9 },
        }}),
        // Signal 'SCN_USERLISTSELECTION'
        QtMocHelpers::SignalData<void(const char *, int, int, int)>(50, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 }, { QMetaType::Int, 14 }, { QMetaType::Int, 10 }, { QMetaType::Int, 11 },
        }}),
        // Signal 'SCN_USERLISTSELECTION'
        QtMocHelpers::SignalData<void(const char *, int)>(50, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 }, { QMetaType::Int, 14 },
        }}),
        // Signal 'SCN_ZOOM'
        QtMocHelpers::SignalData<void()>(51, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'handleVSb'
        QtMocHelpers::SlotData<void(int)>(52, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 53 },
        }}),
        // Slot 'handleHSb'
        QtMocHelpers::SlotData<void(int)>(54, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 53 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<QsciScintillaBase, qt_meta_tag_ZN17QsciScintillaBaseE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject QsciScintillaBase::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractScrollArea::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17QsciScintillaBaseE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17QsciScintillaBaseE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN17QsciScintillaBaseE_t>.metaTypes,
    nullptr
} };

void QsciScintillaBase::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<QsciScintillaBase *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->QSCN_SELCHANGED((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->SCN_AUTOCCANCELLED(); break;
        case 2: _t->SCN_AUTOCCHARDELETED(); break;
        case 3: _t->SCN_AUTOCCOMPLETED((*reinterpret_cast<std::add_pointer_t<const char*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[4]))); break;
        case 4: _t->SCN_AUTOCSELECTION((*reinterpret_cast<std::add_pointer_t<const char*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[4]))); break;
        case 5: _t->SCN_AUTOCSELECTION((*reinterpret_cast<std::add_pointer_t<const char*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 6: _t->SCN_AUTOCSELECTIONCHANGE((*reinterpret_cast<std::add_pointer_t<const char*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 7: _t->SCEN_CHANGE(); break;
        case 8: _t->SCN_CALLTIPCLICK((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->SCN_CHARADDED((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 10: _t->SCN_DOUBLECLICK((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 11: _t->SCN_DWELLEND((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 12: _t->SCN_DWELLSTART((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 13: _t->SCN_FOCUSIN(); break;
        case 14: _t->SCN_FOCUSOUT(); break;
        case 15: _t->SCN_HOTSPOTCLICK((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 16: _t->SCN_HOTSPOTDOUBLECLICK((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 17: _t->SCN_HOTSPOTRELEASECLICK((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 18: _t->SCN_INDICATORCLICK((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 19: _t->SCN_INDICATORRELEASE((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 20: _t->SCN_MACRORECORD((*reinterpret_cast<std::add_pointer_t<uint>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<ulong>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<void*>>(_a[3]))); break;
        case 21: _t->SCN_MARGINCLICK((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 22: _t->SCN_MARGINRIGHTCLICK((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 23: _t->SCN_MODIFIED((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<const char*>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[5])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[6])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[7])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[8])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[9])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[10]))); break;
        case 24: _t->SCN_MODIFYATTEMPTRO(); break;
        case 25: _t->SCN_NEEDSHOWN((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 26: _t->SCN_PAINTED(); break;
        case 27: _t->SCN_SAVEPOINTLEFT(); break;
        case 28: _t->SCN_SAVEPOINTREACHED(); break;
        case 29: _t->SCN_STYLENEEDED((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 30: _t->SCN_URIDROPPED((*reinterpret_cast<std::add_pointer_t<QUrl>>(_a[1]))); break;
        case 31: _t->SCN_UPDATEUI((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 32: _t->SCN_USERLISTSELECTION((*reinterpret_cast<std::add_pointer_t<const char*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[5]))); break;
        case 33: _t->SCN_USERLISTSELECTION((*reinterpret_cast<std::add_pointer_t<const char*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[4]))); break;
        case 34: _t->SCN_USERLISTSELECTION((*reinterpret_cast<std::add_pointer_t<const char*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 35: _t->SCN_ZOOM(); break;
        case 36: _t->handleVSb((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 37: _t->handleHSb((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(bool )>(_a, &QsciScintillaBase::QSCN_SELCHANGED, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)()>(_a, &QsciScintillaBase::SCN_AUTOCCANCELLED, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)()>(_a, &QsciScintillaBase::SCN_AUTOCCHARDELETED, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(const char * , int , int , int )>(_a, &QsciScintillaBase::SCN_AUTOCCOMPLETED, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(const char * , int , int , int )>(_a, &QsciScintillaBase::SCN_AUTOCSELECTION, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(const char * , int )>(_a, &QsciScintillaBase::SCN_AUTOCSELECTION, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(const char * , int , int )>(_a, &QsciScintillaBase::SCN_AUTOCSELECTIONCHANGE, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)()>(_a, &QsciScintillaBase::SCEN_CHANGE, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int )>(_a, &QsciScintillaBase::SCN_CALLTIPCLICK, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int )>(_a, &QsciScintillaBase::SCN_CHARADDED, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int , int , int )>(_a, &QsciScintillaBase::SCN_DOUBLECLICK, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int , int , int )>(_a, &QsciScintillaBase::SCN_DWELLEND, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int , int , int )>(_a, &QsciScintillaBase::SCN_DWELLSTART, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)()>(_a, &QsciScintillaBase::SCN_FOCUSIN, 13))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)()>(_a, &QsciScintillaBase::SCN_FOCUSOUT, 14))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int , int )>(_a, &QsciScintillaBase::SCN_HOTSPOTCLICK, 15))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int , int )>(_a, &QsciScintillaBase::SCN_HOTSPOTDOUBLECLICK, 16))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int , int )>(_a, &QsciScintillaBase::SCN_HOTSPOTRELEASECLICK, 17))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int , int )>(_a, &QsciScintillaBase::SCN_INDICATORCLICK, 18))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int , int )>(_a, &QsciScintillaBase::SCN_INDICATORRELEASE, 19))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(unsigned int , unsigned long , void * )>(_a, &QsciScintillaBase::SCN_MACRORECORD, 20))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int , int , int )>(_a, &QsciScintillaBase::SCN_MARGINCLICK, 21))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int , int , int )>(_a, &QsciScintillaBase::SCN_MARGINRIGHTCLICK, 22))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int , int , const char * , int , int , int , int , int , int , int )>(_a, &QsciScintillaBase::SCN_MODIFIED, 23))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)()>(_a, &QsciScintillaBase::SCN_MODIFYATTEMPTRO, 24))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int , int )>(_a, &QsciScintillaBase::SCN_NEEDSHOWN, 25))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)()>(_a, &QsciScintillaBase::SCN_PAINTED, 26))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)()>(_a, &QsciScintillaBase::SCN_SAVEPOINTLEFT, 27))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)()>(_a, &QsciScintillaBase::SCN_SAVEPOINTREACHED, 28))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int )>(_a, &QsciScintillaBase::SCN_STYLENEEDED, 29))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(const QUrl & )>(_a, &QsciScintillaBase::SCN_URIDROPPED, 30))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(int )>(_a, &QsciScintillaBase::SCN_UPDATEUI, 31))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(const char * , int , int , int , int )>(_a, &QsciScintillaBase::SCN_USERLISTSELECTION, 32))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(const char * , int , int , int )>(_a, &QsciScintillaBase::SCN_USERLISTSELECTION, 33))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)(const char * , int )>(_a, &QsciScintillaBase::SCN_USERLISTSELECTION, 34))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciScintillaBase::*)()>(_a, &QsciScintillaBase::SCN_ZOOM, 35))
            return;
    }
}

const QMetaObject *QsciScintillaBase::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QsciScintillaBase::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17QsciScintillaBaseE_t>.strings))
        return static_cast<void*>(this);
    return QAbstractScrollArea::qt_metacast(_clname);
}

int QsciScintillaBase::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractScrollArea::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 38)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 38;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 38)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 38;
    }
    return _id;
}

// SIGNAL 0
void QsciScintillaBase::QSCN_SELCHANGED(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void QsciScintillaBase::SCN_AUTOCCANCELLED()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void QsciScintillaBase::SCN_AUTOCCHARDELETED()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void QsciScintillaBase::SCN_AUTOCCOMPLETED(const char * _t1, int _t2, int _t3, int _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 4
void QsciScintillaBase::SCN_AUTOCSELECTION(const char * _t1, int _t2, int _t3, int _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 5
void QsciScintillaBase::SCN_AUTOCSELECTION(const char * _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2);
}

// SIGNAL 6
void QsciScintillaBase::SCN_AUTOCSELECTIONCHANGE(const char * _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2, _t3);
}

// SIGNAL 7
void QsciScintillaBase::SCEN_CHANGE()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void QsciScintillaBase::SCN_CALLTIPCLICK(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void QsciScintillaBase::SCN_CHARADDED(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void QsciScintillaBase::SCN_DOUBLECLICK(int _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1, _t2, _t3);
}

// SIGNAL 11
void QsciScintillaBase::SCN_DWELLEND(int _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1, _t2, _t3);
}

// SIGNAL 12
void QsciScintillaBase::SCN_DWELLSTART(int _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1, _t2, _t3);
}

// SIGNAL 13
void QsciScintillaBase::SCN_FOCUSIN()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void QsciScintillaBase::SCN_FOCUSOUT()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void QsciScintillaBase::SCN_HOTSPOTCLICK(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 15, nullptr, _t1, _t2);
}

// SIGNAL 16
void QsciScintillaBase::SCN_HOTSPOTDOUBLECLICK(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 16, nullptr, _t1, _t2);
}

// SIGNAL 17
void QsciScintillaBase::SCN_HOTSPOTRELEASECLICK(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 17, nullptr, _t1, _t2);
}

// SIGNAL 18
void QsciScintillaBase::SCN_INDICATORCLICK(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 18, nullptr, _t1, _t2);
}

// SIGNAL 19
void QsciScintillaBase::SCN_INDICATORRELEASE(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 19, nullptr, _t1, _t2);
}

// SIGNAL 20
void QsciScintillaBase::SCN_MACRORECORD(unsigned int _t1, unsigned long _t2, void * _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 20, nullptr, _t1, _t2, _t3);
}

// SIGNAL 21
void QsciScintillaBase::SCN_MARGINCLICK(int _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 21, nullptr, _t1, _t2, _t3);
}

// SIGNAL 22
void QsciScintillaBase::SCN_MARGINRIGHTCLICK(int _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 22, nullptr, _t1, _t2, _t3);
}

// SIGNAL 23
void QsciScintillaBase::SCN_MODIFIED(int _t1, int _t2, const char * _t3, int _t4, int _t5, int _t6, int _t7, int _t8, int _t9, int _t10)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 23, nullptr, _t1, _t2, _t3, _t4, _t5, _t6, _t7, _t8, _t9, _t10);
}

// SIGNAL 24
void QsciScintillaBase::SCN_MODIFYATTEMPTRO()
{
    QMetaObject::activate(this, &staticMetaObject, 24, nullptr);
}

// SIGNAL 25
void QsciScintillaBase::SCN_NEEDSHOWN(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 25, nullptr, _t1, _t2);
}

// SIGNAL 26
void QsciScintillaBase::SCN_PAINTED()
{
    QMetaObject::activate(this, &staticMetaObject, 26, nullptr);
}

// SIGNAL 27
void QsciScintillaBase::SCN_SAVEPOINTLEFT()
{
    QMetaObject::activate(this, &staticMetaObject, 27, nullptr);
}

// SIGNAL 28
void QsciScintillaBase::SCN_SAVEPOINTREACHED()
{
    QMetaObject::activate(this, &staticMetaObject, 28, nullptr);
}

// SIGNAL 29
void QsciScintillaBase::SCN_STYLENEEDED(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 29, nullptr, _t1);
}

// SIGNAL 30
void QsciScintillaBase::SCN_URIDROPPED(const QUrl & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 30, nullptr, _t1);
}

// SIGNAL 31
void QsciScintillaBase::SCN_UPDATEUI(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 31, nullptr, _t1);
}

// SIGNAL 32
void QsciScintillaBase::SCN_USERLISTSELECTION(const char * _t1, int _t2, int _t3, int _t4, int _t5)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 32, nullptr, _t1, _t2, _t3, _t4, _t5);
}

// SIGNAL 33
void QsciScintillaBase::SCN_USERLISTSELECTION(const char * _t1, int _t2, int _t3, int _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 33, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 34
void QsciScintillaBase::SCN_USERLISTSELECTION(const char * _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 34, nullptr, _t1, _t2);
}

// SIGNAL 35
void QsciScintillaBase::SCN_ZOOM()
{
    QMetaObject::activate(this, &staticMetaObject, 35, nullptr);
}
QT_WARNING_POP
