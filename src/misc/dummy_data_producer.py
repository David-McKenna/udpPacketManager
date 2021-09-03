import argparse
import numpy as np
import numpy.random as rdm
import os
import struct
import time
import tqdm

baseHeader = [struct.pack("<B", 3), struct.pack("<BH", 170, 6848), struct.pack("<B", 16)]
bitmodeBeamlets = {4: 244, 8: 122, 16: 61}
bitmodeConversion = {4: struct.pack("<B", 2), 8: struct.pack("<B", 1), 16: struct.pack("<B", 0)}
bitmodeReplayConversion = {4: struct.pack("<B", 2 + 128), 8: struct.pack("<B", 1 + 128), 16: struct.pack("<B", 0 + 128)}
clockConversion = {200: struct.pack("<B", 128), 160: struct.pack("<B", 0)}


def genericHdrBytes(bitmode, beamlets, clock, droppedPacket=False):
    if bitmodeBeamlets[bitmode] < beamlets or beamlets < 1:
        raise RuntimeWarning(
            f"Bitmode {bitmode} and only supply up to {bitmodeBeamlets[bitmode]} beamlets. Requested: {beamlets}")

    bitmodeByte = bitmodeConversion[bitmode]
    clockByte = clockConversion[clock]

    if droppedPacket:
        bitmodeByte = bitmodeReplayConversion[bitmode]
    else:
        bitmodeByte = bitmodeConversion[bitmode]

    hdr = baseHeader[0] + clockByte + bitmodeByte + baseHeader[1] + struct.pack("<B", beamlets) + baseHeader[2]

    print(hdr, len(hdr))

    return hdr


byteLookup = [struct.pack("<B", ele) for ele in range(256)]


def byteGenerator(size, **kwargs):
    packstr = '<' + "b" * size
    byteReturn = b''

    for byte in ((np.arange(size) + kwargs['seqi']) % 256 - 128):
        byteReturn += byteLookup[byte]
    return byteReturn


# return struct.pack(packstr, *(np.arange(size) % 128))

def orderedByteGenerator(size, **kwargs):
    data = np.arange(size / 4 / 122)
    data = np.repeat(data[..., np.newaxis], 122, -1)
    data = np.repeat(data[..., np.newaxis], 4, -1)
    data = data.T.reshape(-1).astype(int) % 256

    byteReturn = b''

    for byte in data:
        if kwargs['idx'] == 0: print(byte, byteLookup[byte])
        byteReturn += byteLookup[byte]
    return byteReturn


clockSamples = {200: 195312.5, 160: 156250}


