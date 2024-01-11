#include "Unzipper.hpp"

namespace unsnapshot
{

bool UnSnapshot::helperExtractSyslogFrom(const std::string& zipName, const std::string& xzFileName,
    const Path& outputPath, const std::string& newName)
{
    const Path extractXZFromPath = gTempPath / zipName;
    try
    {
        elz::extractFile(extractXZFromPath, xzFileName, gTempPath);
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "Something happened unzipping: -> \"%s\" FROM \"%s\"\n -> what(): %s\n",
            xzFileName.c_str(), extractXZFromPath.string().c_str(), e.what());
        return false;
    }

    /* Get syslog.txt from syslog.xz */
    const Path xzInFilePath = gTempPath / xzFileName;
    const Path xzOutFilePath = outputPath / newName;
    printf("OUTPUT PATH: %s\n", xzOutFilePath.string().c_str());
    std::ifstream inFile(xzInFilePath, std::ios::binary);
    std::ofstream outFile(xzOutFilePath, std::ios::binary);

    if (!xz::decompress(inFile, outFile))
    {
        fprintf(stderr, "Faied to decompress XZ: %s\n", xzInFilePath.string().c_str());
        inFile.close();
        outFile.close();
        return false;
    }

    inFile.close();
    outFile.close();
    return true;
}

void UnSnapshot::extractSyslogsFrom(const Path& unzippedSnapshotPath, const std::string& parentName,
    const std::string& childName, const Path& outputPath)
{
    printf("Trying to extract syslogs\n");

    Path parentNamePath = unzippedSnapshotPath / parentName;
    if (!std::filesystem::exists(parentNamePath))
    {
        fprintf(stderr, "Parent zip %s doesnt exist!\n", parentNamePath.string().c_str());
        return failBye("Unlikely FiNoFound failure");
    }

    /* Get syslog.zip from part_x.zip */
    try
    {
        elz::extractFile(parentNamePath, childName, gTempPath);
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "Something happened unzipping:  \"%s\" FROM \"%s\"\n -> what(): %s\n",
            childName.c_str(), parentNamePath.string().c_str(), e.what());
        return failBye("Weird failure of unzip");
    }

    /* Get syslog.xz from syslog.zip */
    if (childName.find("startup") != std::string::npos)
    {
        if (!helperExtractSyslogFrom(childName, "startup_BTSOM.log.xz", outputPath, "startup_BTSOM.txt"))
        {
            return failBye("Weird failure of unzip XZ");
        }
    }
    else if (childName.find("runtime") != std::string::npos)
    {
        if (!helperExtractSyslogFrom(childName, "runtime_BTSOM.log.xz", outputPath, "runtime_BTSOM.txt"))
        {
            return failBye("Weird failure of unzip XZ");
        }
    }
    else if (childName.find("pm") != std::string::npos)
    {
        auto splittedName = split(childName, '_');
        splittedName.erase(splittedName.begin());
        splittedName.erase(splittedName.end() - 1);
        auto newNamePrefix = join(splittedName, '_');

        bool bothFailed = true;
        if (helperExtractSyslogFrom(childName, "startup_BTSOM.log.xz", outputPath, newNamePrefix + "_startup_BTSOM.txt"))
        {
            bothFailed = false;
        }

        if (helperExtractSyslogFrom(childName, "runtime_BTSOM.log.xz", outputPath, newNamePrefix + "_runtime_BTSOM.txt"))
        {
            bothFailed = false;
        }

        if (bothFailed)
        {
            return failBye("Weird failure of unzip XZ");
        }
    }

    printf("%s\n%s\n", parentName.c_str(), childName.c_str());
}


void UnSnapshot::extractIMS2From(const Path& unzippedSnapshotPath, const std::string& parentName,
    const std::string& childName, const Path& outputPath)
{
    printf("Trying to extract IMS2\n");

    Path parentNamePath = unzippedSnapshotPath / parentName;
    if (!std::filesystem::exists(parentNamePath))
    {
        fprintf(stderr, "Parent zip %s doesnt exist!\n", parentNamePath.string().c_str());
        return failBye("Weird failure of unzip");
    }

    /* Get file.ims2 from part_x.zip */
    try
    {
        elz::extractFile(parentNamePath, childName, outputPath);
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "Something happened unzipping: \"%s\" FROM \"%s\"\n -> what(): %s\n",
            childName.c_str(), parentNamePath.string().c_str(), e.what());
        return failBye("Weird failure of unzip");
    }

    printf("%s\n%s\n", parentName.c_str(), childName.c_str());
}

