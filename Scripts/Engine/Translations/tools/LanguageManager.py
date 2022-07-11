import os
from Dictionary import Dictionary
import copy

class LanguageManager:

    def __init__(s, rootDir, mainLanguage):

        s.languages_ = []
        s.rootDir = rootDir
        s.keyLanguage_ = 'English'

        s.revert()

    def setLanguages(s, langs):
        s.languages_ = []
        s.languages_ = copy.deepcopy(langs)

    def keyLanguage(s):
        for lang in s.languages_:
            if lang.name() == s.keyLanguage_:
                return lang
        print "!!! No key language"
        return None

    def revert(s):
        s.languages_ = []
        files = os.listdir(s.rootDir)
        for f in files:
            if(os.path.isfile(s.rootDir + "\\" + f)):
                s.languages_ += [s.loadLanguage(f)]

    def loadLanguage(s, name):
        newLanguage = Dictionary()
        newLanguage.load(s.rootDir + "\\" + name)
        newLanguage.setLanguage(name)
        return newLanguage


    def languagesInfo(s):
        result = []

        for lang in s.languages_:
            result += [(lang.name(), lang.countTranslated(), lang.codepage())]

        return result

    def saveAll(s):
        for language in s.languages_:
            name = language.name()
            language.save(s.rootDir + "\\" + name)

    def languages(s): return s.languages_

    def languageByName(s, name):
        for lang in s.languages_:
            if lang.name() == name:
                return lang
        return None

