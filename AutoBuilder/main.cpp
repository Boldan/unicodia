// C++
#include <iostream>
#include <fstream>
#include <charconv>
#include <deque>
#include <unordered_set>

// PugiXML
#include "pugixml.hpp"

// My libs
#include "u_Strings.h"

// Unicode
#include "UcCp.h"

// Project-local
#include "ucdcom.h"
#include "data.h"
#include "egyptian.h"
#include "entities.h"
#include "library.h"
#include "loader.h"
#include "sutton.h"
#include "textbase.h"

using namespace std::string_view_literals;

template <class T>
inline auto need(T&& val, const char* errmsg)
{
    if (!val) throw std::logic_error(errmsg);
    return std::forward<T>(val);
}

struct Numeric {
    long long num, denom;
    long long altInt = 0;
    std::string_view type, textValue;
    size_t index;
};

long long fromChars(std::string_view x, std::string_view numType)
{
    long long r;
    auto beg = x.data();
    auto end = beg + x.length();
    auto res = std::from_chars(beg, end, r);
    if (res.ec != std::errc() || res.ptr != end) {
        std::string errm = "[fromChars] Cannot parse number ";
        errm.append(x);
        errm.append(", numType = ");
        errm.append(numType);
        throw std::invalid_argument(errm);
    }
    return r;
}

Numeric parseNumeric(std::string_view numType, std::string_view x, size_t index)
{
    if (x == "NaN"sv) {
        return {
            .num = 0,
            .denom = 0,
                    .altInt = 0,
            .type = numType,
            .textValue = x,
            .index = index };
    }

    long long altNum = 0;
    auto pAlt = x.find(' ');
    if (pAlt != std::string_view::npos) {
        auto sAlt = str::trimSv(x.substr(pAlt + 1));
        altNum = fromChars(sAlt, numType);
        x = x.substr(0, pAlt);
    }

    auto pSlash = x.find('/');
    if (pSlash == std::string_view::npos) {
        // Integer
        return {
            .num = fromChars(x, numType),
            .denom = 1,
            .altInt = altNum,
            .type = numType,
            .textValue = x,
            .index = index };
    } else {
        // Fraction
        auto num = x.substr(0, pSlash);
        auto den = x.substr(pSlash + 1);
        return {
            .num = fromChars(num, numType),
            .denom = fromChars(den, numType),
            .altInt = altNum,
            .type = numType,
            .textValue = x,
            .index = index };
    }
}

struct NumCache
{
    std::deque<Numeric> ord;
    std::map<std::string, const Numeric*> ndx;

    const Numeric& parse(std::string_view numType, std::string_view x);
    size_t size() const { return ord.size(); }
};

const Numeric& NumCache::parse(std::string_view numType, std::string_view x)
{
    // Get key
    std::string key;
    key.reserve(numType.length() + x.length());
    key.append(numType);
    key.append(x);
    // Check index
    auto it = ndx.find(key);
    if (it != ndx.end())
        return *it->second;
    // Insert
    Numeric newNum = parseNumeric(numType, x, ord.size());
    auto& newPlace = ord.emplace_back(newNum);
    ndx[key] = &newPlace;
    return newPlace;
}

    std::string transformVersion(std::string s)
{
    for (auto& c : s) {
        if (c == '.')
            c = '_';
    }
    return s;
}

inline std::string transformVersion(std::string_view s) { return transformVersion(std::string{s}); }

struct StrMap {
    std::string_view source, image;
};

template <size_t N>
std::string_view transform(std::string_view x, StrMap (&map)[N])
{
    for (auto& v : map) {
        if (v.source == x) {
            return v.image;
        }
    }
    std::string errm = "[transform] Bad string ";
    errm.append(x);
    throw std::invalid_argument(std::move(errm));
}

