// My header
#include "FmMain.h"
#include "ui_FmMain.h"

// C++
#include <iostream>
#include <cmath>
#include <bit>

// Qt
#include <QTableView>
#include <QTextFrame>
#include <QClipboard>
#include <QPainter>
#include <QShortcut>
#include <QPaintEngine>
#include <QMessageBox>
#include <QFile>

// Misc
#include "u_Strings.h"
#include "u_Qstrings.h"

// Project-local
#include "Skin.h"
#include "Wiki.h"
#include "MyWiki.h"

// Forms
#include "FmPopup.h"
#include "FmMessage.h"

template class LruCache<char32_t, QPixmap>;

using namespace std::string_view_literals;

namespace {
    // No need custom drawing — solves nothing
    constexpr TableDraw TABLE_DRAW = TableDraw::INTERNAL;
}



///// FmPopup2 /////////////////////////////////////////////////////////////////


FmPopup2::FmPopup2(FmMain* owner) : Super(owner, CNAME_BG_POPUP)
{
    auto vw = viewport();
    vw->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);
    connect(vw, &QLabel::linkActivated, owner, &FmMain::popupLinkActivated);
}


///// RowCache /////////////////////////////////////////////////////////////////

RowCache::RowCache(int anCols)
    : fnCols(anCols), fColMask(anCols - 1), fRowMask(~fColMask) {}


MaybeChar RowCache::charAt(size_t iRow, unsigned iCol) const
{
    // Have row?
    if (iRow >= rows.size() || iCol >= NCOLS)
        return { 0, nullptr };
    auto& rw = rows[iRow];
    auto start = rw.startingCp;

    return { start + iCol, rw.cps[iCol] };
}


int RowCache::startingCpAt(size_t iRow) const
{
    if (iRow >= rows.size())
        return uc::NO_CHAR;
    return rows[iRow].startingCp;
}


RowCache::Row& RowCache::ensureRow(unsigned aStartingCp)
{
    if (!rows.empty()) {
        Row& bk = rows.back();
        if (bk.startingCp == aStartingCp)
            return bk;
    }
    Row& newBk = rows.emplace_back(aStartingCp);
    return newBk;
}


void RowCache::addCp(const uc::Cp& aCp)
{
    unsigned code = aCp.subj;
    int rowCp = code & fRowMask;
    int iCol = code & fColMask;

    Row& bk = ensureRow(rowCp);
    bk.cps[iCol] = &aCp;
}


bool RowCache::isLessRC(const Row& x, char32_t y)
{
    return x.startingCp < y;
}


CacheCoords RowCache::findCode(char32_t code) const
{
    auto it = std::lower_bound(rows.begin(), rows.end(), code, isLessRC);
    if (it == rows.end() || it->startingCp > code)
        --it;
    size_t rw = it - rows.begin();
    unsigned dif = code - it->startingCp;
    if (dif < static_cast<unsigned>(fnCols)) {
        return { rw, dif };
    } else {
        return { rw, 0 };
    }
}


///// BlocksModel //////////////////////////////////////////////////////////////


int BlocksModel::rowCount(const QModelIndex&) const { return uc::N_BLOCKS; }

int BlocksModel::columnCount(const QModelIndex&) const { return 1; }

QVariant BlocksModel::data(const QModelIndex& index, int role) const
{
    #define GET_BLOCK \
            size_t i = index.row();         \
            if (i >= uc::N_BLOCKS)          \
                return {};                  \
            auto& block = uc::blocks[i];

    switch (role) {
    case Qt::DisplayRole: {
            GET_BLOCK
            return str::toQ(block.locName);
            /// @todo [ui] how to show character ranges? — now they are bad
            //char buf[200];
            //snprintf(buf, 200, reinterpret_cast<const char*>(u8"%.*s (%04X—%04X)"),
            //         block.name.length(), block.name.data(),
            //         static_cast<int>(block.startingCp),
            //         static_cast<int>(block.endingCp));
        }
    case Qt::DecorationRole: {
            GET_BLOCK
            if (!block.icon) {
                char buf[48];
                snprintf(buf, std::size(buf), ":/Scripts/%04X.png", static_cast<int>(block.startingCp));
                block.icon.reset(new QPixmap());
                block.icon->load(buf);
            }
            return *block.icon;
        }
    default:
        return {};
    }

    #undef GET_BLOCK
}


///// CharsModel ///////////////////////////////////////////////////////////////


CharsModel::CharsModel(QWidget* aOwner) :
    owner(aOwner),
    match(str::toQ(FAM_DEFAULT)),
    rows(NCOLS)
{
    tcache.connectSignals(this);
}


int CharsModel::rowCount(const QModelIndex&) const
{
    return rows.nRows();
}


int CharsModel::columnCount(const QModelIndex&) const
    { return NCOLS; }


const QFont* CharsModel::fontAt(const QModelIndex& index) const
{
    auto cp = charAt(index);
    if (!cp)
        return nullptr;
    return fontAt(*cp);
}


const QFont* CharsModel::fontAt(const uc::Cp& cp, const uc::Block*& hint)
{
    if (cp.drawMethod() > uc::DrawMethod::LAST_FONT)
        return {};
    auto& font = cp.font(hint);
    return &font.get(font.q.table, uc::FontPlace::CELL, FSZ_TABLE, cp.subj);
}


const QFont* CharsModel::fontAt(const uc::Cp& cp) const
{
    return fontAt(cp, hint.cell);
}


QColor CharsModel::fgAt(const QModelIndex& index, TableColors tcl) const
{
    auto cp = charAt(index);
    if (!cp)
        return {};
    return fgAt(*cp, tcl);
}


QColor CharsModel::fgAt(const uc::Cp& cp, TableColors tcl) const
{
    if (tcl != TableColors::NO) {
        if (isCjkCollapsed) {
            auto block = uc::blockOf(cp.subj, hint.cell);
            if (block->flags.have(uc::Bfg::COLLAPSIBLE))
                return TX_CJK;
        }
    }
    return {};
}


QString CharsModel::textAt(const QModelIndex& index, CharSet chset) const
{
    auto cp = charAt(index);
    if (!cp)
        return {};
    return textAt(*cp, chset);
}


