#include "forget.h"

// STL
#include <unordered_map>
#include <fstream>

// Libs
#include "u_Strings.h"

namespace {

    enum class ForgetChannel { SCRIPT, LETTER, BAN };

    const std::unordered_map<std::string_view, ForgetChannel> forgetWords {
        // Script channel
        { "LATIN", ForgetChannel::SCRIPT },
        { "MODIFIER", ForgetChannel::SCRIPT },
        { "SUPERSCRIPT", ForgetChannel::SCRIPT },
        { "SUBSCRIPT", ForgetChannel::SCRIPT },
        { "MATHEMATICAL", ForgetChannel::SCRIPT },
        { "DOUBLE-STRUCK", ForgetChannel::SCRIPT },
        { "SCRIPT", ForgetChannel::SCRIPT },
        { "BLACK-LETTER", ForgetChannel::SCRIPT },

        // Letter channel
        { "A", ForgetChannel::LETTER },
        { "AA", ForgetChannel::LETTER },
        { "AE", ForgetChannel::LETTER },
        { "ALPHA", ForgetChannel::LETTER },
        { "B", ForgetChannel::LETTER },
        { "C", ForgetChannel::LETTER },
        { "D", ForgetChannel::LETTER },
        { "DZ", ForgetChannel::LETTER },
        { "DEZH", ForgetChannel::LETTER },
        { "E", ForgetChannel::LETTER },
        { "SCHWA", ForgetChannel::LETTER },
        { "F", ForgetChannel::LETTER },
        { "FENG", ForgetChannel::LETTER },
        { "G", ForgetChannel::LETTER },
        { "H", ForgetChannel::LETTER },
        { "I", ForgetChannel::LETTER },
        { "IJ", ForgetChannel::LETTER },
        { "IOTA", ForgetChannel::LETTER },
        { "J", ForgetChannel::LETTER },
        { "K", ForgetChannel::LETTER },
        { "L", ForgetChannel::LETTER },
        { "LJ", ForgetChannel::LETTER },
        { "LEZH", ForgetChannel::LETTER },
        { "M", ForgetChannel::LETTER },
        { "N", ForgetChannel::LETTER },
        { "NJ", ForgetChannel::LETTER },
        { "ENG", ForgetChannel::LETTER},
        { "NUM", ForgetChannel::LETTER},
        { "O", ForgetChannel::LETTER },
        { "OE", ForgetChannel::LETTER },
        { "P", ForgetChannel::LETTER },
        { "Q", ForgetChannel::LETTER },
        { "R", ForgetChannel::LETTER },
        { "S", ForgetChannel::LETTER },
        { "ESH", ForgetChannel::LETTER },
        { "T", ForgetChannel::LETTER },
        { "ETH", ForgetChannel::LETTER },
        { "TESH", ForgetChannel::LETTER },
        { "TS", ForgetChannel::LETTER },
        { "TZ", ForgetChannel::LETTER },
        { "U", ForgetChannel::LETTER },
        { "V", ForgetChannel::LETTER },
        { "W", ForgetChannel::LETTER },
        { "X", ForgetChannel::LETTER },
        { "Y", ForgetChannel::LETTER },
        { "Z", ForgetChannel::LETTER },
        { "EZH", ForgetChannel::LETTER},

        // Banned
        { "CYRILLIC", ForgetChannel::BAN },
        { "SAMARITAN", ForgetChannel::BAN },
        { "FULLWIDTH", ForgetChannel::BAN },
        { "COMBINING", ForgetChannel::BAN },
        { "BALLOT", ForgetChannel::BAN },       // Some UI character
    };

    const std::unordered_map<char32_t, bool> specialCps {
        { 0x00A9,  true  },  // ©
        { 0x00AE,  true  },  // ®
        { 0x0149,  false },  // apos+n, banned from Unicode
        { 0x210E,  true  },  // Planck constant, italic h
        { 0x210F,  true  },  // Same with bar
        { 0x2183,  true  },  // Number form, also letter reversed C
        { 0x1F1AD, true  },  // mask work symbol
    };

    const std::unordered_map<char32_t, unsigned> wantedRepeats {
        { 0x2113, 2 },      // ell symbol
        { 0x2145, 2 },      // double-struck italic
        { 0x2146, 2 },      // same
        { 0x2147, 2 },      // same
        { 0x2148, 2 },      // same
        { 0x2149, 2 },      // same
    };

}   // anon namespace


