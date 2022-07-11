#!/usr/bin/python
# vim:set foldmethod=marker:

import codecs
import copy

def writePairs(f, offset, name, pairs, codepage):#{{{
    f.write(offset + "%s = {\n" % name)
    f.write(offset + "\t%i;" % len(pairs))
    # f.write("\t\t{\n")
    first = True
    for pair in pairs:
        if(first):
            f.write("\n" + offset + "\t{\n")
            first = False
        else:
            f.write(",\n" + offset + "\t{\n")

        first = pair[0].encode('windows-1251', 'strict')
        try:
            second = pair[1].encode('windows-%i' % codepage, 'strict')
        except:
            print "ERROR: unable to encode following strings in codepage %i" % codepage
            print pair[0]
            print pair[1]
            second = ""

        f.write(offset + '\t\tfirst = "%s";\n' % first)
        f.write(offset + '\t\tsecond = "%s";\n' % second)
        
        f.write(offset + "\t}")

    f.write("\n")
    f.write("\t};\n")
    #}}}

import re
import string
import unicodedata

ascii_symbols = ""

for i in xrange(0, 128):
    ascii_symbols += chr(i)

def needTranslation(text):
    chars = text # (text)
    for c in chars:
        if not c in ascii_symbols:
            return True
    return False

class Dictionary:

    def __init__(self):
        self.values_ = dict()
        self.language_ = ""
        self.codepage_ = 1251
        self.useFallback_ = True
        pass

    
    def clone(s):
        result = Dictionary()
        result.language_ = s.language_
        result.codepage_ = s.codepage_
        result.setValues(s)
        return result


    def codePage(s):
        return s.codepage_

    def setValues(s, rhs):
        assert(not s is rhs)
        s.values_   = copy.deepcopy(rhs.values_)

    def clear(s): s.values_ = dict()
    def name(s): return s.language_
    def setLanguage(s, language): s.language_ = language
    def codepage(s): return s.codepage_

    def countTranslated(s):
        result = 0
        for v in s.values_.items():
            if v[1] != u"":
                result += 1
        return result

    def items(self): return self.values_.items()
    def values(self): return self.values_.values()
    def __len__(self): return len(self.values_)

    def __dict__(key): return self.values_[key]
    def __setitem__(self, item, value): self.values_[item] = value


    def splitPairs(self, outUntranslated, outTranslated):#{{{
        for pair in self.items():
            if(len(pair[1]) == 0):
                outUntranslated += [pair]
            else:
                outTranslated += [pair]
        #}}}

    def splitDicts(self, outUntranslated, outTranslated):#{{{
        for pair in self.items():
            if pair[1] == u"":
                outUntranslated[pair[0]] = u""
            else:
                outTranslated[pair[0]] = pair[1]
        #}}}

    def importHeaders(self, fileName):#{{{
        print "\n** Reading StringTable Headers %s" % fileName

        f = file(fileName, 'r')

        linem = re.compile('^\t\t"(.*)"')

        in_header = False
        for line in f:
            if not in_header:
                if(line == "\theader = {\n"):
                    print "found header"
                    in_header = True
            else:
                result = linem.match(line)
                if not result is None:
                    value = unicode((result.groups()[0]), 'windows-1251')
                    if(needTranslation(value)):
                        self.values_[value] = u""
                else:
                    if(line == "};"):
                        break

        #}}}

    def importPot(self, fileName):#{{{
        print "\n** Reading PO Template %s" % fileName

        f = file(fileName, 'r')

        msgid = re.compile('^msgid "(.*)"$', re.M)
        msgstr = re.compile('^msgstr "(.*)"$', re.M)
        juststr = re.compile('^"(.*)"$', re.M)

        key = u""

        lastWasKey = True
        for line in f:
            result = msgid.match(line)
            if not result is None:
                key = unicode(result.groups()[0], 'utf-8')
                lastWasKey = True
            else:
                result = msgstr.match(line)
                if not result is None:
                    self.values_[key] = result.groups()[0]
                    lastWasKey = False
                else:
                    result = juststr.match(line)
                    if not result is None:
                        if lastWasKey:
                            key += result.groups()[0]
                        else:
                            self.values_[key] += result.groups()[0]
        self.values_.pop(u"")

        print "Got %i messages" % len(self.values_)#}}}

    def importVistaEngineScr(s, fileName):#{{{
        try:
            f = file(fileName, 'r')

            if(not f):
                print "Unable to open %s!" % fileName
                return

            print "Parsing %s" % fileName
            nodename = re.compile('\s*NodeName = "(.*)";$', re.M)

            key = u""

            keys = s.values_.keys()

            lastWasKey = True
            for line in f:
                result = nodename.match(line)
                if not result is None:
                    key = unicode(result.groups()[0], 'windows-1251')
                    if key != u"" and not key in keys:
                        s.values_[key] = u""
                    continue

            try:
                s.values_.pop(u"")
            except:
                pass

            print "Got %i messages" % len(s.values_)#}}}#}}}

        except:
            print "Error opening VistaEngine.scr"

    def mergeKeys(self, d):#{{{
        my_keys = values_.keys
        keys = d.keys()
        result = 0
        for key in keys:
            if not key in my_keys:
                self.values_[key] = u""
                result += 1
        return result
    #}}}

    def mergeTranslations(self, d):#{{{
        pairs = d.items()
        result = 0
        for pair in pairs:
            if (pair[0] in self.values_) and pair[1] != u"":
                self.values_[pair[0]] = pair[1]
                result += 1
        return result
    #}}}

    def load(s, fileName):#{{{
        print "\n** Reading Dictionary Script %s" % fileName
        import re

        f = file(fileName, 'r')
        first = re.compile('\s*first = "(.*)";', re.M)
        second = re.compile('\s*second = "(.*)";', re.M)

        language = re.compile('\s*language = "(.*)";', re.M)
        codepage = re.compile('\s*codePage = (\d+);', re.M)
        useFallback = re.compile('\s*useFallback = (.*);', re.M)

        key = u""
        for line in f:
            result = first.match(line)
            if not result is None:
                key = unicode(result.groups()[0], 'windows-1251')
                continue

            result = second.match(line)
            if not result is None:
                s.values_[key] = unicode((result.groups()[0]), 'windows-%i' % s.codepage_)
                continue

            result = language.match(line)
            if not result is None:
                s.language_ = result.groups()[0]
                continue

            result = useFallback.match(line)
            if not result is None:
                if(result.groups()[0] == "true"):
                    s.useFallback_ = True
                else:
                    s.useFallback_ = False
                continue

            result = codepage.match(line)
            if not result is None:
                s.codepage_ = int(result.groups()[0])
                continue

    def wipeNonTranslatable(self):
        keys = self.values_.keys()
        for key in keys:
            if not needTranslation(key):
                self.values_.pop(key)

    def untranslate(s, value):
        for pair in s.items():
            if pair[1] == value:
                return pair[0]

        return None

    def save(s, fileName):#{{{
        f = file(fileName, 'w')

        translated = []
        untranslated = []

        for pair in s.values_.items():
            assert(not pair is None)
            if pair[1] is None:
                pass
            else:
                if(len(pair[1]) == 0):
                    untranslated += [pair]
                else:
                    translated += [pair]

        f.write("Version = 0;\n")
        f.write("Dictionary = {\n")
        #f.write('\tlanguage = "%s";\n' % s.language_)
        if(s.useFallback_):
            f.write('\tuseFallback = true;\n')
        else:
            f.write('\tuseFallback = false;\n')
        f.write('\tcodePage = %i;\n' % s.codepage_)

        writePairs(f, "\t", "untranslated", untranslated, s.codepage_)
        writePairs(f, "\t", "translated", translated, s.codepage_)

        f.write("};\n")#}}}

