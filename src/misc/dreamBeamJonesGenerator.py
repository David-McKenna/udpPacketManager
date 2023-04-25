#!/usr/bin/env python3

import argparse
import multiprocessing.shared_memory
from multiprocessing.resource_tracker import unregister
import lofarantpos.db
import numpy as np
import time as timeLib
from astropy.time import Time, TimeDelta
from casacore.measures import measures
from dreambeam.rime.scenarios import on_pointing_axis_tracking


def generateJones(subbands, antennaSet, stn, mdl, time, dur, inte, pnt, firstOutput=False):
    results = {}
    # Check which antenna we need to generate Jones matrices for
    for ant in ['LBA', 'HBA']:
        if ant not in antennaSet:
            continue

        # Get the pointing for the antenna set if needed
        if isinstance(pnt, dict):
            pntLocal = pnt[ant]
        else:
            pntLocal = pnt

        # Get the Jones Matrix data
        __, __, antJones, __ = on_pointing_axis_tracking("LOFAR", stn, ant, mdl, time, dur, inte, pntLocal,
                                                         do_parallactic_rot=True)

        # Update results with the output for the specified subbands
        for subband in subbands:
            if subband[0] == ant:
                results[subband[1]] = antJones[np.newaxis, subband[2], :]

    # Initialise the output array and append the data in the required order
    jonesMatrix = np.empty((0, antJones.shape[1], 2, 2), dtype=antJones.dtype)
    for subband in subbands:
        jonesMatrix = np.append(jonesMatrix, results[subband[1]], axis=0)

    # Swap the time and frequency axis
    jonesMatrix = jonesMatrix.transpose((1, 0, 2, 3))

    # Invert the matrix so it can be applied to the correlated data
    invJonesMatrix = np.linalg.inv(jonesMatrix)

    # Split out the real and imaginary components into the last axis rather than being accessible by .real and .imag
    # This allows us to pipe a constant steam of data without worrying about numpy layouts
    jointInvJones = np.zeros([jonesMatrix.shape[0], jonesMatrix.shape[1], 2, 4], dtype=np.float32)
    jointInvJones[:, :, :, ::2] = invJonesMatrix[...].real
    jointInvJones[:, :, :, 1::2] = invJonesMatrix[...].imag

    # If we only want a single sample,just return the first value.
    # This works around the behaviour where dreamBeam has a minimum output of 2 time samples
    #  (unless we fudge the duration to be less than the integration)
    if firstOutput:
        return jointInvJones[0]
    else:
        return jointInvJones