QString CharsModel::textAt(const uc::Cp& cp, CharSet chset) const
    { return textAt(cp, hint.cell, chset); }

QString CharsModel::textAt(const uc::Cp& cp, const uc::Block*& hint, CharSet chset)
{
    if (chset == CharSet::SAFE) {
//        hint.cell = uc::blockOf(cp.subj, hint.cell);
//        if (hint.cell->flags.have(uc::Bfg::EXPERIMENT))
//            return {};
    }
    return cp.sampleProxy(hint).text;
}


QVariant CharsModel::data(const QModelIndex& index, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if constexpr (TABLE_DRAW == TableDraw::INTERNAL) {
            if (auto q = textAt(index, CharSet::SAFE); !q.isEmpty())
                return q;
        }
        return {};

    case Qt::FontRole:
        if constexpr (TABLE_DRAW == TableDraw::INTERNAL) {
            if (auto q = fontAt(index))
                return *q;
        }
        return {};

    case Qt::ForegroundRole:
        if constexpr (TABLE_DRAW == TableDraw::INTERNAL) {
            if (auto c = fgAt(index, TableColors::YES); c.isValid())
                return c;
        }
        if (isCjkCollapsed) {

        }
        return {};

    case Qt::BackgroundRole: {
            auto cp = charAt(index);
            if (cp) {
                if (isCjkCollapsed) {
                    hint.cell = uc::blockOf(cp->subj, hint.cell);
                    if (hint.cell->flags.have(uc::Bfg::COLLAPSIBLE))
                        return BG_CJK;
                }
            } else {
                if (uc::isNonChar(cp.code)) {
                    return QBrush(owner->palette().windowText().color(), Qt::DiagCrossPattern);
                }
                return owner->palette().button().color();
            }
            return {};
        }

    case Qt::TextAlignmentRole:
        return Qt::AlignCenter;
    default:
        return {};
    }
}


namespace {

    template <int Ncols>
    QString horzHeaderOf(int section);

    template <>
    [[maybe_unused]] inline QString horzHeaderOf<8>(int section)
    {
        return QString::number(section, 16) + QString::number(section + 8, 16);
    }

    template <>
    [[maybe_unused]] inline QString horzHeaderOf<16>(int section)
    {
        return QString::number(section, 16);
    }

}   // anon namespace


QVariant CharsModel::headerData(int section, Qt::Orientation orientation,
                    int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (orientation == Qt::Horizontal) {
            return horzHeaderOf<NCOLS>(section);
        } else {
            int c = rows.startingCpAt(section);
            if (static_cast<size_t>(section) >= rows.nRows())
                return {};
            char buf[20];
            snprintf(buf, std::size(buf), "%04x", c);
            return buf;
        }
        break;
    default:
        return {};
    }
}


void CharsModel::addCp(const uc::Cp& aCp)
{
    rows.addCp(aCp);
}


QModelIndex CharsModel::indexOf(char32_t code)
{
    auto coords = rows.findCode(code);
    return index(coords.row, coords.col);
}

void CharsModel::build()
{
    beginResetModel();
    rows.clear();
    const uc::Block* hint = &uc::blocks[0];
    for (auto& cp : uc::cpInfo) {
        if (isCharCollapsed(cp.subj, hint))
            continue;
        addCp(cp);
    }
    endResetModel();
}


namespace {

    enum class SplitMode { NORMAL, FIXED };

    struct AbbrLines {
        std::u8string_view line1, line2, line3;

        unsigned nLines() const { return line2.empty() ? 1 : line3.empty() ? 2 : 3; }
        bool wasSplit() const { return !line2.empty(); }
        unsigned length() const {
            return std::max(std::max(line1.length(), line2.length()), line3.length()); }
        qreal sizeQuo() const;
    };

    constexpr qreal Q1 = 2.0;
    constexpr qreal Q3 = 3.3;
    constexpr qreal Q4 = 4.0;
    constexpr qreal Q5 = 5.0;
    const qreal lenQuos[]  = { Q3, Q3, Q3, Q3, Q4, Q5, Q5, Q5, Q5, Q5 };

    qreal AbbrLines::sizeQuo() const
    {
        auto sz = nLines(), len = length();
        switch (sz) {
        case 3: return Q5;
        case 1:
            if (len == 1)
                return Q1;
            [[fallthrough]];
        default:
            return lenQuos[len];
        }
    }

    AbbrLines splitAbbr(std::u8string_view abbr, SplitMode mode)
    {
        if (mode == SplitMode::FIXED)
            return { abbr, {}, {} };
        if (auto pSpace = abbr.find(' '); pSpace != std::u8string_view::npos) {
            auto line1 = abbr.substr(0, pSpace), line23 = abbr.substr(pSpace + 1);
            if (auto pSpace2 = line23.find(' '); pSpace2 != std::u8string_view::npos) {
                auto line2 = line23.substr(0, pSpace2);
                auto line3 = line23.substr(pSpace + 1);
                return { line1, line2, line3 };
            } else {
                return { line1, line23, {} };
            }
        }
        switch (abbr.size()) {
        case 4:
        case 6: {
                auto split = abbr.size() / 2;
                return { abbr.substr(0, split), abbr.substr(split), {} };
            }
        case 5: {
                auto split = 3;
                if (isdigit(abbr[2]) && isdigit(abbr[3]))
                    split = 2;
                return { abbr.substr(0, split), abbr.substr(split), {} };
            }
        default:
            return { abbr, {}, {} };
        }
    }

    struct RcPair {
        QRectF rc1, rc2;

        RcPair(const QRectF& rcFrame, qreal quo) : rc1(rcFrame), rc2(rcFrame) {
            auto dh = rcFrame.height() / quo;
            rc1.setBottom(rc1.bottom() - dh);
            rc2.setTop(rc2.top() + dh);
        }
    };

    struct Rc3Matrix {
        int side, side2, side3, x0, y0;
        Rc3Matrix(const QRectF& rcFrame) {
            auto sz = std::min(rcFrame.width(), rcFrame.height());
            side = std::round(sz / 5.5);
            side2 = side * 2;
            side3 = side * 3;
            auto cen = rcFrame.center();
            x0 = std::round(cen.x() - side * 1.5);
            y0 = std::round(cen.y() - side * 1.5);
        }

