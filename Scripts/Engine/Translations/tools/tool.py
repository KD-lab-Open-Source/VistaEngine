#!/usr/bin/env python
# vim:set foldmethod=marker:

import gtk
import gobject

import os
from os.path import abspath
from os.path import isdir
from os.path import isfile

import fnmatch
import re

from Dictionary import Dictionary
from Dictionary import unescape
from Dictionary import escape
from LanguageManager import LanguageManager
from DictionaryExcel import exportSingleSheetXLS as exportSingleXLS
from DictionaryExcel import importSingleSheetXLS as importSingleXLS

SOURCE_DIRS =  [
                'AI',
                'AttribEditor',
                'AttribEditor\\TreeEditors',
                'AttribEditor\\DLL',
                'SurMap5',
                'SurMap5\\TreeEditors',
                'Configurator',
                'TriggerEditor',
                'TriggerEditor\\Game',
                'TriggerEditor\\Main',
                'TriggerEditor\\Scale',
                'TriggerEditor\\TEEngine',
                'TriggerEditor\\Tree',
                'TriggerEditor\\Utils',
                'Game',
                'Terra',
                'Units',
                'Environment',
                'ht',
                'Network',
                'Water',
                'UserInterface',
                'UIEditor',
                'Physics',
                'Sound',
                'UserInterface',
                'Util',
                'Util\\Serialization',
                'Util\\Serialization\\lib'
                ]

KEYWORDS = ['serialize:3',
            'openBlock:2',
            'TRANSLATE',
            'REGISTER_ENUM:2',
            'REGISTER_ENUM_ENCLOSED:3',
            'BEGIN_ENUM_DESCRIPTOR:3',
            'REGISTER_CLASS:3',
            'REGISTER_CLASS_CONVERSION:3',
            'WRAP_LIBRARY:3']

TYPES = ['*.h', '*.cpp', '*.rc']

PO_DIR = '.'
TRANSLATIONS_DIR = '..'
ROOT = '..\\..\\..\\..'

POT_FILE = 'VistaEngine.pot'
pot_path = PO_DIR + "\\" + POT_FILE
vistaEngineScr_path = ROOT + "\\Scripts\\Content\\VistaEngine.Scr"

keywords = ""

for word in KEYWORDS:
    keywords += "--keyword=%s " % word
        
def fileIncludesString(filename, string):
    f = file(filename, 'r')
    for line in f:
        if line.find(string, 0, -1) != -1:
            return True
    return False

def readPot(fileName):#{{{

    pairs = []

    # print "\n** Reading PO Template %s" % fileName

    f = file(fileName, 'r')

    msgid = re.compile('^msgid "(.*)"$', re.M)
    msgstr = re.compile('^msgstr "(.*)"$', re.M)
    juststr = re.compile('^"(.*)"$', re.M)

    key = u""
    value = u""

    lastWasKey = True
    for line in f:
        result = msgid.match(line)
        if not result is None:
            key = unescape(unicode(result.groups()[0], 'utf-8'))
            lastWasKey = True
        else:
            result = msgstr.match(line)
            if not result is None:
                pairs += [ (key, unescape(result.groups()[0])) ]
                lastWasKey = False
            else:
                result = juststr.match(line)
                if not result is None:
                    if lastWasKey:
                        key += unescape(result.groups()[0])
                    else:
                        last_item = pairs[-1];
                        last_item = (last_item[0], last_item[1] + unescape(result.groups()[0]))

    print "Got %i messages" % len(pairs)
    return pairs
    #}}}

