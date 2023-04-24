#!/bin/env bash

#set -x
numtest=9
if [[ $(find ./testCase*/ -type f | wc -l) -eq $((numtest * 4)) ]]; then
  exit 0
fi;

rm -r testCase{1..9} 2> >(grep -v "No such file or directory" >&2)
mkdir testCase{1..9}



baseFiles=( udp*000 )
secondFile=${baseFiles[1]}
packetLen=7824

# Extract the first N packets and append them to a set file
function headPacket () {

	file="${1}"
	numPack="${2}"
	packLen="${3:-$packetLen}"

	numBytes=$((numPack * packLen))
	touch "${file}_out"
	head -c ${numBytes} "${file}" >> "${file}_out"

}

# Extract the data from a file after N packets and append them to a set file
function dropPacket () {

	file="${1}"
	numPack="${2}"
	packLen="${3:-$packetLen}"

	numBytes=$((numPack * packLen + 1)) # Offset by 1 for tail -c +<n>
	touch "${file}_out"
	tail -c +${numBytes} "${file}" >> "${file}_out"

}

for fil in "${baseFiles[@]}"; do

	# testCase 2: ZSTD compressed
	zstd -3 "${fil}" -o testCase2/"${fil}.zst" >/dev/null

	# testCase 1, 3, 4, 5: Unmodified baseline copy
	for num in 1 {3..5}; do
	  cp "${fil}" testCase"${num}"/
	done;
done

# testCase 3: Simple Packet Misalignment
# Drop 1 packet on the second port of data
dropPacket "${secondFile}" 1
mv "${secondFile}_out" testCase3/"${secondFile}"


# testCase 4: Drop a full iteration (32)
dropPacket "${secondFile}" 33
mv "${secondFile}_out" testCase4/"${secondFile}"


# testCase 5: Drop a full port
rm testCase5/"${secondFile}"
touch testCase5/"${secondFile}"


# testCase 6: Delay a full port
dropPackets=32
for fil in "${baseFiles[@]}"; do
  headPacket "${fil}" ${dropPackets}
  mv "${fil}"_out testCase6/"${fil}"
done
dropPacket "${secondFile}" $((dropPackets + 1))
mv "${secondFile}_out" testCase6/"${secondFile}"


# testCase 7: Target search packet is missing
for fil in "${baseFiles[@]}"; do
  headPacket "${fil}" ${dropPackets}
  dropPacket "${fil}" $((dropPackets + 8))
  mv "${fil}"_out testCase7/"${fil}"
done

# testCase 8: Target search packet is missing (and not in the current or next iteration)
for fil in "${baseFiles[@]}"; do
  headPacket "${fil}" ${dropPackets}
  dropPacket "${fil}" $((dropPackets + 33))
  mv "${fil}"_out testCase8/"${fil}"
done

# testCase 9: Large, unequal packet loss
dropPackets=8
idx=0
for fil in "${baseFiles[@]}"; do
  headPacket "${fil}" ${dropPackets}
  dropPacket "${fil}" $((dropPackets + 19 * idx))
  mv "${fil}"_out testCase9/"${fil}"
  idx=$((idx + 1))
done
