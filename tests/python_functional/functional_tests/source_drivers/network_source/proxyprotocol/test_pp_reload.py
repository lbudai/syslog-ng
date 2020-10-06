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
NUMBER_OF_MESSAGES = 3


def test_pp_reload(config, syslog_ng, loggen, port_allocator):
    loggen_filename = str(tc_parameters.WORKING_DIR) + "/loggen_input.txt"
    write_to_file(loggen_filename, "PROXY TCP4 1.1.1.1 2.2.2.2 3333 4444\r\n")
    for i in range(NUMBER_OF_MESSAGES):
        write_to_file(loggen_filename, "message%s\n" % i)

    network_source = config.create_network_source(ip="localhost", port=port_allocator(), transport='"proxied-tcp"', flags="no-parse")
    file_destination = config.create_file_destination(file_name="output.log", template=TEMPLATE)
    config.create_logpath(statements=[network_source, file_destination])

    syslog_ng.start(config)
    loggen.start(network_source.options["ip"], network_source.options["port"], stream=True, rate=1, read_file=loggen_filename, dont_parse=True)

    # With the current loggen implementation there is no way to properly timing messages.
    # Here I made an assumption that with rate=1, there will be messages which will arrive
    # into syslog-ng AFTER the reload. Timing the reload after the first message arrives.

    # Note: The worst case scenario in case of extreme slowness in the test environment, is
    # that syslog-ng will receive all the messages before reload. In this case the test will
    # not fill it's purpose, and do not test if the headers are reserved between reloads.
    # But at least the test wont be flaky, it will pass in this corner case too.

    log_message = file_destination.read_log()
    assert log_message == "1.1.1.1 2.2.2.2 3333 4444 4 message0\n"

    syslog_ng.reload(config)

    for i in range(1, NUMBER_OF_MESSAGES):
        log_message = file_destination.read_log()
        assert log_message == ("1.1.1.1 2.2.2.2 3333 4444 4 message%d\n" % i)
