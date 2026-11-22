/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * Shared Content-Length framing for line-protocol plugins (LSP and DAP both
 * use "Content-Length: N\r\n\r\n<body>"). Extracted so the framing code —
 * header scanning, oversized-message discarding, resync — lives in one place.
 *
 * Header-only; no QObject. Not thread-safe by itself.
 */

#ifndef CONTENTLENGTHFRAMER_H
#define CONTENTLENGTHFRAMER_H

#include <QByteArray>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <functional>

namespace plugins
{

class ContentLengthFramer
{
public:
    using MessageCallback = std::function<void(const QByteArray& message)>;

    // Feed bytes; invokes onMessage once per complete framed message.
    void handleData(const QByteArray& data, const MessageCallback& onMessage)
    {
        m_buffer.append(data);

        while (true)
        {
            if (m_contentLength < 0)
            {
                static constexpr int kMaxHeaderSize = 16 * 1024;
                int headerEnd = m_buffer.indexOf("\r\n\r\n");
                if (headerEnd < 0)
                {
                    if (m_buffer.size() > kMaxHeaderSize)
                    {
                        qWarning("ContentLengthFramer: header larger than %d bytes, dropping buffer contents to resync", kMaxHeaderSize);
                        m_buffer.clear();
                    }
                    return;
                }

                QByteArray header = m_buffer.left(headerEnd);
                qint64 parsed = -1;
                for (const QByteArray& line : header.split('\n'))
                {
                    QByteArray l = line.trimmed();
                    if (l.startsWith("Content-Length:"))
                    {
                        bool ok = false;
                        const qint64 value = l.mid(15).trimmed().toLongLong(&ok);
                        if (ok)
                            parsed = value;
                    }
                    // Content-Type is consumed but ignored.
                }

                if (parsed <= 0)
                {
                    // Invalid or unusable Content-Length. Resync at the next
                    // plausible header instead of only dropping the current
                    // one, whose body bytes would otherwise be parsed as a
                    // header and cause a cascade of mis-parsed messages.
                    qWarning("ContentLengthFramer: discarding frame with invalid Content-Length");
                    const int nextHeader = m_buffer.indexOf("Content-Length:", headerEnd + 4);
                    if (nextHeader < 0)
                        m_buffer.clear();
                    else
                        m_buffer.remove(0, nextHeader);
                    continue;
                }

                m_contentLength = parsed;
                m_buffer.remove(0, headerEnd + 4);
            }

            if (m_contentLength > kMaxMessageSize)
            {
                // Pathological Content-Length: discard as the bytes stream in
                // (never buffering them) and resync on the next message.
                const qint64 discardable = std::min<qint64>(m_buffer.size(), m_contentLength);
                if (discardable > 0)
                {
                    if (!m_discardingOversized)
                    {
                        m_discardingOversized = true;
                        qWarning("ContentLengthFramer: discarding message larger than %lld bytes", kMaxMessageSize);
                    }
                    m_buffer.remove(0, static_cast<int>(discardable));
                    m_contentLength -= discardable;
                }
                if (m_contentLength > 0)
                    return; // wait for the rest of the oversized message to stream through
                m_contentLength = -1;
                m_discardingOversized = false;
                continue;
            }

            if (m_buffer.size() < m_contentLength)
                return;

            QByteArray message = m_buffer.left(static_cast<int>(m_contentLength));
            m_buffer.remove(0, static_cast<int>(m_contentLength));
            m_contentLength = -1;
            m_discardingOversized = false;

            onMessage(message);
        }
    }

    // Frame a JSON object as one Content-Length message.
    QByteArray frame(const QJsonObject& obj) const
    {
        return frame(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }

    QByteArray frame(const QByteArray& json) const
    {
        QByteArray header =
            QString("Content-Length: %1\r\nContent-Type: application/vscode-jsonrpc; charset=utf-8\r\n\r\n").arg(json.size()).toUtf8();
        return header + json;
    }

private:
    // Ceiling on a single framed message; larger Content-Length values are
    // discarded incrementally instead of being buffered entire in memory.
    static constexpr qint64 kMaxMessageSize = 100LL * 1024 * 1024;

    QByteArray m_buffer;
    qint64 m_contentLength = -1;
    bool m_discardingOversized = false;
};

} // namespace plugins

#endif // CONTENTLENGTHFRAMER_H
