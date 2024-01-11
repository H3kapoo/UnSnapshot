#pragma once

#include <stdio.h>
#include <fstream>
#include <string>
#include <utility>
#include <regex>
#include <filesystem>
#include <functional>

#include "11Zip/include/elzip/elzip.hpp"
#include "xz/decode.hpp"

namespace unsnapshot
{
#define BUFF_CAP 128

class UnSnapshot
{
    typedef std::function<void(const bool status, const std::string& err)> UnSnapshotDoneCb;
    typedef std::function<void(const float progress)> UnSnapshotProgressCb;
    typedef std::filesystem::path Path;
public:
    UnSnapshot() = default;
    void unSnap(const Path& inPath, const Path& outPath, const UnSnapshotProgressCb progressCb,
        const UnSnapshotDoneCb doneCb);

private:
    //TODO: use std::fs::path instead of plain strings. windows reasons
    bool handleParentLine(std::ifstream& snapshotFileList, const std::string& maybeParentLine, char* readBuffer);
    bool helperExtractSyslogFrom(const std::string& zipName, const std::string& xzFileName, const Path& outputPath,
        const std::string& newName);
    void extractSyslogsFrom(const Path& unzippedSnapshotPath, const std::string& parentName,
        const std::string& childName, const Path& outputPath);
    void extractIMS2From(const Path& unzippedSnapshotPath, const std::string& parentZip,
        const std::string& childName, const Path& outputPath);

    void okBye();
    void failBye(const std::string& err);
    void reportProgress(const float progress);

    std::vector<std::string> split(const std::string& str, const char sep);
    std::string join(const std::vector<std::string>& strVec, const char sep);

    std::regex gIms2Regex{ "(BTS[0-9]{1,9}_([1-9])011_(pm_[1-9]_)*im_snapshot.ims2)" };
    std::regex gSyslogRegex{ "(BTS[0-9]{1,9}_([1-9])011_(startup|runtime).zip)|(BTS[0-9]{1,9}_([1-9])011_pm_([1-9])_syslog.zip)" };

    Path gUnzippedSnapshotPath;
    Path gSyslogsOutputPath;
    Path gTempPath;

    UnSnapshotDoneCb gDoneCallback{ nullptr };
    UnSnapshotProgressCb gProgressCallback{ nullptr };
};
}