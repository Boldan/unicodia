// My header
#include "LocList.h"

// C++
#include <charconv>

// Qt
#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wattributes"
#endif
    #include <QLocale>
#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

#include <QApplication>
#include <QTranslator>

// XML
#include "pugixml.hpp"

// Libs
#include "u_Strings.h"
#include "u_Qstrings.h"
#include "mojibake.h"

// L10n
#include "LocDic.h"
#include "LocManager.h"


loc::LangList loc::allLangs;
loc::Lang* loc::currLang = nullptr;

loc::Lang::Icons loc::active::icons;
loc::Lang::Numfmt loc::active::numfmt;
loc::Lang::Punctuation loc::active::punctuation;
loc::EngTerms loc::active::engTerms = loc::EngTerms::OFF;


///// CustomRule ///////////////////////////////////////////////////////////////


loc::Plural loc::CustomRule::ofUint(unsigned long long n) const
{
    for (auto& v : lines) {
        long long q = (v.mod == 0) ? n : n % v.mod;
        if (q >= v.min && q <= v.max) {
            return v.outcome;
        }
    }
    return defaultOutcome;
}


///// Lang /////////////////////////////////////////////////////////////////////


bool loc::Lang::hasMainLang(std::string_view iso) const
{
    return (!triggerLangs.empty() && triggerLangs[0] == iso);
}


bool loc::Lang::hasTriggerLang(std::string_view iso) const
{
    return std::find(triggerLangs.begin(), triggerLangs.end(), iso)
            != triggerLangs.end();
}


void loc::Lang::forceLoad()
{
    if (currLang)
        currLang->unload();
    if (translator)
        QApplication::installTranslator(translator.get());
    loc::loadIni(loc::dic, fnLang);
    currLang = this;

    // Active
    active::icons = icons;
    active::numfmt = numfmt;
    active::engTerms = engTerms;
    active::punctuation = punctuation;

    // loc::FmtL locale
    loc::activeFmtLocale = currLang;

    loc::man.translateMe();
}


void loc::Lang::load()
{
    if (currLang == this)
        return;
    forceLoad();
}


void loc::Lang::unload()
{
    if (currLang == this)
        currLang = nullptr;
    if (translator)
        QApplication::removeTranslator(translator.get());
}


///// Functions ////////////////////////////////////////////////////////////////

namespace {

    constexpr auto MY_OPTS =
        std::filesystem::directory_options::follow_directory_symlink
        | std::filesystem::directory_options::skip_permission_denied;

    loc::Plural parseOutcome(std::string_view value)
    {
        for (size_t i = 0; i < loc::Plural_N; ++i) {
            if (loc::pluralNames[i] == value)
                return static_cast<loc::Plural>(i);
        }
        return loc::Plural::OTHER;
    }

    void loadPluralRules(pugi::xml_node h, loc::CustomRule& r)
    {
        r.defaultOutcome = parseOutcome(h.attribute("default-outcome").as_string());
        for (auto hLine : h.children("rule")) {
            auto& line = r.lines.emplace_back();
            line.mod = hLine.attribute("mod").as_uint();
            line.min = hLine.attribute("min").as_uint();
            line.max = hLine.attribute("max").as_uint();
            line.outcome = parseOutcome(hLine.attribute("outcome").as_string());
        }
    }

