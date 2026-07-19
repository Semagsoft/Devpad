#include "inlinefindbar.h"

#include "codeeditor.h"
#include "settingsmanager.h"
#include "theme.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QStyle>
#include <QTimer>
#include <QToolButton>

#include <Qsci/qsciscintilla.h>
#include <Qsci/qsciscintillabase.h>

// ── Helpers ──────────────────────────────────────────────────

static QToolButton* createToolButton(const QIcon& icon, const QString& tooltip, bool checkable = false)
{
    auto* btn = new QToolButton;
    btn->setIcon(icon);
    btn->setToolTip(tooltip);
    btn->setCheckable(checkable);
    btn->setAutoRaise(true);
    btn->setFixedSize(28, 28);
    btn->setIconSize(QSize(20, 20));
    btn->setFocusPolicy(Qt::NoFocus);
    return btn;
}

// ── Painted icons ───────────────────────────────────────────

QIcon InlineFindBar::paintIcon(const QColor& fg, const QString& text, bool underline, bool brackets)
{
    QPixmap pix(24, 24);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    QFont f = p.font();
    f.setPixelSize(12);
    f.setBold(true);
    p.setFont(f);
    p.setPen(fg);

    if (brackets)
    {
        QString display = QStringLiteral("|%1|").arg(text);
        p.drawText(QRect(0, 0, 24, 24), Qt::AlignCenter, display);
    }
    else
    {
        p.drawText(QRect(0, 1, 24, 22), Qt::AlignCenter, text);
    }

    if (underline)
        p.drawLine(5, 19, 19, 19);

    p.end();
    return QIcon(pix);
}

// ── Constructor / Destructor ─────────────────────────────────

InlineFindBar::InlineFindBar(CodeEditor* editor, QWidget* parent) : QWidget(parent), m_editor(editor)
{
    setVisible(false);
    setFixedHeight(38);
    setupUI();

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(DEBOUNCE_MS);
    connect(m_debounceTimer, &QTimer::timeout, this, &InlineFindBar::onDebounceTimeout);

    connect(m_findInput, &QLineEdit::textChanged, this, &InlineFindBar::onFindTextChanged);
    connect(m_prevBtn, &QToolButton::clicked, this, &InlineFindBar::onFindPreviousClicked);
    connect(m_nextBtn, &QToolButton::clicked, this, &InlineFindBar::onFindNextClicked);
    connect(m_closeBtn, &QToolButton::clicked, this, &InlineFindBar::onCloseClicked);

    connect(m_caseBtn, &QToolButton::clicked, this, [this]() { onFindTextChanged(QString()); });
    connect(m_wordBtn, &QToolButton::clicked, this, [this]() { onFindTextChanged(QString()); });
    connect(m_regexBtn, &QToolButton::clicked, this, [this]() { onFindTextChanged(QString()); });

    connect(m_replaceBtn, &QPushButton::clicked, this, &InlineFindBar::onReplaceClicked);
    connect(m_replaceAllBtn, &QPushButton::clicked, this, &InlineFindBar::onReplaceAllClicked);

    m_findInput->installEventFilter(this);
    restoreState();
}

InlineFindBar::~InlineFindBar() = default;

// ── Events ───────────────────────────────────────────────────

void InlineFindBar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

