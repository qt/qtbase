TEMPLATE = subdirs

contains(S60_VERSION, 3.1)|contains(S60_VERSION, 3.2)|contains(S60_VERSION, 5.0) {
    SUBDIRS += 3_1 3_2
}

# Symbian3 builds the default plugin for winscw so it is always needed
SUBDIRS += symbian_3