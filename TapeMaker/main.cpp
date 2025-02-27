// STL
#include <iostream>
#include <fstream>
#include <filesystem>
#include <set>
#include <unordered_map>

// XML
#include "pugixml.hpp"

// Libs
#include "i_ByteSwap.h"
#include "u_Strings.h"

// Project-local
#include "optstorage.h"

using namespace std::string_view_literals;


std::string seqStem(std::u32string_view seq)
{
    std::string r;
    char buf[20];
    for (auto v : seq) {
        if (!r.empty())
            r += '-';
        snprintf(buf, std::size(buf), "%x", v);
        r += buf;
    }
    return r;
}


struct TapeEntry
{
    unsigned iSubtape,
             offset;
    std::u32string seq;
    std::string stemOut;
    std::string content;
    unsigned priority;
    size_t length() const { return content.size(); }
};


using PriorityMap = std::unordered_map<std::string, int>;
struct OptInfo;


class TapeWriter
{
public:
    /// @return [0] not added [+] its filename
    TapeEntry* addFile(const std::filesystem::path& p, OptInfo& info);
    void sortBy(const PriorityMap& prioMap);
    void write();
private:
    static constexpr int SUBTAPE_SIZE = 1'000'000;
    struct Subtape {
        unsigned iSubtape;
        unsigned size = 0;
        std::vector<const TapeEntry*> entries {};
    };
    std::vector<TapeEntry> allEntries;
    void writeDirectory() const;

    struct DivideIntoSubtapes {
        std::vector<Subtape> subtapes;
        unsigned biggestSubtape = 0;
    };
    DivideIntoSubtapes divideIntoSubtapes();
};

bool isBlacklistedPart(std::string_view s)
{
    return (s == "emoji"sv);
}

std::u32string parseSeq(std::string s)
{
    str::replace(s, '_', '-');      // for Noto
    auto things = str::splitSv(s, '-');
    std::u32string r;
    for (auto v : things) {
        if (isBlacklistedPart(v))
            continue;
        if (v.starts_with('u'))
            v = v.substr(1);
        int value = -1;
        std::from_chars(
            std::to_address(v.begin()), std::to_address(v.end()), value, 16);
        if (value >= 0) {
            r += char32_t(value);
        } else {
            return {};
        }
    }
    return r;
}

TapeEntry* TapeWriter::addFile(const std::filesystem::path& p, OptInfo& info)
{
    // Tape will not add empty files
    if (info.length == 0)
        return nullptr;

    // Get sequence
    auto fname = p.filename().generic_string();
    auto seq = parseSeq(fname);
    if (seq.empty())
        return nullptr;

    auto stemOut = seqStem(seq);
    unsigned prio = (seq.length() == 1) ? seq[0] : 1'000'000'000;
    auto& entry = allEntries.emplace_back(TapeEntry {
                    .iSubtape = 100000,
                    .offset = SUBTAPE_SIZE * 100,
                    .seq = seq,
                    .stemOut = stemOut,
                    .content = std::move(info.content),
                    .priority = prio
                });

    return &entry;
}


//    auto newSize = subtapeSize + fsize;
//    std::ifstream is(p, std::ios::binary);
//    is.read(readySubtape.data() + subtapeSize, fsize);
//    subtapeSize = newSize;

//    // Next??
//    if (subtapeSize >= SUBTAPE_SIZE)
//        nextSubtape();

//    return entry.toName(".svg");
//}

//void TapeWriter::nextSubtape()
//{
//    if (subtapeSize == 0)
//        return;

//    // Generate subtape name
//    char buf[20];
//    snprintf(buf, sizeof(buf), "%d.bin", iSubTape);

//    // Write subtape
//    std::ofstream os(buf, std::ios::binary);
//    os.write(readySubtape.data(), subtapeSize);

//    // ++
//    ++iSubTape;
//    subtapeSize = 0;
//}


TapeWriter::DivideIntoSubtapes TapeWriter::divideIntoSubtapes()
{
    DivideIntoSubtapes r;

    bool wantNewSubtape = true;

    for (auto& entry : allEntries) {
        Subtape* subtape;

        // Get subtape
        unsigned iSubtape = r.subtapes.size();
        if (wantNewSubtape) {
            subtape = &r.subtapes.emplace_back(
                        Subtape { .iSubtape = iSubtape });
            wantNewSubtape = false;
        } else [[likely]] {
            subtape = &r.subtapes.back();
            --iSubtape;
        }

        // Add to subtape
        entry.offset = subtape->size;
        entry.iSubtape = iSubtape;
        subtape->entries.push_back(&entry);

        // Add size
        subtape->size += entry.length();
        r.biggestSubtape = std::max(r.biggestSubtape, subtape->size);
        if (subtape->size > SUBTAPE_SIZE) {
            wantNewSubtape = true;
        }
    }

    return r;
}

