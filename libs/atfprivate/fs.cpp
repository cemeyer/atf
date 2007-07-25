//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007 The NetBSD Foundation, Inc.
// All rights reserved.
//
// This code is derived from software contributed to The NetBSD Foundation
// by Julio M. Merino Vidal, developed as part of Google's Summer of Code
// 2007 program.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. All advertising materials mentioning features or use of this
//    software must display the following acknowledgement:
//        This product includes software developed by the NetBSD
//        Foundation, Inc. and its contributors.
// 4. Neither the name of The NetBSD Foundation nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
// CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "config.h"

extern "C" {
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <unistd.h>
}

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <memory>

#include "atfprivate/exceptions.hpp"
#include "atfprivate/fs.hpp"
#include "atfprivate/ui.hpp" // XXX For split.

namespace impl = atf::fs;
#define IMPL_NAME "atf::fs"

// ------------------------------------------------------------------------
// Auxiliary functions.
// ------------------------------------------------------------------------

//!
//! \brief Normalizes a path.
//!
//! Normalizes a path string by removing any consecutive separators.
//! The returned string should be used to construct a path object
//! immediately.
//!
static
std::string
normalize(const std::string& s)
{
    if (s.empty())
        throw impl::path_error("Path cannot be empty");

    std::vector< std::string > cs = atf::split(s, "/");
    std::string data = (s[0] == '/') ? "/" : "";
    for (std::vector< std::string >::size_type i = 0; i < cs.size(); i++) {
        data += cs[i];
        if (i != cs.size() - 1)
            data += '/';
    }

    return data;
}

// ------------------------------------------------------------------------
// The "path_error" class.
// ------------------------------------------------------------------------

impl::path_error::path_error(const std::string& w) :
    std::runtime_error(w.c_str())
{
}

// ------------------------------------------------------------------------
// The "path" class.
// ------------------------------------------------------------------------

impl::path::path(const std::string& s) :
    m_data(normalize(s))
{
}

const char*
impl::path::c_str(void)
    const
{
    return m_data.c_str();
}

const std::string&
impl::path::str(void)
    const
{
    return m_data;
}

bool
impl::path::is_absolute(void)
    const
{
    return m_data[0] == '/';
}

bool
impl::path::is_root(void)
    const
{
    return m_data == "/";
}

impl::path
impl::path::branch_path(void)
    const
{
    std::string branch;

    std::string::size_type endpos = m_data.rfind('/');
    if (endpos == std::string::npos)
        branch = ".";
    else if (endpos == 0)
        branch = "/";
    else
        branch = m_data.substr(0, endpos);

#if defined(HAVE_CONST_DIRNAME)
    assert(branch == ::dirname(m_data.c_str()));
#endif // defined(HAVE_CONST_DIRNAME)

    return path(branch);
}

std::string
impl::path::leaf_name(void)
    const
{
    std::string::size_type begpos = m_data.rfind('/');
    if (begpos == std::string::npos)
        begpos = 0;
    else
        begpos++;

    std::string leaf = m_data.substr(begpos);

#if defined(HAVE_CONST_BASENAME)
    assert(leaf == ::basename(m_data.c_str()));
#endif // defined(HAVE_CONST_BASENAME)

    return leaf;
}

bool
impl::path::operator==(const path& p)
    const
{
    return m_data == p.m_data;
}

bool
impl::path::operator!=(const path& p)
    const
{
    return m_data != p.m_data;
}

impl::path
impl::path::operator/(const std::string& p)
    const
{
    return path(m_data + "/" + normalize(p));
}

impl::path
impl::path::operator/(const path& p)
    const
{
    return path(m_data + "/" + p.m_data);
}

// ------------------------------------------------------------------------
// The "file_info" class.
// ------------------------------------------------------------------------