StrMap smNumType[] {
    { "De"sv, "DIGIT" },
    { "Di"sv, "SPECIAL_DIGIT" },
    { "None"sv, "NONE" },
    { "Nu"sv, "NUMBER" },
};
StrMap smCharCat[] {
    { "Cc"sv, "CONTROL"sv },
    { "Cf"sv, "FORMAT"sv },
    { "Ll"sv, "LETTER_LOWERCASE"sv },
    { "Lm"sv, "LETTER_MODIFIER"sv },
    { "Lo"sv, "LETTER_OTHER"sv },
    { "Lt"sv, "LETTER_TITLECASE"sv },
    { "Lu"sv, "LETTER_UPPERCASE"sv },
    { "Mc"sv, "MARK_SPACING"sv },
    { "Me"sv, "MARK_ENCLOSING"sv },
    { "Mn"sv, "MARK_NONSPACING"sv },
    { "Nd"sv, "NUMBER_DECIMAL"sv },
    { "Nl"sv, "NUMBER_LETTER"sv },
    { "No"sv, "NUMBER_OTHER"sv },
    { "Pc"sv, "PUNCTUATION_CONNECTOR"sv },
    { "Pd"sv, "PUNCTUATION_DASH"sv },
    { "Pe"sv, "PUNCTUATION_CLOSE"sv },
    { "Pf"sv, "PUNCTUATION_FINAL"sv },
    { "Pi"sv, "PUNCTUATION_INITIAL"sv },
    { "Po"sv, "PUNCTUATION_OTHER"sv },
    { "Ps"sv, "PUNCTUATION_OPEN"sv },
    { "Sc"sv, "SYMBOL_CURRENCY"sv },
    { "Sk"sv, "SYMBOL_MODIFIER"sv },
    { "Sm"sv, "SYMBOL_MATH"sv },
    { "So"sv, "SYMBOL_OTHER"sv },
    { "Zl"sv, "SEPARATOR_LINE"sv },
    { "Zp"sv, "SEPARATOR_PARAGRAPH"sv },
    { "Zs"sv, "SEPARATOR_SPACE"sv },
};

enum class AbbrevState { NORMAL, ALIAS, DISABLE };


struct StringData
{
    std::string s;
    char32_t subj;
    int offset;
    uc::TextRole role;
    bool isLast;
};

struct RememberResult
{
    StringData* data;
    bool wasIns;    
};

class StringLib
{
public:
    RememberResult remember(
            char32_t subj,
            uc::TextRole role,
            std::string_view s);
    RememberResult forceRemember(
            char32_t subj,
            uc::TextRole role,
            std::string_view s);
    void finishCp();
    auto& inOrder() const { return fInOrder; }
private:
    using M = std::unordered_map<std::string_view, StringData*>;
    M fNdx;
    std::deque<StringData> fInOrder;
    size_t fLength = 0;
};

RememberResult StringLib::forceRemember(
        char32_t subj, uc::TextRole role, std::string_view aS)
{
    auto& v = fInOrder.emplace_back(StringData{
                    .s = std::string { aS },
                    .subj = subj,
                    .offset = static_cast<int>(fLength),
                    .role = role,
                    .isLast = false });
    fLength += (aS.length() + 2);
    return { .data =&v, .wasIns = true };
}

RememberResult StringLib::remember(
        char32_t subj, uc::TextRole role, std::string_view s)
{
    if (role != uc::TextRole::MAIN_NAME)
        return forceRemember(subj, role, s);
    auto it = fNdx.find(s);
    if (it == fNdx.end()) {   // Was inserted
        auto r = forceRemember(subj, role, s);
        fNdx[r.data->s] = r.data;
        return r;
    } else {
        return {
            .data = it->second,
            .wasIns = false
        };
    }
}


void StringLib::finishCp()
{
    if (!fInOrder.empty()) {
        auto& bk = fInOrder.back();
        if (!bk.isLast) {
            bk.isLast = true;
            // +1: command CMD_END
            ++fLength;
        }
    }
}