def extractStrings(dirs, outFileName, progressCallback):#{{{
    try:
        os.unlink(PO_DIR + '\\' + POT_FILE)
    except:
        pass

    f = file(outFileName, 'w')
    f.write("#\n")
    f.close()

    fileNames = []
    for d in dirs:
        path = ROOT + "\\" + d
        if isdir(path):
            # print "** Entering %s..." % d
            for mask in TYPES:
                files = fnmatch.filter(os.listdir(path), mask)
                for f in files:
                    filename = path + "\\" + f
                    if fileIncludesString(filename, "_VISTA_ENGINE_EXTERNAL_"):
                        fileNames += [filename]
        elif isfile(path):
                # print "\n** Listing %s..." % d
                if isValidSource(path):
                    fileNames += path

    def processCpp(sourceFileName):#{{{
        index = 0
        command = 'xgettext %s -c++ --from-code=cp1251 -o %s -' % (keywords, outFileName)
        (stdin, stdout) = os.popen2(command, 't')
        sourceFile = file(sourceFileName, 'r')
        for line in sourceFile:
            stdin.write(line)
        sourceFile.close()

        progressCallback(float(index) / float(count))
        index += 1
        stdin.close()
        for line in stdout:
            print line
        stdout.close()

        return readPot(outFileName)#}}}

    def processRC(sourceFileName):#{{{
        re_menu     = re.compile('^\s*(IDR_\w+)\s*MENU\s*$', re.M)
        re_string   = re.compile('^\s*(ID_\w+)\s*"(.*)"\s*$', re.M)

        re_begin    = re.compile('^\s*BEGIN\s*$', re.M)
        re_end      = re.compile('^\s*END\s*$', re.M)
        re_popup    = re.compile('^\s*POPUP\s*"(.*)"$', re.M)
        re_menuitem_text = re.compile('^\s*MENUITEM\s*"(.*)",\s*(ID_\w+)\s*$', re.M)

        f = file(sourceFileName, 'r')

        class Menu:
            def visit(self, visitor):
                visitor(self)
                for child in self.children:
                    child.visit(visitor)
            def __init__(self, parent, name = u"", path = u""):
                self.parent = parent
                self.path = path
                self.name = name
                self.children = []

            def __call__(self, line):
                result = re_popup.match(line)
                if not result is None:
                    text = unescape(unicode(result.groups()[0], 'windows-1251'))
                    if len(self.path):
                        path = self.path + u"\\" + text
                    else:
                        path = text
                    self.children += [ Menu(self, u"", path) ]
                    return self.children[-1]

                result = re_menuitem_text.match(line)
                if not result is None:
                    text = unescape(unicode(result.groups()[0], 'windows-1251'))
                    name = unicode(result.groups()[1], 'windows-1251')

                    position = text.find("\t")
                    if position != -1:
                        text = text[0:position]

                    print text
                    if len(self.path):
                        path = self.path + u"\\" + text
                    else:
                        path = text
                    self.children += [ Menu(self, name, path) ]
                    return self

                if not re_end.match(line) is None:
                    return self.parent

                return self

        class Root:

            def __init__(self):
                self.children = []
                self.strings = dict()
            def __call__(self, line):
                result = re_menu.match(line)
                if not result is None:
                    text = unicode(result.groups()[0], 'windows-1251')
                    self.children += [ Menu(self, u"", u"") ]
                    return self.children[-1]
                result = re_string.match(line)
                if not result is None:
                    name = unicode(result.groups()[0], 'windows-1251')
                    text = unescape(unicode(result.groups()[1], 'windows-1251'))
                    self.strings[name] = text
                return self
            def visit(self, visitor):
                for child in self.children:
                    child.visit(visitor)
            def generatePairs(self):
                pairs = []
                class Parser:
                    def __init__(self, pairs, strings): self.pairs = pairs; self.strings = strings
                    def __call__(self, item):
                        key = item.path
                        tooltip = self.strings.get(item.name, None)
                        if not tooltip is None:
                            pos = tooltip.find("\n")
                            if pos != -1:
                                tooltip = tooltip[pos:]
                                key += tooltip
                                print key
                        self.pairs += [ (key, u"") ]
                self.visit(Parser(pairs, self.strings))
                return pairs

        root = Root()
        cur = root

        for line in f:
            cur = cur(line)

        pairs = root.generatePairs()
        return pairs
    #}}}

    associations = dict()
    associations['.cpp'] = processCpp
    associations['.h']   = processCpp
    associations['.rc']  = processRC

    result = []

    count = len(fileNames)
    for sourceFileName in fileNames:

        extStart = sourceFileName.rfind(".")
        if(not extStart is None):
            fileExtension = sourceFileName[extStart:].lower()
            action = associations.get(fileExtension, None)
            if not action is None:
                print "* Extracting strings from %s" % sourceFileName
                result += action(sourceFileName)
                continue

        print " * Skipping %s (unknown type)" % sourceFileName;

    return result

#}}}

languageManager = LanguageManager(TRANSLATIONS_DIR, "Russian")

class ProgressWindow:
    def __init__(s, parent):
        s.window = gtk.Window()
        s.window.set_modal(True)
        s.window.set_border_width(4)
        s.window.set_title("Progress")
        s.window.set_default_size(200, 40)
        s.window.set_position(gtk.WIN_POS_CENTER)
        s.window.set_skip_taskbar_hint(True)

        
        s.progressLabel = gtk.Label("0.0")
        s.progressBar = gtk.ProgressBar()

        vbox = gtk.VBox()
        vbox.pack_start(s.progressLabel)
        vbox.pack_start(s.progressBar)
        s.window.add(vbox)

    def show(s):
        s.window.show_all()

    def hide(s):
        s.window.hide()

    def onProgress(s, progress):
        s.progressBar.set_fraction(float(progress))
        s.progressLabel.set_text("%.1f%%" % float(progress * 100.0))

        gtk.main_iteration()

