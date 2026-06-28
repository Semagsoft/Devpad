/****************************************************************************
** Meta object code from reading C++ file 'qscilexervhdl.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../build/_deps/qscintilla/src/ep_qscintilla/src/Qsci/qscilexervhdl.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qscilexervhdl.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN13QsciLexerVHDLE_t {};
} // unnamed namespace

template <> constexpr inline auto QsciLexerVHDL::qt_create_metaobjectdata<qt_meta_tag_ZN13QsciLexerVHDLE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "QsciLexerVHDL",
        "setFoldComments",
        "",
        "fold",
        "setFoldCompact",
        "setFoldAtElse",
        "setFoldAtBegin",
        "setFoldAtParenthesis"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'setFoldComments'
        QtMocHelpers::SlotData<void(bool)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 3 },
        }}),
        // Slot 'setFoldCompact'
        QtMocHelpers::SlotData<void(bool)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 3 },
        }}),
        // Slot 'setFoldAtElse'
        QtMocHelpers::SlotData<void(bool)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 3 },
        }}),
        // Slot 'setFoldAtBegin'
        QtMocHelpers::SlotData<void(bool)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 3 },
        }}),
        // Slot 'setFoldAtParenthesis'
        QtMocHelpers::SlotData<void(bool)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 3 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<QsciLexerVHDL, qt_meta_tag_ZN13QsciLexerVHDLE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject QsciLexerVHDL::staticMetaObject = { {
    QMetaObject::SuperData::link<QsciLexer::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13QsciLexerVHDLE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13QsciLexerVHDLE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13QsciLexerVHDLE_t>.metaTypes,
    nullptr
} };

void QsciLexerVHDL::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<QsciLexerVHDL *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->setFoldComments((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->setFoldCompact((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 2: _t->setFoldAtElse((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->setFoldAtBegin((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 4: _t->setFoldAtParenthesis((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *QsciLexerVHDL::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QsciLexerVHDL::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13QsciLexerVHDLE_t>.strings))
        return static_cast<void*>(this);
    return QsciLexer::qt_metacast(_clname);
}

int QsciLexerVHDL::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QsciLexer::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