impl::file_info::file_info(const path& p) :
    m_path(p)
{
    struct stat sb;

    if (::stat(p.c_str(), &sb) == -1)
        throw atf::system_error(IMPL_NAME "::file_info(" + p.str() + ")",
                                "stat(2) failed", errno);

    switch (sb.st_mode & S_IFMT) {
    case S_IFBLK:  m_type = blk_type;  break;
    case S_IFCHR:  m_type = chr_type;  break;
    case S_IFDIR:  m_type = dir_type;  break;
    case S_IFIFO:  m_type = fifo_type; break;
    case S_IFLNK:  m_type = lnk_type;  break;
    case S_IFREG:  m_type = reg_type;  break;
    case S_IFSOCK: m_type = sock_type; break;
    case S_IFWHT:  m_type = wht_type;  break;
    default:
        throw std::runtime_error(IMPL_NAME "::file_info(" + p.str() + "): "
                                 "stat(2) returned an unknown file type");
    }

    m_device = sb.st_dev;
    m_mode = sb.st_mode & ~S_IFMT;
}

dev_t
impl::file_info::get_device(void)
    const
{
    return m_device;
}

const impl::path&
impl::file_info::get_path(void)
    const
{
    return m_path;
}

impl::file_info::type
impl::file_info::get_type(void)
    const
{
    return m_type;
}

bool
impl::file_info::is_owner_readable(void)
    const
{
    return m_mode & S_IRUSR;
}

bool
impl::file_info::is_owner_writable(void)
    const
{
    return m_mode & S_IWUSR;
}

bool
impl::file_info::is_owner_executable(void)
    const
{
    return m_mode & S_IXUSR;
}

bool
impl::file_info::is_group_readable(void)
    const
{
    return m_mode & S_IRGRP;
}

bool
impl::file_info::is_group_writable(void)
    const
{
    return m_mode & S_IWGRP;
}

bool
impl::file_info::is_group_executable(void)
    const
{
    return m_mode & S_IXGRP;
}

bool
impl::file_info::is_other_readable(void)
    const
{
    return m_mode & S_IROTH;
}

bool
impl::file_info::is_other_writable(void)
    const
{
    return m_mode & S_IWOTH;
}

bool
impl::file_info::is_other_executable(void)
    const
{
    return m_mode & S_IXOTH;
}

// ------------------------------------------------------------------------
// The "directory" class.
// ------------------------------------------------------------------------

impl::directory::directory(const path& p)
{
    DIR* dp = ::opendir(p.c_str());
    if (dp == NULL)
        throw system_error(IMPL_NAME "::directory::directory(" +
                           p.str() + ")", "opendir(3) failed", errno);

    struct dirent* dep;
    while ((dep = ::readdir(dp)) != NULL) {
        path entryp = p / dep->d_name;
        insert(value_type(dep->d_name, file_info(entryp)));
    }

    if (::closedir(dp) == -1)
        throw system_error(IMPL_NAME "::directory::directory(" +
                           p.str() + ")", "closedir(3) failed", errno);
}

std::set< std::string >
impl::directory::names(void)
    const
{
    std::set< std::string > ns;

    for (const_iterator iter = begin(); iter != end(); iter++)
        ns.insert((*iter).first);

    return ns;
}

// ------------------------------------------------------------------------
// Free functions.
// ------------------------------------------------------------------------

impl::path
impl::change_directory(const path& dir)
{
    path olddir = get_current_dir();

    if (olddir != dir) {
        if (::chdir(dir.c_str()) == -1)
            throw system_error(IMPL_NAME "::chdir(" + dir.str() + ")",
                               "chdir(2) failed", errno);
    }

    return olddir;
}

impl::path
impl::create_temp_dir(const path& tmpl)
{
    std::auto_ptr< char > buf(new char[tmpl.str().length() + 1]);
    std::strcpy(buf.get(), tmpl.c_str());
    if (::mkdtemp(buf.get()) == NULL)
        throw system_error(IMPL_NAME "::create_temp_dir(" +
                           tmpl.str() + "/", "mkdtemp(3) failed",
                           errno);
    return path(buf.get());
}

