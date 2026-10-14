# A Python 2 cab screen with the interface of python/local/abstractscreenrenderer.py and no
# dependency on it or on PIL: two RGBA pixels drawn from the state, and back as commands the
# format it was given, the directory it was built with and the state values as it received them.
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
		return chr(level) * 4 + chr(state['step']) * 3 + chr(alpha)

	def getCommands(self):
		return [self.format, self.lookup_path, self.received]