def sequenceWriter(numPackets, outputloc='./debug.pcap', droppedPackets=[], generatorFunction=byteGenerator, bitmode=8,
                   beamlets=122, clock=200, initTime=None, bufferlen=10000, reference=False, zeroReference=False):
    packets = list(range(numPackets))

    for idx in reversed(droppedPackets):
        del packets[idx]

    packetDeltas = [ele - packets[idx] for idx, ele in enumerate(packets[1:])] + [1]

    tsi = int(initTime) or int(time.time())
    seqi = 0

    clockCount = clockSamples[clock]
    hdrBytes = genericHdrBytes(bitmode, beamlets, clock)

    if reference:
        hdrBytesReference = genericHdrBytes(bitmode, beamlets, clock, True)

    byteBuffer = b''
    print("Writing base file...")
    with open(outputloc, 'wb') as ref:
        for idx, dt in tqdm.tqdm(enumerate(packetDeltas), total=len(packetDeltas)):
            if (idx > 0 and idx % bufferlen == 0):
                ref.write(byteBuffer)
                byteBuffer = b''

            byteBuffer += hdrBytes
            byteBuffer += struct.pack("<i", tsi)  # Both are int, not unsigned like everyhting else
            byteBuffer += struct.pack("<i", seqi)
            byteBuffer += generatorFunction(beamlets * 16 * 4, bitmode=bitmode, beamlets=beamlets, clock=clock, idx=idx,
                                            seqi=seqi, packet=packets[idx])

            seqi += 16 * dt
            if (seqi > clockCount):
                seqi = seqi % clockCount
                tsi += 1
        ref.write(byteBuffer)

    if reference:
        print("Writing reference file...")
        referenceOut = outputloc.split('.')
        referenceOut.insert(-1, 'reference')
        referenceOut = '.'.join(referenceOut)
        structStr = '<' + "b" * (beamlets * 16 * 4)
        with open(referenceOut, 'wb') as ref:
            for idx, dt in tqdm.tqdm(enumerate(packetDeltas), total=len(packetDeltas)):
                if (idx > 0 and idx % bufferlen == 0):
                    ref.write(byteBuffer)
                    byteBuffer = b''

                byteBuffer += hdrBytes
                byteBuffer += struct.pack("<i", tsi)  # Both are int, not unsigned like everyhting else
                byteBuffer += struct.pack("<i", seqi)
                lastBuff = generatorFunction(beamlets * 16 * 4, bitmode=bitmode, beamlets=beamlets, clock=clock,
                                             idx=idx, seqi=seqi, packet=packets[idx])
                byteBuffer += lastBuff

                if dt > 1:
                    print("PacketLoss: ", dt)
                    for i in range(dt):
                        byteBuffer += hdrBytesReference
                        seqi += 16
                        if (seqi > clockCount):
                            seqi = seqi % clockCount
                            tsi += 1

                        byteBuffer += struct.pack("<i", tsi)  # Both are int, not unsigned like everyhting else
                        byteBuffer += struct.pack("<i", seqi)

                        if zeroReference:
                            byteBuffer += struct.pack(structStr, *(np.zeros(beamlets * 16 * 4, dtype=int)))
                        else:
                            byteBuffer += lastBuff

            ref.write(byteBuffer)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate some dummy LOFAR CEP UDP data.')

    parser.add_argument('-n', dest='num_packets', default=128, type=int, help="Number of packets to generate.")
    parser.add_argument('-d', dest='dropped_packets', default='[]',
                        help="Indexes of packets to drop (with random: [maxLength, count]")
    parser.add_argument('-r', dest='random', action='store_true', default=False,
                        help="Generate packet loss through randomisation.")

    parser.add_argument('-b', dest='beamlets', default=122, type=int, help="Number of beamlets per packet.")
    parser.add_argument('-c', dest='clock', default=200, const=160, action='store_const',
                        help="Switch from the 200MHz clock to the 160MHz clock.")
    parser.add_argument('-l', dest='bits', default=8, type=int, help="Change the bitmode [4, (8), 16].")

    parser.add_argument('-t', dest='time', default=time.time(),
                        help="Set the Unix time of the first time sample (default: current time)")
    parser.add_argument('-w', dest='buffer', default=10000, type=int,
                        help="Number of packets to buffer before writing to disk")
    parser.add_argument('-g', dest='generator', default=orderedByteGenerator,
                        help="Provide a custom packet contents generator")

    parser.add_argument('-o', dest='outfile', default='./debug.pcap', help="Output file location")
    parser.add_argument('-e', dest='expected', default=False, action='store_true',
                        help="Save a reference (expected output file) fora LOFAR UDP comparison. (default: false)")
    parser.add_argument('-z', dest='zero', default=False, action='store_true',
                        help="Have replayed packets in the output reference be 0's instead of replaying the last packet. (default: false)")

    args = parser.parse_args()
    args.dropped_packets = [ele for ele in args.dropped_packets.strip('[').strip(']').split(',')]
    packetClone = []
    for ele in args.dropped_packets:
        if ':' in ele:
            packetClone += range(int(ele.split(':')[0]), int(ele.split(':')[1]))
        else:
            packetClone.append(int(ele))

    args.dropped_packets = packetClone

    if (args.random):
        assert (len(args.dropped_packets) == 2)
        tempPackets = (rdm.rand(args.dropped_packets[1]) * args.num_packets).astype(int)
        counts = (rdm.rand(args.dropped_packets[1]) * args.dropped_packets[0]).astype(int)

        args.dropped_packets = []
        for packetOffset, length in zip(tempPackets, counts):
            args.dropped_packets += list(range(packetOffset, packetOffset + length))

    assert (args.bits in [4, 8, 16])
    assert (args.clock in [160, 200])
    assert (args.time > 0)
    assert (args.buffer > 0)
    assert (not os.path.exists(args.outfile))

    sequenceWriter(numPackets=args.num_packets,
                   outputloc=args.outfile,
                   droppedPackets=args.dropped_packets,
                   generatorFunction=args.generator,
                   bitmode=args.bits,
                   beamlets=args.beamlets,
                   clock=args.clock,
                   initTime=args.time,
                   bufferlen=args.buffer,
                   reference=args.expected,
                   zeroReference=args.zero)
