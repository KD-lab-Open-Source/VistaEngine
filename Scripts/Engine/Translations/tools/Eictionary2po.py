import re

f = file("Dictionary", 'r')

first = re.compile('\s*first = "(.*)";', re.M)
second = re.compile('\s*second = "(.*)";', re.M)

for line in f:
    first_res = first.match(line)
    second_res = second.match(line)
    if(not first_res is None):
        print 'msgid "%s"' % first_res.groups()[0]
    if(not second_res is None):
        print 'msgstr "%s"' % second_res.groups()[0]