bool hasSubstr(std::string_view haystack, std::string_view needle)
{
    auto pos = haystack.find(needle);
    return (pos != std::string_view::npos);
}


class NewLine
{
public:
    void trigger();
private:
    bool isCocked = true;
};


void NewLine::trigger()
{
    if (isCocked) {
        std::cout << std::endl;
        isCocked = false;
    }
}


struct NotoData {
    std::unordered_set<char32_t> singleChar;
};


//inline std::string_view chompPrefSuffSv(
//        std::string_view x, std::string_view pref, std::string_view suff)
//{
//    auto len2 = pref.length() + suff.length();
//    if (x.length() > len2 && x.starts_with(pref) && x.ends_with(suff)) {
//        return x.substr(pref.length(), x.length() - len2);
//    } else {
//        return {};
//    }
//}


//inline std::string_view chompPref(std::string_view x, std::string_view pref)
//{
//    if (x.starts_with(pref)) {
//        return x.substr(pref.length());
//    } else {
//        return {};
//    }
//}


NotoData loadNotoEmoji()
{
    NotoData r;
    std::ifstream is(SINGLEEMOJI_TXT);
    std::string s;
    while (std::getline(is, s)) {
        // Single-char emoji
        auto code = fromHex(s);
        r.singleChar.insert(code);
    }
    return r;
}


namespace {
    tx::Cp DUMMY_CP;
}   // anon namespace


std::string encodeC(std::string_view x)
{
    std::string r;
    r.reserve(x.size());
    for (auto c : x) {
        switch (c) {
        case '"':
            r += R"(\")";
            break;
        case '\\':
            r += R"(\\)";
            break;
        default:
            r += c;
            break;
        }
    }
    return r;
}