bool InlineFindBar::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_findInput && event->type() == QEvent::KeyPress)
    {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
        {
            if (keyEvent->modifiers() & Qt::ShiftModifier)
                onFindPreviousClicked();
            else
                onFindNextClicked();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Escape)
        {
            onCloseClicked();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// ── UI Setup ─────────────────────────────────────────────────

void InlineFindBar::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(3);

    // Search icon on the left of the find input
    m_findInput = new QLineEdit(this);
    m_findInput->setPlaceholderText(tr("Search..."));
    m_findInput->setClearButtonEnabled(true);
    m_findInput->setFixedHeight(28);
    m_findInput->addAction(QIcon(QStringLiteral(":/icons/Edit/find.svg")), QLineEdit::LeadingPosition);
    layout->addWidget(m_findInput, 1);

    // Navigation buttons
    m_prevBtn = createToolButton(QIcon(QStringLiteral(":/icons/Edit/findprevious.svg")), tr("Find Previous (Shift+Enter)"));
    m_nextBtn = createToolButton(QIcon(QStringLiteral(":/icons/Edit/findnext.svg")), tr("Find Next (Enter)"));
    layout->addWidget(m_prevBtn);
    layout->addWidget(m_nextBtn);

    // Painted toggle icons (initially empty; filled by updateIcons)
    m_caseBtn = createToolButton(QIcon(), tr("Match Case"), true);
    m_wordBtn = createToolButton(QIcon(), tr("Match Whole Word"), true);
    m_regexBtn = createToolButton(QIcon(), tr("Use Regular Expressions"), true);
    layout->addWidget(m_caseBtn);
    layout->addWidget(m_wordBtn);
    layout->addWidget(m_regexBtn);

    // Match count
    m_matchLabel = new QLabel(this);
    m_matchLabel->setAlignment(Qt::AlignCenter);
    QFont labelFont = m_matchLabel->font();
    labelFont.setPixelSize(11);
    m_matchLabel->setFont(labelFont);
    m_matchLabel->setFixedWidth(56);
    layout->addWidget(m_matchLabel);

    // Replace section (container)
    m_replaceSection = new QWidget(this);
    auto* repLayout = new QHBoxLayout(m_replaceSection);
    repLayout->setContentsMargins(0, 0, 0, 0);
    repLayout->setSpacing(3);

    auto* sep = new QFrame(m_replaceSection);
    sep->setFrameShape(QFrame::VLine);
    sep->setFixedWidth(2);
    repLayout->addWidget(sep);

    m_replaceInput = new QLineEdit(m_replaceSection);
    m_replaceInput->setPlaceholderText(tr("Replace..."));
    m_replaceInput->setFixedWidth(130);
    m_replaceInput->setFixedHeight(28);
    repLayout->addWidget(m_replaceInput);

    m_replaceBtn = new QPushButton(tr("Rep"), m_replaceSection);
    m_replaceBtn->setFixedHeight(28);
    m_replaceBtn->setFixedWidth(44);
    m_replaceBtn->setFocusPolicy(Qt::NoFocus);
    repLayout->addWidget(m_replaceBtn);

    m_replaceAllBtn = new QPushButton(tr("All"), m_replaceSection);
    m_replaceAllBtn->setFixedHeight(28);
    m_replaceAllBtn->setFixedWidth(36);
    m_replaceAllBtn->setFocusPolicy(Qt::NoFocus);
    repLayout->addWidget(m_replaceAllBtn);

    m_replaceSection->setVisible(false);
    layout->addWidget(m_replaceSection);

    // Close button
    QIcon closeIcon = style()->standardIcon(QStyle::SP_TitleBarCloseButton);
    m_closeBtn = createToolButton(closeIcon, tr("Close (Esc)"));
    layout->addWidget(m_closeBtn);

    // Theme-aware appearance
    setAutoFillBackground(true);
    updateIcons();
}

// ── Visibility ───────────────────────────────────────────────

void InlineFindBar::setReplaceMode(bool replace)
{
    m_replaceMode = replace;
    m_replaceSection->setVisible(replace);
    if (replace)
        m_replaceInput->setFocus();
    else
        m_findInput->setFocus();
}

void InlineFindBar::showFindMode(const QString& selectedText)
{
    updateIcons();
    setReplaceMode(false);
    if (!selectedText.isEmpty())
        m_findInput->setText(selectedText);
    m_findInput->selectAll();
    show();
    raise();
    m_findInput->setFocus();
    performSearch();
}

void InlineFindBar::showReplaceMode(const QString& selectedText)
{
    updateIcons();
    setReplaceMode(true);
    if (!selectedText.isEmpty())
        m_findInput->setText(selectedText);
    m_findInput->selectAll();
    show();
    raise();
    m_findInput->setFocus();
    performSearch();
}

void InlineFindBar::closeBar()
{
    saveState();
    clearHighlights();
    hide();
}

// ── Accessors ────────────────────────────────────────────────

QString InlineFindBar::searchText() const
{
    return m_findInput->text();
}

bool InlineFindBar::matchCase() const
{
    return m_caseBtn->isChecked();
}

bool InlineFindBar::matchWholeWord() const
{
    return m_wordBtn->isChecked();
}

bool InlineFindBar::useRegex() const
{
    return m_regexBtn->isChecked();
}

// ── Slots ────────────────────────────────────────────────────

void InlineFindBar::onFindTextChanged(const QString&)
{
    if (isVisible())
        m_debounceTimer->start();
}

void InlineFindBar::onDebounceTimeout()
{
    performSearch();
}

void InlineFindBar::onFindNextClicked()
{
    findNext();
}

void InlineFindBar::onFindPreviousClicked()
{
    findPrevious();
}

void InlineFindBar::onCloseClicked()
{
    closeBar();
    emit closed();
}

void InlineFindBar::findNext()
{
    if (m_matches.isEmpty())
    {
        performSearch();
        if (m_matches.isEmpty())
            return;
    }
    int next = m_currentMatch + 1;
    if (next >= m_matches.size())
        next = 0;
    navigateToMatch(next);
}

void InlineFindBar::findPrevious()
{
    if (m_matches.isEmpty())
    {
        performSearch();
        if (m_matches.isEmpty())
            return;
    }
    int prev = m_currentMatch - 1;
    if (prev < 0)
        prev = m_matches.size() - 1;
    navigateToMatch(prev);
}

void InlineFindBar::onReplaceClicked()
{
    if (m_matches.isEmpty() || m_currentMatch < 0 || m_currentMatch >= m_matches.size())
        return;

    const auto& match = m_matches[m_currentMatch];
    m_editor->setSelection(match.pos, 0, match.pos + match.len, 0);
    m_editor->replaceSelectedText(m_replaceInput->text());

    performSearch();

    if (!m_matches.isEmpty())
    {
        m_currentMatch = qMin(m_currentMatch, m_matches.size() - 1);
        navigateToMatch(m_currentMatch);
    }
}

void InlineFindBar::onReplaceAllClicked()
{
    if (m_matches.isEmpty())
        return;

    QString replaceText = m_replaceInput->text();

    for (int i = m_matches.size() - 1; i >= 0; --i)
    {
        const auto& match = m_matches[i];
        m_editor->setSelection(match.pos, 0, match.pos + match.len, 0);
        m_editor->replaceSelectedText(replaceText);
    }

    performSearch();
    if (!m_matches.isEmpty())
        navigateToMatch(0);
}

// ── Search / Highlight / Navigation ──────────────────────────

void InlineFindBar::clearHighlights()
{
    long length = m_editor->SendScintilla(QsciScintillaBase::SCI_GETLENGTH);
    m_editor->SendScintilla(QsciScintillaBase::SCI_SETINDICATORCURRENT, FIND_INDICATOR);
    m_editor->SendScintilla(QsciScintillaBase::SCI_INDICATORCLEARRANGE, 0, length);
    m_matches.clear();
    m_currentMatch = -1;
    m_matchLabel->clear();
}

void InlineFindBar::performSearch()
{
    clearHighlights();

    QString text = m_findInput->text();
    if (text.isEmpty())
    {
        m_matchLabel->setText(tr("0/0"));
        return;
    }

    bool regex = m_regexBtn->isChecked();
    bool caseSensitive = m_caseBtn->isChecked();
    bool wholeWord = m_wordBtn->isChecked();

    QColor col = highlightColor();
    m_editor->SendScintilla(QsciScintillaBase::SCI_INDICSETSTYLE, FIND_INDICATOR, QsciScintillaBase::INDIC_ROUNDBOX);
    m_editor->SendScintilla(QsciScintillaBase::SCI_INDICSETFORE, FIND_INDICATOR,
                            static_cast<long>(col.red() | (col.green() << 8) | (col.blue() << 16)));
    m_editor->SendScintilla(QsciScintillaBase::SCI_SETINDICATORCURRENT, FIND_INDICATOR);

    int pos = 0;
    bool caseless = !caseSensitive;
    bool found = m_editor->findFirst(text, regex, caseless, wholeWord, false, true, 0, false);

    while (found)
    {
        long selStart = m_editor->SendScintilla(QsciScintillaBase::SCI_GETSELECTIONSTART);
        long selEnd = m_editor->SendScintilla(QsciScintillaBase::SCI_GETSELECTIONEND);
        int len = static_cast<int>(selEnd - selStart);

        m_editor->SendScintilla(QsciScintillaBase::SCI_INDICATORFILLRANGE, selStart, len);
        m_matches.append({static_cast<int>(selStart), len});

        found = m_editor->findNext();
        pos++;
        if (pos > 100000)
            break;
    }

    if (m_matches.isEmpty())
    {
        m_matchLabel->setText(tr("0/0"));
        m_currentMatch = -1;
    }
    else
    {
        navigateToMatch(0);
    }
}

void InlineFindBar::navigateToMatch(int index)
{
    if (index < 0 || index >= m_matches.size())
        return;

    m_currentMatch = index;
    const auto& match = m_matches[index];

    m_editor->setSelection(match.pos, 0, match.pos + match.len, 0);
    int line = static_cast<int>(m_editor->SendScintilla(QsciScintillaBase::SCI_LINEFROMPOSITION, match.pos));
    m_editor->ensureLineVisible(line);

    m_matchLabel->setText(tr("%1/%2").arg(index + 1).arg(m_matches.size()));
}

// ── State persistence ────────────────────────────────────────

void InlineFindBar::saveState()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("Find"));
    settings.setValue(QStringLiteral("MatchCase"), m_caseBtn->isChecked());
    settings.setValue(QStringLiteral("MatchWholeWord"), m_wordBtn->isChecked());
    settings.setValue(QStringLiteral("UseRegex"), m_regexBtn->isChecked());
    settings.endGroup();
}

