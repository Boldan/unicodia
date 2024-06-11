// My header
#include "loader.h"

// C++
#include <cstdlib>
#include <string_view>
#include <string>
#include <filesystem>

// Libs
#include "u_TypedFlags.h"

namespace {

    class Action {  // interface
    public:
        virtual void exec() const = 0;
        virtual ~Action() = default;
    };

    class NoAction final : public Action
    {
    public:
        void exec() const override {}
        //static const NoAction INST;
    };

    //const NoAction NoAction::INST;

    // class SingleZip final : public Action
    // {
    // public:
    //     constexpr SingleZip(
    //             std::vector<unsigned char>& aDest,
    //             std::string_view aArc, std::string_view aFile)
    //         : dest(aDest), arc(aArc), file(aFile) {}
    //     void exec() const override;
    // private:
    //     std::vector<unsigned char>& dest;
    //     std::string_view arc, file;
    // };

    // void SingleZip::exec() const
    // {
    //     Zippy::ZipArchive za;
    //     za.Open(std::string{arc});
    //     auto entry = za.GetEntry(std::string{file});
    //     auto data = entry.GetData();
    //     if (data.empty())
    //         throw std::logic_error("File not found in archive");
    //     dest = std::move(data);
    // }

    constexpr int N_UPDIRS = 4;

    class LocalFile
    {
    public:
        constexpr LocalFile(std::string_view aFname) : fname(aFname) {}
        void preload() const;
    private:
        std::string_view fname;
    };

    void LocalFile::preload() const
    {
        std::filesystem::path rqPath(fname);
        auto destPath = rqPath.filename();
        std::string testPath { fname };
        for (int i = 0; i < N_UPDIRS; ++i) {
            std::filesystem::path srcPath(testPath);
            if (std::filesystem::exists(srcPath)) {
                // filesystem::copy does not overwrite, even with options
                if (std::filesystem::exists(destPath))
                    std::filesystem::remove(destPath);
                // Will throw if problems, that’s OK
                std::filesystem::copy(srcPath, destPath);
                // Check once again for existence
                if (!std::filesystem::exists(destPath)) {
                    throw std::runtime_error("Copied " + rqPath.generic_string() + ", still not found");
                }
                return;
            }
            testPath = "../" + testPath;
        }
        throw std::runtime_error("Cannot find file " + rqPath.generic_string());
    }

    // Step flags
    enum class Stfg {
        FINAL = 1,
    };

    struct Step {
        std::string_view what;
        const Action& action;
        std::string_view file;
        Flags<Stfg> flags;
    };

    constinit const LocalFile allLocalFiles[] {
        { RAWDATA ENTITIES_JSON },
        { RAWDATA UCD_PVA },
        { RAWDATA UCD_NAMES },
        { RAWDATA UCD_SCRIPTS },
        { RAWDATA UCD_ALIASES },
        { RAWDATA UCD_PROP },
        { RAWDATA UCD_BLOCKS },
        { RAWDATA UCD_MAIN },
        { RAWDATA DER_AGE },
        { RAWDATA DER_CORE },
        { RAWDATA DER_NAME },
        { RAWDATA HAN_DIC },
        { RAWDATA HAN_NUM_VALUE },
        { RAWDATA HAN_IRG },
        { RAWDATA EMOJI_TEST },
        { RAWDATA EGYP_UNICODE },
        { RAWDATA EGYP_DESCRIPTION },
        { MISCFILES LIBRARY_XML },
        { MISCFILES SUTTON_TXT },

        // Old computers
        { MAPPINGS OLD_APPLE2_1 },
        { MAPPINGS OLD_APPLE2_2 },
        { MAPPINGS OLD_APPLE2_3 },
        { MAPPINGS OLD_APPLE2_4 },
    };

}

void checkLoader()
{
    for (auto& lf : allLocalFiles) {
        lf.preload();
    }
}
