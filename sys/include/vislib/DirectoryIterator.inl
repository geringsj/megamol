/*
 * DirectoryIterator.inl
 *
 * Copyright (C) 2007 - 2010 by Visualisierungsinstitut Universitaet Stuttgart. 
 */


#ifdef _WIN32
#include <Windows.h>
#else /* _WIN32 */
#include "the/not_supported_exception.h"
#endif /* _WIN32 */

namespace vislib {
namespace sys {


    /*
     * DirectoryIterator<T>::DirectoryIterator
     */
    template<> DirectoryIterator<the::astring>::DirectoryIterator(
            const char* path, bool isPattern, bool showDirs) : nextItem(),
            currentItem(), omitFolders(!showDirs) {
#ifdef _WIN32
        WIN32_FIND_DATAA fd;
        the::astring p = path;
        if (!isPattern) {
            if (!the::text::string_utility::ends_with(p, vislib::sys::Path::SEPARATOR_A)) {
                if (!the::text::string_utility::ends_with(p, ":")) {
                    p += vislib::sys::Path::SEPARATOR_A;
                }
            }
            p += "*.*";
        }

        this->findHandle = FindFirstFileA(p.c_str(), &fd);
        if (this->findHandle == INVALID_HANDLE_VALUE) {
            unsigned int le = ::GetLastError();
            if ((le == ERROR_FILE_NOT_FOUND) || (le == ERROR_PATH_NOT_FOUND)) {
                this->nextItem.Path.clear();
            } else {
                throw the::system::system_exception(le, __FILE__, __LINE__);
            }
        } else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            this->nextItem.Type = Entry::DIRECTORY;
            if (this->omitFolders || (strcmp(fd.cFileName, ".") == 0)
                    || (strcmp(fd.cFileName, "..") == 0)) {
                this->fetchNextItem();
            } else {
                this->nextItem.Path = fd.cFileName;
            }
        } else {
            this->nextItem.Type = Entry::FILE;
            this->nextItem.Path = fd.cFileName;
        }
#else /* _WIN32 */
        if (isPattern) {
            this->basePath = vislib::sys::Path::GetDirectoryName(path);
            this->pattern = path + (this->basePath.size() + 1);

        } else {
            this->basePath = path;
            this->pattern.clear();

        }
        if (vislib::sys::File::Exists(this->basePath.c_str())) {
            if ((this->dirStream = opendir(this->basePath.c_str())) == NULL) {
                throw the::system::system_exception(__FILE__, __LINE__);
            }
            this->fetchNextItem();
        } else {
            this->dirStream = NULL;
            this->nextItem.Path.clear();
        }
#endif /* _WIN32 */
    }


    /*
     * DirectoryIterator<T>::DirectoryIterator
     */
    template<> DirectoryIterator<the::wstring>::DirectoryIterator(
            const wchar_t* path, bool isPattern, bool showDirs) : nextItem(),
            currentItem(), omitFolders(!showDirs) {
#ifdef _WIN32
        WIN32_FIND_DATAW fd;
        the::wstring p = path;
        if (!isPattern) {
            if (!the::text::string_utility::ends_with(p, vislib::sys::Path::SEPARATOR_W)) {
                if (!the::text::string_utility::ends_with(p, L":")) {
                    p += vislib::sys::Path::SEPARATOR_W;
                }
            }
            p += L"*.*";
        }

        this->findHandle = FindFirstFileW(p.c_str(), &fd);
        if (this->findHandle == INVALID_HANDLE_VALUE) {
            unsigned int le = ::GetLastError();
            if ((le == ERROR_FILE_NOT_FOUND) || (le == ERROR_PATH_NOT_FOUND)) {
                this->nextItem.Path.clear();
            } else {
                throw the::system::system_exception(le, __FILE__, __LINE__);
            }
        } else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            this->nextItem.Type = Entry::DIRECTORY;
            if (this->omitFolders || (wcscmp(fd.cFileName, L".") == 0)
                    || (wcscmp(fd.cFileName, L"..") == 0)) {
                this->fetchNextItem();
            } else {
                this->nextItem.Path = fd.cFileName;
            }
        } else {
            this->nextItem.Type = Entry::FILE;
            this->nextItem.Path = fd.cFileName;
        }
#else /* _WIN32 */
        if (isPattern) {
            the::text::string_converter::convert(this->basePath, vislib::sys::Path::GetDirectoryName(path));
            the::text::string_converter::convert(this->pattern, path + (this->basePath.size() + 1));

        } else {
            the::text::string_converter::convert(this->basePath, path);
            this->pattern.clear();

        }
        if (vislib::sys::File::Exists(this->basePath.c_str())) {
            if ((this->dirStream = opendir(this->basePath.c_str())) == NULL) {
                throw the::system::system_exception(__FILE__, __LINE__);
            }
            this->fetchNextItem();
        } else {
            this->dirStream = NULL;
            this->nextItem.Path.clear();
        }
#endif /* _WIN32 */
    }
    

    /*
     * DirectoryIterator<T>::fetchNextItem
     */
    template<> void DirectoryIterator<the::astring>::fetchNextItem(void) {
#ifdef _WIN32
        WIN32_FIND_DATAA fd;
        unsigned int le;
        bool found = false;
        do {
            if (this->findHandle == INVALID_HANDLE_VALUE) {
                this->nextItem.Path.clear();
            } else if (FindNextFileA(this->findHandle, &fd) == 0) {
                if ((le = GetLastError()) == ERROR_NO_MORE_FILES) {
                    this->nextItem.Path.clear();
                    found = true;
                } else {
                    throw the::system::system_exception(le, __FILE__, __LINE__);
                }
            } else {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    this->nextItem.Type = Entry::DIRECTORY;
                    if (!this->omitFolders && (strcmp(fd.cFileName, ".") != 0)
                            && (strcmp(fd.cFileName, "..") != 0)) {
                        found = true;
                    }
                } else {
                    this->nextItem.Type = Entry::FILE;
                    found = true;
                }
                this->nextItem.Path = fd.cFileName;
            }
        } while (!found);
