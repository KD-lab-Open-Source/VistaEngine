#!/usr/bin/env python

import Dictionary
from Excel import Excel
import time
import os

def strFromExcel(text):
    assert(type(text) is unicode)
    return text

def strToExcel(text):
    assert(type(text) is unicode)
    return text

def writePairs(e, begin, end, pairs, progressCallback):
    assert(begin[0] == end[0])
    assert(begin[1] < end[1])
    column = begin[0]
    row = begin[1] + 1

    index = 0
    count = len(pairs)
    for pair in pairs:
        if(row >= end[1] - 1):
            e.insertRowBefore(end[1])
            end = (end[0], end[1] + 1)

        key = pair[0].replace(u"\\n", u"\n")
        value = pair[1].replace(u"\\n", u"\n")

        e.setCellText((column, row), key)
        e.setCellText((column + 1, row), value)

        row += 1
        index += 1
        progressCallback(float(index) / float(count))

def readPairs(e, begin, end, pairs, progressCallback, codePage):
    assert(begin[0] == end[0])
    assert(begin[1] < end[1])

    column = begin[0]
    row = begin[1] + 1

    count = end[1] - begin[1]
    for index in xrange(count):
        pair = (e.getCellText((column, row)), e.getCellText((column + 1, row)))
        if(not pair[0] is None and not pair[1] is None):
            if pair[0] != "" and pair[1] != "":
                key = strFromExcel(pair[0]).replace(u"\n", u"\\n")
                value = strFromExcel(pair[1]).replace(u"\n", u"\\n")
                pairs += [(key, value)]

        row += 1
        if(index % 10 == 0):
            progressCallback(float(index) / float(count))

def importTemp(dictionary, fileName, progressCallback):
    e = Excel()

    e.open(fileName)
    e.setCurrentSheet(1)
    
    column = 1
    begin = 3
    end = 960

    index = 0
    for row in xrange(begin, end):
        try:
            key = fromUnicode(e.getCellText((0, row))).replace("\n", "\\n")
            value = fromUnicode(e.getCellText((1, row))).replace("\n", "\\n")

            dictionary[key] = value
        except:
            print "vaaaa...", row
        progressCallback(float(index) / float(end - begin))
        ++index

def export(dictionary, fileName, outFileName, progressCallback):
    e = Excel()

    e.open(fileName)

    e.setCurrentSheet("$language")

    pos = e.findCellsByText("$language")[0]
    e.setCellText(pos, dictionary.name())

    pos = e.findCellsByText("$codepage")[0]
    e.setCellText(pos, 0)


    translated = []
    untranslated = []

    dictionary.splitPairs(untranslated, translated)

    untranslated_begin = e.findCellsByText("$untranslated_begin")
    untranslated_end = e.findCellsByText("$untranslated_end")
    if len(untranslated_begin) and len(untranslated_end):
        writePairs(e, untranslated_begin[0], untranslated_end[0], untranslated, progressCallback)

    translated_begin = e.findCellsByText("$translated_begin")
    translated_end = e.findCellsByText("$translated_end")
    if len(translated_begin) and len(translated_end):
        writePairs(e, translated_begin[0], translated_end[0], translated, progressCallback)
    
    e.show()
    if not outFileName is None:
        e.saveAs(outFileName)


def importSingleXLS(languageManager, fileName, progressCallback):

    e = Excel()
    e.open(fileName)

    langs = languageManager.languages()
    
    for lang in langs:
        keyLanguage = lang.name() == languageManager.keyLanguage().name()

        try:
            e.setCurrentSheet(lang.name())
        except:
            print 'Unable to select sheet "%s"' % lang.name()
            continue

        translated = []
        untranslated = []

        untranslated_begin = (0, 5)
        res = e.findCellsByText("> Translated")
        if len(res):
            untranslated_end = res[0]
        else:
            untranslated_end = (0, 6)

        readPairs(e, untranslated_begin, untranslated_end, untranslated, progressCallback, lang.codePage())
        if(len(untranslated)):
            for pair in untranslated:
                key = languageManager.keyLanguage().untranslate(pair[0])
                if(not key is None):
                    value = pair[1]
                    lang[key] = value
                else:
                    print "No such key: %s" % pair[0]

        usedRange = e.usedRange()

        res = e.findCellsByText("> Translated")
        if len(res):
            translated_begin = res[0]
        else:
            translated_begin = (0, usedRange[1])
        translated_end = (translated_begin[0], usedRange[1])

        readPairs(e, translated_begin, translated_end, translated, progressCallback, lang.codePage())

        if keyLanguage:
            pass
        else:
            pass

def importSingleSheetXLS(languageManager, fileName, progressCallback):

    e = Excel()
    print "Importing " + fileName
    e.open(fileName)

    langs = languageManager.languages()
    
    keyColumn = 0
    column = 1

    def readValue(pos):
        text = e.getCellText(pos)
        if not text is None:
            if text != "":
                text = strFromExcel(text).replace(u"\n", u"\\n")
                return text

    while True:
        languageName = e.getCellText((column, 0))
        if languageName == u"":
            break

        lang = languageManager.languageByName(languageName)
        if lang is None:
            print "Warning! Unknown language!"
            break

        translated = []
        untranslated = []

        row = 2
        last_row = e.usedRange()[1]

        print "last_row = %i\n" % last_row

        while row < last_row:
            key = readValue((keyColumn, row))
            value = readValue((column, row))

            if not key is None and key != u"":
                if value is None:
                    value = u""
                if key in lang.values_:
                    lang.values_[key] = value
                else:
                    lang.unusedValues_[key] = value

            row += 1
            if row % 10 == 0:
                progressCallback(float(row) / float(last_row))

        column += 1


