//
// Automated Testing Framework (atf)
//
// Copyright (c) 2008 The NetBSD Foundation, Inc.
// All rights reserved.
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

extern "C" {
#include <signal.h>
}

#include <cstdlib>
#include <iostream>

#include "atf/application.hpp"
#include "atf/procs.hpp"
#include "atf/sanity.hpp"
#include "atf/text.hpp"
#include "atf/ui.hpp"

class atf_kill_tree : public atf::application::app {
    static const char* m_description;

    int m_signo;

    std::string specific_args(void) const;
    options_set specific_options(void) const;
    void process_option(int, const char*);

public:
    atf_kill_tree(void);

    int main(void);
};

const char* atf_kill_tree::m_description =
    "atf-kill-tree recursively kills a process and all of its "
    "children.";

atf_kill_tree::atf_kill_tree(void) :
    app(m_description, "atf-kill-tree(1)", "atf(7)"),
    m_signo(SIGTERM)
{
}

std::string
atf_kill_tree::specific_args(void)
    const
{
    return "pid";
}

atf_kill_tree::options_set
atf_kill_tree::specific_options(void)
    const
{
    using atf::application::option;
    options_set opts;
    opts.insert(option('s', "signal", "The signal to send (default SIGTERM)"));
    return opts;
}

void
atf_kill_tree::process_option(int ch, const char* arg)
{
    switch (ch) {
    case 's':
        m_signo = atf::text::to_type< int >(arg);
        break;

    default:
        UNREACHABLE;
    }
}

int
atf_kill_tree::main(void)
{
    if (m_argc == 0)
        throw atf::application::usage_error("Missing process identifier");

    pid_t pid = atf::text::to_type< pid_t >(m_argv[0]);

    atf::procs::pid_grabber pg;
    atf::procs::errors_vector errors = kill_tree(pid, m_signo, pg);
    for (atf::procs::errors_vector::const_iterator iter = errors.begin();
         iter != errors.end(); iter++) {
        pid_t epid = (*iter).first;
        const std::string& emsg = (*iter).second;

        std::string msg = "PID " + atf::text::to_string(epid) + ": " + emsg;
        std::cerr << atf::ui::format_error(m_prog_name, msg) << std::endl;
    }
    return errors.empty() ? EXIT_SUCCESS : EXIT_FAILURE;
}

int
main(int argc, char* const* argv)
{
    return atf_kill_tree().run(argc, argv);
}
