#
# compiles proposal details for instrumental report
# @author Tobias Weber <tweber@ill.fr>
# @date feb-19
# @license see 'LICENSE' file
#

import sys
import requests_html as req


#
# constants
#
prop_search_url = "https://userclub.ill.eu/userclub/proposalSearch"
prop_detail_url = "https://userclub.ill.eu/userclub/proposalSearch/details/%s"
filter_instr = True

sep = ";"
sep2 = ","



#
# search for all in20 proposals in a given span of years
#
def get_props(session_no, startyear, endyear):
	propids = []
	proptexts = []


	with req.HTMLSession() as session:

		cookies = { "JSESSIONID" : session_no }

		searchform = { \
			"selectedInstrument":		"50",
			"selectedCollege":		"-1",
			"accepted":			"true",
			"year":				"-1",
			"month":			"-1",
			"fromYear":			startyear,
			"toYear":			endyear,
			"_mainProposer":		"on",
			"_proposer":			"on",
			"localContact":			"true",
			"_localContact":		"on",
			"instrumentResponsible":	"true",
			"_instrumentResponsible":	"on",
			"_subcommitteeMember":		"on",
			"mainProposerName":		"",
			"proposerName":			"",
			"keywords":			"",
			"proposalNumber":		"",
		}

		reply = session.post(prop_search_url, cookies=cookies, data=searchform)
		if reply.status_code != 200:
			return None

		for elem in reply.html.xpath("body/div[1]/div[2]/div[2]/div[2]/table/tbody/*", clean=False):
			row = elem.xpath("tr/td/a/text()")
			rowlink = elem.xpath("tr/td/a/@href")

			council = row[0].strip()
			num = row[1].strip()
			proposer = row[3].strip()
			proptexts.append(num + " (" + council + ")" + " by " + proposer)

			link = rowlink[0].strip()
			propids.append(link[link.find("(")+1 : link.find(",")])

	return [ propids, proptexts ]


#
# downloads proposal detail information
#
def get_prop_details(session_no, prop_no):

	infos = { \
		"allproposers": [], "alllabs": [], "allcountries": [],
		"allinstruments": [], "allreqdays": [], "allallocdays": [], "allgrades": [], "allschedules": [], \
	}


	with req.HTMLSession() as session:

		cookies = { "JSESSIONID" : session_no }
		headers = { "User-Agent" : "n/a" }

		reply = session.get(prop_detail_url % prop_no, cookies=cookies, headers=headers)
		if reply.status_code != 200:
			return None

		# get main proposer
		for elem in reply.html.xpath("div[1]/div[3]/div[1]/p/text()", clean=False):
			name = elem.strip()
			if(name != ""):
				infos["mainproposer"] = name
				break

		# get local contact
		for elem in reply.html.xpath("div[1]/div[4]/p/text()", clean=False):
			name = elem.strip()
			if(name != ""):
				infos["localcontact"] = name
				break

		# get council
		for elem in reply.html.xpath("div[1]/div[1]/p/text()", clean=False):
			name = elem.strip()
			if(name != ""):
				infos["council"] = name
				break

		# get proposal id
		for elem in reply.html.xpath("div[1]/div[2]/div[1]/p/text()", clean=False):
			name = elem.strip()
			if(name != ""):
				infos["id"] = name
				break

		# get proposal title
		for elem in reply.html.xpath("div[1]/div[2]/div[2]/p/text()", clean=False):
			name = elem.strip()
			if(name != ""):
				infos["title"] = name
				break

		# get sample environments
		for elem in reply.html.xpath("div[1]/div[5]/p/text()", clean=False):
			name = elem.strip()
			if(name != ""):
				infos["env"] = name
				break

		# get all proposers and affiliations
		for elem in reply.html.xpath("div[1]/div[3]/div[3]/table/tbody/*", clean=False):
			row = elem.xpath("tr/td/text()")

			infos["allproposers"].append(row[0].strip())
			infos["alllabs"].append(row[1].strip())
			infos["allcountries"].append(row[2].strip())

		# get all requested instruments
		for elem in reply.html.xpath("div[1]/div[6]/table/tbody/*", clean=False):
			row = elem.xpath("tr/td/text()")
			row_alt = elem.xpath("tr/td/p/text()")

			infos["allinstruments"].append(row[0].strip())
			infos["allreqdays"].append(row[1].strip())
			infos["allallocdays"].append(row[2].strip())
			infos["allgrades"].append(row[3].strip())

			sched = row[4].strip()
			if sched == "" and len(row_alt) > 0:
				sched = row_alt[0].strip()

			infos["allschedules"].append(sched)


	# simplify proposer fields
	infos["allproposers_simple"] = ""
	for [prop, lab, country] in zip(infos["allproposers"], infos["alllabs"], infos["allcountries"]):
		if infos["allproposers_simple"] != "":
			infos["allproposers_simple"] += sep2 + " "
		infos["allproposers_simple"] += prop

	# remove separator character from fields, for csv export
	infos["title"] = infos["title"].replace(sep, sep2)
	infos["env"] = infos["env"].replace(sep, sep2)
	infos["localcontact"] = infos["localcontact"].replace(sep, sep2)
	infos["title"] = infos["title"].replace("\n", " ")
	infos["title"] = infos["title"].replace("\r", " ")

	return infos





