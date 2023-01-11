#include "IconEngines.h"

// Qt
#include <QPainter>
#include <QGuiApplication>
#include <private/qguiapplication_p.h>

// Char paint
#include "routines.h"
#include "Skin.h"

///// Veng /////////////////////////////////////////////////////////////////////

QPixmap ie::Veng::pixmap(
        const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    return scaledPixmap(size, mode, state, 1.0);
}

QPixmap ie::Veng::myScaledPixmap(const QSize &bigSize, QIcon::Mode mode, qreal scale)
{    
    QPixmap localPix;
    auto* workingPix = cache(scale);
    if (workingPix) {
        if (workingPix->size() == bigSize)
            return *workingPix;      // we rely on pixmap’s data sharing here
        // otherwise mutate existing pixmap
    } else {
        localPix = QPixmap{ bigSize };
        workingPix = &localPix;
    }
    workingPix->setDevicePixelRatio(1.0);
    workingPix->fill(Qt::transparent);
    { QPainter ptr(workingPix);
        // Paint in 100% (in pixels) here
        paint1(&ptr, QRect{ QPoint(0, 0), bigSize }, scale);
        // Paint in dipels here (none currently)
        //workingPix->setDevicePixelRatio(scale);
        //workingPix->setDevicePixelRatio(1.0);
    }

    if (qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        if (mode != QIcon::Normal) {
            const QPixmap generated = QGuiApplicationPrivate::instance()->applyQIconStyleHelper(mode, *workingPix);
            if (!generated.isNull()) {
                *workingPix = generated;
            }
        }
    }

    workingPix->setDevicePixelRatio(scale);
    return *workingPix;
}

QPixmap ie::Veng::scaledPixmap(
        const QSize &size, QIcon::Mode mode, QIcon::State, qreal scale)
{
    return myScaledPixmap(size, mode, scale);
}


void ie::Veng::paint(
        QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State)
{
    qreal scale = 1.0;
    if (painter->device())
        scale = painter->device()->devicePixelRatio();
    QSize sz { lround(rect.width() * scale), lround(rect.height() * scale) };
    int dw = 0;
    if (sz.width() > sz.height()) {
        sz.setWidth(sz.height());
        dw = (rect.width() - rect.height()) >> 1;
    }
    auto pix = myScaledPixmap(sz, mode, scale);
    painter->drawPixmap(rect.left() + dw, rect.top(), pix);
}


///// Cp ///////////////////////////////////////////////////////////////////////

ie::Cp::Cp(const PixSource& aSource, uc::EmojiDraw aEmojiDraw, const uc::Cp* aCp)
    : source(aSource), emojiDraw(aEmojiDraw), cp(aCp) {}

void ie::Cp::paint1(QPainter *painter, const QRect &rect, qreal scale)
{
    auto clFg = source.winColor();
    drawSearchChar(painter, rect, cp, clFg, emojiDraw, scale);
}


///// Nonchar //////////////////////////////////////////////////////////////////

void ie::Nonchar::paint1(QPainter *painter, const QRect &rect, qreal)
{
    auto clFg = source.winColor();
    drawCharBorder(painter, rect, clFg);
    QBrush brush (clFg, Qt::DiagCrossPattern);
    painter->fillRect(rect, brush);
}


///// CustomAbbr ///////////////////////////////////////////////////////////////

ie::CustomAbbr::CustomAbbr(const PixSource& aSource, const char8_t* aText)
    : source(aSource), text(aText) {}

void ie::CustomAbbr::paint1(QPainter *painter, const QRect &rect, qreal)
{
    auto clFg = source.winColor();
    drawCharBorder(painter, rect, clFg);
    drawAbbreviation(painter, rect, text, clFg);
}


///// Murky ////////////////////////////////////////////////////////////////////

void ie::Murky::paint1(QPainter *painter, const QRect &rect, qreal)
{
    auto clFg = source.winColor();
    drawMurkyRect(painter, rect, clFg);
}


///// Synth ////////////////////////////////////////////////////////////////////

ie::Synth::Synth(const PixSource& aSource, const uc::SynthIcon& aSi)
    : source(aSource), si(aSi) {}

void ie::Synth::paint1(QPainter *painter, const QRect &rect, qreal scale)
{
    // No continent → draw murky, otherwise use icon colours
    auto clFg = source.winColor();
    if (si.ecContinent == uc::EcContinent::NONE) {
        drawMurkyRect(painter, rect, clFg);
    } else {
        auto cont = si.continent();
        painter->fillRect(rect, cont.icon.bgColor);
        drawCharBorder(painter, rect, clFg);
        clFg = cont.icon.fgColor;
    }
    // Draw icon a bit larger — 120%
    drawChar(painter, rect, lround(120 * scale), si.cp(), clFg,
             TableDraw::CUSTOM, uc::EmojiDraw::CONSERVATIVE);
}

///// Node /////////////////////////////////////////////////////////////////////

ie::Node::Node(const PixSource& aSource, const uc::LibNode& aNode)
    : source(aSource), node(aNode) {}

