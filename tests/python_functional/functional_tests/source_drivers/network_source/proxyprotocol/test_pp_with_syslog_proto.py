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
from src.message_builder.log_message import LogMessage


def write_proxy_protocol_message_to_file(path, header, message):
    f = open(path, "w")
    f.write(header)
    f.write(message + "\n")
    f.close()


def create_proxy_proto_bsd_input(bsd_formatter, hostname):
    loggen_input_path = str(tc_parameters.WORKING_DIR) + "/loggen_input.txt"

    header = "PROXY TCP4 192.168.1.1 192.168.1.2 20000 20001\r\n"

    input_log_message = LogMessage().hostname(hostname)
    input_message = bsd_formatter.format_message(input_log_message)
    expected_message = bsd_formatter.format_message(input_log_message.remove_priority())

    write_proxy_protocol_message_to_file(loggen_input_path, header, input_message)

    return loggen_input_path, expected_message


def test_pp_with_syslog_proto(config, port_allocator, syslog_ng, bsd_formatter, loggen):
    network_source = config.create_network_source(ip="localhost", port=port_allocator(), transport="proxied-tcp")
    file_destination = config.create_file_destination(file_name="output.log")
    config.create_logpath(statements=[network_source, file_destination])

    syslog_ng.start(config)

    loggen_input_path, expected_message = create_proxy_proto_bsd_input(bsd_formatter, network_source.options["ip"])

    loggen.start(
        network_source.options["ip"], network_source.options["port"],
        inet=True, stream=True, read_file=loggen_input_path, dont_parse=True,
    )

    assert file_destination.read_log() == expected_message