        //  789
        //  456
        //  123
        QRect rect7() const
            { return QRect( QPoint{ x0, y0 }, QSize { side, side }); }
        QRect rect96() const
            { return QRect( QPoint{ x0 + side2, y0 }, QSize { side, side2 }); }
        QRect rect13() const
            { return QRect( QPoint{ x0, y0 + side2 }, QSize { side3, side }); }
        QRect rect1() const
            { return QRect( QPoint{ x0, y0 + side2 }, QSize { side, side }); }
        QRect rect63() const
            { return QRect( QPoint{ x0 + side2, y0 + side }, QSize { side, side2 }); }
        QRect rect79() const
            { return QRect( QPoint{ x0, y0 }, QSize { side3, side }); }
        QRect rect74() const
            { return QRect( QPoint{ x0, y0 }, QSize { side, side2 }); }
        QRect rect9() const
            { return QRect( QPoint{ x0 + side2, y0 }, QSize { side, side }); }
        QRect rect41() const
            { return QRect( QPoint{ x0, y0 + side }, QSize { side, side2 }); }
        QRect rect3() const
            { return QRect( QPoint{ x0 + side2, y0 + side2 }, QSize { side, side }); }
    };

    void drawAbbrevText(QPainter* painter, std::u8string_view abbreviation,
            const QColor& color, QRectF rcFrame, qreal thickness,
            SplitMode mode = SplitMode::NORMAL)
    {
        // Draw text
        auto sp = splitAbbr(abbreviation, mode);
        auto availSize = rcFrame.width();
        auto sz = availSize / sp.sizeQuo();
        QFont font { str::toQ(FAM_CONDENSED) };
            font.setStyleStrategy(QFont::PreferAntialias);
            font.setPointSizeF(sz);
        painter->setFont(font);
        painter->setBrush(QBrush(color, Qt::SolidPattern));
        rcFrame.setLeft(rcFrame.left() + std::max(thickness, 1.0));
        switch (sp.nLines()) {
        case 1:
            painter->drawText(rcFrame, Qt::AlignCenter, str::toQ(sp.line1));
            break;
        case 2: {
                RcPair p(rcFrame, 2.5);
                painter->drawText(p.rc1, Qt::AlignCenter, str::toQ(sp.line1));
                painter->drawText(p.rc2, Qt::AlignCenter, str::toQ(sp.line2));
            } break;
        default: { // case 3
                RcPair p(rcFrame, 1.8);
                painter->drawText(p.rc1, Qt::AlignCenter, str::toQ(sp.line1));
                painter->drawText(rcFrame, Qt::AlignCenter, str::toQ(sp.line2));
                painter->drawText(p.rc2, Qt::AlignCenter, str::toQ(sp.line3));
            }
        }
    }

