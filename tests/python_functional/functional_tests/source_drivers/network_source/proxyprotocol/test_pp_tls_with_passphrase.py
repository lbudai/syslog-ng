#!/usr/bin/env python
#############################################################################
# Copyright (c) 2020 One Identity
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as published
# by the Free Software Foundation, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# As an additional exemption you are allowed to compile & link against the
# OpenSSL libraries as published by the OpenSSL project. See the file
# COPYING for details.
#
#############################################################################
import src.testcase_parameters.testcase_parameters as tc_parameters
from src.common.operations import copy_shared_file
from src.common.operations import write_to_file

TEMPLATE = r'"${PROXIED_SRCIP} ${PROXIED_DSTIP} ${PROXIED_SRCPORT} ${PROXIED_DSTPORT} ${PROXIED_IP_VERSION} ${MESSAGE}\n"'


def test_pp_tls_with_passphrase(config, syslog_ng, syslog_ng_ctl, port_allocator, loggen, testcase_parameters):
    server_key_path = copy_shared_file(testcase_parameters, "server-protected-asdfg.key")
    server_cert_path = copy_shared_file(testcase_parameters, "server-protected-asdfg.crt")

    network_source = config.create_network_source(
        ip="localhost",
        port=port_allocator(),
        transport='"proxied-tls"',
        flags="no-parse",
        tls={
            "key-file": server_key_path,
            "cert-file": server_cert_path,
            "peer-verify": '"optional-untrusted"',
        },
    )
    file_destination = config.create_file_destination(file_name="output.log", template=TEMPLATE)
    config.create_logpath(statements=[network_source, file_destination])

    syslog_ng.start(config)

    syslog_ng_ctl.credentials_add(credential=server_key_path, secret="asdfg")

    loggen_file_path = str(tc_parameters.WORKING_DIR) + "loggen_input.txt"
    write_to_file(loggen_file_path, "PROXY TCP4 1.1.1.1 2.2.2.2 3333 4444\n")
    write_to_file(loggen_file_path, "hello world\n")
    loggen.start(target=network_source.options["ip"], port=network_source.options["port"], use_ssl=True, read_file=loggen_file_path, dont_parse=True)

    log_message = file_destination.read_log()
    assert log_message == "1.1.1.1 2.2.2.2 3333 4444 4 hello world\n"
