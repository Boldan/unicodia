#pragma once

// Qt
#include <QColor>

// Libs
#include "u_TypedFlags.h"

namespace uc {

    enum class EcContinent {
        NONE,
        TECH,       ///< For Unicode’s own needs
        EUROPE,     ///< Europe, incl. Georgia and Mediterranean
        ASIA,       ///< Mainland Asia
        CJK,        ///< Hani and derived scripts
        OCEAN,      ///< Indian and Pacific Ocean
        AFRICA,
        AMERICA,
        NN
    };

    struct Continent
    {
        // Colors used on 16×16 icons: BG and FG
        // We use the same colors for synthesized icons
        struct Icon {
            QColor bgColor, fgColor, frameColor;

            Icon() = delete;
            consteval Icon(const QColor& aBg, const QColor& aFg) noexcept
                : bgColor(aBg), fgColor(aFg), frameColor(aFg) {}
            consteval Icon(const QColor& aBg, const QColor& aFg, const QColor& aFrame) noexcept
                : bgColor(aBg), fgColor(aFg), frameColor(aFrame) {}
        } icon;

        struct Collapse {
            QColor bgColor, textColor;
        } collapse;

        bool isInternational;
    };
    extern const Continent continentInfo[];

    enum class Ifg {
        CONTINENT_OK      = 1<<0,   ///< [+] disable auto-check, continent is really OK
        MISSING           = 1<<1,   ///< [+] red icon, missing block
        CUSTOM_ENGINE     = 1<<2,   ///< [+] use custom engine in lo-res
        FORMAT            = 1<<3,   ///< [+] format char is on icon
        SMALLER           = 1<<4,   ///< [+] draw synth. icon smaller (legacy)
        ROTATE_LTR_CW     = 1<<5,   ///< [+] To display properly, 90°↷: Mong, Phag (→ in Unicode)
        ROTATE_RTL_CCW    = 1<<6,   ///< [+] To display properly, 90°↶: Sogd, Ougr (← in Unicode)
        SHIFT_LEFT        = 1<<7,   ///< [+] To display a synth. icon, shift it to the left
        SHIFT_RIGHT       = 1<<8,   ///< [+] To display a synth. icon, shift it to the right
        SHIFT_DOWN        = 1<<9,   ///< [+] To display a synth. icon, shift it down a bit
        // These flags are merely informational and do nothing,
        // and certify that the icon is synthesized approximately because of…
        APPROX_SQUARE     = 0,      ///< [+] block consists mostly of modifiers, and tofu of main char is drawn:
                                    ///<      Latin ex F, Cyr ex D
        APPROX_OTHER_LINES = 0,     ///< [+] other lines on icon: Georian Mtavruli
        APPROX_COLLECTIVE = 0,      ///< [+] graphic icon contains collective image, not specific char:
                                    ///<      variation selectors, tags
        APPROX_COLOR      = 0,      ///< [+] graphic icon is coloured/colourless:
                                    ///<      Psalter Pahlavi, board games, CJK emoji
        APPROX_HISTORICAL = 0,      ///< [+] icon is from historical font with © issues:
                                    ///<      Cpmn, Tang, Kits
        // Synthesized icon is BIG, at least 39px, and we CAN afford drawing
        // dotted circle completely → no flag for such approximation
    };
    DEFINE_ENUM_OPS(Ifg)

    struct TwoChars {
        char32_t v[2];

        consteval TwoChars(char32_t x) : v{x, 0} {}
        consteval TwoChars(const char32_t x[3]) : v{x[0], x[1]} {}

        unsigned length() const { return (v[1] != 0) + 1; }
        std::u32string_view sv() const { return {v, length()}; }
    };

    enum class ImbaX : signed char {
        PERFECT = 0,
        LEFT_1 = -1, LEFT_2 = -2, LEFT_3 = -3, LEFT_4 = -4,
        RIGHT_1 = 1, RIGHT_2 = 2, RIGHT_3 = 3, RIGHT_4 = 4,
    };

