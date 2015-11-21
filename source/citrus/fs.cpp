#include "citrus/fs.hpp"
#include "citrus/err.hpp"
#include "internal.hpp"

#include <sys/unistd.h>
#include <dirent.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

#include <3ds.h>
#include <3ds/services/fs.h>

bool ctr::fs::init() {
    romfsInit();
    chdir("sdmc:/");
    return true;
}

void ctr::fs::exit() {
    romfsExit();
}

u64 ctr::fs::freeSpace(MediaType mediaType) {
    FS_ArchiveResource resource;
    ctr::err::parse(ctr::err::SOURCE_FS_GET_ARCHIVE_RESOURCE, (u32) FSUSER_GetArchiveResource(&resource, (FS_MediaType) mediaType));
    if(ctr::err::has()) {
        return 0;
    }

    return (u64) resource.clusterSize * (u64) resource.freeClusters;
}

bool ctr::fs::exists(const std::string path) {
    FILE* fd = fopen(path.c_str(), "r");
    if(fd) {
        fclose(fd);
        return true;
    }

    return directory(path);
}

bool ctr::fs::directory(const std::string path) {
    DIR* dir = opendir(path.c_str());
    if(dir) {
        closedir(dir);
        return true;
    }

    return false;
}

const std::string ctr::fs::fileName(const std::string path) {
    std::string::size_type slashPos = path.rfind('/');
    if(slashPos == std::string::npos) {
        return path;
    }

    return path.substr(slashPos + 1);
}

const std::string ctr::fs::extension(const std::string path) {
    std::string::size_type dotPos = path.rfind('.');
    if(dotPos == std::string::npos) {
        return "";
    }

    return path.substr(dotPos + 1);
}

bool ctr::fs::hasExtension(const std::string path, const std::string extension) {
    if(extension.empty()) {
        return true;
    }

    const std::string ext = ctr::fs::extension(path);
    return strcasecmp(ext.c_str(), extension.c_str()) == 0;
}

bool ctr::fs::hasExtensions(const std::string path, const std::vector<std::string> extensions) {
    if(extensions.empty()) {
        return true;
    }

    const std::string ext = ctr::fs::extension(path);
    for(std::vector<std::string>::const_iterator it = extensions.begin(); it != extensions.end(); it++) {
        std::string extension = *it;
        if(strcasecmp(ext.c_str(), extension.c_str()) == 0) {
            return true;
        }
    }

    return false;
}

std::vector<std::string> ctr::fs::contents(const std::string directory, bool directories, const std::vector<std::string> extensions) {
    std::vector<std::string> result;

    bool hasSlash = directory.size() != 0 && directory[directory.size() - 1] == '/';
    const std::string dirWithSlash = hasSlash ? directory : directory + "/";

    DIR* dir = opendir(dirWithSlash.c_str());
    if(dir == NULL) {
        return result;
    }

    while(true) {
        struct dirent* ent = readdir(dir);
        if(ent == NULL) {
            break;
        }

        std::string path = dirWithSlash + std::string(ent->d_name);
        if((directories || !ctr::fs::directory(path)) && ctr::fs::hasExtensions(path, extensions)) {
            result.push_back(path);
        }
    }

    closedir(dir);
    return result;
}