    void drawAbbreviation(
            QPainter* painter, const QRect& rect, std::u8string_view abbreviation,
            const QColor& color, char32_t subj)
    {
        const auto availW = rect.width() * 5 / 6;
        const auto availH = rect.height() * 7 / 10;
        const auto availSize = std::min(availW, availH);
        // Now we draw availSize×availSize, shrink rect
        auto ofsX = (rect.width() - availSize) / 2;
        auto ofsY = (rect.height() - availSize) / 2;
        auto rcFrame = QRectF(QPoint(rect.left() + ofsX, rect.top() + ofsY),
                              QSize(availSize, availSize));
        // Draw frame
        auto thickness = availSize / 60.0f;
        painter->setPen(QPen(color, thickness, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rcFrame);

        // Need this brush for both rects and fonts

        if (abbreviation == u8"`"sv) {
            // Draw special image
            switch (subj) {
            case 0x13432: {
                    Rc3Matrix m(rcFrame);
                    painter->fillRect(m.rect7(), color);
                    painter->fillRect(m.rect96(), color);
                    painter->fillRect(m.rect13(), color);
                } break;
            case 0x13433: {
                    Rc3Matrix m(rcFrame);
                    painter->fillRect(m.rect79(), color);
                    painter->fillRect(m.rect63(), color);
                    painter->fillRect(m.rect1(), color);
                } break;
            case 0x13434: {
                    Rc3Matrix m(rcFrame);
                    painter->fillRect(m.rect74(), color);
                    painter->fillRect(m.rect9(), color);
                    painter->fillRect(m.rect13(), color);
                } break;
            case 0x13435: {
                    Rc3Matrix m(rcFrame);
                    painter->fillRect(m.rect79(), color);
                    painter->fillRect(m.rect41(), color);
                    painter->fillRect(m.rect3(), color);
                } break;
            case 0xE0001:
                drawAbbrevText(painter, u8"BEGIN"sv, color, rcFrame, thickness, SplitMode::FIXED);
                break;
            case 0xE0020:
                drawAbbrevText(painter, u8"SP"sv, color, rcFrame, thickness);
                break;
            case 0xE007F:
                drawAbbrevText(painter, u8"END"sv, color, rcFrame, thickness);
                break;
            default:
                // Tags
                if (subj >= 0xE0000 && subj <= 0xE00FF) {
                    char8_t c = subj;
                    std::u8string_view uv { &c, 1 };
                    drawAbbrevText(painter, uv, color, rcFrame, thickness);
                }
            }
        } else {
            drawAbbrevText(painter, abbreviation, color, rcFrame, thickness);
        }
    }

    QSize spaceDimensions(const QFont& font, char32_t subj)
    {
        QString s = QString::fromUcs4(&subj, 1);
        QFontMetrics metrics(font);
        return {   // Spaces are SPACING → ban 0
                   // All known spaces are direction-neutral and thus LTR, but who knows?
            std::max(std::abs(metrics.horizontalAdvance(s)), 1),
            metrics.height() * 4 / 5 };
    }

    void drawSpace(
            QPainter* painter, const QRect& rect,
            const QFont& font, QColor color, char32_t subj)
    {
        // Quotient to convert space height to line thickness
        constexpr auto Q_H2THICK = 1.0 / 25.0;

        auto dim = spaceDimensions(font, subj);
        color.setAlpha(ALPHA_SPACE);
        auto lineW = std::max(
                    static_cast<int>(std::round(dim.height() * Q_H2THICK)), 1);
        auto x = rect.left() - lineW + (rect.width() - dim.width()) / 2;
        auto y = rect.top() + (rect.height() - dim.height()) / 2;
        QBrush brush(color);
        painter->fillRect(x, y, lineW, dim.height(), brush);
        x += lineW;
        x += dim.width();
        painter->fillRect(x, y, lineW, dim.height(), brush);
    }

    void drawDeprecated(QPainter* painter, const QRect& r)
    {
        static constexpr int SZ = 8;    // we draw lines between pixel centers, actually +1
        static constexpr int OFS = 4;
        const int x1 = r.right() - OFS;
        const int x0 = x1 - SZ;
        const int y0 = r.top() + OFS;
        const int y1 = y0 + SZ;
        painter->setPen(FG_DEPRECATED);
        painter->drawLine(x0, y0, x1, y1);
        painter->drawLine(x0, y1, x1, y0);
    }

}   // anon namespace


void CharsModel::drawChar(QPainter* painter, const QRect& rect,
            const uc::Cp& cp, const QColor& color,
            const uc::Block*& hint, TableDraw mode)
{
    switch (cp.drawMethod()) {
    case uc::DrawMethod::ABBREVIATION:
        drawAbbreviation(painter, rect, cp.abbrev(), color, cp.subj);
        break;
    case uc::DrawMethod::SPACE:
        drawSpace(painter, rect, *fontAt(cp, hint), color, cp.subj);
        break;
    case uc::DrawMethod::SAMPLE:
        if (mode == TableDraw::CUSTOM) {
            // Char
            painter->setFont(*fontAt(cp, hint));
            painter->setBrush(color);
            painter->setPen(color);
            painter->drawText(rect,
                              Qt::AlignCenter | Qt::TextSingleLine,
                              textAt(cp, hint));
        } break;
    }
    if (cp.isDeprecated())
        drawDeprecated(painter, rect);
}

void CharsModel::drawChar(QPainter* painter, const QRect& rect,
            const QModelIndex& index, const QColor& color) const
{
    auto ch = charAt(index);
    if (ch) {
        auto color1 = fgAt(*ch, TableColors::YES);
        if (!color1.isValid())
            color1 = color;
        drawChar(painter, rect, *ch, color1, hint.cell, TABLE_DRAW);
    }
}


void CharsModel::initStyleOption(QStyleOptionViewItem *option,
                     const QModelIndex &index) const
{
    SuperD::initStyleOption(option, index);
    if (option->state & (QStyle::State_HasFocus | QStyle::State_Selected)) {
        option->state.setFlag(QStyle::State_Selected, false);
        option->state.setFlag(QStyle::State_HasFocus, false);
        if (option->backgroundBrush.style() != Qt::NoBrush) {
            auto clBg = option->backgroundBrush.color();
            if (clBg.isValid()) {
                clBg.setAlpha(clBg.alpha() / 2);
                option->backgroundBrush.setColor(clBg);
            }
        }
    }
}


void CharsModel::paintItem(
        QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (option.state.testFlag(QStyle::State_HasFocus)) {
        // It’d be nice to draw some nice focus using Windows skin, but cannot
        //Super::paint(painter, option, index);
        // Draw it as a button
        QStyleOptionButton sob;
            sob.state = QStyle::State_HasFocus | QStyle::State_MouseOver | QStyle::State_Selected
                        | QStyle::State_Active | QStyle::State_Enabled;
            sob.rect = option.rect;
        owner->style()->drawControl(QStyle::CE_PushButton, &sob, painter, option.widget);
        SuperD::paint(painter, option, index);
        drawChar(painter, option.rect, index, owner->palette().buttonText().color());
    } else if (option.state.testFlag(QStyle::State_Selected)) {
        // Selected, not focused? Initial style is bad
        auto opt2 = option;
        opt2.state.setFlag(QStyle::State_Selected, false);
        owner->style()->drawPrimitive(QStyle::PE_FrameMenu, &opt2, painter, option.widget);
        SuperD::paint(painter, option, index);
        drawChar(painter, option.rect, index, owner->palette().windowText().color());
    } else {
        SuperD::paint(painter, option, index);
        drawChar(painter, option.rect, index, owner->palette().windowText().color());
    }
}


void CharsModel::paint(QPainter *painter, const QStyleOptionViewItem &option,
           const QModelIndex &index) const
{
    tcache.paint(painter, option, index, *this);
}


bool CharsModel::isCharCollapsed(char32_t code, const uc::Block*& hint) const
{
    if (isCjkCollapsed) {
        auto blk = uc::blockOf(code, hint);
        return (blk->flags.have(uc::Bfg::COLLAPSIBLE)
                && (code ^ static_cast<uint32_t>(blk->firstAllocated->subj)) >= NCOLS);
    } else {
        return false;
    }
}


bool CharsModel::isCharCollapsed(char32_t code) const
{
    const uc::Block* hint = &uc::blocks[0];
    return isCharCollapsed(code, hint);
}


///// SearchModel //////////////////////////////////////////////////////////////


void SearchModel::clear()
{
    beginResetModel();
    v.clear();
    endResetModel();
}


const uc::Cp& SearchModel::cpAt(size_t index) const
{
    if (index >= v.size())
        return uc::cpInfo[0];
    return *v[index];
}


void SearchModel::set(SafeVector<const uc::Cp*>&& x)
{
    beginResetModel();
    v = std::move(x);
    endResetModel();
}


QVariant SearchModel::data(const QModelIndex& index, int role) const
{
    auto& cp = cpAt(index.row());
    char buf[30];

    switch (role) {
    case Qt::DisplayRole:
        uc::sprintUPLUS(buf, cp.subj.ch32());
        return QString(buf) + '\n' + cp.viewableName();

    case Qt::DecorationRole:
        return cache.getT(cp.subj.ch32(),
            [&cp, this](QPixmap& pix) {
                auto& font = sample->font();
                QFontMetrics metrics{font};
                auto size = (metrics.ascent() + metrics.descent()) * 3;
                if (pix.size() != QSize{size, size}) {
                    pix = QPixmap{size, size};
                }
                pix.fill(Qt::transparent);

                // Create painter
                QColor color = sample->palette().windowText().color();
                QPainter painter(&pix);
                auto bounds = pix.rect();

                // Create transparent color
                QColor clTrans(color);
                clTrans.setAlpha(30);

                // Draw bounds
                auto bounds1 = bounds;
                bounds1.adjust(0, 0, -1, -1);
                painter.setBrush(Qt::NoBrush);
                painter.setPen(clTrans);
                painter.drawRect(bounds1);

                // OK w/o size, as 39 ≈ 40
                CharsModel::drawChar(&painter, bounds, cp, color, hint, TableDraw::CUSTOM);
            });
    default:
        return {};
    }
}


//void SearchModel::forceAdd(char32_t x)
//{
//    ndx.insert(x);
//    result.push_back(x);
//}


//void SearchModel::softAdd(char32_t x)
//{
//    if (ndx.insert(x).second)
//        result.push_back(x);
//}



///// WiCustomDraw /////////////////////////////////////////////////////////////


void WiCustomDraw::init()
{
    initialSize = minimumSize();
}


void WiCustomDraw::paintEvent(QPaintEvent *event)
{
    Super::paintEvent(event);
    switch (mode) {
    case Mode::NONE: break;
    case Mode::ABBREVIATION: {
            QPainter painter(this);
            drawAbbreviation(&painter, geometry(), abbreviation,
                             palette().windowText().color(),
                             subj);
        } break;
    case Mode::SPACE: {
            QPainter painter(this);
            drawSpace(&painter, geometry(), *fontSpace,
                      palette().windowText().color(),
                      subj);
        } break;
    }
}

void WiCustomDraw::setNormal()
{
    mode = Mode::NONE;
    setMinimumSize(initialSize);
}

void WiCustomDraw::setAbbreviation(std::u8string_view x, char32_t aSubj)
{
    setNormal();
    mode = Mode::ABBREVIATION;
    abbreviation = x;
    subj = aSubj;
    update();
}


void WiCustomDraw::setSpace(const QFont& font, char32_t aSubj)
{
    static constexpr auto SPACE_PLUS = 30;

    fontSpace = &font;
    mode = Mode::SPACE;
    subj = aSubj;

    // Set appropriate size
    auto dim = spaceDimensions(font, aSubj);
    setMinimumSize(QSize(
                std::max(initialSize.width(), dim.width() + SPACE_PLUS),
                std::max(initialSize.height(), dim.height())));
}


///// FmMain ///////////////////////////////////////////////////////////////////

FmMain::FmMain(QWidget *parent)
    : Super(parent),
      ui(new Ui::FmMain),
      model(this),
      searchModel(this),
      fontBig(str::toQ(FAM_DEFAULT), FSZ_BIG)
{
    ui->setupUi(this);

    // Tabs to 0
    ui->tabsMain->setCurrentIndex(0);

    // Collapse bar
    ui->wiCollapse->hide();
    ui->wiCollapse->setStyleSheet(
                "#wiCollapse { background-color: " + BG_CJK.name() + "; }"   );
    connect(ui->btCollapse, &QPushButton::clicked,
            this, &This::cjkExpandCollapse);
    cjkReflectCollapseState();

    // Top bar
    QPalette pal = ui->wiCharBar->palette();
    const QColor& color = pal.color(QPalette::Normal, QPalette::Button);
    ui->wiCharBar->setStyleSheet("#wiCharBar { background-color: " + color.name() + " }");
    ui->pageInfo->setStyleSheet("#pageInfo { background-color: " + color.name() + " }");
    ui->pageSearch->setStyleSheet("#pageSearch { background-color: " + color.name() + " }");

    // Fill chars
    model.build();

    // Combobox
    ui->comboBlock->setModel(&blocksModel);

    // Table
    ui->tableChars->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableChars->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableChars->setItemDelegate(&model);
    ui->tableChars->setModel(&model);

    // Divider
    auto w = width();
    /// @todo [sizes] Not really a cool size
    QList<int> sizes { w * 45 / 100, w * 55 / 100 };
    ui->splitBlocks->setStretchFactor(0, 0);
    ui->splitBlocks->setStretchFactor(1, 1);
    ui->splitBlocks->setSizes(sizes);

    // Connect events
    connect(ui->tableChars->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &This::charChanged);

    // Sample
    ui->pageSampleCustom->init();

    // OS style    
    auto& font = uc::fontInfo[0];
    ui->lbOs->setFont(font.get(font.q.big, uc::FontPlace::SAMPLE, FSZ_BIG, NO_TRIGGER));    

    // Copy
        // Ctrl+C
    auto shcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_C), ui->tableChars,
                nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
    connect(shcut, &QShortcut::activated, this, &This::copyCurrentChar);
        // Ctrl+Ins
    shcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Insert), ui->tableChars,
                nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
    connect(shcut, &QShortcut::activated, this, &This::copyCurrentChar);
        // Button
    connect(ui->btCopy, &QPushButton::clicked, this, &This::copyCurrentChar);
        // 2click
    ui->tableChars->viewport()->installEventFilter(this);

    // Copy sample: Ctrl+Shift+C
    shcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C), ui->tableChars,
                nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
    connect(shcut, &QShortcut::activated, this, &This::copyCurrentSample);
    shcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Insert), ui->tableChars,
                nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
    connect(shcut, &QShortcut::activated, this, &This::copyCurrentSample);

    // Tofu stats
    shcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), this);
    connect(shcut, &QShortcut::activated, this, &This::showTofuStats);

    // Clicked
    connect(ui->vwInfo, &QTextBrowser::anchorClicked, this, &This::anchorClicked);
    connect(ui->lbCharCode, &QLabel::linkActivated, this, &This::labelLinkActivated);

    // Search
    ui->stackSearch->setCurrentWidget(ui->pageInfo);
    connect(ui->btCloseSearch, &QPushButton::clicked, this, &This::closeSearch);
    connect(ui->tableChars, &CharsTable::focusIn, this, &This::closeSearch);    
    ui->listSearch->setModel(&searchModel);
    connect(ui->edSearch, &SearchEdit::searchPressed, this, &This::startSearch);
    connect(ui->edSearch, &SearchEdit::focusIn, this, &This::focusSearch);
    connect(ui->listSearch, &SearchList::enterPressed, this, &This::searchEnterPressed);

    // Terms
    initTerms();
    // About
    initAbout();

    // Set focus defered
    connect(this, &This::setFocusDefered, this, &This::slotSetFocusDefered,
            Qt::QueuedConnection);

    // Select index
    ui->tableChars->setFocus();
    ui->tableChars->selectionModel()->select(model.index(0, 0), QItemSelectionModel::SelectCurrent);
}