bool
impl::exists(const path& p)
{
    bool ok;

    int res = ::access(p.c_str(), F_OK);
    if (res == 0)
        ok = true;
    else if (res == -1 && errno == ENOENT)
        ok = false;
    else
        throw system_error(IMPL_NAME "::exists(" + p.str() + ")",
                           "access(2) failed", errno);

    return ok;
}

impl::path
impl::get_current_dir(void)
{
#if defined(MAXPATHLEN)
    char buf[MAXPATHLEN];

    if (::getcwd(buf, sizeof(buf)) == NULL)
        throw system_error(IMPL_NAME "::get_current_dir",
                           "getcwd(3) failed", errno);

    return path(buf);
#else // !defined(MAXPATHLEN)
#   error "Not implemented."
#endif // defined(MAXPATHLEN)
}

static
void
rm_rf_aux(const impl::path& p, const impl::path& root,
          const impl::file_info& rootinfo)
{
    impl::file_info pinfo(p);
    if (pinfo.get_device() != rootinfo.get_device())
        throw std::runtime_error("Cannot cross mount-point " +
                                 p.str() + " while removing " +
                                 root.str());

    if (pinfo.get_type() != impl::file_info::dir_type) {
        if (::unlink(p.c_str()) == -1)
            throw atf::system_error(IMPL_NAME "::rm_rf(" + p.str() + ")",
                                    "unlink(" + p.str() + ") failed",
                                    errno);
    } else {
        impl::directory dir(p);
        dir.erase(".");
        dir.erase("..");

        for (impl::directory::iterator iter = dir.begin(); iter != dir.end();
             iter++) {
            const impl::file_info& entry = (*iter).second;

            rm_rf_aux(entry.get_path(), root, rootinfo);
        }

        if (::rmdir(p.c_str()) == -1)
            throw atf::system_error(IMPL_NAME "::rm_rf(" + p.str() + ")",
                                    "rmdir(" + p.str() + ") failed", errno);
    }
}

//!
//! \brief Recursively removes a directory tree.
//!
//! Recursively removes a directory tree, which can contain both files and
//! subdirectories.  This will raise an error if asked to remove the root
//! directory.
//!
//! Mount points are not crossed; if one is found, an error is raised.
//!
static
void
rm_rf(const impl::path& p)
{
    if (p.is_root())
        throw std::runtime_error("Cannot delete root directory for "
                                 "safety reasons");

    rm_rf_aux(p, p, impl::file_info(p));
}

static
std::vector< impl::path >
find_mount_points(const impl::path& p, const impl::file_info& parentinfo)
{
    impl::file_info pinfo(p);
    std::vector< impl::path > mntpts;

    if (pinfo.get_type() == impl::file_info::dir_type) {
        impl::directory dir(p);
        dir.erase(".");
        dir.erase("..");

        for (impl::directory::iterator iter = dir.begin(); iter != dir.end();
             iter++) {
            const impl::file_info& entry = (*iter).second;
            std::vector< impl::path > aux =
                find_mount_points(entry.get_path(), pinfo);
            mntpts.insert(mntpts.end(), aux.begin(), aux.end());
        }
    }

    if (pinfo.get_device() != parentinfo.get_device())
        mntpts.push_back(p);

    return mntpts;
}

void
impl::cleanup(const path& p)
{
    std::vector< path > mntpts = find_mount_points(p, file_info(p));
    for (std::vector< path >::const_iterator iter = mntpts.begin();
         iter != mntpts.end(); iter++) {
        const path& p2 = *iter;
        if (::unmount(p2.c_str(), 0) == -1)
            throw system_error(IMPL_NAME "::cleanup(" + p.str() + ")",
                               "unmount(" + p2.str() + ") failed", errno);
    }
    rm_rf(p);
}
