/****************************************************************************
** Meta object code from reading C++ file 'qscilexer.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../build/_deps/qscintilla/src/ep_qscintilla/src/Qsci/qscilexer.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qscilexer.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9QsciLexerE_t {};
} // unnamed namespace

template <> constexpr inline auto QsciLexer::qt_create_metaobjectdata<qt_meta_tag_ZN9QsciLexerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "QsciLexer",
        "colorChanged",
        "",
        "QColor",
        "c",
        "style",
        "eolFillChanged",
        "eolfilled",
        "fontChanged",
        "QFont",
        "f",
        "paperChanged",
        "propertyChanged",
        "const char*",
        "prop",
        "val",
        "setAutoIndentStyle",
        "autoindentstyle",
        "setColor",
        "setEolFill",
        "eoffill",
        "setFont",
        "setPaper"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'colorChanged'
        QtMocHelpers::SignalData<void(const QColor &, int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { QMetaType::Int, 5 },
        }}),
        // Signal 'eolFillChanged'
        QtMocHelpers::SignalData<void(bool, int)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 7 }, { QMetaType::Int, 5 },
        }}),
        // Signal 'fontChanged'
        QtMocHelpers::SignalData<void(const QFont &, int)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 10 }, { QMetaType::Int, 5 },
        }}),
        // Signal 'paperChanged'
        QtMocHelpers::SignalData<void(const QColor &, int)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { QMetaType::Int, 5 },
        }}),
        // Signal 'propertyChanged'
        QtMocHelpers::SignalData<void(const char *, const char *)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 13, 14 }, { 0x80000000 | 13, 15 },
        }}),
        // Slot 'setAutoIndentStyle'
        QtMocHelpers::SlotData<void(int)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 17 },
        }}),
        // Slot 'setColor'
        QtMocHelpers::SlotData<void(const QColor &, int)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { QMetaType::Int, 5 },
        }}),
        // Slot 'setColor'
        QtMocHelpers::SlotData<void(const QColor &)>(18, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Slot 'setEolFill'
        QtMocHelpers::SlotData<void(bool, int)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 20 }, { QMetaType::Int, 5 },
        }}),
        // Slot 'setEolFill'
        QtMocHelpers::SlotData<void(bool)>(19, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::Bool, 20 },
        }}),
        // Slot 'setFont'
        QtMocHelpers::SlotData<void(const QFont &, int)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 10 }, { QMetaType::Int, 5 },
        }}),
        // Slot 'setFont'
        QtMocHelpers::SlotData<void(const QFont &)>(21, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { 0x80000000 | 9, 10 },
        }}),
        // Slot 'setPaper'
        QtMocHelpers::SlotData<void(const QColor &, int)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { QMetaType::Int, 5 },
        }}),
        // Slot 'setPaper'
        QtMocHelpers::SlotData<void(const QColor &)>(22, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<QsciLexer, qt_meta_tag_ZN9QsciLexerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject QsciLexer::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9QsciLexerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9QsciLexerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9QsciLexerE_t>.metaTypes,
    nullptr
} };

void QsciLexer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<QsciLexer *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->colorChanged((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 1: _t->eolFillChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 2: _t->fontChanged((*reinterpret_cast<std::add_pointer_t<QFont>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 3: _t->paperChanged((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 4: _t->propertyChanged((*reinterpret_cast<std::add_pointer_t<const char*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<const char*>>(_a[2]))); break;
        case 5: _t->setAutoIndentStyle((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->setColor((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 7: _t->setColor((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 8: _t->setEolFill((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 9: _t->setEolFill((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 10: _t->setFont((*reinterpret_cast<std::add_pointer_t<QFont>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 11: _t->setFont((*reinterpret_cast<std::add_pointer_t<QFont>>(_a[1]))); break;
        case 12: _t->setPaper((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 13: _t->setPaper((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (QsciLexer::*)(const QColor & , int )>(_a, &QsciLexer::colorChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciLexer::*)(bool , int )>(_a, &QsciLexer::eolFillChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciLexer::*)(const QFont & , int )>(_a, &QsciLexer::fontChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciLexer::*)(const QColor & , int )>(_a, &QsciLexer::paperChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (QsciLexer::*)(const char * , const char * )>(_a, &QsciLexer::propertyChanged, 4))
            return;
    }
}

const QMetaObject *QsciLexer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QsciLexer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9QsciLexerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int QsciLexer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void QsciLexer::colorChanged(const QColor & _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void QsciLexer::eolFillChanged(bool _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void QsciLexer::fontChanged(const QFont & _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void QsciLexer::paperChanged(const QColor & _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void QsciLexer::propertyChanged(const char * _t1, const char * _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}
QT_WARNING_POP