void FmMain::initTerms()
{
    QString text;
    mywiki::appendStylesheet(text);

    const size_t n = uc::nTerms();
    auto lastCat = uc::EcTermCat::NN;
    for (size_t i = 0; i < n; ++i) {
        const uc::Term& term = uc::terms[i];
        if (term.cat != lastCat) {
            lastCat = term.cat;
            auto& cat = uc::termCats[static_cast<int>(lastCat)];
            str::append(text, "<h1>");
            str::append(text, cat.locName);
            str::append(text, "</h1>");
        }
        str::append(text, "<p><a href='pt:");
        str::append(text, term.key);
        str::append(text, "' class='popup'><b>");
        str::append(text, term.locName);
        str::append(text, "</b></a>");
        if (!term.engName.empty()) {
            str::append(text, "&nbsp;/ ");
            str::append(text, term.engName);
        }
    }

    ui->vwTerms->setText(text);
    connect(ui->vwTerms, &QTextBrowser::anchorClicked, this, &This::anchorClicked);
}


void FmMain::initAbout()
{
    // Get version
    auto version = QApplication::applicationVersion();
        // Count “.” chars
    int nDots = 0;
    for (auto c : version)
        if (c == '.')
            ++nDots;
    // Remove '.0' if there will be dots remaining
    while (nDots > 1 && version.endsWith(".0")) {
        version.resize(version.length() - 2);
        --nDots;
    }

    // lbVersion
    QString text;
    str::append(text, u8"Версия <b>");
    text.append(version);
    str::append(text, u8"</b> (Юникод ");
    str::append(text, uc::versionInfo[static_cast<int>(uc::EcVersion::LAST)].name);
    text.append(")");
    ui->lbVersion->setText(text);

    // vwVersion
    QFile f(":/Texts/about.htm");
    f.open(QIODevice::ReadOnly);
    QString s = f.readAll();
    s = "<style>a { text-decoration: none; color: " CNAME_LINK_OUTSIDE "; }</style>" + s;
    ui->vwAbout->setText(s);
}