# -----------------------------------------------------------------------------

# command line arguments
if len(sys.argv) < 4:
	print("Please specify a session id, a start and an end year.")
	exit(-1)

session_no = sys.argv[1]
startyear = sys.argv[2]
endyear = sys.argv[3]



# proposal list
print("Posting proposal search request ...")

[ propids, proptexts ] = get_props(session_no, startyear, endyear)

if propids == None:
	print("Error: Cannot get proposal list!")
	exit(-1)

print("Found %d proposals." % len(propids))
print()



# proposal details
infos = []
print("Retrieving proposal details ...")

for idx in range(len(propids)):
	propid = propids[idx]
	proptext = proptexts[idx]
	print("Requesting details for proposal %s ..." % proptext)

	info = get_prop_details(session_no, propid)
	if info == None:
		print("Error retrieving proposal details!")
		continue
	infos.append(info)
print()



# output results
with open("proposals.csv", "w") as file:
	print("Writing results to %s ..." % file.name)

	file.write("#\n")
	file.write("# Coucil" + sep + " " + \
		"Proposal No" + sep + " " + \
		"Title" + sep + " " + \
		"Main proposer" + sep + " " + \
		"All proposers" + sep + " " + \
		"Requested days" + sep + " " + \
		"Allocated days" + sep + " " + \
		"Grade" + sep + " " + \
		"Schedule" + sep + " " + \
		"Instrument" + sep + " " + \
		"Local contact" + sep + " " + \
		"Environment\n")
	file.write("#\n")

	for info in infos:
		#row = "{council:>10} {id:>12} {mainproposer} {allproposers_simple}".format(**info)

		for [instr, req, alloc, grade, sched] in zip(info["allinstruments"], info["allreqdays"], info["allallocdays"], info["allgrades"], info["allschedules"]):
			if filter_instr and instr.lower() != "in20":
				continue

			row = ("{council}" + sep + " " + \
				"{id}" + sep + " " + \
				"{title}" + sep + " " + \
				"{mainproposer}" + sep + " " + \
				"{allproposers_simple}" + sep + " " + \
				"{req}" + sep + " " + \
				"{alloc}" + sep + " " + \
				"{grade}" + sep + " " + \
				"{sched}" + sep + " " + \
				"{instr}" + sep + " " + \
				"{localcontact}" + sep + " " + \
				"{env}").format(**info, instr=instr, req=req, alloc=alloc, grade=grade, sched=sched)
			file.write(row)
			file.write("\n")

# -----------------------------------------------------------------------------
