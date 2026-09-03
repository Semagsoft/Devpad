/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "core/fileservice.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTemporaryFile>

#include <gtest/gtest.h>

TEST(FileService, DetectEncodingEmpty)
{
    EXPECT_EQ(FileService::detectEncoding({}), QStringLiteral("UTF-8"));
}

TEST(FileService, LoadNonExistent)
{
    auto res = FileService::load(QStringLiteral("/nonexistent/path/xyz_123.txt"));
    EXPECT_FALSE(res.ok);
    EXPECT_FALSE(res.error.isEmpty());
}

TEST(FileService, LoadAndSaveRoundtrip)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString path = dir.filePath(QStringLiteral("hello.txt"));
    QString original = QStringLiteral("Hello\nWorld\n");
    QString err;
    ASSERT_TRUE(FileService::save(path, original, QStringLiteral("UTF-8"), &err)) << err.toStdString();

    auto res = FileService::load(path);
    EXPECT_TRUE(res.ok);
    EXPECT_EQ(res.text, original);
    EXPECT_EQ(res.encoding, QStringLiteral("UTF-8"));
}

TEST(FileService, SaveWithBomUtf8)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString path = dir.filePath(QStringLiteral("bom.txt"));
    ASSERT_TRUE(FileService::save(path, QStringLiteral("hi"), QStringLiteral("UTF-8")));

    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    QByteArray raw = f.readAll();
    // Default save without BOM caller? Our save writes BOM only for encodings that have BOM mapping.
    // UTF-8 triggers BOM per bomForEncoding -> but we write BOM for UTF-8 in filemanager legacy.
    // For FileService we also write BOM for UTF-8.
    EXPECT_EQ(raw.left(3), QByteArray("\xEF\xBB\xBF", 3));
}

TEST(FileService, LoadTooLarge)
{
    // Just test the constant is set - not actually creating 100MB file
    EXPECT_EQ(FileService::MaxFileSize, 100LL * 1024 * 1024);
}