# Default integration, 2^16 packets
defaultInt = 2 ** 16 * (5.12 * 10 ** -6) * 16
if __name__ == '__main__':
    initTime = timeLib.perf_counter()
    # dreamBeamJonesGenerator.py --stn STNID --sub ANT,SBL:SBU --time TIME --dur DUR --int INT --pnt P0,P1,BASIS --pipe /tmp/pipe
    parser = argparse.ArgumentParser()
    parser.add_argument('--stn', dest='stn', required=True, help="Observing station code eg. IE613, SE607, etc.")
    parser.add_argument('--sub', dest='sub', required=True, help="Observing antenna set and subbands. Add 512 to access mode 7 subbands from the HBAs. Comma separated values, on ranges the upper limit is included to match bamctl logic. eg. 'SUBLWR:SUBUPR,SUB,SUB,SUB',  LBA: '12:488,499,500,510', HBA: '12:128 HBA,540:590'")
    # Need more models as inputs...
    parser.add_argument('--mdl', dest='mdl', default="Hamaker-default", help="Antenna simulation model",
                        choices={"Hamaker-default", "Hamaker"})
    parser.add_argument('--time', dest='time', required=True, help="Time of the first sample (UTC, MJD)")
    parser.add_argument('--dur', dest='dur', default=10., type=float, help="Total duration of time to sample")
    parser.add_argument('--int', dest='inte', default=defaultInt, type=float, help="Integration time per step")
    parser.add_argument('--pnt', dest='pnt', required=True, help="Pointing of the source, eg '0.1,0.3,J2000")
    parser.add_argument('--shm_key', dest='shm_key', required = True, type = str, help="Key of the shared memory location")
    parser.add_argument('--shm_size', dest='shm_size', required = True, type = int, help="Size of the buffer attached to the shared memory")
    parser.add_argument('--always-exact-position', dest='exact_pos', default=False, action='store_true',
                        help="When used, recalculate the exact RA/Dec in J2000 for sources in other coordinate systems at every time step.")
    parser.add_argument('--silent', dest='silent', default=True, action='store_false',
                        help="Don't silence all outputs.")

    args = parser.parse_args()
    # Split up the input subband string
    args.sub = list(map(int, args.sub.split(',')))



    # Split the pointing into it's components
    args.pnt = args.pnt.split(',')
    args.pnt[0] = float(args.pnt[0])
    args.pnt[1] = float(args.pnt[1])

    # Convert the parameters to astropy Time-like objects
    args.time = Time(args.time, format='mjd')
    args.dur = TimeDelta(args.dur, format='sec')
    args.inte = TimeDelta(args.inte, format='sec')

    # Determine all the subbands, and antenna, we need to extract data for
    subbands = []
    beamlet = 0
    antennaSet = set()
    for sub in args.sub:
        if sub < 512:
            subbands.append(("LBA", sub, beamlet))
            antennaSet.add('LBA')
        else:
            subbands.append(("HBA", sub - 512, beamlet))
            antennaSet.add('HBA')
        beamlet += 1

    endParseTime = timeLib.perf_counter()
    print(f"dreamBeamJonesGenerator.py: Parsing completed in {endParseTime - initTime:.3f} seconds.")
    print(f"dreamBeamJonesGenerator.py: Generating {int(args.dur / args.inte) + 1} Jones Matrices for {len(subbands)} subbands at MJD={args.time.mjd}")

    # If we aren't in J2000, find the Ra/Dec of the source at each step of the observation (slow, but accurate.)
    if args.pnt[2] != 'J2000':
        jointInvJones = None

        # Generate a casacore measures object
        dm = measures()
        # Generate a LOFAR station location object
        db = lofarantpos.db.LofarAntennaDatabase()
        obsLoc = {}
        for ant in antennaSet:
            obsLoc[ant] = db.phase_centres[f'{args.stn}{ant}']
        obsTime = args.time + args.dur / 2

        pnt = dm.direction(args.pnt[2], f'{args.pnt[0]} rad', f'{args.pnt[1]} rad')

        # Make args.pnt a dict if we have both LBA and HBA antenna in use
        args.pnt = {}

        # Center the integration time in the middle of the samples
        obsTime = args.time - args.inte / 2
        numSamples = int(np.ceil(args.dur / args.inte).value)

        # For each time sample, calculate the time and determine a J2000 coordinate for the source, which can then be used to determine the Jones matrices
        if args.exact_pos:
           for i in range(numSamples):
                obsTime = obsTime + args.inte
                time = dm.epoch('utc', f'{obsTime.mjd}d')
                dm.do_frame(time)

                for ant in antennaSet:
                    obs = dm.position('ITRF', f'{obsLoc[ant][0]}m', f'{obsLoc[ant][1]}m', f'{obsLoc[ant][2]}m')
                    dm.do_frame(obs)
                    newPnt = dm.measure(pnt, 'J2000')
                    args.pnt[ant] = [newPnt['m0']['value'], newPnt['m1']['value'], 'J2000']

                if jointInvJones is None:
                    antJones = generateJones(subbands, antennaSet, args.stn, args.mdl, obsTime.datetime, args.dur.datetime,
                                             args.inte.datetime, args.pnt, firstOutput=True)
                    jointInvJones = np.empty((numSamples, antJones.shape[0], 2, 4), dtype=antJones.dtype)
                    jointInvJones[i, ...] = antJones
                else:
                    jointInvJones[i, ...] = generateJones(subbands, antennaSet, args.stn, args.mdl, obsTime.datetime,
                                                          args.dur.datetime, args.inte.datetime, args.pnt, firstOutput=True)
        else:
            # Otherwise, just use the pointing at the midpoint of the observation
            obsTime += args.dur / 2
            time = dm.epoch('utc', f'{obsTime.mjd}d')
            dm.do_frame(time)
            for ant in antennaSet:
                obs = dm.position('ITRF', f'{obsLoc[ant][0]}m', f'{obsLoc[ant][1]}m', f'{obsLoc[ant][2]}m')
                dm.do_frame(obs)
                newPnt = dm.measure(pnt, 'J2000')
                args.pnt[ant] = [newPnt['m0']['value'], newPnt['m1']['value'], 'J2000']

            jointInvJones = generateJones(subbands, antennaSet, args.stn, args.mdl, args.time.datetime, args.dur.datetime,
                                      args.inte.datetime, args.pnt, firstOutput=False)


    else:
        jointInvJones = generateJones(subbands, antennaSet, args.stn, args.mdl, args.time.datetime, args.dur.datetime,
                                      args.inte.datetime, args.pnt, firstOutput=False)

    jonesGeneratorTime = timeLib.perf_counter()
    print(f"dreamBeamJonesGenerator.py: {jointInvJones.shape[0]} Jones Matrices generated and inverted in {jonesGeneratorTime - endParseTime:.3f} seconds.")


    # Move the data into the provided shared memory array
    ref = multiprocessing.shared_memory.SharedMemory(name = args.shm_key, create = False)
    assert(ref.size == args.shm_size)
    data = np.frombuffer(ref.buf, dtype = np.float32, count = args.shm_size // jointInvJones.dtype.itemsize)
    if (data[0] * data[1] * 4 * 2) != (data.size - 2):
        ts = (data.size - 2) / 8 / jointInvJones.shape[1]
        print(f"dreamBeamJonesGenerator.py: Unexpected generated shape: Expected {(data.size - 2)} samples  ({ts} time samples), generated {jointInvJones.size} samples (({(jointInvJones.size) / 8 / jointInvJones.shape[1]} time samples), reducing output size.")
        jointInvJones = jointInvJones[:ts]
    data[0] = jointInvJones.shape[0]
    data[1] = jointInvJones.shape[1]
    data[2:] = jointInvJones.ravel()[:(args.shm_size // jointInvJones.dtype.itemsize) - 2] # Drop extra time samples if not necessary
    # Remove hanging references
    del data
    ref.close()

    # By default, Python will destroy any shared memory it has touched on exit
    # THANKS FOR DOCUMENTING THIS! /s
    # https://github.com/python/cpython/issues/82300
    unregister(ref._name, 'shared_memory')


    copiedTime = timeLib.perf_counter()
    print(f"dreamBeamJonesGenerator.py: Jones Matrices copied to {args.shm_key} shared memory in {copiedTime - jonesGeneratorTime:.3f} seconds.")