void ie::Node::paint1(QPainter *painter, const QRect &rect, qreal scale)
{
    auto clFg = source.winColor();
    drawCharBorder(painter, rect, clFg);
    // draw char
    switch (node.value.length()) {
    case 0:
        drawFolderTile(painter, rect, node, clFg, scale);
        break;
    default:
        drawSearchChars(painter, rect, node.value, clFg, node.emojiDraw(), scale);
        break;
    }
}

///// BlockElem ////////////////////////////////////////////////////////////////

ie::BlockElem::BlockElem()
{
    texture.load(":Misc/blockelem.png");
}

void ie::BlockElem::paint1(QPainter *painter, const QRect &rect, qreal)
{
    // White BG
    painter->fillRect(rect, Qt::white);

    // Get width
    auto width = rect.height() * 3 / 5;
    if (width % 2 != 0)
        ++width;
    width = std::min(width, rect.width());
    // Get starting X
    auto dx = (rect.width() - width) >> 1;
    auto x0 = rect.left() + dx;
    // Top margin: top line of texture is empty, this precise 1
    // Bottom: 0/1 is somehow nicer than 1/2, let it be
    QRect newRect { x0, rect.top(), width, rect.height()};
    QBrush brush(texture);
    painter->fillRect(newRect, brush);
}


///// CoarseImage //////////////////////////////////////////////////////////////


ie::CoarseImage::CoarseImage(const QColor& aBg, const QSize& aMargins, const char* fname)
    : bg(aBg), margins(aMargins)
{
    texture.load(fname);
}


unsigned ie::CoarseImage::getMargin(unsigned side, unsigned value) noexcept
{
    if (value == 0)
        return 0;
    auto r = (side * value) / 24u;
    if (r == 0)
        r = 1;
    return r << 1;
}


void ie::CoarseImage::paint1(QPainter *painter, const QRect &rect, qreal scale)
{
    // Fill BG
    painter->fillRect(rect, bg);

    // Get rect
    unsigned side = std::lround(16.0 * scale - 0.1);  // 0 / 0.5px — sometimes we request a bit smaller icon
    auto mx = getMargin(side, margins.width());
    auto my = getMargin(side, margins.height());
    int times = std::min((side - mx) / texture.width(),
                         (side - my) / texture.height());
    if (times < 1)
        times = 1;
    int ww = texture.width() * times;
    int hh = texture.height() * times;
    int x0 = rect.left() + ((rect.width() - ww) >> 1);
    int y0 = rect.top() + ((rect.height() - hh) >> 1);

    QRect rcDest { x0, y0, ww, hh };
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter->drawPixmap(rcDest, texture);
}


///// Taixu ////////////////////////////////////////////////////////////////////

void ie::Taixu::paint1(QPainter *painter, const QRect &rect, qreal scale)
{
    painter->fillRect(rect, BG_CJK);

    unsigned side = std::lround(16.0 * scale - 2.1);
    auto hunit = std::max(1u, side / 7u);
    auto hh = hunit * 7;
    auto ww = hh;
    // Need remainder 0 or 2, bigger unit is a SPACE for dotted
    switch (ww % 5) {
    case 1:
    case 3: --ww; break;
    case 4: ww -= 2; break;
    default: ;  // 0,2
    }
    auto wSmallUnit = ww / 5;
    auto wBigUnit = wSmallUnit;
    if (ww % 5 != 0)
        ++wBigUnit;
    int x0 = rect.left() + ((rect.width() - ww) >> 1);
    int y0 = rect.top()  + ((rect.height() - hh) >> 1);
    int x1 = x0 + wSmallUnit;
    int x2 = x1 + wBigUnit;
    int x3 = x2 + wSmallUnit;
    int x4 = x3 + wBigUnit;
    int x5 = x4 + wSmallUnit;
    auto drawL = [&](int x1, int x2, int y) {
        int ystart = y0 + y * hunit;
        QRect r { x1, ystart, x2 - x1, static_cast<int>(hunit) };
        painter->fillRect(r, FG_CJK);
    };
    drawL(x0, x2, 0);                     drawL(x3, x5, 0);
                       drawL(x0, x5, 2);
    drawL(x0, x1, 4);  drawL(x2, x3, 4);  drawL(x4, x5, 4);
    drawL(x0, x1, 6);  drawL(x2, x3, 6);  drawL(x4, x5, 6);
}


///// Legacy ///////////////////////////////////////////////////////////////////

ie::Legacy::Legacy()
{
    texture.load(":Misc/legacy.png");
}


void ie::Legacy::paint1(QPainter *painter, const QRect &rect, qreal scale)
{
    // Fill BG
    painter->fillRect(rect, Qt::white);

    static constexpr unsigned MIN_WIDTH = 11;

    // Get rect
    unsigned side = std::lround(16.0 * scale - 0.1);  // 0 / 0.5px — sometimes we request a bit smaller icon
    int times = (side - 1) / MIN_WIDTH;
    if (times < 1)
        times = 1;
    int ww = texture.width() * times;
    int hh = texture.height() * times;
    int x0 = rect.left() + ((rect.width() - ww) >> 1);
    if (x0 < 1)
        x0 = 1;
    int y0 = rect.top() + ((rect.height() - hh) >> 1);

    QRect rcDest { x0, y0, ww, hh };
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter->drawPixmap(rcDest, texture);
}
