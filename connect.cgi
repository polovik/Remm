#!/usr/local/bin/python

import os, sys
# Import modules for CGI handling 
import cgi, cgitb 

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

print "Content-type:text/html\r\n\r\n"
print '<html>'
print '<head>'
print '<title>Connect with copter</title>'
print '</head>'
print '<body>'

# http://copter.ho.ua/connect.cgi?host=client&cmd=prepare
host = form.getvalue('host')
cmd = form.getvalue('cmd')
if (cmd != None):
	if (cmd == "prepare"):
		if (host == "client"):
			server_info = open("server_info", "w")
			server_info.close()
			print "Success. Server info cleared." + "<br>"
		elif (host == "server"):
			client_info = open("client_info", "w")
			client_info.close()
			print "Success. Client info cleared." + "<br>"
		else:
			print "Error. Incorrect host id = " + host + "<br>"
	else:
		print "Error. Incorrect command cmd = " + cmd + "<br>"
	print '</body>'
	print '</html>'
	sys.exit()

# Get data from fields
# a=candidate:Sc0a80a5f 1 UDP 1862270975 91.149.128.89 51756 typ srflx
# a=candidate:%s %d %s %d %s %d typ %s", foundation, &comp_id, transport, &prio, ipaddr, &port, type
# http://copter.ho.ua/connect.cgi?host=client&foundation=Sc0a80a5f&comp_id=1&prio=1862270975&ip=192.168.1.1&port=51756&type=srflx

foundation = form.getvalue('foundation')
comp_id = form.getvalue('comp_id')
prio = form.getvalue('prio')
ip = form.getvalue('ip')
port = form.getvalue('port')
type = form.getvalue('type')

if ((host == None) or (foundation == None) or (comp_id == None) or (prio == None) or (ip == None) or (port == None) or (type == None)):
	print "Error. Incorrect query list.<br>"
	print '</body>'
	print '</html>'
	sys.exit()

info = "a=candidate:%s %s UDP %s %s %s typ %s" % (foundation, comp_id, prio, ip, port, type)

client_info = open("client_info", "r+")
server_info = open("server_info", "r+")

client = client_info.read()
server = server_info.read()

if (host == "client"):
	client_info.seek(0, 0)
	client_info.truncate()
	client_info.write(info)
	print "Success. server:" + server + "<br>"
elif (host == "server"):
	server_info.seek(0, 0)
	server_info.truncate()
	server_info.write(info)
	print "Success. client:" + client + "<br>"
else:
	print "Error. Incorrect host id = " + host + "<br>"
	print '</body>'
	print '</html>'
	client_info.close()
	server_info.close()
	sys.exit()

client_info.close()
server_info.close()

# print "<font size=+1>Environment</font><br>";
# for param in os.environ.keys():
#     print "<b>%20s</b>: %s<br>" % (param, os.environ[param])

# print "<h2>Hello %s %s</h2>" % (format, ip)

print '</body>'
print '</html>'