    bool parseLang(loc::Lang& r, const std::filesystem::path& path)
    {
        // Remove translator
        r.translator.reset();
        r.name.tech = str::toUpper(path.filename().u8string());

        // Check lang.ini
        r.fnLang = path / "lang.ini";
        if (!std::filesystem::exists(r.fnLang))
            return false;

        // Check/parse locale.xml
        auto fnLocale = path / "locale.xml";
        if (!std::filesystem::exists(fnLocale))
            return false;

        pugi::xml_document doc;
        if (!doc.load_file(fnLocale.c_str()))
            return false;

        auto hLocale = doc.child("locale");
        r.name.native = str::toU8(hLocale.attribute("native").as_string());
        r.name.international = str::toU8(hLocale.attribute("international").as_string());

        unsigned et = hLocale.attribute("eng-terms").as_uint(static_cast<int>(loc::EngTerms::NORMAL));
        if (et > static_cast<unsigned>(loc::EngTerms::MAX))
            et = static_cast<unsigned>(loc::EngTerms::NORMAL);
        r.engTerms = static_cast<loc::EngTerms>(et);

        mojibake::simpleCaseFold(r.name.international, r.name.sortKey);

        r.stamp = hLocale.attribute("trigger-stamp").as_int(0);

        r.triggerLangs.clear();
        auto hTriggerLangs = hLocale.child("trigger-langs");
        for (auto& v : hTriggerLangs.children("lang")) {
            std::string_view iso = v.attribute("iso").as_string();
            if (!iso.empty()) {
                r.triggerLangs.emplace_back(iso);
            }
        }
        // Fallback: dir = Gibberish → let it be "!Gibberish",
        // but not empty
        if (r.triggerLangs.empty())
            r.triggerLangs.emplace_back(str::toSv(u8"!" + r.name.tech));

        r.sortOrder.clear();
        auto hAlphaSort = hLocale.child("alpha-sort");
        int base = 0, greatest = -1;
        for (auto& v : hAlphaSort.children("alp")) {
            if (v.attribute("follow").as_bool(false)) {
                base = greatest + 1;
            }
            auto alph = mojibake::toS<std::u32string>(v.text().as_string());
            for (size_t i = 0; i < alph.length(); ++i) {
                if (auto c = alph[i]; c != '_') {
                    int newCode = base + i;
                    r.sortOrder[c] = newCode;
                    greatest = std::max(greatest, newCode);
                }
            }
        }

        r.alphaFixup.clear();
        auto hAlphaFixup = hLocale.child("alpha-fixup");
        for (auto& v : hAlphaFixup.children("blk")) {
            std::string_view hex = v.attribute("start").as_string(0);
            uint32_t code;
            auto [ptr, ec] = std::from_chars(
                        std::to_address(hex.begin()),
                        std::to_address(hex.end()),
                        code, 16);
            if (ec == std::errc()) {
                auto target = mojibake::toS<std::u32string>(v.attribute("target").as_string());
                r.alphaFixup[code] = std::move(target);
            }
        }

        r.ellipsis.blocks.clear();
        auto hEllipsis = hLocale.child("ellipsis");
        r.ellipsis.text = str::toU8(hEllipsis.attribute("text").as_string("..."));
        for (auto& v : hEllipsis.children("blk")) {
            std::string_view hex = v.attribute("start").as_string(0);
            uint32_t code;
            auto [ptr, ec] = std::from_chars(
                        std::to_address(hex.begin()),
                        std::to_address(hex.end()),
                        code, 16);
            if (ec == std::errc()) {
                r.ellipsis.blocks.push_back(code);
            }
        }
        std::sort(r.ellipsis.blocks.begin(), r.ellipsis.blocks.end());

        r.wikiTemplates.clear();
        auto hTemplates = hLocale.child("wiki-templates");
        for (auto& v : hTemplates.children("tmpl")) {
            if (std::string_view name = v.attribute("name").as_string(); !name.empty()) {
                auto text = v.text().as_string();
                r.wikiTemplates[std::string{name}] = text;
            }
        }

        auto hIcons =  hLocale.child("icons");
        r.icons.sortAZ = hIcons.attribute("sort-az").as_string();

        r.numfmt.decimalPoint = '.';
        auto hNumFormat = hLocale.child("num-format");
        auto dp = mojibake::toS<std::u32string>(
                    hNumFormat.attribute("dec-point").as_string());
        if (!dp.empty() && dp[0] < 65536) {
            r.numfmt.decimalPoint = dp[0];
        }
        r.numfmt.thousand.minLength =
                hNumFormat.attribute("thou-min-length").as_uint(
                            loc::Lang::Numfmt::Thousand::DEFAULT_MIN_LENGTH);
        r.numfmt.denseThousand = r.numfmt.thousand;
        r.numfmt.denseThousand.minLength =
                hNumFormat.attribute("thou-min-length-dense").as_uint(
                            r.numfmt.thousand.minLength);

        auto hCardinalRules = hLocale.child("cardinal-rules");
        loadPluralRules(hCardinalRules, r.cardRule);

        auto hPeculiarities = hLocale.child("peculiarities");
        r.peculiarities.stillUsesBurmese = hPeculiarities.attribute("still-uses-burmese").as_bool(false);

        auto hPunctuation = hLocale.child("punctuation");
        r.punctuation.keyValueColon = str::toU8sv(
                    hPunctuation.attribute("key-value-colon").as_string("::::::"));
        r.punctuation.uniformComma = str::toU8sv(
                    hPunctuation.attribute("uniform-comma").as_string(",,,,,,"));
        r.punctuation.semicolon = str::toU8sv(
                    hPunctuation.attribute("semicolon").as_string(";;;;;;"));
        r.punctuation.leftParen = str::toU8sv(
                    hPunctuation.attribute("left-paren").as_string("(((((("));
        r.punctuation.rightParen = str::toU8sv(
                    hPunctuation.attribute("right-paren").as_string("))))))"));
        r.punctuation.indentEllip = str::toU8sv(
                    hPunctuation.attribute("indent-ellip").as_string(">>>>>>"));

        // Find Qt translator
        std::filesystem::directory_iterator di(path, MY_OPTS);
        for (auto& v : di) {
            if (v.is_regular_file() && v.path().extension() == ".qm") {
                r.translator = std::make_unique<QTranslator>();
                if (!r.translator->load(
                        str::toQ(v.path().stem().u16string()),
                        str::toQ(path.u16string()))) {
                    r.translator.reset();
                }
            }
        }

        std::cout << "Loaded lang " << str::toSv(r.name.tech) << '\n';
        return true;
    }

}