#else /* _WIN32 */
        struct dirent *de = NULL;
        if (this->dirStream != NULL) {
            do {
                // BUG: Linux documentation is all lies. the errno stunt does not work at all.
                //errno = 0;
                if ((de = readdir(this->dirStream)) != NULL) {
                    if (!this->pattern.empty()) {
                        if (!vislib::sys::FilenameGlobMatch(de->d_name, this->pattern.c_str())) {
                            continue; // one more time
                        }
                    }
                    if (vislib::sys::File::IsDirectory((this->basePath + Path::SEPARATOR_A + de->d_name).c_str())) {
                        this->nextItem.Type = Entry::DIRECTORY;
                        if (this->omitFolders) continue; // one more time
                        if ((strcmp(de->d_name, "..") != 0) && (strcmp(de->d_name, ".") != 0)) break;
                    } else {
                        this->nextItem.Type = Entry::FILE;
                        break;
                    }
                }
            } while (de != NULL);
        }
        if (de == NULL) {
            this->nextItem.Path.clear();
        }
        else {
            this->nextItem.Path = de->d_name;
        }
#endif /* _WIN32 */
    }


    /*
     * DirectoryIterator<T>::fetchNextItem
     */
    template<> void DirectoryIterator<the::wstring>::fetchNextItem(void) {
#ifdef _WIN32
        WIN32_FIND_DATAW fd;
        unsigned int le;
        bool found = false;
        do {
            if (this->findHandle == INVALID_HANDLE_VALUE) {
                this->nextItem.Path.clear();
            } else if (FindNextFileW(this->findHandle, &fd) == 0) {
                if ((le = GetLastError()) == ERROR_NO_MORE_FILES) {
                    this->nextItem.Path.clear();
                    found = true;
                } else {
                    throw the::system::system_exception(le, __FILE__, __LINE__);
                }
            } else {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    this->nextItem.Type = Entry::DIRECTORY;
                    if (!this->omitFolders && (wcscmp(fd.cFileName, L".") != 0)
                            && (wcscmp(fd.cFileName, L"..") != 0)) {
                        found = true;
                    }
                } else {
                    this->nextItem.Type = Entry::FILE;
                    found = true;
                }
                this->nextItem.Path = fd.cFileName;
            }
        } while (!found);
#else /* _WIN32 */
        struct dirent *de = NULL;
        if (this->dirStream != NULL) {
            do {
                // BUG: Linux documentation is all lies. the errno stunt does not work at all.
                //errno = 0;
                if ((de = readdir(this->dirStream)) != NULL) {
                    if (!this->pattern.empty()) {
                        if (!vislib::sys::FilenameGlobMatch(de->d_name, this->pattern.c_str())) {
                            continue; // one more time
                        }
                    }
                    if (vislib::sys::File::IsDirectory((this->basePath + Path::SEPARATOR_A + de->d_name).c_str())) {
                        this->nextItem.Type = Entry::DIRECTORY;
                        if (this->omitFolders) continue; // one more time
                        if ((strcmp(de->d_name, "..") != 0) && (strcmp(de->d_name, ".") != 0)) break;
                    } else {
                        this->nextItem.Type = Entry::FILE;
                        break;
                    }
                }
            } while (de != NULL);
        }
        if (de == NULL) {
            this->nextItem.Path.clear();
        } else {
            the::text::string_converter::convert(this->nextItem.Path, de->d_name);
        }
#endif /* _WIN32 */
    }


} /* end namespace sys */
} /* end namespace vislib */