int main()
{
    std::cout << "Have " << dictionary.size() << " words in dictionary, "
              << nExceptions() << " exceptions." << '\n';

    ///// Loader ///////////////////////////////////////////////////////////////
    ///
    std::cout << "Checking for loader..." << std::flush;
    try {
        checkLoader();
        std::cout << "OK" << '\n';
    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << '\n';
        return 1;
    }

    ///// HTML entities ////////////////////////////////////////////////////////

    std::cout << "Loading HTML entities..." << std::flush;
    auto htmlEntities = readEntities(ENTITIES_JSON);
    std::cout << "OK, " << htmlEntities.data.size() << " chars, "
                        << htmlEntities.stats.entities.nGood << " good en's, "
                        << htmlEntities.stats.entities.nTooLong << " too long (2+) en's, "
                        << htmlEntities.stats.entities.nWrong << " wrong en's." << std::endl;

    ///// Noto emoji ///////////////////////////////////////////////////////////

    std::cout << "Loading Noto emoji list..." << std::flush;
    NotoData noto = loadNotoEmoji();
    std::cout << "OK, " << noto.singleChar.size() << " single-char emoji." << '\n';

    ///// Property list ////////////////////////////////////////////////////////

    std::cout << "Loading Unicode property list..." << std::flush;
    const ucd::PropBase propBase = ucd::loadPropBase();
    std::cout << "OK, " << propBase.nScripts() << " scripts." << '\n';

    ///// Script data //////////////////////////////////////////////////////////

    std::cout << "Loading Unicode script list..." << std::flush;
    const tx::Scripts scripts = tx::loadScripts(propBase);
    std::cout << "OK, " << scripts.size() << " lines." << '\n';

    ///// Age data /////////////////////////////////////////////////////////////

    std::cout << "Loading Unicode ages..." << std::flush;
    const tx::Ages ages = tx::loadAges();
    std::cout << "OK, " << ages.size() << " lines." << '\n';

    ///// Bracket //////////////////////////////////////////////////////////////

    std::cout << "Loading Unicode bidi mirroring..." << std::flush;
    const tx::Mirroring mirroring = tx::loadMirroring();
    std::cout << "OK, " << mirroring.size() << " chars." << '\n';

    ///// Emoji ////////////////////////////////////////////////////////////////

    std::cout << "Loading Unicode emoji table..." << std::flush;
    auto emoji = lib::loadEmoji(EMOJI_TEST);
    std::cout << "OK, " << emoji.count << " emoji, " << emoji.vs16.size() << " are VS16." << '\n';

    lib::StrangeCjk strangeCjk;

    ///// Egyptian /////////////////////////////////////////////////////////////

    std::cout << "Loading Egyptian base..." << std::flush;
    auto egypBase = egyp::loadBase();
    std::cout << "OK, " << egypBase.size() << " hieros." << '\n';

    ///// Library //////////////////////////////////////////////////////////////

    std::cout << "Loading manual library..." << std::flush;
    auto manualLib = lib::loadManual(LIBRARY_XML);
    std::cout << "OK, " << manualLib.forgetMap.size() << " CPs in forget checker." << '\n';

    ///// text base ////////////////////////////////////////////////////////////

    std::cout << "Loading Unicode text base..." << std::flush;
    auto textBase = tx::loadBase();
    std::cout << "OK, " << textBase.size() << " text objects." << '\n';

    ///// Open output file /////////////////////////////////////////////////////

    std::ofstream os("UcAuto.cpp");
    os << "// Automatically generated, do not edit!" << '\n';
    os << '\n';
    os << R"(#include "UcAutoDefines.h")" << '\n';
    os << '\n';

    pugi::xml_document doc;

    std::cout << "Loading Unicode XML base..." << std::flush;
    doc.load_buffer(memXml.data(), memXml.size());
    std::cout << "OK" << '\n';

    ///// CpInfo ///////////////////////////////////////////////////////////////

    unsigned nChars = 0, nSpecialRanges = 0;

    auto elRoot = need(doc.root().child("ucd"), "Need <ucd>");
    auto elRepertoire = need(elRoot.child("repertoire"), "Need <repertoire>");
    std::cout << "Found repertoire, generating character info..." << std::flush;
    NewLine nl;
    os << '\n';
    os << R"(uc::Cp uc::cpInfo[N_CPS] {)" << '\n';

    StringLib strings;
    NumCache nums;
    int nDeprecated = 0;
    for (pugi::xml_node elChar : elRepertoire.children("char")) {
        /// @todo [urgent] get CP → column 0
        std::string_view sCp = elChar.attribute("cp").as_string();
        if (sCp.empty()) {
            ++nSpecialRanges;
            continue;
        }
        // Get CP
        auto cp = fromHex(sCp);

        // Aliases:
        // • Abbreviation: Implement later
        // • Alternate: Prefer na1
        // • Control: Prefer na1
        // • Figment: Implement

        /// @todo [urgent] get name, replace code with # → column 1
        std::string_view sName = elChar.attribute("na").as_string();

        std::string_view defaultAbbrev {};      // empty

        AbbrevState abbrevState = AbbrevState::NORMAL;
        if (auto it = abbrevs.find(cp); it != abbrevs.end()) {
            if (it->second.empty()) {
                abbrevState = AbbrevState::DISABLE;
            } else {
                if (it->second == "!"sv) {
                    abbrevState = AbbrevState::ALIAS;
                } else {
                    defaultAbbrev = it->second;
                }
            }
        }

        // Find other names in text base
        auto itCp = textBase.find(cp);
        tx::Cp* textCp = &DUMMY_CP;
        bool hasTextEntry = false;
        if (itCp != textBase.end()) {
            textCp = &itCp->second;
            hasTextEntry = true;
        }

        // Get corrected name
        std::string sLowerName;
        if (!textCp->correction.empty()) {
            sLowerName = std::move(textCp->correction);
        } else {
            sLowerName = decapitalize(sName, cp);
        }

        if (hasTextEntry) {
            if (sLowerName.empty() && !textCp->controls.empty()) {
                sLowerName = std::move(textCp->controls[0]);
                textCp->controls.erase(textCp->controls.begin());
            }

            if (abbrevState == AbbrevState::DISABLE)
                textCp->abbrs.clear();

            // Name
            textCp->eraseName(sLowerName);

            textCp->eraseName(defaultAbbrev);
            textCp->eraseName(decapitalize(defaultAbbrev, cp));
        }

        bool hasAbbrev = !defaultAbbrev.empty() || !textCp->abbrs.empty();
        auto [pTech, wasIns] = strings.remember(cp, uc::TextRole::MAIN_NAME, sLowerName);
        if (hasAbbrev && !wasIns) {
            nl.trigger();
            std::cout << "WARNING: char " << std::hex << cp << " has an abbreviation and a repeating name." << '\n';
        }

        auto abbrevRole = (abbrevState == AbbrevState::NORMAL)
                          ? uc::TextRole::ABBREV : uc::TextRole::ALT_NAME;
        if (!defaultAbbrev.empty()) {
            strings.forceRemember(cp, abbrevRole, defaultAbbrev);
        }

        for (auto& v : textCp->abbrs) {
            strings.forceRemember(cp, abbrevRole, std::string{v});
        }

        for (auto& v : textCp->controls) {
            if (v == sLowerName)
                continue;
            strings.forceRemember(cp, uc::TextRole::ALT_NAME, v);
        }

        if (auto v = egypBase.find(cp); v != egypBase.end()) {
            strings.forceRemember(cp, uc::TextRole::ALT_NAME, v->second);
        }

        for (auto& v : textCp->names) {
            strings.forceRemember(cp, uc::TextRole::ALT_NAME, v);
        }

        // HTML
        auto v = htmlEntities.data.find(cp);
        if (v != htmlEntities.data.end()) {
            if (!wasIns) {
                nl.trigger();
                std::cout << "WARNING: char " << std::hex << cp << " has HTML and a repeating name." << '\n';
            }
            for (auto &w : v->second)
                strings.forceRemember(cp, uc::TextRole::HTML, w);
        }

        /// @todo [urgent] get deprecated, somewhere else
        // Deprecated
        bool isDeprecated = (elChar.attribute("Dep").as_string()[0] == 'Y');
        if (isDeprecated) {
            auto itDep = deprecatedInfo.find(cp);
            if (itDep != deprecatedInfo.end()) {
                auto& q = itDep->second;
                if (!q.whatsInstead.empty()) {
                    strings.forceRemember(cp, uc::TextRole::DEP_INSTEAD, str::toSv(q.whatsInstead));
                    if (!q.whatsInstead2.empty()) {
                        strings.forceRemember(cp, uc::TextRole::DEP_INSTEAD2, str::toSv(q.whatsInstead2));
                    }
                }
            }
        }

        Flags<uc::Cfg> flags;
        if (hasAbbrev && abbrevState == AbbrevState::NORMAL)
            flags |= uc::m::ABBREVIATION;
        // Deprecated
        if (isDeprecated) {
            flags |= uc::Cfg::U_DEPRECATED;
            ++nDeprecated;
        }
        // Alternate
        if (isAlternate(cp))
            flags |= uc::Cfg::G_RENDER_BUG;
        // Virtual virama
        if (customDrawnControlChars.contains(cp)
                || (cp >= ::cp::TAG_MIN && cp <= ::cp::TAG_MAX))
            flags |= uc::m::CUSTOM_CONTROL;
        // No anti-aliasing
        if (isNoAa(cp))
            flags |= uc::m::NO_AA;
        // Default-ignorable
        /// @todo [urgent] default-ignorable
        if (elChar.attribute("DI").as_string()[0] == 'Y') {
            flags |= uc::Cfg::U_DEF_IGNORABLE;
        }
        // VS16
        if (emoji.vs16.contains(cp))
            flags |= uc::Cfg::U_VS16_EMOJI;
        // Misrenders
        if (emoji.misrenders.contains(cp))
            flags |= uc::Cfg::G_MISRENDER;
        // SVG emoji
        if (noto.singleChar.contains(cp))
            flags |= uc::m::SVG_EMOJI;
        // Draw as space
        if (charsDrawnAsSpaces.contains(cp))
            flags |= uc::m::SPACE;
        flags |= styleFlags(cp);

        // CJK strange
        /// @todo [urgent] take strange
        if (std::string_view sStrange = elChar.attribute("kStrange").as_string();
                !sStrange.empty()) {
            strangeCjk.processCp(cp, sStrange);
        }

        // OUTPUT
        os << "{ "
           << "0x" << std::hex << cp << ", "    // subj
           << "{ "                              // name
                << std::dec << pTech->offset    // name.tech,
           << " }, ";                           // /name

        // Char’s type
        /// @todo [urgent] get char’s type → column 2
        std::string_view sCharCat = elChar.attribute("gc").as_string();
        os << "EcCategory::" << transform(sCharCat, smCharCat) << ", ";

        // Char’s version
        auto& sVersion = ages.findRq(cp);
        os << "EcVersion::V_" << transformVersion(sVersion) << ", ";

        // Char’s bidirectional data
        /// @todo [urgent] get bidirectional class → column 3
        std::string_view sBidiClass = elChar.attribute("bc").as_string();
            // Check whether have bracket
        /// @todo [urgent] Can check column 9 instead of a separate file?
        if (mirroring.contains(cp)) {
            if (sBidiClass != "ON"sv)
                throw std::logic_error("Got strange bidi class for mirrored char");
            sBidiClass = "MIR"sv;
        }
        os << "EcBidiClass::z_" << sBidiClass << ", ";

        auto sScript = scripts.findDefRef(cp, "Zyyy"sv);
        // Char’s script
            // Hiragana → Hentaigata
        if (sScript == "Hira"sv && sLowerName.starts_with("Hentaigana"))
            sScript = "Hent"sv;
        os << "EcScript::" << sScript << ", ";

        char32_t upCase = 0;
        /// @todo [urgent] get uppercase → column 12
        std::string_view upText = elChar.attribute("uc").as_string();
        if (!upText.empty()             // Empty → no upper case
                && upText[0] != '#'     // # → no upper case
                && upText.find(' ') == std::string_view::npos) { // upcases to several chars like ẞ→SS → drop
            upCase = fromHex(upText);
        }
        forget::processCp(manualLib.forgetMap, cp, sLowerName, sScript, upCase);

        // Char’s numeric values
        // nt = …
        //    • None — no numeric value
        //    • De — decimal digit
        //    • Di — special digit
        //    • Nu — number
        // nv = Nan / whole number / vulgar fraction
        std::string_view sNumType = elChar.attribute("nt").as_string();
        std::string_view sNumValue = elChar.attribute("nv").as_string();
        auto& numPlace = nums.parse(sNumType, sNumValue);
        os << numPlace.index << ", ";

        if (flags) {
            os << "Cfgs{" << flags.numeric() << "}";
        } else {
            os << "{}";
        }

        os << "}," << '\n';
        ++nChars;

        strings.finishCp();
    }

    os << "};" << '\n';

    std::cout << "OK" << '\n';
    std::cout << "  Found " << std::dec << nChars << " chars, "
              << nDeprecated << " deprecated, "
              << nSpecialRanges << " special ranges." << '\n';

    os << "const char8_t uc::allStrings[] = \n";
    char text[40];
    for (auto& v : strings.inOrder()) {
        snprintf(text, std::size(text), R"(u8"\x%02X\x%02X" ")",
                 static_cast<unsigned>(v.role),
                 static_cast<unsigned>(v.s.length()));
        os << text << encodeC(v.s) << R"("  )";
        if (v.isLast)
            os << R"("\0")";
        os << "  // " << std::hex << static_cast<int>(v.subj) << '\n';
    }
    os << ";\n";

    ///// Blocks ///////////////////////////////////////////////////////////////

    size_t nBlocks = 0;
    auto elBlocks = need(elRoot.child("blocks"), "Need <blocks>");

    for (pugi::xml_node elBlock : elBlocks.children("block")) {
        //auto sFirst = elBlock.attribute("first-cp").as_string();
        //auto sLast = elBlock.attribute("last-cp").as_string();
        std::string_view name = elBlock.attribute("name").as_string();

        if (hasSubstr(name, "Private Use") || hasSubstr(name, "Surrogate"))
            continue;

        ++nBlocks;
    }
    std::cout << "  Found " << std::dec << nBlocks << " blocks." << '\n';

    ///// Numerics /////////////////////////////////////////////////////////////

    std::cout << "  Stockpiled " << nums.size() << " numerics." << '\n';
    os << "const uc::Numeric uc::allNumerics[uc::N_NUMERICS] { \n";
    for (const auto& v : nums.ord) {
        os << "{ " << std::dec << v.num << ", " << v.denom << ", " << v.altInt
           << ", EcNumType::" << transform(v.type, smNumType)
           << " },  // " << v.index << " is " << v.textValue << '\n';
    }
    os << "};\n";

    ///// Close main file //////////////////////////////////////////////////////

    os.close();

    ///// Write UcAutoLib.cpp //////////////////////////////////////////////////

    std::cout << "Saving library..." << std::flush;
    lib::Node root;
    // Node 1: emoji
    root.children.emplace_back(std::move(emoji.root));
    // Node 2+: manual library
    for (auto& v : manualLib.root.children) {
        root.children.emplace_back(std::move(v));
    }
    // Node N: strange hiero
    root.children.emplace_back(strangeCjk.give());
    // Write!
    auto libr = lib::write(root, "UcAutoLib.cpp");
    auto longest = root.maxValueLength();
    std::cout << "OK, " << libr.nNodes << " nodes, longest is " << longest << '.' << std::endl;

    ///// Write UcAutoCount ////////////////////////////////////////////////////

    os.open("UcAutoCount.h");
    os << "#pragma once\n";
    os << '\n';
    os << "// Automatically generated, do not edit!\n";
    os << '\n';
    os << "namespace uc {\n";
    os << "constexpr int N_CPS = " << std::dec << nChars << ";\n";
    os << "constexpr int N_BLOCKS = " << std::dec << nBlocks << ";\n";
    os << "constexpr int N_NUMERICS = " << std::dec << nums.size() << ";\n";
    os << "constexpr int N_EMOJI = " << std::dec << emoji.count << ";\n";
    os << "constexpr unsigned LONGEST_LIB = " << std::dec << longest << ";  // in codepoints" "\n";
    os << "}\n";

    ///// Sutton SignWriting ///////////////////////////////////////////////////

    std::cout << "Processing Sutton base..." << std::flush;
    auto swr = sw::process();
    std::cout << "OK, " << swr.nLines << " lines, first inequal "
              << std::hex << static_cast<uint32_t>(swr.firstInequal) << std::endl;

    ///// Forgotten CPs ////////////////////////////////////////////////////////

    static constinit const char* FNAME_FORGET = "forget.log";
    auto forgetStats = forget::postprocess(manualLib.forgetMap, FNAME_FORGET);
    std::cout << "Forgotten: " << std::dec
              << forgetStats.nRepeat << " repeating, "
              << forgetStats.nMissing << " missing, "
              << forgetStats.nExtra << " extra, "
              << forgetStats.nBadCase << " bad case, see " << FNAME_FORGET << "." << std::endl;

    ///// Done !! //////////////////////////////////////////////////////////////

    std::cout << "Successfully finished!" << std::endl << std::endl;

    return 0;
}