bool FmMain::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonDblClick: {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (obj == ui->tableChars->viewport()
                    && mouseEvent->button() == Qt::LeftButton) {
                copyCurrentChar();
                return true;
            }
        }
    default:;
    }
    return Super::eventFilter(obj, event);
}


FmMain::~FmMain()
{
    delete ui;
}


void FmMain::showCopied(QWidget* widget, const QRect& absRect)
{
    fmMessage.ensure(this)
             .showAtAbs("Скопировано", widget, absRect);
}


void FmMain::showCopied(QAbstractItemView* table)
{
    auto widget = qobject_cast<QWidget*>(sender());
    QPoint corner { 0, 0 };
    QSize size { 0, 0 };
    if (widget) {
        // corner is (0,0)
        size = widget->size();
    } else {
        auto selIndex = table->currentIndex();
        auto selRect = table->visualRect(selIndex);
        widget = table->viewport();
        // Geometry is in PARENT’s coord system
        // Rect is (0,0) W×H
        auto visibleGeo = widget->rect();
        selRect = selRect.intersected(visibleGeo);
        if (selRect.isEmpty()) {
            corner = visibleGeo.center();
            // Size is still 0
        } else {
            corner = selRect.topLeft();
            size = selRect.size();
        }
    }

    corner = widget->mapToGlobal(corner);
    showCopied(widget, QRect{ corner, size});
}


void FmMain::copyCurrentThing(CurrThing thing)
{
    auto ch = model.charAt(ui->tableChars->currentIndex());
    if (ch) {
        QString q;
        if (thing == CurrThing::SAMPLE
                && ch->category().upCat == uc::UpCategory::MARK) {
            q = uc::STUB_CIRCLE + str::toQ(ch->subj);
        } else {
            q = str::toQ(ch->subj);
        }
        QApplication::clipboard()->setText(q);
        showCopied(ui->tableChars);
    }
}


void FmMain::copyCurrentChar()
{
    copyCurrentThing(CurrThing::CHAR);
}


void FmMain::copyCurrentSample()
{
    copyCurrentThing(CurrThing::SAMPLE);
}


void FmMain::drawSampleWithQt(const uc::Cp& ch)
{
    ui->pageSampleCustom->setNormal();

    // Font
    auto& font = ch.font(hint.sample);
    ui->lbSample->setFont(font.get(font.q.big, uc::FontPlace::SAMPLE, FSZ_BIG, ch.subj));

    // Sample char
    ui->stackSample->setCurrentWidget(ui->pageSampleQt);
    auto proxy = ch.sampleProxy(hint.sample);
    // Color
    if (ch.isTrueSpace()) {
        auto c = palette().text().color();
        c.setAlpha(ALPHA_SPACE);
        QString css = "color: " + c.name(QColor::HexArgb) + ';';
        ui->lbSample->setStyleSheet(css);
    } else {
        ui->lbSample->setStyleSheet(str::toQ(proxy.styleSheet));
    }
    ui->lbSample->setText(proxy.text);
}


template<>
void wiki::append(QString& s, const char* start, const char* end)
{
    s.append(QByteArray::fromRawData(start, end - start));
}

namespace {

    template <class X>
    [[deprecated]] inline void appendWiki(QString& text, const X& obj, std::u8string_view x)
        { mywiki::append(text, x, obj.font()); }

}   // anon namespace


void FmMain::clearSample()
{
    ui->lbSample->clear();
    ui->lbSample->setFont(QFont());
}


