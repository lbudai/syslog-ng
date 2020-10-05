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
from src.helpers.loggen.loggen import Loggen

TEMPLATE = r'"${PROXIED_SRCIP} ${PROXIED_DSTIP} ${PROXIED_SRCPORT} ${PROXIED_DSTPORT} ${PROXIED_IP_VERSION} ${MESSAGE}\n"'


def write_proxy_protocol_messages_to_file(path, header, messages):
    f = open(path, "w")
    f.write(header)
    for message in messages:
        f.write(message + "\n")
    f.close()


def create_input_with_id(idx):
    input_path = str(tc_parameters.WORKING_DIR) + "/loggen{}_input.txt".format(idx)

    ip_ver = "4"
    src_ip = "192.168.{}.1".format(idx)
    dst_ip = "192.168.{}.2".format(idx)
    src_port = "{}0000".format(idx)
    dst_port = "{}0001".format(idx)

    header = "PROXY TCP{} {} {} {} {}\r\n".format(ip_ver, src_ip, dst_ip, src_port, dst_port)

    input_message1 = "test message {}.1".format(idx)
    input_message2 = "test message {}.2".format(idx)

    write_proxy_protocol_messages_to_file(input_path, header, [input_message1, input_message2])

    expected_message1 = "{} {} {} {} {} {}\n".format(src_ip, dst_ip, src_port, dst_port, ip_ver, input_message1)
    expected_message2 = "{} {} {} {} {} {}\n".format(src_ip, dst_ip, src_port, dst_port, ip_ver, input_message2)

    return input_path, [expected_message1, expected_message2]


def test_pp_with_multiple_clients(config, port_allocator, syslog_ng):
    network_source = config.create_network_source(ip="localhost", port=port_allocator(), transport="proxied-tcp", flags="no-parse")
    file_destination = config.create_file_destination(file_name="output.log", template=TEMPLATE)
    config.create_logpath(statements=[network_source, file_destination])

    syslog_ng.start(config)

    loggen1_input_path, expected_messages1 = create_input_with_id(1)
    loggen2_input_path, expected_messages2 = create_input_with_id(2)

    # These 2 run simultaneously
    Loggen().start(
        network_source.options["ip"], network_source.options["port"], inet=True,
        stream=True, read_file=loggen1_input_path, dont_parse=True, rate=1,
    )
    Loggen().start(
        network_source.options["ip"], network_source.options["port"], inet=True,
        stream=True, read_file=loggen2_input_path, dont_parse=True, rate=1,
    )

    output_messages = file_destination.read_logs(counter=4)
    assert sorted(output_messages) == sorted(expected_messages1 + expected_messages2)
