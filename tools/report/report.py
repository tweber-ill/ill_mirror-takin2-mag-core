#
# compiles proposal details for instrumental report
# @author Tobias Weber <tweber@ill.fr>
# @date feb-19
# @license see 'LICENSE' file
#


#
# constants
#
prop_detail_url = "https://userclub.ill.eu/userclub/proposalSearch/details/%d"



#
# downloads proposal detail information
#
def get_prop_details(prop_no, session_no):

	infos = { \
		"allproposers": [], "alllabs": [], "allcountries": [], \
		"allinstruments": [], "allreqdays": [], "allallocdays": [], "allgrades": [], "allschedules": [], \
	}

	#import requests as req
	import requests_html as req

	#with req.Session() as session:
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

			infos["allinstruments"].append(row[0].strip())
			infos["allreqdays"].append(row[1].strip())
			infos["allallocdays"].append(row[2].strip())
			infos["allgrades"].append(row[3].strip())
			infos["allschedules"].append(row[4].strip())

		return infos
	return None




# -----------------------------------------------------------------------------

import sys

if len(sys.argv) < 2:
	print("Please specify a session id.")
	exit(-1)

session_no = sys.argv[1]

#prop_no = 79781
prop_no = 79659

infos = get_prop_details(prop_no, session_no)
print(infos)

# -----------------------------------------------------------------------------