void FmMain::showCp(MaybeChar ch)
{
    if (ch.code == shownCp)
        return;
    shownCp = ch.code;

    // Code
    char buf[30];
    { QString ucName;
        uc::sprintUPLUS(buf, ch.code);
        mywiki::appendCopyable(ucName, buf, "' style='" STYLE_BIGCOPY);
        ui->lbCharCode->setText(ucName);
    }

    // Block
    int iBlock = ui->comboBlock->currentIndex();
    auto block = uc::blockOf(ch.code, iBlock);
    int newIBlock = block->index();
    if (newIBlock != iBlock)
        ui->comboBlock->setCurrentIndex(newIBlock);
    ui->wiCollapse->setVisible(block->flags.have(uc::Bfg::COLLAPSIBLE));

    // Copy
    ui->btCopy->setEnabled(ch.hasCp());

    hint.sample = uc::blockOf(ch.code, hint.sample);
    if (ch) {
        if (ch->isTrueSpace()) {
                auto palette = this->palette();
                auto color = palette.windowText().color();
                color.setAlpha(ALPHA_SPACE);
                palette.setColor(QPalette::WindowText, color);
                ui->lbSample->setPalette(palette);
        } else {
            ui->lbSample->setPalette(this->palette());
        }

        // Sample char
        bool wantSysFont = true;
        switch (ch->drawMethod()) {
        case uc::DrawMethod::ABBREVIATION:
            clearSample();
            ui->stackSample->setCurrentWidget(ui->pageSampleCustom);
            ui->pageSampleCustom->setAbbreviation(ch->abbrev(), ch.code);
            wantSysFont = false;
            break;
        case uc::DrawMethod::SPACE: {
                clearSample();
                ui->stackSample->setCurrentWidget(ui->pageSampleCustom);
                auto& font = ch->font(hint.sample);
                auto& qfont = font.get(font.q.big, uc::FontPlace::SAMPLE, FSZ_BIG, ch.code);
                ui->pageSampleCustom->setSpace(qfont, ch.code);
            } break;
        case uc::DrawMethod::SAMPLE:
            drawSampleWithQt(*ch);
            break;
        }

        // OS char
        std::optional<QFont> font = std::nullopt;
        QFontDatabase::WritingSystem ws = QFontDatabase::Any;
        if (wantSysFont) {
            QString osProxy = ch->osProxy();
            if (osProxy.isEmpty()) {
                ui->lbOs->setFont(fontBig);
                ui->lbOs->setText({});
            } else {
                ws = ch->scriptEx(hint.sample).qtCounterpart;
                font = model.match.sysFontFor(*ch, ws, FSZ_BIG);
                if (font) {
                    ui->lbOs->setFont(*font);
                    ui->lbOs->setText(osProxy);
                } else {
                    ui->lbOs->setFont(fontBig);
                    ui->lbOs->setText("?");
                }
            }
        } else {
            ui->lbOs->setFont(fontBig);
            ui->lbOs->setText({});
        }

        QString text = mywiki::buildHtml(*ch, hint.sample, font, ws);
        ui->vwInfo->setText(text);
    } else {
        // No character
        ui->stackSample->setCurrentWidget(ui->pageSampleQt);
        ui->lbSample->setText({});
        ui->lbOs->setText({});
        if (uc::isNonChar(ch.code)) {
            QString text = mywiki::buildNonCharHtml(ch.code, hint.sample);
            ui->vwInfo->setText(text);
        } else {
            auto color = palette().color(QPalette::Disabled, QPalette::WindowText);
            QString text = mywiki::buildEmptyCpHtml(ch.code, color, hint.sample);
            ui->vwInfo->setText(text);
        }
    }
}


void FmMain::charChanged(const QModelIndex& current)
{
    showCp(model.charAt(current));
}


namespace {
    template <class T>
    [[deprecated]] inline void appendHeader(QString& text, const T& x)
    {
        str::append(text, "<p><nobr><b>");
        str::append(text, x.locName);
        str::append(text, "</b> ("sv);
        str::append(text, x.nChars);
        str::append(text, " шт.)</nobr></p>");
    }

}   // anon namespace


void FmMain::popupAtAbs(
        QWidget* widget, const QRect& absRect, const QString& html)
{
    popup.ensure(this)
         .setText(html)
         .popupAtAbsBacked(widget, absRect);
}

void FmMain::copyTextAbs(
        QWidget* widget, const QRect& absRect, const QString& text)
{
    QApplication::clipboard()->setText(text);
    showCopied(widget, absRect);
}


FontList FmMain::allSysFonts(
        char32_t cp, QFontDatabase::WritingSystem ws, size_t maxCount)
    { return model.match.allSysFonts(cp, ws, maxCount); }


void FmMain::linkClicked(
        std::string_view link, QWidget* widget, TinyOpt<QRect> rect)
{
    mywiki::go(widget, rect, *this, link);
}


void FmMain::anchorClicked(const QUrl &arg)
{
    auto str = arg.url().toStdString();
    auto snd = qobject_cast<QTextBrowser*>(sender());
    auto rect = snd->cursorRect();
    // Get some parody for link rect
    // Unglitch: we don’t know how to get EXACT coords of link,
    // so improvise somehow
    rect.setLeft(rect.left() - 80);

    linkClicked(str, snd, rect);
}


void FmMain::popupLinkActivated(const QString& link)
{
    // nullptr & {} = last position
    linkClicked(link.toStdString(), nullptr, {});
}


void FmMain::labelLinkActivated(const QString& link)
{
    QRect rect;
    auto snd = qobject_cast<QWidget*>(sender());
    if (snd) {
        rect = snd->rect();
    } else {
        snd = this;
        rect = QRect(snd->rect().center(), QSize{1, 1});
    }
    mywiki::go(snd, rect, *this, link.toStdString());
    // Deselect, does not influence double and triple clicks
    if (auto label = qobject_cast<QLabel*>(sender())) {
        label->setSelection(0, 0);
    }
}


void FmMain::slotSetFocusDefered(QWidget* wi)
{
    wi->setFocus();
}


void FmMain::on_comboBlock_currentIndexChanged(int index)
{
    if (index < 0)
        return;
    auto oldChar = model.charAt(ui->tableChars->currentIndex());
    auto oldBlock = uc::blockOf(oldChar.code, index);
    if (oldBlock->index() != static_cast<size_t>(index)) {
        auto& newBlock = uc::blocks[index];
        selectChar(newBlock.firstAllocated->subj);
        emit setFocusDefered(ui->tableChars);
    }
}