class ToolWindow:
    def __init__(s):

        #s.dictionary = Dictionary()
        #s.dictionary.load("Template")
        #s.dictionary.importPot(pot_path)

        #d = Dictionary()
        #d.load("Template-my")
        #s.dictionary.mergeTranslations(d)

        s.createWindow()
        s.updateTable()
        #s.dictionary.importHeaders("..\\..\\..\\Content\\ParameterValue")
        #s.dictionary.importHeaders("..\\..\\..\\Content\\ParameterGroup")
        #s.dictionary.importHeaders("..\\..\\..\\Content\\ParameterFormula")

    def createButton(s, vbox, title, func):
        button = gtk.Button(title)
        # button.set_mnemonic(True)
        button.connect("clicked", func)
        vbox.pack_start(button, False, False)

    def createWindow(s):#{{{
        s.window = gtk.Window()
        s.window.connect("delete_event", gtk.main_quit)
        s.window.set_position(gtk.WIN_POS_CENTER)

        s.window.set_border_width(4)
        s.window.set_title("Translation Tool")
        s.window.set_default_size(600, 400)

        hbox = gtk.HBox(False, 4)

        s.model = gtk.ListStore(gobject.TYPE_BOOLEAN, str, int, int)
        s.table = gtk.TreeView(s.model)

        s.column_key = gtk.TreeViewColumn("Key", gtk.CellRendererToggle())
        s.table.append_column(s.column_key)

        s.column_language = gtk.TreeViewColumn("Language", gtk.CellRendererText(), text=1)
        s.column_language.set_sort_column_id(1)
        s.table.append_column(s.column_language)

        s.column_translated = gtk.TreeViewColumn("#Msgs", gtk.CellRendererText(), text=2)
        s.column_translated.set_sort_column_id(2)
        s.table.append_column(s.column_translated)

        s.column_codepage = gtk.TreeViewColumn("CodePage", gtk.CellRendererText(), text=3)
        s.column_codepage.set_sort_column_id(3)
        s.table.append_column(s.column_codepage)

        s.table.set_reorderable(True)
        s.table.set_search_column(0)

        scrolled = gtk.ScrolledWindow()
        scrolled.add(s.table)

        top_hbox = gtk.HBox()
        # combo = gtk.ComboBox()
        # top_hbox.pack_start(combo)

        left_vbox = gtk.VBox()
        left_vbox.pack_start(top_hbox, False, False)
        left_vbox.pack_start(scrolled, True, True)

        hbox.pack_start(left_vbox, True, True)

        vbox = gtk.VBox(False, 2)

        s.createButton(vbox, "_Parse Sources", s.onParseSources)
        #s.createButton(vbox, "_Add Language", s.onRevert)

        s.createButton(vbox, "_Revert", s.onRevert)
        s.createButton(vbox, "_Save All", s.onSaveAll)
        s.createButton(vbox, "Import _Dictionary", s.onImportDictionary)
        s.createButton(vbox, "_Import XLS", s.onImportAll)
        s.createButton(vbox, "_Export XLS", s.onExportAll)
        # s.createButton(vbox, "Export _English", s.onExportEnglish)

        hbox.pack_start(vbox, False, False)
        s.window.add(hbox)
        s.window.show_all()#}}}

    def updateTable(s): #{{{
        model = s.model

        model.clear()

        infos = languageManager.languagesInfo()
        for info in infos:
            i = (True, info[0], info[1], info[2])
            model.append(i)

        #it = model.append(('English', 2500, 1250))
        #}}}

    def run(s):#{{{
        gtk.main()
        #}}}


    #def onCreateTemplate(s, button):#{{{

    #    # dictionary.importHeaders("..\\..\\..\\Content\\ParameterValue")
    #    # dictionary.importHeaders("..\\..\\..\\Content\\ParameterGroup")
    #    # dictionary.importHeaders("..\\..\\..\\Content\\ParameterFormula")

    #    oldDictionary = Dictionary()
    #    oldDictionary.load("..\\..\\Dictionary")
    #    s.dictionary.mergeTranslations(oldDictionary)

    #    s.dictionary.save("Template-all")
    #    s.dictionary.wipeNonTranslatable()
    #    s.dictionary.save("Template")

    #    #}}}

    def onSaveAll(s, button):
        languageManager.saveAll()

    def onParseSources(s, button):
        wnd = ProgressWindow(s.window)
        wnd.show()
        pairs = extractStrings(SOURCE_DIRS, pot_path, wnd.onProgress)
        wnd.hide()
        message = "Merge results:\n\n"

        d = Dictionary()
        d.importPairs(pairs)
        d.importVistaEngineScr(vistaEngineScr_path)
        d.wipeNonTranslatable()

        langs = languageManager.languages()[:]
        print d.countTranslated()

        for lang in langs:
            print lang is langs[0]

        for lang in langs:
            newLang = d.clone()
            num = newLang.mergeTranslations(lang)
            message += "<b>%s</b>:\n" % lang.name()
            message += "\tTotal messages: <b>%i</b>\n" % len(newLang)
            message += "\tPreserved messages: <b>%i</b>\n" % num
            message += "\tLost translations: <b>%i</b>\n" % (lang.countTranslated() - num)
            message += "\tLeft untranslated: <b>%i</b>\n" % (len(newLang) - (newLang.countTranslated()))
            lang.setValues(newLang)

        message += "\n<b>Proceed with merge?</b>"

        msgbox = gtk.MessageDialog(type = gtk.MESSAGE_QUESTION, buttons = gtk.BUTTONS_OK_CANCEL)
        msgbox.set_markup(message)
        msgbox.set_position(gtk.WIN_POS_CENTER)

        if msgbox.run() == gtk.RESPONSE_OK:
            languageManager.setLanguages(langs)
            s.updateTable()

        msgbox.hide()


    def onRevert(s, button):
        languageManager.revert()
        s.updateTable()

    # def onImport(s, button):
    #     wnd = ProgressWindow()
    #     wnd.show()
    #     d = Dictionary()
    #     DictionaryExcel.importTemp(d, abspath(".\\translated.xls"), wnd.onProgress)
    #     print s.dictionary.mergeTranslations(d)
    #     wnd.hide()
    #     s.dictionary.save("Template")

    # def onExportEnglish(s, button):
    #     wnd = ProgressWindow()
    #     wnd.show()
    #     DictionaryExcel.export(s.dictionary,
    #                            abspath(".\\Template.xls"),
    #                            abspath(".\\English.xls"),
    #                            wnd.onProgress)
    #     wnd.hide()

    def onExportEnglish(s, button):
        wnd = ProgressWindow(s.window)
        wnd.show()

        #exportSingleXLS(languageManager, wnd.onProgress, "English")

        wnd.hide()

    def selectedDictionaryName(s):
        treeselection = s.table.get_selection()
        result = "English"
        # treeselection.selected_for_each(retrieveSelectionLanguage, result)
        (model, iter) = treeselection.get_selected()
        if iter:
            return model.get(iter, 1)[0] # s.column_language
        else:
            return result

    def onImportDictionary(s, button):
        dlg = gtk.FileChooserDialog("Open..", s.window, gtk.FILE_CHOOSER_ACTION_OPEN);
        dlg.set_position(gtk.WIN_POS_CENTER_ON_PARENT);
        filter = gtk.FileFilter()
        filter.add_pattern("*")
        filter.set_name("Dictionary Files")
        dlg.set_filter(filter)
        dlg.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
        dlg.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)

        
        response = dlg.run()
        dlg.hide()
        if response == gtk.RESPONSE_OK:
            filename = dlg.get_filename()

            imported = Dictionary()
            imported.load(filename)
            languageName = s.selectedDictionaryName()
            print "Selected language: %s" % languageName
            dic = languageManager.languageByName(languageName)
            mergedCount = dic.mergeTranslations(imported)
            print "Translations merged: %i" % mergedCount
            s.updateTable()

    def onImportAll(s, button):
        dlg = gtk.FileChooserDialog("Open..", s.window, gtk.FILE_CHOOSER_ACTION_OPEN);
        dlg.set_position(gtk.WIN_POS_CENTER_ON_PARENT);
        filter = gtk.FileFilter()
        filter.add_pattern("*.xls")
        filter.set_name("XLS Files")
        dlg.set_filter(filter)
        dlg.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
        dlg.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
        if dlg.run() == gtk.RESPONSE_OK:
            dlg.hide()
            filename = dlg.get_filename()
            wnd = ProgressWindow(s.window)
            wnd.show()
            importSingleXLS(languageManager, filename, wnd.onProgress)
            wnd.hide()
        else:
            dlg.hide()

    def onExportAll(s, button):
        wnd = ProgressWindow(s.window)
        wnd.show()

        exportSingleXLS(languageManager, wnd.onProgress)

        wnd.hide()

def message(text):
    msgbox = gtk.MessageDialog(buttons = gtk.BUTTONS_OK)
    msgbox.set_title("Test")
    msgbox.set_position(gtk.WIN_POS_CENTER)
    msgbox.set_markup(text)
    msgbox.run()


wnd = ToolWindow()
wnd.run()

# message("Building...")
