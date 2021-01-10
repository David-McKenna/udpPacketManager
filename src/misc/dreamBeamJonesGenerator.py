#!/usr/bin/env python3
import argparse
import numpy as np
import itertools

from dreambeam.rime.scenarios import on_pointing_axis_tracking

from casacore.measures import measures
from astropy.time import Time, TimeDelta
import lofarantpos.db
import tqdm

import sys

def generateJones(subbands, antennaSet, stn, mdl, time, dur, inte, pnt, firstOutput = False):

	results = {}
	# Antenna list is sorted -> reverse the order so that we process the LBAs first if multiple antenna sets are used
	for ant in reversed(antennaSet):
		# Get the Jones Matrix data

		if isinstance(pnt, dict):
			pntLocal = pnt[ant]
		else:
			pntLocal = pnt
		__, __, antJones, __ = on_pointing_axis_tracking("LOFAR", stn, ant, mdl, time, dur, inte, pntLocal, do_parallactic_rot=True)
		
		# Update results with the output for the specified subbands
		for subband in subbands:
			if subband[0] == ant:
				results[subband[1]] = antJones[subband[2], :] 

	# Initialise the output array and append the data in the required order
	jonesMatrix = np.empty((0, antJones.shape[1], 2, 2), dtype = antJones.dtype)
	for subband in subbands:
		jonesMatrix = np.append(jonesMatrix, results[subband[1]], axis = 0)

	# Swap the time and frequency axis
	jonesMatrix = jonesMatrix.transpose((1, 0, 2, 3))
	# Invert the matrix so it can be applied to the visibility data
	invJonesMatrix = np.linalg.inv(jonesMatrix)

	# Split out the real and imaginary components into the last axis rather than being accessible by .real and .imag
	jointInvJones = np.zeros([jonesMatrix.shape[0], jonesMatrix.shape[1], 2, 4], dtype = np.float32)
	jointInvJones[:, :, :, ::2] = invJonesMatrix[...].real
	jointInvJones[:, :, :, 1::2] = invJonesMatrix[...].imag

	# If we only want a single sample,just retrun the first value this works around the behaviour where
	# dreamBeam has a minimum output of 2 time samples.
	if firstOutput:
		return jointInvJones[0]
	else:
		return jointInvJones


def pipeJones(pipeDest, silence, jointInvJones, antennaSet):

	# Open the output FIFO in binary mode
	# We will transfer all data as ASCII chars for easier decoding.
	with open(pipeDest, 'wb') as pipeRef:
		# Write out the number of time and frequency samples
		pipeRef.write(f"{jointInvJones.shape[0]},{jointInvJones.shape[1]}\n".encode("ascii"))
	
		# Write out the Jones matrices for each time sample
		for timeIdx in range(jointInvJones.shape[0]):
			jonesTimeSample = f"{','.join(jointInvJones[timeIdx, ...].ravel().astype(str))}|".encode("ascii")
			
			pipeRef.write(jonesTimeSample)

		# For debugging, optionally print out the results
		if not silence:
			for ant in reversed(antennaSet):
				print(f"\n\ndreamBeam pipe: {ant} {','.join([','.join([str(sb) for sb in sub[1:]]) for sub in subbands if sub[0] == ant])}\n")
				print(jonesTimeSample)




