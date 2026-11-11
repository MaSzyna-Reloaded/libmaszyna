# A cab screen that decodes a string of its state as cp1250, as the data's scripts do
# (scripts/tablica_pti.py), and sends back the code points of what it decoded, in hex.


class fixture_python2_cp1250_screen(object):
	def __init__(self, lookup_path):
		self.received = ''

	def manul_set_format(self, format_str):
		pass

	def get_width(self):
		return 1

	def get_height(self):
		return 1

	def render(self, state):
		self.received = ' '.join('%x' % ord(character) for character in state['do'].decode('windows-1250'))
		return chr(0) * 4

	def getCommands(self):
		return [self.received]
