#!/usr/bin/python

#
# KiCon badge bootloader
#
# Copyright (c) 2015-2016, Atmel Corporation.
# Copyright (c) 2019 twl <twlostow@printf.cc>
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 2 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.

import sys
import time
import serial
import struct
import getopt


class SerialIF:
    def __init__(self, device="/dev/ttyACM0"):
        self.ser = serial.Serial(
            port=device, baudrate=4000000, timeout=0.1, rtscts=False, bytesize=8, parity='N', stopbits=1, xonxoff=0)
        self.ser.set_buffer_size(rx_size = 12800, tx_size = 12800)

    def send(self, x):
        if isinstance(x, str):
            for c in x.encode("ascii"):
                self.ser.write(struct.pack("B", c))
        elif isinstance(x, int):
            self.ser.write(struct.pack("B", x))
        else:
            self.ser.write(x)
        self.ser.flush()

    def recv(self, count=1, timeout=10):
        r = b''
        t_start = time.time()
        while True:
            try:
                state = self.ser.read(1)
                if len(state) > 0:
                    r = r + state
                    if len(r) == count:
                        return r
            except:
                time.sleep(0.01)
                t_current = time.time()
                if (t_current - t_start > timeout):
                    raise Exception(
                        "Serial interface timeout exceeded (%d seconds)" % timeout)

    def enter_bootloader(self):
        try:
            self.send("N#")
            r = self.recv(2)
        except:
            return False
        return r == b'\x0a\x0d'

    def samba_read_word(self, addr):
        self.send("w%08x,#" % addr)
        rv = self.recv(4)
        return struct.unpack("<L", rv)[0]

    def samba_write_word(self, addr, value):
        self.send("W%08x,%08x#" % (addr, value))

    def samba_read(self, addr, size):
        r = b''
        for n in range(0, (size+3)//4):
            self.send("w%08x,#" % (addr + n * 4))
            r = r + self.recv(4)
        return r

    def samba_go(self, addr):
        self.send("G%08x#" % (addr))

    def read_chip_id(self):
        R_CIDR = 0x400E0740
        R_EXID = 0x400E0744
        cidr = self.samba_read_word(R_CIDR)
        exid = self.samba_read_word(R_EXID)
        return (cidr, exid)


class AtmelFlash:
    def __init__(self):
        self.pages_per_sector = 16
        self.size = 0
        self.page_size = 0
        self.locks = []
        self.planes = []
        self.nb_locks = 0
        self.nb_planes = 0
        self.id = 0
        self.base_addr = 0x00400000

    def __str__(self):
        return "ID: 0x%08x [planes: %d, size: %d bytes, page size: %d bytes]" % (self.id, self.nb_planes, self.size, self.page_size)


class AtmelEEFC:
    PAGE_SIZE = 512
    SECTOR_SIZE = PAGE_SIZE * 16

    EEFC_FMR = 0x00000000
    EEFC_FCR = 0x00000004
    EEFC_FSR = 0x00000008
    EEFC_FRR = 0x0000000c

    EEFC_FCR_FKEY = (0x5a << 24)
    EEFC_FCR_FCMD_GETD = 0x00                 # Get Flash descriptor
    EEFC_FCR_FCMD_WP = 0x01                 # Write page
    EEFC_FCR_FCMD_WPL = 0x02                 # Write page and lock
    EEFC_FCR_FCMD_EWP = 0x03                 # Erase page and write page
    EEFC_FCR_FCMD_EWPL = 0x04                 # Erase page and write page then lock
    EEFC_FCR_FCMD_EA = 0x05                 # Erase all
    EEFC_FCR_FCMD_EPA = 0x07                 # Erase pages
    EEFC_FCR_FCMD_SLB = 0x08                 # Set lock bit
    EEFC_FCR_FCMD_CLB = 0x09                 # Clear lock bit
    EEFC_FCR_FCMD_GLB = 0x0A                 # Get lock bit
    EEFC_FCR_FCMD_SGPB = 0x0B                 # Set GPNVM bit
    EEFC_FCR_FCMD_CGPB = 0x0C                 # Clear GPNVM bit
    EEFC_FCR_FCMD_GGPB = 0x0D                 # Get GPNVM bit

    EEFC_FSR_FRDY = (1 << 0)
    EEFC_FSR_CMDE = (1 << 1)
    EEFC_FSR_FLOCKE = (1 << 2)
    EEFC_FSR_FLERR = (1 << 3)

    def __init__(self, iface, base_addr):
        self.base = base_addr
        self.iface = iface
        self.flash = AtmelFlash()

    def wait_ready(self):
        while True:
            rv = self.iface.samba_read_word(self.base + self.EEFC_FSR)
            if rv & self.EEFC_FSR_FRDY:
                return rv

    def send_command(self, cmd, arg):
        self.iface.samba_write_word(
            self.base + self.EEFC_FCR, self.EEFC_FCR_FKEY | (arg << 8) | cmd)
        return self.wait_ready()

    def read_result(self):
        return self.iface.samba_read_word(self.base + self.EEFC_FRR)

    def read_flash_id(self):
        self.send_command(self.EEFC_FCR_FCMD_GETD, 0)
        self.flash.id = self.read_result()
        self.flash.size = self.read_result()
        self.flash.page_size = self.read_result()
        self.flash.nb_planes = self.read_result()
        for i in range(0, self.flash.nb_planes):
            self.flash.planes.append(self.read_result())
        self.flash.nb_locks = self.read_result()
        for i in range(0, self.flash.nb_locks):
            self.flash.locks.append(self.read_result())
        return self.flash

    def set_page_lock(self, lock, enabled):
        cmd = self.EEFC_FCR_FCMD_SLB if enabled else self.EEFC_FCR_FCMD_CLB
        self.send_command(cmd, lock)

    def unlock_all(self):
        offset = 0
        for i in range(0, len(self.flash.locks)):
            self.set_page_lock(i, 0)

    def handle_status_error(self, status):
        if status & self.EEFC_FSR_FLOCKE:
            raise Exception("Write/erase error: at least one page is locked")
        elif status & self.EEFC_FSR_FLERR:
            raise Exception("Write/erase error: flash error")

    def erase_all(self):
        status = self.send_command(self.EEFC_FCR_FCMD_EA, 0)
        self.handle_status_error(status)

    def erase_sector(self, addr):
        first_page = (addr // self.SECTOR_SIZE) * 16
        status = self.send_command(self.EEFC_FCR_FCMD_EPA, first_page | 2)
        self.handle_status_error(status)

    def write_page(self, addr, data):
        buf = data
        if len(buf) < self.PAGE_SIZE:
            buf += b'\x00' * (self.PAGE_SIZE - len(buf))
        i = 0
        for d in struct.unpack("<%dL" % (self.PAGE_SIZE // 4), buf):
            self.iface.samba_write_word(self.flash.base_addr + addr + i * 4, d)
            i += 1

        status = self.send_command(
            self.EEFC_FCR_FCMD_WP, addr // self.PAGE_SIZE)
        self.handle_status_error(status)

    def read_page(self, addr):
        return self.iface.samba_read(self.flash.base_addr + addr, self.PAGE_SIZE)

    def set_gpnvm(self, gpnvm):
        status = self.send_command(self.EEFC_FCR_FCMD_SGPB, gpnvm)
        self.handle_status_error(status)


class AtmelFlashProgrammer:
    BASE_EEFC = 0x400E0A00

    EXPECTED_CIDR = 0x29970ce0
    EXPECTED_EXID = 0x00000000

    def __init__(self, device):
        self.iface = SerialIF(device)
        self.eefc = AtmelEEFC(self.iface, self.BASE_EEFC)

    def init(self):
        if not self.iface.enter_bootloader():
            raise Exception(
                "Can't enter SAM-BA bootloader mode. Did you forget to reset the board?")

        chip_id = self.iface.read_chip_id()

        if chip_id != (self.EXPECTED_CIDR, self.EXPECTED_EXID):
            raise Exception(
                "Unrecognized chip ID: 0x%08x. This tool is only for flashing the KiCon Badge. Use official Atmel SAM-BA for generic Atmel ARMs")

        self.flash = self.eefc.read_flash_id()

    def get_flash_info(self):
        return self.flash

    def program(self, image, verify=False, progressFunc=None):
        self.eefc.unlock_all()

        page_count = len(image) // self.flash.page_size

        for page in range(0, page_count+1):
            addr = page * self.flash.page_size
            remaining = len(image) - addr
            l = min(remaining, self.flash.page_size)
            buf = image[addr: addr + l]

            if page % self.flash.pages_per_sector == 0:
                progressFunc("Erasing", page, page_count)
                self.eefc.erase_sector(addr)

            progressFunc("Writing", page, page_count)
            self.eefc.write_page(addr, buf)


        for page in range(0, page_count+1):
            addr = page * self.flash.page_size
            remaining = len(image) - addr
            l = min(remaining, self.flash.page_size)
            buf = image[addr: addr + l]
            readback_buf = self.eefc.read_page(addr)[0:l]
            progressFunc("Verifying", page, page_count)
            if readback_buf != buf:
                raise Exception(
                    "Verification failed: offending page 0x%08x" % addr)

        self.eefc.set_gpnvm(1)  # boot from flash
 
    def run(self):
        self.iface.samba_go( self.flash.base_addr )


def report_progress(stage, n, max_n):
    width = 50
    percentage = float(n) / float(max_n)
    hashes = int(percentage * width)
    sys.stdout.write("%s: [%s%s]\r" %
                     (stage, "#" * hashes, " " * (width - hashes)))
    sys.stdout.flush()
    pass


def load_binary_file(filename, size_limit):
    try:
        f = open(filename, "rb")
    except Exception as error:
        raise Exception("Can't load file: %s : %s" % (filename, error))
    f.seek(0, 2)
    size = f.tell()
    f.seek(0, 0)
    if size > size_limit:
        raise Exception("Image size in %s is larger than the Flash size of the microcontroller (%d kBytes)" % (
            filename, size_limit/1024))
    data = f.read()
    f.close()
    return data


def main():
    if len(sys.argv) < 2:
        print("This is KiCon Badge Programmer, a simple script to program your KiCon badge over the USB port")
        print(
            "Usage: %s file_name.bin [device_with_serial_port]" % sys.argv[0])
        sys.exit(0)

    port = "/dev/ttyACM1"

    if len(sys.argv) > 2:
        port = sys.argv[2]

    filename = sys.argv[1]
    try:
        pgm = AtmelFlashProgrammer(port)
        pgm.init()
        image = load_binary_file(filename, pgm.get_flash_info().size)
        print("Programming: %s [%d bytes]" % (filename, len(image)))
        pgm.program(image, True, report_progress)
        print("\nProgramming complete, running the program.")
        pgm.run()
    except Exception as error:
        print("Error occured: %s" % error)
        sys.exit(-1)


if __name__ == "__main__":
    main()