def exportSingleSheetXLS(languageManager, progressCallback):
    e = Excel()
    e.newWorkbook()

    langs = languageManager.languages()

    count =  len(langs[0].items()) # * len(langs)
    index = 0
    column = 0

    def writeLanguage(lang, keys, keyLanguage, column):
        e.setColumnWidth(0, 56)
        e.setCellText((column, 0), lang.name())
        e.setCellText((column, 1), lang.codepage())
        e.setColumnWidth(column, 56)

        pos = (column, 2)

        index = 0

        for key in keys:
            pos = (pos[0], pos[1] + 1)

            if keyLanguage:
                e.setCellText(pos, key)
            else:
                value = lang.values_.get(key, u"")
                e.setCellText(pos, value)

            if index % 10 == 0:
                progressCallback(float(index) / float(count))
            index += 1

        #for pair in untranslated.items():
        #    (key, value) = pair

        #    pos = (pos[0], pos[1] + 1)
        #    if keyLanguage:
        #        e.setCellText(pos, key)
        #    else:
        #        e.setCellText(pos, value)
        #    if index % 10 == 0:
        #        progressCallback(float(index) / float(count))
        #    index += 1 

        #for key in keys:
        #    value = translated.get(key, None)
        #    if not value is None:
        #        pos = (pos[0], pos[1] + 1)
        #        if keyLanguage:
        #            e.setCellText(pos, key)
        #        else:
        #            e.setCellText(pos, value)
        #    if index % 10 == 0:
        #        progressCallback(float(index) / float(count))
        #    index += 1 

    keys = []

    for lang in langs:
        mainLanguage = lang.useFallback_ == False
        keyLanguage = lang.name() == languageManager.keyLanguage().name()
        if mainLanguage:
            keys = lang.values_.keys()
            writeLanguage(lang, keys, mainLanguage, column)
            column += 1

    for lang in langs:
        mainLanguage = lang.useFallback_ == False
        keyLanguage = lang.name() == languageManager.keyLanguage().name()
        if keyLanguage and not mainLanguage:
            writeLanguage(lang, keys, mainLanguage, column)
            column += 1

    for lang in langs:
        keyLanguage = lang.name() == languageManager.keyLanguage().name()
        if not keyLanguage and not mainLanguage:
            writeLanguage(lang, keys, mainLanguage, column)
            column += 1

    e.show()


def exportSingleXLS(languageManager, progressCallback):

    e = Excel()
    e.newWorkbook()

    langs = languageManager.languages()

    keyCells = {}
    refColumn = {}

    count = len(langs) * len(langs[0].items())
    index = 0

    for lang in langs:
        keyLanguage = (lang.name() == languageManager.keyLanguage().name())

        if keyLanguage:
            e.setCurrentSheet(1)
            e.duplicateSheet(lang.name())

            e.setCellText((0, 0), "Language:")
            e.setCellText((1, 0), lang.name())
            e.setColumnWidth(0, 56)

            e.setCellText((0, 1), "Codepage:")
            e.setCellText((1, 1), lang.codepage())
            e.setColumnWidth(1, 56)

            translated = {}
            untranslated = {}

            lang.splitDicts(untranslated, translated)

            flow = [ (untranslated, "> Untranslated"), (translated, "> Translated") ]
            pos = (0, 4)
            for f in flow:
                d = f[0]
                title = f[1]

                e.setCellText(pos, title)
                pos = (pos[0], pos[1] + 1)
                pairs = d.items()
                for pair in pairs:
                    key = strToExcel(pair[0])
                    value = strToExcel(pair[1])
                    refColumn[pair[0]] = e.cellName((pos[0] + 1, pos[1]))

                    e.setCellText((pos[0], pos[1]), key)
                    e.setCellText((pos[0] + 1, pos[1]), value)
                    pos = (pos[0], pos[1] + 1)
                    if(index % 10 == 0):
                        progressCallback(float(index) / float(count))
                    index += 1 



    for lang in langs:
        keyLanguage = lang.name() == languageManager.keyLanguage().name()

        if not keyLanguage:
            e.setCurrentSheet(1)
            e.duplicateSheet(lang.name())

            e.setCellText((0, 0), "Language:")
            e.setCellText((1, 0), lang.name())
            e.setColumnWidth(0, 56)

            e.setCellText((0, 1), "Codepage:")
            e.setCellText((1, 1), lang.codepage())
            e.setColumnWidth(1, 56)

            translated = {}
            untranslated = {}
            lang.splitDicts(untranslated, translated)

            flow = [ (untranslated, "> Untranslated"), (translated, "> Translated") ]
            pos = (0, 4)

            for f in flow:
                d = f[0]
                title = f[1]
                pairs = d.items()

                e.setCellText(pos, title)
                pos = (pos[0], pos[1] + 1)
                for pair in pairs:
                    if pair[0] in refColumn:
                        key = u"=%s!%s" % (languageManager.keyLanguage().name(), refColumn[pair[0]])
                        value = pair[1]

                        e.setCellText((pos[0], pos[1]), strToExcel(key))
                        # e.lockCell(pos)
                        e.setCellText((pos[0] + 1, pos[1]), strToExcel(value))
                    else:
                        print "No cell with such key: %s" % pair[0]

                    pos = (pos[0], pos[1] + 1)
                    if(index % 10 == 0):
                        progressCallback(float(index) / float(count))
                    index += 1

    e.removeSheet(1)
    e.setCurrentSheet(1)

    e.show()
