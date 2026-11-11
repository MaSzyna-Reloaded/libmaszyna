# A cab screen written for Python 2 only (a print statement): it does not compile on Python 3, so
# it runs on 2.7, and sends back the major version of the Python it runs on.
import sys


class fixture_python2_screen(object):
	def __init__(self, lookup_path):
		print "built in", lookup_path

	def manul_set_format(self, format_str):
		pass

	def get_width(self):
		return 1

	def get_height(self):
		return 1

	def render(self, state):
		return chr(0) * 4

	def getCommands(self):
		return [str(sys.version_info[0])]
