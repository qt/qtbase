# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import importlib
import sys
import PySide2
from PySide2.QtCore import Signal
from PySide2.QtWidgets import QVBoxLayout, QRadioButton, QGroupBox, QWidget, QApplication, QPlainTextEdit, QHBoxLayout

import generate_testcase
from helpers import insert_testcases_into_file
from option_management import (Option, OptionManager, testcase_describing_options, function_describing_options,
                               skip_function_description, disabled_testcase_describing_options,
                               skip_testcase_description)


class MyRadioButton(QRadioButton):
    def __init__(self, value):
        super(MyRadioButton, self).__init__(text=str(value))
        self.value = value

        self.toggled.connect(lambda x: x and self.activated.emit(self.value))

    activated = Signal(object)


class OptionSelector(QGroupBox):
    def __init__(self, parent: QWidget, option: Option):
        super(OptionSelector, self).__init__(title=option.name, parent=parent)
        self.layout = QVBoxLayout()
        self.setLayout(self.layout)

        self.radiobuttons = []
        for val in option.possible_options:
            rb = MyRadioButton(val)
            self.layout.addWidget(rb)
            rb.activated.connect(lambda value: self.valueSelected.emit(option.name, value))
            self.radiobuttons.append(rb)

        self.radiobuttons[0].setChecked(True)

    valueSelected = Signal(str, object)


class OptionsSelector(QGroupBox):
    def __init__(self, parent: QWidget, option_manager: OptionManager):
        super(OptionsSelector, self).__init__(title=option_manager.name, parent=parent)
        self.vlayout = QVBoxLayout()
        self.setLayout(self.vlayout)
        self.layout1 = QHBoxLayout()
        self.layout2 = QHBoxLayout()
        self.layout3 = QHBoxLayout()
        self.vlayout.addLayout(self.layout1)
        self.vlayout.addLayout(self.layout2)
        self.vlayout.addLayout(self.layout3)
        self.disabledOptions = []

        self.selectors = {}
        for option in option_manager.options.values():
            os = OptionSelector(parent=self, option=option)
            if "type" in option.name:
                self.layout2.addWidget(os)
            elif "passing" in option.name:
                self.layout3.addWidget(os)
            else:
                self.layout1.addWidget(os)
            os.valueSelected.connect(self._handle_slection)
            self.selectors[option.name] = os

        self.selectedOptionsDict = {option.name: option.possible_options[0] for option in
                                    option_manager.options.values()}

    def get_current_option_set(self):
        return {k: v for k, v in self.selectedOptionsDict.items() if k not in self.disabledOptions}

    def _handle_slection(self, name: str, value: object):
        self.selectedOptionsDict[name] = value
        self.optionsSelected.emit(self.get_current_option_set())

    def set_disabled_options(self, options):
        self.disabledOptions = options
        for name, selector in self.selectors.items():
            if name in self.disabledOptions:
                selector.setEnabled(False)
            else:
                selector.setEnabled(True)

    optionsSelected = Signal(dict)


class MainWindow(QWidget):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.layout = QVBoxLayout()
        self.setLayout(self.layout)

        self.functionSelector = OptionsSelector(parent=self, option_manager=function_describing_options())
        self.layout.addWidget(self.functionSelector)
        self.testcaseSelector = OptionsSelector(parent=self, option_manager=testcase_describing_options())
        self.layout.addWidget(self.testcaseSelector)

        self.plainTextEdit = QPlainTextEdit()
        self.plainTextEdit.setReadOnly(True)
        self.layout.addWidget(self.plainTextEdit)
        self.plainTextEdit.setFont(PySide2.QtGui.QFont("Fira Code", 8))

        # temp
        self.functionSelector.optionsSelected.connect(lambda o: self._handle_function_change())
        self.testcaseSelector.optionsSelected.connect(lambda o: self._handle_testcase_change())

        self._handle_function_change()

    def _handle_function_change(self):
        options = self.functionSelector.get_current_option_set()
        if m := skip_function_description(options):
            self.plainTextEdit.setPlainText(m)
            return

        options_to_disable = disabled_testcase_describing_options(options)
        self.testcaseSelector.set_disabled_options(options_to_disable)

        options.update(self.testcaseSelector.get_current_option_set())
        if m := skip_testcase_description(options):
            self.plainTextEdit.setPlainText(m)
            return

        self._generate_new_testcase()

    def _handle_testcase_change(self):
        options = self.functionSelector.get_current_option_set()
        options.update(self.testcaseSelector.get_current_option_set())
        if m := skip_testcase_description(options):
            self.plainTextEdit.setPlainText(m)
            return

        self._generate_new_testcase()

    def _generate_new_testcase(self):
        foptions = self.functionSelector.get_current_option_set()
        toptions = self.testcaseSelector.get_current_option_set()
        importlib.reload(generate_testcase)
        testcase = generate_testcase.generate_testcase(foptions, toptions)
        self.plainTextEdit.setPlainText(testcase[1])
        filename = "../tst_qtconcurrentfiltermapgenerated.cpp"
        insert_testcases_into_file(filename, [testcase])
        filename = "../tst_qtconcurrentfiltermapgenerated.h"
        insert_testcases_into_file(filename, [testcase])


if __name__ == "__main__":
    app = QApplication(sys.argv)

    m = MainWindow()
    m.show()

    app.exec_()