void FmMain::selectChar(char32_t code)
{
    auto index = model.indexOf(code);
    ui->tableChars->setCurrentIndex(index);
    ui->tableChars->scrollTo(index);
}


void FmMain::cjkSetCollapseState(bool x)
{
    if (model.isCjkCollapsed == x)
        return;
    model.isCjkCollapsed = x;
    model.build();
    cjkReflectCollapseState();
}


void FmMain::selectCharEx(char32_t code)
{
    if (model.isCharCollapsed(code)) {
        cjkSetCollapseState(false);
    }
    selectChar(code);
}


void FmMain::cjkReflectCollapseState()
{
    if (model.isCjkCollapsed) {
        ui->lbCollapse->setText(str::toQ(u8"ККЯ свёрнуты (кроме слоговых азбук и маленьких блоков)."sv));
        ui->btCollapse->setText(str::toQ(u8"Развернуть"sv));
    } else {
        ui->lbCollapse->setText(str::toQ(u8"ККЯ развёрнуты."sv));
        ui->btCollapse->setText(str::toQ(u8"Свернуть"sv));
    }
}


void FmMain::cjkExpandCollapse()
{
    // Remember current char, index and offset
    auto oldIndex = ui->tableChars->currentIndex();
    auto cp = model.charAt(oldIndex);
    auto scrollTop = ui->tableChars->verticalHeader()->logicalIndexAt(0);
    auto scrollOffset = std::max(0, oldIndex.row() - scrollTop);

    // Rebuild model
    cjkSetCollapseState(!model.isCjkCollapsed);

    // Generate new index
    auto newIndex = model.indexOf(cp.code);
    auto newIndex2 = model.index(newIndex.row(), oldIndex.column());
    auto newScrollTop = std::max(0, newIndex2.row() - scrollOffset);
    auto newTopIndex = model.index(newScrollTop, 0);

    // UI changes
    ui->tableChars->setCurrentIndex(newIndex2);
    ui->tableChars->scrollTo(newTopIndex, QAbstractItemView::PositionAtTop);
    ui->tableChars->scrollTo(newIndex2);
    ui->tableChars->viewport()->setFocus();
}


void FmMain::installTempPrefix()
{
    tempPrefix = model.match.findPrefix();
}


void FmMain::showTofuStats()
{
    int nGoodP0 = 0, nTofuP0 = 0,
        nGoodP1 = 0, nTofuP1 = 0,
        nGoodCjk = 0, nTofuCjk = 0,
        nGoodRest = 0, nTofuRest = 0,
        firstTofuCjk = 0, firstTofuRest = 0;
    const uc::Block* hint = &uc::blocks[0];
    for (size_t i = 0; i < uc::N_CPS; ++i) {
        auto& cp = uc::cpInfo[i];
        auto code = cp.subj.uval();
        switch (cp.tofuState(hint)) {
        case uc::TofuState::NO_FONT: break;
        case uc::TofuState::TOFU:
            if (code <= 65535)
                ++nTofuP0;
                else ++nTofuP1;
            if (hint->flags.have(uc::Bfg::COLLAPSIBLE)) {
                ++nTofuCjk;
                if (firstTofuCjk == 0)
                    firstTofuCjk = code;
            } else {
                ++nTofuRest;
                if (firstTofuRest == 0)
                    firstTofuRest = code;
            }
            break;
        case uc::TofuState::PRESENT:
            if (code <= 65535)
                ++nGoodP0;
                else ++nGoodP1;
            if (hint->flags.have(uc::Bfg::COLLAPSIBLE)) {
                ++nGoodCjk;
            } else {
                ++nGoodRest;
            }
            break;
        }
    }
    char buf[300];
    int nTotalP0 = nGoodP0 + nTofuP0;
    int nTotalP1 = nGoodP1 + nTofuP1;
    int nTotalCjk = nGoodCjk + nTofuCjk;
    int nTotalRest = nGoodRest + nTofuRest;
    snprintf(buf, std::size(buf),
             "Basic (P0): %d good, %d tofu, %d total" "\n"
             "Supp (P1+): %d good, %d tofu, %d total" "\n"
             "CJK: %d good, %d tofu, %d total" "\n"
             "Rest: %d good, %d tofu, %d total" "\n"
             "First tofu: CJK %04X, rest %04X",
             nGoodP0, nTofuP0, nTotalP0,
             nGoodP1, nTofuP1, nTotalP1,
             nGoodCjk, nTofuCjk, nTotalCjk,
             nGoodRest, nTofuRest, nTotalRest,
             firstTofuCjk, firstTofuRest);
    QMessageBox::information(this, "Tofu stats", buf);
}


void FmMain::openSearch()
{
    ui->stackSearch->setCurrentWidget(ui->pageSearch);
}


void FmMain::closeSearch()
{
    if (ui->stackSearch->currentWidget() == ui->pageSearch)
        ui->stackSearch->setCurrentWidget(ui->pageInfo);
}


void FmMain::startSearch()
{
    doSearch(ui->edSearch->text());
}


void FmMain::showSearchError(const QString& text)
{
    fmMessage.ensure(this)
             .showAtWidget(text, ui->edSearch);
}


void FmMain::showSearchResult(uc::SearchResult&& x)
{
    switch (x.err) {
    case uc::SingleError::ONE:
        closeSearch();
        selectCharEx(x.one->subj);
        break;
    case uc::SingleError::MULTIPLE: {
            searchModel.set(std::move(x.multiple));
            openSearch();
            ui->listSearch->setFocus();
            auto index0 = searchModel.index(0, 0);
            ui->listSearch->setCurrentIndex(index0);
            ui->listSearch->scrollTo(index0);
        } break;
    case uc::SingleError::NO_SEARCH:
        break;
    default:
        showSearchError(str::toQ(
                uc::errorStrings[static_cast<int>(x.err)]));
    }
}


void FmMain::doSearch(QString what)
{
    auto r = uc::doSearch(what);
    showSearchResult(std::move(r));
}


void FmMain::focusSearch()
{
    if (searchModel.hasData())
        openSearch();
}


void FmMain::searchEnterPressed(int index)
{
    selectCharEx(searchModel.cpAt(index).subj);
    ui->tableChars->setFocus();
}
