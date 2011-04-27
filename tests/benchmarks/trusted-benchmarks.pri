# Edit the list of trusted benchmarks in each of the sub-targets

# command terminating newline in Makefile
NL=$$escape_expand(\\n\\t)

check-trusted.depends = qmake
for(benchmark, TRUSTED_BENCHMARKS) {
    check-trusted.commands += -cd $$benchmark && $(MAKE) -f $(MAKEFILE) check$${NL}
}

QMAKE_EXTRA_TARGETS += check-trusted