bool UnSnapshot::handleParentLine(std::ifstream& snapshotFileList, const std::string& maybeParentLine, char* readBuffer)
{
    std::smatch match;

    bool foundAnything = false;
    std::string maybePatternMatch;
    maybePatternMatch.reserve(BUFF_CAP);

    /* Go down the lines to find a pattern */
    while (!snapshotFileList.eof())
    {
        snapshotFileList.getline(readBuffer, BUFF_CAP, '\n');
        maybePatternMatch = readBuffer;

        /* If the line is empty, means we are at a border between end of parent contents and start of next parent contents
           so it's impossible to have a match here. Exit */
        if (maybePatternMatch.empty()) { return foundAnything; }

        /* Extract the ims2 file from the /parent/zip/ location */
        if (std::regex_search(maybePatternMatch, match, gSyslogRegex))
        {
            extractSyslogsFrom(gUnzippedSnapshotPath, maybeParentLine.substr(0, maybeParentLine.size() - 1), match[0].str(), gSyslogsOutputPath);
            foundAnything = true;
        }

        /* Extract the ims2 file from the /parent/zip/ location */
        if (std::regex_search(maybePatternMatch, match, gIms2Regex))
        {
            extractIMS2From(gUnzippedSnapshotPath, maybeParentLine.substr(0, maybeParentLine.size() - 1), match[0].str(), gSyslogsOutputPath);
            foundAnything = true;
        }
    }

    return foundAnything;
}

void UnSnapshot::unSnap(const Path& inPath, const Path& outPath, const UnSnapshotProgressCb progressCb,
    const UnSnapshotDoneCb doneCb)
{
    gDoneCallback = doneCb;
    gProgressCallback = progressCb;

    /* Extract big snapshot. */
    gTempPath = outPath / "temp";
    try
    {
        elz::extractZip(inPath, gTempPath);
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "Something happened unzipping big snapshot: \"%s\"\n -> what(): %s\n",
            inPath.string().c_str(), e.what());
        return failBye("Unzip failure");
    }
    reportProgress(10.0f);

    gUnzippedSnapshotPath = gTempPath;
    gSyslogsOutputPath = outPath;
    Path snapshotFileListPath = gTempPath / "snapshot_file_list.txt";

    std::ifstream snapshotFileList(snapshotFileListPath);
    if (snapshotFileList.fail() || snapshotFileList.bad())
    {
        fprintf(stderr, "File couldn't be found at 1st level of zip or loaded %s\n", snapshotFileListPath.string().c_str());
        snapshotFileList.close();
        return failBye("snapshot_file_list missing");
    }
    reportProgress(20.0f);

    std::string maybeParentLine;
    maybeParentLine.reserve(BUFF_CAP);
    char readBuffer[BUFF_CAP];
    int32_t filesFound = 0;
    float fakeIncrementFactor = 5.0f;

    while (!snapshotFileList.eof())
    {
        snapshotFileList.getline(readBuffer, BUFF_CAP, '\n');
        maybeParentLine = readBuffer;

        /* If line ends with : means it's parent to possible regex pattern */
        if (maybeParentLine.ends_with(":"))
        {
            if (handleParentLine(snapshotFileList, maybeParentLine, readBuffer))
            {
                filesFound++;
                reportProgress(std::clamp(50.0f + filesFound * fakeIncrementFactor, 0.0f, 99.0f));
            }
        }
    }

    snapshotFileList.close();
    reportProgress(100.0f);
    okBye();
}

std::vector<std::string> UnSnapshot::split(const std::string& str, const char sep)
{
    std::vector<std::string> splitted;
    uint64_t lastIdx = 0;
    uint64_t idx = str.find_first_of(sep);
    while (idx != std::string::npos)
    {
        splitted.emplace_back(str.begin() + lastIdx, str.begin() + idx);
        lastIdx = idx + 1;
        idx = str.find_first_of(sep, lastIdx);
    }
    splitted.emplace_back(str.begin() + lastIdx, str.end());
    return splitted;
}

std::string UnSnapshot::join(const std::vector<std::string>& strVec, const char sep)
{
    std::string str;
    for (uint64_t i = 0; i < strVec.size() - 1; i++)
    {
        str += strVec[i] + sep;
    }
    str += strVec[strVec.size() - 1];
    return str;
}

void UnSnapshot::okBye()
{
    if (gDoneCallback)
    {
        /* Clean up */
        std::filesystem::remove_all(gTempPath);

        gDoneCallback(true, "");
        gDoneCallback = nullptr;
    }
}

void UnSnapshot::failBye(const std::string& err)
{
    if (gDoneCallback)
    {
        /* Clean up */
        std::filesystem::remove_all(gTempPath);

        gDoneCallback(false, err);
        gDoneCallback = nullptr;
        gProgressCallback = nullptr;
    }
}

void UnSnapshot::reportProgress(const float progress)
{
    if (gProgressCallback)
    {
        gProgressCallback(progress);
    }
}

}