#!/bin/bash
#
# search gh log data
# @author Tobias Weber <tweber@ill.fr>
# @date 11-jul-20
# @license see 'LICENSE' file
#

# string to search
strtofind="test"


# date ranges
y_start=2017
y_end=2017

m_start=11
m_end=11

d_start=11
d_end=11

h_start=0
h_end=23


# tools
WGET=wget
ZCAT=zcat


# base url for gh logs
GH="https://data.gharchive.org/"

# report file
report="found.txt"


function dl_log()
{
	local file="${GH}$1"
	echo -e "Downloading ${file} ... "

	wget -v ${file}
}


function find_str()
{
	local file="$1"
	local str="$2"
	local report_tmp="${report%.txt}.tmp"

	echo -en "File \"${file}\" ... " 

	if ${ZCAT} $file | grep $str > $report_tmp; then
		echo -e "match found" 
		echo -e "--------------------------------------------------------------------------------" >> $report
		echo -e "Matches for \"${str}\" in file \"${file}\"" >> $report
		echo -e "--------------------------------------------------------------------------------" >> $report

		cat $report_tmp >> $report
		rm -f $report_tmp
		echo -e "--------------------------------------------------------------------------------" >> $report
	else
		echo -e "no match" 
	fi
}


# delete old report
rm -fv $report

for y in $(seq $y_start $y_end); do
	for m in $(seq $m_start $m_end); do
		for d in $(seq $d_start $d_end); do
			for h in $(seq $h_start $h_end); do
				year="$(printf "%04d" $y)"
				month="$(printf "%02d" $m)"
				day="$(printf "%02d" $d)"
				hour="$(printf "%02d" $h)"

				file="${year}-${month}-${day}-${hour}.json.gz"

				# download and search in file
				if dl_log $file; then
					find_str "${file}" "${strtofind}"
				fi

				# delete downloaded file
				rm -fv $file
			done
		done
	done
done