bool forget::isIn(char32_t cp, std::string_view name)
{
    // Ranges where never in
    if ((cp <= 127)                             // ASCII
            || (cp >= 0x1D6A8 && cp <= 0x1D7CB) // Math Greek
            || (cp >= 0x2'0000))                // Plane 2+
        return false;
    // Special?
    auto it = specialCps.find(cp);
    if (it != specialCps.end())
        return it->second;
    // Stock name → never in
    if (name.find('#') != std::string_view::npos)
        return false;
    // Split into words
    // Should contain both script and letter, should not contain ban
    auto upCase = str::toUpper(name);
    auto words = str::splitSv(upCase, ' ');
    bool hasScript = false, hasLetter = false;
    for (auto word : words) {
        auto q = forgetWords.find(word);
        if (q != forgetWords.end()) {
            switch (q->second) {
            case ForgetChannel::SCRIPT:
                hasScript = true; break;
            case ForgetChannel::LETTER:
                hasLetter = true; break;
            case ForgetChannel::BAN:
                return false;
            }
        }
    }
    return hasScript && hasLetter;
}


namespace {

    /// Register CP in forgotten base
    void reg(forget::Cp& q, std::string_view name, char32_t capital, bool isIn)
    {
        q.database.name = name;
        q.database.capital = capital;
        q.computed.isIn = isIn;
    }

}   // anon namespace


void forget::processCp(Map& map, char32_t cp, std::string_view name, char32_t capital)
{
    if (isIn(cp, name)) {
        auto& q = map[cp];
        reg(q, name, capital, true);
    } else {
        auto it = map.find(cp);
        if (it != map.end()) {
            reg(it->second, name, capital, false);
        }
    }
}


namespace {

    /// Section manager: automatically writes header
    class SectionMan
    {
    public:
        explicit SectionMan(std::ostream& aOs) noexcept : os(aOs) {}
        void arm(std::string aHead);
        void trigger();
        void disarm();
        bool hasEverTriggered() const { return fHasEverTriggered; }
    private:
        enum class State { SAFE, ARMED, TRIGGERED };

        std::ostream& os;
        std::string head;
        State state = State::SAFE;
        bool fHasEverTriggered = false;
    };

    void SectionMan::arm(std::string aHead)
    {
        disarm();
        head = std::move(aHead);
        state = State::ARMED;
    }

    void SectionMan::trigger()
    {
        if (state == State::ARMED) {
            os << head << '\n';
            state = State::TRIGGERED;
            fHasEverTriggered = true;
        }
    }

    void SectionMan::disarm()
    {
        if (state == State::TRIGGERED) {
            os << '\n';
        }
        state = State::SAFE;
    }
}


forget::Stats forget::postprocess(const Map& map, const char* fname)
{
    std::ofstream os(fname);
    SectionMan secMan(os);

    auto print = [&os, &secMan](const Map::value_type& val) {
        // Trigger
        secMan.trigger();
        // Print code
        char q[20];
        snprintf(q, std::size(q), "%04X", static_cast<unsigned>(val.first));
        os << q;
        // Print name
        if (!val.second.database.name.empty()) {
            os << ' ' << val.second.database.name;
        }
    };

    auto println = [&os, &print](const Map::value_type& val) {
        print(val);
        os << '\n';
    };

    Stats r;

    secMan.arm("REPEATING");
    for (auto& q : map) {
        if (wantedRepeats.find(q.first) == wantedRepeats.end()) {
            if (q.second.lib.count > 1) {
                println(q);
                ++r.nRepeat;
            }
        }
    }

    secMan.arm("MISMATCHED REPEAT");
    for (auto& q : map) {
        auto it = wantedRepeats.find(q.first);
        if (it != wantedRepeats.end()) {
            if (q.second.lib.count != it->second) {
                print(q);
                os << ": wanted " << it->second << ", found " << q.second.lib.count << '\n';
                ++r.nRepeat;
            }
        }
    }

    secMan.arm("MISSING");
    for (auto& q : map) {
        if (q.second.lib.count == 0 && q.second.computed.isIn) {
            println(q);
            ++r.nMissing;
        }
    }

    secMan.arm("EXTRANEOUS");
    for (auto& q : map) {
        if (q.second.lib.count > 0 && !q.second.computed.isIn) {
            println(q);
            ++r.nExtra;
        }
    }

    if (!secMan.hasEverTriggered()) {
        os << "No forgotten codepoints." << '\n';
    }
    return r;
}