    enum class ImbaY : signed char {
        PERFECT = 0,
        ABOVE_1 = -1, ABOVE_2 = -2, ABOVE_3 = -3, ABOVE_4 = -4,
        BELOW_1 =  1, BELOW_2 =  2, BELOW_3 =  3, BELOW_4 =  4,
    };

    struct SvgHint {
        /// Hint’s position: y=0 → none; y=5 → align that line to pixels
        /// WHY: when enlarging to hiDPI, you need to hold the primary line to
        ///    physical pixels.
        /// e.g. for Devanagari KA primary lines are top and left/right
        ///    side of vertical stem
        struct Pos  { uint8_t x = 0, y = 0; } pos;

        /// Picture imbalance: y=0 → none; y=4 → the letter is drawn 0.4dip below
        ///    e.g. BELOW_4 = first you move 0.4px up, then align to pixels
        /// WHY: to hold the primary line aligned to pixels at primary resolution
        ///    16×16 device-independent pixels, you need to imbalance the picture
        ///    sometimes. E.g. got height of 11 → top is either 2 or 3.
        ///    When top is 2, use ABOVE_Y. Top is 3 → BELOW_Y.
        /// How big is Y — decide for yourself by subjective balance
        /// That’s why no ±0.5: the author preferred 2 to 3 (or vice-versa) for some reason
        /// Makes no sense when this coordinate does not have a primary line.
        struct Imba {  int8_t x = 0, y = 0; } imba;

        using BiggerType = uint16_t;
        static_assert(sizeof(BiggerType) == sizeof(Pos), "BiggerType wrong");

        constexpr SvgHint() = default;
        explicit constexpr SvgHint(uint8_t aX, uint8_t aY) : pos{.x = aX, .y = aY } {}
        explicit constexpr SvgHint(uint8_t aX, uint8_t aY, ImbaY imbaY)
            : pos{.x = aX, .y = aY }, imba { .y = static_cast<int8_t>(imbaY) } {}
        explicit constexpr SvgHint(uint8_t aX, uint8_t aY, ImbaX imbaX)
            : pos{.x = aX, .y = aY }, imba { .x = static_cast<int8_t>(imbaX) } {}
        explicit constexpr SvgHint(uint8_t aX, ImbaX imbaX)
            : pos{.x = aX }, imba { .x = static_cast<int8_t>(imbaX) } {}
        explicit constexpr SvgHint(uint8_t aY, ImbaY imbaY)
            : pos{.y = aY }, imba { .y = static_cast<int8_t>(imbaY) } {}

        operator bool() const { return std::bit_cast<BiggerType>(pos); }

        static constexpr int SIDE = 16;
        static constexpr double RECIPSIDE = 1.0 / SIDE;     // this is a precise float!
        /// hint’s relative position (0..1) baked into picture
        constexpr double bakedQX() const noexcept { return static_cast<double>(pos.x) * RECIPSIDE; }
        constexpr double bakedQY() const noexcept { return static_cast<double>(pos.y) * RECIPSIDE; }
        /// hint’s relative position (0..1) with balance fixup added
        constexpr double balancedQX() const noexcept { return (pos.x - 0.1 * imba.x) * RECIPSIDE; }
        constexpr double balancedQY() const noexcept { return (pos.y - 0.1 * imba.y) * RECIPSIDE; }
    };

    ///
    /// \brief The SynthIcon class
    ///    Initially was a description of synthesized icon in Search.
    ///    Currently it describes lo-res (16×16 dip) icon in Blocks too.
    ///
    struct SynthIcon
    {
        TwoChars subj;              ///< character(s) drawn on an icon
        EcContinent ecContinent;    ///< continent (colour scheme)
        Flags<Ifg> flags {};        ///< misc. flags (both to synthesized and lo-res)
        SvgHint svgHint { 0, 0 };   ///< hinting of lo-res icon

        /// @return  Continent, never missing
        const Continent& normalContinent() const
            { return continentInfo[static_cast<int>(ecContinent)]; }

        /// @return  Continent, maybe missing
        const Continent& maybeMissingContinent() const;
    };



}   // namespace uc
