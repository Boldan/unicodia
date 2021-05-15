// C++
#include <iostream>
#include <fstream>
#include <charconv>

// PugiXML
#include "pugixml.hpp"

// Project-local
#include "data.h"

using namespace std::string_view_literals;

template <class T>
inline auto need(T&& val, const char* errmsg)
{
    if (!val) throw std::logic_error(errmsg);
    return std::forward<T>(val);
}

struct Fraction {
    long long num, denom;
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

Fraction parseFraction(std::string_view numType, std::string_view x)
{
    auto pSlash = x.find('/');
    if (pSlash == std::string_view::npos) {
        // Integer
        return { fromChars(x, numType), 1 };
    } else {
        // Fraction
        auto num = x.substr(0, pSlash);
        auto den = x.substr(pSlash + 1);
        return { fromChars(num, numType), fromChars(den, numType) };
    }
}

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
    { "Nu"sv, "NUMBER" },
};

int main()
{
    std::ofstream os("UcAuto.cpp");
    os << "// Automatically generated, do not edit!" << '\n';
    os << '\n';
    os << R"(#include "UcData.h")" << '\n';
    os << '\n';
    os << R"(using namespace std::string_view_literals;)" << '\n';
    os << '\n';

    pugi::xml_document doc;

    std::cout << "Loading Unicode base..." << std::flush;
    doc.load_file("ucd.all.flat.xml");
    std::cout << "OK" << std::endl;

    ///// CpInfo ///////////////////////////////////////////////////////////////

    unsigned nChars = 0, nSpecialRanges = 0;

    auto elRoot = need(doc.root().child("ucd"), "Need <ucd>");
    auto elRepertoire = need(elRoot.child("repertoire"), "Need <repertoire>");
    std::cout << "Found repertoire, generating character info..." << std::flush;
    os << '\n';
    os << R"(uc::Cp uc::cpInfo[] {)" << '\n';

    for (pugi::xml_node elChar : elRepertoire.children("char")) {
        std::string_view sCp = elChar.attribute("cp").as_string();
        if (sCp.empty()) {
            ++nSpecialRanges;
            continue;
        }
        // Aliases:
        // • Abbreviation: Implement later
        // • Alternate: Prefer na1
        // • Control: Prefer na1
        // • Figment: Implement
        // • Correction: These are former names, implement together with abbreviations

        /// @todo [future] Sometimes we have fixups, what to take?
        /// @todo [future] Sometimes we have abbreviations, take them
        std::string_view sName = elChar.attribute("na").as_string();
        if (sName.empty())
            sName = elChar.attribute("na1").as_string();

        // Aliases?
        for (auto elAlias : elChar.children("name-alias")) {
            std::string_view sType = elAlias.attribute("type").as_string();
            if (sType == "alternate"sv || sType == "control"sv || sType == "figment") {
                if (sName.empty())
                    sName = elAlias.attribute("alias").as_string();
            }
        }

        std::string sLowerName = decapitalize(sName);
        os << "{ "
           << "0x" << sCp << ", "       // subj
           << "0x" << sCp << ", "       // proxy
           << "{ "                      // name
                << '\"' << sLowerName << R"("sv, )"  // name.tech,
                << R"(""sv)"                    // name.loc
           << " }, {";                  // /name

        // Char’s numeric values
        // nt = …
        //    • None — no numeric value
        //    • De — decimal digit
        //    • Di — special digit
        //    • Nu — number
        // nv = Nan / whole number / vulgar fraction
        std::string_view sNumType = elChar.attribute("nt").as_string();
        if (sNumType != "None"sv) {
            std::string_view sNumValue = elChar.attribute("nv").as_string();
            auto frac = parseFraction(sNumType, sNumValue);
            os << " EcNumType::" << transform(sNumType, smNumType)
               << ", " << frac.num << ", " << frac.denom << ' ';
        }
        os << "}";

        /// @todo [urgent] Char type

        os << " }," << '\n';
        ++nChars;
    }

    os << "};" << '\n';

    std::cout << "OK" << std::endl;
    std::cout << "Found " << nChars << " chars, " << nSpecialRanges << " special ranges " << std::endl;

    os << "unsigned uc::nCps() { return " << nChars << "; }" << '\n';

    std::cout << "Successfully finished!" << std::endl << std::endl;

    return 0;
}