namespace {

    [[maybe_unused]] void writeIW(std::ostream& os, uint16_t x)
    {
        Word1 w { x };
        swapIW(w);
        os.write(w.asChars, 2);
    }

    [[maybe_unused]] void writeID(std::ostream& os, uint32_t x)
    {
        Dword1 d { x };
        swapID(d);
        os.write(d.asChars, 4);
    }

}   // anon namespace

void TapeWriter::writeDirectory() const
{
    std::ofstream os("tape.bin", std::ios::binary);
    writeID(os, allEntries.size());
    for (auto& entry : allEntries) {
        writeIW(os, entry.iSubtape);
        writeID(os, entry.offset);
        writeID(os, entry.length());
        auto fname = entry.stemOut + ".svg";
        writeIW(os, fname.length());
        os.write(fname.data(), fname.length());
    }
}

void TapeWriter::sortBy(const PriorityMap& prioMap)
{
    for (auto& entry : allEntries) {
        if (auto it = prioMap.find(entry.stemOut); it != prioMap.end()) {
            entry.priority = it->second;
        }
    }
    std::stable_sort(allEntries.begin(), allEntries.end(),
                    +[](const TapeEntry& x, const TapeEntry& y) {
                        return (x.priority < y.priority);
                    });
}


void TapeWriter::write()
{
    auto div = divideIntoSubtapes();

    std::string tapeBuf;
    tapeBuf.resize(div.biggestSubtape + 10000);

    for (auto& subtape : div.subtapes) {
        // Build contents
        tapeBuf.clear();
        for (auto& pEntry : subtape.entries) {
            std::ranges::copy(pEntry->content, tapeBuf.data() + pEntry->offset);
        }

        // Generate subtape name
        char nameBuf[20];
        snprintf(nameBuf, sizeof(nameBuf), "%u.bin", subtape.iSubtape);

        // Write subtape
        std::ofstream os(nameBuf, std::ios::binary);
        os.write(tapeBuf.data(), subtape.size);
    }
    writeDirectory();
}

void deleteBinaries()
{
    std::filesystem::path pExt(".bin");
    std::filesystem::directory_iterator di(".");
    for (const auto& entry: di) {
        if (entry.is_regular_file() && entry.path().extension() == pExt) {
            std::filesystem::remove(entry.path());
        }
    }
}

PriorityMap loadPrioMap(const char* fname)
{
    PriorityMap r;
    pugi::xml_document doc;
    doc.load_file(fname);
    auto hRoot = doc.root().child("opt");
    for (auto child : hRoot.children("file")) {
        auto name = child.attribute("name").as_string();
        auto prio = child.attribute("prio").as_int(999'999);
        // Priority is…
        // 1. Previews (=priority 0)
        // 2. Single-char, in code order (not here and just omitted)
        // 3. Others
        if (prio != 0) {
            prio += 300'000;
            if (strchr(name, '-') == nullptr) {
                continue;
            }
        }
        r[name] = prio;
    }
    return r;
}

int main()
{
    try {
        auto prioMap = loadPrioMap("opt.xml");

        OptStorage storage;
        if (!storage.findOptimizer()) {
            std::cout << "WARNING: optimizer (svgcleaner-cli) was not found, not an error until actually need" "\n";
        }
        storage.readXml("files.xml");

        deleteBinaries();
        TapeWriter tw;
        std::cout << "TRAVERSING DIRECTORY..." "\n";
        std::filesystem::path pExt(".svg");
        std::filesystem::directory_iterator di(".");
        for (const auto& entry: di) {
            if (entry.is_regular_file() && entry.path().extension() == pExt) {
                auto result = storage.checkFile(entry.path().string().c_str());
                if (result.info) {
                    auto q = tw.addFile(entry.path(), *result.info);
                    if (!q) {
                        std::cout << "NOT ADDED: " << entry.path().filename().generic_string() << '\n';
                    }
                }
            }
        }
        std::cout << "BUILDING TAPE..." "\n";
        tw.sortBy(prioMap);
        tw.write();

        storage.removeUntouched();
        if (storage.isModified && !storage.isEmpty()) {
            storage.writeXml("files.xml");
        }
    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << '\n';
    }
}