# Default integration, 2^16 packets
defaultInt = 2**16 * (5.12 * 10 ** -6) * 16
if __name__ == '__main__':
	# dreamBeamJonesGenerator.py --stn STNID --sub ANT,SBL:SBU --time TIME --dur DUR --int INT --pnt P0,P1,BASIS --pipe /tmp/pipe 
	parser = argparse.ArgumentParser()
	parser.add_argument('--stn', dest = 'stn', required = True, help = "Observing station code eg. IE613, SE607, etc.")
	parser.add_argument('--sub', dest = 'sub', required = True, nargs = '+', help = "Observing antenna set and subbands. Add 512 to access mode 7 subbands from the HBAs. Comma separated values, on ranges the upper limit is included to match bamctl logic. eg. 'ANT,SUBLWR:SUBUPR,SUB,SUB,SUB LBA,12:488,499,500,510 HBA,12:128 HBA,540:590")
	# Need more models as inputs...
	parser.add_argument('--mdl', dest = 'mdl', default = "Hamaker-default", help = "Antenna simulation model", choices = {"Hamaker-default", "Hamaker"})
	parser.add_argument('--time', dest = 'time', required = True, help = "Time of the first sample (UTC, UNIX seconds)")
	parser.add_argument('--dur', dest = 'dur', default = 10., type = float, help = "Total duration of time to sample")
	parser.add_argument('--int', dest = 'inte', default = defaultInt, type = float, help = "Integration time per step")
	parser.add_argument('--pnt', dest = 'pnt', required = True, help = "Pointing of the source, eg '0.1,0.3,J2000")
	parser.add_argument('--pipe', dest = 'pipe', default = '/tmp/udp_pipe', help = "Where to pipe the output data")
	parser.add_argument('--silent', dest = 'silent', default = True, action = 'store_false', help = "Don't silence all outputs.")

	try:
		args = parser.parse_args()

	# If we failed while parsing, try to write out to the pipe before exiting.
	except Exception as e:
		print(e)
		if "--pipe" in sys.argv:
			pipeInput = sys.argv.index('--pipe')

			with open(sys.argv[pipeInput + 1], 'wb') as outPipe:
				outPipe.write("-1,-1\n".encode("ascii"))

		exit(1)

	# Determine if both HBA and LBAs are needed
	antennaSet = list(set([antSet.split(',')[0].upper() for antSet in args.sub]))
	antennaSet.sort()

	# Split the pointing into it's components
	args.pnt = args.pnt.split(',')
	args.pnt[0] = float(args.pnt[0])
	args.pnt[1] = float(args.pnt[1])

	# Convert the parameters to astropy Time-like objects
	args.time = Time(args.time, format = 'mjd')
	args.dur = TimeDelta(args.dur, format = 'sec')
	args.inte = TimeDelta(args.inte, format = 'sec')

	# Determine all the subbands we need to extract data for
	subbands = []
	for subStr in args.sub:
		result = []
		splitSub = subStr.split(',')

		for ele in splitSub[1:]:
			if ':' in ele:
				# Generate an input for range, either a:b or a:b:c where c is stepping
				subs = list(map(int, ele.split(':')))
				# Include the last subband
				subs[1] += 1
				# Generate all the frequency subbands
				result += list(range(*subs))

			else:
				result += [int(ele)]

		# Accumulate the results
		subbands.append([splitSub[0].upper(), subStr, result])


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

		obsTime = args.time - args.inte / 2
		numSamples = int(np.ceil(args.dur / args.inte).value)
		for i in tqdm.trange(numSamples):
			obsTime = obsTime + args.inte
			time = dm.epoch('utc', f'{obsTime.mjd}d')
			dm.do_frame(time)

			for ant in antennaSet:
				obs = dm.position('ITRF', f'{obsLoc[ant][0]}m', f'{obsLoc[ant][1]}m', f'{obsLoc[ant][2]}m')
				dm.do_frame(obs)
				newPnt = dm.measure(pnt, 'J2000')
				args.pnt[ant] = [newPnt['m0']['value'], newPnt['m1']['value'], 'J2000']

			if jointInvJones is None:
				antJones = generateJones(subbands, antennaSet, args.stn, args.mdl, obsTime.datetime, args.dur.datetime, args.inte.datetime, args.pnt, firstOutput = True)
				jointInvJones = np.empty((numSamples, antJones.shape[0], 2, 4), dtype = antJones.dtype)
				jointInvJones[i, ...] = antJones
			else:
				jointInvJones[i, ...] = generateJones(subbands, antennaSet, args.stn, args.mdl, obsTime.datetime, args.dur.datetime, args.inte.datetime, args.pnt, firstOutput = True)

		print(time)

	else:
		jointInvJones = generateJones(subbands, antennaSet, args.stn, args.mdl, args.time.datetime, args.dur.datetime, args.inte.datetime, args.pnt, firstOutput = False)
	
	pipeJones(args.pipe, args.silent, jointInvJones, antennaSet)
