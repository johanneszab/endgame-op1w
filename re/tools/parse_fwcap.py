#!/usr/bin/env python3
"""Decode a USBPcap capture of an Endgame Gear firmware update.

Stdlib only -> runs on Linux. Understands the USBPcap pseudo-header (DLT 249)
and the vendor's report 0xA0 / 0xA1 framing described in re/firmware/FIRMWARE.md.

  python parse_fwcap.py <capture.pcap> [--payload N] [--all]

By default it prints device descriptors, every control transfer carrying a
feature report, and a one-line summary per block write. --all also prints the
interrupt and cancelled transfers, which is what you want when diagnosing a
failed run rather than reading a successful one.
"""
import struct, sys, collections

XFER = {0: 'ISOCH', 1: 'INTERRUPT', 2: 'CONTROL', 3: 'BULK'}

# USBD_STATUS values seen in practice around a firmware update.
USBD = {
    0x00000000: 'SUCCESS',
    0xC0010000: 'CANCELED',
    0xC0000004: 'STALL_PID',
    0x80000300: 'PENDING/ERROR',
}

# Report 0xA0 sub-commands, FIRMWARE.md §3.
BLDR = {
    0x01: 'echo test',
    0x03: 'start',
    0x06: 'write block',
    0x09: 'complete',
    0x3A: 'enter bootloader',
}


def packets(data):
    """Yield (ts, usec, hdr, payload) for each captured packet."""
    if struct.unpack_from('<I', data, 0)[0] != 0xA1B2C3D4:
        raise SystemExit('not a little-endian pcap')
    link = struct.unpack_from('<I', data, 20)[0]
    if link != 249:
        raise SystemExit('linktype %d, expected 249 (USBPcap)' % link)
    off = 24
    while off + 16 <= len(data):
        ts, us, caplen, _orig = struct.unpack_from('<IIII', data, off)
        off += 16
        pkt = data[off:off + caplen]
        off += caplen
        if len(pkt) < 27:
            continue
        hlen = struct.unpack_from('<H', pkt, 0)[0]
        irp, status, func, info, bus, dev, ep, xfer, dlen = \
            struct.unpack_from('<QIHBHHBBI', pkt, 2)
        yield ts, us, dict(hlen=hlen, status=status, func=func, info=info,
                           bus=bus, dev=dev, ep=ep, xfer=xfer, dlen=dlen,
                           stage=pkt[27] if xfer == 2 and hlen >= 28 else None), \
            pkt[hlen:hlen + dlen]


def describe_device_descriptor(p):
    # bLength 0x12 AND bDescriptorType 0x01. Checking only the second byte
    # matches any response whose status byte is 0x01 (= OK), which is most of
    # them, and reports every block acknowledgement as a device descriptor.
    if len(p) < 18 or p[0] != 0x12 or p[1] != 0x01:
        return None
    vid, pid, bcd = struct.unpack_from('<HHH', p, 8)
    # Hex digits, not decimal: 0x0110 is firmware 1.10, not 1.16.
    return 'DEVICE  VID %04X  PID %04X  bcdDevice %04X (firmware %x.%02x)' % (
        vid, pid, bcd, bcd >> 8, bcd & 0xFF)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    show_all = '--all' in sys.argv
    nbytes = 48
    for a in sys.argv[1:]:
        if a.startswith('--payload'):
            nbytes = int(a.split('=', 1)[1]) if '=' in a else 1040
    if not args:
        raise SystemExit(__doc__)

    data = open(args[0], 'rb').read()
    t0 = None
    blocks = collections.Counter()
    first_block = None
    n = 0
    for ts, us, h, payload in packets(data):
        n += 1
        t = ts + us / 1e6
        if t0 is None:
            t0 = t
        rel = t - t0
        st = USBD.get(h['status'], '%08X' % h['status'])

        # Device descriptors, wherever they appear.
        if payload:
            d = describe_device_descriptor(payload)
            if d:
                print('[%8.3f] dev=%d  %s' % (rel, h['dev'], d))
                continue

        if h['xfer'] == 2 and payload[:1] in (b'\x21', b'\xa1') and len(payload) >= 8:
            bm, br, wval, widx, wlen = struct.unpack_from('<BBHHH', payload, 0)
            body = payload[8:]
            kind = 'SET_REPORT' if br == 0x09 else ('GET_REPORT' if br == 0x01 else 'req%02X' % br)
            rid = wval & 0xFF
            note = ''
            if body[:1] == b'\xa0' and len(body) >= 2:
                sub = body[1]
                note = '  <- %s' % BLDR.get(sub, 'sub 0x%02X' % sub)
                if sub == 0x06 and len(body) >= 6:
                    index = struct.unpack_from('<H', body, 2)[0]
                    csum = struct.unpack_from('<H', body, 4)[0]
                    note += '  index=0x%04X sum16=0x%04X' % (index, csum)
                    blocks[index] += 1
                    if first_block is None:
                        first_block = body[16:16 + 16]
            print('[%8.3f] dev=%d %s rid=0x%02X len=%d %s%s' % (
                rel, h['dev'], kind, rid, wlen, st, note))
            if body and nbytes:
                print('           ', body[:nbytes].hex(' '))
        elif show_all:
            print('[%8.3f] dev=%d ep=%02X %-9s %s len=%d' % (
                rel, h['dev'], h['ep'], XFER.get(h['xfer'], h['xfer']), st, h['dlen']))

    print('\n%d packets' % n)
    if blocks:
        lo, hi = min(blocks), max(blocks)
        resent = {k: v for k, v in blocks.items() if v > 1}
        print('block writes : %d, indices 0x%04X..0x%04X%s' % (
            len(blocks), lo, hi, ', RESENT: %r' % resent if resent else ''))
        print('first block  : %s' % first_block.hex(' '))
        print('  compare against record 0 of the extracted FWFILE payload;')
        print('  a match proves the host streams the resource verbatim.')


if __name__ == '__main__':
    main()
