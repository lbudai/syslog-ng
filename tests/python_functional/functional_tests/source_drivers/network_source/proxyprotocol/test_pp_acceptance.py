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
from src.common.operations import write_to_file

TEMPLATE = r'"${PROXIED_SRCIP} ${PROXIED_DSTIP} ${PROXIED_SRCPORT} ${PROXIED_DSTPORT} ${PROXIED_IP_VERSION} ${MESSAGE}\n"'


def test_pp_acceptance(config, syslog_ng, loggen, port_allocator):
    loggen_filename = str(tc_parameters.WORKING_DIR) + "/loggen_input.txt"

    network_source = config.create_network_source(ip="localhost", port=port_allocator(), transport='"proxied-tcp"', flags="no-parse")
    file_destination = config.create_file_destination(file_name="output.log", template=TEMPLATE)
    config.create_logpath(statements=[network_source, file_destination])

    write_to_file(loggen_filename, "PROXY TCP4 1.1.1.1 2.2.2.2 3333 4444\r\n")
    write_to_file(loggen_filename, "first message")

    syslog_ng.start(config)

    loggen.start(network_source.options["ip"], network_source.options["port"], stream=True, read_file=loggen_filename, dont_parse=True)

    log_message = file_destination.read_log()
    assert log_message == "1.1.1.1 2.2.2.2 3333 4444 4 first message\n"