void InlineFindBar::restoreState()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("Find"));
    m_caseBtn->setChecked(settings.value(QStringLiteral("MatchCase"), false).toBool());
    m_wordBtn->setChecked(settings.value(QStringLiteral("MatchWholeWord"), false).toBool());
    m_regexBtn->setChecked(settings.value(QStringLiteral("UseRegex"), false).toBool());
    settings.endGroup();
}

// ── Theme ─────────────────────────────────────────────────────

void InlineFindBar::updateIcons()
{
    ThemeColors colors = getThemeColors(m_editor->themeId());
    QColor fg = colors.dialogFg;

    m_caseBtn->setIcon(paintIcon(fg, QStringLiteral("Aa"), true));
    m_wordBtn->setIcon(paintIcon(fg, QStringLiteral("ab"), false, true));
    m_regexBtn->setIcon(paintIcon(fg, QStringLiteral(".*")));

    QPalette pal = palette();
    pal.setColor(QPalette::Window, colors.dialogBg);
    pal.setColor(QPalette::WindowText, colors.dialogFg);
    setPalette(pal);
}

QColor InlineFindBar::highlightColor() const
{
    if (!m_editor)
        return QColor(255, 255, 0, 100);
    ThemeColors colors = getThemeColors(m_editor->themeId());
    QColor base = colors.accent;
    base.setAlpha(80);
    return base;
}