void loc::LangList::collect(const std::filesystem::path& programPath)
{
    auto dir = programPath / "Languages";

    std::unique_ptr<Lang> tempLang;

    std::filesystem::directory_iterator di(dir, MY_OPTS);
    for (const auto& v : di) {
        if (v.is_directory()) {
            //std::cout << v.path().string() << std::endl;
            //std::cout << v.path().filename().string() << std::endl;

            if (!tempLang) {
                tempLang = std::make_unique<loc::Lang>();
            }

            // Parse it
            if (parseLang(*tempLang, v.path())) {  // NOLINT(bugprone-use-after-move)
                push_back(std::move(tempLang));
            }
        }
    }

    // Sort languages
    std::sort(begin(), end(),
        [](const auto& x, const auto& y) {
            return (x->name.sortKey < y->name.sortKey);
        });
}


loc::Lang* loc::LangList::byIso(std::string_view x, int lastStamp)
{
    // Main language
    auto it = std::find_if(begin(), end(),
            [x, lastStamp](auto& p) {
                return (p->stamp > lastStamp && p->hasMainLang(x));
            });
    // Other languages
    if (it != end()) return it->get();
    it = std::find_if(begin(), end(),
            [x, lastStamp](auto& p){
                return (p->stamp > lastStamp && p->hasTriggerLang(x));
            });
    if (it != end()) return it->get();
    // Did not find
    return nullptr;
}


loc::Lang* loc::LangList::findStarting(
        std::string_view lastLang, int lastStamp)
{
    if (empty())
        return nullptr;

    // Find system language
    auto sysLoc = QLocale::system();
    auto locName = sysLoc.name().toLower().toStdString();
    if (auto p = locName.find('_'); p != std::string::npos) {
        locName = locName.substr(0, p);
    }

    // Override stamp if such language is present is system
    if (auto p = byIso(locName, lastStamp))
        return p;

    // Find by locale
    if (auto p = byIso(lastLang))
        return p;

    // Find by locale
    if (auto p = byIso(locName))
        return p;

    // Find English
    if (auto p = byIso("en"))
        return p;

    // LAST RESORT — first available
    return front().get();
}


void loc::LangList::loadStarting(
        std::string_view lastLang, int lastStamp)
{
    if (auto pLang = findStarting(lastLang, lastStamp)) {
        pLang->load();
    }

}


int loc::LangList::byPtr(const Lang* x)
{
    if (!x)
        return -1;
    auto it = std::find_if(begin(), end(),
            [x](auto& p){ return (p.get() == x); }
        );
    if (it != end()) return it - begin();
    return -1;
}


int loc::LangList::lastStamp() const
{
    int r = 0;
    for (auto& v : *this) {
        r = std::max(r, v->stamp);
    }
    return r;
}
