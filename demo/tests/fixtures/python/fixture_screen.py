# A cab screen with the interface of python/local/abstractscreenrenderer.py and no dependency on it
# or on PIL, the same on Python 2 and 3: two RGBA pixels drawn from the state, and back as commands
# the format it was given, the directory it was built with, the state values as it received them
# and the major version of the Python it runs on.
import sys


class fixture_screen(object):
	def __init__(self, lookup_path):
		self.lookup_path = lookup_path
		self.format = "RGB"

	def manul_set_format(self, format_str):
		self.format = format_str

	def get_width(self):
		return 2

	def get_height(self):
		return 1

	def render(self, state):
		self.received = '%s;%s;%s;%s;%s' % (repr(state['level']), repr(state['step']), repr(state['enabled']), state['name'], repr(state['touches']))
		level = int(state['level'] * 255)
		alpha = 255 if state['enabled'] else 0
		return bytes(bytearray([level] * 4 + [state['step']] * 3 + [alpha]))

	def getCommands(self):
		return [self.format, self.lookup_path, self.received, str(sys.version_info[0])]
